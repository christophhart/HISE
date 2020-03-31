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


namespace NamespaceResolver
{

struct CanExist
{
	static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
	{
		auto r = n.resolve(c, true);

		if (!r.wasOk())
			l.throwError(r.getErrorMessage());
	}
};

struct MustExist
{
	static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
	{
		auto r = n.resolve(c);

		if (!r.wasOk())
			l.throwError(r.getErrorMessage());
	}
};

struct MustBeNew
{
	static void resolve(NamespaceHandler& n, NamespacedIdentifier& c, const ParserHelpers::CodeLocation& l)
	{

	}
};
}

#define PROCESS_IF_NOT_NULL(expr) if (expr != nullptr) expr->process(compiler, scope);
#define COMPILER_PASS(x) if (compiler->getCurrentPass() == x)
#define CREATE_ASM_COMPILER(type) AsmCodeGenerator(getFunctionCompiler(compiler), &compiler->registerPool, type);
#define SKIP_IF_CONSTEXPR if(isConstExpr()) return;

class SymbolParser : public ParserHelpers::TokenIterator
{
public:

	SymbolParser(ParserHelpers::TokenIterator& other_, NamespaceHandler& handler_) :
		ParserHelpers::TokenIterator(other_),
		handler(handler_),
		other(other_)
	{};

	TokenIterator& other;

	Symbol parseExistingSymbol(bool needsStaticTyping)
	{
		parseNamespacedIdentifier<NamespaceResolver::MustExist>();

		auto type = handler.getVariableType(currentNamespacedIdentifier);
		auto s = Symbol(currentNamespacedIdentifier, type);

		if (needsStaticTyping && s.typeInfo.isDynamic())
			location.throwError("Can't resolve symbol type");

		return s;
	}

	Symbol parseNewDynamicSymbolSymbol(NamespaceHandler::SymbolType t)
	{
		jassert(other.currentTypeInfo.isDynamic());
		parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();
		auto s = Symbol(currentNamespacedIdentifier, other.currentTypeInfo);
		return s;
	}

	Symbol parseNewSymbol(NamespaceHandler::SymbolType t)
	{
		auto type = other.currentTypeInfo;

		parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();
		
		auto s = Symbol(currentNamespacedIdentifier, type);

		if(t != NamespaceHandler::Unknown)
			handler.addSymbol(s.id, type, t);

		if (s.typeInfo.isDynamic() && t != NamespaceHandler::UsingAlias)
			location.throwError("Can't resolve symbol type");

		return s;
	}
	

	template <class T> void parseNamespacedIdentifier()
	{
		auto c = handler.getCurrentNamespaceIdentifier();

		auto id = parseIdentifier();

		auto isExplicit = currentType == JitTokens::double_colon;

		if (isExplicit)
		{
			// reset the namespace or it will stack up...
			c = NamespacedIdentifier();
		}

		c = c.getChildId(id);

		while (matchIf(JitTokens::double_colon))
		{
			c = c.getChildId(parseIdentifier());
		}

		T::resolve(handler, c, location);
		currentNamespacedIdentifier = c;

		other.seek(*this);
	}

	NamespacedIdentifier currentNamespacedIdentifier;
	NamespaceHandler& handler;
};

class TypeParser : public ParserHelpers::TokenIterator
{
public:

	TypeParser(TokenIterator& other_, NamespaceHandler& handler) :
		ParserHelpers::TokenIterator(other_),
		namespaceHandler(handler),
		other(other_),
		nId(NamespacedIdentifier::null())
	{
		
	};

	TokenIterator& other;

	void matchType()
	{
		if (!matchIfType())
			throwTokenMismatch("Type");
	}

	bool matchIfType()
	{
		auto isConst = matchIf(JitTokens::const_);

		if (matchIfTypeInternal())
		{
			auto isRef = matchIf(JitTokens::bitwiseAnd);
			currentTypeInfo = currentTypeInfo.withModifiers(isConst, isRef);
			other.seek(*this);
			return true;
		}

		return false;
	}

private:

