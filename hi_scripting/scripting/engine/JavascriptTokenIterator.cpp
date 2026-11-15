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

struct HiseJavascriptEngine::RootObject::TokenIterator
{
	TokenIterator(RootObject& r, const String& processorId, const String& code, const String& externalFile) :
		root(r),
		location(processorId, code, externalFile),
		p(code.getCharPointer())
	{
		skip();
	}

	UriLocation createDebugLocation()
	{
		UriLocation loc;
		location.fillLocation(loc, false);
		return loc;
	}

	void skip()
	{
		skipWhitespaceAndComments();
		location.location = p;
		currentType = matchNextToken();
	}

	void skipRestOfStatement()
	{
		int numBraces = 0;

		while (currentType != TokenTypes::semicolon && currentType != TokenTypes::eof)
		{
			if (matchIf(TokenTypes::openBrace))
			{
				numBraces++;
				continue;
			}

			if (currentType == TokenTypes::closeBrace)
			{
				if (--numBraces < 0)
					return;
			}

			skip();
		}
	}

	void skipBlock()
	{
		match(TokenTypes::openBrace);

		int braceCount = 1;

		while (currentType != TokenTypes::eof && braceCount > 0)
		{
			if (currentType == TokenTypes::openBrace) braceCount++;
			else if (currentType == TokenTypes::closeBrace) braceCount--;

			skip();
		}
	}

	void match(TokenType expected)
	{
		if (currentType != expected)
			throwError("Found " + getTokenName(currentType) + " when expecting " + getTokenName(expected));

		skip();
	}

	bool matchIf(TokenType expected) { if (currentType == expected) { skip(); return true; } return false; }
	bool matchesAny(TokenType t1, TokenType t2) const { return currentType == t1 || currentType == t2; }
	bool matchesAny(TokenType t1, TokenType t2, TokenType t3) const { return matchesAny(t1, t2) || currentType == t3; }

	CodeLocation location;
	TokenType currentType;
	var currentValue;

	void clearLastComment()
	{
		lastComment = String();
	}

	String lastComment;

	Identifier parseIdentifier()
	{
		Identifier i;
		if (currentType == TokenTypes::identifier)
			i = currentValue.toString();

		match(TokenTypes::identifier);
		return i;
	}

	VarTypeChecker::WrappedType matchVarType()
	{
		if (matchIf(TokenTypes::colon))
		{
			auto id = parseIdentifier();

#if ENABLE_SCRIPTING_SAFE_CHECKS
			auto t = VarTypeChecker::getTypeFromString(id);

			if (!(bool)t)
				throwError("Unknown type annotation '" + id.toString() + "'. Valid types: int, double, String, Array, Buffer.");

			return t;
#else
			return VarTypeChecker::WrappedType();
#endif
		}

		return VarTypeChecker::WrappedType();
	}

	void skipWhitespaceAndComments()
	{
		for (;;)
		{
			p = p.findEndOfWhitespace();

			if (*p == '/')
			{
				const juce_wchar c2 = p[1];

				if (c2 == '/') { p = CharacterFunctions::find(p, (juce_wchar)'\n'); continue; }

				if (c2 == '*')
				{
					location.location = p;

					lastComment = String(p).upToFirstOccurrenceOf("*/", false, false).fromFirstOccurrenceOf("/**", false, false).trim();

					p = CharacterFunctions::find(p + 2, CharPointer_ASCII("*/"));


					if (p.isEmpty())
						throwError("Unterminated '/*' comment");

					p += 2; continue;
				}
			}

			break;
		}
	}

protected:

	virtual void throwError(const String& errorMessage) const
	{
		location.throwError(root, errorMessage);
	}

	RootObject& root;

private:


	String::CharPointerType p;

	static bool isIdentifierStart(const juce_wchar c) noexcept { return CharacterFunctions::isLetter(c) || c == '_'; }
	static bool isIdentifierBody(const juce_wchar c) noexcept { return CharacterFunctions::isLetterOrDigit(c) || c == '_'; }

