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

/** Allow line number parsing. set this to false to speed up the parsing (until  there's a faster method available). */
#ifndef SNEX_PARSE_LINE_NUMBERS
#define SNEX_PARSE_LINE_NUMBERS 1
#endif


#define SNEX_PREPARSE_LINE_NUMBERS 1

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
			Error e(*this);

			e.errorMessage = message;
			throw e;
		}

		Point<int> getXYPosition() const
		{
			return { getLine(), getColNumber() };
		}

		void test(const Result& r) const
		{
			if (!r.wasOk())
				throwError(r.getErrorMessage());
		}

		int getLine() const
		{
#if SNEX_PARSE_LINE_NUMBERS
#if SNEX_PREPARSE_LINE_NUMBERS
			return lineNumber;
#else
			return calculateLineNumber();
#endif
#else
			return 0;
#endif
		}

		void calculateLineIfEnabled(bool isEnabled)
		{
			if (isEnabled)
				calculatePosition(false, true);
		}

		void calculatePosIfEnabled(bool isEnabled)
		{
			if (isEnabled)
				calculatePosition(true, true);
		}

		void calculatePosition(bool calculateColToo, bool forceUpdate=false)
		{
#if SNEX_PREPARSE_LINE_NUMBERS
			if (getXYPosition().isOrigin() || forceUpdate)
			{
				lineNumber = calculateLineNumber();

				if (calculateColToo)
					colNumber = calculateColNumber();
			}
#endif
		}

		int getColNumber() const
		{
#if SNEX_PARSE_LINE_NUMBERS
#if SNEX_PREPARSE_LINE_NUMBERS
			return colNumber;
#else
			return calculateColNumber();
#endif
#else
			return 0;
#endif
		}

		int calculateLineNumber() const
		{
			auto start = program;
			auto end = location;

			int line = 1;

			while(start != end)
			{
				if (*start++ == '\n')
					line++;
			}

			return line;

#if 0
			int col = 0;

			auto start = program;
			auto end = location;

			auto charactersFromStart = (end - start);

			for (int i = 0; i < jmin<int>((int)charactersFromStart, (int)start.length()); i++)
			{
				if (start[i] == '\n')
					col = 0;
				else
					col++;
			}

			return col;
#endif
		}

		int calculateColNumber() const
		{


			int col = 0;

			auto start = program;
			auto end = location;

			while (start != end)
			{
				if (*(end) == '\n')
				{
					break;
				}

				col++;
				--end;
			}

			return col - 1;
		}

		String::CharPointerType program;
		String::CharPointerType location;

#if SNEX_PREPARSE_LINE_NUMBERS
		int lineNumber = 0;
		int colNumber = 0;
