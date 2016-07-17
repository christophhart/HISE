
struct HiseJavascriptEngine::RootObject::RegisterVarStatement : public Statement
{
	RegisterVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		s.root->hiseSpecialData.varRegister.addRegister(name, initialiser->getResult(s));
		return ok;
	}

	Identifier name;
	ExpPtr initialiser;
};


struct HiseJavascriptEngine::RootObject::RegisterAssignment : public Expression
{
	RegisterAssignment(const CodeLocation &l, int registerId, ExpPtr source_) noexcept: Expression(l), registerIndex(registerId), source(source_) {}

	var getResult(const Scope &s) const override
	{
		var value(source->getResult(s));

		s.root->hiseSpecialData.varRegister.setRegister(registerIndex, value);
		return value;

	}

	int registerIndex;

	ExpPtr source;
};

struct HiseJavascriptEngine::RootObject::RegisterName : public Expression
{
	RegisterName(const CodeLocation& l, const Identifier& n, int registerIndex) noexcept : Expression(l), name(n), indexInRegister(registerIndex) {}

	var getResult(const Scope& s) const override
	{
		return s.root->hiseSpecialData.varRegister.getFromRegister(indexInRegister);
	}

	void assign(const Scope& s, const var& newValue) const override
	{
		s.root->hiseSpecialData.varRegister.setRegister(indexInRegister, newValue);
	}

	Identifier name;
	int indexInRegister;
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
		for (int i = 0; i < 4; i++)
		{
			results[i] = var::undefined();
			argumentList[i] = nullptr;
		}
	};

	var getResult(const Scope& s) const override
	{
		for (int i = 0; i < expectedNumArguments; i++)
		{
			results[i] = argumentList[i]->getResult(s);
		}

		CHECK_CONDITION(apiClass != nullptr, "API class does not exist");

		return apiClass->callFunction(functionIndex, results, expectedNumArguments);
	}

	const int expectedNumArguments;

	ExpPtr argumentList[4];
	const int functionIndex;
	mutable var results[4];

	const ReferenceCountedObjectPtr<ApiClass> apiClass;
};

struct HiseJavascriptEngine::RootObject::InlineFunction
{
	struct FunctionCall;

	struct Object : public DynamicObject,
					public DebugableObject
	{
	public:

		Object(Identifier &n, const Array<Identifier> &p) : 
			body(nullptr) ,
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
		}

		~Object()
		{
			parameterNames.clear();
			body = nullptr;
		}

		String getDebugValue() const override { return lastReturnValue.toString(); }

		/** This will be shown as name of the object. */
		String getDebugName() const override { return functionDef; }

		String getDebugDataType() const override { return DebugInformation::getVarType(lastReturnValue); }

		AttributedString getDescription() const override 
		{ 
			return DebugableObject::Helpers::getFunctionDoc(commentDoc, parameterNames); 
		}

		void setFunctionCall(const FunctionCall *e_)
		{
			e = e_;
		}

		Identifier name;
		Array<Identifier> parameterNames;
		typedef ReferenceCountedObjectPtr<Object> Ptr;
		ScopedPointer<BlockStatement> body;

		String functionDef;
		String commentDoc;

		var lastReturnValue = var::undefined();
		
		const FunctionCall *e;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)

	};

	struct FunctionCall : public Expression
	{
		FunctionCall(const CodeLocation &l, Object *referredFunction) : 
			Expression(l),
			f(referredFunction),
			numArgs(f->parameterNames.size())
		{
			for (int i = 0; i < numArgs; i++)
			{
				parameterResults.add(var::undefined());
			}
		};

		~FunctionCall()
		{
			f = nullptr;
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

			ResultCode c = f->body->perform(s, &returnVar);

			f->lastReturnValue = returnVar;

			if (c == Statement::returnWasHit) return returnVar;
			else return var::undefined();
		}

		Object::Ptr f;

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

		var getResult(const Scope&) const override   { return  (f->e->parameterResults.getUnchecked(index)); }

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


struct HiseJavascriptEngine::RootObject::CallbackParameterReference: public Expression
{
	CallbackParameterReference(const CodeLocation& l, var* data_) noexcept : Expression(l), data(data_) {}

	var getResult(const Scope& s) const override
	{
		return *data;
	}

	var* data;
};