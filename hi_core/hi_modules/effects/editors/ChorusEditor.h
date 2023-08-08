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

#ifndef __JUCE_HEADER_2FC10C12BF7D2C7E__
#define __JUCE_HEADER_2FC10C12BF7D2C7E__

//[Headers]     -- You can add your own extra header files here --

namespace hise { using namespace juce;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ChorusEditor  : public ProcessorEditorBody,
                      public SliderListener
{
public:
    //==============================================================================
    ChorusEditor (ProcessorEditor *p);
    ~ChorusEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	int getBodyHeight() const override { return 90; };

	void updateGui() override
	{
		rateSlider->updateValue();
		widthSlider->updateValue();
		feedBackSlider->updateValue();
		delaySlider->updateValue();

	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> rateSlider;
    ScopedPointer<HiSlider> widthSlider;
    ScopedPointer<HiSlider> feedBackSlider;
    ScopedPointer<HiSlider> delaySlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_2FC10C12BF7D2C7E__
