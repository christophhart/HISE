/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_339F69E0E85BB86C__
#define __JUCE_HEADER_339F69E0E85BB86C__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PhaserEditor  : public ProcessorEditorBody,
                      public SliderListener
{
public:
    //==============================================================================
    PhaserEditor (BetterProcessorEditor *p);
    ~PhaserEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    
    void updateGui()
    {
        speedSlider->updateValue();
        rangeSlider->updateValue();
        feedBackSlider->updateValue();
        wetSlider->updateValue();
    }
    
    int getBodyHeight() const override
    {
        return 80;
    }
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> speedSlider;
    ScopedPointer<HiSlider> rangeSlider;
    ScopedPointer<HiSlider> feedBackSlider;
    ScopedPointer<HiSlider> wetSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_339F69E0E85BB86C__
