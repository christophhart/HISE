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

#ifndef __JUCE_HEADER_A0E3EB6B16ECB822__
#define __JUCE_HEADER_A0E3EB6B16ECB822__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

class AhdsrEnvelopeEditor;










//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]

	\cond HIDDEN_SYMBOLS

	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AhdsrEnvelopeEditor  : public ProcessorEditorBody,
                             public Timer,
                             public SliderListener
{
public:
    //==============================================================================
    AhdsrEnvelopeEditor (ProcessorEditor *p);
    ~AhdsrEnvelopeEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{

		attackSlider->updateValue();
		attackLevelSlider->updateValue();
		holdSlider->updateValue();
		decaySlider->updateValue();
		sustainSlider->updateValue();
		releaseSlider->updateValue();
		attackCurveSlider->updateValue();
		decayCurveSlider->updateValue();
	}

	int getBodyHeight() const override
	{
		return h;
	}

	void timerCallback() override
	{
		using namespace scriptnode::envelope::pimpl;

		attackLevelSlider->setDisplayValue(getProcessor()->getChildProcessor(ahdsr_base::InternalChains::AttackLevelChain)->getOutputValue());
		attackSlider->setDisplayValue(getProcessor()->getChildProcessor		(ahdsr_base::InternalChains::AttackTimeChain)->getOutputValue());
		decaySlider->setDisplayValue(getProcessor()->getChildProcessor		(ahdsr_base::InternalChains::DecayTimeChain)->getOutputValue());
		sustainSlider->setDisplayValue(getProcessor()->getChildProcessor	(ahdsr_base::InternalChains::SustainLevelChain)->getOutputValue());
		releaseSlider->setDisplayValue(getProcessor()->getChildProcessor	(ahdsr_base::InternalChains::ReleaseTimeChain)->getOutputValue());


	}

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<HiSlider> attackLevelSlider;
    ScopedPointer<HiSlider> holdSlider;
    ScopedPointer<HiSlider> decaySlider;
    ScopedPointer<HiSlider> sustainSlider;
    ScopedPointer<AhdsrGraph> ahdsrGraph;
    ScopedPointer<HiSlider> attackCurveSlider;
    ScopedPointer<HiSlider> decayCurveSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AhdsrEnvelopeEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_A0E3EB6B16ECB822__
