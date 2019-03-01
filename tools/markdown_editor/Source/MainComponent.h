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
							   public CodeDocument::Listener,
							   public Timer
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override
	{
		startTimer(300);
	}

	void codeDocumentTextInserted(const String& newText, int insertIndex) override
	{
		startTimer(300);
	}

	void timerCallback() override
	{
		preview.setNewText(doc.getAllContent(), editor.currentFile);
		stopTimer();
	}

private:

	MarkdownDataBase database;

	juce::TooltipWindow tooltip;

	MarkdownParser::Tokeniser tokeniser;
	hise::GlobalHiseLookAndFeel klaf;
	CodeDocument doc;
	MarkdownEditor editor;
	
	MarkdownPreview preview;
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
