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

	struct SnexCallWrapper : public ReferenceCountedObject
	{

	};

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

		var performDynamically(const Scope& s, const var* args, int numArgs)
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

struct HiseJavascriptEngine::RootObject::SnexDefinition : public Statement
{
	SnexDefinition(const CodeLocation& l, const String& code_, const Identifier& classId) noexcept : 
		Statement(l),
	    code(code_),
	    id(classId)
	{};

	SnexDefinition(const CodeLocation& l, const String& code_, const String& refFileName, bool isExternal):
		Statement(l),
		code(code_)
	{
		jassert(isExternal);

		id = File(refFileName).getFileNameWithoutExtension();
	}

	ResultCode perform(const Scope& s, var*) const override
	{
		s.root->hiseSpecialData.addSnexClass(s.root, code, id);
		return ok;
	}

	String code;
	Identifier id;
};

struct HiseJavascriptEngine::RootObject::SnexConstructor : public Expression
{
	SnexConstructor(const CodeLocation& l, JavascriptNamespace* ns_, const Identifier& classId_):
		Expression(l),
		ns(ns_),
		classId(classId_)
	{}

	var getResult(const Scope& s) const override
	{
		if (ns != nullptr)
		{
			var constructorArgs[4];

			int numArgs = jmin(4, arguments.size());

			for (int i = 0; i < numArgs; i++)
			{
				constructorArgs[i] = arguments[i]->getResult(s);
			}

			var thisObj;

			var::NativeFunctionArgs args(thisObj, constructorArgs, numArgs);
			
			if (ns->hasSnexClass(classId))
				return ns->getSnexStruct(classId)->create(args);

			location.throwError("Can't resolve SNEX struct " + classId);
		}

		location.throwError("Can't resolve namespace");

		RETURN_IF_NO_THROW(var());
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		location.throwError("assignments not allowed");
	}

	
	OwnedArray<Expression> arguments;
	JavascriptNamespace* ns;
	Identifier classId;
};

struct SnexScriptFunctionCall: public DynamicObject
{
	struct HashedFunctionTable
	{
		template <typename T> static constexpr snex::Types::ID id()
		{
			return snex::Types::Helpers::getTypeFromTypeId<T>();
		}

		struct Signature
		{
			Signature()
			{
				using namespace snex;

				returnType = Types::ID::Void;
				args[0] = Types::ID::Void;
				args[1] = Types::ID::Void;
			}

			Signature(const snex::jit::FunctionData& f)
			{
				using namespace snex;

				returnType = f.returnType.getType();

				args[0] = Types::ID::Void;
				args[1] = Types::ID::Void;

				int index = 0;

				for (const auto& a : f.args)
					args[index++] = a.typeInfo.getType();
			};

			String getId() const
			{
				String s;

				using namespace snex::Types;

				s << Helpers::getTypeChar(returnType);
				s << Helpers::getTypeChar(args[0]);
				s << Helpers::getTypeChar(args[1]);

				return s;
			}

			template <typename A1> void addArgs()
			{
				args[0] = id<A1>();
			}

			template <typename A1, typename A2> void addArgs()
			{
				args[0] = id<A1>();
				args[1] = id<A2>();
			}

			snex::Types::ID returnType;
			snex::Types::ID args[2];
			int numArgs = 0;
		};

		template <typename R> static Signature sig0()
		{
			Signature s;
			s.returnType = id<R>();
			s.numArgs = 0;
			return s;
		}

		template <typename R, typename... Args>static Signature sig()
		{
			Signature s;
			s.returnType = id<R>();
			s.numArgs = 0;
			s.addArgs<Args...>();
			return s;
		}

		void addv0()
		{
			map.set(sig0<void>().getId(), (void*)CallWrappers::cv0);
		}

		template <typename A> void addv1()
		{
			map.set(sig<void, A>().getId(), (void*)CallWrappers::cv1<A>);
		}

