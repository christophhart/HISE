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
using namespace juce;

namespace debug
{

using namespace mcl;

struct MathFunctionProvider : public TokenCollection::Provider
{
	struct MathFunction : public TokenCollection::Token
	{
		MathFunction(FunctionData f) :
			Token(f.getSignature().replace("Math::", "Math."))
		{
			markdownDescription = f.description;
		};

		Range<int> getSelectionRangeAfterInsert() const override
		{
			auto c = getCodeToInsert("");

			auto start = c.indexOf("(");
			auto end = c.indexOf(")");

			return { start, end };
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const override
		{
			if (previousToken != "Math.")
				return false;

			return matchesInput(input, getCodeToInsert(""));
		}

		String getCodeToInsert(const String& input) const override
		{
			return tokenContent.fromFirstOccurrenceOf(".", false, false);
		}
	};

	void addTokens(TokenCollection::List& tokens);
};

struct KeywordProvider : public TokenCollection::Provider
{
	struct KeywordToken : public TokenCollection::Token
	{
		KeywordToken(const String& s) :
			Token(s)
		{
			c = Colours::red;
		};

		bool matches(const String& input, const String& previousToken, int lineNumber) const override
		{
			if (previousToken.isNotEmpty())
				return false;

			return matchesInput(input, tokenContent);
		}

		
	};

	void addTokens(TokenCollection::List& tokens)
	{
		tokens.add(new KeywordToken("double"));
		tokens.add(new KeywordToken("float"));
		tokens.add(new KeywordToken("span"));
		tokens.add(new KeywordToken("return"));
		tokens.add(new KeywordToken("template"));
		tokens.add(new KeywordToken("typename"));
		tokens.add(new KeywordToken("break"));
		tokens.add(new KeywordToken("continue"));
		tokens.add(new KeywordToken("namespace"));
		tokens.add(new KeywordToken("enum"));
		tokens.add(new KeywordToken("struct"));
		tokens.add(new KeywordToken("class"));
		tokens.add(new KeywordToken("private"));
		tokens.add(new KeywordToken("protected"));
		tokens.add(new KeywordToken("public"));
	}
};

struct SymbolProvider : public TokenCollection::Provider
{
	SymbolProvider(CodeDocument& d):
		doc(d)
	{
		
	}

	void addTokens(TokenCollection::List& tokens)
	{
		GlobalScope s;
		Compiler c(s);
		Types::SnexObjectDatabase::registerObjects(c, 2);
		auto obj = c.compileJitObject(doc.getAllContent());

		tokens.addArray(c.getNamespaceHandler().getTokenList());
	}

	StringArray sa;
	CodeDocument& doc;
};


}


}