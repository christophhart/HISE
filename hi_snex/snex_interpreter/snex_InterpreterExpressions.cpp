namespace snex {
namespace interpreter {

struct FunctionCallBase : public Expression
{
	static Identifier getStatementType() { RETURN_STATIC_IDENTIFIER("FunctionCall"); }
	WantsNode shouldCreateNode() const override { return WantsNode::No; };

	FunctionCallBase(Factory& f, const ValueTree& v, const FunctionData& fd_) :
		Expression(f, v),
		fd(fd_),
		inliner(OpFunctions::Inliners::getInliner(f, v)),
		inlinerArgs(*this)
	{
		inlinerData.allocateReturnType(*this);

		int idx = 0;

		if (v[AstIds::CallType].toString() == "MemberFunction")
		{
			auto ot = Helpers::getType(f, v, AstIds::ObjectType);
			
			argRef.setBit(idx++, ot.isRef());
			hasThisPointer = true;

			inlinerData.argTypes.push_back(ot.getComplexType());
			inlinerArgs.fillElementData(ot);
		}

		for (auto& a : fd.args)
		{
			argRef.setBit(idx++, a.isReference());
			inlinerData.argTypes.push_back(a.typeInfo.getTypedIfComplexType<ComplexType>());
		}

		inlinerData.fillArgs(inlinerArgs);
	};

	TypeInfo getTypeInfo() const override { return fd.returnType; }

	void resolveDataPointers(Scope* s) override
	{
		if (inliner != nullptr)
			return;

		if (fd.function == nullptr)
		{
			auto callType = Identifier(astNode[AstIds::CallType].toString());

			if (callType == Identifier("ApiFunction"))
			{
				jit::GlobalScope* gs = s->engine->getGlobalScope();

				auto thisLabel = fd.toLabel();

				for (const auto& p : gs->getMap())
				{
					if (p.label == thisLabel)
					{
						fd.function = p.function;
						return;
					}
				}
			}

			if (callType == Identifier("MemberFunction"))
			{
				auto objectType = inlinerData.argTypes[0];

				jassert(objectType != nullptr);
				auto mf = objectType->getNonOverloadedFunction(fd.id.id);

				if (!mf.isResolved())
					mf = s->engine->getFunction(fd.id);

				if (mf.isResolved())
				{
					fd = mf;
					if (fd.function != nullptr)
						return;
				}
			}

			if (callType == Identifier("RootFunction"))
			{
				auto rf = s->engine->getRootFunction(fd);

				if (rf.isResolved())
				{
					fd = rf;
					if (fd.function != nullptr)
						return;
				}
			}

			if (fd.inliner != nullptr)
			{
				auto needsInlinerFor = fd.getSignature();
				throwRuntimeError("unimplemented inliner " + needsInlinerFor);
			}

			throwRuntimeError("Unresolved function: " + fd.getSignature());
		}
	}

protected:

	VariableStorage getArgs(Scope* s, int argIndex, Expression* a)
	{
		if (argRef[argIndex])
		{
			auto lv = a->getLValue(s);
			auto ptr = (void*)(reinterpret_cast<uint8*>(lv.first) + lv.second);
			return VariableStorage(ptr);
		}
		else
		{
			return a->getResult(s);
		}
	}

	template <typename T> static juce_wchar getArgChar()
	{
		static constexpr Types::ID ty = Types::Helpers::getTypeFromTypeId<T>();
		return Types::Helpers::getTypeChar(ty);
	}

	hise::VoiceBitMap<32> argRef;
	FunctionData fd;

	OpFunctions::Inliners::ArgContainer inlinerData;
	OpFunctions::Inliners::Args inlinerArgs;
	OpFunctions::Inliners::Prototype inliner;

	bool hasThisPointer = false;
};

template <typename RT> struct FunctionCallT0 : public FunctionCallBase
{
	FunctionCallT0(Factory& f, const ValueTree& v, const FunctionData& fd) :
		FunctionCallBase(f, v, fd)
	{
		
	}

	static Statement::Ptr create(Factory& f, const ValueTree& v, const FunctionData& fd)
	{
		return new FunctionCallT0(f, v, fd);
	}

	static String getSigString()
	{
		String s;
		s << getArgChar<RT>();
		return s;
	}

	VariableStorage getResult(Scope* s) final override
	{
		if (inliner != nullptr)
			return inliner(inlinerArgs);

		if constexpr (std::is_same<void, RT>())
		{
			fd.callVoidUnchecked<>();
			return VariableStorage();
		}
		else
		{
			return VariableStorage(fd.callUnchecked<RT>());
		}
	}
};

template <typename RT, typename A1> struct FunctionCallT1 : public FunctionCallBase
{
	FunctionCallT1(Factory& f, const ValueTree& v, const FunctionData& fd):
	  FunctionCallBase(f, v, fd),
	  arg1(Helpers::getChildExpression(this, 0))
	{}

	static Statement::Ptr create(Factory& f, const ValueTree& v, const FunctionData& fd)
	{
		return new FunctionCallT1(f, v, fd);
	}

	static String getSigString()
	{
		String s;
		s << getArgChar<RT>();
		s << getArgChar<A1>();
		return s;
	}

	VariableStorage getResult(Scope* s) final override
	{
		static constexpr Types::ID T1 = Types::Helpers::getTypeFromTypeId<A1>();

		auto v1 = getArgs(s, hasThisPointer, arg1);

		if (inliner != nullptr)
		{
			inlinerArgs.args = { &v1, 1 };
			return inliner(inlinerArgs);
		}

		if constexpr (std::is_same<void, RT>())
		{
			fd.callVoid<A1>(v1.toType<T1>());
			return VariableStorage();
		}
		else
		{
			return VariableStorage(fd.call<RT, A1>(v1.toType<T1>()));
		}
	}

	ExprPtr arg1;
};

struct Statement::Factory::TypedFunctionFactory
{
	using CreateFunction = std::function<Statement::Ptr(Factory&, const ValueTree&, const FunctionData&)>;

	static Identifier getFunctionCallId() { RETURN_STATIC_IDENTIFIER("FunctionCall"); }

	std::map<String, CreateFunction> map;

	TypedFunctionFactory()
	{
		registerT0();
		registerT1<int>();
		registerT1<double>();
		registerT1<float>();
		registerT1<void*>();
	}

	template <typename F> void registerFunctionSignature()
	{
		map[F::getSigString()] = F::create;
	}

	void registerT0()
	{
		registerFunctionSignature<FunctionCallT0<void>>();
		registerFunctionSignature<FunctionCallT0<int>>();
		registerFunctionSignature<FunctionCallT0<double>>();
		registerFunctionSignature<FunctionCallT0<float>>();
		registerFunctionSignature<FunctionCallT0<void*>>();
	}

	template <typename A1> void registerT1()
	{
		registerFunctionSignature<FunctionCallT1<void, A1>>();
		registerFunctionSignature<FunctionCallT1<int, A1>>();
		registerFunctionSignature<FunctionCallT1<double, A1>>();
		registerFunctionSignature<FunctionCallT1<float, A1>>();
		registerFunctionSignature<FunctionCallT1<void*, A1>>();
	}

	Statement::Ptr createIfMapped(Statement::Factory& f, const ValueTree& v) const
	{
		if (v.getType() == getFunctionCallId())
		{
			auto fd = Helpers::getFunctionData(f, v);

			auto ti = Helpers::getType(f, v, AstIds::ObjectType);

			if (ti.isComplexType())
			{
				auto objectType = ti.getComplexType();
				auto addThisPointer = objectType != nullptr;
				auto sigString = OpFunctions::getSigString(fd, addThisPointer);

				if (map.find(sigString) != map.end())
					return map.at(sigString)(f, v, fd);
			}
		}

		return nullptr;
	}
};

Statement::Ptr Statement::Factory::create(const ValueTree& v)
{
	if (auto e = functionFactory->createIfMapped(*this, v))
		return e;

	if (createFunctions.find(v.getType()) != createFunctions.end())
	{
		if (auto t = createFunctions.at(v.getType())(*this, v))
			return t;
	}

	DBG("No node for " + v.getType().toString());

	DBG(v.createXml()->createDocument(""));
	jassertfalse;
	return nullptr;
}

namespace nodes {

struct FunctionCall : public FunctionCallBase
{
	SNEX_INTERPRETER_NODE(FunctionCall, WantsNode::No);

	enum class ExecutionType
	{
		CallVariadic, // <= 4 args
		CallDynamic,  // >  4 args,
		Inliner		  // forward raw args array
	};

	typedef VariableStorage(*CallDynamicSignature)(void* obj, VariableStorage*, int);

	FunctionCall(Factory& f, const ValueTree& v) :
		FunctionCallBase(f, v, Helpers::getFunctionData(f, v))
	{
		if (hasThisPointer)
		{
			inlinerData.args.push_back(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, 0));

			// Member inliners need the receiver's storage, not a by-value dyn view.
			if (inliner != nullptr)
				argRef.setBit(0, true);
		}

		for (int i = 0; i < fd.args.size(); i++)
		{
			inlinerData.args.push_back(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, i + (int)hasThisPointer));
		}

		executionType = inliner != nullptr ? ExecutionType::Inliner :
			inlinerData.args.size() < 4 ? ExecutionType::CallVariadic : ExecutionType::CallDynamic;

		jassert(inlinerData.args.size() == childStatements.size());

		inlinerData.fillArgs(inlinerArgs);
	};

	VariableStorage getResult(Scope* s) override
	{	
		for (int i = 0; i < inlinerData.args.size(); i++)
			inlinerData.args[i] = getArgs(s, i, argExpressions[i].get());

		switch (executionType)
		{
		case ExecutionType::Inliner:
		{
			return inliner(inlinerArgs);
		}
		case ExecutionType::CallVariadic:
		{
			if (fd.returnType.getType() == Types::ID::Void)
			{
				fd.callVoidDynamic(inlinerData.args.data(), inlinerData.args.size());
				return VariableStorage();
			}
			else
			{
				return fd.callDynamic(inlinerData.args.data(), inlinerData.args.size());
			}
		}
		case ExecutionType::CallDynamic:
		{
			auto f = (CallDynamicSignature)fd.function;
			return f(fd.object, inlinerData.args.data(), inlinerData.args.size());
		}
		}
	}

	ExecutionType executionType;
	std::vector<ExprPtr> argExpressions;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
