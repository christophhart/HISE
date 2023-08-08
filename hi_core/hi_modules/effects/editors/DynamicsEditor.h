/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --

namespace hise { using namespace juce;

//#include "../../../projects/standalone/JuceLibraryCode/JuceHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class DynamicsEditor  : public ProcessorEditorBody,
                        public Timer,
                        public Button::Listener,
                        public Slider::Listener
{
public:
    //==============================================================================
    DynamicsEditor (ProcessorEditor *p);
    ~DynamicsEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const
	{
		return h;
	}

	void updateGui() override;

	void timerCallback() override;

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiToggleButton> gateEnabled;
    ScopedPointer<VuMeter> gateMeter;
    ScopedPointer<Label> label;
    ScopedPointer<HiSlider> gateThreshold;
    ScopedPointer<HiSlider> gateAttack;
    ScopedPointer<HiSlider> gateRelease;
    ScopedPointer<HiToggleButton> compEnabled;
    ScopedPointer<VuMeter> compMeter;
    ScopedPointer<HiSlider> compThreshold;
    ScopedPointer<HiSlider> compAttack;
    ScopedPointer<HiSlider> compRelease;
    ScopedPointer<HiSlider> compRatio;
    ScopedPointer<HiToggleButton> limiterEnabled;
    ScopedPointer<VuMeter> limiterMeter;
    ScopedPointer<HiSlider> limiterThreshold;
    ScopedPointer<HiSlider> limiterAttack;
    ScopedPointer<HiSlider> limiterRelease;
    ScopedPointer<HiToggleButton> compMakeup;
    ScopedPointer<HiToggleButton> limiterMakeup;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicsEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
