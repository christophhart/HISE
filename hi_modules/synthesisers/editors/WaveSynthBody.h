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
                       public LabelListener
{
public:
    //==============================================================================
    WaveSynthBody (BetterProcessorEditor *p);
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

		mixSlider->updateValue();

		mixSlider->setEnabled(getProcessor()->getChildProcessor(WaveSynth::MixModulation)->getNumChildProcessors() == 0);

		waveformDisplay->setType((int)getProcessor()->getAttribute(WaveSynth::WaveForm1));
		waveformDisplay2->setType((int)getProcessor()->getAttribute(WaveSynth::WaveForm2));
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
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void labelTextChanged (Label* labelThatHasChanged);



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


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSynthBody)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_56A727D0E28A3C9A__
