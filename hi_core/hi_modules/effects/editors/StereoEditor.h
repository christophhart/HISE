/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_B32AB9083F57539A__
#define __JUCE_HEADER_B32AB9083F57539A__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class StereoEditor  : public ProcessorEditorBody,
                      public Timer,
                      public SliderListener
{
public:
    //==============================================================================
    StereoEditor (ProcessorEditor *p);
    ~StereoEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		panSlider->updateValue();
		widthSlider->updateValue();
	};

	void timerCallback() override
	{
		const float displayValue = (float)getProcessor()->getChildProcessor(StereoEffect::BalanceChain)->getOutputValue();

		panSlider->setDisplayValue(displayValue);
	}

	int getBodyHeight() const override {return h; };

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> widthSlider;
    ScopedPointer<HiSlider> panSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoEditor)
};

//[EndFile] You can add extra defines here...

/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_B32AB9083F57539A__
