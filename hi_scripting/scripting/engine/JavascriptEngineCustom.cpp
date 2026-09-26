namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::RegisterVarStatement : public Statement
{
	RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
        try
        {
            varRegister->addRegister(name, initialiser->getResult(s));
        }
        catch(String& error)
        {
            throw Error::fromLocation(location, error);
        }
		
		return ok;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; };

	VarRegister* varRegister = nullptr;

	Identifier name;
	ExpPtr initialiser;
};


struct HiseJavascriptEngine::RootObject::RegisterAssignment : public Expression
{
	RegisterAssignment(const CodeLocation &l, int registerId, ExpPtr source_) noexcept: Expression(l), registerIndex(registerId), source(source_) {}

	var getResult(const Scope &s) const override
	{
		var value(source->getResult(s));

		VarRegister* reg = &s.root->hiseSpecialData.varRegister;
		reg->setRegister(registerIndex, value);
		return value;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? source.get() : nullptr; };

	int registerIndex;

	ExpPtr source;
};

struct HiseJavascriptEngine::RootObject::RegisterName : public Expression
{
	RegisterName(const CodeLocation& l, const Identifier& n, VarRegister* rootRegister_, int indexInRegister_, var* data_, VarTypeChecker::VarTypes type_) noexcept :
	  Expression(l), 
	  rootRegister(rootRegister_),
	  indexInRegister(indexInRegister_),
      name(n),
#if ENABLE_SCRIPTING_SAFE_CHECKS
      type(type_),
#endif
	  data(data_)
      
    {}

	var getResult(const Scope& /*s*/) const override
	{
		return *data;
	}

	void assign(const Scope& /*s*/, const var& newValue) const override
	{
#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(type)
        {
            auto ok = VarTypeChecker::checkType(newValue, type, true);
            
            if(ok.failed())
            {
                throw Error::fromLocation(location, ok.getErrorMessage());
            }
        }
#endif
        
		*data = newValue;
	}

	Identifier getVariableName() const override { return name; }

	Statement* getChildStatement(int ) override { return nullptr; };

	VarRegister* rootRegister;
	int indexInRegister;
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    VarTypeChecker::VarTypes type;
#endif

	var* data;

	Identifier name;
};


struct HiseJavascriptEngine::RootObject::ApiConstant : public Expression
{
	ApiConstant(const CodeLocation& l) noexcept : Expression(l) {}
	var getResult(const Scope&) const override   { return value; }

	Statement* getChildStatement(int) override { return nullptr; };

	bool isConstant() const override { return true; }

	var value;
};

#if USE_BACKEND
/** Placeholder expression returned by parser soft-fail recovery in diagnostic mode.
    Carries the error message and fuzzy suggestions so the ApiValidationAnalyzer can
    collect them during its AST walk. Evaluates to var::undefined(). */
struct HiseJavascriptEngine::RootObject::DiagnosticPlaceholder : public Expression
{
	using Severity = ApiClass::DiagnosticResult::Severity;

	DiagnosticPlaceholder(const CodeLocation& l, const String& errorMessage,
	                      const StringArray& suggestions_ = {},
	                      Severity sev = Severity::Error)
		: Expression(l), message(errorMessage), suggestions(suggestions_), severity(sev) {}

	var getResult(const Scope&) const override { return var::undefined(); }

	Statement* getChildStatement(int) override { return nullptr; }

	String message;
	StringArray suggestions;
	Severity severity;
};
#endif

struct HiseJavascriptEngine::RootObject::ApiCall : public Expression
{
	struct DynamicCall
	{
		DynamicCall(const CodeLocation& l_, ApiClass* apiClass_, int expectedNumArguments_, int functionIndex_):
		  l(l_),
		  apiClass(apiClass_),
		  numArgs(expectedNumArguments_),
		  functionIndex(functionIndex_)
		{}

		var operator()(const var::NativeFunctionArgs& args)
		{
			if(args.numArguments != numArgs)
				throw Error::fromLocation(l, "Expected num arguments: " + String(numArgs));

			try
			{
				return apiClass->callFunction(functionIndex, const_cast<var*>(args.arguments), args.numArguments);
			}
			catch (String& error)
			{
				throw Error::fromLocation(l, error);
				RETURN_IF_NO_THROW(var());
			}
		}

	private:

		CodeLocation l;
		ReferenceCountedObjectPtr<ApiClass> apiClass;
		int functionIndex;
		int numArgs;
	};

	ApiCall(const CodeLocation &l, ApiClass *apiClass_, int expectedArguments_, int functionIndex, const VarTypeChecker::ParameterTypes& types_) noexcept:
	Expression(l),
		expectedNumArguments(expectedArguments_),
		functionIndex(functionIndex),
#if ENABLE_SCRIPTING_SAFE_CHECKS
        types(types_),
#endif
		apiClass(apiClass_)
	{
#if ENABLE_SCRIPTING_BREAKPOINTS
		static const Identifier cId("Console");
		isDebugCall = apiClass_->getInstanceName() == cId;

		if (isDebugCall)
		{
			int unused;
			l.fillColumnAndLines(unused, lineNumber, unused);
			callbackName = l.getCallbackName(true);
		}

#endif

		for (int i = 0; i < 5; i++)
		{
			argumentList[i] = nullptr;
		}
	};

	String getProfileName() const override
	{
		auto s = apiClass->getObjectName().toString();
		s << ".";
		s << functionName;
		s << "()";
		return s;
	}

	var getResult(const Scope& s) const override
	{
#if JUCE_ENABLE_AUDIO_GUARD
        const bool allowIllegalCalls = apiClass->allowIllegalCallsOnAudioThread(functionIndex);
		AudioThreadGuard::Suspender suspender(allowIllegalCalls);
#endif

		var results[5];
		for (int i = 0; i < expectedNumArguments; i++)
		{
			results[i] = argumentList[i]->getResult(s);

#if ENABLE_SCRIPTING_SAFE_CHECKS
			HiseJavascriptEngine::checkValidParameter(i, results[i], argumentList[i]->location, types[i]);
#endif
		}

		CHECK_CONDITION_WITH_LOCATION(apiClass != nullptr, "API class does not exist");

		try
		{
#if ENABLE_SCRIPTING_BREAKPOINTS
			if (isDebugCall)
			{
				if (auto console = dynamic_cast<ScriptingApi::Console*>(apiClass.get()))
				{
					console->setDebugLocation(callbackName, lineNumber);
				}
			}
#endif

			return apiClass->callFunction(functionIndex, results, expectedNumArguments);
		}
		catch (String& error)
		{
			throw Error::fromLocation(location, error);
		}
	}

	Statement* getChildStatement(int index) override 
	{
		if (isPositiveAndBelow(index, expectedNumArguments))
			return argumentList[index].get();

		return nullptr;
	};

	bool isConstant() const override
	{
		if (!apiClass->isInlineableFunction(callbackName))
			return false;

		for (int i = 0; i < expectedNumArguments; i++)
		{
			if (!argumentList[i]->isConstant())
				return false;
		}

		return true;
	}

	bool replaceChildStatement(Ptr& newS, Statement* sToReplace) override
	{
		return  swapIf(newS, sToReplace, argumentList[0]) ||
				swapIf(newS, sToReplace, argumentList[1]) ||
				swapIf(newS, sToReplace, argumentList[2]) ||
				swapIf(newS, sToReplace, argumentList[3]) ||
				swapIf(newS, sToReplace, argumentList[4]);
	}

	const int expectedNumArguments;

	ExpPtr argumentList[5];
	const int functionIndex;
	
#if ENABLE_SCRIPTING_BREAKPOINTS
	bool isDebugCall = false;
	int lineNumber;
#endif
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    VarTypeChecker::ParameterTypes types;
#endif

	String functionName;
	Identifier callbackName;

	const ReferenceCountedObjectPtr<ApiClass> apiClass;
};


