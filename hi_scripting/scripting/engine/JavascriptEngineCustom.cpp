namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::RegisterVarStatement : public Statement
{
	RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		varRegister->addRegister(name, initialiser->getResult(s));
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
	RegisterName(const CodeLocation& l, const Identifier& n, VarRegister* rootRegister_, int indexInRegister_, var* data_) noexcept : 
	  Expression(l), 
	  rootRegister(rootRegister_),
	  indexInRegister(indexInRegister_),
      name(n), 
	  data(data_) {}

	var getResult(const Scope& /*s*/) const override
	{
		return *data;
	}

	void assign(const Scope& /*s*/, const var& newValue) const override
	{
		*data = newValue;
	}

	Statement* getChildStatement(int ) override { return nullptr; };

	VarRegister* rootRegister;
	int indexInRegister;

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

struct HiseJavascriptEngine::RootObject::ApiCall : public Expression
{
	ApiCall(const CodeLocation &l, ApiClass *apiClass_, int expectedArguments_, int functionIndex) noexcept:
	Expression(l),
		expectedNumArguments(expectedArguments_),
		functionIndex(functionIndex),
		apiClass(apiClass_)
	{
#if ENABLE_SCRIPTING_BREAKPOINTS
		static const Identifier cId("Console");
		isDebugCall = apiClass_->getInstanceName() == cId;

		if (isDebugCall)
		{
			int unused;
			l.fillColumnAndLines(unused, lineNumber);
			callbackName = l.getCallbackName(true);
		}

#endif

		for (int i = 0; i < 5; i++)
		{
			argumentList[i] = nullptr;
		}
	};

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

			HiseJavascriptEngine::checkValidParameter(i, results[i], argumentList[i]->location);
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

	var getResult(const Scope& s) const override
	{
		if (!initialised)
		{
			initialised = true;

			CHECK_CONDITION_WITH_LOCATION(objectPointer != nullptr, "Object Pointer does not exist");

			object = dynamic_cast<ConstScriptingObject*>(objectPointer->getObject());

			CHECK_CONDITION_WITH_LOCATION(object != nullptr, "Object doesn't exist");

			object->getIndexAndNumArgsForFunction(functionName, functionIndex, expectedNumArguments);

			CHECK_CONDITION_WITH_LOCATION(functionIndex != -1, "function " + functionName.toString() + " not found.");
		}

		var results[5];

		for (int i = 0; i < expectedNumArguments; i++)
		{
			results[i] = argumentList[i]->getResult(s);

			HiseJavascriptEngine::checkValidParameter(i, results[i], location);
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
	{
	public:

		Object(Identifier &n, const Array<Identifier> &p) : 
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
			return DebugableObject::Helpers::getFunctionDoc(commentDoc, parameterNames); 
		}

		bool updateCyclicReferenceList(ThreadData& data, const Identifier &id) override;

		void prepareCycleReferenceCheck() override;

		void setFunctionCall(const FunctionCall *e_)
		{
#if SEE_YOU_LATER_BUDDY
			if (e_ != nullptr && e.get() == e_)
			{
				e_->location.throwError("Inline functions can't be called recursively");
			}
#endif

			e.get() = e_;
		}

		void cleanUpAfterExecution()
		{
            cleanLocalProperties();
			setFunctionCall(nullptr);
		}

        bool isRealtimeSafe() const override { return true; }
        
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

			for (int i = 0; i < numArgs; i++)
			{
				dynamicFunctionCall->parameterResults.setUnchecked(i, args[i]);
			}

			Statement::ResultCode c = body->perform(s, &lastReturnValue.get());

            for (int i = 0; i < numArgs; i++)
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
					SimpleReadWriteLock::ScopedWriteLock sl(debugLock);
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
					SimpleReadWriteLock::ScopedWriteLock sl(debugLock);
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
		Array<Identifier> parameterNames;
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

		var getResult(const Scope& s) const override
		{
			f->setFunctionCall(this);

            LocalScopeCreator::ScopedSetter svs(s.root, f);
            
			for (int i = 0; i < numArgs; i++)
			{
                auto v = parameterExpressions.getUnchecked(i)->getResult(s);
				parameterResults.setUnchecked(i, v);
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

				if (c == Statement::returnWasHit) return returnVar;
				else return var::undefined();
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
				location.throwError("Accessing parameter reference outside the function call");
				
				RETURN_DEBUG_ONLY(var());
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

struct HiseJavascriptEngine::RootObject::CallbackLocalStatement : public Statement
{
	CallbackLocalStatement(const CodeLocation& l, Callback* parentCallback_) noexcept : Statement(l), parentCallback(parentCallback_) {}

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
			if (sb->lockStatements.isEmpty())
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

void HiseJavascriptEngine::RootObject::HiseSpecialData::registerOptimisationPasses()
{
	bool shouldOptimize = false;

#if USE_BACKEND

	auto enable = GET_HISE_SETTING(processor->mainController->getMainSynthChain(), HiseSettings::Scripting::EnableOptimizations).toString();
	
	shouldOptimize = enable == "1";

	optimizations.add(new LocationInjector());

#endif



	if (shouldOptimize)
	{
		optimizations.add(new ConstantFolding());
		optimizations.add(new BlockRemover());
		optimizations.add(new FunctionInliner());
	}

	
}

} // namespace hise
