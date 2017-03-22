/*
  ==============================================================================

    TokenIterator.h
    Created: 27 Feb 2017 2:44:11pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef TOKENITERATOR_H_INCLUDED
#define TOKENITERATOR_H_INCLUDED

#include <typeindex>

#define NATIVE_JIT_OPERATORS(X) \
    X(semicolon,     ";")        X(dot,          ".")       X(comma,        ",") \
    X(openParen,     "(")        X(closeParen,   ")")       X(openBrace,    "{")    X(closeBrace, "}") \
    X(openBracket,   "[")        X(closeBracket, "]")       X(colon,        ":")    X(question,   "?") \
    X(typeEquals,    "===")      X(equals,       "==")      X(assign_,       "=") \
    X(typeNotEquals, "!==")      X(notEquals,    "!=")      X(logicalNot,   "!") \
    X(plusEquals,    "+=")       X(plusplus,     "++")      X(plus,         "+") \
    X(minusEquals,   "-=")       X(minusminus,   "--")      X(minus,        "-") \
    X(timesEquals,   "*=")       X(times,        "*")       X(divideEquals, "/=")   X(divide,     "/") \
    X(moduloEquals,  "%=")       X(modulo,       "%")       X(xorEquals,    "^=")   X(bitwiseXor, "^") \
    X(andEquals,     "&=")       X(logicalAnd,   "&&")      X(bitwiseAnd,   "&") \
    X(orEquals,      "|=")       X(logicalOr,    "||")      X(bitwiseOr,    "|") \
    X(leftShiftEquals,    "<<=") X(lessThanOrEqual,  "<=")  X(leftShift,    "<<")   X(lessThan,   "<") \
    X(rightShiftUnsigned, ">>>") X(rightShiftEquals, ">>=") X(rightShift,   ">>")   X(greaterThanOrEqual, ">=")  X(greaterThan,  ">")

#define NATIVE_JIT_KEYWORDS(X) \
    X(float_,      "float")      X(int_, "int")     X(double_,  "double")   X(bool_, "bool") \
    X(return_, "return")		X(true_,  "true")   X(false_,    "false")	X(const_, "const") \
	X(void_, "void")			X(buffer_, "Buffer") X(public_, "public")	X(private_, "private") \
	X(class_, "class")

namespace NativeJitTokens
{
#define DECLARE_NATIVE_JIT_TOKEN(name, str)  static const char* const name = str;
	NATIVE_JIT_KEYWORDS(DECLARE_NATIVE_JIT_TOKEN)
		NATIVE_JIT_OPERATORS(DECLARE_NATIVE_JIT_TOKEN)
		DECLARE_NATIVE_JIT_TOKEN(eof, "$eof")
	DECLARE_NATIVE_JIT_TOKEN(literal, "$literal")
	DECLARE_NATIVE_JIT_TOKEN(identifier, "$identifier")
}


struct ParserHelpers
{
	typedef const char* TokenType;

	static String getTokenName(TokenType t) { return t[0] == '$' ? String(t + 1) : ("'" + String(t) + "'"); }



	struct CodeLocation
	{
		struct Error
		{
			Error(const String::CharPointerType &program_, const String::CharPointerType &location_) : program(program_), location(location_) {};

			int offsetFromStart = 0;
			String errorMessage;
			const String::CharPointerType program;
			String::CharPointerType location;

		};

		CodeLocation(const String::CharPointerType& code) noexcept        : program(code), location(program) {}
		CodeLocation(const CodeLocation& other) noexcept : program(other.program), location(other.location) {}

		void throwError(const String& message) const
		{
			Error e(program, location);

			e.errorMessage = message;
			e.offsetFromStart = (int)(location - program);

			throw e;
		}

		String::CharPointerType program;
		String::CharPointerType location;
	};

	struct TokenIterator
	{
		TokenIterator(const String::CharPointerType& code, int length_=-1) :
			location(code),
			p(code),
			length(length_),
			endPointer(length_ > 0 ? p + length_ : String::CharPointerType(nullptr))
		{ skip(); }

		void skip()
		{
			skipWhitespaceAndComments();
			location.location = p;
			currentType = matchNextToken();
		}

		void match(TokenType expected)
		{
			if (currentType != expected)
				location.throwError("Found " + getTokenName(currentType) + " when expecting " + getTokenName(expected));

			skip();
		}

		bool matchIf(TokenType expected) { if (currentType == expected) { skip(); return true; } return false; }
		bool matchesAny(TokenType t1, TokenType t2) const { return currentType == t1 || currentType == t2; }
		bool matchesAny(TokenType t1, TokenType t2, TokenType t3) const { return matchesAny(t1, t2) || currentType == t3; }

		CodeLocation location;
		TokenType currentType;
		var currentValue;

		String program;

		String currentString;

		int offset = 0;

		String::CharPointerType p;

		String::CharPointerType endPointer;
		int length = -1;

		static bool isIdentifierStart(const juce_wchar c) noexcept { return CharacterFunctions::isLetter(c) || c == '_'; }
		static bool isIdentifierBody(const juce_wchar c) noexcept { return CharacterFunctions::isLetterOrDigit(c) || c == '_'; }

		TokenType matchNextToken()
		{
			if (length > 0 && p > endPointer) return NativeJitTokens::eof;

			if (isIdentifierStart(*p))
			{
				String::CharPointerType end(p);
				while (isIdentifierBody(*++end)) {}

				const size_t len = (size_t)(end - p);
#define NATIVE_JIT_COMPARE_KEYWORD(name, str) if (len == sizeof (str) - 1 && matchToken (NativeJitTokens::name, len)) return NativeJitTokens::name;
				NATIVE_JIT_KEYWORDS(NATIVE_JIT_COMPARE_KEYWORD)

					currentValue = String(p, end); p = end;
				currentString = String(p, end);
				return NativeJitTokens::identifier;
			}

			if (p.isDigit())
			{
				if (parseHexLiteral() || parseFloatLiteral() || parseOctalLiteral() || parseDecimalLiteral())
					return NativeJitTokens::literal;

				location.throwError("Syntax error in numeric constant");
			}

			if (parseStringLiteral(*p) || (*p == '.' && parseFloatLiteral()))
				return NativeJitTokens::literal;

#define NATIVE_JIT_COMPARE_OPERATOR(name, str) if (matchToken (NativeJitTokens::name, sizeof (str) - 1)) return NativeJitTokens::name;
			NATIVE_JIT_OPERATORS(NATIVE_JIT_COMPARE_OPERATOR)

				if (!p.isEmpty())
					location.throwError("Unexpected character '" + String::charToString(*p) + "' in source");

			return NativeJitTokens::eof;
		}

		bool matchToken(TokenType name, const size_t len) noexcept
		{
			if (p.compareUpTo(CharPointer_ASCII(name), (int)len) != 0) return false;
			p += (int)len;  return true;
		}

		void skipWhitespaceAndComments()
		{
			for (;;)
			{
				p = p.findEndOfWhitespace();

				if (*p == '/')
				{
					const juce_wchar c2 = p[1];

					if (c2 == '/') { p = CharacterFunctions::find(p, (juce_wchar) '\n'); continue; }

					if (c2 == '*')
					{
						location.location = p;
						p = CharacterFunctions::find(p + 2, CharPointer_ASCII("*/"));
						if (p.isEmpty()) location.throwError("Unterminated '/*' comment");
						p += 2; continue;
					}
				}

				break;
			}
		}

		bool parseStringLiteral(juce_wchar quoteType)
		{
			if (quoteType != '"' && quoteType != '\'')
				return false;

			Result r(JSON::parseQuotedString(p, currentValue));
			if (r.failed()) location.throwError(r.getErrorMessage());
			return true;
		}

		Identifier parseIdentifier()
		{
			Identifier i;
			if (currentType == NativeJitTokens::identifier)
				i = currentValue.toString();

			match(NativeJitTokens::identifier);
			return i;
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
			currentString = String(v);
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

			double v = CharacterFunctions::getDoubleValue(p);

			currentValue = v;

			currentString = String(v);

			if (!currentString.containsChar('.'))
			{
				currentString << '.';
			}

			if (*t == 'f')
			{
				currentString << 'f';
				t++;
			}

			p = t;
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
				else if (isPositiveAndBelow(digit, 10))  location.throwError("Decimal digit in octal constant");
				else break;
			}

			currentValue = v;  p = t;
			currentString = String(v);
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
			currentString = String(v);
			return true;
		}
	};


};



