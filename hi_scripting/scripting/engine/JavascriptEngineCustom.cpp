namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::RegisterVarStatement : public Statement
{
	RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		varRegister->addRegister(name, initialiser->getResult(s));
		return ok;
	}

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

	VarRegister* rootRegister;
	int indexInRegister;

	var* data;

	Identifier name;
};


struct HiseJavascriptEngine::RootObject::ApiConstant : public Expression
{
	ApiConstant(const CodeLocation& l) noexcept : Expression(l) {}
	var getResult(const Scope&) const override   { return value; }

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

			HiseJavascriptEngine::checkValidParameter(i, results[i], location);
		}

		CHECK_CONDITION_WITH_LOCATION(apiClass != nullptr, "API class does not exist");

		try
		{
			return apiClass->callFunction(functionIndex, results, expectedNumArguments);
		}
		catch (String& error)
		{
			throw Error::fromLocation(location, error);
		}
	}

	const int expectedNumArguments;

	ExpPtr argumentList[5];
	const int functionIndex;
	

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

	ExpPtr test;
};

struct HiseJavascriptEngine::RootObject::InlineFunction
{
	struct FunctionCall;

	struct Object : public DynamicObject,
					public DebugableObjectBase,
					public CyclicReferenceCheckBase
	{
	public:

		Object(Identifier &n, const Array<Identifier> &p) : 
			name(n),
			e(nullptr)
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

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("InlineFunction"); }

		String getDebugValue() const override { return lastReturnValue.toString(); }

		/** This will be shown as name of the object. */
		String getDebugName() const override { return functionDef; }

		String getDebugDataType() const override { return DebugInformation::getVarType(lastReturnValue); }

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
			e = e_;
		}

		void cleanUpAfterExecution()
		{
			const int numArgs = dynamicFunctionCall->parameterResults.size();

			for (int i = 0; i < numArgs; i++)
			{
				dynamicFunctionCall->parameterResults.setUnchecked(i, var::undefined());
			}

			cleanLocalProperties();

			setFunctionCall(nullptr);
		}

		var createDynamicObjectForBreakpoint()
		{
			auto functionCallToUse = e != nullptr ? e : dynamicFunctionCall;

			if (functionCallToUse == nullptr)
				return var();

			DynamicObject::Ptr object = new DynamicObject();

			DynamicObject::Ptr arguments = new DynamicObject();
			
			for (int i = 0; i < parameterNames.size(); i++)
			{
				arguments->setProperty(parameterNames[i], functionCallToUse->parameterResults[i]);
			}

			DynamicObject::Ptr locals = new DynamicObject();

			for (int i = 0; i < localProperties.size(); i++)
			{
				locals->setProperty(localProperties.getName(i), localProperties.getValueAt(i));
			}

			object->setProperty("args", var(arguments));
			object->setProperty("locals", var(locals));

			return var(object);
		}

		var performDynamically(const Scope& s, var* args, int numArgs)
		{
			setFunctionCall(dynamicFunctionCall);

			for (int i = 0; i < numArgs; i++)
			{
				dynamicFunctionCall->parameterResults.setUnchecked(i, args[i]);
			}

			Statement::ResultCode c = body->perform(s, &lastReturnValue);

			cleanUpAfterExecution();

			if (c == Statement::returnWasHit) return lastReturnValue;
			else return var::undefined();
		}

		void cleanLocalProperties()
		{
#if ENABLE_SCRIPTING_SAFE_CHECKS
			if (enableCycleCheck) // Keep the scope, don't mind the leaking...
				return;
#endif

			if (!localProperties.isEmpty())
			{
				for (int i = 0; i < localProperties.size(); i++)
				{
					*localProperties.getVarPointerAt(i) = var();
				}
			}
		}

		Identifier name;
		Array<Identifier> parameterNames;
		typedef ReferenceCountedObjectPtr<Object> Ptr;
		ScopedPointer<BlockStatement> body;

		String functionDef;
		String commentDoc;

		var lastReturnValue = var::undefined();
		
		const FunctionCall *e;

		ScopedPointer<FunctionCall> dynamicFunctionCall;

		NamedValueSet localProperties;

		bool enableCycleCheck = false;

		var lastScopeForCycleCheck;

		Location location;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)

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
				parameterResults.add(var::undefined());
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

			for (int i = 0; i < numArgs; i++)
			{
				parameterResults.setUnchecked(i, parameterExpressions.getUnchecked(i)->getResult(s));
			}

			s.root->addToCallStack(f->name, &location);

			try
			{
				ResultCode c = f->body->perform(s, &returnVar);

				s.root->removeFromCallStack(f->name);

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

		var getResult(const Scope&) const override 
		{
			if (f->e != nullptr)
			{
				return  (f->e->parameterResults[index]);
			}
			else
			{
				location.throwError("Accessing parameter reference outside the function call");
				return var();
			}
		}

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

	DynamicObject::Ptr globals;
	const Identifier id;

	int index;
};



