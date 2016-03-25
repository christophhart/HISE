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

#ifndef __JUCE_HEADER_276191B71AEC15F6__
#define __JUCE_HEADER_276191B71AEC15F6__

//[Headers]     -- You can add your own extra header files here --



class CCEnvelopeDisplay: public Component,
						 public Timer
{
public:

	CCEnvelopeDisplay(CCEnvelope *processor_) :
		processor(processor_),
		hold(1400.0f),
		decay(900.0f),
		targetValue(0.4f),
		currentRulerValue(0.3f),
		startLevel(1.0f),
		endLevel(1.0f)
	{
		setOpaque(false);

#if JUCE_DEBUG
		startTimer(150);
#else
		startTimer(30);
#endif
	}

	void timerCallback() override;

	void paint(Graphics &g);

private:

	float hold;

	float decay;

	float targetValue;

	float currentRulerValue;

	float startLevel;
	float endLevel;


	CCEnvelope* processor;
};

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class CCEnvelopeEditor  : public ProcessorEditorBody,
                          public Timer,
                          public ButtonListener,
                          public SliderListener
{
public:
    //==============================================================================
    CCEnvelopeEditor (BetterProcessorEditor *pe);
    ~CCEnvelopeEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void timerCallback() override
	{
		holdTimeSlider->setDisplayValue((float)getProcessor()->getChildProcessor(CCEnvelope::HoldChain)->getOutputValue());
		decayTimeSlider->setDisplayValue((float)getProcessor()->getChildProcessor(CCEnvelope::DecayChain)->getOutputValue());
		startLevelSlider->setDisplayValue((float)getProcessor()->getChildProcessor(CCEnvelope::StartLevelChain)->getOutputValue());
		endLevelSlider->setDisplayValue((float)getProcessor()->getChildProcessor(CCEnvelope::EndLevelChain)->getOutputValue());
	}

	int getBodyHeight() const override
	{
		return tableUsed ? h : 198;
	}

	void updateGui() override
	{
		learnButton->setToggleState(dynamic_cast<CCEnvelope*>(getProcessor())->learnModeActive(), dontSendNotification);

		controllerNumberSlider->updateValue();
		defaultSlider->updateValue();
		smoothingSlider->updateValue();
		holdTimeSlider->updateValue();
		decayTimeSlider->updateValue();
		startLevelSlider->updateValue();
		endLevelSlider->updateValue();
		fixedNoteOffButton->updateValue();
		useTableButton->setToggleState(getProcessor()->getAttribute(CCEnvelope::UseTable) == 1.0f, dontSendNotification);
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	bool tableUsed;
	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<HiSlider> controllerNumberSlider;
    ScopedPointer<HiSlider> smoothingSlider;
    ScopedPointer<ToggleButton> learnButton;
    ScopedPointer<HiSlider> defaultSlider;
    ScopedPointer<HiSlider> decayTimeSlider;
    ScopedPointer<HiSlider> holdTimeSlider;
    ScopedPointer<CCEnvelopeDisplay> display;
    ScopedPointer<HiToggleButton> fixedNoteOffButton;
    ScopedPointer<HiSlider> startLevelSlider;
    ScopedPointer<HiSlider> endLevelSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CCEnvelopeEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_276191B71AEC15F6__
