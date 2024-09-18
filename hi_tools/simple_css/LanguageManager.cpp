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

namespace hise
{
namespace simple_css
{
CodeEditorComponent::ColourScheme KeywordDataBase::getColourScheme()
{
	CodeEditorComponent::ColourScheme scheme;
		
	scheme.set("Type", Colour(0xffDDAADD));
	scheme.set("Properties", Colour(0xffbbbbff));
	scheme.set("PseudoClass", Colour(0xffEEAA00));
	scheme.set("AtRule", Colour(0xFFB474C1));
	scheme.set("Keyword", Colours::orange);
	scheme.set("Expression", Colour(0xFFF787F5));
	scheme.set("Class", Colour(0xff88bec5));
	scheme.set("ID", Colour(0xffDDAAAA));
	scheme.set("SpecialCharacters", Colours::white);
	scheme.set("Value", Colour(0xffCCCCEE));
	scheme.set("Comment", Colour(0xff77CC77));
	scheme.set("Important", Colour(0xffBB3333));
	scheme.set("String", Colour(0xffCCCCEE).withMultipliedBrightness(0.8f));
	
	return scheme;
}

KeywordDataBase::KeywordDataBase()
{
	inheritedProperties = { "color", "cursor", "font-family", "font-size", "font-style",
		                    "font-variant", "font-weight", "font", "letter-spacing", "opacity",
	                        "text-align", "text-transform" };
	
	keywords[(int)KeywordType::PseudoClass] = { "hover", "active", "focus", "disabled", "hidden", "before", "after", "root", "checked", "first-child", "last-child" };
	keywords[(int)KeywordType::AtRules] = { "@font-face", "@import" };
	keywords[(int)KeywordType::Type] = { "button", "body", "div", "select", "img", "input", "hr", "label", "table", "th", "tr", "td", "p", "progress", "h1", "h2", "h3", "h4" };
	keywords[(int)KeywordType::ExpressionKeywords] = { "calc", "clamp", "min", "max" };
	keywords[(int)KeywordType::Property] = {
		"::selection",
		"align-items", "align-content", "align-self",
		"background", "background-color", "background-size", "background-position", "background-image",
        "border", "border-width", "border-style", "border-color",
        "border-radius", "border-top-left-radius", "border-top-right-radius", "border-bottom-left-radius", "border-bottom-right-radius",
		"bottom",
        "box-shadow", "box-sizing",
		"color",
		"content",
		"caret-color",
        "cursor",
		"display",
		"flex-wrap", "flex-direction", "flex-grow", "flex-shrink", "flex-basis",
        "font-family", "font-size", "font-weight", "font-stretch",
		"gap",
        "height",
		"justify-content",
		"left",
        "letter-spacing",
		"margin", "margin-top", "margin-left", "margin-right", "margin-bottom",
		"min-width", "max-width", "min-height", "max-height",
        "opacity",
		"object-fit",
		"order",
		"overflow",
		"padding", "padding-top", "padding-left", "padding-right", "padding-bottom",
		"position",
		"right",
        "text-align",
        "text-transform",
		"text-shadow",
		"transition",
		"transform",
		"top",
        "vertical-align",
        "width"
	};

	valueNames["position"] = { "initial", "relative", "absolute", "fixed" };
	valueNames["flex-direction"] = { "row", "row-reverse", "column", "column-reverse" };
	valueNames["flex-wrap"] = { "nowrap", "wrap", "wrap-reverse" };
	valueNames["justify-content"] = { "flex-start", "flex-end", "center", "space-between", "space-around" };
	valueNames["align-items"] = { "stretch", "flex-start", "flex-end", "center" };
	valueNames["align-content"] = { "stretch", "flex-start", "flex-end", "center" };
	valueNames["align-self"] = { "auto", "flex-start", "flex-end", "center", "stretch" };
	valueNames["font-weight"] = { "default", "normal", "unset", "400", "bold", "bolder", "500", "600", "700", "800", "900" };
	valueNames["font-style"] = { "normal", "italic" };
	valueNames["cursor"] = { "default", "pointer", "wait", "crosshair", "text", "copy", "grabbing" };
    valueNames["box-sizing"] = { "initial", "content-box", "border-box" };
    valueNames["transition"] = { "linear", "ease", "ease-in", "ease-in-out" };
    valueNames["text-transform"] = { "none", "capitalize", "uppercase", "lowercase" };
	valueNames["object-fit"] = { "fill", "contain", "cover", "none", "scale-down" };
	valueNames["background-size"] = { "fill", "contain", "cover", "none", "scale-down" };
	functNames["transform"] = { "none", "matrix", "translate", "translateX", "translateY", "translateZ",
								"scale", "scaleX", "scaleY", "scaleZ", "rotate", "rotateX", "rotateY", "rotateZ",
								"skew", "skewX", "skewY" };
    functNames["color"] = { "rgba", "rgb", "hsl", "linear-gradient" };
}

String KeywordDataBase::getKeywordName(KeywordType type)
{
	static const StringArray typeNames({
		"type",
		"property",
		"pseudo-class",
		"at-rule",
		"keywords",
		"expression",
		"numKeywords",
		"undefined"
	});

	return typeNames[(int)type];
}

const StringArray& KeywordDataBase::getValuesForProperty(const String& propertyId) const
{
	static StringArray empty;
	empty.clear();

	if(keywords[(int)KeywordType::Property].contains(propertyId))
	{
		for(auto& v: valueNames)
		{
			if(v.first == propertyId)
				return v.second;
		}

		for(auto& v: functNames)
		{
			if(v.first == propertyId)
				return v.second;
		}
	}

	return empty;
}

void KeywordDataBase::printReport()
{
	String s;
	String nl = "\n";

	s << "CSS Property report" << nl;
	s << "-------------------" << nl << nl;
	
	KeywordDataBase database;

	auto printValues = [&](KeywordType t)
	{
		s << "supported " << getKeywordName(t) << " ids:" << nl;

		for(const auto& v: database.getKeywords(t))
		{
			s << "- " << v << nl;
		}
	};

	printValues(KeywordType::Type);
	printValues(KeywordType::PseudoClass);
	printValues(KeywordType::ExpressionKeywords);
	printValues(KeywordType::Property);

	s << "supported property constants: " << nl;

	for(const auto& ev: database.valueNames)
	{
		s << "- " << ev.first << ":";

		for(const auto& v: ev.second)
			s << " " << v;

		s << nl;
	}

	s << "supported property expressions: " << nl;

	for(const auto& ev: database.functNames)
	{
		s << "- " << ev.first << ":";

		for(const auto& v: ev.second)
			s << " " << v;

		s << nl;
	}

	DBG(s);
}

const StringArray& KeywordDataBase::getKeywords(KeywordType type) const
{
	return keywords[(int)type];
}

KeywordDataBase::KeywordType KeywordDataBase::getKeywordType(const String& t)
{
	int idx = 0;
	for(const auto& s: keywords)
	{
		if(s.contains(t))
			return (KeywordType)idx;

		idx++;
	}

	return KeywordType::Undefined;
}



void LanguageManager::Tokeniser::skipNumberValue(CodeDocument::Iterator& source)
{
	auto c = source.peekNextChar();
	while(!source.isEOF() && ((CharacterFunctions::isLetterOrDigit(c) || c == '-') || c == '%'))
	{
		source.skip();
		c = source.peekNextChar();
	}
}

void LanguageManager::Tokeniser::skipToSemicolon(CodeDocument::Iterator& source)
{
	auto nextChar = source.peekNextChar();
	while(!source.isEOF() && nextChar != ';')
	{
		if(nextChar == '!')
			break;

		if(nextChar == '"' || nextChar == '\'')
			break;
				
		if(nextChar == '/')
		{
			source.skip();

			if(source.peekNextChar() == '*')
			{
				source.previousChar();
				break;
			}
		}

		source.skip();
		nextChar = source.peekNextChar();
	}
			
}

void LanguageManager::Tokeniser::skipComment(CodeDocument::Iterator& source)
{
	while(!source.isEOF())
	{
		auto n = source.nextChar();

		if(n == '*')
		{
			if(!source.isEOF())
			{
				if(source.nextChar() == '/')
					break;
			}
		}
	}
}

void LanguageManager::Tokeniser::skipStringLiteral(CodeDocument::Iterator& source)
{
	auto quoteChar = source.nextChar();

	while(!source.isEOF())
	{
		auto n = source.nextChar();

		if(n == quoteChar)
		{
			return;
		}
	}
}

String LanguageManager::Tokeniser::skipWord(CodeDocument::Iterator& source)
{
	String s;

	auto c = source.peekNextChar();

	while(!source.isEOF() && (CharacterFunctions::isLetterOrDigit(c) || c == '-'))
	{
		s << c;

		source.skip();
		c = source.peekNextChar();
	}

	return s;
}

bool LanguageManager::Tokeniser::isNumber(CodeDocument::Iterator& source)
{
	juce_wchar nextChar = source.peekNextChar();

	return CharacterFunctions::isDigit(nextChar) || nextChar == '-';
}

bool LanguageManager::Tokeniser::isIdentifierStart(CodeDocument::Iterator& source)
{
	juce_wchar nextChar = source.peekNextChar();

	return CharacterFunctions::isLetter(nextChar);
}

int LanguageManager::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	source.skipWhitespace();

