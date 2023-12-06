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

#ifndef JAVASCRIPT_TOKENISER_H_INCLUDED
#define JAVASCRIPT_TOKENISER_H_INCLUDED

namespace hise { using namespace juce;

class JavascriptTokeniser    : public CodeTokeniser
{
public:
    //==============================================================================
    JavascriptTokeniser();
    ~JavascriptTokeniser();

    //==============================================================================
    int readNextToken (CodeDocument::Iterator&) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

    void setUseScopeStatements(bool shouldUse)
    {
	    useScopeStatementParser = shouldUse;
    }

    static bool isReservedKeyword (const String& token) noexcept;

    enum TokenType
    {
        tokenType_error = 0,
        tokenType_comment,
        tokenType_keyword,
        tokenType_operator,
        tokenType_identifier,
        tokenType_integer,
        tokenType_float,
        tokenType_string,
        tokenType_bracket,
        tokenType_punctuation,
        tokenType_preprocessor,
        tokenType_scopedstatement,
        tokenType_deactivated
    };

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR (JavascriptTokeniser)

    struct ScopedBrackets
    {
	    bool parsingStatements = true;
        bool parsingArguments = false;
        int numOpenParen = 0;
        int numOpenBrackets = 0;
    };

    Array<ScopedBrackets> scopedBrackets;
    bool checkDot = false;
    bool useScopeStatementParser = false;
};

} // namespace hise

#endif   // JUCE_CPLUSPLUSCODETOKENISER_H_INCLUDED