struct HiseJavascriptEngine::RootObject::ConstObjectApiCall : public Expression
{
	ConstObjectApiCall(const CodeLocation &l, var *objectPointer_, const Identifier& functionName_) noexcept:
	Expression(l),
		objectPointer(objectPointer_),
		functionName(functionName_),
		expectedNumArguments(-1),
		functionIndex(-1),
		initialised(false)
	{
		for (int i = 0; i < 4; i++)
		{
			argumentList[i] = nullptr;
		}
	};

	bool isConstant() const override
	{
		// this might be turned into a constant...
		jassertfalse;
		return false;
	}

	String getProfileName() const override
	{
		return functionName.toString() + "()";
	}

	var getResult(const Scope& s) const override
	{
		if (!initialised)
		{
			initialised = true;

			CHECK_CONDITION_WITH_LOCATION(objectPointer != nullptr, "Object Pointer does not exist");

			object = dynamic_cast<ConstScriptingObject*>(objectPointer->getObject());

			CHECK_CONDITION_WITH_LOCATION(object != nullptr, "Object doesn't exist");

#if ENABLE_SCRIPTING_SAFE_CHECKS
            forcedTypes = object->getForcedParameterTypes(functionIndex, expectedNumArguments);
#endif
            
			object->getIndexAndNumArgsForFunction(functionName, functionIndex, expectedNumArguments);

			CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function " + functionName.toString() + " not found.");
		}

		var results[5];

		for (int i = 0; i < expectedNumArguments; i++)
		{
			results[i] = argumentList[i]->getResult(s);
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
			HiseJavascriptEngine::checkValidParameter(i, results[i], location, forcedTypes[i]);
#endif
		}

		CHECK_CONDITION_WITH_LOCATION(object != nullptr, "Object does not exist");

		return object->callFunction(functionIndex, results, expectedNumArguments);
	}

	Statement* getChildStatement(int index) override
	{
		if (isPositiveAndBelow(index, 4))
			return argumentList[index].get();

		return nullptr;
	};

	bool replaceChildStatement(Ptr& newS, Statement* sToReplace) override
	{
		return  swapIf(newS, sToReplace, argumentList[0]) ||
				swapIf(newS, sToReplace, argumentList[1]) ||
				swapIf(newS, sToReplace, argumentList[2]) ||
				swapIf(newS, sToReplace, argumentList[3]);
	}

	mutable bool initialised;
	ExpPtr argumentList[4];
	mutable int expectedNumArguments;
	mutable int functionIndex;
	Identifier functionName;

#if ENABLE_SCRIPTING_SAFE_CHECKS
    mutable VarTypeChecker::ParameterTypes forcedTypes;
#endif
    
	var* objectPointer;

	mutable ReferenceCountedObjectPtr<ConstScriptingObject> object;
};

struct HiseJavascriptEngine::RootObject::IsDefinedTest : public Expression
{
	IsDefinedTest(const CodeLocation& l, Expression* expressionToTest) noexcept :
		Expression(l),
		test(expressionToTest)
	{}

	var getResult(const Scope& s) const override
	{
		auto result = test->getResult(s);

		if (result.isUndefined() || result.isVoid())
		{
			return var(false);
		}

		return var(true);
	}

	bool isConstant() const override { return test->isConstant(); }

	Statement* getChildStatement(int index) override { return index == 0 ? test.get() : nullptr; };

	ExpPtr test;
};

struct HiseJavascriptEngine::RootObject::InlineFunction
{
	struct FunctionCall;

	struct SnexCallWrapper : public ReferenceCountedObject
	{

	};

	struct Object : public DynamicObject,
					public DebugableObjectBase,
					public WeakCallbackHolder::CallableObject,
					public CyclicReferenceCheckBase,
                    public LocalScopeCreator
#if USE_BACKEND
					, public RealtimeSafetyInfo::Holder
#endif
	{
	public:

        struct Argument
        {
            Argument(VarTypeChecker::VarTypes type_, const Identifier& id_):
              type(type_),
              id(id_)
            {};
            
            Argument(const Identifier& id_):
              type(VarTypeChecker::Undefined),
              id(id_)
            {};
            
            Argument():
              type(VarTypeChecker::Undefined),
              id(Identifier())
            {};
            
            operator Identifier() const
            {
                return id;
            }
            
            bool operator==(const Argument& other) const
            {
                return id == other.id;
            }
            
            bool operator!=(const Argument& other) const
            {
                return id != other.id;
            }
            
            String toString() const
            {
                String s;
                
                if(type != VarTypeChecker::Undefined)
                    s << VarTypeChecker::getTypeName(type) << " ";
                
                s << id.toString();
                
                return s;
            }
            
            VarTypeChecker::VarTypes type;
            Identifier id;
        };
        
        using ArgumentList = Array<Argument>;
        
		Object(Identifier &n, const ArgumentList &p) :
			name(n)
		{
			parameterNames.addArray(p);

			functionDef = name.toString();
			functionDef << "(";

			for (int i = 0; i < parameterNames.size(); i++)
			{
				functionDef << parameterNames[i].toString();
				if (i != parameterNames.size()-1) functionDef << ", ";
			}

			functionDef << ")";

			CodeLocation lo = CodeLocation("", "");

			dynamicFunctionCall = new FunctionCall(lo, this, false);
		}

		~Object()
		{
			parameterNames.clear();
			body = nullptr;
			dynamicFunctionCall = nullptr;
		}

		Location getLocation() const override
		{
			return location;
		}

        DynamicObject::Ptr createScope(RootObject* r) override
        {
            DynamicObject::Ptr n = new DynamicObject();

            for (auto& v : *localProperties)
                n->setProperty(v.name, v.value);
            
            auto fToUse = e.get();
            
            if(fToUse == nullptr)
                fToUse = dynamicFunctionCall;
            
            if(fToUse != nullptr)
            {
                int index = 0;

                for (auto& p : parameterNames)
                    n->setProperty(p, fToUse->parameterResults[index++]);
            }
            
            return n;
        }
        
		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("InlineFunction"); }

		String getDebugValue() const override { return lastReturnValue->toString(); }

		/** This will be shown as name of the object. */
		String getDebugName() const override { return functionDef; }

		String getDebugDataType() const override { return "function"; }

		String getComment() const override { return commentDoc; }

		Identifier getCallId() const override { return name; }

		int getNumChildElements() const override
		{
			return ENABLE_SCRIPTING_BREAKPOINTS * 2;
		}

		DebugInformationBase* getChildElement(int index) override
		{
#if ENABLE_SCRIPTING_BREAKPOINTS
			WeakReference<Object> safeThis(this);

			auto vf = [safeThis, index]()
			{
				if (safeThis == nullptr)
					return var();

				SimpleReadWriteLock::ScopedReadLock s(safeThis->debugLock);
				return index == 1 ? safeThis->debugLocalProperties : safeThis->debugArgumentProperties;
			};

			String mId;
			mId << name << ".";

			mId << (index == 0 ? "args" : "locals");

			auto mi = new LambdaValueInformation(vf, Identifier(mId), {}, DebugInformation::Type::InlineFunction, location);
			mi->setAutocompleteable(false);

			return mi;
#else
			return nullptr;
#endif

		}

		void doubleClickCallback(const MouseEvent &/*event*/, Component* ed)
		{
			DebugableObject::Helpers::gotoLocation(ed, nullptr, location);
		}

		AttributedString getDescription() const override 
		{
            Array<Identifier> justIds;
            
            for(const auto& p: parameterNames)
                justIds.add((Identifier)p);
            
			return DebugableObject::Helpers::getFunctionDoc(commentDoc, justIds);
		}

		bool updateCyclicReferenceList(ThreadData& data, const Identifier &id) override;

		void prepareCycleReferenceCheck() override;

		void setFunctionCall(const FunctionCall *e_)
		{
			e.get() = e_;
		}

		void cleanUpAfterExecution()
		{
            cleanLocalProperties();
			setFunctionCall(nullptr);
		}

		int getNumArguments() const override { return parameterNames.size(); }

