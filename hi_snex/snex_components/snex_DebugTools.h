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
			priority = 200;
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
		tokens.add(new KeywordToken("using"));
		tokens.add(new KeywordToken("protected"));
		tokens.add(new KeywordToken("public"));
	}
};

struct TemplateProvider : public TokenCollection::Provider
{
	struct TemplateToken : public TokenCollection::Token
	{
		TemplateToken(const TemplateObject& s):
			Token(getTokenString(s))
		{
			priority = 100;

			markdownDescription = s.description;

			c = Colours::orange;
		};

#if 0
		virtual Range<int> getSelectionRangeAfterInsert() const 
		{ 
			return { tokenContent.indexOfChar('<'), tokenContent.lastIndexOfChar('>') }; 
		}
#endif

		static String getTokenString(const TemplateObject& o)
		{
			String s;
			s << o.id.toString();
			s << TemplateParameter::ListOps::toString(o.argList, true);

			return s;
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const override
		{
			if (previousToken.isNotEmpty())
				return false;

			return matchesInput(input, tokenContent);
		}

		int pStart, pEnd;
	};

	void addTokens(TokenCollection::List& tokens)
	{
		GlobalScope s;
		Compiler c(s);
		Types::SnexObjectDatabase::registerObjects(c, 2);

		for (auto id : c.getNamespaceHandler().getTemplateClassTypes())
			tokens.add(new TemplateToken(id));
	}
};

struct ComplexTypeProvider : public TokenCollection::Provider
{
	

	
};

struct PreprocessorMacroProvider : public TokenCollection::Provider
{
	PreprocessorMacroProvider(CodeDocument& d) :
		doc(d)
	{

	}

	struct PreprocessorToken : public TokenCollection::Token
	{
		PreprocessorToken(const String& name, const String& code, const String& description, int lineNumber):
			Token(name),
			definitionLine(lineNumber)
		{
			markdownDescription = description;
			codeToInsert = code;
		}

		String getCodeToInsert(const String& input)
		{
			return codeToInsert;
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const override
		{
			if (previousToken.isEmpty())
			{
				return lineNumber > definitionLine && matchesInput(input, tokenContent);
			}
			
			return false;
		}

		String codeToInsert;
		int definitionLine;

	};

	void addTokens(TokenCollection::List& tokens) override;

	CodeDocument& doc;
};

struct SymbolProvider : public TokenCollection::Provider
{
	struct ComplexMemberToken : public TokenCollection::Token
	{
		ComplexMemberToken(SymbolProvider& parent_, ComplexType::Ptr p_, FunctionData& f);

		bool matches(const String& input, const String& previousToken, int lineNumber) const override;

		SymbolProvider& parent;
		ComplexType::Ptr p;

		String codeToInsert;

		String getCodeToInsert(const String& input) const override
		{
			return codeToInsert;
		}
	};

	SymbolProvider(CodeDocument& d):
		doc(d),
		c(s)
	{
		
	}

	void addTokens(TokenCollection::List& tokens)
	{
		c.reset();
		Types::SnexObjectDatabase::registerObjects(c, 2);

		c.compileJitObject(doc.getAllContent());

		auto ct = c.getNamespaceHandler().getComplexTypeList();

		for (auto c : ct)
		{
			FunctionClass::Ptr fc = c->getFunctionClass();
			
			for (auto id : fc->getFunctionIds())
			{
				Array<FunctionData> fData;
				fc->addMatchingFunctions(fData, id);

				for(auto& f: fData)
					tokens.add(new ComplexMemberToken(*this, c, f));
			}
		}

		auto l = c.getNamespaceHandler().getTokenList();

		for (auto a : l)
		{
			DBG(a->getCodeToInsert(""));
		}

		tokens.addArray(l);
	}

	GlobalScope s;
	Compiler c;

	ReferenceCountedArray<jit::ComplexType> typeList;
	StringArray sa;
	CodeDocument& doc;
};


}


}