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

#ifndef __JUCE_HEADER_38A6EE76FFB94F36__
#define __JUCE_HEADER_38A6EE76FFB94F36__

//[Headers]     -- You can add your own extra header files here --




class WavetableDisplayComponent: public Component,
								 public Timer
{
public:

	WavetableDisplayComponent(ModulatorSynth *synth_):
		synth(synth_),
		tableLength(0),
		tableValues(nullptr)
	{
		if(isDisplayForWavetableSynth()) startTimer(50);
	}

	bool isDisplayForWavetableSynth() const { return dynamic_cast<WavetableSynth*>(synth) != nullptr; };

	void timerCallback();

	void paint(Graphics &g)
	{
		Path p;

		float w = (float)getWidth();
		float h = (float)getHeight();

		p.startNewSubPath(0.0, h / 2.0f);

		const float cycle = tableLength / w;

		if(tableValues != nullptr)
		{

			for(int i = 0; i < getWidth(); i++)
			{
				const int tableIndex = (int) ((float)i * cycle);

				jassert(tableIndex < tableLength);

				const float value = normalizeValue * tableValues[tableIndex];

				p.lineTo((float)i, value * -(h-2)/2 + h/2);


			}
		}

		p.lineTo(w, h/2.0f);

		p.closeSubPath();

		KnobLookAndFeel::fillPathHiStyle(g, p, (int)w, (int)h);


	}

private:

	ModulatorSynth *synth;

	float const *tableValues;

	int tableLength;

	float normalizeValue;

};


//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class WavetableBody  : public ProcessorEditorBody,
                       public LabelListener,
                       public ButtonListener
{
public:
    //==============================================================================
    WavetableBody (BetterProcessorEditor *p);
    ~WavetableBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override {return h;};

	void updateGui() override
	{
		hiqButton->updateValue();

		fadeTimeEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::KillFadeTime)), dontSendNotification);
		voiceAmountEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::VoiceLimit)), dontSendNotification);
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<WavetableDisplayComponent> waveTableDisplay;
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<TableEditor> component;
    ScopedPointer<HiToggleButton> hiqButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableBody)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_38A6EE76FFB94F36__