        bool isRealtimeSafe() const override { return true; }

#if USE_BACKEND
		mutable RealtimeSafetyInfo realtimeSafetyInfoData;

		RealtimeSafetyInfo* getRealtimeSafetyInfo() override
		{
			return &realtimeSafetyInfoData;
		}

		Identifier getCallScopeId() const override { return name; }
		Identifier getCallScopeType() const override { RETURN_STATIC_IDENTIFIER("InlineFunction"); }

		/** Defined out-of-line after CallScopeAnalyzer (needs full AST type definitions). */
		void performLazyCallScopeAnalysis() const;

		SafetyReport getRealtimeSafetyReport(StrictnessLevel strictness) const override
		{
			if (strictness <= StrictnessLevel::Unsafe)
				return {};

			if (!realtimeSafetyInfoData.analyzed && body != nullptr)
				performLazyCallScopeAnalysis();

			if (realtimeSafetyInfoData.isEmpty())
				return {};

			auto msg = realtimeSafetyInfoData.toString(strictness, nullptr);
			if (msg.isEmpty())
				return {};

			// Find worst (lowest enum value) scope across all warnings
			CallScope worst = CallScope::Safe;

			for (auto w : realtimeSafetyInfoData.items)
			{
				if ((int)w->scope < (int)worst)
					worst = w->scope;
			}

			return { worst, msg };
		}
#endif
        
		var createDynamicObjectForBreakpoint()
		{

			auto functionCallToUse = *e != nullptr ? *e : dynamicFunctionCall;

			if (functionCallToUse == nullptr)
				return var();

			auto object = new DynamicObject();

#if ENABLE_SCRIPTING_BREAKPOINTS
			auto arguments = new DynamicObject();
			
			for (int i = 0; i < parameterNames.size(); i++)
				arguments->setProperty(parameterNames[i], functionCallToUse->parameterResults[i]);

			object->setProperty("args", var(arguments));
			object->setProperty("locals", debugLocalProperties);
#endif

			return var(object);
		}

		var performDynamically(const Scope& s, const var* args, int numArgs)
		{
            LocalScopeCreator::ScopedSetter sls(s.root, this);
            
			setFunctionCall(dynamicFunctionCall);

#if ENABLE_SCRIPTING_BREAKPOINTS && 0
			if(numArgs != dynamicFunctionCall->parameterResults.size())
			{
				String e;
				e << "argument amount mismatch: " << String(dynamicFunctionCall->parameterResults.size()) << " (expected: " << String(numArgs) << ")";
				auto error = RootObject::Error::fromLocation(body->location, e);
				throw error;
			}
#endif

			auto numToSet = jmin(numArgs, dynamicFunctionCall->parameterResults.size());

			for (int i = 0; i < numToSet; i++)
			{
				dynamicFunctionCall->parameterResults.setUnchecked(i, args[i]);
			}

			Statement::ResultCode c = body->perform(s, &lastReturnValue.get());

            for (int i = 0; i < numToSet; i++)
            {
                dynamicFunctionCall->parameterResults.setUnchecked(i, {});
            }
            
			cleanUpAfterExecution();

			if (c == Statement::returnWasHit) return lastReturnValue.get();
			else return var::undefined();
		}

		void cleanLocalProperties()
		{
#if ENABLE_SCRIPTING_SAFE_CHECKS
			if (enableCycleCheck) // Keep the scope, don't mind the leaking...
				return;
#endif

#if ENABLE_SCRIPTING_BREAKPOINTS
			if (!localProperties->isEmpty())
			{
				DynamicObject::Ptr n = new DynamicObject();

				for (auto& v : *localProperties)
					n->setProperty(v.name, v.value);

				var nObj(n.get());

				{
					SimpleReadWriteLock::ScopedMultiWriteLock sl(debugLock);
					std::swap(nObj, debugLocalProperties);
				}
			}

			if (dynamicFunctionCall != nullptr && !parameterNames.isEmpty())
			{
				DynamicObject::Ptr obj = new DynamicObject();

				int index = 0;

				for (auto& p : parameterNames)
					obj->setProperty(p, dynamicFunctionCall->parameterResults[index++]);

				var nObj(obj.get());

				{
					SimpleReadWriteLock::ScopedMultiWriteLock sl(debugLock);
					std::swap(nObj, debugArgumentProperties);
				}
			}
#endif

			if (!localProperties->isEmpty())
			{
				for (int i = 0; i < localProperties->size(); i++)
					*localProperties->getVarPointerAt(i) = var();
			}
		}

		Identifier name;
		ArgumentList parameterNames;
		typedef ReferenceCountedObjectPtr<Object> Ptr;
		ScopedPointer<BlockStatement> body;

		String functionDef;
		String commentDoc;

        ThreadLocalValue<var> lastReturnValue;
		
		ThreadLocalValue<const FunctionCall*> e;

		ScopedPointer<FunctionCall> dynamicFunctionCall;

		ThreadLocalValue<NamedValueSet> localProperties;

#if ENABLE_SCRIPTING_BREAKPOINTS
		SimpleReadWriteLock debugLock;
		var debugArgumentProperties;
		var debugLocalProperties;
#endif

		bool enableCycleCheck = false;

		var lastScopeForCycleCheck;

		Location location;
        
        VarTypeChecker::VarTypes returnType;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object);
		JUCE_DECLARE_WEAK_REFERENCEABLE(Object);

	};

	struct FunctionCall : public Expression
	{
		FunctionCall(const CodeLocation &l, Object *referredFunction, bool storeReferenceToObject = true) :
			Expression(l),
			f(referredFunction),
			numArgs(f->parameterNames.size())
		{
			if (storeReferenceToObject)
			{
				referenceToObject = referredFunction;
			}

			for (int i = 0; i < numArgs; i++)
			{
                parameterResults.add({});
			}
		};

		~FunctionCall()
		{
			f = nullptr;
			referenceToObject = nullptr;
		}

		void addParameter(Expression *e)
		{
			parameterExpressions.add(e);
		}

		String getProfileName() const override
		{
			return referenceToObject->name.toString() + "()";
		}

		var getResult(const Scope& s) const override
		{
			f->setFunctionCall(this);

            LocalScopeCreator::ScopedSetter svs(s.root, f);
            
			for (int i = 0; i < numArgs; i++)
			{
                auto v = parameterExpressions.getUnchecked(i)->getResult(s);
				parameterResults.setUnchecked(i, v);
                
#if ENABLE_SCRIPTING_SAFE_CHECKS
                if(auto et = f->parameterNames.getReference(i).type)
                {
                    auto ok = VarTypeChecker::checkType(v, f->parameterNames.getReference(i).type);
                    
                    if(!ok.wasOk())
                    {
                        f->setFunctionCall(nullptr);
                        location.throwError("Parameter #" + String(i) + ": " + ok.getErrorMessage());
                    }
                }
#endif
			}

			s.root->addToCallStack(f->name, &location);

			try
			{
				ResultCode c = f->body->perform(s, &returnVar);

				s.root->removeFromCallStack(f->name);

				if(f->e.get() == this)
					f->cleanUpAfterExecution();
				 
				f->lastReturnValue = returnVar;

				for (int i = 0; i < numArgs; i++)
				{
					parameterResults.setUnchecked(i, var());
				}

                var rv;
                
				if (c == Statement::returnWasHit)
                    rv = returnVar;
				
#if ENABLE_SCRIPTING_SAFE_CHECKS
                if(f->returnType)
                {
                    auto ok = VarTypeChecker::checkType(rv, f->returnType);
                    
                    if(ok.failed())
                    {
                        location.throwError("Return value: " + ok.getErrorMessage());
                    }
                }
#endif
                
                return rv;
			}
			catch (Breakpoint& bp)
			{
				if(bp.localScope == nullptr)
					bp.localScope = f->createDynamicObjectForBreakpoint().getDynamicObject();

				throw bp;
			}
			
			
		}

		Statement* getChildStatement(int index) override 
		{ 
			if(isPositiveAndBelow(index, parameterExpressions.size()))
				return parameterExpressions[index];
			
			return nullptr;
		};
		
		bool replaceChildStatement(Ptr& newS, Statement* sToReplace) override
		{
			return swapIfArrayElement(newS, sToReplace, parameterExpressions);
		}

		Object::Ptr referenceToObject;

		Object* f;

		OwnedArray<Expression> parameterExpressions;
		mutable Array<var> parameterResults;

		mutable var returnVar;

		const int numArgs;
	};

	struct ParameterReference : public Expression
	{
		ParameterReference(const CodeLocation &l, Object *referedFunction, int id):
			Expression(l),
			index(id),
			f(referedFunction)
		{}

		~ParameterReference()
		{
			f = nullptr;
		}

		Identifier getVariableName() const override
		{
			return f->parameterNames[index];
		}

		var getResult(const Scope&) const override 
		{
			if (f->e.get() != nullptr)
			{
				return  (f->e.get()->parameterResults[index]);
			}
			else
			{
				location.throwError("Accessing parameter reference outside the function call. The parameter is only valid during function execution.");
				RETURN_IF_NO_THROW({});
			}
		}

		Statement* getChildStatement(int) override { return nullptr; };

		Object* f;
		int index;
	};
};



