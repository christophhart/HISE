/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

#define TYPED_NODE template <typename T> AsmJitHelpers::TypedNode<T>*
#define TYPED_VOID template <typename T> void

#define TypedNodePtr AsmJitHelpers::TypedNode<T>*

#define BOOL_NODE AsmJitHelpers::TypedNode<BooleanType>&

typedef AsmJitHelpers::BaseNode* BaseNodePtr;
typedef asmjit::X86Compiler JitCompiler;

typedef AsmJitHelpers::BaseNode NamedNode;

class FunctionParserBase : protected ParserHelpers::TokenIterator,
						   public BaseScope
{
public:

	struct MissingOperatorFunctions;

	FunctionParserBase(JITScope::Pimpl* scope_, const FunctionParserInfo& info_);
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

private:

	

	TYPED_VOID parseLine(bool isConst);

	void parseAssignmentFunky(BaseScope::Reference target);

	TYPED_VOID parseLineAssignment(AsmJitHelpers::BaseNode* l);

	void parseUntypedLine();

	TYPED_VOID parseGlobalAssignment(GlobalBase* g);

	void parseVariableAssignment(VariableStorage& v);

	

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

	BaseNodePtr	parseReferenceFunky(const BaseScope::Reference& ref);

	BaseNodePtr parseParameterReference(const Identifier &id);
    
#if INCLUDE_BUFFERS
	BaseNodePtr parseBufferOperation(const Identifier &id);
#endif
	
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
    
#if INCLUDE_BUFFERS
	AsmJitHelpers::TypedNode<Buffer*>* getBufferNode(const Identifier& id);
#endif
    
	AsmJitHelpers::TypedNode<float>* parseBufferAccess(const Identifier &id);
	void parseBufferAssignment(const Identifier &id);

private:

	template <typename T> BaseNodePtr getGlobalNodeGetFunction(const Identifier &id);

	BaseNodePtr getOrCreateVariableNode(const BaseScope::Reference& ref);

	BaseNodePtr getGlobalReference(const Identifier& id);
	BaseNodePtr parseLiteral();
	BaseNodePtr parseFunctionCall(const AsmJitHelpers::ParameterInfo& pi);

	

#if 0

	BaseNodePtr parseMemberFunctionCall(const AsmJitHelpers::ParameterInfo& pi);

	TYPED_VOID checkParameterType(BaseFunction* b, int parameterIndex);

	template <typename R> BaseNodePtr parseFunctionParameterList(BaseFunction* b);
	

	template <typename R, typename ParamType> BaseNodePtr parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1);
	template <typename R, typename ParamType1, typename ParamType2> BaseNodePtr parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1, BaseNodePtr param2);
#endif

	BaseNodePtr getVariableNode(const BaseScope::Reference& ref);

	BaseNodePtr getGlobalNode(const Identifier& id);
	AsmJitHelpers::TypedNode<uint64_t>* getBufferDataNode(const Identifier& id);
	AsmJitHelpers::BaseNode* getLine(const Identifier& id);
	
	protected:
	JITScope::Pimpl* scope;
	const FunctionParserInfo& info;

	OwnedArray<AsmJitHelpers::BaseNode> lines;

	JitCompiler*  asmCompiler;
	Identifier lastParsedLine;

	AsmJitHelpers::BaseNode::List funkyExistingNodes;

	OwnedArray<AsmJitHelpers::BaseNode> parameterNodes;
	OwnedArray<AsmJitHelpers::BaseNode> globalNodes;
	OwnedArray<AsmJitHelpers::TypedNode<uint64_t>> bufferDataNodes;


};



} // end namespace jit
} // end namespace hnode

#include "FunctionParserBase_impl.h"

