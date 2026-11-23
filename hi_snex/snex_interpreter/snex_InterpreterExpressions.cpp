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
		objectType(Helpers::getObjectType(f, v))
	{
		Helpers::allocateReturnType(*this, returnData);

		if (v[AstIds::CallType].toString() == "MemberFunction")
		{
			objectType = Helpers::getType(f, v, AstIds::ObjectType).getComplexType();
			thisObj = Helpers::getChildExpression(this, 0);
			thisOffset = 1;
		}

		argRef.clear();
		int idx = 0;

		for (auto& a : fd.args)
		{
			argRef.setBit(idx++ + thisOffset, a.isReference());
			argTypePtrs.push_back(a.typeInfo.getTypedIfComplexType<ComplexType>());
		}

		numFormalArgTypes = (int)argTypePtrs.size();
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

	int thisOffset = 0;
	hise::VoiceBitMap<32> argRef;
	ExprPtr thisObj;
	heap<uint8> returnData;
	FunctionData fd;
	OpFunctions::Inliners::Prototype inliner;
	ComplexType* objectType;
	std::vector<ComplexType*> argTypePtrs;
	int numFormalArgTypes = 0;
};

template <typename RT> struct FunctionCallT0 : public FunctionCallBase
{
	FunctionCallT0(Factory& f, const ValueTree& v, const FunctionData& fd) :
		FunctionCallBase(f, v, fd)
	{}

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
		{
			OpFunctions::Inliners::Args a(*this);
			a.args = nullptr;
			a.argTypePtrs = this->argTypePtrs.data();
			a.numArgTypes = numFormalArgTypes;
			a.numArgs = 0;
			a.preallocatedReturnData = { returnData.begin(), (size_t)returnData.size() };
			a.typePtr = objectType;

			return inliner(a);
		}

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

		auto v1 = getArgs(s, thisOffset, arg1);

		if (inliner != nullptr)
		{
			OpFunctions::Inliners::Args a(*this);
			a.args = &v1;
			a.argTypePtrs = this->argTypePtrs.data();
			a.numArgTypes = numFormalArgTypes;
			a.numArgs = 1;
			a.preallocatedReturnData = { returnData.begin(), (size_t)returnData.size() };
			a.typePtr = objectType;

			return inliner(a);
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
		if (this->objectType != nullptr)
		{
			args.add(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, 0));
			// Member inliners need the receiver's storage, not a by-value dyn view.
			if (thisOffset != 0 && inliner != nullptr)
				argRef.setBit(0, true);
		}

		for (int i = 0; i < fd.args.size(); i++)
		{
			args.add(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, i + thisOffset));
		}

		executionType = inliner != nullptr ? ExecutionType::Inliner :
			args.size() < 4 ? ExecutionType::CallVariadic : ExecutionType::CallDynamic;
		jassert(args.size() == childStatements.size());
	};

	VariableStorage getResult(Scope* s) override
	{	
		for (int i = 0; i < args.size(); i++)
			args.set(i, getArgs(s, i, argExpressions[i].get()));

		switch (executionType)
		{
		case ExecutionType::Inliner:
		{
			OpFunctions::Inliners::Args a(*this);

			if(!returnData.isEmpty())
				a.preallocatedReturnData.referTo(returnData);

			a.argTypePtrs = this->argTypePtrs.data();
			a.numArgTypes = numFormalArgTypes;
			a.typePtr = objectType;
			a.args = args.getRawDataPointer();
			a.numArgs = args.size();

			return inliner(a);
		}
		case ExecutionType::CallVariadic:
		{
			if (fd.returnType.getType() == Types::ID::Void)
			{
				fd.callVoidDynamic(args.getRawDataPointer(), args.size());
				return VariableStorage();
			}
			else
			{
				return fd.callDynamic(args.getRawDataPointer(), args.size());
			}
		}
		case ExecutionType::CallDynamic:
		{
			auto f = (CallDynamicSignature)fd.function;
			return f(fd.object, args.getRawDataPointer(), args.size());
		}
		}
	}

	ExecutionType executionType;
	Array<VariableStorage> args;
	std::vector<ExprPtr> argExpressions;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