	Array<TemplateParameter> parseTemplateParameters();

	VariableStorage parseConstExpression(bool canBeTemplateParameter);

	bool parseNamespacedIdentifier()
	{
		if (currentType == JitTokens::identifier)
		{
			SymbolParser p(*this, namespaceHandler);
			p.parseNamespacedIdentifier<NamespaceResolver::CanExist>();
			nId = p.currentNamespacedIdentifier;
			return true;
		}

		return false;
	}

	bool matchIfTypeInternal()
	{
		if (parseNamespacedIdentifier())
		{
			if (namespaceHandler.isTemplateTypeArgument(nId))
			{
				currentTypeInfo = TypeInfo(nId);
				return true;
			}

			auto t = namespaceHandler.getAliasType(nId);

			if (namespaceHandler.isTemplateClass(nId))
			{
				auto tp = parseTemplateParameters();

				Result r = Result::ok();

				auto s = namespaceHandler.createTemplateInstantiation(nId, tp, r);

				if (!r.wasOk())
					location.throwError(r.getErrorMessage());

				currentTypeInfo = TypeInfo(s);
				parseSubType();
				return true;
			}

			if (auto vId = namespaceHandler.getVariadicTypeForId(nId))
			{
				auto newType = new VariadicTypeBase(vId);

				auto tp = parseTemplateParameters();

				for (auto p : tp)
					newType->addType(p.type.getComplexType());

				currentTypeInfo = TypeInfo(namespaceHandler.registerComplexTypeOrReturnExisting(newType));

				parseSubType();
				return true;
			}

			if (auto typePtr = namespaceHandler.getComplexType(nId))
			{
				currentTypeInfo = TypeInfo(typePtr);
				parseSubType();
				return true;
			}

			if (t.isValid())
			{
				currentTypeInfo = t;
				return true;
			}

			auto p = nId;

			while (p.isValid())
			{
				auto id = NamespacedIdentifier(p.getIdentifier());
				p = p.getParent();
				t = namespaceHandler.getAliasType(p);

				if (t.isValid() && t.isComplexType())
				{
					if (auto st = t.getComplexType()->createSubType(id))
					{
						currentTypeInfo = TypeInfo(st, currentTypeInfo.isConst(), currentTypeInfo.isRef());
						return true;
					}
				}
			}
		}

		if (matchIfSimpleType())
			return true;
		else if (matchIfComplexType())
		{
			parseSubType();
			return true;
		}

		return false;
	}

	void parseSubType()
	{
		while (matchIf(JitTokens::double_colon))
		{
			auto parentType = currentTypeInfo;

			if (!parseNamespacedIdentifier())
				location.throwError("funky");

			if (auto subType = parentType.getComplexType()->createSubType(nId))
			{
				currentTypeInfo = TypeInfo(subType, parentType.isConst(), parentType.isRef());
			}
		}
	}

	bool matchIfSimpleType()
	{
		Types::ID t;

		if (matchIf(JitTokens::float_))		  t = Types::ID::Float;
		else if (matchIf(JitTokens::int_))	  t = Types::ID::Integer;
		else if (matchIf(JitTokens::double_)) t = Types::ID::Double;
		else if (matchIf(JitTokens::void_))	  t = Types::ID::Void;
		else if (matchIf(JitTokens::auto_))	  t = Types::ID::Dynamic;
		else
		{
			currentTypeInfo = {};
			return false;
		}

		currentTypeInfo = TypeInfo(t);
		return true;
	}

	bool matchIfComplexType()
	{
		if (nId.isValid())
		{
			
		}

		return false;
	}

	ComplexType::Ptr parseComplexType(const juce::String& token)
	{
		jassertfalse;
		return nullptr;
	}

