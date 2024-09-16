/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


#pragma once

namespace mcl
{

using namespace juce;


#define DECLARE_ID(x) static const Identifier x(#x);

namespace LanguageIds
{
	DECLARE_ID(XML);
    DECLARE_ID(Markdown);
    DECLARE_ID(HiseScript);
    DECLARE_ID(SNEX);
    DECLARE_ID(Faust);
    DECLARE_ID(GLSL);
}

#undef DECLARE_ID

/** This object will manage different properties of languages:

    - code tokeniser & colour scheme
    - line range functions
    - autocomplete tokens
    - bookmark title processing
*/
class LanguageManager
{
public:

    virtual ~LanguageManager();;
    
    virtual CodeTokeniser* createCodeTokeniser() = 0;

    virtual FoldableLineRange::List createLineRange(const juce::CodeDocument& doc);

    struct InplaceDebugValue
    {
        void init()
        {
	        if(!initialised)
	        {
		        location = CodeDocument::Position(*location.getOwner(), originalLineNumber, 99);
                location.setPositionMaintained(true);
                initialised = true;
	        }
        }

        int originalLineNumber;
        bool initialised = false;
        CodeDocument::Position location;
        String value;
    };

    /** Used for coallascating the token providers. */
    virtual Identifier getLanguageId() const = 0;

    virtual bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const;

    virtual void processBookmarkTitle(juce::String& bookmarkTitle) = 0;

    /** Add all token providers you want to use for this language. */
    virtual void addTokenProviders(TokenCollection* t) = 0;

    /** Use this for additional setup. */
    virtual void setupEditor(TextEditor* editor);

    /** Override this and check if the current line is commented. */
    virtual bool isLineCommented(TextDocument& document, Selection s) const;

    /** Overwrite this and toggle the line comment. */
    virtual void toggleCommentForLine(TextEditor* editor, bool shouldBeCommented);

    bool hashIsPreprocessor = true;
};

struct XmlLanguageManager: public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override;

    void processBookmarkTitle(juce::String& bookmarkTitle) override;

    Identifier getLanguageId() const override { return LanguageIds::XML; }

    void addTokenProviders(mcl::TokenCollection*) override;;

    mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc) override;
    
    void setupEditor(mcl::TextEditor* editor) override;
};

struct MarkdownLanguageManager : public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override
    {
        return new MarkdownParser::Tokeniser();
    }

    void processBookmarkTitle(juce::String& bookmarkTitle) override;

    Identifier getLanguageId() const override { return LanguageIds::Markdown; }

    void addTokenProviders(mcl::TokenCollection*) override {};

    mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc) override;
    
    void setupEditor(mcl::TextEditor* editor) override;
    
};


struct FaustLanguageManager: public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override;

    void processBookmarkTitle(juce::String& bookmarkTitle) {};
    
    Identifier getLanguageId() const override { return LanguageIds::Faust; }

    void setupEditor(mcl::TextEditor* e) override;
    
    void addTokenProviders(TokenCollection* t) override;
    
    mcl::TextEditor* currentEditor = nullptr;
};


}
