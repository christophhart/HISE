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

struct HiseJavascriptEngine::RootObject::IncludeParser : private TokenIterator
{
	enum class Category
	{
		Scope,
		Include,
		Parameter,
		Namespace,
		Loop,
		Iterator
	};

	IncludeParser(RootObject& r, UriLocationTree::Ptr parent, const String& includeUri, HiseJavascriptPreprocessor::Ptr preprocessor_) :
		TokenIterator(r, parent->processorId, getFileContent(r, parent.get(), includeUri), includeUri),
		thisNode(parent),
		preprocessor(preprocessor_),
		scopeRoot(new UriLocationTree(nullptr))
	{
		scopeRoot->processorId = thisNode->processorId;
		scopeRoot->uri = includeUri;
		scopeRoot->col = 0;
		scopeRoot->line = 0;
		scopeRoot->charIndex = 0;

		setCategory(scopeRoot.get(), Category::Scope);

		currentScope = scopeRoot.get();
	}

	std::pair<UriLocationTree::Ptr, UriLocationTree::Ptr> parseIncludeStatements()
	{
		if (parsed)
			return { thisNode, scopeRoot };

		while (currentType != TokenTypes::eof)
		{
			if (matchIf(TokenTypes::identifier))
			{
				if(isIncludeMethod(Identifier(currentValue.toString())))
					parseIncludeMethod();
			}

			if (matchIf(TokenTypes::namespace_))
				parseNamespace();
			else if (matchIf(TokenTypes::include_))
				parseInclude();
			else if (getDefinitionTokens().contains(currentType))
				parseDefinition();
			else if (matchIf(TokenTypes::for_))
				parseForLoop();
			else if (matchesAny(TokenTypes::openBrace, TokenTypes::closeBrace))
				parseScope();
			else
				skip();
		}

		scopeRoot->closeLocation = location.createUriLocation();

		parsed = true;
		return { thisNode, scopeRoot };
	}

	Array<UriError> getIncludeErrors() const { return includeErrors; }

private:

	void parseIncludeMethod()
	{
		if (matchIf(TokenTypes::openParen))
		{
			auto uri = currentValue.toString();
			
			if (matchIf(TokenTypes::literal))
			{
				if (matchIf(TokenTypes::closeParen))
				{
					UriLocationTree::Ptr np = new UriLocationTree(thisNode.get());
					location.fillLocation(*np);
					np->uri = uri;
					np->id = uri;
					setCategory(np.get(), Category::Include);
				}
			}
		}
	}

	static Identifier getIdForCategory(Category c)
	{
		switch (c)
		{
		case Category::Scope:     return Identifier("scope");
		case Category::Include:   return Identifier("include");
		case Category::Parameter: return Identifier("parameter");
		case Category::Namespace: return Identifier("namespace");
		case Category::Loop:	  return Identifier("loop");
		case Category::Iterator:  return Identifier("iterator");
		}
	}

	void setCategory(UriLocationTree::WeakPtr obj, Category c)
	{
		obj->category = getIdForCategory(c);
	}

	void parseNamespace()
	{
		currentNamespaceId = parseIdentifier();

		attachComment(currentScope);
		parseScope();

		setCategory(currentScope, Category::Namespace);
		currentScope->id = currentNamespaceId;
	}

	void attachComment(UriLocationTree::WeakPtr obj, const String& commentToUse = {})
	{
		if (commentToUse.isNotEmpty())
		{
			obj->data->setProperty("comment", lastComment);
			return;
		}
		
		if (lastComment.isNotEmpty())
		{
			obj->data->setProperty("comment", lastComment);
			clearLastComment();
		}
	}

	void parseForLoop()
	{
		clearLastComment();
		auto blockStart = location;

		match(TokenTypes::openParen);

		enum LoopType
		{
			Undefined,
			Iterator,
			Increment
		};

		LoopType lt = LoopType::Undefined;

		if (matchesAny(TokenTypes::var, TokenTypes::local_, TokenTypes::register_var))
			lt = LoopType::Increment;
			
		auto iteratorLocation = location;

		auto iteratorId = parseIdentifier();

		if (matchIf(TokenTypes::in))
		{
			if (lt != LoopType::Undefined)
				throwError("loop syntax error");

			lt = LoopType::Iterator;
		}
		else
			lt = LoopType::Increment;


		if (lt == LoopType::Increment)
		{
			skipRestOfStatement();
			skipRestOfStatement();
		}

		while (!matchesAny(TokenTypes::eof, TokenTypes::closeParen))
			skip();

		match(TokenTypes::closeParen);

		auto createIterator = [&]()
		{
			UriLocationTree::Ptr iterator = new UriLocationTree(currentScope);

			iteratorLocation.fillLocation(*iterator);
			iterator->id = iteratorId;
			setCategory(iterator.get(), Category::Iterator);
		};

		if (currentType == TokenTypes::openBrace)
		{
			parseScope();
			setCategory(currentScope, Category::Loop);
			
			blockStart.fillLocation(*currentScope);
			createIterator();
			
		}
		else
		{
			// create a dummy scope for the next non-block statement
			auto scopeLocation = location.createUriLocation();

			currentScope = new UriLocationTree(currentScope.get());
			setCategory(currentScope, Category::Loop);
			blockStart.fillLocation(*currentScope);

			createIterator();

			while (!matchesAny(TokenTypes::semicolon, TokenTypes::eof))
			{
				skip();
			}

			matchIf(TokenTypes::semicolon);

			currentScope->closeLocation = location.createUriLocation();
			currentScope = currentScope->getParent();
		}
	}

