/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"



//==============================================================================
MainComponent::MainComponent():
  Thread("Unit Test thread")
{
    addAndMakeVisible(viewer);
    viewer.onStart = [this](){ startThread(7); };
	startTimer(150);

#if JUCE_WINDOWS
    context.attachTo(*this);
    setSize(1440, 900);
	//setSize (2560, 1080);
#else
	setSize(1440, 900);
#endif
    
    
    
    
}

MainComponent::~MainComponent()
{
#if JUCE_WINDOWS
	context.detach();
#endif
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	
}

void MainComponent::resized()
{
    viewer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
}



