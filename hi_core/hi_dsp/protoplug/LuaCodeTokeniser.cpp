#include "LuaCodeTokeniser.h"

struct LuaTokeniserFunctions
{
    static bool isReservedKeyword (String::CharPointerType token, const int tokenLength) noexcept
    {
        static const char* const keywords2Char[] =
            { "if", "or", "in", "do", nullptr };

        static const char* const keywords3Char[] =
            { "and", "end", "for", "nil", "not", nullptr };

        static const char* const keywords4Char[] =
            { "then", "true", "else", nullptr };

        static const char* const keywords5Char[] =
            {  "false", "local", "until", "while", "break", nullptr };

        static const char* const keywords6Char[] =
            { "repeat", "return", "elseif", nullptr};

        static const char* const keywords7Char[] =
            { "require", 0 };

        static const char* const keywordsOther[] =
            { "function", "@interface", "@end", "@synthesize", "@dynamic", "@public",
              "@private", "@property", "@protected", "@class", nullptr };

        const char* const* k;

        switch (tokenLength)
        {
            case 2:   k = keywords2Char; break;
            case 3:   k = keywords3Char; break;
            case 4:   k = keywords4Char; break;
            case 5:   k = keywords5Char; break;
            case 6:   k = keywords6Char; break;
            case 7:   k = keywords7Char; break;

            default:
                if (tokenLength < 2 || tokenLength > 16)
                    return false;

                k = keywordsOther;
                break;
        }

        for (int i = 0; k[i] != 0; ++i)
            if (token.compare (CharPointer_ASCII (k[i])) == 0)
                return true;

        return false;
    }
	
    template <typename Iterator>
    static bool skipQuotedString (Iterator& source) noexcept
    {
        const juce_wchar quote = source.nextChar();

        for (;;)
        {
            const juce_wchar c = source.nextChar();
			const juce_wchar next = source.peekNextChar();

            if (c == quote)
                break;

            if (c == 0 || c == '\n')
                return false;

            if (c == '\\' && next != '\r' && next != '\n')
                source.skip();
        }
		return true;
    }

    template <typename Iterator>
    static int parseIdentifier (Iterator& source) noexcept
    {
        int tokenLength = 0;
        String::CharPointerType::CharType possibleIdentifier [100];
        String::CharPointerType possible (possibleIdentifier);

        while (CppTokeniserFunctions::isIdentifierBody (source.peekNextChar()))
        {
            const juce_wchar c = source.nextChar();

            if (tokenLength < 20)
                possible.write (c);

            ++tokenLength;
        }

        if (tokenLength > 1 && tokenLength <= 16)
        {
            possible.writeNull();

            if (isReservedKeyword (String::CharPointerType (possibleIdentifier), tokenLength))
                return LuaTokeniser::tokenType_keyword;
        }

        return LuaTokeniser::tokenType_identifier;
    }

    template <typename Iterator>
    static int readNextToken (Iterator& source)
    {
        source.skipWhitespace();

        const juce_wchar firstChar = source.peekNextChar();

        switch (firstChar)
        {
        case 0:
            break;

        case '0':   case '1':   case '2':   case '3':   case '4':
        case '5':   case '6':   case '7':   case '8':   case '9':
        case '.':
        {
            int result = CppTokeniserFunctions::parseNumber (source);

            if (result == LuaTokeniser::tokenType_error)
            {
                source.skip();

                if (firstChar == '.')
                    return LuaTokeniser::tokenType_punctuation;
            }

            return result;
        }

        case ',':
        case ';':
        case ':':
            source.skip();
            return LuaTokeniser::tokenType_punctuation;

        case '(':   case ')':
        case '{':   case '}':
        case ']':
            source.skip();
            return LuaTokeniser::tokenType_bracket;

        case '[':
		{
            source.skip();
            if (source.peekNextChar() == '[')
            {
				// skip to ]] or eof
				source.nextChar();
				for (;;)
				{
					const juce_wchar c = source.nextChar();
					if ((c == ']' && source.peekNextChar() == ']')|| c == 0)
					{
						source.skip();
						break;
					}
				}
				return LuaTokeniser::tokenType_string;
			}
            return LuaTokeniser::tokenType_bracket;
		}

        case '"':
        case '\'':
            if (skipQuotedString (source))
				return LuaTokeniser::tokenType_string;
			else
				return LuaTokeniser::tokenType_error;

        case '+':
            source.skip();
            CppTokeniserFunctions::skipIfNextCharMatches (source, '+', '=');
            return LuaTokeniser::tokenType_operator;

        case '-':
        {
            source.skip();
            int result = CppTokeniserFunctions::parseNumber (source);

            if (source.peekNextChar() == '-')
            {
				source.skip();
				if (source.peekNextChar() == '[')
				{
					source.skip();
					if (source.peekNextChar() == '[')
					{
						// skip to ]] or eof
						source.nextChar();
						for (;;)
						{
							const juce_wchar c = source.nextChar();
							if ((c == ']' && source.peekNextChar() == ']')|| c == 0)
							{
								source.skip();
								break;
							}
						}
						return LuaTokeniser::tokenType_comment;
					}
				}

                source.skipToEndOfLine();
                return LuaTokeniser::tokenType_comment;
            }

            if (result == LuaTokeniser::tokenType_error)
            {
                CppTokeniserFunctions::skipIfNextCharMatches (source, '-', '=');
                return LuaTokeniser::tokenType_operator;
            }

            return result;
        }

        case '*':   case '%':
        case '=':   case '!':
            source.skip();
            CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
            return LuaTokeniser::tokenType_operator;

        case '?':
        case '~':
            source.skip();
            return LuaTokeniser::tokenType_operator;

        case '<':   case '>':
        case '|':   case '&':   case '^':
            source.skip();
            CppTokeniserFunctions::skipIfNextCharMatches (source, firstChar);
            CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
            return LuaTokeniser::tokenType_operator;

        default:
            if (CppTokeniserFunctions::isIdentifierStart (firstChar))
                return parseIdentifier (source);

            source.skip();
            break;
        }

        return LuaTokeniser::tokenType_error;
    }
};

//==============================================================================
ProtoLuaTokeniser::ProtoLuaTokeniser() {}
ProtoLuaTokeniser::~ProtoLuaTokeniser() {}

int ProtoLuaTokeniser::readNextToken (CodeDocument::Iterator& source)
{
    return LuaTokeniserFunctions::readNextToken (source);
}

CodeEditorComponent::ColourScheme ProtoLuaTokeniser::getDefaultColourScheme()
{
    static const CodeEditorComponent::ColourScheme::TokenType types[] =
    {
        { "Error",          Colour (0xffcc0000) },
        { "Comment",        Colour (0xff3c3c3c) },
        { "Keyword",        Colour (0xff0000cc) },
        { "Operator",       Colour (0xff225500) },
        { "Identifier",     Colour (0xff000000) },
        { "Integer",        Colour (0xff880000) },
        { "Float",          Colour (0xff885500) },
        { "String",         Colour (0xff990099) },
        { "Bracket",        Colour (0xff000055) },
        { "Punctuation",    Colour (0xff004400) }
    };

    CodeEditorComponent::ColourScheme cs;

    for (unsigned int i = 0; i < sizeof (types) / sizeof (types[0]); ++i)  // (NB: numElementsInArray doesn't work here in GCC4.2)
        cs.set (types[i].name, types[i].colour);

    return cs;
}
