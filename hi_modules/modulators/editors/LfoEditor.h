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
#define LfoEditor(x) ProcessorEditor(x, new LfoEditorBody(x))
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class LfoEditorBody  : public ProcessorEditorBody,
                       public Timer,
                       public Slider::Listener,
                       public ComboBox::Listener,
                       public Button::Listener
{
public:
    //==============================================================================
    LfoEditorBody (ProcessorEditor *p);
    ~LfoEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		fadeInSlider->updateValue();

		waveFormSelector->updateValue();

		auto type = (int)getProcessor()->getAttribute(LfoModulator::WaveFormType);

		

		tempoSyncButton->updateValue();
		retriggerButton->updateValue();

		frequencySlider->updateValue();

		smoothTimeSlider->updateValue();

		loopButton->setEnabled(type == LfoModulator::Waveform::Custom || LfoModulator::Waveform::Steps);

		loopButton->updateValue();

		if (getProcessor()->getAttribute(LfoModulator::TempoSync) > 0.5f)
		{
			frequencySlider->setMode(HiSlider::Mode::TempoSync);
		}
		else
		{
			frequencySlider->setMode(HiSlider::Frequency, 0.5, 40.0, 10.0);
		}

		const bool newTableUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Custom;
		const bool newStepsUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Steps;

		if (newTableUsed != tableUsed || newStepsUsed != stepsUsed)
		{
			tableUsed = newTableUsed;
			stepsUsed = newStepsUsed;
			refreshBodySize();
			resized();
		}

	};

	void timerCallback() override
	{
		//attackSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::AttackChain)->getOutputValue());
		//releaseSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::ReleaseChain)->getOutputValue());
		frequencySlider->setDisplayValue(getProcessor()->getChildProcessor(LfoModulator::FrequencyChain)->getOutputValue());

	}

	int getBodyHeight() const override
	{
		return (tableUsed || stepsUsed) ? h : 120 ;
	};



    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

	bool tableUsed;
	bool stepsUsed;

	ScopedPointer<SliderPack> stepPanel;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> frequencySlider;
    ScopedPointer<HiSlider> fadeInSlider;
    ScopedPointer<Label> label;
    ScopedPointer<HiComboBox> waveFormSelector;
    ScopedPointer<WaveformComponent> waveformDisplay;
    ScopedPointer<HiToggleButton> tempoSyncButton;
    ScopedPointer<HiToggleButton> retriggerButton;
    ScopedPointer<TableEditor> waveformTable;
    ScopedPointer<HiSlider> smoothTimeSlider;
    ScopedPointer<HiToggleButton> loopButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]