	auto c = source.peekNextChar();

	bool isClass = false;
	bool isID = false;
	bool isAtRule = false;
	bool wasProperty = false;

	if(String("{};").containsChar(c))
	{
		source.skip();
		return (int)Token::SpecialCharacters;
	}

	if(c == '\'' || c == '"')
	{
		skipStringLiteral(source);
		return (int)Token::StringLiteral;
	}
	if(c == '!')
	{
		source.skip();
		auto w = skipWord(source);

		if(w == "important")
			return (int)Token::Important;
		else
			return (int)Token::Value;
	}
	if(c == '/')
	{
		source.skip();

		if(source.peekNextChar() == '*')
		{
			skipComment(source);
			return (int)Token::Comment;
		}
	}

	if(c == '.')
	{
		isClass = true;
		source.skip();
	}
	if(c == '*')
	{
		source.skip();
		return (int)Token::Type;
	}
	if(c == '@')
	{
		isAtRule = true;
		source.skip();
	}
	if(c == '#')
	{
		isID = true;
		source.skip();
	}
	if(c == ':')
	{
		wasProperty = true;
		source.skip();

		if(source.peekNextChar() == ':')
		{
			source.skip();
			skipWord(source);
			return (int)Token::PseudoClass;
		}
	}

	if(isIdentifierStart(source))
	{
		auto word = skipWord(source);

		if(isClass)
			return (int)Token::Class;

		if(isID)
			return (int)Token::ID;

		if(isAtRule)
			return (int)Token::AtRule;

		auto keyType = database->getKeywordType(word);

		if(keyType != KeywordDataBase::KeywordType::Undefined)
			return (int)keyType;

		if (wasProperty)
		{
			skipToSemicolon(source);
			return (int)Token::Value;
		}
				
		return (int)Token::Value;
	}

