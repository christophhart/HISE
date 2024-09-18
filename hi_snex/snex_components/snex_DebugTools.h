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

struct Helpers
{
	static mcl::FoldableLineRange::List createLineRanges(const CodeDocument& doc);
};

using namespace mcl;

struct ApiDatabase
{
	ApiDatabase()
	{
		v = ValueTree::readFromData(SnexApi::apivaluetree_dat, SnexApi::apivaluetree_datSize);
	}

	static Identifier getPath(const NamespacedIdentifier& id)
	{
		return Identifier(id.toString().replace("::", "_"));
	}

	bool addDocumentation(TokenCollection::TokenPtr p, const NamespacedIdentifier& id, String member)
	{
		auto parent = getPath(id);

		auto classTree = v.getChildWithName(parent);

		
		if (member.isEmpty() && classTree.isValid())
		{
			p->markdownDescription = classTree.getProperty("description");
			return true;
		}
		else
		{
			member = member.upToFirstOccurrenceOf("(", false, false);

			auto mTree = classTree.getChildWithProperty("name", member);

			if (mTree.isValid())
			{
				p->markdownDescription = mTree.getProperty("description");
				return true;
			}
		}
		return false;
	}

	

	using Instance = SharedResourcePointer<ApiDatabase>;

private:

	ValueTree v;
};

struct FourColourScheme
{
	enum Types
	{
		Keyword,
		Method,
		Classes,
		Preprocessor
	};

	static Colour getColour(Types index)
	{
		switch (index)
		{
			case Keyword:		return Colour(0xffbbbbff);
			case Classes:		return Colour(0xFF70FFE4);
			case Preprocessor:	return Colour(0xFFB5C792);
			case Method:		return Colour(0xFFA0FF51);
		}

		return {};
	}
};

struct SnexTokenProvider: public TokenCollection::Provider
{
    bool shouldAbortTokenRebuild(Thread* t) const override
    {
        return t != nullptr && t->threadShouldExit();
    }
};

