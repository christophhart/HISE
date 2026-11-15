/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise {
using namespace juce;

struct HiseJavascriptEngine::SimpleExpressionData::Parser : private HiseJavascriptEngine::RootObject::TokenIterator
{
	Parser(HiseJavascriptEngine* engine, const UriLocation& location, const String& token) :
		TokenIterator(*engine->root, location.processorId, token, location.uri),
		baseLocation(location),
		scopeTree(engine->root->lastScopeTree),
		pwsc(dynamic_cast<ProcessorWithScriptingContent*>(engine->root->hiseSpecialData.processor))
	{};

	void throwError(const String& errorMessage) const override
	{
		UriError e;
		e.errorMessage = errorMessage;
		e.errorType = UriError::Error;
		location.fillLocation(e);
		throw e;
	}

	SimpleExpressionData parseSuffix(SimpleExpressionData& d)
	{
		if (matchIf(TokenTypes::dot))
		{
			if (matchIf(TokenTypes::eof))
				return d;

			d.type = Type::NeedsEvaluation;
			d.postTokenExpression = "." + getRemainingString();
			return d;
		}

		if (matchIf(TokenTypes::eof))
			return d;

		d.type = Type::NeedsEvaluation;
		d.postTokenExpression = getRemainingString();
		return d;
	}

	SimpleExpressionData parse()
	{
		SimpleExpressionData data;
		data.fullExpression = location.program;
		data.parseError.fillLocation(baseLocation);

		if (matchIf(TokenTypes::eof))
		{
			data.type = Type::Nothing;
			data.resolvedTokenNode = scopeTree != nullptr ? scopeTree->getInnerTree(baseLocation) : nullptr;
			return data;
		}

		if (matchIf(TokenTypes::literal))
		{
			data.type = Type::LiteralValue;
			data.literalValue = currentValue;
			data.typeInfo = f.createTypeInfoObject(pwsc, currentValue, currentValue.toString());
			data.postTokenExpression = location.program;
			return parseSuffix(data);
		}

		if (scopeTree == nullptr)
			throwError("Scope tree is not available");

		innerTree = scopeTree->getInnerTree(baseLocation);

		if (innerTree == nullptr)
			throwError("Can't find location within base scope tree");

		currentNamespace = innerTree->getThisOrParentWithCategory({ Identifiers.namespace_ });
		currentFunction = innerTree->getThisOrParentWithCategory(Identifiers.functions);

		auto firstId = parseIdentifier();

		if (LightweightDiagnostics::Helpers::isApiClass(firstId))
		{
			data.tokenId = firstId;
			data.type = Type::ApiClass;
			data.resolvedParentNode = nullptr;
			data.typeInfo = f.createLightweightPrototype(pwsc, firstId, "");
			return parseSuffix(data);
		}

		if (auto ns = findNamespace(firstId))
		{
			matchIf(TokenTypes::dot);

			if (matchIf(TokenTypes::eof))
			{
				data.tokenId = firstId;
				data.resolvedTokenNode = ns;
				data.type = Type::Namespace;
				return data;
			}

			data.parentId = firstId;
			data.resolvedParentNode = ns;
			data.tokenId = parseIdentifier();
			data.resolvedTokenNode = ns->getSubTreeWithId(data.tokenId, false);
		}
		else
		{
			auto thisScope = innerTree;

			while (thisScope != nullptr)
			{
				for (auto c : *thisScope)
				{
					if (c->id == firstId)
					{
						data.tokenId = firstId;
						data.parentId = thisScope->id;
						data.resolvedTokenNode = c;
						data.resolvedParentNode = thisScope;
						thisScope = nullptr;
						break;
					}
				}

				if(thisScope != nullptr)
					thisScope = thisScope->getParent();
			}
		}

		if (data.resolvedTokenNode == nullptr)
			throwError("Can't resolve token " + firstId.toString());

		if (Identifiers.functions.contains(data.resolvedTokenNode->category))
			data.type = Type::Function;
		else
		{
			data.type = Type::VarDefinition;

			auto typeInfoData = data.resolvedTokenNode->data->getProperty("typeInfo");

			if (typeInfoData.isString())
				data.typeInfo = f.createLightweightPrototype(pwsc, Identifier(typeInfoData), "");
			else
				data.typeInfo = dynamic_cast<LightweightDiagnostics*>(typeInfoData.getObject());
		}

		return parseSuffix(data);
		throwError("Can't resolve expression");
	}

private:

	struct IdentifierDatabase
	{
		IdentifierDatabase() :
			namespace_("namespace_"),
			functions({ Identifier("inline"), Identifier("function") }),
			this_(Identifier("this"))
		{}

		const Identifier namespace_;
		const Array<Identifier> functions;
		const Identifier this_;
	} Identifiers;

	UriLocationTree::Ptr findNamespace(const Identifier& possibleNamespaceId)
	{
		UriLocationTree::Ptr namespaceTree;

		auto found = scopeTree->forEach([&](const UriLocationTree& t)
			{
				if (t.category == Identifier("namespace") &&
					t.id == possibleNamespaceId)
				{
					namespaceTree = const_cast<UriLocationTree*>((&t));
					return true;
				}

				return false;
			});

		jassert(found == (namespaceTree != nullptr));
		return namespaceTree;
	}

	String getRemainingString() const
	{
		return String(location.location, location.program.end());
	}

	LightweightDiagnostics::Factory f;
	UriLocation baseLocation;
	UriLocationTree::Ptr scopeTree;
	UriLocationTree::Ptr innerTree;
	UriLocationTree::Ptr currentNamespace;
	UriLocationTree::Ptr currentFunction;
	ProcessorWithScriptingContent* pwsc;
};


HiseJavascriptEngine::SimpleExpressionData HiseJavascriptEngine::SimpleExpressionData::parse(HiseJavascriptEngine* engine, const UriLocation::WithExpression& token)
{
	try
	{
		SimpleExpressionData::Parser p(engine, token.second, token.first);
		return p.parse();
	}
	catch (UriError& e)
	{
		SimpleExpressionData d;
		d.fullExpression = token.first;
		d.type = Type::Error;
		d.parseError = e;
		return d;
	}
}

UriError HiseJavascriptEngine::SimpleExpressionData::getParseError() const { return parseError; }

UriLocationTree::Ptr HiseJavascriptEngine::SimpleExpressionData::getResolvedNode() const
{
	return resolvedTokenNode;
}



var HiseJavascriptEngine::SimpleExpressionData::getResolvedValue(EvaluationType evaluationType) const
{
	// call evaluate first
	jassert(getType() != Type::NeedsEvaluation);

	if (evaluationType == EvaluationType::TypeInfo)
		return var(typeInfo.get());
	else if (evaluationType == EvaluationType::RuntimeValue)
		return literalValue;

	jassertfalse;
	return var();
}

LightweightDiagnostics::Ptr HiseJavascriptEngine::SimpleExpressionData::getTypeInfo() const
{
	jassert(type == Type::Evaluated || 
			type == Type::VarDefinition || 
			type == Type::LiteralValue ||
			type == Type::ApiClass);

	return typeInfo;
}

String HiseJavascriptEngine::SimpleExpressionData::getAutocompleteTokenContent() const
{
	switch (type)
	{
	case Type::ApiClass:
	case Type::Namespace:
		return tokenId.toString();
	case Type::VarDefinition:
	case Type::Evaluated:
		return fullExpression.trimCharactersAtEnd(".");
	default:
		return getRemainingExpression(RemainingExpressionFormat::NoTrailingDot);
	}
}

String HiseJavascriptEngine::SimpleExpressionData::getRemainingExpression(RemainingExpressionFormat format) const
{
	switch (format)
	{
	case RemainingExpressionFormat::NoTrailingDot:
		return postTokenExpression.trimCharactersAtEnd(".");
	case RemainingExpressionFormat::Full:
	default:
		return postTokenExpression;
	}
}

/** This implements a few special things:

- resolves namespace: if namespace matches and is within namespace, removes it from the token
- adds function signature with selection range like API methods

*/
struct LocationTreeToken : public mcl::TokenCollection::Token
{
	static String getTypeInfoString(const UriLocationTree& t)
	{
		String typeInfo;

		if (t.data->hasProperty("typeInfo"))
		{
			typeInfo << ":";
			var v = t.data->getProperty("typeInfo");

			if (auto lb = dynamic_cast<LightweightDiagnostics*>(v.getObject()))
				typeInfo << lb->getObjectName();
			else
				typeInfo << v.toString();
		}

		return typeInfo;
	}

	LocationTreeToken(const UriLocationTree& c, const String& namespaceId_) :
		Token(c.id.toString()),
		namespaceId(namespaceId_)
	{
		priority = 90;

		codeToInsert << c.id.toString();

		if (c.category == Identifier("namespace"))
		{
			this->c = Colour(0xFF9922AA);
			kind = mcl::TokenCollection::Token::CompletionItemKind::Namespace;

			markdownDescription << "> `" << c.category.toString() << " " << c.id;
		}
		else
		{
			this->c = Colour(0xFF9922AA);

			auto isFunction = c.hasRange();
			auto parentId = c.getParent()->id;

			markdownDescription << "> `" << c.category.toString() << " ";

			if (parentId.isValid())
				markdownDescription << parentId.toString() << ".";

			markdownDescription << c.id.toString();

			markdownDescription << getTypeInfoString(c);

			if (isFunction)
			{


				String signature;

				signature << "(";

				for (auto child : c)
				{
					if (child->category == Identifier("parameter"))
					{
						signature << child->id;
						signature << getTypeInfoString(*child);
						signature << ", ";
					}
				}

				signature = signature.trimCharactersAtEnd(", ");
				signature << ")";

				codeToInsert << signature;
				markdownDescription << signature;
				tokenContent << signature;
			}

			kind = isFunction ? mcl::TokenCollection::Token::CompletionItemKind::Function :
				mcl::TokenCollection::Token::CompletionItemKind::Variable;
		}

		markdownDescription << "` @";
		markdownDescription << c.toLocationString(UriLocation::UriFormat::Compact);

		auto comment = c.data->getProperty("comment").toString();

		if (comment.isNotEmpty())
			markdownDescription << "  \n" << comment;

		location.fillLocation(c);
	}



	bool matches(const String& input, const String& previousToken, int lineNumber) const override
	{
		auto namespaceMatch = previousToken.isEmpty() || previousToken.startsWith(namespaceId);

		if (!namespaceMatch)
			return false;

		return Token::matches(input, previousToken, lineNumber);
	}

	String getCodeToInsert(const String& input) const override
	{
		return codeToInsert;
	}

	String namespaceId;
	String codeToInsert;
};

void HiseJavascriptEngine::SimpleExpressionData::addAutocompleteTokens(mcl::TokenCollection::List& tokens) const
{
	auto stringForToken = getAutocompleteTokenContent();

	switch (getType())
	{
	case Type::Error:
		break;
	case Type::Nothing:
		getResolvedNode()->forEachParent([&](const UriLocationTree& t)
		{
			for (auto c : t)
			{
				if (c->id.isValid())
					tokens.add(new LocationTreeToken(*c, ""));
			}
			return false;
		});
		break;
	case Type::LiteralValue:
	case Type::VarDefinition:
	case Type::ApiClass:
	case Type::Evaluated:
		if(auto t = getTypeInfo())
			t->fillAutocompleteTokens(tokens, stringForToken);
		break;
	case Type::Namespace:
		for (auto c : *getResolvedNode())
			tokens.add(new LocationTreeToken(*c, stringForToken));
		break;
	case Type::NeedsEvaluation:
	{
		jassertfalse;
		break;
	}
	default:
		jassertfalse;
		break;
	}
}

struct HiseJavascriptEngine::SimpleExpressionData::Evaluator
{
	Evaluator(HiseJavascriptEngine* engine_, const SimpleExpressionData& d) :
		engine(engine_),
		data(d),
		p(dynamic_cast<ProcessorWithScriptingContent*>(engine_->root->hiseSpecialData.processor))
	{}

	SimpleExpressionData evaluate(EvaluationType evaluationType)
	{
		if (evaluationType == EvaluationType::RuntimeValue)
		{
			data.type = Type::Error;
			data.parseError.errorMessage = "unimplemented";
		}
		else
		{
			Array<UriError> diagnostics;
			RootObject::ObjectTypeMap::Ptr analyzer = new RootObject::ObjectTypeMap(*engine->root, &diagnostics);
			auto p = dynamic_cast<ProcessorWithScriptingContent*>(engine->root->hiseSpecialData.processor);

			DynamicObject::Ptr scope = analyzer->getCurrentScope().getShadowParserRootObject();

			auto thisScope = data.resolvedTokenNode;

			while (thisScope != nullptr)
			{
				for (auto c : *thisScope)
				{
					if (c->id.isValid() && !c->hasRange())
					{
						if (!scope->hasProperty(c->id))
						{
							if (auto t = createTypeFromNode(*c, *analyzer))
								scope->setProperty(c->id, var(t.get()));
						}
					}
				}

				thisScope = thisScope->getParent();
			}

			try
			{
				auto expressionCode = data.tokenId + data.getRemainingExpression(RemainingExpressionFormat::NoTrailingDot);

				ScopedPointer<RootObject::HiseSpecialData> hd = engine->root->hiseSpecialData.createShadowParseData(analyzer.get());
				RootObject::ExpressionTreeBuilder tb(*engine->root, dynamic_cast<Processor*>(p)->getId(), expressionCode, "", nullptr);
				tb.setupApiData(*hd.get(), "");
				ScopedPointer<RootObject::BlockStatement> sl = tb.parseStatementList();

				if (auto e = dynamic_cast<RootObject::Expression*>(sl->getChildStatement(0)))
				{
					data.literalValue = e->getResult(analyzer->getCurrentScope());
					data.typeInfo = analyzer->createTypeInfoObject(data.literalValue, "");
					data.type = Type::Evaluated;
					return data;
				}
			}
			catch (UriError& e)
			{
				data.type = Type::Error;
				data.parseError = e;
			}
		}

		return data;
	}

private:

	LightweightDiagnostics::Ptr createTypeFromNode(const UriLocationTree& t, const RootObject::ObjectTypeMap& analyzer) const
	{
		if (!t.hasRange() && t.data->hasProperty("typeInfo"))
		{
			auto typeInfo = t.data->getProperty("typeInfo");
			return analyzer.createTypeInfoObject(typeInfo, "");
		}

		return LightweightDiagnostics::Ptr();
	}

	ProcessorWithScriptingContent* p;
	WeakReference<HiseJavascriptEngine> engine;
	SimpleExpressionData data;
};

HiseJavascriptEngine::SimpleExpressionData HiseJavascriptEngine::SimpleExpressionData::evaluate(HiseJavascriptEngine* engine, EvaluationType evaluationType)
{
	if (getType() != Type::NeedsEvaluation)
		return *this;

	Evaluator ev(engine, *this);
	return ev.evaluate(evaluationType);
}

void HiseJavascriptEngine::addTokensWithContext(mcl::TokenCollection::List& tokens, const UriLocation& location, const String& prevExpression)
{
	auto data = SimpleExpressionData::parse(this, { prevExpression, location });
	data = data.evaluate(this, SimpleExpressionData::EvaluationType::TypeInfo);
	data.addAutocompleteTokens(tokens);
}

}