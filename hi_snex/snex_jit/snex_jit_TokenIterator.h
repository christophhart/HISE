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

#define HNODE_JIT_KEYWORDS(X) \
    X(float_,      "float")      X(int_, "int")     X(double_,  "double")   X(bool_, "bool") \
    X(return_, "return")		X(true_,  "true")   X(false_,    "false")	X(const_, "const") \
	X(void_, "void")			X(public_, "public")	X(private_, "private") \
	X(class_, "class")			X(block_, "block")	X(event_, "event")		X(for_, "for") \
	X(if_, "if")				X(else_, "else")	X(sfloat, "sfloat")		X(sdouble, "sdouble") \
	X(auto_, "auto")			X(wblock, "wblock")	X(zblock, "zblock")		X(struct_, "struct")	X(span_, "span") \
	X(using_, "using")		    X(wrap, "wrap")		X(static_, "static")

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

		Identifier createAnonymousScopeId() const
		{
			return Identifier("AnonymousScopeLine" + juce::String(getLine()));
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

		TokenIterator(const String::CharPointerType& code, const String::CharPointerType& wholeProgram, int length, const Symbol& rootSymbol_) :
			location(code, wholeProgram),
			p(code),
			endPointer(code + length),
			rootSymbol(rootSymbol_)
		{
			skip(); 
		}

        virtual ~TokenIterator() {};
        
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

		void matchSymbol()
		{
			if (currentType != JitTokens::identifier)
				location.throwError("Expected symbol");

			clearSymbol();

			bool repeat = true;

			while (currentType == JitTokens::identifier && repeat)
			{
				addSymbolChild(parseIdentifier());
				repeat = matchIf(JitTokens::dot);
			}
		}

		Symbol getCurrentSymbol(bool includeRootAndSetType)
		{
			return currentSymbol.withType(currentHnodeType);
		}

		CodeLocation location;
		TokenType currentType;
		var currentValue; // TODO make VariableStorage

		String program;

		String currentString;

		Types::ID currentHnodeType;
		
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

		

#if 0
		bool matchIfSymbol(bool isConst)
		{
			auto isReference = matchIf(JitTokens::bitwiseAnd);

			if (currentType == JitTokens::identifier)
			{
				Array<Identifier> idList;

				idList.add(parseIdentifier());

				while(matchIf(JitTokens::dot))
					idList.add(parseIdentifier());

				currentSymbol = BaseScope::Symbol(idList, Types::ID::Dynamic, isConst, isReference);

				return true;
			}

			return false;
		}
#endif

		static bool isIdentifierStart(const juce_wchar c) noexcept { return CharacterFunctions::isLetter(c) || c == '_'; }
		static bool isIdentifierBody(const juce_wchar c) noexcept { return CharacterFunctions::isLetterOrDigit(c) || c == '_'; }

		virtual bool matchIfSimpleType()
		{
			if (matchIf(JitTokens::float_))		  currentHnodeType = Types::ID::Float;
			else if (matchIf(JitTokens::int_))	  currentHnodeType = Types::ID::Integer;
			else if (matchIf(JitTokens::double_)) currentHnodeType = Types::ID::Double;
			else if (matchIf(JitTokens::event_))  currentHnodeType = Types::ID::Event;
			else if (matchIf(JitTokens::block_))  currentHnodeType = Types::ID::Block;
			else if (matchIf(JitTokens::void_))	  currentHnodeType = Types::ID::Void;
			else if (matchIf(JitTokens::auto_)) { currentHnodeType = Types::ID::Dynamic; return true; }
			else								  currentHnodeType = Types::ID::Dynamic;
			
			return currentHnodeType != Types::ID::Dynamic;
		}

		void throwTokenMismatch(const char* expected)
		{
			String s;

			s << "Parsing error: Expected: " << expected;
			s << "Actual: " << currentType;

			location.throwError(s);
		}

		bool isWrappedBlockType() const
		{
			if (currentType == JitTokens::wblock)
				return true;
			if (currentType == JitTokens::zblock)
				return true;

			return false;
		}

		bool isSmoothedVariableType() const
		{
			if (currentType == JitTokens::sfloat)
				return true;
			if (currentType == JitTokens::sdouble)
				return true;

			return false;
		}

		virtual Types::ID matchType()
		{
			if (matchIf(JitTokens::float_)) return Types::ID::Float;
			if (matchIf(JitTokens::int_))	return Types::ID::Integer;
			if (matchIf(JitTokens::double_)) return Types::ID::Double;
			if (matchIf(JitTokens::event_))	return Types::ID::Event;
			if (matchIf(JitTokens::block_))	return Types::ID::Block;
			if (matchIf(JitTokens::void_))	return Types::ID::Void;
			if (matchIf(JitTokens::sfloat)) return Types::ID::Float;
			if (matchIf(JitTokens::sdouble)) return Types::ID::Double;
			if (matchIf(JitTokens::zblock)) return Types::ID::Block;
			if (matchIf(JitTokens::wblock)) return Types::ID::Block;
			if (matchIf(JitTokens::wrap))   return Types::ID::Integer;
			if (matchIf(JitTokens::auto_))  return Types::ID::Dynamic;

			throwTokenMismatch("Type");

			RETURN_IF_NO_THROW(Types::ID::Void);
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
			currentString = String(v);
			return true;
		}

		void pushAnonymousScopeId()
		{
			anonymousScopeIds.add(location.createAnonymousScopeId());
		}

		void popAnonymousScopeId()
		{
			jassert(!anonymousScopeIds.isEmpty());
			anonymousScopeIds.removeLast();
		}

		struct ScopedChildRoot
		{
			ScopedChildRoot(TokenIterator& iter, const Identifier& id):
				p(iter)
			{
				p.rootSymbol = p.rootSymbol.getChildSymbol(id);
			}

			~ScopedChildRoot()
			{
				p.rootSymbol = p.rootSymbol.getParentSymbol();
			}

			TokenIterator& p;
		};

		void clearSymbol()
		{
			currentSymbol = {};
		}

		void addSymbolChild(const Identifier& id)
		{
			currentSymbol = currentSymbol.getChildSymbol(id);
		}

		

	private:

		Array<Identifier> anonymousScopeIds;
		
		Symbol currentSymbol;
		Symbol rootSymbol;
	};


};

} // end namespace jit
} // end namespace snex
