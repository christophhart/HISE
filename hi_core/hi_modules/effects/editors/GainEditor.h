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

#ifndef __JUCE_HEADER_E5F6E8EEDB7946A4__
#define __JUCE_HEADER_E5F6E8EEDB7946A4__

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
class GainEditor  : public ProcessorEditorBody,
                    public Timer,
                    public SliderListener
{
public:
    //==============================================================================
    GainEditor (ProcessorEditor *p);
    ~GainEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const { return 80; };

	void updateGui()
	{
		gainSlider->updateValue();
        delaySlider->updateValue();
        widthSlider->updateValue();
		balanceSlider->updateValue();
	}

    void timerCallback()
    {
        gainSlider->setDisplayValue(getProcessor()->getChildProcessor(GainEffect::InternalChains::GainChain)->getOutputValue());
        delaySlider->setDisplayValue(getProcessor()->getChildProcessor(GainEffect::InternalChains::DelayChain)->getOutputValue());
        widthSlider->setDisplayValue(getProcessor()->getChildProcessor(GainEffect::InternalChains::WidthChain)->getOutputValue());
		balanceSlider->setDisplayValue(getProcessor()->getChildProcessor(GainEffect::InternalChains::BalanceChain)->getOutputValue());

    }

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> widthSlider;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> delaySlider;
    ScopedPointer<HiSlider> balanceSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainEditor)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_E5F6E8EEDB7946A4__
