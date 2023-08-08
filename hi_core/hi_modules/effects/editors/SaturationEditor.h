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

#ifndef __JUCE_HEADER_51E7880F16D45702__
#define __JUCE_HEADER_51E7880F16D45702__

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
class SaturationEditor  : public ProcessorEditorBody,
                          public Timer,
                          public SliderListener
{
public:
    //==============================================================================
    SaturationEditor (ProcessorEditor *p);
    ~SaturationEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void timerCallback() override
	{

	}

	int getBodyHeight() const override { return h; }

	void updateGui()
	{
		saturationSlider->updateValue();
		wetSlider->updateValue();
        pregainSlider->updateValue();
        postGainSlider->updateValue();
	}
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> saturationSlider;
    ScopedPointer<HiSlider> wetSlider;
    ScopedPointer<HiSlider> pregainSlider;
    ScopedPointer<HiSlider> postGainSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturationEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_51E7880F16D45702__
