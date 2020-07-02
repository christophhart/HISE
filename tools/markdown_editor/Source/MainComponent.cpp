/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainContentComponent::MainContentComponent() :
	editor(doc, &tok),
	preview(*this)
{
	setLookAndFeel(&laf);

	doc.addListener(this);

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	preview.setViewOptions((int)MarkdownPreview::ViewOptions::Naked);
	preview.setStyleData(MarkdownLayout::StyleData::createBrightStyle());

    setSize (1280, 800);
}

MainContentComponent::~MainContentComponent()
{
    
}

void MainContentComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(0xFF393939));

    
}

void MainContentComponent::resized()
{
	auto b = getLocalBounds();

	editor.setBounds(b.removeFromLeft(getWidth() / 2));
	preview.setBounds(b);
}
