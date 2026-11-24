#pragma once

namespace snex {
namespace interpreter {

// macro definitions

#define DECLARE_ID(x) static const Identifier x(#x);

namespace AstIds
{
DECLARE_ID(Type);
DECLARE_ID(Value);
DECLARE_ID(Signature);
DECLARE_ID(OpType);
DECLARE_ID(AssignmentType);
DECLARE_ID(Target);
DECLARE_ID(Symbol);
DECLARE_ID(First);
DECLARE_ID(InitValuesB64);
DECLARE_ID(InitValues);
DECLARE_ID(Ids);
DECLARE_ID(ScopeId);
DECLARE_ID(CallType);
DECLARE_ID(ObjectType);
DECLARE_ID(MemberType);
DECLARE_ID(Offset);
DECLARE_ID(ReturnType);
DECLARE_ID(ParameterName);

DECLARE_ID(Iterator);
DECLARE_ID(LoadIterator);
DECLARE_ID(LoopType);
DECLARE_ID(command);

DECLARE_ID(ElementSize);
DECLARE_ID(NumElements);

DECLARE_ID(IsPre);
DECLARE_ID(IsDec);

DECLARE_ID(ElementType);
DECLARE_ID(ParentType);

DECLARE_ID(charIndex);
DECLARE_ID(Line);
}

#undef DECLARE_ID

#define SNEX_INTERPRETER_NODE(ValueTreeType, wn) static Identifier getStatementType() { RETURN_STATIC_IDENTIFIER(#ValueTreeType); } \
	static Statement::Ptr create(Statement::Factory& f, const ValueTree& v) { return new ValueTreeType(f, v); } \
	WantsNode shouldCreateNode() const override { return (wn); };

// forward declarations;


struct Engine;
struct Scope;
struct Statement;
struct Expression;
struct ExpressionTreeBuilder;

namespace nodes
{
struct SyntaxTree;
struct Function;
struct ReturnStatement;
struct Immediate;
struct BinaryOp;
struct Cast;
struct VariableReference;
struct Assignment;
}

} // namespace interpreter
} // namespace snex