	if(wasProperty)
	{
		skipWord(source);//skipToSemicolon(source);
		return (int)Token::Value;
	}

	if(isNumber(source))
	{
		skipNumberValue(source);
		return (int)Token::Value;
	}

	source.skip();
	return (int)Token::SpecialCharacters;
			
}

CodeEditorComponent::ColourScheme LanguageManager::Tokeniser::getDefaultColourScheme()
{
	return database->getColourScheme();
}

LanguageManager::LanguageManager(mcl::TextDocument& doc_):
	doc(doc_)
{
	this->hashIsPreprocessor = false;	
}

void LanguageManager::CssTokens::addTokens(mcl::TokenCollection::List& tokens)
{
	StringArray names({
		"Type",
		"Property",
		"PseudoClass",
		"AtRules",
		"ReservedKeywords",
		"Expression operator"
	});

	auto colours = database.getColourScheme();

	for(int i = 0; i < (int)KeywordDataBase::KeywordType::numKeywords; i++)
	{
		for(auto& s: database.getKeywords((KeywordDataBase::KeywordType)i))
		{
			auto d = new mcl::TokenCollection::Token(s);

			d->c = colours.types[i].colour;
			d->priority = i;
			d->markdownDescription << "`" << s << "` (" << names[i] << ")";

			if(i == (int)KeywordDataBase::KeywordType::ExpressionKeywords)
			{
				d->tokenContent << "(op1, op2)";
			}

			tokens.add(d);
		}
	}

	for(const auto& enumValues: database.valueNames)
	{
		for(const auto& v: enumValues.second)
		{
			auto t = new mcl::TokenCollection::Token(v);
			t->c = colours.types[8].colour;
			t->priority = -8;
			t->markdownDescription << v << " (property value for `" << enumValues.first << "`)";
			tokens.add(t);
		}
	}

	for(const auto& enumValues: database.functNames)
	{
		for(const auto& v: enumValues.second)
		{
			auto t = new mcl::TokenCollection::Token(v);
			t->c = colours.types[8].colour;
			t->priority = -8;
			t->tokenContent << "(expr)";
			t->markdownDescription << v << " (property expression for `" << enumValues.first << "`)";
			tokens.add(t);
		}
	}
}

void LanguageManager::setupEditor(mcl::TextEditor* editor)
{
	addTokenProviders(editor->tokenCollection.get());
}

void LanguageManager::addTokenProviders(mcl::TokenCollection* t)
{
	t->addTokenProvider(new mcl::SimpleDocumentTokenProvider(doc.getCodeDocument()));
	t->addTokenProvider(new CssTokens());

	t->updateIfSync();
}

}
}


