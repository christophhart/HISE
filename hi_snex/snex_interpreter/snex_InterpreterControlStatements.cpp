namespace snex
{
namespace interpreter
{

#define DECLARE_ID(x) static const Identifier x(#x);

namespace AstIds
{
DECLARE_ID(Type);
DECLARE_ID(Value);
DECLARE_ID(Signature);
DECLARE_ID(OpType);
DECLARE_ID(Target);
DECLARE_ID(Symbol);
}

#undef DECLARE_ID

#define SNEX_INTERPRETER_NODE(ValueTreeType) static Identifier getStatementType() { RETURN_STATIC_IDENTIFIER(#ValueTreeType); } \
	static Statement::Ptr create(StatementFactory& f, const ValueTree& v) { return new ValueTreeType(f, v); }

struct Engine::Helpers
{
	static jit::Symbol getSymbol(StatementFactory& f, const ValueTree& astNode, const Identifier& id=Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::Symbol;
		auto symbolString = astNode[idToUse].toString();
		TypeParserBase p(symbolString, true, f.typeLibrary);
		jit::Symbol s;
		s.typeInfo = p.getTypeInfo();
		s.id = p.parseNamespacedIdentifier();
		return s;
	}

	static TypeInfo getType(StatementFactory& f, const ValueTree& astNode, const Identifier& id=Identifier())
	{
		auto idToUse = id.isValid() ? id : AstIds::Type;
		auto typeString = astNode[idToUse].toString();
		TypeParserBase p(typeString, true, f.typeLibrary);
		return p.getTypeInfo();
	}

	static FunctionData getFunctionData(StatementFactory& f, const ValueTree& astNode)
	{
		auto signature = astNode[AstIds::Signature].toString();
		TypeParserBase p(signature, true, f.typeLibrary);
		return p.parseFunctionData();
	}

	static Expression::ExprPtr getChildExpression(StatementFactory& f, const ValueTree& astNode, int childIndex)
	{
		auto cn = astNode.getChild(childIndex);

		if (cn.isValid())
		{
			auto s = f.create(cn);

			if (auto e = dynamic_cast<Expression*>(s.get()))
				return e;
		}

		return nullptr;
	}
};

struct Engine::SyntaxTree : public Engine::Statement
{
	SNEX_INTERPRETER_NODE(SyntaxTree);

	SyntaxTree(StatementFactory& f, const ValueTree& v) :
	  Statement(v)
	{
		for (auto c : v)
		{
			if (auto ptr = f.create(c))
				children.add(ptr);
		}
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{ 
		for (auto c : children)
		{
			if (auto r = c->perform(s, returnValue))
				return r;
		}

		return ResultCode::ok; 
	};

	List children;
};

struct Engine::Function : public Engine::Statement
{
	SNEX_INTERPRETER_NODE(Function);

	Function(StatementFactory& f, const ValueTree& v) :
		Statement(v)
	{
		body = f.create(v.getChildWithName(SyntaxTree::getStatementType()));
		fData = Helpers::getFunctionData(f, v);
		jassert(body != nullptr);
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		return body->perform(s, returnValue);
	};

	FunctionData fData;
	Ptr body;
};

struct Engine::ReturnStatement : public Engine::Statement
{
	SNEX_INTERPRETER_NODE(ReturnStatement);

	ReturnStatement(StatementFactory& f, const ValueTree& v) :
		Statement(v),
		value(Helpers::getChildExpression(f, v, 0))
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		if (returnValue != nullptr && value != nullptr)
			*returnValue = value->getResult(s);
		
		return ResultCode::returnWasHit;
	};

	Expression::ExprPtr value;
};









} // namespace interpreter
} // namespace snex