	UriLocationTree::Ptr parseParameter()
	{
		UriLocationTree::Ptr p = new UriLocationTree(currentScope);
		location.fillLocation(*p);
		p->id = parseIdentifier();
		setCategory(p.get(), Category::Parameter);

		if (auto wt = matchVarType())
			p->data->setProperty("typeInfo", VarTypeChecker::getTypeName(wt).toString());

		matchIf(TokenTypes::comma);

		return p;
	}

	void parseDefinition()
	{
		auto category = String(currentType);
		auto blockStart = location;
		auto isFunction = matchesAny(TokenTypes::function, TokenTypes::inline_);

		jassert(getDefinitionTokens().contains(currentType));
		skip();
		matchIf(TokenTypes::var);
		matchIf(TokenTypes::function);

		UriLocationTree::Ptr thisDef;

		Identifier id;

		auto wt = matchVarType();

		UriLocation tokenLocation;

		if (isFunction)
		{
			if (matchIf(TokenTypes::openBracket))
			{
				while (!matchesAny(TokenTypes::eof, TokenTypes::closeBracket))
					skip();

				match(TokenTypes::closeBracket);
			}

			location.fillLocation(tokenLocation);

			if (currentType == TokenTypes::identifier)
				id = parseIdentifier();
			else
				id = "anonymousFunction"; // allow anonymous functions as callback parameter
		}
		else
		{
			location.fillLocation(tokenLocation);
			id = parseIdentifier();
		}

		if (isFunction)
		{
			match(TokenTypes::openParen);

			UriLocationTree::List parameters;

			{
				UriLocationTree::Ptr thisParameter = new UriLocationTree(currentScope);
				location.fillLocation(*thisParameter);
				thisParameter->id = "this";
				thisParameter->category = "this";
				parameters.add(thisParameter);
			}

			while (!matchesAny(TokenTypes::closeParen, TokenTypes::eof))
				parameters.add(parseParameter());

			match(TokenTypes::closeParen);

			auto lc = lastComment;

			parseScope();

			thisDef = currentScope.get();

			attachComment(thisDef.get(), lc);

			for (auto p : parameters)
				p->setParent(thisDef.get());

			thisDef->setToStringFunction([](const UriLocationTree& t)
			{
				String signature;
				signature << t.id << "(";

				t.forEachChildWithCategory(getIdForCategory(Category::Parameter), [&](const UriLocationTree& t)
				{
					signature << t.id;

					if (t.data->hasProperty("typeInfo"))
						signature << ":" << t.data->getProperty("typeInfo").toString();

					signature << ", ";
					return false;
				});

				signature = signature.upToLastOccurrenceOf(", ", false, false);
				signature << ")";
				return signature;
			});
		}
		else
		{
			matchIf(TokenTypes::assign);

			if (currentType == TokenTypes::function)
			{
				parseDefinition();

				jassert(currentScope->id == Identifier("anonymousFunction"));

				currentScope->id = id;
				return;
			}

			thisDef = new UriLocationTree(currentScope);
			attachComment(thisDef.get());
		}

		blockStart.fillLocation(*thisDef);

		if (wt)
		{
			auto typeId = VarTypeChecker::getTypeName(wt);
			thisDef->data->setProperty("typeInfo", typeId.toString());
		}

		thisDef->id = id;
		thisDef->category = category;

		thisDef->setTokenLocation(tokenLocation);
	}

	void parseInclude()
	{
		if (!currentNamespaceId.isNull())
			throwError("Can't include within namespace");

		match(TokenTypes::openParen);
		match(TokenTypes::literal);

		auto filePath = currentValue.toString().unquoted();

		match(TokenTypes::closeParen);
		match(TokenTypes::semicolon);

		UriLocationTree::Ptr np = new UriLocationTree(thisNode.get());

		thisNode->forEachParent([&](const UriLocationTree& t)
		{
			if (t.category == Identifier("include") &&
				t.id.toString() == filePath)
			{
				throwError("Recursive include " + filePath.quoted());
			}

			return false;
		});

		thisNode->getRoot()->forEach([&](const UriLocationTree& t)
		{
			if (t.category == Identifier("include") &&
				t.id.toString() == filePath)
			{
				throwError("Duplicate include " + filePath.quoted());
			}

			return false;
		});

		location.fillLocation(*np);

		IncludeParser cp(root, np, filePath, preprocessor);

		std::pair<UriLocationTree::Ptr, UriLocationTree::Ptr> inc;

		try
		{
			inc = cp.parseIncludeStatements();
			includeErrors.addArray(cp.getIncludeErrors());
		}
		catch (UriError& e)
		{
			includeErrors.add(e);
			inc = { cp.thisNode, cp.scopeRoot };
		}

		setCategory(inc.first.get(), Category::Include);
		inc.first->id = filePath;

		inc.second->id = filePath;
		inc.second->setParent(scopeRoot.get());
		inc.second->setParent(scopeRoot.get());
	}