struct HiseJavascriptEngine::RootObject::LocalVarStatement : public Statement
{
	LocalVarStatement(const CodeLocation& l, InlineFunction::Object* parentFunction_) noexcept : Statement(l), parentFunction(parentFunction_) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		parentFunction->localProperties.set(name, initialiser->getResult(s));
		return ok;
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
		return parentFunction->localProperties[id];
	}

	void assign(const Scope& /*s*/, const var& newValue) const override
	{
		parentFunction->localProperties.set(id, newValue);
	}

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

	Callback* parentCallback;
	Identifier name;

	CallbackLocalStatement* target;
};

#if INCLUDE_NATIVE_JIT

struct HiseJavascriptEngine::RootObject::NativeJIT
{
	struct ProcessBufferCall : public Expression
	{
		ProcessBufferCall(const CodeLocation& l, NativeJITScope* scope_) : Expression(l), scope(scope_)
		{
			static const Identifier p("process");

#if JUCE_64BIT
			tickFunction = scope->getCompiledFunction<float, float>(p);
#endif
		}

		var getResult(const Scope& s) const override
		{
			if (tickFunction == nullptr)
			{
				location.throwError("float process(float input) function not defined");
			}

			var t = target->getResult(s);
			if (t.isBuffer())
			{
				VariantBuffer* b = t.getBuffer();

				float* data = b->buffer.getWritePointer(0);

				for (int i = 0; i < b->size; i++)
				{
					data[i] = tickFunction(data[i]);
				}
			}

			return var();
		}

		NativeJITScope::Ptr scope;

		ExpPtr target;

		float (*tickFunction)(float);
	};

	struct ScopeReference : public Expression
	{
		ScopeReference(const CodeLocation& l, NativeJITScope* scope_): Expression(l), scope(scope_)
		{}

		NativeJITScope::Ptr scope;
	};

	struct GlobalReference : public Expression
	{
		GlobalReference(CodeLocation& l, NativeJITScope* s) : Expression(l), scope(s) {};

		var getResult(const Scope&) const override
		{
			return scope->getGlobalVariableValue(index);
		}

		void assign(const Scope& /*s*/, const var& newValue) const
		{
			scope->setGlobalVariable(index, newValue);
		}

		NativeJITScope::Ptr scope;
		int index = -1;
	};

	struct GlobalAssignment : public Statement
	{
		GlobalAssignment(const CodeLocation& l) noexcept : Statement(l) 
		{}
		
		ResultCode perform(const Scope& s, var*) const 
		{ 
			scope->setGlobalVariable(id, expr->getResult(s));

			return Statement::ok;
		}

		NativeJITScope::Ptr scope;
		Identifier id;
		ExpPtr expr;
		
	};

	struct FunctionCall : public Expression
	{
		FunctionCall(const CodeLocation& l) noexcept : Expression(l) {}

		var getResult(const Scope& s) const override
		{
			var args[2];

			for (int i = 0; i < numArgs; i++)
			{
				args[i] = arguments[i]->getResult(s);
			}

			var::NativeFunctionArgs nArgs(var(), args, numArgs);

			return scope->invokeMethod(functionName, nArgs);
		}
		

		NativeJITScope::Ptr scope;

		OwnedArray<Expression> arguments;
		int numArgs;
		Identifier functionName;
	};
};

#endif


} // namespace hise