struct HiseJavascriptEngine::RootObject::GlobalVarStatement : public Statement
{
	GlobalVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		s.root->hiseSpecialData.globals->setProperty(name, initialiser->getResult(s));
		return ok;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; };
	
	Identifier name;
	ExpPtr initialiser;
};

struct HiseJavascriptEngine::RootObject::GlobalReference : public Expression
{
	GlobalReference(const CodeLocation& l, DynamicObject *globals_, const Identifier &id_) noexcept : Expression(l), globals(globals_), id(id_) {}

	var getResult(const Scope& s) const override
	{
		return s.root->hiseSpecialData.globals->getProperty(id);
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		s.root->hiseSpecialData.globals->setProperty(id, newValue);
	}

	Statement* getChildStatement(int) override { return nullptr; };

	DynamicObject::Ptr globals;
	const Identifier id;

	int index;
};



struct HiseJavascriptEngine::RootObject::LocalVarStatement : public Expression
{
	LocalVarStatement(const CodeLocation& l, InlineFunction::Object* parentFunction_) noexcept : Expression(l), parentFunction(parentFunction_) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		parentFunction->localProperties->set(name, initialiser->getResult(s));
		return ok;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; };
	
	bool replaceChildStatement(Ptr& newS, Statement* sToReplace) override
	{
		return swapIf(newS, sToReplace, initialiser);
	}

	mutable InlineFunction::Object* parentFunction;
	Identifier name;
	ExpPtr initialiser;
};



struct HiseJavascriptEngine::RootObject::LocalReference : public Expression
{
	LocalReference(const CodeLocation& l, InlineFunction::Object *parentFunction_, const Identifier &id_) noexcept : Expression(l), parentFunction(parentFunction_), id(id_) {}

	var getResult(const Scope& /*s*/) const override
	{
		return (parentFunction->localProperties.get())[id];
	}

	void assign(const Scope& /*s*/, const var& newValue) const override
	{
		parentFunction->localProperties->set(id, newValue);
	}

	Identifier getVariableName() const override { return id; }

	Statement* getChildStatement(int) override { return nullptr; };

	InlineFunction::Object* parentFunction;
	const Identifier id;

	int index;
};



struct HiseJavascriptEngine::RootObject::CallbackParameterReference: public Expression
{
	CallbackParameterReference(const CodeLocation& l, var* data_) noexcept : Expression(l), data(data_) {}

	var getResult(const Scope& /*s*/) const override
	{
		return *data;
	}

	Statement* getChildStatement(int) override { return nullptr; };

	var* data;
};

struct HiseJavascriptEngine::RootObject::CallbackLocalStatement : public Expression
{
	CallbackLocalStatement(const CodeLocation& l, Callback* parentCallback_) noexcept : Expression(l), parentCallback(parentCallback_) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		parentCallback->localProperties.set(name, initialiser->getResult(s));
		return ok;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; };
	
	mutable Callback* parentCallback;
	Identifier name;
	ExpPtr initialiser;
};

struct HiseJavascriptEngine::RootObject::CallbackLocalReference : public Expression
{
	CallbackLocalReference(const CodeLocation& l, Callback* parent_, const Identifier& name_) noexcept : 
	Expression(l), 
	parentCallback(parent_),
	name(name_)
	{}

	var getResult(const Scope& /*s*/) const override
	{
		return parentCallback->localProperties[name];
	}

	void assign(const Scope& /*s*/, const var& newValue) const
	{ 
		parentCallback->localProperties.set(name, newValue);
	}

	Identifier getVariableName() const override { return name; }


	Statement* getChildStatement(int) override { return nullptr; };

	Callback* parentCallback;
	Identifier name;

	CallbackLocalStatement* target;
};

struct ConstantFolding : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using Statement = HiseJavascriptEngine::RootObject::Statement;

	ConstantFolding()
	{}

	String getPassName() const override { return "Constant Folding"; };

	Statement* getOptimizedStatement(Statement* parent, Statement* statementToOptimize) override
	{
		if (statementToOptimize->isConstant() && dynamic_cast<HiseJavascriptEngine::RootObject::LiteralValue*>(statementToOptimize) == nullptr)
		{
			HiseJavascriptEngine::RootObject::Scope s(nullptr, nullptr, nullptr);
			auto immValue = dynamic_cast<HiseJavascriptEngine::RootObject::Expression*>(statementToOptimize)->getResult(s);
			return new HiseJavascriptEngine::RootObject::LiteralValue(statementToOptimize->location, immValue);
		}

		return statementToOptimize;
	}
};

struct LocationInjector : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using Statement = HiseJavascriptEngine::RootObject::Statement;

	LocationInjector()
	{}

	String getPassName() const override { return "Location Injector"; };

	Statement* getOptimizedStatement(Statement* parent, Statement* statementToOptimize) override
	{
		if (auto dot = dynamic_cast<HiseJavascriptEngine::RootObject::DotOperator*>(statementToOptimize))
		{
			if (auto cr = dynamic_cast<HiseJavascriptEngine::RootObject::ConstReference*>(dot->parent.get()))
			{
				HiseJavascriptEngine::RootObject::Scope s(nullptr, nullptr, nullptr);

				auto obj = cr->getResult(s);

				if (auto cso = dynamic_cast<ConstScriptingObject*>(obj.getObject()))
				{
					DebugableObjectBase::Location loc;
					loc.charNumber = dot->location.getCharIndex();
					loc.fileName = dot->location.externalFile;

					try
					{
						cso->addLocationForFunctionCall(dot->child, loc);
					}
					catch (String& e)
					{
						dot->location.throwError(e);
					}
					
				}
			}
		}

		return statementToOptimize;
	}
};

struct BlockRemover : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using Statement = HiseJavascriptEngine::RootObject::Statement;

	String getPassName() const override { return "Redundant StatementBlock Remover"; }

	Statement* getOptimizedStatement(Statement* parentStatement, Statement* statementToOptimize) override
	{
		if (auto sb = dynamic_cast<HiseJavascriptEngine::RootObject::BlockStatement*>(statementToOptimize))
		{
			if (sb->scopedBlockStatements.isEmpty())
			{
				if (sb->statements.isEmpty())
					return nullptr;

				if (sb->statements.size() == 1)
					return sb->statements.removeAndReturn(0);
			}
		}

		return statementToOptimize;
	}
};

