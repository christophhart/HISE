/*
  ==============================================================================

    FunctionParserBase.h
    Created: 7 Mar 2017 10:44:39pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef FUNCTIONPARSERBASE_H_INCLUDED
#define FUNCTIONPARSERBASE_H_INCLUDED



#define TYPED_NODE template <typename T> AsmJitHelpers::TypedNode<T>*
#define TYPED_VOID template <typename T> void

#define TypedNodePtr AsmJitHelpers::TypedNode<T>*

#define BOOL_NODE AsmJitHelpers::TypedNode<BooleanType>&

typedef AsmJitHelpers::BaseNode* BaseNodePtr;
typedef asmjit::X86Compiler JitCompiler;

typedef AsmJitHelpers::BaseNode NamedNode;

class FunctionParserBase : protected ParserHelpers::TokenIterator
{
public:


	

	struct MissingOperatorFunctions;
	//struct NamedNode;

	FunctionParserBase(HiseJITScope::Pimpl* scope_, const FunctionInfo& info_);
	virtual ~FunctionParserBase() {};

	void setCompiler(JitCompiler* b) { asmCompiler = b; }

	void parseFunctionBody();


protected:

	bool voidReturnWasFound = false;

	virtual void parseReturn() = 0;
	virtual void addVoidReturnStatement() = 0;
	virtual BaseNodePtr parseParameterReferenceTyped(const Identifier& id) = 0;

#if INCLUDE_BUFFERS
	virtual void parseBufferLine(const Identifier &id) = 0;
#endif

	

	int getParameterIndex(const Identifier& id);

	BaseNodePtr getParameterNode(const Identifier& id);
	
	TYPED_NODE parseTypedExpression();

#if 0
	void checkAllLinesReferenced();
#endif

#if 0
	int getNumGlobalNodes() { return globalNodes.size(); }
	GlobalNode* getGlobalNode(int index) { return globalNodes[index]; };
#endif

#if 0
	int getNumAnonymousLines() { return anonymousLines.size(); }
	TYPED_NODE getAnonymousLine(int index);
#endif

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

	TYPED_VOID parseLine(bool isConst);

	TYPED_VOID parseLineAssignment(AsmJitHelpers::BaseNode* l);

	void parseUntypedLine();

	TYPED_VOID parseGlobalAssignment(GlobalBase* g);

	TokenType parseAssignType();

	BaseNodePtr parseExpression();
	BaseNodePtr parseTernaryOperator();

	template <typename ExpectedType> BaseNodePtr parseCast();

	bool nodesHaveSameType(BaseNodePtr a, BaseNodePtr b);

	BaseNodePtr parseSum();
	BaseNodePtr parseDifference();
	BaseNodePtr parseProduct();
	BaseNodePtr parseTerm();

	BaseNodePtr parseFactor();
    BaseNodePtr parseUnary();
	BaseNodePtr parseSymbolOrLiteral();
	BaseNodePtr getNodeForLine(NamedNode* r);
	BaseNodePtr parseSymbol();
	BaseNodePtr parseParameterReference(const Identifier &id);
	BaseNodePtr parseBufferOperation(const Identifier &id);
	
	BaseNodePtr createBinaryNode(BaseNodePtr a, BaseNodePtr b, TokenType op);

	template <typename T> BaseNodePtr createTypedBinaryNode(TypedNodePtr a, TypedNodePtr b, TokenType op);

	

	TYPED_NODE getCastedNode(BaseNodePtr node);


	TypeInfo getTypeForNode(BaseNodePtr node);

	BaseNodePtr parseBool();
    BaseNodePtr parseLogicOperation();
    BaseNodePtr parseComparation();

protected:
	
	TYPED_NODE getTypedNode(BaseNodePtr node);

	template <typename T> BaseNodePtr parseBufferFunction(const Identifier& id);
	AsmJitHelpers::TypedNode<Buffer*>* getBufferNode(const Identifier& id);
	AsmJitHelpers::TypedNode<float>* parseBufferAccess(const Identifier &id);
	void parseBufferAssignment(const Identifier &id);

private:

	template <typename T> BaseNodePtr getGlobalNodeGetFunction(const Identifier &id);

	BaseNodePtr getGlobalReference(const Identifier& id);
	BaseNodePtr parseLiteral();
	BaseNodePtr parseFunctionCall(BaseFunction* b);
	TYPED_VOID checkParameterType(BaseFunction* b, int parameterIndex);

	template <typename R> BaseNodePtr parseFunctionParameterList(BaseFunction* b);

	

	struct ParameterInfo
	{
		ParameterInfo(TypeInfo returnType_) :
			returnType(returnType_)
		{};

		OwnedArray<AsmJitHelpers::BaseNode> nodes;
		std::vector<TypeInfo> types;

		TypeInfo returnType;
	};

	

	template <typename R, typename ParamType> BaseNodePtr parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1);
	template <typename R, typename ParamType1, typename ParamType2> BaseNodePtr parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1, BaseNodePtr param2);


	BaseNodePtr getGlobalNode(const Identifier& id);

	AsmJitHelpers::TypedNode<uint64_t>* getBufferDataNode(const Identifier& id);

	AsmJitHelpers::BaseNode* getLine(const Identifier& id);
	
	protected:
	HiseJITScope::Pimpl* scope;
	const FunctionInfo& info;

#if 0
	OwnedArray<GlobalNode> globalNodes;
#endif
	//ScopedPointer<MissingOperatorFunctions> missingOperatorFunctions;
	OwnedArray<AsmJitHelpers::BaseNode> lines;

	//Array<BaseNodePtr> anonymousLines;

	JitCompiler*  asmCompiler;

	//HiseJIT::ExpressionNodeFactory* exprBase;
	Identifier lastParsedLine;

	OwnedArray<AsmJitHelpers::BaseNode> parameterNodes;

	OwnedArray<AsmJitHelpers::BaseNode> globalNodes;

	OwnedArray<AsmJitHelpers::TypedNode<uint64_t>> bufferDataNodes;

	//HiseJIT::Node<BooleanType>* yes = nullptr;
	//HiseJIT::Node<BooleanType>* no = nullptr;
	//HiseJIT::Node<BooleanType>* and_ = nullptr;

	

};

#include "FunctionParserBase.cpp"

#endif  // FUNCTIONPARSERBASE_H_INCLUDED
