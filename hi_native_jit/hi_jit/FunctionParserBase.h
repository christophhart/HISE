/*
  ==============================================================================

    FunctionParserBase.h
    Created: 7 Mar 2017 10:44:39pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef FUNCTIONPARSERBASE_H_INCLUDED
#define FUNCTIONPARSERBASE_H_INCLUDED


#define TYPED_NODE template <typename T> NativeJIT::Node<T>&
#define TYPED_NODE_VOID template <typename T> void

class FunctionParserBase : protected ParserHelpers::TokenIterator
{
public:

	struct MissingOperatorFunctions;
	struct NamedNode;

	FunctionParserBase(NativeJITScope::Pimpl* scope_, const FunctionInfo& info_);
	virtual ~FunctionParserBase() {};

	void parseFunctionBody();

protected:

	virtual void parseReturn() = 0;
	virtual void addVoidReturnStatement() = 0;
	virtual NativeJIT::NodeBase* parseParameterReferenceTyped(const Identifier& id) = 0;
	virtual void parseBufferLine(const Identifier &id) = 0;

	void setExpressionNodeFactory(NativeJIT::ExpressionNodeFactory* b) { exprBase = b; }

	int getParameterIndex(const Identifier& id);

	TYPED_NODE parseExpression();

	void checkAllLinesReferenced();

	int getNumGlobalNodes() { return globalNodes.size(); }
	GlobalNode* getGlobalNode(int index) { return globalNodes[index]; };

private:

	TYPED_NODE_VOID parseLine(bool isConst);
	TYPED_NODE_VOID parseGlobalAssignment(GlobalBase* g);
	
	TYPED_NODE parseSum();
	TYPED_NODE parseProduct();
	TYPED_NODE parseCondition();
	TYPED_NODE getEmptyNode() { return exprBase->Immediate(T()); }

	template <typename T, typename ConditionType, NativeJIT::JccType compareFlag> NativeJIT::Node<T>& parseBranches(NativeJIT::Node<ConditionType>& left, bool hasOpenParen);
	template <typename T, typename ConditionType> NativeJIT::Node<T>& parseTernaryOperator();
	template <typename TargetType, typename ExpectedType> NativeJIT::Node<TargetType>& parseCast();

	TYPED_NODE parseTerm();
	TYPED_NODE parseFactor();
    TYPED_NODE parseUnary();
	TYPED_NODE parseSymbolOrLiteral();
	TYPED_NODE getNodeForLine(NamedNode* r);
	TYPED_NODE parseSymbol();
	TYPED_NODE parseParameterReference(const Identifier &id);
	TYPED_NODE parseBufferOperation(const Identifier &id);
	
protected:
	
	TYPED_NODE parseBufferFunction(const Identifier& id);
	NativeJIT::Node<Buffer*>& getBufferNode(const Identifier& id);
	NativeJIT::Node<float>& parseBufferAccess(const Identifier &id);
	TYPED_NODE parseBufferAssignment(const Identifier &id);

private:

	TYPED_NODE getGlobalReference(const Identifier& id);
	TYPED_NODE parseLiteral();
	TYPED_NODE parseFunctionCall(BaseFunction* b);
	TYPED_NODE_VOID checkParameterType(BaseFunction* b, int parameterIndex);

	template <typename R> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b);
	template <typename R, typename ParamType> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1);
	template <typename R, typename ParamType1, typename ParamType2> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2);

	GlobalNode* getGlobalNode(const Identifier& id);
	NamedNode* getLine(const Identifier& id);
	TypeInfo peekFirstType();
	NativeJITScope::Pimpl* scope;
	const FunctionInfo& info;
	OwnedArray<GlobalNode> globalNodes;
	ScopedPointer<MissingOperatorFunctions> missingOperatorFunctions;
	OwnedArray<NamedNode> lines;
	NativeJIT::ExpressionNodeFactory* exprBase;
	Identifier lastParsedLine;

};

#include "FunctionParserBase.cpp"

#endif  // FUNCTIONPARSERBASE_H_INCLUDED