struct FunctionInliner : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using Statement = HiseJavascriptEngine::RootObject::Statement;

	String getPassName() const override { return "Function inliner"; }

	Statement* getOptimizedStatement(Statement* parentStatement, Statement* statementToOptimize) override
	{
		if (auto apiCall = dynamic_cast<HiseJavascriptEngine::RootObject::ApiCall*>(statementToOptimize))
		{
			if (apiCall->isConstant())
			{
				jassertfalse;
				auto apiClass = apiCall->apiClass;
				auto fId = apiCall->callbackName;

				if (apiClass->isInlineableFunction(fId))
				{
					int numArgs, idx;
					apiClass->getIndexAndNumArgsForFunction(fId, idx, numArgs);

					HiseJavascriptEngine::RootObject::Scope s(nullptr, nullptr, nullptr);
					
					auto immValue = apiCall->getResult(s);

					return new HiseJavascriptEngine::RootObject::LiteralValue(apiCall->location, immValue);
				}
			}
		}

	return statementToOptimize;
	}
};

#if USE_BACKEND

bool isStaticRealtimeCallbackName(const Identifier& id)
{
	return id == Identifier("onNoteOn")
		|| id == Identifier("onNoteOff")
		|| id == Identifier("onController")
		|| id == Identifier("onTimer")
		|| id == Identifier("onVoiceStart")
		|| id == Identifier("onVoiceStop")
		|| id == Identifier("prepareToPlay")
		|| id == Identifier("processBlock");
}

struct CallScopeAnalyzer : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using RO = HiseJavascriptEngine::RootObject;
	using Statement = RO::Statement;
	using RealtimeSafetyInfo = RO::RealtimeSafetyInfo;
	using RealtimeSafetyWarning = RO::RealtimeSafetyWarning;
	using CallStackEntry = RO::CallStackEntry;
	using CodeLocation = RO::CodeLocation;

	String getPassName() const override { return "CallScope Analyzer"; }

	struct ScopedHolderSetter
	{
		ScopedHolderSetter(RO::OptimizationPass* a, RealtimeSafetyInfo::Holder* h):
			analyser(dynamic_cast<CallScopeAnalyzer*>(a)),
			prev(analyser != nullptr ? analyser->currentHolder : nullptr)
		{
			if(analyser != nullptr)
				analyser->setCurrentRealtimeInfoHolder(h);
		}

		~ScopedHolderSetter()
		{
			if(analyser != nullptr)
				analyser->setCurrentRealtimeInfoHolder(prev);
		}

	private:

		CallScopeAnalyzer* analyser;
		RealtimeSafetyInfo::Holder* prev;

		JUCE_DECLARE_NON_COPYABLE(ScopedHolderSetter);
	};

	void setCurrentRealtimeInfoHolder(RealtimeSafetyInfo::Holder* h)
	{
		currentHolder = h;

		if (currentHolder != nullptr)
		{
			if (auto* info = currentHolder->getRealtimeSafetyInfo())
			{
				info->items.clear();
				info->analyzed = true;
			}
		}
	}

	

	void handleChildIteration(Statement* statement, bool isBefore) override
	{
		auto* block = dynamic_cast<RO::BlockStatement*>(statement);
		if (block == nullptr)
			return;

		auto level = block->getSuppressLevel();
		if (level == ApiHelpers::CallScope::Safe)
			return;

		if (isBefore)
		{
			suppressStack.add(suppressLevel);
			suppressLevel = level;
		}
		else
		{
			suppressLevel = suppressStack.getLast();
			suppressStack.removeLast();
		}
	}

	Statement* getOptimizedStatement(Statement* parent, Statement* statementToOptimize) override
	{
		if (currentHolder == nullptr)
			return statementToOptimize;

		auto* info = currentHolder->getRealtimeSafetyInfo();
		if (info == nullptr)
			return statementToOptimize;

		// --- ApiCall: global API class method (Console.print, Engine.getSampleRate, etc.) ---
		if (auto* apiCall = dynamic_cast<RO::ApiCall*>(statementToOptimize))
		{
			auto className = apiCall->apiClass->getObjectName().toString();
			auto methodName = apiCall->functionName;

			if (methodName.isNotEmpty())
			{
				auto scopeInfo = ApiHelpers::getCallScope(className, methodName);
				addWarningIfNeeded(info, className + "." + methodName, scopeInfo,
				                   statementToOptimize->location);
			}
		}
		// --- ConstObjectApiCall: method on a const-declared scripting object ---
		else if (auto* constCall = dynamic_cast<RO::ConstObjectApiCall*>(statementToOptimize))
		{
			auto methodName = constCall->functionName.toString();
			String className;

			if (constCall->objectPointer != nullptr)
			{
				if (auto* obj = dynamic_cast<ConstScriptingObject*>(constCall->objectPointer->getObject()))
					className = obj->getObjectName().toString();
			}

			if (className.isNotEmpty() && methodName.isNotEmpty())
			{
				auto scopeInfo = ApiHelpers::getCallScope(className, methodName);
				addWarningIfNeeded(info, className + "." + methodName, scopeInfo,
				                   statementToOptimize->location);
			}
			else if (methodName.isNotEmpty())
			{
				// Object not resolved — fall back to greedy
				auto scopeInfo = ApiHelpers::getCallScope("*", methodName);
				addWarningIfNeeded(info, "*." + methodName, scopeInfo,
				                   statementToOptimize->location);
			}
		}
		// --- InlineFunction::FunctionCall: calling another inline function ---
		else if (auto* ilCall = dynamic_cast<RO::InlineFunction::FunctionCall*>(statementToOptimize))
		{
			inheritCalleeWarnings(info, ilCall);
		}
		// --- FunctionCall: generic dot-operator or bare function call ---
		else if (auto* funcCall = dynamic_cast<RO::FunctionCall*>(statementToOptimize))
		{
			if (auto* dot = dynamic_cast<RO::DotOperator*>(funcCall->object.get()))
			{
				auto methodName = dot->child.toString();
				auto scopeInfo = ApiHelpers::getCallScope("*", methodName);
				addWarningIfNeeded(info, "*." + methodName, scopeInfo,
				                   statementToOptimize->location);
			}
			// Bare FunctionCall without DotOperator (var holding a function) is skipped.
			// These are caught at registration time via isRealtimeSafe().
		}

		// Never replace statements — this is a read-only analysis pass
		return statementToOptimize;
	}

private:

	void addWarningIfNeeded(RealtimeSafetyInfo* info, const String& apiCall,
	                        const ApiHelpers::CallScopeInfo& scopeInfo,
	                        const CodeLocation& location)
	{
		using CS = ApiHelpers::CallScope;

		if (scopeInfo.scope == CS::Safe || scopeInfo.scope == CS::Unknown)
			return;

		// Check suppression threshold
		if (suppressLevel != CS::Safe && (int)scopeInfo.scope >= (int)suppressLevel)
			return;

		// Unsafe, Init, or Warning — add warning
		auto w = new RealtimeSafetyWarning();
		w->apiCall = apiCall;
		w->scope = scopeInfo.scope;
		w->note = scopeInfo.note;
		w->outerHolderType = currentHolder->getCallScopeType();

		CallStackEntry entry;
		entry.location = location;
		entry.functionName = currentHolder->getCallScopeId();
		w->callStack.add(entry);

		info->items.add(w);
	}

	void inheritCalleeWarnings(RealtimeSafetyInfo* info,
	                           RO::InlineFunction::FunctionCall* ilCall)
	{
		if (ilCall->f == nullptr)
			return;

		if (!ilCall->f->realtimeSafetyInfoData.analyzed && ilCall->f->body != nullptr)
			ilCall->f->performLazyCallScopeAnalysis();

		auto* calleeInfo = ilCall->f->getRealtimeSafetyInfo();
		if (calleeInfo == nullptr || calleeInfo->isEmpty())
			return;

		using CS = ApiHelpers::CallScope;

		for (auto item : calleeInfo->items)
		{
			// Check suppression threshold for inherited warnings
			if (suppressLevel != CS::Safe && (int)item->scope >= (int)suppressLevel)
				continue;

			auto inherited = item->clone();
			inherited->outerHolderType = currentHolder->getCallScopeType();

			auto* w = static_cast<RealtimeSafetyWarning*>(inherited.get());

			// Prepend the call site (where the inline function is called from)
			CallStackEntry callerEntry;
			callerEntry.location = ilCall->location;
			callerEntry.functionName = currentHolder->getCallScopeId();
			w->callStack.insert(0, callerEntry);

			// Deduplicate: skip if we already have a warning for the same API call at the same leaf location
			auto leafLoc = w->callStack.getLast().location.location;
			bool alreadyPresent = false;

			for (auto existing : info->items)
			{
				auto* ew = static_cast<RealtimeSafetyWarning*>(existing);

				if (ew->apiCall == w->apiCall
					&& !ew->callStack.isEmpty()
					&& ew->callStack.getLast().location.location == leafLoc)
				{
					alreadyPresent = true;
					break;
				}
			}

			if (alreadyPresent)
				continue;

			info->items.add(inherited);
		}
	}

	RealtimeSafetyInfo::Holder* currentHolder = nullptr;
	ApiHelpers::CallScope suppressLevel = ApiHelpers::CallScope::Safe;
	Array<ApiHelpers::CallScope> suppressStack;
};

