/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"



//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component,
							   public MarkdownDatabaseHolder,
							   public CodeDocument::Listener
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

	void registerContentProcessor(MarkdownContentProcessor* processor) {}
	void registerItemGenerators() {}

	File getCachedDocFolder() const override {
		return File();
	}
	File getDatabaseRootDirectory() const
	{
		return File();
	}

	bool shouldUseCachedData() const override { return false; }
	
	/** Called by a CodeDocument when text is added. */
	virtual void codeDocumentTextInserted(const String& newText, int insertIndex)
	{
		preview.setNewText(doc.getAllContent(), {});
	}

	/** Called by a CodeDocument when text is deleted. */
	virtual void codeDocumentTextDeleted(int startIndex, int endIndex)
	{
		preview.setNewText(doc.getAllContent(), {});
	}

private:

	CodeDocument doc;
	MarkdownParser::Tokeniser tok;
	MarkdownEditor editor;
	MarkdownPreview preview;

	hise::GlobalHiseLookAndFeel laf;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
