/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define SHOW_VALUE_TREE_GEN 1





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

	hise::PooledUIUpdater updater;

	snex::ui::WorkbenchData::Ptr data;

	ScopedPointer<snex::ui::WorkbenchData::CodeProvider> provider;

    Value v;

	hise::GlobalHiseLookAndFeel laf;
	Slider funkSlider;

	ScopedPointer<snex::jit::SnexPlayground> playground;
	ScopedPointer<snex::ui::Graph> graph1;
	ScopedPointer<snex::ui::Graph> graph2;
	ScopedPointer<snex::ui::TestComplexDataManager> complexData;
	ScopedPointer<snex::ui::ParameterList> parameters;
	ScopedPointer<snex::ui::TestDataComponent> testData;
	OpenGLContext context;

    ScopedPointer<Component> webViewWrapper;
    


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
