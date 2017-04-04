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

#include "AudioFileEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AudioFileEnvelopeEditor::AudioFileEnvelopeEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("envelope follower")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (smoothSlider = new HiSlider ("Smooth Time"));
    smoothSlider->setTooltip (TRANS("The smoothing factor for the envelope follower\n"));
    smoothSlider->setRange (0, 5000, 1);
    smoothSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    smoothSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    smoothSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    smoothSlider->addListener (this);

    addAndMakeVisible (retriggerButton = new HiToggleButton ("Legato"));
    retriggerButton->setTooltip (TRANS("Disables retriggering of the LFO if multiple keys are pressed."));
    retriggerButton->addListener (this);
    retriggerButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (sampleBufferContent = new AudioSampleBufferComponent (dynamic_cast<AudioSampleProcessor*>(getProcessor())->getCache()));
    sampleBufferContent->setName ("new component");

    addAndMakeVisible (modeSelector = new HiComboBox ("Mode Selection"));
    modeSelector->setTooltip (TRANS("Select the Envelope Follower mode"));
    modeSelector->setEditableText (false);
    modeSelector->setJustificationType (Justification::centredLeft);
    modeSelector->setTextWhenNothingSelected (TRANS("Select Envelope Mode"));
    modeSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    modeSelector->addItem (TRANS("Simple LP"), 1);
    modeSelector->addItem (TRANS("Ramped Average"), 2);
    modeSelector->addItem (TRANS("Attack Release Follower"), 3);
    modeSelector->addListener (this);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setTooltip (TRANS("The smoothing factor for the envelope follower\n"));
    gainSlider->setRange (0, 5000, 1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    gainSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    gainSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    gainSlider->addListener (this);

    addAndMakeVisible (attackSlider = new HiSlider ("Attack"));
    attackSlider->setTooltip (TRANS("The attack time of the envelope follower"));
    attackSlider->setRange (0, 5000, 1);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackSlider->addListener (this);

    addAndMakeVisible (releaseSlider = new HiSlider ("Release"));
    releaseSlider->setTooltip (TRANS("The Release time of the Envelope Follower"));
    releaseSlider->setRange (0, 5000, 1);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    releaseSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    releaseSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    releaseSlider->addListener (this);

    addAndMakeVisible (offsetSlider = new HiSlider ("Offset"));
    offsetSlider->setTooltip (TRANS("Sets the lowest value for the envelope follower output."));
    offsetSlider->setRange (0, 1, 0.01);
    offsetSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    offsetSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    offsetSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    offsetSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    offsetSlider->addListener (this);

    addAndMakeVisible (rampSlider = new HiSlider ("Ramp Length"));
    rampSlider->setTooltip (TRANS("Sets the ramp length"));
    rampSlider->setRange (0, 1, 0.01);
    rampSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rampSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    rampSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    rampSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    rampSlider->addListener (this);

    addAndMakeVisible (syncToHost = new HiComboBox ("Mode Selection"));
    syncToHost->setTooltip (TRANS("Select the Envelope Follower mode"));
    syncToHost->setEditableText (false);
    syncToHost->setJustificationType (Justification::centredLeft);
    syncToHost->setTextWhenNothingSelected (TRANS("Sync to Tempo"));
    syncToHost->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    syncToHost->addItem (TRANS("Free running"), 1);
    syncToHost->addItem (TRANS("1 Beat"), 2);
    syncToHost->addItem (TRANS("2 Beats"), 3);
    syncToHost->addItem (TRANS("1 Bar"), 4);
    syncToHost->addItem (TRANS("2 Bars"), 5);
    syncToHost->addItem (TRANS("4 Bars"), 6);
    syncToHost->addSeparator();
    syncToHost->addListener (this);


    //[UserPreSize]

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

	AudioSampleProcessor *asp = dynamic_cast<AudioSampleProcessor*>(getProcessor());

	sampleBufferContent->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());

	sampleBufferContent->addChangeListener(asp);

	getProcessor()->addChangeListener(sampleBufferContent);

	sampleBufferContent->setBackgroundColour(getProcessor()->getColour().withMultipliedBrightness(.6f));

	sampleBufferContent->addAreaListener(this);

	modeSelector->setup(getProcessor(), AudioFileEnvelope::Parameters::Mode, "Envelope Mode");
	attackSlider->setup(getProcessor(), AudioFileEnvelope::Parameters::AttackTime, "Attack Time");
	attackSlider->setMode(HiSlider::Time);

	releaseSlider->setup(getProcessor(), AudioFileEnvelope::Parameters::ReleaseTime, "Release Time");
	releaseSlider->setMode(HiSlider::Time);

	gainSlider->setup(getProcessor(), AudioFileEnvelope::Parameters::Gain, "Gain");
	gainSlider->setMode(HiSlider::Linear, 1.0, 10.0, 5.0);

	retriggerButton->setup(getProcessor(), AudioFileEnvelope::Legato, "Legato");
	smoothSlider->setup(getProcessor(), AudioFileEnvelope::SmoothTime, "Smooth Time");
	smoothSlider->setMode(HiSlider::Time, 0.0, 8000.0, 60.0);

	offsetSlider->setup(getProcessor(), AudioFileEnvelope::Offset, "Offset");
	offsetSlider->setRange(-1.0, 1.0, 0.01);

	rampSlider->setup(getProcessor(), AudioFileEnvelope::RampLength, "Ramp Length");
	rampSlider->setMode(HiSlider::Discrete, 512, 8192);

	syncToHost->setup(getProcessor(), AudioFileEnvelope::SyncMode, "Sync To Host");

#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif
    
    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));

    //[/UserPreSize]

    setSize (800, 310);


    //[Constructor] You can add your own custom stuff here..
    h = getHeight();



    //[/Constructor]
}

