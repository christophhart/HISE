namespace snex {
namespace interpreter {
namespace nodes {



struct BinaryOp : public Expression
{
	SNEX_INTERPRETER_NODE(BinaryOp, WantsNode::No);

	BinaryOp(Factory& f, const ValueTree& v) :
		Expression(f, v),
		l(Helpers::getChildExpression(this, 0)),
		r(Helpers::getChildExpression(this, 1))
	{
		juce::juce_wchar opMode = v[AstIds::OpType].toString()[0];
		Types::ID opType = l->getTypeInfo().getType();

		if (opType == Types::ID::Integer)
		{
			switch (opMode)
			{
			case '+': op = opFunction<Types::ID::Integer, '+'>; break;
			case '*': op = opFunction<Types::ID::Integer, '*'>; break;
			case '-': op = opFunction<Types::ID::Integer, '-'>; break;
			case '/': op = opFunction<Types::ID::Integer, '/'>; break;
			}
		}
		if (opType == Types::ID::Double)
		{
			switch (opMode)
			{
			case '+': op = opFunction<Types::ID::Double, '+'>; break;
			case '*': op = opFunction<Types::ID::Double, '*'>; break;
			case '-': op = opFunction<Types::ID::Double, '-'>; break;
			case '/': op = opFunction<Types::ID::Double, '/'>; break;
			}
		}
		if (opType == Types::ID::Float)
		{
			switch (opMode)
			{
			case '+': op = opFunction<Types::ID::Float, '+'>; break;
			case '*': op = opFunction<Types::ID::Float, '*'>; break;
			case '-': op = opFunction<Types::ID::Float, '-'>; break;
			case '/': op = opFunction<Types::ID::Float, '/'>; break;
			}
		}

		jassert(op != nullptr);
	};

	typedef VariableStorage(*OpFunction)(const VariableStorage&, const VariableStorage&);

	template <Types::ID T, char OpType> static VariableStorage opFunction(const VariableStorage& v1, const VariableStorage& v2)
	{
		switch (OpType)
		{
		case '+': return VariableStorage(v1.toType<T>() + v2.toType<T>());
		case '*': return VariableStorage(v1.toType<T>() * v2.toType<T>());
		case '-': return VariableStorage(v1.toType<T>() - v2.toType<T>());
		case '/': return VariableStorage(v1.toType<T>() / v2.toType<T>());
		}
	}

	VariableStorage getResult(Scope* s) override
	{
		auto lv = l->getResult(s);
		auto rv = r->getResult(s);
		return op(lv, rv);
	}

	TypeInfo getTypeInfo() const override
	{
		return l->getTypeInfo();
	}

	OpFunction op = nullptr;
	Expression::ExprPtr l, r;
	Types::ID type;
	
};

struct Cast : public Expression
{
	SNEX_INTERPRETER_NODE(Cast, WantsNode::No);

	Cast(Factory& f, const ValueTree& v) :
		Expression(f, v),
		targetType(Helpers::getType(f, v, AstIds::Target)),
		sourceValue(Helpers::getChildExpression(this, 0))
	{
		switch (targetType.getType())
		{
		case Types::ID::Integer:
			castFunction = castStatic<Types::ID::Integer>; 
			break;
		case Types::ID::Double:
			castFunction = castStatic<Types::ID::Double>;
			break;
		case Types::ID::Float:
			castFunction = castStatic<Types::ID::Float>;
			break;
		}

		jassert(castFunction != nullptr);
	};

	typedef VariableStorage(*CastFunction)(const VariableStorage&);

	template <Types::ID TargetType> static VariableStorage castStatic(const VariableStorage& source)
	{
		return VariableStorage(source.template toType<TargetType>());
	}

	VariableStorage getResult(Scope* s) override
	{
		auto v = sourceValue->getResult(s);
		return castFunction(v);
	}

	TypeInfo getTypeInfo() const override
	{
		return targetType;
	}

	CastFunction castFunction = nullptr;
	TypeInfo targetType;
	ExprPtr sourceValue;

};

struct Subscript : public Expression
{
	SNEX_INTERPRETER_NODE(Subscript, WantsNode::No);

	Subscript(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  elementType(Helpers::getType(f, v, AstIds::ElementType)),
	  parentType(Helpers::getType(f, v, AstIds::ParentType)),
	  parent(Helpers::getChildExpression(this, 0)),
	  index(Helpers::getChildExpression(this, 1)),
	  f(OpFunctions::createDataFunctions(elementType))
	{};

	typedef VariableStorage(*CastFunction)(const VariableStorage&);

	template <Types::ID TargetType> static VariableStorage castStatic(const VariableStorage& source)
	{
		return VariableStorage(source.template toType<TargetType>());
	}

	VariableStorage getResult(Scope* s) override
	{
		auto dataPtr = reinterpret_cast<uint8*>(parent->getResult(s).toPtr());
		auto idx = index->getResult(s).toInt();
		auto elementSize = elementType.getRequiredByteSize();
		return f.first(dataPtr, elementSize * idx);
	}

	void assign(Scope* s, const VariableStorage& v) override
	{
		jassertfalse;
	}

	TypeInfo getTypeInfo() const override
	{
		return elementType;
	}

	TypeInfo parentType;
	TypeInfo elementType;
	OpFunctions::DataFunctions f;
	ExprPtr parent;
	ExprPtr index;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
