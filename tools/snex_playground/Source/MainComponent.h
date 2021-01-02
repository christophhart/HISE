/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define SHOW_VALUE_TREE_GEN 0

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...

	snex::ui::WorkbenchData::Ptr data;

    Value v;

	ScopedPointer<snex::jit::SnexPlayground> playground;
	OpenGLContext context;

	CodeDocument d;
	mcl::TextDocument doc;
	mcl::FullEditor editor;

	struct Updater : public Timer
	{
		
		Updater(mcl::TextDocument& doc):
			d(doc)
		{
			startTimer(1000);
		}
		void timerCallback() override;

		ValueTree lastTree;
		mcl::TextDocument& d;
	} treeUpdater;
	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
