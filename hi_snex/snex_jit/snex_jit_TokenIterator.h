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
namespace jit {
using namespace juce;

#define HNODE_JIT_OPERATORS(X) \
    X(semicolon,     ";")        X(dot,          ".")       X(comma,        ",") \
    X(openParen,     "(")        X(closeParen,   ")")       X(openBrace,    "{")    X(closeBrace, "}") \
    X(openBracket,   "[")        X(closeBracket, "]")       X(double_colon, "::")   X(colon,        ":")    X(question,   "?") \
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

#define HNODE_JIT_KEYWORDS(X) \
    X(float_,      "float")      X(int_, "int")     X(double_,  "double")   X(bool_, "bool") \
    X(return_, "return")		X(true_,  "true")   X(false_,    "false")	X(const_, "const") \
	X(void_, "void")			X(public_, "public")	X(private_, "private") \
	X(class_, "class")			X(block_, "block")	X(for_, "for") \
	X(if_, "if")				X(else_, "else")	\
	X(auto_, "auto")			X(struct_, "struct")	X(span_, "span") \
	X(using_, "using")		    X(wrap, "wrap")		X(static_, "static")	X(break_, "break") \
	X(continue_, "continue")	X(dyn_, "dyn")		X(namespace_, "namespace")  

namespace JitTokens
{
#define DECLARE_HNODE_JIT_TOKEN(name, str)  static const char* const name = str;
	HNODE_JIT_KEYWORDS(DECLARE_HNODE_JIT_TOKEN)
		HNODE_JIT_OPERATORS(DECLARE_HNODE_JIT_TOKEN)
		DECLARE_HNODE_JIT_TOKEN(eof, "$eof")
	DECLARE_HNODE_JIT_TOKEN(literal, "$literal")
	DECLARE_HNODE_JIT_TOKEN(identifier, "$identifier")
}

#if JUCE_DEBUG
#define THROW_PARSING_ERROR(message, dummyReturnValue) location.throwError(message); return dummyReturnValue;
#else
#define THROW_PARSING_ERROR(message, dummyReturnValue) location.throwError(message); ignoreUnused(dummyReturnValue);
#endif

struct ParserHelpers
{
	using TokenType = const char*;

	static String getTokenName(TokenType t) { return t[0] == '$' ? String(t + 1) : ("'" + String(t) + "'"); }

	struct CodeLocation
	{
		
		CodeLocation(const juce::String::CharPointerType& code,
			         const juce::String::CharPointerType& wholeProgram) noexcept        :
		  program(wholeProgram),
		  location(code)
		{};

		CodeLocation(const CodeLocation& other) noexcept : program(other.program), location(other.location) {}

		void throwError(const juce::String& message) const
		{
			Error e(program, location);

			e.errorMessage = message;

			throw e;
		}

		int getLine() const
		{
			return getLineNumber(program, location);
		}

		NamespacedIdentifier createAnonymousScopeId(const NamespacedIdentifier& parent = {}) const
		{
			auto id = Identifier("AnonymousScopeLine" + juce::String(getLine()));

			if (parent.isValid())
				return parent.getChildId(id);
				
			return NamespacedIdentifier(id);
		}

		static int getLineNumber(juce::String::CharPointerType start, 
			juce::String::CharPointerType end)
		{
			int line = 1;

			auto charactersFromStart = (end - start);

			for (int i = 0; i < jmin<int>((int)charactersFromStart, (int)start.length()); i++)
			{
				if (start[i] == '\n')
				{
					line++;
				}
			}

			return line;
		}

		struct Error
		{
			Error(const String::CharPointerType &program_, const String::CharPointerType &location_) : 
				program(program_), 
				location(location_)
			{};

			String toString() const
			{
				String s;

				s << "Line " << CodeLocation::getLineNumber(program, location);
				s << ": " << errorMessage;

				return s;
			}

			String errorMessage;
			String::CharPointerType location;
			String::CharPointerType program;

		};

		String::CharPointerType program;
		String::CharPointerType location;
	};

	

	struct TokenIterator
	{

		enum AssignSideEffectType
		{
			Nothing = 0,
			PostIncrement,
			PostDecrement,
			PreIncrement,
			PreDecrement
		};


	typedef const char* TokenType;

		TokenIterator(const juce::String::CharPointerType& code, const juce::String::CharPointerType& wholeProgram, int length) :
			location(code, wholeProgram),
			p(code),
			endPointer(code + length)
		{
			skip(); 
		}

        virtual ~TokenIterator() {};
        
		void seek(TokenIterator& other)
		{
			if (other.location.program != location.program)
				location.throwError("Can't skip different locations");

			while (location.location != other.location.location && !isEOF())
				skip();
		}

		bool seekAndReturnTrue(TokenIterator* parentToSeek)
		{
			if (parentToSeek == this)
				return true;

			if (parentToSeek != nullptr)
				parentToSeek->seek(*this);

			return true;
		}

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

		bool isEOF() const { return currentType == JitTokens::eof; }

