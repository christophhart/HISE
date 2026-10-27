namespace hise { using namespace juce;



std::pair<var, DiagnosticBase::DiagnosticResult> HiseJavascriptEngine::RootObject::FunctionObject::invokeShadowParse(const Scope& s, const var::NativeFunctionArgs& args, Strictness lv) const
{
	std::pair<var, DiagnosticBase::DiagnosticResult> rv;

	if (auto sp = s.getShadowParser())
	{
		Range<int> r;

		if (auto bl = dynamic_cast<BlockStatement*>(body.get()))
			r = bl->getBlockCharIndexRange();

		auto scopeType = WeakCallbackHolder::CallableObject::CallScope::Unsafe;

		ObjectTypeMap::TypeScope ts(*sp, r, lv, scopeType);

		rv.second = ts.getCallscopeReport();

		if (ts.getCallscopeReport().shouldReport())
		{
			body->logDiagnostics(s, ts.getCallscopeReport());
		}
		
		auto obj = ts.getCurrentScopeObject();

		if (args.thisObject.isObject())
			obj->setProperty("this", args.thisObject);

		Array<var> thisArgs;

		for (int i = 0; i < parameters.size(); i++)
		{
			auto id = parameters[i];

			if (isPositiveAndBelow(i, args.numArguments))
				obj->setProperty(id, args.arguments[i]);
		}

		body.get()->perform(*ts.getCurrentScope(), &rv.first);
	}

	return rv;
}

struct HiseJavascriptEngine::RootObject::RegisterVarStatement : public Statement
{
	RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
#if HISE_SHADOW_PARSER_ENABLED

		if (s.isShadowParsing())
		{
			s.getShadowParserRootObject()->setProperty(name, initialiser->getResult(s));
			return ok;
		}

#endif

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


struct HiseJavascriptEngine::RootObject::RegisterName : public Expression
{
	RegisterName(const CodeLocation& l, const Identifier& n, VarRegister* rootRegister_, int indexInRegister_, var* data_, VarTypeChecker::VarTypes type_, const Identifier& nsId) noexcept :
	  Expression(l), 
	  rootRegister(rootRegister_),
	  indexInRegister(indexInRegister_),
      name(n),
#if ENABLE_SCRIPTING_SAFE_CHECKS
      type(type_),
#endif
	  data(data_),
	  namespaceId(nsId)
    {}

	var getResult(const Scope& s) const override
	{
#if HISE_SHADOW_PARSER_ENABLED

		if (s.isShadowParsing())
			return s.getShadowParserRootObject(namespaceId)->getProperty(name);

#endif

		return *data;
	}

	void assign(const Scope& s, const var& newValue) const override
	{
#if HISE_SHADOW_PARSER_ENABLED
		if (s.isShadowParsing())
		{
			s.getShadowParserRootObject(namespaceId)->setProperty(name, newValue);
			return;
		}
#endif

#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(type)
        {
            auto ok = VarTypeChecker::checkType(newValue, VarTypeChecker::WrappedType(type), true);
            
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

	Identifier namespaceId;
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
#if HISE_SHADOW_PARSER_ENABLED
		
		if (auto sp = s.getShadowParser())
		{
			Array<var> thisArgs;

			for (int i = 0; i < expectedNumArguments; i++)
				thisArgs.add(argumentList[i]->getResult(s));

			if (auto ptr = sp->getApiClassFromMap(apiClass->getObjectName()))
			{
				logDiagnostics(s, ptr->runTests(sp, functionName, thisArgs));
				return ptr->getReturnType(functionName, thisArgs);
			}

			jassertfalse;
			return var();
		}

#endif

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
            Argument(VarTypeChecker::WrappedType type_, const Identifier& id_):
              type(type_),
              id(id_)
            {};
            
            Argument(const Identifier& id_):
              type(VarTypeChecker::WrappedType()),
              id(id_)
            {};
            
            Argument():
              type(VarTypeChecker::WrappedType()),
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
                
                if((bool)type)
                    s << VarTypeChecker::getTypeName(type) << " ";
                
                s << id.toString();
                
                return s;
            }
            
            VarTypeChecker::WrappedType type;
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

		using Strictness = WeakCallbackHolder::CallableObject::StrictnessLevel;

		std::pair<var, DiagnosticBase::DiagnosticResult> invokeShadowParse(const Scope& s, const var::NativeFunctionArgs& args, StrictnessLevel lv)
		{
			std::pair<var, DiagnosticBase::DiagnosticResult> rv;

			if (auto st = s.getShadowParser())
			{
				auto scopeType = WeakCallbackHolder::CallableObject::CallScope::Safe;

				ObjectTypeMap::TypeScope ts(*st, body->getBlockCharIndexRange(), lv, scopeType);

				rv.second = ts.getCallscopeReport();

				auto obj = ts.getCurrentScopeObject();

				if (args.thisObject.isObject())
					obj->setProperty("this", args.thisObject);

				Array<var> thisArgs;

				for (int i = 0; i < args.numArguments; i++)
				{
					auto id = parameterNames[i].id;

					if (isPositiveAndBelow(i, args.numArguments))
						obj->setProperty(id, args.arguments[i]);
				}

				body.get()->perform(*ts.getCurrentScope(), &rv.first);
				
			}

			return rv;
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
        
        VarTypeChecker::WrappedType returnType;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object);
		JUCE_DECLARE_WEAK_REFERENCEABLE(Object);

	};

#if HISE_SHADOW_PARSER_ENABLED
	struct Definition : public Statement
	{
		Definition(const CodeLocation& l, Object::Ptr o) :
			Statement(l),
			obj(o)
		{}

		ResultCode perform(const Scope& s, var*) const override
		{
			if (s.isShadowParsing())
			{
				Array<var> args;

				for (int i = 0; i < obj->parameterNames.size(); i++)
				{
					auto p = obj->parameterNames[i];

					if ((bool)p.type)
						args.add(s.getShadowParser()->createAnonoymousDiagnosticObject(VarTypeChecker::getTypeName(p.type), "").get());
					else
						args.add(var());
				}

				var::NativeFunctionArgs a(var(), args.getRawDataPointer(), args.size());

				auto rv = obj->invokeShadowParse(s, a, WeakCallbackHolder::CallableObject::StrictnessLevel::AllowInit);

				if (rv.second.shouldReport())
					logDiagnostics(s, rv.second);

				if ((bool)obj->returnType)
				{
					auto ok = VarTypeChecker::checkType(rv.first, obj->returnType, true);

					if (ok.failed())
					{
						String msg;
						msg << "error at return value for inline function " << obj->name.toString() << ": ";
						msg << ok.getErrorMessage();

						auto dr = DiagnosticBase::DiagnosticResult::fail(msg)
							.withErrorCode(DiagnosticBase::DiagnosticResult::ErrorCode::TypeMismatch);

						logDiagnostics(s, dr);
					}
				}
			}
			else
			{
				// should never happen, only used in diagnostic mode
				jassertfalse;
			}

			return ResultCode::ok;
		}

		Object::Ptr obj;
	};
#endif

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
#if HISE_SHADOW_PARSER_ENABLED
			if (s.isShadowParsing())
			{
				Array<var> args;

				for (int i = 0; i < numArgs; i++)
				{
					args.add(parameterExpressions[i]->getResult(s));
				}

				auto lv = WeakCallbackHolder::CallableObject::StrictnessLevel::Unset;
				
				ObjectTypeMap::ScopedCallStack sc(s.currentShadowParser, this);

				auto rv = f->invokeShadowParse(s, var::NativeFunctionArgs(var(), args.getRawDataPointer(), args.size()), lv);

				if (rv.second.shouldReport())
					logDiagnostics(s, rv.second);

				return rv.first;
			}
#endif

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

		var getResult(const Scope& s) const override 
		{
#if HISE_SHADOW_PARSER_ENABLED
			if (s.isShadowParsing())
			{
				auto id = f->parameterNames[index];
				return s.scope->getProperty(id);
			}
#endif

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
#if HISE_SHADOW_PARSER_ENABLED
		if (s.isShadowParsing())
		{
			s.scope->setProperty(name, initialiser->getResult(s));
			return ok;
		}
#endif

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

	var getResult(const Scope& s) const override
	{
#if HISE_SHADOW_PARSER_ENABLED
		if (s.isShadowParsing())
		{
			return s.scope->getProperty(id);
		}
#endif

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