	Types::ID matchTypeId()
	{
		if (matchIf(JitTokens::float_)) return Types::ID::Float;
		if (matchIf(JitTokens::int_))	return Types::ID::Integer;
		if (matchIf(JitTokens::double_)) return Types::ID::Double;
		if (matchIf(JitTokens::void_))	return Types::ID::Void;
		if (matchIf(JitTokens::auto_))  return Types::ID::Dynamic;

		throwTokenMismatch("Type");

		RETURN_IF_NO_THROW(Types::ID::Void);
	}

	private:

	NamespacedIdentifier nId;
	NamespaceHandler& namespaceHandler;
};


class BlockParser : public ParserHelpers::TokenIterator
{
public:

	using ExprPtr = Operations::Expression::Ptr;
	using StatementPtr = Operations::Statement::Ptr;

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
    
    virtual ~BlockParser() {};

	StatementPtr parseStatementList();

	virtual StatementPtr parseFunction(const Symbol& s)
	{
		location.throwError("Can't parse function");
		return nullptr;
	}

	virtual StatementPtr parseStatement() = 0;

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

	void matchType()
	{
		if (!matchIfType())
			throwTokenMismatch("Type");
	}

	bool matchIfType()
	{
		TypeParser t(*this, compiler->namespaceHandler);

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

	StatementPtr parseComplexTypeDefinition();

	StatementPtr matchIfSemicolonAndReturn(StatementPtr p)
	{
		matchIf(JitTokens::semicolon);
		return p;
	}

	StatementPtr matchSemicolonAndReturn(StatementPtr p)
	{
		match(JitTokens::semicolon);
		return p;
	}

	InitialiserList::Ptr parseInitialiserList();

	WeakReference<BaseScope> currentScope;

	WeakReference<BaseCompiler> compiler;

	Operations::ScopeStatementBase* getCurrentScopeStatement() { return currentScopeStatement; }

	TypeInfo getDotParentType(ExprPtr e);

	NamespacedIdentifier getDotParentName(ExprPtr e);
	
	void parseUsingAlias();

	ExprPtr createBinaryNode(ExprPtr l, ExprPtr r, TokenType op);
	ExprPtr parseExpression();
	ExprPtr parseTernaryOperator();
	ExprPtr parseBool();
	ExprPtr parseLogicOperation();
	ExprPtr parseComparation();
	ExprPtr parseSum();
	ExprPtr parseDifference();
	ExprPtr parseProduct();
	ExprPtr parseTerm();
	ExprPtr parseCast(Types::ID type);
	ExprPtr parseUnary();
	ExprPtr parseFactor();
	ExprPtr parseDotOperator(ExprPtr p);
	ExprPtr parseSubscript(ExprPtr p);
	ExprPtr parseCall(ExprPtr p);
	ExprPtr parsePostSymbol();
	ExprPtr parseReference(bool mustBeValidRootSymbol=true);
	ExprPtr parseLiteral(bool isNegative = false);

private:

	Symbol currentSymbol;

	bool isParsingTemplateArgument = false;

	WeakReference<Operations::ScopeStatementBase> currentScopeStatement;
	
};





class NewClassParser : public BlockParser
{
public:

	NewClassParser(BaseCompiler* c, const juce::String& code):
		BlockParser(c, code.getCharPointer(), code.getCharPointer(), code.length())
	{}

	NewClassParser(BaseCompiler* c, const ParserHelpers::CodeLocation& l, int codeLength) :
		BlockParser(c, l.location, l.program, codeLength)
	{}

    virtual ~NewClassParser() {};
    
	TemplateParameter::List templateArguments;

	StatementPtr parseStatement() override;
	ExprPtr parseBufferInitialiser();
	StatementPtr parseVariableDefinition();
	StatementPtr parseFunction(const Symbol& s) override;
	StatementPtr parseSubclass();
	//StatementPtr parseWrapDefinition();
	
	StatementPtr parseDefinition();

	

};



}
}
