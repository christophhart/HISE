/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.3

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

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

		phaseSlider->updateValue();

		tempoSyncButton->updateValue();
		retriggerButton->updateValue();
		clockSyncButton->updateValue();
		frequencySlider->updateValue();

		smoothTimeSlider->updateValue();

		loopButton->setEnabled(type == LfoModulator::Waveform::Custom || LfoModulator::Waveform::Steps);

		loopButton->updateValue();

		if (getProcessor()->getAttribute(LfoModulator::TempoSync) > 0.5f)
			frequencySlider->setMode(HiSlider::Mode::TempoSync);
		else
			frequencySlider->setMode(HiSlider::Frequency, 0.5, 40.0, 10.0);

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

    int h = 0;

    bool tableUsed = false;
    bool stepsUsed = false;

	ScopedPointer<SliderPack> stepPanel;

    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<HiSlider> frequencySlider;
    std::unique_ptr<HiSlider> fadeInSlider;
    std::unique_ptr<Label> label;
    std::unique_ptr<HiComboBox> waveFormSelector;
    std::unique_ptr<WaveformComponent> waveformDisplay;
    std::unique_ptr<HiToggleButton> tempoSyncButton;
	std::unique_ptr<HiToggleButton> clockSyncButton;
    std::unique_ptr<HiToggleButton> retriggerButton;
    std::unique_ptr<TableEditor> waveformTable;
    std::unique_ptr<HiSlider> smoothTimeSlider;
    std::unique_ptr<HiToggleButton> loopButton;
    std::unique_ptr<HiSlider> phaseSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

