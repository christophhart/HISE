/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"


//==============================================================================
MainContentComponent::MainContentComponent():
	editor(doc, &tokeniser)
{
	editor.setLookAndFeel(&klaf);
	preview.setLookAndFeel(&klaf);

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFAAAAAA));
	editor.setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.2f));
	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor.setFont(GLOBAL_MONOSPACE_FONT().withHeight(18.0f));

	doc.addListener(this);

    setSize (600, 400);
}

MainContentComponent::~MainContentComponent()
{
	doc.removeListener(this);
}

void MainContentComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(0xFF393939));

    
}

void MainContentComponent::resized()
{
	auto ar = getLocalBounds();

	editor.setBounds(ar.removeFromLeft(getWidth() / 2));
	preview.setBounds(ar.reduced(20));
}
