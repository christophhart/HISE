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


//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class WavetableBody  : public ProcessorEditorBody,
                       public Label::Listener,
                       public Button::Listener,
                       public ComboBox::Listener
{
public:
    //==============================================================================
    WavetableBody (ProcessorEditor *p);
    ~WavetableBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override {return h;};

	void updateGui() override
	{
		hiqButton->updateValue();
		mipmapButton->updateValue();
		wavetableSelector->updateValue();
		tableSlider->updateValue();

		fadeTimeEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::KillFadeTime)), dontSendNotification);
		voiceAmountEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::VoiceLimit)), dontSendNotification);
	}


    void paint (Graphics& g) override;
    void resized() override;
    void labelTextChanged (Label* labelThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;



private:

	int h;


    //==============================================================================
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<HiToggleButton> hiqButton;
	ScopedPointer<HiToggleButton> mipmapButton;
    ScopedPointer<Label> voiceAmountLabel2;
    
    ScopedPointer<Label> voiceAmountLabel3;
    ScopedPointer<HiComboBox> wavetableSelector;
	ScopedPointer<HiSlider> tableSlider;
	ScopedPointer<Component> waterfall;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableBody)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
