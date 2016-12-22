/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

// Use this to quickly scale the window
#define SCALE_2 0

//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
	standaloneProcessor = new StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);
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
#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif

}
