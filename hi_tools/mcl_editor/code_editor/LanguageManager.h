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

    virtual ~LanguageManager() {};
    
    virtual CodeTokeniser* createCodeTokeniser() = 0;

    virtual FoldableLineRange::List createLineRange(const juce::CodeDocument& doc);

    struct InplaceDebugValue
    {
        int originalLineNumber;
        CodeDocument::Position location;
        String value;
    };
    
    virtual bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const
    {
        return false;
    }
    
    virtual void processBookmarkTitle(juce::String& bookmarkTitle) = 0;

    /** Add all token providers you want to use for this language. */
    virtual void addTokenProviders(TokenCollection* t) = 0;

    /** Use this for additional setup. */
    virtual void setupEditor(TextEditor* editor) {}
};

struct XmlLanguageManager: public LanguageManager
{
    CodeTokeniser* createCodeTokeniser() override
    {
        return new XmlTokeniser();
    }

    void processBookmarkTitle(juce::String& bookmarkTitle) override
    {
        if(!bookmarkTitle.trim().endsWith("/>"))
            bookmarkTitle = bookmarkTitle.replace(">", "/>");
        
        if(auto xml = XmlDocument::parse(bookmarkTitle))
        {
            bookmarkTitle = "<";
            
            bookmarkTitle << xml->getTagName();
            
            static const StringArray possibleIds =
            {
                "FileName",
                "ID",
                "id",
                "name",
                "file"
            };
            
            for(auto& id: possibleIds)
            {
                if(xml->hasAttribute(id))
                {
                    bookmarkTitle << " (" << xml->getStringAttribute(id) << ")";
                    break;
                }
            }
            
            bookmarkTitle << ">";
        }
        
    }

    void addTokenProviders(mcl::TokenCollection*) override {};

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
    CodeTokeniser* createCodeTokeniser() override
    {
#if USE_BACKEND
        return new FaustTokeniser();
#else
        // I don't know of any use case where the faust tokeniser is required
        // in a compiled project so this will most likely never get called
        jassertfalse;
        return nullptr;
#endif
    }
    
    void processBookmarkTitle(juce::String& bookmarkTitle) {};

    void setupEditor(mcl::TextEditor* e) override;
    
    void addTokenProviders(TokenCollection* t) override;
    
    mcl::TextEditor* currentEditor = nullptr;
};


}
