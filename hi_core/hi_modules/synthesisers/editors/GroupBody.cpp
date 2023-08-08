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

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "GroupBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GroupBody::GroupBody (ProcessorEditor *p)
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
    label->setJustificationType (Justification::centred);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (unisonoSlider = new HiSlider ("Detune"));
    unisonoSlider->setRange (-100, 100, 1);
    unisonoSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    unisonoSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    unisonoSlider->addListener (this);

    addAndMakeVisible (detuneSlider = new HiSlider ("Detune"));
    detuneSlider->setRange (-100, 100, 1);
    detuneSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    detuneSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    detuneSlider->addListener (this);

    addAndMakeVisible (spreadSlider = new HiSlider ("Detune"));
    spreadSlider->setRange (-100, 100, 1);
    spreadSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    spreadSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    spreadSlider->addListener (this);

    addAndMakeVisible (forceMonoButton = new HiToggleButton (String()));
    forceMonoButton->setTooltip (TRANS("Enables FM Modulation\n"));
    forceMonoButton->setButtonText (TRANS("Force Mono"));
    forceMonoButton->addListener (this);
    forceMonoButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	label->setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));

	forceMonoButton->setup(getProcessor(), ModulatorSynthGroup::ForceMono, "Force Mono");
	fmButton->setup(getProcessor(), ModulatorSynthGroup::EnableFM, "Enable FM");
	modSelector->setup(getProcessor(), ModulatorSynthGroup::ModulatorIndex, "Modulation Carrier");
	carrierSelector->setup(getProcessor(), ModulatorSynthGroup::CarrierIndex, "Carrier Index");

	label->setJustificationType(Justification::centred);

	fadeTimeEditor->setFont (GLOBAL_FONT());
	voiceAmountEditor->setFont (GLOBAL_FONT());
	voiceAmountLabel->setFont (GLOBAL_FONT());
	fadeTimeLabel->setFont (GLOBAL_FONT());
    fmStateLabel->setFont(GLOBAL_FONT());
    fmStateLabel->setEditable(false, false);

	unisonoSlider->setup(getProcessor(), ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount, "Unisono Voices");
	unisonoSlider->setMode(HiSlider::Mode::Discrete, 1, 16, 8, 1.0);

	detuneSlider->setup(getProcessor(), ModulatorSynthGroup::SpecialParameters::UnisonoDetune, "Detune");
	detuneSlider->setMode(HiSlider::Mode::Linear, 0.0, 6.0, 1.0, 0.01);
	detuneSlider->setTextValueSuffix(" st");

	spreadSlider->setup(getProcessor(), ModulatorSynthGroup::SpecialParameters::UnisonoSpread, "Spread");
	spreadSlider->setMode(HiSlider::Mode::NormalizedPercentage);
	spreadSlider->setRange(0.0, 2.0, 0.01);

	spreadSlider->setIsUsingModulatedRing(true);
	detuneSlider->setIsUsingModulatedRing(true);

    //[/UserPreSize]

    setSize (800, 120);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

	startTimer(50);
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
    unisonoSlider = nullptr;
    detuneSlider = nullptr;
    spreadSlider = nullptr;
    forceMonoButton = nullptr;


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
    carrierSelector->setBounds (getWidth() - 149 - 128, 78, 128, 28);
    fmButton->setBounds (getWidth() - 16 - 128, 16, 128, 32);
    modSelector->setBounds (getWidth() - 11 - 128, 78, 128, 28);
    fmStateLabel->setBounds (getWidth() - 12 - 264, 56, 264, 16);
    label->setBounds (560 - 264, 8, 264, 40);
    unisonoSlider->setBounds (16, 64, 128, 48);
    detuneSlider->setBounds (160, 64, 128, 48);
    spreadSlider->setBounds (296, 64, 128, 48);
    forceMonoButton->setBounds (103 + 79 - 6, 16, 128, 32);
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
    else if (buttonThatWasClicked == forceMonoButton)
    {
        //[UserButtonCode_forceMonoButton] -- add your button handler code here..
        //[/UserButtonCode_forceMonoButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void GroupBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == unisonoSlider)
    {
        //[UserSliderCode_unisonoSlider] -- add your slider handling code here..
        //[/UserSliderCode_unisonoSlider]
    }
    else if (sliderThatWasMoved == detuneSlider)
    {
        //[UserSliderCode_detuneSlider] -- add your slider handling code here..
        //[/UserSliderCode_detuneSlider]
    }
    else if (sliderThatWasMoved == spreadSlider)
    {
        //[UserSliderCode_spreadSlider] -- add your slider handling code here..
        //[/UserSliderCode_spreadSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GroupBody" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="120">
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
            virtualName="HiComboBox" explicitFocusOrder="0" pos="149Rr 78 128 28"
            tooltip="Set the carrier synthesizer" editable="0" layout="33"
            items="" textWhenNonSelected="Select Carrier" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="FM Synthesiser" id="e77edc03c117de85" memberName="fmButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="16Rr 16 128 32"
                tooltip="Enables FM Modulation&#10;" txtcol="ffffffff" buttonText="Enable FM"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <COMBOBOX name="Modulation Selection" id="96d6c3d91b3a6311" memberName="modSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="11Rr 78 128 28"
            tooltip="Set the modulation synthesizer" editable="0" layout="33"
            items="" textWhenNonSelected="Select Modulator" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="3fc5b4f17a9b1eb" memberName="fmStateLabel"
         virtualName="" explicitFocusOrder="0" pos="12Rr 56 264 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="FM deactivated" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="560r 8 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Synthesiser Group"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="26" bold="1" italic="0" justification="36"/>
  <SLIDER name="Detune" id="5539f75b4ea811c1" memberName="unisonoSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="16 64 128 48"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Detune" id="e0537e402c8fc75" memberName="detuneSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="160 64 128 48"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Detune" id="e3fe6ec018016207" memberName="spreadSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="296 64 128 48"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <TOGGLEBUTTON name="" id="de09f67cde1a32b8" memberName="forceMonoButton" virtualName="HiToggleButton"
                explicitFocusOrder="0" pos="6R 16 128 32" posRelativeX="f18e00eab8404cdf"
                tooltip="Enables FM Modulation&#10;" txtcol="ffffffff" buttonText="Force Mono"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
