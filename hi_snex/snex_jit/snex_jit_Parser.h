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

namespace snex {
namespace jit
{
using namespace juce;


#define PROCESS_IF_NOT_NULL(expr) if (expr != nullptr) expr->process(compiler, scope);
#define COMPILER_PASS(x) if (compiler->getCurrentPass() == x)
#define CREATE_ASM_COMPILER(type) AsmCodeGenerator(getFunctionCompiler(compiler), &compiler->registerPool, type);
#define SKIP_IF_CONSTEXPR if(isConstExpr()) return;





class BlockParser : public ParserHelpers::TokenIterator
{
public:

	using ExprPtr = Operations::Expression::Ptr;
	using StatementPtr = Operations::Statement::Ptr;

	BlockParser(BaseCompiler* c, const juce::String::CharPointerType& code, const juce::String::CharPointerType& wholeProgram, int length) :
		TokenIterator(code, wholeProgram, length),
		compiler(c)
	{};
    
    virtual ~BlockParser() {};

	SyntaxTree* parseStatementList();

	virtual StatementPtr parseStatement() = 0;

	virtual void finaliseSyntaxTree(SyntaxTree* tree) 
	{
		ignoreUnused(tree);
	}

	VariableStorage parseVariableStorageLiteral();

	WeakReference<BaseCompiler> compiler;
};



class NewClassParser : public BlockParser
{
public:

	NewClassParser(BaseCompiler* c, const juce::String& code):
		BlockParser(c, code.getCharPointer(), code.getCharPointer(), code.length())
	{}

    virtual ~NewClassParser() {};
    
	StatementPtr parseStatement() override;
	StatementPtr parseSmoothedVariableDefinition();
	StatementPtr parseWrappedBlockDefinition();
	ExprPtr parseBufferInitialiser();
	StatementPtr parseVariableDefinition(bool isConst);
	StatementPtr parseFunction();
};



}
}