	void parseScope()
	{
		auto scopeLocation = location.createUriLocation();

		if (matchIf(TokenTypes::openBrace))
		{
			currentScope = new UriLocationTree(currentScope.get());
			setCategory(currentScope, Category::Scope);
			currentScope->fillLocation(scopeLocation);
		}
		else
		{
			match(TokenTypes::closeBrace);
			currentScope->closeLocation = location.createUriLocation();

			if (currentScope->id == currentNamespaceId)
				currentNamespaceId = {};

			currentScope = currentScope->getParent();

			if (currentScope == nullptr)
				throwError("Can't pop scope");
		}
	}

	static StringArray getDefinitionTokens()
	{
		return {
			TokenTypes::const_,
			TokenTypes::function,
			TokenTypes::var,
			TokenTypes::register_var,
			TokenTypes::const_,
			TokenTypes::inline_,
			TokenTypes::local_,
			TokenTypes::global_
		};
	}

	void throwError(const String& errorMessage) const override
	{
		UriError e;
		e.fillLocation(location.createUriLocation());
		e.errorMessage = errorMessage;
		e.errorType = UriError::ErrorType::Error;

		thisNode->forEachParent([&](const UriLocationTree& p)
		{
			e.callstack.push_back({ "include", p });
			return false;
		});

		throw e;
	}

	static void throwErrorFromLocation(UriLocationTree::WeakPtr loc, const String& errorMessage)
	{
		UriError e;
		e.fillLocation(*loc);
		e.errorMessage = errorMessage;

		loc->forEachParent([&](const UriLocationTree& p)
		{
			if (p != *loc)
				e.callstack.push_back({ "include", p });

			return false;
		});

		throw e;
	}

	static bool isIncludeMethod(const Identifier& id)
	{
		static const Array<Identifier> includeMethods("setStyleSheet", "setShader");
		return includeMethods.contains(id);
	}

	static String getFileContent(RootObject& r, UriLocationTree::WeakPtr parent, const String& includeUri)
	{
		if (parent == nullptr)
		{
			jassertfalse;
			return {};
		}

		auto content = r.hiseSpecialData.processor->getUriContent(includeUri);

		if (content.first)
			return content.second;

		String errorMessage = content.second;
		throwErrorFromLocation(parent, errorMessage);
	}

	Array<UriError> includeErrors;

	UriLocationTree::Ptr scopeRoot;
	UriLocationTree::WeakPtr currentScope;

	Identifier currentNamespaceId;
	HiseJavascriptPreprocessor::Ptr preprocessor;
	bool parsed = false;
	UriLocationTree::Ptr thisNode;
};


std::pair<UriLocationTree::Ptr, UriLocationTree::Ptr> HiseJavascriptEngine::parseIncludeTree()
{
	PerformanceCounter pc("parseInclude", 1);
	pc.start();

	root->lastIncludeTree = nullptr;
	root->lastScopeTree = nullptr;

	UriLocationTree::Ptr np = new UriLocationTree(nullptr);

	auto jp = root->hiseSpecialData.processor;
	np->uri = "onInit";
	np->processorId = jp->getUriProcessorId();
	np->category = Identifier("include");
	np->id = "onInit";

	try
	{
		jp->errorProvider.clearUriErrors(UriErrorLogger::SourceType::Include);
		RootObject::IncludeParser ip(*root, np, "onInit", preprocessor);

		auto rv = ip.parseIncludeStatements();

		auto includeErrors = ip.getIncludeErrors();

		if (includeErrors.isEmpty())
		{
			auto result = UriError::ok();

			if (rv.first != nullptr)
			{
				auto ok = rv.first->checkSanity();

				if (ok.wasOk())
					root->lastIncludeTree = rv.first;
				else
					result = ok;
			}

			if (rv.second != nullptr)
			{
				auto ok = rv.second->checkSanity();

				if (ok.wasOk())
					root->lastScopeTree = rv.second;
				else
					result = ok;
			}

			jp->errorProvider.logUriError(UriErrorLogger::SourceType::Include, result);
		}
		else
		{
			for (auto e : ip.getIncludeErrors())
				jp->errorProvider.logUriError(UriErrorLogger::SourceType::Include, e);

			root->lastIncludeTree = rv.first;
			root->lastScopeTree = rv.second;
		}

		pc.stop();
		return rv;

	}
	catch (UriError& e)
	{
		jp->errorProvider.logUriError(UriErrorLogger::SourceType::Include, e);
		return { nullptr, nullptr };
	}
}


}