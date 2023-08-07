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
        int originalLineNumber;
        CodeDocument::Position location;
        String value;
    };
    
    virtual bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const;

    virtual void processBookmarkTitle(juce::String& bookmarkTitle) = 0;

    /** Add all token providers you want to use for this language. */
    virtual void addTokenProviders(TokenCollection* t) = 0;

    /** Use this for additional setup. */
    virtual void setupEditor(TextEditor* editor);
};

struct XmlLanguageManager: public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override;

    void processBookmarkTitle(juce::String& bookmarkTitle) override;

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

    void processBookmarkTitle(juce::String& bookmarkTitle) override
    {
        bookmarkTitle = bookmarkTitle.trimCharactersAtStart("#").trim();
    }

    void addTokenProviders(mcl::TokenCollection*) override {};

    mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc) override;
    
    void setupEditor(mcl::TextEditor* editor) override;
    
};


struct FaustLanguageManager: public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override;

    void processBookmarkTitle(juce::String& bookmarkTitle) {};

    void setupEditor(mcl::TextEditor* e) override;
    
    void addTokenProviders(TokenCollection* t) override;
    
    mcl::TextEditor* currentEditor = nullptr;
};


}