AudioFileEnvelopeEditor::~AudioFileEnvelopeEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    smoothSlider = nullptr;
    retriggerButton = nullptr;
    sampleBufferContent = nullptr;
    modeSelector = nullptr;
    gainSlider = nullptr;
    attackSlider = nullptr;
    releaseSlider = nullptr;
    offsetSlider = nullptr;
    rampSlider = nullptr;
    syncToHost = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AudioFileEnvelopeEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 16), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 16), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AudioFileEnvelopeEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (getWidth() - 48 - 264, 16, 264, 40);
    smoothSlider->setBounds ((getWidth() / 2) + 31 - (128 / 2), 18, 128, 48);
    retriggerButton->setBounds (59, 55, 128, 32);
    sampleBufferContent->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 141, getWidth() - 112, 144);
    modeSelector->setBounds (60, 20, 128, 28);
    gainSlider->setBounds ((getWidth() / 2) + -60 - 128, 18, 128, 48);
    attackSlider->setBounds ((getWidth() / 2) + -33, 80, 128, 48);
    releaseSlider->setBounds ((getWidth() / 2) + 111, 80, 128, 48);
    offsetSlider->setBounds ((getWidth() / 2) + -60 - 128, 80, 128, 48);
    rampSlider->setBounds ((getWidth() / 2) + -33, 80, 128, 48);
    syncToHost->setBounds (60, 95, 128, 28);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AudioFileEnvelopeEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == smoothSlider)
    {
        //[UserSliderCode_smoothSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothSlider]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == attackSlider)
    {
        //[UserSliderCode_attackSlider] -- add your slider handling code here..
        //[/UserSliderCode_attackSlider]
    }
    else if (sliderThatWasMoved == releaseSlider)
    {
        //[UserSliderCode_releaseSlider] -- add your slider handling code here..
        //[/UserSliderCode_releaseSlider]
    }
    else if (sliderThatWasMoved == offsetSlider)
    {
        //[UserSliderCode_offsetSlider] -- add your slider handling code here..
        //[/UserSliderCode_offsetSlider]
    }
    else if (sliderThatWasMoved == rampSlider)
    {
        //[UserSliderCode_rampSlider] -- add your slider handling code here..
        //[/UserSliderCode_rampSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void AudioFileEnvelopeEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == retriggerButton)
    {
        //[UserButtonCode_retriggerButton] -- add your button handler code here..
        //[/UserButtonCode_retriggerButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void AudioFileEnvelopeEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == modeSelector)
    {
        //[UserComboBoxCode_modeSelector] -- add your combo box handling code here..
		getProcessor()->sendChangeMessage();
        //[/UserComboBoxCode_modeSelector]
    }
    else if (comboBoxThatHasChanged == syncToHost)
    {
        //[UserComboBoxCode_syncToHost] -- add your combo box handling code here..
        //[/UserComboBoxCode_syncToHost]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AudioFileEnvelopeEditor"
                 componentName="" parentClasses="public ProcessorEditorBody, public Timer, public AudioDisplayComponent::Listener"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="310">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 16M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="48Rr 16 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="envelope follower"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  <SLIDER name="Smooth Time" id="74a9e7103fec3764" memberName="smoothSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="31Cc 18 128 48"
          tooltip="The smoothing factor for the envelope follower&#10;"
          thumbcol="80666666" textboxtext="ffffffff" min="0" max="5000"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="Legato" id="dfdc6e861a38fb62" memberName="retriggerButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="59 55 128 32"
                tooltip="Disables retriggering of the LFO if multiple keys are pressed."
                txtcol="ffffffff" buttonText="Legato" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="sampleBufferContent"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 141 112M 144"
                    class="AudioSampleBufferComponent" params="dynamic_cast&lt;AudioSampleProcessor*&gt;(getProcessor())-&gt;getCache()"/>
  <COMBOBOX name="Mode Selection" id="223afd792a25b6b" memberName="modeSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="60 20 128 28"
            tooltip="Select the Envelope Follower mode" editable="0" layout="33"
            items="Simple LP&#10;Ramped Average&#10;Attack Release Follower"
            textWhenNonSelected="Select Envelope Mode" textWhenNoItems="(no choices)"/>
  <SLIDER name="Gain" id="8937c9d0cce27dee" memberName="gainSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-60Cr 18 128 48" tooltip="The smoothing factor for the envelope follower&#10;"
          thumbcol="80666666" textboxtext="ffffffff" min="0" max="5000"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Attack" id="eba5a14864875f77" memberName="attackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-33C 80 128 48"
          tooltip="The attack time of the envelope follower" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="5000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Release" id="97ca18d8a7d619f3" memberName="releaseSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="111C 80 128 48"
          tooltip="The Release time of the Envelope Follower" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="5000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Offset" id="6e3a02fd45332ac7" memberName="offsetSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-60Cr 80 128 48"
          tooltip="Sets the lowest value for the envelope follower output."
          thumbcol="80666666" textboxtext="ffffffff" min="0" max="1" int="0.01"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Ramp Length" id="c86ec072bfac4510" memberName="rampSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-33C 80 128 48"
          tooltip="Sets the ramp length" thumbcol="80666666" textboxtext="ffffffff"
          min="0" max="1" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <COMBOBOX name="Mode Selection" id="63f5b1527f75c45b" memberName="syncToHost"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="60 95 128 28"
            tooltip="Select the Envelope Follower mode" editable="0" layout="33"
            items="Free running&#10;1 Beat&#10;2 Beats&#10;1 Bar&#10;2 Bars&#10;4 Bars&#10;"
            textWhenNonSelected="Sync to Tempo" textWhenNoItems="(no choices)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
