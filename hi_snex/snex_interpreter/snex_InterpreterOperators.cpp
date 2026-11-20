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
		opMode(Helpers::getOpType(v)),
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
		jassertfalse;
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

struct Subscript : public Expression
{
	SNEX_INTERPRETER_NODE(Subscript, WantsNode::No);

	Subscript(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  parentType (Helpers::getType(f, v, AstIds::ParentType)),
	  elementType(Helpers::getType(f, v, AstIds::ElementType)),
	  parent     (Helpers::getChildExpression(this, 0)),
	  index      (Helpers::getChildExpression(this, 1)),
	  f          (OpFunctions::Data::getFunctionPtr(elementType))
	{};

	VariableStorage getResult(Scope* s) override
	{
		return f.first(getLValue(s));
	}

	void assign(Scope* s, const VariableStorage& v) override
	{
		f.second(getLValue(s), v);
	}

	Scope::LValue& getLValue(Scope* s) override
	{
		auto dataPtr = reinterpret_cast<uint8*>(parent->getResult(s).toPtr());
		auto elementSize = elementType.getRequiredByteSize();
		auto idx = index->getResult(s).toInt();
		address = { dataPtr, elementSize * idx };
		return address;
	}

	TypeInfo getTypeInfo() const override
	{
		return elementType;
	}

	Scope::LValue address;
	TypeInfo parentType;
	TypeInfo elementType;
	ExprPtr parent;
	ExprPtr index;
	OpFunctions::Data::Functions f;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
