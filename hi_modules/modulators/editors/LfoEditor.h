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

#ifndef __JUCE_HEADER_52D6A5016C50158E__
#define __JUCE_HEADER_52D6A5016C50158E__

//[Headers]     -- You can add your own extra header files here --
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
                       public SliderListener,
                       public ComboBoxListener,
                       public ButtonListener
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

		waveformDisplay->setType((int)getProcessor()->getAttribute(LfoModulator::WaveFormType));

		tableUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Custom;

		tempoSyncButton->updateValue();
		retriggerButton->updateValue();

		frequencySlider->updateValue();

		smoothTimeSlider->updateValue();

	};

	void timerCallback() override
	{
		//attackSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::AttackChain)->getOutputValue());
		//releaseSlider->setDisplayValue(getProcessor()->getChildProcessor(TableEnvelope::ReleaseChain)->getOutputValue());
		frequencySlider->setDisplayValue(getProcessor()->getChildProcessor(LfoModulator::FrequencyChain)->getOutputValue());

	}

	int getBodyHeight() const override
	{
		return tableUsed ? h : 120 ;
	};



    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

	bool tableUsed;

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


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */

//[/EndFile]

#endif   // __JUCE_HEADER_52D6A5016C50158E__
