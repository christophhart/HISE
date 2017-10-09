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

#ifndef __JUCE_HEADER_56A727D0E28A3C9A__
#define __JUCE_HEADER_56A727D0E28A3C9A__

//[Headers]     -- You can add your own extra header files here --

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class WaveSynthBody  : public ProcessorEditorBody,
                       public SliderListener,
                       public ComboBoxListener,
                       public LabelListener,
                       public ButtonListener
{
public:
    //==============================================================================
    WaveSynthBody (ProcessorEditor *p);
    ~WaveSynthBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		fadeTimeEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::KillFadeTime)), dontSendNotification);
		voiceAmountEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::VoiceLimit)), dontSendNotification);


		waveFormSelector->updateValue();
		waveFormSelector2->updateValue();

		octaveSlider->updateValue();
		octaveSlider2->updateValue();

		detuneSlider->updateValue();
		detuneSlider2->updateValue();

		panSlider->updateValue();
		panSlider2->updateValue();

		enableSecondButton->updateValue();

		pulseSlider1->updateValue();
		pulseSlider2->updateValue();

		const bool enableSecond = enableSecondButton->getToggleState();

		mixSlider->updateValue();

		mixSlider->setEnabled(getProcessor()->getChildProcessor(WaveSynth::MixModulation)->getNumChildProcessors() == 0);

		refreshWaveform(waveformDisplay, ((int)getProcessor()->getAttribute(WaveSynth::WaveForm1)));
		refreshWaveform(waveformDisplay2, ((int)getProcessor()->getAttribute(WaveSynth::WaveForm2)));

		if (!enableSecond)
		{
			mixSlider->setEnabled(false);
			panSlider2->setEnabled(false);
			pulseSlider2->setEnabled(false);
			waveFormSelector2->setEnabled(false);
			octaveSlider2->setEnabled(false);
			detuneSlider2->setEnabled(false);
			panSlider2->setEnabled(false);
		}
	};

	void changeListenerCallback(SafeChangeBroadcaster *b)
	{
		if(dynamic_cast<ModulatorChain*>(b) != nullptr)
		{
			const bool mixActive = dynamic_cast<ModulatorChain*>(b)->getNumChildProcessors() == 0;

			mixSlider->setEnabled(mixActive);
		}
	}

	int getBodyHeight() const override
	{
		return h;
	}
	
	void refreshWaveform(WaveformComponent* component, int type)
	{
		if (type < WaveformComponent::WaveformType::Custom)
		{
			component->setType(type);
			return;
		}
		
		switch (type)
		{
		case WaveSynth::AdditionalWaveformTypes::Square2: component->setType(WaveformComponent::Square); break;
		case WaveSynth::AdditionalWaveformTypes::Triangle2: component->setType(WaveformComponent::Triangle); break;
		case WaveSynth::AdditionalWaveformTypes::Trapezoid1: component->setType(WaveformComponent::Saw); break;
		case WaveSynth::AdditionalWaveformTypes::Trapezoid2: component->setType(WaveformComponent::Saw); break;
		}
	}

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void labelTextChanged (Label* labelThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> octaveSlider;
    ScopedPointer<HiComboBox> waveFormSelector;
    ScopedPointer<WaveformComponent> waveformDisplay;
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<HiSlider> octaveSlider2;
    ScopedPointer<HiComboBox> waveFormSelector2;
    ScopedPointer<WaveformComponent> waveformDisplay2;
    ScopedPointer<HiSlider> mixSlider;
    ScopedPointer<HiSlider> panSlider;
    ScopedPointer<HiSlider> panSlider2;
    ScopedPointer<HiSlider> detuneSlider2;
    ScopedPointer<HiSlider> detuneSlider;
    ScopedPointer<HiToggleButton> enableSecondButton;
    ScopedPointer<HiSlider> pulseSlider1;
    ScopedPointer<HiSlider> pulseSlider2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSynthBody)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_56A727D0E28A3C9A__
