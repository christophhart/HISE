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

namespace hise { using namespace juce;

//==============================================================================
JavascriptTokeniser::JavascriptTokeniser() {}
JavascriptTokeniser::~JavascriptTokeniser() {}

int JavascriptTokeniser::readNextToken (CodeDocument::Iterator& source)
{
	if(!useScopeStatementParser)
	{
		return JavascriptTokeniserFunctions::readNextToken (source);
	}

	if(source.getPosition() == 0)
		scopedBrackets.clearQuick();

	source.skipWhitespace();

	auto c = source.peekNextChar();

	if(!scopedBrackets.isEmpty())
	{
		auto& current = scopedBrackets.getReference(scopedBrackets.size()-1);

		if(current.parsingStatements)
		{
			if(c == '(')
			{
				if(!current.numOpenParen++)
				{
					source.skip();
					source.skipWhitespace();
					current.parsingArguments = true;
					return JavascriptTokeniser::tokenType_scopedstatement;
				}
			}
			if(c == ')')
			{
				if(--current.numOpenParen == 0)
				{
					source.skip();
					source.skipWhitespace();
					current.parsingArguments = false;

					if(source.peekNextChar() == ':')
					{
						source.skip();
						source.skipWhitespace();
						auto t = JavascriptTokeniserFunctions::parseIdentifier(source, true);
						return t;
					}

					return JavascriptTokeniser::tokenType_scopedstatement;
				}
			}
			if(c == '.' && !current.parsingArguments)
			{
				source.skip();
				source.skipWhitespace();
				auto t = JavascriptTokeniserFunctions::parseIdentifier(source, true);
				return t;
			}
			if(c == ';')
			{
				source.skip();
				source.skipWhitespace();
				current.parsingStatements = false;
				return JavascriptTokeniser::tokenType_scopedstatement;
			}
		}

		if(c == '}')
		{
			for(auto& b: scopedBrackets)
				b.numOpenBrackets--;
			
			if(current.numOpenBrackets < 0)
			{
				scopedBrackets.removeLast();
				source.skip();
				source.skipWhitespace();
				return JavascriptTokeniser::tokenType_bracket;
			}
		}
	}
	if(c == '{')
	{
		source.skip();
		source.skipWhitespace();

		if(!scopedBrackets.isEmpty())
		{
			scopedBrackets.getReference(scopedBrackets.size()-1).numOpenBrackets++;
		}

		checkDot = true;
		return JavascriptTokeniser::tokenType_bracket;
	}

	if(checkDot && source.peekNextChar() == '.')
	{
		source.skip();
		source.skipWhitespace();
		scopedBrackets.add({});
		return JavascriptTokeniserFunctions::parseIdentifier(source, true);
	}

    auto t = JavascriptTokeniserFunctions::readNextToken (source);
    
    if(t != tokenType_preprocessor &&
       t != tokenType_comment)
    {
        checkDot = false;
    }
    
    return t;
}

CodeEditorComponent::ColourScheme JavascriptTokeniser::getDefaultColourScheme()
{
    struct Type
    {
        const char* name;
        uint32 colour;
    };
   
	const Type types[] =
	{
		{ "Error", 0xffBB3333 },
		{ "Comment", 0xff77CC77 },
		{ "Keyword", 0xffbbbbff },
		{ "Operator", 0xffCCCCCC },
		{ "Identifier", 0xffDDDDFF },
		{ "Integer", 0xffDDAADD },
		{ "Float", 0xffEEAA00 },
		{ "String", 0xffDDAAAA },
		{ "Bracket", 0xffFFFFFF },
		{ "Punctuation", 0xffCCCCCC },
		{ "Preprocessor Text", 0xffCC7777 },
		{ "ScopedStatement", 0xff88bec5 },
		{ "Preprocessor Deactive", 0xFF444444 }
	};

    CodeEditorComponent::ColourScheme cs;

    for (unsigned int i = 0; i < sizeof (types) / sizeof (types[0]); ++i)  // (NB: numElementsInArray doesn't work here in GCC4.2)
        cs.set (types[i].name, Colour (types[i].colour));

    return cs;
}

bool JavascriptTokeniser::isReservedKeyword (const String& token) noexcept
{
    return JavascriptTokeniserFunctions::isReservedKeyword (token.getCharPointer(), token.length());
}

} // namespace hise
