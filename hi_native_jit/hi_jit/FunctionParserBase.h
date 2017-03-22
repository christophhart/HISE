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
#define BASE_NODE NativeJIT::NodeBase* 
#define BOOL_NODE NativeJIT::Node<BooleanType>&

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

	
	TYPED_NODE parseTypedExpression();

	void checkAllLinesReferenced();

	int getNumGlobalNodes() { return globalNodes.size(); }
	GlobalNode* getGlobalNode(int index) { return globalNodes[index]; };

	int getNumAnonymousLines() { return anonymousLines.size(); }
	TYPED_NODE getAnonymousLine(int index);

private:


	enum AssignType
	{
		Assign = 0,
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		numAssignTypes
	};

	typedef const char* TokenType;

	TYPED_NODE_VOID parseLine(bool isConst);

	TYPED_NODE_VOID parseLineAssignment(NamedNode* l);

	void parseUntypedLine();

	TYPED_NODE_VOID parseGlobalAssignment(GlobalBase* g);

	TokenType parseAssignType();


	
    template <typename T> BASE_NODE getOSXDummyNode(NativeJIT::NodeBase* node);
    
	//TYPED_NODE parseCondition();
	TYPED_NODE getEmptyNode() { return exprBase->Immediate(T()); }

	BASE_NODE parseExpression();
	BASE_NODE parseTernaryOperator();

	template <typename ExpectedType> NativeJIT::NodeBase* parseCast();

	bool nodesHaveSameType(NativeJIT::NodeBase* a, NativeJIT::NodeBase* b);

	BASE_NODE parseSum();
	BASE_NODE parseProduct();
	BASE_NODE parseTerm();

	BASE_NODE parseFactor();
    BASE_NODE parseUnary();
	BASE_NODE parseSymbolOrLiteral();
	BASE_NODE getNodeForLine(NamedNode* r);
	BASE_NODE parseSymbol();
	BASE_NODE parseParameterReference(const Identifier &id);
	BASE_NODE parseBufferOperation(const Identifier &id);
	
	NativeJIT::NodeBase* createBinaryNode(NativeJIT::NodeBase* a, NativeJIT::NodeBase* b, TokenType op);

	template <typename T> BASE_NODE createTypedBinaryNode(NativeJIT::Node<T>& a, NativeJIT::Node<T>& b, TokenType op);

	TYPED_NODE getTypedNode(NativeJIT::NodeBase* node);

	TYPED_NODE getCastedNode(NativeJIT::NodeBase* node);


	TypeInfo getTypeForNode(NativeJIT::NodeBase* node);

	BASE_NODE parseBool();
    BASE_NODE parseLogicOperation();
    BASE_NODE parseComparation();

protected:
	
	template <typename T> BASE_NODE parseBufferFunction(const Identifier& id);
	NativeJIT::Node<Buffer*>& getBufferNode(const Identifier& id);
	NativeJIT::Node<float>& parseBufferAccess(const Identifier &id);
	template <typename T> BASE_NODE parseBufferAssignment(const Identifier &id);

private:

	template <typename T> BASE_NODE getGlobalNodeGetFunction(const Identifier &id);

	BASE_NODE getGlobalReference(const Identifier& id);
	BASE_NODE parseLiteral();
	BASE_NODE parseFunctionCall(BaseFunction* b);
	TYPED_NODE_VOID checkParameterType(BaseFunction* b, int parameterIndex);

	template <typename R> BASE_NODE parseFunctionParameterList(BaseFunction* b);

	

	struct ParameterInfo
	{
		ParameterInfo(TypeInfo returnType_) :
			returnType(returnType_)
		{};

		Array<NativeJIT::NodeBase*> nodes;
		std::vector<TypeInfo> types;

		TypeInfo returnType;
	};

	

	template <typename R, typename ParamType> BASE_NODE parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1);
	template <typename R, typename ParamType1, typename ParamType2> BASE_NODE parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2);

	GlobalNode* getGlobalNode(const Identifier& id);
	NamedNode* getLine(const Identifier& id);
	TypeInfo peekFirstType();
	NativeJITScope::Pimpl* scope;
	const FunctionInfo& info;
	OwnedArray<GlobalNode> globalNodes;
	ScopedPointer<MissingOperatorFunctions> missingOperatorFunctions;
	OwnedArray<NamedNode> lines;

	Array<NativeJIT::NodeBase*> anonymousLines;

	NativeJIT::ExpressionNodeFactory* exprBase;
	Identifier lastParsedLine;

	NativeJIT::Node<BooleanType>* yes = nullptr;
	NativeJIT::Node<BooleanType>* no = nullptr;
	NativeJIT::Node<BooleanType>* and_ = nullptr;

	

};

#include "FunctionParserBase.cpp"

#endif  // FUNCTIONPARSERBASE_H_INCLUDED
