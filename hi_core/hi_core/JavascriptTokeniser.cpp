/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

//==============================================================================
JavascriptTokeniser::JavascriptTokeniser() {}
JavascriptTokeniser::~JavascriptTokeniser() {}

int JavascriptTokeniser::readNextToken (CodeDocument::Iterator& source)
{
    return JavascriptTokeniserFunctions::readNextToken (source);
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
		{ "Preprocessor Text", 0xffCCCCCC }
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