		template <typename A1, typename A2> void addv2()
		{
			map.set(sig<void, A1, A2>().getId(), (void*)CallWrappers::cv2<A1, A2>);
		}

		template <typename R> void addr0()
		{
			map.set(sig0<R>().getId(), (void*)CallWrappers::cr0<R>);
		}

		template <typename R, typename A1> void addr1()
		{
			map.set(sig<R, A1>().getId(), (void*)CallWrappers::cr1<R, A1>);
		}

		template <typename R, typename A1, typename A2> void addr2()
		{
			map.set(sig<R, A1, A2>().getId(), (void*)CallWrappers::cr2<R, A1, A2>);
		}

		bool injectFunctionPointer(snex::jit::FunctionData& f)
		{
			Signature s(f);

			if (map.contains(s.getId()))
			{
				f.function = map[s.getId()];
				return true;
			}

			return false;
		}

		HashedFunctionTable()
		{
			addv0();
			addv1<int>();				  addv1<double>();				   addv1<float>();
			addv2<int, int>();			  addv2<int, double>();			   addv2<int, float>();
			addv2<double, int>();		  addv2<double, double>();		   addv2<double, float>();
			addv2<float, int>();		  addv2<float, double>();		   addv2<float, float>();

			addr0<int>();				  addr0<double>();				   addr0<float>();
			addr1<int, int>();			  addr1<int, double>();			   addr1<int, float>();
			addr1<double, int>();		  addr1<double, double>();		   addr1<double, float>();
			addr1<float, int>();		  addr1<float, double>();		   addr1<float, float>();
			addr2<int, int, int>();		  addr2<int, int, double>();	   addr2<int, int, float>();
			addr2<int, double, int>();	  addr2<int, double, double>();    addr2<int, double, float>();
			addr2<int, float, int>();	  addr2<int, float, double>();	   addr2<int, float, float>();
			addr2<double, int, int>();	  addr2<double, int, double>();    addr2<double, int, float>();
			addr2<double, double, int>(); addr2<double, double, double>(); addr2<double, double, float>();
			addr2<double, float, int>();  addr2<double, float, double>();  addr2<double, float, float>();
			addr2<float, int, int>();	  addr2<float, int, double>();     addr2<float, int, float>();
			addr2<float, double, int>();  addr2<float, double, double>();  addr2<float, double, float>();
			addr2<float, float, int>();   addr2<float, float, double>();   addr2<float, float, float>();
		}

		HashMap<String, void*> map;
	};

	struct CallWrappers
	{
		static void cv0(void* s)
		{
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			binding->call(nullptr, 0);
		}

		template <typename R> static R cr0(void* s)
		{
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			return (R)binding->call(nullptr, 0);
		}

		template <typename A1> static void cv1(void* s, A1 a1)
		{
			var args[1] = { a1 };
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			binding->call(args, 1);
		}

		template <typename R, typename A1> static R cr1(void* s, A1 a1)
		{
			var args[1] = { a1 };
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			return (R)binding->call(args, 1);
		}

		template <typename A1, typename A2> static void cv2(void* s, A1 a1, A2 a2)
		{
			var args[1] = { a1 };
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			binding->call(args, 1);
		}

		template <typename R, typename A1, typename A2> static R cr2(void* s, A1 a1, A2 a2)
		{
			var args[1] = { a1 };
			auto binding = static_cast<SnexScriptFunctionCall*>(s);
			return (R)binding->call(args, 1);
		}
	};

	SnexScriptFunctionCall(snex::jit::FunctionData* nf, HiseJavascriptEngine::RootObject* root_, HiseJavascriptEngine::RootObject::InlineFunction::Object* if_) :
		root(root_),
		inlineFunction(if_),
		injectResult(Result::ok())
	{
		using namespace snex;
		using namespace snex::jit;

		nf->object = this;

		ScopedPointer<FunctionData> newFunction = nf;
		auto classId = nf->id.getParent();

		FunctionClass::Ptr fc = root->snexGlobalScope.getGlobalFunctionClass(classId);

		if (fc == nullptr)
		{
			fc = new FunctionClass(classId);
			root->snexGlobalScope.addFunctionClass(fc);
		}

		auto ok = functionTable->injectFunctionPointer(*newFunction);

		if (ok)
			fc->addFunction(newFunction.release());
		else
			injectResult = Result::fail("Can't find " + newFunction->getSignature() + " in hash table");
	};