#endif
	};

	struct Error
	{
		enum class Format
		{
			LineNumbers,
			CodeExample
		};

		Error(const CodeLocation& location_) :
			location(location_)
		{
			
		};

		String toString(Format f = Format::LineNumbers) const
		{
			auto lToUse = location;

			lToUse.calculatePosition(true);

			String s;

			if (f == Format::LineNumbers)
			{
				s << "Line " << lToUse.getLine();
				s << "(" << lToUse.getColNumber() << ")";
				s << ": ";
				s << errorMessage;
			}
			else
			{
				auto x = jmax(location.program, location.location - 4);
				auto end = x;
				int pos = 0;

				while (!end.isEmpty() && pos++ < 8)
					end++;

				while (pos < 8)
				{
					--x;
					pos++;
				}

				auto xPos = (int)(location.location - x);

				for (int i = 0; i < xPos; i++)
					s << ' ';

				s << "V\n";
				s << String(x, end) << ": ";
				s << errorMessage << "\n";
			}
            
			return s;
		}

		String errorMessage;

		CodeLocation location;
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
			try
			{
				skip();
			}
			catch (Error& )
			{

			}
		}

		TokenIterator(const juce::String& c) :
			location(c.getCharPointer(), c.getCharPointer()),
			p(c.getCharPointer()),
			endPointer(c.getCharPointer() + c.length())
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

		void skipWithoutWhitespace()
		{
			location.location = p;
			currentType = matchNextToken();
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

		StringArray parsePreprocessorParameterList()
		{
			match(JitTokens::openParen);

			StringArray parameters;

			while (!isEOF() && currentType != JitTokens::closeParen)
			{
				auto newP = parsePreprocessorParameter();
				parameters.add(newP);
				matchIf(JitTokens::comma);
			}

			match(JitTokens::closeParen);

			return parameters;
		}

		String parsePreprocessorParameter()
		{
			juce::String s;

			int numOpen = 0;

			while (!isEOF())
			{
				auto breakAtEnd = false;

				if (currentType == JitTokens::openParen)
					numOpen++;
				if (currentType == JitTokens::closeParen)
				{
					numOpen--;

					if (numOpen == -1)
						return s;

					if (numOpen == 0)
						breakAtEnd = true;
				}
				if (currentType == JitTokens::comma)
				{
					if (numOpen == 0)
						return s;
				}
				
				auto prev = location.location;
				skip();
				auto now = location.location;

				s << juce::String(prev, now);

				if (breakAtEnd)
					return s;
			}

			return s;
		}

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

		void throwIf(TokenType name)
		{
			if (currentType == name)
				throwTokenMismatch(name);
		}

		

		void skipWhitespaceAndComments()
		{
			for (;;)
			{
				p = p.findEndOfWhitespace();

				if (*p == '/')
				{
					lastComment = "";

					const juce_wchar c2 = p[1];

					if (c2 == '/') 
					{ 
						auto start = p;
						p = CharacterFunctions::find(p, (juce_wchar) '\n'); 
						lastComment = String(start, p);

						continue;
					}

					if (c2 == '*')
					{
						auto start = p;
						location.location = p;
						p = CharacterFunctions::find(p + 2, CharPointer_ASCII("*/"));
						if (p.isEmpty()) location.throwError("Unterminated '/*' comment");

						lastComment = String(start, p);

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

		Identifier parseOperatorId()
		{
			if (matchIf(JitTokens::assign_))
			{
				return FunctionClass::getSpecialSymbol({}, FunctionClass::AssignOverload);
			}
			if (matchIf(JitTokens::openBracket))
			{
				match(JitTokens::closeBracket);
				return FunctionClass::getSpecialSymbol({}, FunctionClass::Subscript);
			}
			if (matchIf(JitTokens::plusplus))
			{
				return FunctionClass::getSpecialSymbol({}, FunctionClass::IncOverload);
			}
			if (matchIf(JitTokens::minusminus))
			{
				return FunctionClass::getSpecialSymbol({}, FunctionClass::DecOverload);
			}

			location.throwError("Unsupported operator overload");

            RETURN_DEBUG_ONLY({});
		}

		Identifier parseIdentifier()
		{
			if(matchIf(JitTokens::operator_))
			{
				return parseOperatorId();
			}

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

		TemplateParameter::VariadicType parseVariadicDots()
		{
			if (matchIf(JitTokens::dot))
			{
				match(JitTokens::dot);
				match(JitTokens::dot);
				return TemplateParameter::VariadicType::Variadic;
			}

			return TemplateParameter::VariadicType::Single;
		}



		VariableStorage parseVariableStorageLiteral()
		{
			bool isMinus = matchIf(JitTokens::minus);

			auto type = Types::Helpers::getTypeFromStringValue(currentString);


			juce::String stringValue = currentString;

			if (matchIf(JitTokens::true_))
				return VariableStorage(1);

			if (matchIf(JitTokens::false_))
				return VariableStorage(0);

			match(JitTokens::literal);

			if (type == Types::ID::Integer)
				return VariableStorage(stringValue.getIntValue() * (!isMinus ? 1 : -1));
			else if (type == Types::ID::Float)
				return VariableStorage(stringValue.getFloatValue() * (!isMinus ? 1.0f : -1.0f));
			else if (type == Types::ID::Double)
				return VariableStorage(stringValue.getDoubleValue() * (!isMinus ? 1.0 : -1.0));
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

		String lastComment;

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
