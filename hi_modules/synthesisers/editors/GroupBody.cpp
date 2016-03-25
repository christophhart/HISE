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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "GroupBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GroupBody::GroupBody (BetterProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (fadeTimeLabel = new Label ("new label",
                                                  TRANS("Fade Time")));
    fadeTimeLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    fadeTimeLabel->setJustificationType (Justification::centredLeft);
    fadeTimeLabel->setEditable (false, false, false);
    fadeTimeLabel->setColour (Label::textColourId, Colours::white);
    fadeTimeLabel->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountLabel = new Label ("new label",
                                                     TRANS("Voice Amount")));
    voiceAmountLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    voiceAmountLabel->setJustificationType (Justification::centredLeft);
    voiceAmountLabel->setEditable (false, false, false);
    voiceAmountLabel->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountEditor = new Label ("new label",
                                                      TRANS("64")));
    voiceAmountEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    voiceAmountEditor->setJustificationType (Justification::centredLeft);
    voiceAmountEditor->setEditable (true, true, false);
    voiceAmountEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceAmountEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    voiceAmountEditor->addListener (this);

    addAndMakeVisible (fadeTimeEditor = new Label ("new label",
                                                   TRANS("15 ms")));
    fadeTimeEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    fadeTimeEditor->setJustificationType (Justification::centredLeft);
    fadeTimeEditor->setEditable (true, true, false);
    fadeTimeEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fadeTimeEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    fadeTimeEditor->addListener (this);

    addAndMakeVisible (carrierSelector = new HiComboBox ("Carrier Selection"));
    carrierSelector->setTooltip (TRANS("Set the carrier synthesizer"));
    carrierSelector->setEditableText (false);
    carrierSelector->setJustificationType (Justification::centredLeft);
    carrierSelector->setTextWhenNothingSelected (TRANS("Select Carrier"));
    carrierSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    carrierSelector->addListener (this);

    addAndMakeVisible (fmButton = new HiToggleButton ("FM Synthesiser"));
    fmButton->setTooltip (TRANS("Enables FM Modulation\n"));
    fmButton->setButtonText (TRANS("Enable FM"));
    fmButton->addListener (this);
    fmButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (modSelector = new HiComboBox ("Modulation Selection"));
    modSelector->setTooltip (TRANS("Set the modulation synthesizer"));
    modSelector->setEditableText (false);
    modSelector->setJustificationType (Justification::centredLeft);
    modSelector->setTextWhenNothingSelected (TRANS("Select Modulator"));
    modSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    modSelector->addListener (this);

    addAndMakeVisible (fmStateLabel = new Label ("new label",
                                                 TRANS("FM deactivated")));
    fmStateLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    fmStateLabel->setJustificationType (Justification::centred);
    fmStateLabel->setEditable (true, true, false);
    fmStateLabel->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    fmStateLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    fmStateLabel->setColour (TextEditor::textColourId, Colours::black);
    fmStateLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fmStateLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    fmStateLabel->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Synthesiser Group")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	fmButton->setup(getProcessor(), ModulatorSynthGroup::EnableFM, "Enable FM");
	modSelector->setup(getProcessor(), ModulatorSynthGroup::ModulatorIndex, "Modulation Carrier");
	carrierSelector->setup(getProcessor(), ModulatorSynthGroup::CarrierIndex, "Carrier Index");



	fadeTimeEditor->setFont (GLOBAL_FONT());
	voiceAmountEditor->setFont (GLOBAL_FONT());
	voiceAmountLabel->setFont (GLOBAL_FONT());
	fadeTimeLabel->setFont (GLOBAL_FONT());
    fmStateLabel->setFont(GLOBAL_FONT());
    fmStateLabel->setEditable(false, false);

    //[/UserPreSize]

    setSize (800, 60);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

GroupBody::~GroupBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    voiceAmountEditor = nullptr;
    fadeTimeEditor = nullptr;
    carrierSelector = nullptr;
    fmButton = nullptr;
    modSelector = nullptr;
    fmStateLabel = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void GroupBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void GroupBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    fadeTimeLabel->setBounds (103, 8, 79, 24);
    voiceAmountLabel->setBounds (15, 9, 79, 24);
    voiceAmountEditor->setBounds (20, 27, 68, 16);
    fadeTimeEditor->setBounds (108, 27, 51, 16);
    carrierSelector->setBounds (308, 6, 128, 28);
    fmButton->setBounds (304 - 128, 12, 128, 32);
    modSelector->setBounds (446, 6, 128, 28);
    fmStateLabel->setBounds (309, 39, 264, 16);
    label->setBounds (getWidth() - 264, 0, 264, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void GroupBody::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == voiceAmountEditor)
    {
        //[UserLabelCode_voiceAmountEditor] -- add your label text handling code here..

		int value = labelThatHasChanged->getText().getIntValue();



		if(value > 0)
		{
			value = jmin(128, value);

			getProcessor()->setAttribute(ModulatorSynth::VoiceLimit, (float)value, dontSendNotification);
		}

        //[/UserLabelCode_voiceAmountEditor]
    }
    else if (labelThatHasChanged == fadeTimeEditor)
    {
        //[UserLabelCode_fadeTimeEditor] -- add your label text handling code here..
		int value = labelThatHasChanged->getText().getIntValue();

		if(value > 0)
		{
			value = jmin(20000, value);

			getProcessor()->setAttribute(ModulatorSynth::KillFadeTime, (float)value, dontSendNotification);
		}
        //[/UserLabelCode_fadeTimeEditor]
    }
    else if (labelThatHasChanged == fmStateLabel)
    {
        //[UserLabelCode_fmStateLabel] -- add your label text handling code here..
        //[/UserLabelCode_fmStateLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void GroupBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == carrierSelector)
    {
        //[UserComboBoxCode_carrierSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_carrierSelector]
    }
    else if (comboBoxThatHasChanged == modSelector)
    {
        //[UserComboBoxCode_modSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_modSelector]
    }

    //[UsercomboBoxChanged_Post]
	getProcessor()->sendChangeMessage();
    //[/UsercomboBoxChanged_Post]
}

void GroupBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == fmButton)
    {
        //[UserButtonCode_fmButton] -- add your button handler code here..
		getProcessor()->sendChangeMessage();
        //[/UserButtonCode_fmButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GroupBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="BetterProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="60">
  <BACKGROUND backgroundColour="8e0000"/>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="103 8 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Fade Time" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="15 9 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Voice Amount" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="20 27 68 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="108 27 51 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <COMBOBOX name="Carrier Selection" id="223afd792a25b6b" memberName="carrierSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="308 6 128 28"
            tooltip="Set the carrier synthesizer" editable="0" layout="33"
            items="" textWhenNonSelected="Select Carrier" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="FM Synthesiser" id="e77edc03c117de85" memberName="fmButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="304r 12 128 32"
                tooltip="Enables FM Modulation&#10;" txtcol="ffffffff" buttonText="Enable FM"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <COMBOBOX name="Modulation Selection" id="96d6c3d91b3a6311" memberName="modSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="446 6 128 28"
            tooltip="Set the modulation synthesizer" editable="0" layout="33"
            items="" textWhenNonSelected="Select Modulator" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="3fc5b4f17a9b1eb" memberName="fmStateLabel"
         virtualName="" explicitFocusOrder="0" pos="309 39 264 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="FM deactivated" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="0Rr 0 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Synthesiser Group"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="26" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
