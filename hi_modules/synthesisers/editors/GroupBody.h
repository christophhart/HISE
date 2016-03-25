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

#ifndef __JUCE_HEADER_2557D757746C7D52__
#define __JUCE_HEADER_2557D757746C7D52__

//[Headers]     -- You can add your own extra header files here --

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class GroupBody  : public ProcessorEditorBody,
                   public LabelListener,
                   public ComboBoxListener,
                   public ButtonListener
{
public:
    //==============================================================================
    GroupBody (BetterProcessorEditor *p);
    ~GroupBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override { return h; };

	void updateGui() override
	{
		fmStateLabel->setText(dynamic_cast<ModulatorSynthGroup*>(getProcessor())->getFMState(), dontSendNotification);

		if (getProcessor()->getAttribute(ModulatorSynthGroup::EnableFM))
		{
			modSelector->setVisible(true);
			carrierSelector->setVisible(true);

			modSelector->clear(dontSendNotification);
			carrierSelector->clear(dontSendNotification);
			for (int i = ModulatorSynthGroup::InternalChains::numInternalChains; i < getProcessor()->getNumChildProcessors(); i++)
			{
				modSelector->addItem(getProcessor()->getChildProcessor(i)->getId(), i);
				carrierSelector->addItem(getProcessor()->getChildProcessor(i)->getId(), i);
			}

			modSelector->updateValue();
			carrierSelector->updateValue();


		}
		else
		{
			modSelector->setVisible(false);
			carrierSelector->setVisible(false);
		}

		fmButton->updateValue();

		fadeTimeEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::KillFadeTime)), dontSendNotification);
		voiceAmountEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::VoiceLimit)), dontSendNotification);

	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<HiComboBox> carrierSelector;
    ScopedPointer<HiToggleButton> fmButton;
    ScopedPointer<HiComboBox> modSelector;
    ScopedPointer<Label> fmStateLabel;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroupBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
//[/EndFile]

#endif   // __JUCE_HEADER_2557D757746C7D52__