/** AST analysis pass that validates method calls on const var objects and collects
    DiagnosticPlaceholder nodes left by the parser's diagnostic-mode recovery sites.
    
    Tier 2 (exact): FunctionCall + DotOperator + ConstReference → resolve ConstScriptingObject,
                    validate method existence, argument count, and literal argument types.
    Tier 3 (greedy): FunctionCall + DotOperator without ConstReference → check method name
                     against ALL known API classes via the enrichment ValueTree.
    
    This is a read-only pass — never replaces statements. */
struct ApiValidationAnalyzer : public HiseJavascriptEngine::RootObject::OptimizationPass
{
	using RO = HiseJavascriptEngine::RootObject;
	using Statement = RO::Statement;
	using ApiDiagnostic = RO::ApiDiagnostic;
	using CS = ApiClass::DiagnosticResult::Classification;
	using SV = ApiClass::DiagnosticResult::Severity;

	String getPassName() const override { return "API Validation Analyzer"; }

	/** Must be called before running the pass. */
	void setDiagnosticTarget(Array<ApiDiagnostic>* target, RO::HiseSpecialData* hsd)
	{
		diagnostics = target;
		hiseSpecialData = hsd;
	}

	// ==================== Diagnostic helpers ====================

	using DR = ApiClass::DiagnosticResult;

	struct ObjectTypeItem
	{
		RO::JavascriptNamespace* ns = nullptr;
		Identifier constRef;
		Identifier typeInfo;
        ReferenceCountedObjectPtr<DiagnosticBase> obj;
	};

    ObjectTypeItem getObjectTypeFromTypeMap(RO::ConstReference* ref) const
	{
		if (ref == nullptr || ref->ns == nullptr)
			return {};

		for (const auto& item : typeMap)
		{
			if (ref->ns.get() == item.ns && ref->getVariableName() == item.constRef)
				return item;
		}

		return {};
	}

	void setObjectTypeInTypeMap(RO::JavascriptNamespace* ns, const Identifier& constRef,
	                            const Identifier& typeInfo)
	{
		if (ns == nullptr || !typeInfo.isValid())
			return;

		for (auto& item : typeMap)
		{
			if (item.ns == ns && item.constRef == constRef)
			{
				item.typeInfo = typeInfo;
				return;
			}
		}

        auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(hiseSpecialData->processor);
        jassert(pwsc != nullptr);

        auto db = lightweightDiagnostics.createLightweightPrototype(pwsc, typeInfo);


		typeMap.add(ObjectTypeItem{ ns, constRef, typeInfo, db });
	}

	Array<ObjectTypeItem> typeMap;

	/** Run the QueryFunction for a method (if registered) and return the result.
	    Does NOT emit — the caller coalesces with other checks before emitting. */
	DR getQueryDiagnostic(DiagnosticBase* c, Statement* s, const Identifier& methodName)
	{
		if (!c->hasDiagnosticCheck(methodName))
			return DR::ok();

		Array<RO::Expression*> args;

		if (auto fc = dynamic_cast<RO::FunctionCall*>(s))
		{
			for (auto a : fc->arguments)
			{
				if (a->isConstant())
					args.add(a);
				else
					args.add(nullptr);
			}
		}
		if (auto ac = dynamic_cast<RO::ApiCall*>(s))
		{
			for (int i = 0; i < ac->expectedNumArguments; i++)
			{
				if (ac->argumentList[i]->isConstant())
					args.add(ac->argumentList[i].get());
				else
					args.add(nullptr);
			}
		}

		Array<var> diagnosticArgs;

		for (auto a : args)
		{
			if (a != nullptr)
				diagnosticArgs.add(a->getResult(RO::Scope(nullptr, nullptr, nullptr)));
			else
				diagnosticArgs.add(var());
		}

		return c->performDiagnostic(methodName, diagnosticArgs);
	}

	DR checkContentAddFunction(RO::FunctionCall* funcCall)
	{
		auto numArgs = funcCall->arguments.size();

		if (numArgs != 1)
		{
			String msg;
			msg << "omit the x, y arguments to avoid repositioning UI components after recompilation.";
			return DR::fail(msg);
		}

		return DR::ok();
	}

	/** Check argument count against expected. Returns Error on mismatch, OK otherwise. */
	DR checkArgCount(RO::FunctionCall* funcCall, int numExpected)
	{
		auto numActual = funcCall->arguments.size();

		if (numActual != numExpected)
		{
			String msg;
			msg << "expects " << numExpected << " argument"
			    << (numExpected != 1 ? "s" : "")
			    << ", got " << numActual;

			return DR::fail(msg);
		}

		return DR::ok();
	}

	/** Emit a single diagnostic with "ClassName.methodName - message" format. */
	void emitDiagnostic(const RO::CodeLocation& loc, const String& className,
	                    const Identifier& methodName, const DR& dr)
	{
		String msg;
		msg << className << "." << methodName.toString();

		auto errorMsg = dr.getErrorMessage();
		if (errorMsg.isNotEmpty())
			msg << " - " << errorMsg;

		addDiagnostic(loc, msg, dr.getSuggestions(), dr.getSeverity(), dr.getClassification());
	}

	/** Emit method-not-found with fuzzy suggestion. */
	void emitMethodNotFound(RO::FunctionCall* funcCall, ConstScriptingObject* cso,
	                        const String& className, const Identifier& methodName)
	{
		StringArray candidates;
		Array<Identifier> funcIds;
		cso->getAllFunctionNames(funcIds);
		for (auto& id : funcIds)
			candidates.add(id.toString());

		String msg = className + " has no method '" + methodName.toString() + "'";

		StringArray suggestions;
		auto suggestion = FuzzySearcher::suggestCorrection(methodName.toString(), candidates, 0.6);
		if (suggestion.isNotEmpty())
			suggestions.add(suggestion);

		addDiagnostic(funcCall->location, msg, suggestions, SV::Error, CS::ApiValidation);
	}

	// ==================== Main pass ====================