struct MathFunctionProvider : public SnexTokenProvider
{
	struct MathFunction : public TokenCollection::Token
	{
		MathFunction(FunctionData f) :
			Token(f.getSignature().replace("Math::", "Math."))
		{
			markdownDescription = f.description;
			c = FourColourScheme::getColour(FourColourScheme::Method);
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

struct KeywordProvider : public SnexTokenProvider
{
    
    
	struct KeywordToken : public TokenCollection::Token
	{
		KeywordToken(const String& s, int priority_=100) :
			Token(s)
		{
			c = FourColourScheme::getColour(FourColourScheme::Keyword);
			priority = priority_;
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
		tokens.add(new KeywordToken("double", 200));
		tokens.add(new KeywordToken("float", 200));
		tokens.add(new KeywordToken("return"));
		tokens.add(new KeywordToken("template", 200));
		tokens.add(new KeywordToken("typename", 200));
		tokens.add(new KeywordToken("break"));
		tokens.add(new KeywordToken("continue"));
		tokens.add(new KeywordToken("namespace"));
		tokens.add(new KeywordToken("enum"));
		tokens.add(new KeywordToken("struct", 200));
		tokens.add(new KeywordToken("class", 200));
		tokens.add(new KeywordToken("private"));
		tokens.add(new KeywordToken("using"));
		tokens.add(new KeywordToken("protected"));
		tokens.add(new KeywordToken("public"));
	}
};

struct TemplateProvider : public SnexTokenProvider
{
	struct TemplateToken : public TokenCollection::Token
	{
		TemplateToken(const TemplateObject& s):
			Token(getTokenString(s))
		{
			priority = 120;

			markdownDescription = s.description;

			c = FourColourScheme::getColour(FourColourScheme::Classes);
		};

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

	void addTokens(TokenCollection::List& tokens);
};

struct ComplexTypeProvider : public SnexTokenProvider
{
	

	
};

struct PreprocessorMacroProvider : public SnexTokenProvider
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
			c = FourColourScheme::getColour(FourColourScheme::Preprocessor);
		}

		PreprocessorToken(const ExternalPreprocessorDefinition& def) :
			Token(def.name)
		{
			markdownDescription << "Value: `" << def.value << "`";
			codeToInsert = def.name;
			definitionLine = 0;
			c = FourColourScheme::getColour(FourColourScheme::Preprocessor);
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

struct SymbolProvider : public SnexTokenProvider
{
	struct ComplexMemberToken : public TokenCollection::Token
	{
		ComplexMemberToken(SymbolProvider& parent_, ComplexType::Ptr p_, FunctionData& f);

		bool matches(const String& input, const String& previousToken, int lineNumber) const override;

		bool equals(const Token* other) const override
		{
			if (auto co = dynamic_cast<const ComplexMemberToken*>(other))
			{
				if (co->p != p)
					return false;
			}

			return Token::equals(other);
		}

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
		c(s),
		nh(c.getNamespaceHandlerReference())
	{
	}

	void addTokens(TokenCollection::List& tokens);

	static void addDocumentation(const ValueTree& v, TokenCollection::TokenPtr t, const String& methodName)
	{
		auto c = v.getChildWithProperty("name", methodName);

		if (c.isValid())
			t->markdownDescription = c["description"].toString();
	}

	GlobalScope s;
	Compiler c;
	NamespaceHandler::Ptr nh;

	ReferenceCountedArray<jit::ComplexType> typeList;
	StringArray sa;
	CodeDocument& doc;
};


struct SnexLanguageManager : public mcl::LanguageManager,
							 public snex::jit::DebugHandler
{
	SnexLanguageManager(CodeDocument& d, snex::GlobalScope& s) :
		doc(d)
	{
		s.addDebugHandler(this);
	};

	Array<LanguageManager::InplaceDebugValue> debugValues;

	void recompiled() override
	{
		debugValues.clear();
	}

	Identifier getLanguageId() const override { return mcl::LanguageIds::SNEX; }

	void logMessage(int level, const juce::String& s) override
	{
		if (s.startsWith("Line"))
		{
			auto lineNumber = s.fromFirstOccurrenceOf("Line ", false, false).getIntValue() - 1;
			auto value = s.fromFirstOccurrenceOf(":", false, false).trim().replaceCharacter('\t', ' ');

			if (value.length() > 60)
				value = value.substring(0, 60) + "(...)";

			for (auto& v : debugValues)
			{
				if (v.location.getLineNumber() == lineNumber)
				{
					v.value = value;
					return;
				}
			}

			InplaceDebugValue newValue;
			newValue.value = value;

			newValue.location = CodeDocument::Position(doc, lineNumber, 99);
			debugValues.add(std::move(newValue));
			(debugValues.getRawDataPointer() + debugValues.size() - 1)->location.setPositionMaintained(true);
		}
	}

	bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const override
	{
		values.addArray(debugValues);
		return !debugValues.isEmpty();
	}

	CodeTokeniser* createCodeTokeniser() override
	{
		return new CPlusPlusCodeTokeniser();
	}

	void processBookmarkTitle(juce::String& bookmarkTitle) override
	{

	}

	mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc) override;

	void addTokenProviders(mcl::TokenCollection* t) override
	{
		t->addTokenProvider(new debug::KeywordProvider());
		t->addTokenProvider(new debug::SymbolProvider(doc));
		t->addTokenProvider(new debug::TemplateProvider());
		t->addTokenProvider(new debug::MathFunctionProvider());
		t->addTokenProvider(new debug::PreprocessorMacroProvider(doc));
	}

	void setupEditor(mcl::TextEditor* e) override
	{
		e->tokenCollection = new mcl::TokenCollection(getLanguageId());
		addTokenProviders(e->tokenCollection.get());
	}

	CodeDocument& doc;
};

}

struct LiveCodePopup
{
	using Argument = std::tuple<snex::jit::Symbol, snex::VariableStorage>;

	template <typename T> static Argument createArg(T value, const char* s)
	{
		TypeInfo type(snex::Types::Helpers::getTypeFromTypeId<T>());

		String variableName(s);

		auto id = snex::NamespacedIdentifier::fromString(variableName.replaceCharacters(".->", "___"));
		snex::jit::Symbol sym(id, type);
		snex::VariableStorage v(value);
		return { sym, v };
	}

	
	
    struct ItemBase
    {
        virtual ~ItemBase() {};
        
        virtual VariableStorage evaluate(const Array<Argument>& args) = 0;
    };

    struct Data;
    
	static Data* instance;

	static Data* getInstance();

    static ItemBase* getItem(const String& expression, const char* file, int lineNumber, Types::ID returnType, const Array<Argument>& args);
    
	template <typename ReturnType> static ReturnType show(const String& expression, const char* file, int lineNumber, const Array<Argument>& args)
	{
		auto returnType = Types::Helpers::getTypeFromTypeId<ReturnType>();
        auto item = getItem(expression, file, lineNumber, returnType, args);
		return (ReturnType)item->evaluate(args);
	}
};

#define LIVE_ARG(x) snex::LiveCodePopup::createArg(x, #x)
#define LIVE_ARGS0() {}
#define LIVE_ARGS1(arg1) { LIVE_ARG(arg1) }
#define LIVE_ARGS2(arg1, arg2) { LIVE_ARG(arg1), LIVE_ARG(arg2) }
#define LIVE_ARGS3(arg1, arg2, arg3) { LIVE_ARG(arg1),LIVE_ARG(arg2),LIVE_ARG(arg3) }
#define LIVE_ARGS4(arg1, arg2, arg3, arg4) { LIVE_ARG(arg1),LIVE_ARG(arg2), LIVE_ARG(arg3), LIVE_ARG(arg4) }
#define LIVE_EXPRESSION_P(rt, code, args) snex::LiveCodePopup::show<rt>(code, __FILE__, __LINE__ - 1, args)

//LIVE_EXPRESSION(double, "Math.max(input, 2.0)", input)
#define LIVE_EXPRESSION0(rt, code)							LIVE_EXPRESSION_P(rt, code, LIVE_ARGS0())
#define LIVE_EXPRESSION1(rt, code, arg1)					LIVE_EXPRESSION_P(rt, code, LIVE_ARGS1(arg1))
#define LIVE_EXPRESSION2(rt, code, arg1, arg2)				LIVE_EXPRESSION_P(rt, code, LIVE_ARGS2(arg1, arg2))
#define LIVE_EXPRESSION3(rt, code, arg1, arg2, arg3)		LIVE_EXPRESSION_P(rt, code, LIVE_ARGS3(arg1, arg2, arg3))
#define LIVE_EXPRESSION4(rt, code, arg1, arg2, arg3, arg4)  LIVE_EXPRESSION_P(rt, code, LIVE_ARGS4(arg1, arg2, arg3, arg4))



}
