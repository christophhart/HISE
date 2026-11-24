namespace snex {
namespace interpreter {
namespace nodes {

struct BinaryOp : public Expression
{
	SNEX_INTERPRETER_NODE(BinaryOp, WantsNode::No);

	BinaryOp(Factory& f, const ValueTree& v) :
		Expression(f, v),
		l(Helpers::getChildExpression(this, 0)),
		r(Helpers::getChildExpression(this, 1)),
		opMode(OpFunctions::BinaryOp::getOpType(v)),
		op(OpFunctions::BinaryOp::getFunctionPtr(getTypeInfo(), opMode))
	{
		jassert(op != nullptr);
	};

	VariableStorage getResult(Scope* s) override
	{
		if (op != nullptr)
		{
			auto lv = l->getResult(s);
			auto rv = r->getResult(s);
			return op(lv, rv);
		}

		String msg;
		msg << "Can't resolve binary op '";
		msg << astNode[AstIds::OpType].toString();
		msg << "' for type " + getTypeInfo().toString();

		throwRuntimeError(msg);
	}

	TypeInfo getTypeInfo() const override
	{
		return l->getTypeInfo();
	}

	Expression::ExprPtr l, r;
	const OpFunctions::BinaryOp::Ops opMode;
	OpFunctions::BinaryOp::Function op = nullptr;
};

struct Comparison : public BinaryOp
{
	SNEX_INTERPRETER_NODE(Comparison, WantsNode::No);

	Comparison(Factory& f, const ValueTree& v) :
		BinaryOp(f, v)
	{};
};

struct Increment : public Expression
{
	SNEX_INTERPRETER_NODE(Increment, WantsNode::No);

	Increment(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  isPre((bool)v[AstIds::IsPre]),
	  target(Helpers::getChildExpression(this, 0))
	{
		auto isDec = (bool)v[AstIds::IsDec];
		auto opMode = isDec ? OpFunctions::BinaryOp::Ops::SUB : OpFunctions::BinaryOp::Ops::ADD;
		op = OpFunctions::BinaryOp::getFunctionPtr(target->getTypeInfo(), opMode);
	};

	void assign(Scope* s, const VariableStorage& v) override
	{
		throwRuntimeError("Can't assign to increment operator");
	}

	VariableStorage getResult(Scope* s) override
	{
		auto prevValue = target->getResult(s);
		auto postValue = op(prevValue, 1);
		target->assign(s, postValue);
		return isPre ? postValue : prevValue;
	}

	TypeInfo getTypeInfo() const override
	{
		return target->getTypeInfo();
	}

	const bool isPre;
	ExprPtr target;
	OpFunctions::BinaryOp::Function op;
};

struct Cast : public Expression
{
	SNEX_INTERPRETER_NODE(Cast, WantsNode::No);

	Cast(Factory& f, const ValueTree& v) :
		Expression(f, v),
		targetType(Helpers::getType(f, v, AstIds::Target)),
		sourceValue(Helpers::getChildExpression(this, 0)),
		castFunction(OpFunctions::Cast::getFunctionPtr(targetType))
	{
		jassert(castFunction != nullptr);
	};

	VariableStorage getResult(Scope* s) override
	{
		auto v = sourceValue->getResult(s);
		return castFunction(v);
	}

	TypeInfo getTypeInfo() const override
	{
		return targetType;
	}

	TypeInfo targetType;
	ExprPtr sourceValue;
	OpFunctions::Cast::Function castFunction = nullptr;
};

struct LogicalNot : public Expression
{
	SNEX_INTERPRETER_NODE(LogicalNot, WantsNode::No);

	LogicalNot(Factory& f, const ValueTree& v) :
		Expression(f, v),
		value(Helpers::getChildExpression(this, 0))
	{};

	VariableStorage getResult(Scope* s) override
	{
		auto v = value->getResult(s).toInt();
		return VariableStorage(v == 0);
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(Types::ID::Integer);
	}

	ExprPtr value;
};

struct Subscript : public Expression
{
	SNEX_INTERPRETER_NODE(Subscript, WantsNode::No);

	Subscript(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  parentType (Helpers::getType(f, v, AstIds::ParentType)),
	  elementType(Helpers::getType(f, v, AstIds::ElementType)),
	  elementSize(elementType.getRequiredByteSize()),
	  isReference(elementType.isRef()),
	  isDyn      (parentType.getTypedComplexType<DynType>() != nullptr),
	  parent     (Helpers::getChildExpression(this, 0)),
	  index      (Helpers::getChildExpression(this, 1)),
	  f          (OpFunctions::Data::getFunctionPtr(elementType)),
	  customArgs(*this)
	{
		if (auto st = parentType.getTypedComplexType<StructType>())
		{
			customData.argTypes.push_back(st);
			customData.argTypes.push_back(nullptr);
			customFunction = OpFunctions::Inliners::getInliner(st, FunctionClass::SpecialSymbols::Subscript);

			customData.fillArgs(customArgs);

			if (customFunction == nullptr)
			{
				throwRuntimeError("unimplemented subscript inliner for " + parentType.toString());
			}
		}
	};

	VariableStorage getResult(Scope* s) override
	{
		if (customFunction != nullptr && !isReference)
			return callCustomSubscript(s);

		return f.first(getLValue(s));
	}

	void assign(Scope* s, const VariableStorage& v) override
	{
		f.second(getLValue(s), v);
	}

	Scope::LValue& getLValue(Scope* s) override
	{
		if (customFunction != nullptr)
		{
			if (!isReference)
				return Expression::getLValue(s);

			auto result = callCustomSubscript(s);
			if (result.getType() != Types::ID::Pointer)
				throwRuntimeError("subscript inliner did not return a reference");

			address = { result.getDataPointer(), 0 };
			return address;
		}

		auto parentValue = parent->getResult(s);
		void* dataPtr = nullptr;

		if (isDyn)
		{
			dataPtr = OpFunctions::Data::readDynView(parentValue).data;
		}
		else
		{
			dataPtr = parentValue.toPtr();
		}

		auto idx = index->getResult(s).toInt();
		address = { dataPtr, elementSize * idx };
		return address;
	}

	VariableStorage callCustomSubscript(Scope* s)
	{
		VariableStorage args[2];
		args[0] = parent->getResult(s);
		args[1] = index->getResult(s);

		customArgs.args = { args, 2 };
		return customFunction(customArgs);
	}

	TypeInfo getTypeInfo() const override
	{
		return elementType;
	}

	Scope::LValue address;
	TypeInfo parentType;
	TypeInfo elementType;
	const size_t elementSize;
	const bool isReference;
	const bool isDyn;
	ExprPtr parent;
	ExprPtr index;
	OpFunctions::Data::Functions f;

	OpFunctions::Inliners::ArgContainer customData;
	OpFunctions::Inliners::Args customArgs;
	OpFunctions::Inliners::Prototype customFunction = nullptr;
	
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
