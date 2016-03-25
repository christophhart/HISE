#ifndef JAVASCRIPT_TOKENISER_H_INCLUDED
#define JAVASCRIPT_TOKENISER_H_INCLUDED


class JavascriptTokeniser    : public CodeTokeniser
{
public:
    //==============================================================================
    JavascriptTokeniser();
    ~JavascriptTokeniser();

    //==============================================================================
    int readNextToken (CodeDocument::Iterator&) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

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
        tokenType_preprocessor
    };

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR (JavascriptTokeniser)
};


#endif   // JUCE_CPLUSPLUSCODETOKENISER_H_INCLUDED
