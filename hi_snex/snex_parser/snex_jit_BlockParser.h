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



class BlockParser : public ParserHelpers::TokenIterator
{
public:

	using ExprPtr = Operations::Expression::Ptr;
	using StatementPtr = Operations::Statement::Ptr;

	struct CommentAttacher
	{
		CommentAttacher(TokenIterator& t):
			lineNumber(0)
		{
			t.skipWhitespaceAndComments();
			comment = t.lastComment;

			processComment(comment);

			t.lastComment = {};
		}

		ExprPtr withComment(ExprPtr p)
		{
			p->comment = comment;
			return p;
		}

		static void processComment(String& s)
		{
			s = s.replace("//", "");
			s = s.replace("/**", "");
			s = s.replace("*/", "");
			s = s.replace("/*", "");

			auto lines = StringArray::fromLines(s);

			for (auto& s : lines)
			{
				s = s.trim();
			}

			s = lines.joinIntoString("\n");
		}

		NamespaceHandler::SymbolDebugInfo getInfo() const
		{
			NamespaceHandler::SymbolDebugInfo info;
			info.comment = comment;
			info.lineNumber = lineNumber;
			return info;
		}

		FunctionData& withComment(FunctionData& d)
		{
			d.description = comment;
			return d;
		}

		int lineNumber;
		String comment;
	};

	ExprPtr withAttachedComment(ExprPtr p)
	{
		p->comment = lastComment;
		lastComment = {};
		return p;
	}

	struct ScopedTemplateArgParser
	{
		ScopedTemplateArgParser(BlockParser& p_, bool isTemplateArgument):
			p(p_)
		{
			wasTemplateArgument = p.isParsingTemplateArgument;
			p.isParsingTemplateArgument = isTemplateArgument;
		}

		~ScopedTemplateArgParser()
		{
			p.isParsingTemplateArgument = wasTemplateArgument;
		}

		bool wasTemplateArgument;
		BlockParser& p;
	};

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
		bool pushNamespace = true;
	};

	BlockParser(BaseCompiler* c, const juce::String::CharPointerType& code, const juce::String::CharPointerType& wholeProgram, int length) :
		TokenIterator(code, wholeProgram, length),
		compiler(c)
	{};
    
	BlockParser(BaseCompiler* c, const String& code) :
		TokenIterator(code),
		compiler(c)
	{};

    virtual ~BlockParser() {};

	StatementPtr parseStatementList();

	virtual StatementPtr parseFunction(const Symbol& s);

	virtual StatementPtr parseStatement(bool mustHaveSemicolon = true) = 0;

	Symbol parseNewSymbol(NamespaceHandler::SymbolType t)
	{
		SymbolParser p(*this, compiler->namespaceHandler);
		currentSymbol = p.parseNewSymbol(t);
		return currentSymbol;
	}

	Symbol parseExistingSymbol()
	{
		SymbolParser p(*this, compiler->namespaceHandler);

		currentSymbol = p.parseExistingSymbol(false);
		return currentSymbol;
	}

	Symbol getCurrentSymbol()
	{
		return currentSymbol;
	}

	virtual void finaliseSyntaxTree(SyntaxTree* tree) 
	{
		ignoreUnused(tree);
	}

	void matchType(const TemplateParameter::List& previouslyParsedArguments)
	{
		if (!matchIfType(previouslyParsedArguments))
			throwTokenMismatch("Type");
	}

	bool matchIfType(const TemplateParameter::List& previouslyParsedArguments)
	{
		TypeParser t(*this, compiler->namespaceHandler, previouslyParsedArguments);

		if (t.matchIfType())
		{
			currentTypeInfo = t.currentTypeInfo;
			return true;
		}

		return false;
	}

	VariableStorage parseConstExpression(bool isTemplateArgument);

	ComplexType::Ptr getCurrentComplexType() const { return currentTypeInfo.getTypedIfComplexType<ComplexType>(); }

	Array<TemplateParameter> parseTemplateParameters(bool parseTemplateDefinition);

	StatementPtr parseComplexTypeDefinition(bool mustBeConstructor=false);

	virtual StatementPtr addConstructorToComplexTypeDef(StatementPtr def, const Array<NamespacedIdentifier>& ids, bool matchSemicolon=true);

	StatementPtr matchIfSemicolonAndReturn(StatementPtr p)
	{
		while (currentType == JitTokens::semicolon)
			match(JitTokens::semicolon);

		matchIf(JitTokens::semicolon);
		return p;
	}

	StatementPtr matchSemicolonAndReturn(StatementPtr p, bool mustHaveSemicolon)
	{
		if(mustHaveSemicolon)
			match(JitTokens::semicolon);
		
		return matchIfSemicolonAndReturn(p);
	}

	InitialiserList::Ptr parseInitialiserList();

	WeakReference<BaseScope> currentScope;

	WeakReference<BaseCompiler> compiler;

	Operations::ScopeStatementBase* getCurrentScopeStatement() { return currentScopeStatement; }

	TypeInfo getDotParentType(ExprPtr e, bool checkFunctionCallObjectExpression);

	NamespacedIdentifier getDotParentName(ExprPtr e);
	
	TemplateInstance getTemplateInstanceFromParent(ExprPtr e, NamespacedIdentifier id);

	void parseUsingAlias();

	void parseEnum();

	ExprPtr createBinaryNode(ExprPtr l, ExprPtr r, TokenType op);
	virtual ExprPtr parseExpression();
	ExprPtr parseTernaryOperator();
	ExprPtr parseBool();
	ExprPtr parseLogicOperation();
	ExprPtr parseComparation();
	ExprPtr parseSum();
	ExprPtr parseDifference();
	ExprPtr parseProduct();
	ExprPtr parseTerm();
	ExprPtr parseCast(Types::ID type);
	virtual ExprPtr parseUnary();
	ExprPtr parseThis();
	ExprPtr parseFactor();
	ExprPtr parseDotOperator(ExprPtr p);
	ExprPtr parseSubscript(ExprPtr p);
	ExprPtr parseCall(ExprPtr p);
	ExprPtr parsePostSymbol();
	ExprPtr parseReference(bool mustBeValidRootSymbol=true);
	ExprPtr parseLiteral(bool isNegative = false);

	static bool isVectorOp(TokenType t, ExprPtr l, ExprPtr r = nullptr);

	bool skipIfConsoleCall();

protected:

	WeakReference<Operations::ScopeStatementBase> currentScopeStatement;

private:

	Symbol currentSymbol;
	bool isParsingTemplateArgument = false;
};





}
}