	Statement* getOptimizedStatement(Statement*, Statement* statementToOptimize) override
	{
		if (diagnostics == nullptr)
			return statementToOptimize;

		// DiagnosticPlaceholder nodes are NOT collected here — the parser already
		// recorded their diagnostics via recordDiagnostic() at each recovery site.
		// The placeholders exist in the AST to allow parsing to continue past errors.

		// --- Walk into anonymous function bodies ---
		// LiteralValue and AnonymousFunctionWithCapture wrap FunctionObject in a var,
		// making getChildStatement() return nullptr and blocking the tree walker.
		// We extract the FunctionObject and recursively walk its body so that
		// g.fillRect(...) etc. inside paint routines get validated.
		if (auto* lit = dynamic_cast<RO::LiteralValue*>(statementToOptimize))
		{
			if (auto* fo = dynamic_cast<RO::FunctionObject*>(lit->value.getDynamicObject()))
			{
				if (fo->body != nullptr)
				{
					clearAllPrototypeTouchedState();
					executePass(fo->body);
				}
			}
		}
		else if (auto* anon = dynamic_cast<RO::AnonymousFunctionWithCapture*>(statementToOptimize))
		{
			if (auto* fo = dynamic_cast<RO::FunctionObject*>(anon->function.getDynamicObject()))
			{
				if (fo->body != nullptr)
				{
					clearAllPrototypeTouchedState();
					executePass(fo->body);
				}
			}
		}

		// --- Tier 1: ApiCall (global API classes like Synth, Engine, Console) ---
		if (auto ac = dynamic_cast<RO::ApiCall*>(statementToOptimize))
		{
			auto queryDr = getQueryDiagnostic(ac->apiClass.get(), ac, ac->functionName);
			auto returnType = ac->apiClass->getReturnType(ac->functionName);

			statementToOptimize->setObjectTypeInformation(returnType);

			if (queryDr.shouldReport())
			{
				auto className = ac->apiClass->getObjectName().toString();
				emitDiagnostic(ac->location, className, ac->functionName, queryDr);
			}
		}

		if (auto cas = dynamic_cast<RO::ConstVarStatement*>(statementToOptimize))
		{
			// Top-level siblings are visited before their descendants, so infer the
			// initializer type here instead of waiting for the ApiCall visit.
			if (auto* ac = dynamic_cast<RO::ApiCall*>(cas->initialiser.get()))
			{
				auto returnType = ac->apiClass->getReturnType(ac->functionName);

				ac->setObjectTypeInformation(returnType);
				cas->setObjectTypeInformation(returnType);
				setObjectTypeInTypeMap(cas->ns, cas->name, returnType);
			}
            else
            {
                // argh, Content being a DynamicObject (wtf) needs a special route...
                std::pair<ScriptingApi::Content*, Identifier> rt;
                rt.first = nullptr;

                if(auto f = dynamic_cast<RO::FunctionCall*>(cas->initialiser.get()))
                {
                    if(auto dot = dynamic_cast<RO::DotOperator*>(f->object.get()))
                    {
                        if(auto uc = dynamic_cast<RO::UnqualifiedName*>(dot->parent.get()))
                        {
                            if(uc->name.toString() == "Content")
                            {
                                auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(hiseSpecialData->processor);
                                rt.first = pwsc->getScriptingContent();
                                rt.second = dot->child.toString();
                            }
                        }
                    }
                }

                if(rt.first != nullptr)
                {
                    auto returnType = rt.first->getReturnType(rt.second);
                    cas->setObjectTypeInformation(returnType);
                    setObjectTypeInTypeMap(cas->ns, cas->name, returnType);
                }
            }
		}

		// --- FunctionCall: check for DotOperator pattern ---
		if (auto* funcCall = dynamic_cast<RO::FunctionCall*>(statementToOptimize))
		{
			if (auto* dot = dynamic_cast<RO::DotOperator*>(funcCall->object.get()))
			{
				auto methodName = dot->child.toString();

				if (auto* constRef = dynamic_cast<RO::ConstReference*>(dot->parent.get()))
				{
					// --- Tier 2: Exact resolution via ConstReference ---
					validateTier2(funcCall, dot, constRef);
				}
				else
				{
					// --- Tier 3: Greedy lookup across all API classes ---
					validateTier3(funcCall, dot);
				}
			}
		}

		// Never replace — read-only analysis pass
		return statementToOptimize;
	}

private:

	void validateTier2(RO::FunctionCall* funcCall, RO::DotOperator* dot,
	                   RO::ConstReference* constRef)
	{
		if (constRef->ns == nullptr)
			return;

		auto constValue = constRef->ns->constObjects.getValueAt(constRef->index);
		auto* cso = dynamic_cast<ConstScriptingObject*>(constValue.getObject());
		auto typeMapType = getObjectTypeFromTypeMap(constRef);

		if (typeMapType.obj.get() != nullptr)
		{
			// Use the live object only when it agrees with the current source. This
			// preserves object-dependent diagnostics without trusting a stale value.
			if (cso != nullptr && cso->getObjectName() == typeMapType.typeInfo)
				validateWithKnownObject(funcCall, dot, cso);
			else
            {
                validateWithKnownTypeId(funcCall, dot, typeMapType);
            }


			return;
		}

		if (cso != nullptr)
		validateWithKnownObject(funcCall, dot, cso);
	}

	void validateWithKnownTypeId(RO::FunctionCall* funcCall, RO::DotOperator* dot,
	                             const ObjectTypeItem& resolvedType)
	{
		auto classNode = ApiHelpers::getApiTree().getChildWithName(resolvedType.typeInfo);

		if (!classNode.isValid())
			return;

		auto methodName = dot->child.toString();
		ValueTree methodNode;
		StringArray candidates;

        if(resolvedType.obj.get() != nullptr)
        {
            resolvedType.obj->markMethodTouched(methodName);
        }

		for (const auto& child : classNode)
		{
			auto candidate = child.getProperty("name").toString();

			if (candidate.isNotEmpty())
				candidates.add(candidate);

			if (candidate == methodName)
				methodNode = child;
		}

		if (!methodNode.isValid())
		{
			StringArray suggestions;
			auto suggestion = FuzzySearcher::suggestCorrection(methodName, candidates, 0.6);

			if (suggestion.isNotEmpty())
				suggestions.add(suggestion);

			String msg = resolvedType.typeInfo.toString() + " has no method '" + methodName + "'";
			addDiagnostic(funcCall->location, msg, suggestions, SV::Error, CS::ApiValidation);
			return;
		}

		auto argString = methodNode.getProperty("arguments").toString();
		auto argDr = checkArgCount(funcCall, GreedyApiMethodMap::countArgsFromString(argString));

        auto queryDr = getQueryDiagnostic(resolvedType.obj.get(), funcCall, methodName);

        auto best = DR::max(queryDr, argDr);

        if (best.shouldReport())
            emitDiagnostic(funcCall->location, resolvedType.typeInfo.toString(), methodName, best);
	}

	/** Tier 2 validation with a known ConstScriptingObject.
	    Coalesces QueryFunction and arg count checks via DR::max. */
	void validateWithKnownObject(RO::FunctionCall* funcCall, RO::DotOperator* dot,
	                             ConstScriptingObject* cso)
	{
		auto className = cso->getObjectName().toString();
		auto methodName = dot->child;

		int functionIndex = -1;
		int numArgs = 0;

		// Method existence — hard error, not coalesced
		if (!cso->getIndexAndNumArgsForFunction(methodName, functionIndex, numArgs))
		{
			emitMethodNotFound(funcCall, cso, className, methodName);
			return;
		}

		auto queryDr = getQueryDiagnostic(cso, funcCall, methodName);
		auto argDr   = checkArgCount(funcCall, numArgs);

		auto best = DR::max(queryDr, argDr);

		if (best.shouldReport())
			emitDiagnostic(funcCall->location, className, methodName, best);

		// Per-argument type check — independent, only when arg count is correct
		if (argDr.getSeverity() == DR::Severity::OK)
			checkLiteralArgTypes(funcCall, cso, functionIndex, numArgs, className, methodName.toString());
	}

	void validateTier3(RO::FunctionCall* funcCall, RO::DotOperator* dot)
	{
		auto methodName = dot->child.toString();

		auto& methodMap = getGreedyApiMethodMap();
		auto info = methodMap.find(methodName);

		if (info == nullptr)
			return; // Method not found in any API class — can't validate without knowing the object type

		

		// Method exists somewhere — check arg count if unambiguous
		if (info->ambiguousArgCount)
			return; // Different classes have different arg counts — can't validate

		auto objectKey = dot->parent->getProfileName();
		DR queryDr = DR::ok();

		if (auto* proto = getOrCreatePrototype(info->className, objectKey))
		{
			proto->markMethodTouched(Identifier(methodName));
			queryDr = getQueryDiagnostic(proto, funcCall, Identifier(methodName));
		}

		auto isContentAddFunction = info->className == Identifier("Content") 
								 && methodName.startsWith("add")
								 && methodName != "addVisualGuide";

		auto argDr = isContentAddFunction ? checkContentAddFunction(funcCall) : checkArgCount(funcCall, info->numArgs);

		// Tier 3 arg count is Warning (not Error) — less certainty without type info
		if (argDr.shouldReport())
			argDr = argDr.withSeverity(DR::Severity::Warning);



		auto best = DR::max(queryDr, argDr);

		if (best.shouldReport())
		{
			if (info->className.isValid())
			{
				emitDiagnostic(funcCall->location, info->className.toString(),
				               Identifier(methodName), best);
			}
			else
			{
				// Ambiguous class — fall back to generic format
				String msg = "Method '" + methodName + "'";
				auto errorMsg = best.getErrorMessage();
				if (errorMsg.isNotEmpty())
					msg << " - " << errorMsg;

				addDiagnostic(funcCall->location, msg, best.getSuggestions(),
				              best.getSeverity(), best.getClassification());
			}
		}

		// No type checking for Tier 3 — we don't know the concrete class
	}