struct NativeJITTypeHelpers
{
	/** Returns a pretty name for the given Type. */
	template <typename T> static String getTypeName();

	/** Returns a pretty name for the given String literal. */
	static String getTypeName(const String &t);

	static String getTypeName(const TypeInfo& info);

	/** Creates a error message if the types don't match. */
	template <typename ActualType, typename ExpectedType> static String getTypeMismatchErrorMessage();

	template <typename ExpectedType> static String getTypeMismatchErrorMessage(const TypeInfo& actualType);

	static String getTypeMismatchErrorMessage(const TypeInfo& type1, const TypeInfo& type2);

	/** Returns the type ID for the given String literal. Currently supported: double, float & int. */
	static TypeInfo getTypeForLiteral(const String &t);;

	/** Checks if the given type ID matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const TypeInfo& actualType);

	/** Checks if the given string matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const String& t);;

	/** Checks if the given token matches the type. */
	template <typename ExpectedType> static bool matchesToken(const char* token)
	{
		return matchesType<ExpectedType>(getTypeForToken(token));
	}

	template <typename ExpectedType> static bool matchesToken(const Identifier& tokenName)
	{
		return matchesType<ExpectedType>(getTypeForToken(tokenName.toString().getCharPointer()));
	}

	static TypeInfo getTypeForToken(const char* token);

	/** Compares two types. */
	template <typename R1, typename R2> static bool is();;
};

#define TYPE_MATCH(typeclass, typeinfo) NativeJITTypeHelpers::matchesType<typeclass>(typeinfo)

#endif  // TOKENITERATOR_H_INCLUDED
