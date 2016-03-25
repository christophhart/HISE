#pragma once

// Lua code tokeniser from Ctrlr project
// http://ctrlr.org

#include "../JuceLibraryCode/JuceHeader.h"

class ProtoLuaTokeniser   : public CodeTokeniser
{
public:
    //==============================================================================
    ProtoLuaTokeniser();
    ~ProtoLuaTokeniser();

    //==============================================================================
    int readNextToken (CodeDocument::Iterator&) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

    /** The token values returned by this tokeniser. */
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
        tokenType_punctuation
    };

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProtoLuaTokeniser)
};