	void checkLiteralArgTypes(RO::FunctionCall* funcCall, ConstScriptingObject* cso,
	                          int functionIndex, int numArgs,
	                          const String& className, const String& methodName)
	{
		auto types = cso->getForcedParameterTypes(functionIndex, numArgs);

		for (int i = 0; i < numArgs; i++)
		{
			auto expectedType = types[i];

			if (expectedType == VarTypeChecker::Undefined)
				continue; // No type constraint for this parameter

			var argValue;
			bool hasKnownType = false;

			if (auto* literal = dynamic_cast<RO::LiteralValue*>(funcCall->arguments[i]))
			{
				argValue = literal->value;
				hasKnownType = true;
			}
			else if (auto* constRef = dynamic_cast<RO::ConstReference*>(funcCall->arguments[i]))
			{
				if (constRef->ns != nullptr)
				{
					argValue = constRef->ns->constObjects.getValueAt(constRef->index);
					hasKnownType = true;
				}
			}

			if (!hasKnownType)
				continue; // Dynamic expression — skip type check

			auto actualType = VarTypeChecker::getType(argValue);

			if ((actualType & expectedType) == 0 && actualType != VarTypeChecker::Undefined)
			{
				auto expectedName = VarTypeChecker::getTypeName(expectedType);
				auto actualName = VarTypeChecker::getTypeName(actualType);

				String msg = className + "." + methodName + "() parameter " + String(i + 1)
				           + " requires " + expectedName + ", got " + actualName;

				addDiagnostic(funcCall->arguments[i]->location, msg, {}, SV::Error, CS::TypeCheck);
			}
		}
	}

	void addDiagnostic(const RO::CodeLocation& loc, const String& message,
	                   const StringArray& suggestions, SV severity, CS source)
	{
		ApiDiagnostic d;
		loc.fillColumnAndLines(d.col, d.line, d.charIndex);
		d.fileName = loc.externalFile;
		d.message = message;
		d.suggestions = suggestions;
		d.severity = severity;
		d.classification = source;
		diagnostics->add(d);
	}

	// ==================== Greedy API Method Map (Tier 3) ====================

	struct GreedyMethodInfo
	{
		int numArgs = 0;           // Consistent arg count (if not ambiguous)
		bool ambiguousArgCount = false;  // True if different classes disagree on arg count
		Identifier className;
	};

	struct GreedyApiMethodMap
	{
		HashMap<String, GreedyMethodInfo> map;
		StringArray allMethodNames;  // For fuzzy search
		bool built = false;

		GreedyMethodInfo* find(const String& methodName)
		{
			if (map.contains(methodName))
				return &map.getReference(methodName);
			return nullptr;
		}

		void buildFromTree(const ValueTree& apiTree)
		{
			for (const auto& classNode: apiTree)
			{
				auto parentClassName = classNode.getType();

				for (const auto& methodNode: classNode)
				{
					auto name = methodNode.getProperty("name").toString();

					if (name.isEmpty())
						continue;

					auto argStr = methodNode.getProperty("arguments").toString();
					int argCount = countArgsFromString(argStr);

					if (map.contains(name))
					{
						auto& existing = map.getReference(name);

						if(existing.className != parentClassName)
							existing.className = {};

						if (!existing.ambiguousArgCount && existing.numArgs != argCount)
							existing.ambiguousArgCount = true;
					}
					else
					{
						GreedyMethodInfo info;
						info.numArgs = argCount;
						info.className = parentClassName;
						allMethodNames.add(name);
						map.set(name, info);
					}
				}
			}

			built = true;
		}

		/** Parse an argument string like "(int x, double y)" to count parameters. */
		static int countArgsFromString(const String& argStr)
		{
			auto trimmed = argStr.trim();

			if (trimmed.isEmpty() || trimmed == "()")
				return 0;

			// Remove outer parens
			auto inner = trimmed.substring(1, trimmed.length() - 1).trim();

			if (inner.isEmpty())
				return 0;

			// Count commas + 1
			int count = 1;

			for (int i = 0; i < inner.length(); i++)
			{
				if (inner[i] == ',')
					count++;
			}

			return count;
		}
	};

	static GreedyApiMethodMap& getGreedyApiMethodMap()
	{
		static GreedyApiMethodMap instance;

		if (!instance.built)
			instance.buildFromTree(ApiHelpers::getApiTree());

		return instance;
	}

	// Contains prototypes for all tier 3 function call parents that can be resolved unambiguously
	// by their method name
	std::map<std::pair<Identifier, String>, ReferenceCountedObjectPtr<DiagnosticBase>> prototypeCache;

	DiagnosticBase* getOrCreatePrototype(const Identifier& className, const String& expression)
	{
		std::pair<Identifier, String> key = { className, expression };

		if (prototypeCache.find(key) != prototypeCache.end())
			return prototypeCache.at(key).get();

		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(hiseSpecialData->processor);
		jassert(pwsc != nullptr);

		auto n = ConstScriptingObject::createDiagnosticPrototype(className, pwsc);

		if(n != nullptr)
			prototypeCache[key] = n;

		return n.get();
	}

	void clearAllPrototypeTouchedState()
	{
		for (auto& pair : prototypeCache)
			pair.second->clearTouchedMethods();
	}

	Array<ApiDiagnostic>* diagnostics = nullptr;
	RO::HiseSpecialData* hiseSpecialData = nullptr;
    LightweightDiagnostics::Factory lightweightDiagnostics;
};

void HiseJavascriptEngine::RootObject::InlineFunction::Object::performLazyCallScopeAnalysis() const
{
	realtimeSafetyInfoData.analyzed = true;

	// Ensure callee inline functions are analyzed first (for transitive inheritance).
	OptimizationPass::callForEach(body, [](Statement* st) -> bool
	{
		if (auto* ilCall = dynamic_cast<InlineFunction::FunctionCall*>(st))
		{
			if (ilCall->f != nullptr && !ilCall->f->realtimeSafetyInfoData.analyzed
				&& ilCall->f->body != nullptr)
			{
				ilCall->f->performLazyCallScopeAnalysis();
			}
		}
		return false;
	});

	// Analyze using the same CallScopeAnalyzer as the optimization pass.
	CallScopeAnalyzer analyzer;
	analyzer.setCurrentRealtimeInfoHolder(const_cast<Object*>(this));
	analyzer.executePass(body);
}

#endif // USE_BACKEND

void HiseJavascriptEngine::RootObject::HiseSpecialData::registerOptimisationPasses()
{
	bool shouldOptimize = false;

#if USE_BACKEND

	auto enable = GET_HISE_SETTING(processor->mainController->getMainSynthChain(), HiseSettings::Scripting::EnableOptimizations).toString();
	
	shouldOptimize = enable == "1";

	optimizations.add(new LocationInjector());

	optimizations.add(new CallScopeAnalyzer());

#endif



	if (shouldOptimize)
	{
		optimizations.add(new ConstantFolding());
		optimizations.add(new BlockRemover());
		optimizations.add(new FunctionInliner());
	}

	
}

} // namespace hise
