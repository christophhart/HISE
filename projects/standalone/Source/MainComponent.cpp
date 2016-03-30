/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"


//==============================================================================
MainContentComponent::MainContentComponent()
{
	standaloneProcessor = new StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	//open.attachTo(*editor);

	setSize(editor->getWidth(), editor->getHeight());

	DBG(Desktop::getInstance().getDisplays().getMainDisplay().scale);
}

MainContentComponent::~MainContentComponent()
{
	
	//open.detach();
	editor = nullptr;

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colours::lightgrey);
}

void MainContentComponent::resized()
{
	editor->setSize(getWidth(), getHeight());

}
