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

#ifndef __JUCE_HEADER_7427E0CDBCAABE04__
#define __JUCE_HEADER_7427E0CDBCAABE04__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

#define SimpleEnvelopeEditor(x) ModulatorEditor(x, new SimpleEnvelopeEditorBody(x))
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SimpleEnvelopeEditorBody  : public ProcessorEditorBody,
                                  public Timer,
                                  public SliderListener,
                                  public ButtonListener
{
public:
    //==============================================================================
    SimpleEnvelopeEditorBody (ProcessorEditor *p);
    ~SimpleEnvelopeEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		attackSlider->updateValue();
		releaseSlider->updateValue();

		useLinearMode->updateValue();

	};

	void timerCallback() override
	{
		const float displayValue = (float)sm->getChildProcessor(SimpleEnvelope::AttackChain)->getOutputValue();

		attackSlider->setDisplayValue(displayValue);
	}

	int getBodyHeight() const override
	{
		return h;
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	SimpleEnvelope *sm;

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<HiToggleButton> useLinearMode;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEnvelopeEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_7427E0CDBCAABE04__