	var invokeMethod(Identifier methodName, const var::NativeFunctionArgs& args) override
	{
		jassert(methodName == inlineFunction->name);
		
		return call(args.arguments, args.numArguments);
	}

	var call(const var* v, int numArgs)
	{
		HiseJavascriptEngine::RootObject::Scope s(nullptr, root, root);
		return inlineFunction->performDynamically(s, v, numArgs);
	}

	Result injectResult;
	SharedResourcePointer<HashedFunctionTable> functionTable;
	HiseJavascriptEngine::RootObject* root;
	WeakReference<HiseJavascriptEngine::RootObject::InlineFunction::Object> inlineFunction;
};

struct HiseJavascriptEngine::RootObject::SnexBinding : public Statement
{
	SnexBinding(const CodeLocation& l, const snex::jit::FunctionData& f_, ExpPtr cn, ExpPtr ife) :
		Statement(l),
		f(f_),
		className(cn),
		inlineFunctionExpression(ife)
	{
		f.object = this;
	}

	ResultCode perform(const Scope& s, var* v) const override
	{
		using namespace snex::jit;
		using namespace snex::Types;
		
		auto classId = className->getResult(s).toString();
		auto ifv = inlineFunctionExpression->getResult(s);

		if (classId.isEmpty())
			location.throwError("Can't resolve class id");

		if (auto i = dynamic_cast<InlineFunction::Object*>(ifv.getObject()))
		{
			auto newFunction = new FunctionData(f);
			newFunction->id = snex::NamespacedIdentifier(classId).getChildId(i->name);

			ScopedPointer<SnexScriptFunctionCall> newCallWrapper = new SnexScriptFunctionCall(newFunction, s.root, i);

			if (!newCallWrapper->injectResult.wasOk())
				location.throwError(newCallWrapper->injectResult.getErrorMessage());

			s.root->hiseSpecialData.inlineFunctionSnexBindings.add(newCallWrapper.release());
		}
		else
			location.throwError("Argument 2 is not a inline function");

		return ResultCode::ok;
	}

	ExpPtr inlineFunctionExpression;
	ExpPtr className;
	snex::jit::FunctionData f;
};

struct HiseJavascriptEngine::RootObject::SnexConfiguration : public Statement
{
	SnexConfiguration(const CodeLocation& l, Expression* c, Expression* a):
		Statement(l),
		command(c),
		argument(a)
	{}

	ResultCode perform(const Scope& s, var*) const override
	{
		auto cResult = command->getResult(s).toString();

		if (cResult.isEmpty())
			location.throwError("Expected String as first argument");

		static const Identifier optimization_("optimization");
		static const Identifier define_("define");

		Identifier cId(cResult);

		if (cId == optimization_)
		{
			var av = argument->getResult(s);

			if (auto list = av.getArray())
			{
				StringArray sa;

				for (auto& a : *list)
					sa.add(a.toString());

				auto allIds = snex::jit::OptimizationIds::getAllIds();

				s.root->snexGlobalScope.clearOptimizations();

				for (auto& o : sa)
				{
					if (!allIds.contains(o))
						location.throwError("Unknown optimization: " + o);

					s.root->snexGlobalScope.addOptimization(o);
				}
			}
		}
		if (cId == define_)
		{
			auto arg = argument->getResult(s);
			
			if (auto d = arg.getDynamicObject())
				s.root->snexGlobalScope.setPreprocessorDefinitions(arg);
			else
				s.root->snexGlobalScope.setPreprocessorDefinitions({});
		}
		
		return ok;
	}

	ExpPtr command;
	ExpPtr argument;
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