	TokenType matchNextToken()
	{
		if (isIdentifierStart(*p))
		{
			String::CharPointerType end(p);
			while (isIdentifierBody(*++end)) {}

			const size_t len = (size_t)(end - p);
#define JUCE_JS_COMPARE_KEYWORD(name, str) if (len == sizeof (str) - 1 && matchToken (TokenTypes::name, len)) return TokenTypes::name;
			JUCE_JS_KEYWORDS(JUCE_JS_COMPARE_KEYWORD)

				currentValue = String(p, end); p = end;
			return TokenTypes::identifier;
		}

		if (p.isDigit())
		{
			if (parseHexLiteral() || parseFloatLiteral() || parseOctalLiteral() || parseDecimalLiteral())
				return TokenTypes::literal;

			throwError("Syntax error in numeric constant");
		}

		if (parseStringLiteral(*p) || (*p == '.' && parseFloatLiteral()))
			return TokenTypes::literal;

#define JUCE_JS_COMPARE_OPERATOR(name, str) if (matchToken (TokenTypes::name, sizeof (str) - 1)) return TokenTypes::name;
		JUCE_JS_OPERATORS(JUCE_JS_COMPARE_OPERATOR)

			if (!p.isEmpty())
				throwError("Unexpected character '" + String::charToString(*p) + "' in source");

		return TokenTypes::eof;
	}

	bool matchToken(TokenType name, const size_t len) noexcept
	{
		if (p.compareUpTo(CharPointer_ASCII(name), (int)len) != 0) return false;
		p += (int)len;  return true;
	}




	bool parseStringLiteral(juce_wchar quoteType)
	{
		if (quoteType != '"' && quoteType != '\'')
			return false;

		Result r(JSON::parseQuotedString(p, currentValue));
		if (r.failed()) throwError(r.getErrorMessage());
		return true;
	}

	bool parseHexLiteral()
	{
		if (*p != '0' || (p[1] != 'x' && p[1] != 'X')) return false;

		String::CharPointerType t(++p);
		int64 v = CharacterFunctions::getHexDigitValue(*++t);
		if (v < 0) return false;

		for (;;)
		{
			const int digit = CharacterFunctions::getHexDigitValue(*++t);
			if (digit < 0) break;
			v = v * 16 + digit;
		}

		currentValue = v; p = t;
		return true;
	}

	bool parseFloatLiteral()
	{
		int numDigits = 0;
		String::CharPointerType t(p);
		while (t.isDigit()) { ++t; ++numDigits; }

		const bool hasPoint = (*t == '.');

		if (hasPoint)
			while ((++t).isDigit())  ++numDigits;

		if (numDigits == 0)
			return false;

		juce_wchar c = *t;
		const bool hasExponent = (c == 'e' || c == 'E');

		if (hasExponent)
		{
			c = *++t;
			if (c == '+' || c == '-')  ++t;
			if (!t.isDigit()) return false;
			while ((++t).isDigit()) {}
		}

		if (!(hasExponent || hasPoint)) return false;

		currentValue = CharacterFunctions::getDoubleValue(p);  p = t;
		return true;
	}

	bool parseOctalLiteral()
	{
		String::CharPointerType t(p);
		int64 v = *t - '0';
		if (v != 0) return false;  // first digit of octal must be 0

		for (;;)
		{
			const int digit = (int)(*++t - '0');
			if (isPositiveAndBelow(digit, 8))        v = v * 8 + digit;
			else if (isPositiveAndBelow(digit, 10))  throwError("Decimal digit in octal constant");
			else break;
		}

		currentValue = v;  p = t;
		return true;
	}

	bool parseDecimalLiteral()
	{
		int64 v = 0;

		for (;; ++p)
		{
			const int digit = (int)(*p - '0');
			if (isPositiveAndBelow(digit, 10))  v = v * 10 + digit;
			else break;
		}

		currentValue = v;
		return true;
	}
};

}