		bool matchIf(TokenType expected) { if (currentType == expected) { skip(); return true; } return false; }
		bool matchesAny(TokenType t1, TokenType t2) const { return currentType == t1 || currentType == t2; }
		bool matchesAny(TokenType t1, TokenType t2, TokenType t3) const { return matchesAny(t1, t2) || currentType == t3; }

		CodeLocation location;
		TokenType currentType;
		var currentValue; // TODO make VariableStorage

		String program;

		String currentString;

		TypeInfo currentTypeInfo;

		int offset = 0;

		String::CharPointerType p;

		String::CharPointerType endPointer;
		int length = -1;

		TokenType currentAssignmentType;

		AssignSideEffectType currentSideEffect = Nothing;

		void parseSideEffect()
		{
			if (currentSideEffect != Nothing)
				location.throwError("Can't use more than one side effect");

			if (matchIf(JitTokens::plusplus))
				currentSideEffect = PreIncrement;
			if (matchIf(JitTokens::minusminus))
				currentSideEffect = PreDecrement;
			else
				currentSideEffect = Nothing;
		}

		static bool isIdentifierStart(const juce_wchar c) noexcept { return CharacterFunctions::isLetter(c) || c == '_'; }
		static bool isIdentifierBody(const juce_wchar c) noexcept { return CharacterFunctions::isLetterOrDigit(c) || c == '_'; }


		void throwTokenMismatch(const char* expected)
		{
			String s;

			s << "Parsing error: Expected: " << expected;
			s << "Actual: " << currentType;

			location.throwError(s);
		}

		bool matchIfAssignmentType()
		{
			TokenType assignType;

			if (matchIf(JitTokens::minusEquals))		assignType = JitTokens::minus;
			else if (matchIf(JitTokens::divideEquals))  assignType = JitTokens::divide;
			else if (matchIf(JitTokens::timesEquals))	assignType = JitTokens::times;
			else if (matchIf(JitTokens::plusEquals))	assignType = JitTokens::plus;
			else if (matchIf(JitTokens::moduloEquals))	assignType = JitTokens::modulo;
			else if (matchIf(JitTokens::assign_))		assignType = JitTokens::assign_;
			else										assignType = JitTokens::void_;

			
			currentAssignmentType = assignType;

			return assignType != JitTokens::void_;
		}

		TokenType matchNextToken()
		{
			if (length > 0 && p > endPointer) return JitTokens::eof;

			if (isIdentifierStart(*p))
			{
				String::CharPointerType end(p);
				while (isIdentifierBody(*++end)) {}

				const size_t len = (size_t)(end - p);
#define HNODE_JIT_COMPARE_KEYWORD(name, str) if (len == sizeof (str) - 1 && matchToken (JitTokens::name, len)) return JitTokens::name;
				HNODE_JIT_KEYWORDS(HNODE_JIT_COMPARE_KEYWORD)

					currentValue = String(p, end); p = end;
				currentString = String(p, end);
				return JitTokens::identifier;
			}

			if (p.isDigit())
			{
				if (parseHexLiteral() || parseFloatLiteral() || parseOctalLiteral() || parseDecimalLiteral())
					return JitTokens::literal;

				location.throwError("Syntax error in numeric constant");
			}

			if (parseStringLiteral(*p) || (*p == '.' && parseFloatLiteral()))
				return JitTokens::literal;

#define HNODE_JIT_COMPARE_OPERATOR(name, str) if (matchToken (JitTokens::name, sizeof (str) - 1)) return JitTokens::name;
			HNODE_JIT_OPERATORS(HNODE_JIT_COMPARE_OPERATOR)

				if (!p.isEmpty())
					location.throwError("Unexpected character '" + String::charToString(*p) + "' in source");

			return JitTokens::eof;
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
			if (currentType == JitTokens::identifier)
				i = currentValue.toString();

			match(JitTokens::identifier);
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

		VariableStorage parseVariableStorageLiteral()
		{
			bool isMinus = matchIf(JitTokens::minus);

			auto type = Types::Helpers::getTypeFromStringValue(currentString);

			juce::String stringValue = currentString;
			match(JitTokens::literal);

			if (type == Types::ID::Integer)
				return VariableStorage(stringValue.getIntValue() * (!isMinus ? 1 : -1));
			else if (type == Types::ID::Float)
				return VariableStorage(stringValue.getFloatValue() * (!isMinus ? 1.0f : -1.0f));
			else if (type == Types::ID::Double)
				return VariableStorage(stringValue.getDoubleValue() * (!isMinus ? 1.0 : -1.0));
			else if (type == Types::ID::Block)
				return block();
			else
				return {};
		}



		static String getSideEffectDescription(AssignSideEffectType s)
		{

			switch (s)
			{
			case Nothing:		return "Nothing";
			case PostIncrement:	return "Post Increment";
			case PostDecrement: return "Post Decrement";
			case PreIncrement:	return "Pre Increment";
			case PreDecrement:	return "Pre Decrement";
			default:
				break;
			}
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
			currentString = juce::String(v);
			return true;
		}
	};


};

} // end namespace jit
} // end namespace snex
