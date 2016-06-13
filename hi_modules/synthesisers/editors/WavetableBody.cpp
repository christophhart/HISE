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

#include "WavetableBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
WavetableBody::WavetableBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (waveTableDisplay = new WavetableDisplayComponent (dynamic_cast<WavetableSynth*>(getProcessor())));
    waveTableDisplay->setName ("new component");

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

    addAndMakeVisible (component = new TableEditor (dynamic_cast<WavetableSynth*>(getProcessor())->getGainTable()));
    component->setName ("new component");

    addAndMakeVisible (hiqButton = new HiToggleButton ("HQ"));
    hiqButton->setTooltip (TRANS("Enables HQ rendering mode (more CPU intensive)"));
    hiqButton->addListener (this);
    hiqButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	fadeTimeLabel->setFont (GLOBAL_FONT());
    voiceAmountLabel->setFont (GLOBAL_FONT());
    voiceAmountEditor->setFont (GLOBAL_FONT());
    fadeTimeEditor->setFont (GLOBAL_FONT());

	hiqButton->setup(getProcessor(), WavetableSynth::HqMode, "HQ");



    //[/UserPreSize]

    setSize (800, 130);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

    //[/Constructor]
}

WavetableBody::~WavetableBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    waveTableDisplay = nullptr;
    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    voiceAmountEditor = nullptr;
    fadeTimeEditor = nullptr;
    component = nullptr;
    hiqButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void WavetableBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x23ffffff));
    g.drawRect (16, 0, getWidth() - 32, getHeight() - 16, 1);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("WAVETABLE"),
                getWidth() - 24 - 200, 4, 200, 30,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void WavetableBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    waveTableDisplay->setBounds (32, 10, 160, 94);
    fadeTimeLabel->setBounds (getWidth() - 33 - 79, 31, 79, 24);
    voiceAmountLabel->setBounds (getWidth() - 109 - 79, 32, 79, 24);
    voiceAmountEditor->setBounds (getWidth() - 115 - 68, 50, 68, 16);
    fadeTimeEditor->setBounds (getWidth() - 56 - 51, 50, 51, 16);
    component->setBounds (203, 10, getWidth() - 412, 91);
    hiqButton->setBounds (getWidth() - 54 - 128, 72, 128, 32);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void WavetableBody::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == voiceAmountEditor)
    {
        //[UserLabelCode_voiceAmountEditor] -- add your label text handling code here..
        //[/UserLabelCode_voiceAmountEditor]
    }
    else if (labelThatHasChanged == fadeTimeEditor)
    {
        //[UserLabelCode_fadeTimeEditor] -- add your label text handling code here..
        //[/UserLabelCode_fadeTimeEditor]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void WavetableBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == hiqButton)
    {
        //[UserButtonCode_hiqButton] -- add your button handler code here..
        //[/UserButtonCode_hiqButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void WavetableDisplayComponent::timerCallback()
{
	ModulatorSynthVoice *voice = dynamic_cast<ModulatorSynthVoice*>(synth->getVoice(0));

	if (voice == nullptr) return;

	ModulatorSynthSound *sound = dynamic_cast<ModulatorSynthSound*>(voice->getCurrentlyPlayingSound().get());

	if (dynamic_cast<WavetableSound*>(sound) != nullptr)
	{
		WavetableSound *wavetableSound = dynamic_cast<WavetableSound*>(sound);
		WavetableSynthVoice *wavetableVoice = dynamic_cast<WavetableSynthVoice*>(voice);

		const int tableIndex = wavetableVoice->getCurrentTableIndex();
		tableValues = wavetableSound->getWaveTableData(tableIndex);
		tableLength = wavetableSound->getTableSize();

		normalizeValue = 1.0f / wavetableSound->getMaxLevel();

		repaint();
	}
	else if (!isDisplayForWavetableSynth())
	{
		tableValues = dynamic_cast<SineSynth*>(synth)->getSaturatedTableValues();
		tableLength = 128;
		normalizeValue = 1.0f;

		repaint();
	}
	else
	{
		tableValues = nullptr;
		repaint();
	}



}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="WavetableBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="0" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="130">
  <BACKGROUND backgroundColour="ffffff">
    <RECT pos="16 0 32M 16M" fill="solid: 43a52a" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 23ffffff"/>
    <TEXT pos="24Rr 4 200 30" fill="solid: 52ffffff" hasStroke="0" text="WAVETABLE"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="5bdd135efdbc6b85" memberName="waveTableDisplay"
                    virtualName="" explicitFocusOrder="0" pos="32 10 160 94" class="WavetableDisplayComponent"
                    params="dynamic_cast&lt;WavetableSynth*&gt;(getProcessor())"/>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="33Rr 31 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Fade Time" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="109Rr 32 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Voice Amount" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="115Rr 50 68 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="56Rr 50 51 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <GENERICCOMPONENT name="new component" id="86c524f43e825eb1" memberName="component"
                    virtualName="" explicitFocusOrder="0" pos="203 10 412M 91" class="TableEditor"
                    params="dynamic_cast&lt;WavetableSynth*&gt;(getProcessor())-&gt;getGainTable()"/>
  <TOGGLEBUTTON name="HQ" id="dfdc6e861a38fb62" memberName="hiqButton" virtualName="HiToggleButton"
                explicitFocusOrder="0" pos="54Rr 72 128 32" tooltip="Enables HQ rendering mode (more CPU intensive)"
                txtcol="ffffffff" buttonText="HQ" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
