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
DECLARE_ID(Target);
DECLARE_ID(Symbol);
DECLARE_ID(First);

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
struct UriLocationBuilder;
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

struct UriLocationBuilder : public ReferenceCountedObject
{
	UriLocationBuilder(const String& code, const String& preprocessedCode);

	using Ptr = ReferenceCountedObjectPtr<UriLocationBuilder>;

	UriLocation createLocation(const ValueTree& v) const;

	bool resolvePreprocessedCode = false;
	CodeDocument codeDoc;
	CodeDocument preprocessedDoc;

	UriLocation baseLocation;
};

} // namespace interpreter
} // namespace snex