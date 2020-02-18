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

	struct ScopedScopeStatementSetter // Scoped...
	{
		ScopedScopeStatementSetter(BlockParser* p_, Operations::ScopeStatementBase* current):
			p(p_)
		{
			old = p->currentScopeStatement;
			p->currentScopeStatement = current;
		}

		~ScopedScopeStatementSetter()
		{
			p->currentScopeStatement = old;
		}

		BlockParser* p;
		WeakReference<Operations::ScopeStatementBase> old;
	};

	BlockParser(BaseCompiler* c, const juce::String::CharPointerType& code, const juce::String::CharPointerType& wholeProgram, int length, const Symbol& rootSymbol) :
		TokenIterator(code, wholeProgram, length, rootSymbol),
		compiler(c)
	{};
    
    virtual ~BlockParser() {};

	SyntaxTree* parseStatementList();

	virtual StatementPtr parseStatement() = 0;

	virtual void finaliseSyntaxTree(SyntaxTree* tree) 
	{
		ignoreUnused(tree);
	}

	Types::ID matchType() override
	{
		if (currentType == JitTokens::identifier)
		{
			auto cs = getCurrentScopeStatement();

			auto id = parseIdentifier();

			Types::ID type = Types::ID::Void;

			if (cs = cs->getScopedStatementForAlias(id))
			{
				type = cs->getAliasNativeType(id);
			}

			if (type == Types::ID::Void)
				throwTokenMismatch("Type");

			return type;
		}

		return TokenIterator::matchType();
	}

	bool matchIfTypeToken() override
	{
		if (currentType == JitTokens::identifier)
		{
			auto id = Identifier(currentValue.toString());

			auto cs = getCurrentScopeStatement();

			if (cs = cs->getScopedStatementForAlias(id))
			{
				match(JitTokens::identifier);
				currentHnodeType = cs->getAliasNativeType(id);
				return true;
			}
		}

		return TokenIterator::matchIfTypeToken();
	}

	VariableStorage parseVariableStorageLiteral();

	InitialiserList::Ptr parseInitialiserList();

	SpanType::Ptr parseSpanType();

	WeakReference<BaseCompiler> compiler;

	Operations::ScopeStatementBase* getCurrentScopeStatement() { return currentScopeStatement; }

	void parseUsingAlias();



private:

	WeakReference<Operations::ScopeStatementBase> currentScopeStatement;
};



class NewClassParser : public BlockParser
{
public:

	NewClassParser(BaseCompiler* c, const juce::String& code, const Symbol& rootSymbol):
		BlockParser(c, code.getCharPointer(), code.getCharPointer(), code.length(), rootSymbol)
	{}

	NewClassParser(BaseCompiler* c, const ParserHelpers::CodeLocation& l, int codeLength, const Symbol& rootSymbol) :
		BlockParser(c, l.location, l.program, codeLength, rootSymbol)
	{}

    virtual ~NewClassParser() {};
    
	StatementPtr parseStatement() override;
	StatementPtr parseSmoothedVariableDefinition();
	StatementPtr parseWrappedBlockDefinition();
	ExprPtr parseBufferInitialiser();
	StatementPtr parseVariableDefinition(bool isConst);
	StatementPtr parseFunction();
	StatementPtr parseSubclass();
	
	StatementPtr parseComplexTypeDefinition(ComplexType::Ptr p);

	StatementPtr parseDefinition(bool isConst, Types::ID type, bool isWrappedBuffer, bool isSmoothedVariable);

};



}
}
