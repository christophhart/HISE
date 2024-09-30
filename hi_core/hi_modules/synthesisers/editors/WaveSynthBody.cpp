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

#include "WaveSynthBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
WaveSynthBody::WaveSynthBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (octaveSlider = new HiSlider ("Octave 1"));
    octaveSlider->setRange (-5, 5, 1);
    octaveSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    octaveSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    octaveSlider->addListener (this);

    addAndMakeVisible (waveFormSelector = new HiComboBox ("new combo box"));
    waveFormSelector->setTooltip (TRANS("Selects the synthesiser\'s waveform"));
    waveFormSelector->setEditableText (false);
    waveFormSelector->setJustificationType (Justification::centredLeft);
    waveFormSelector->setTextWhenNothingSelected (TRANS("Select Waveform"));
    waveFormSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    waveFormSelector->addItem (TRANS("Sine"), 1);
    waveFormSelector->addItem (TRANS("Triangle"), 2);
    waveFormSelector->addItem (TRANS("Saw"), 3);
    waveFormSelector->addItem (TRANS("Square"), 4);
    waveFormSelector->addItem (TRANS("Noise"), 5);
    waveFormSelector->addItem (TRANS("Triangle 2"), 6);
    waveFormSelector->addItem (TRANS("Square 2"), 7);
    waveFormSelector->addItem (TRANS("Trapezoid 1"), 8);
    waveFormSelector->addItem (TRANS("Trapezoid 2"), 9);
    waveFormSelector->addSeparator();
    waveFormSelector->addListener (this);

    addAndMakeVisible (waveformDisplay = new WaveformComponent(getProcessor(), 0));
    waveformDisplay->setName ("new component");

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

    addAndMakeVisible (octaveSlider2 = new HiSlider ("Octave 2"));
    octaveSlider2->setRange (-5, 5, 1);
    octaveSlider2->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    octaveSlider2->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    octaveSlider2->addListener (this);

    addAndMakeVisible (waveFormSelector2 = new HiComboBox ("new combo box"));
    waveFormSelector2->setTooltip (TRANS("Selects the synthesiser\'s waveform"));
    waveFormSelector2->setEditableText (false);
    waveFormSelector2->setJustificationType (Justification::centredLeft);
    waveFormSelector2->setTextWhenNothingSelected (TRANS("Select Waveform"));
    waveFormSelector2->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    waveFormSelector2->addItem (TRANS("Sine"), 1);
    waveFormSelector2->addItem (TRANS("Triangle"), 2);
    waveFormSelector2->addItem (TRANS("Saw"), 3);
    waveFormSelector2->addItem (TRANS("Square"), 4);
    waveFormSelector2->addItem (TRANS("Noise"), 5);
    waveFormSelector2->addItem (TRANS("Triangle 2"), 6);
    waveFormSelector2->addItem (TRANS("Square 2"), 7);
    waveFormSelector2->addItem (TRANS("Trapezoid 1"), 8);
    waveFormSelector2->addItem (TRANS("Trapezoid 2"), 9);
    waveFormSelector2->addSeparator();
    waveFormSelector2->addListener (this);

    addAndMakeVisible (waveformDisplay2 = new WaveformComponent(getProcessor(), 1));
    waveformDisplay2->setName ("new component");

    addAndMakeVisible (mixSlider = new HiSlider ("Mix"));
    mixSlider->setRange (0, 100, 1);
    mixSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    mixSlider->addListener (this);

    addAndMakeVisible (panSlider = new HiSlider ("Pan 1"));
    panSlider->setRange (-100, 100, 1);
    panSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    panSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    panSlider->addListener (this);

    addAndMakeVisible (panSlider2 = new HiSlider ("Pan 2"));
    panSlider2->setRange (-100, 100, 1);
    panSlider2->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    panSlider2->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    panSlider2->addListener (this);

    addAndMakeVisible (detuneSlider2 = new HiSlider ("Detune 2"));
    detuneSlider2->setRange (-100, 100, 1);
    detuneSlider2->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    detuneSlider2->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    detuneSlider2->addListener (this);

    addAndMakeVisible (detuneSlider = new HiSlider ("Detune"));
    detuneSlider->setRange (-100, 100, 1);
    detuneSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    detuneSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    detuneSlider->addListener (this);

    addAndMakeVisible (enableSecondButton = new HiToggleButton ("enableSecondButton"));
    enableSecondButton->setButtonText (TRANS("Enable 2nd Osc"));
    enableSecondButton->addListener (this);

	addAndMakeVisible(enableSyncButton = new HiToggleButton("enableSyncButton"));
	enableSyncButton->setButtonText(TRANS("Sync 2nd Osc"));
	enableSyncButton->addListener(this);

    addAndMakeVisible (pulseSlider1 = new HiSlider ("Pulse 1"));
    pulseSlider1->setTooltip (TRANS("Select the pulse width if possible"));
    pulseSlider1->setRange (0, 1, 0.01);
    pulseSlider1->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    pulseSlider1->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    pulseSlider1->addListener (this);

    addAndMakeVisible (pulseSlider2 = new HiSlider ("Pulse 2"));
    pulseSlider2->setTooltip (TRANS("Select the pulse width if possible"));
    pulseSlider2->setRange (0, 1, 0.01);
    pulseSlider2->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    pulseSlider2->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    pulseSlider2->addListener (this);

    addAndMakeVisible (semiToneSlider1 = new HiSlider ("SemiTones 1"));
    semiToneSlider1->setRange (-12, 12, 1);
    semiToneSlider1->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    semiToneSlider1->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    semiToneSlider1->addListener (this);

    addAndMakeVisible (semiToneSlider2 = new HiSlider ("SemiTones 2"));
    semiToneSlider2->setRange (-12, 12, 1);
    semiToneSlider2->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    semiToneSlider2->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    semiToneSlider2->addListener (this);

    //[UserPreSize]

	waveFormSelector->setup(getProcessor(), WaveSynth::SpecialParameters::WaveForm1, "Waveform 1");
	waveFormSelector2->setup(getProcessor(), WaveSynth::SpecialParameters::WaveForm2, "Waveform 2");

	octaveSlider->setup(getProcessor(), WaveSynth::SpecialParameters::OctaveTranspose1, "Octave 1");
	octaveSlider->setMode(HiSlider::Discrete, -5.0, 5.0);
	octaveSlider->setRange(-5.0, 5.0, 1.0);
	octaveSlider2->setup(getProcessor(), WaveSynth::SpecialParameters::OctaveTranspose2, "Octave 2");
	octaveSlider2->setMode(HiSlider::Discrete, -5.0, 5.0);
	octaveSlider2->setRange(-5.0, 5.0, 1.0);

	detuneSlider->setup(getProcessor(), WaveSynth::SpecialParameters::Detune1, "Detune 1");
	detuneSlider2->setup(getProcessor(), WaveSynth::SpecialParameters::Detune2, "Detune 2");
	detuneSlider->setMode(HiSlider::Mode::Linear, -100.0, 100.0);
	detuneSlider2->setMode(HiSlider::Mode::Linear, -100.0, 100.0);
	detuneSlider->setTextValueSuffix("ct");
	detuneSlider2->setTextValueSuffix("ct");

	panSlider->setup(getProcessor(), WaveSynth::SpecialParameters::Pan1, "Pan 1");
	panSlider2->setup(getProcessor(), WaveSynth::SpecialParameters::Pan2, "Pan 2");
	panSlider->setMode(HiSlider::Pan);
	panSlider2->setMode(HiSlider::Pan);

	mixSlider->setup(getProcessor(), WaveSynth::SpecialParameters::Mix, "Mix");
	mixSlider->setMode(HiSlider::Mode::NormalizedPercentage);

	enableSecondButton->setup(getProcessor(), WaveSynth::SpecialParameters::EnableSecondOscillator, "Enable 2nd Osc");
	enableSyncButton->setup(getProcessor(), WaveSynth::SpecialParameters::HardSync, "Sync 2nd Osc");

	pulseSlider1->setup(getProcessor(), WaveSynth::SpecialParameters::PulseWidth1, "Pulse Width 1");
	pulseSlider1->setMode(HiSlider::Mode::NormalizedPercentage);

	pulseSlider2->setup(getProcessor(), WaveSynth::SpecialParameters::PulseWidth2, "Pulse Width 2");
	pulseSlider2->setMode(HiSlider::Mode::NormalizedPercentage);

    semiToneSlider1->setup(getProcessor(), WaveSynth::SpecialParameters::SemiTones1, "SemiTones 1");
    semiToneSlider1->setMode(HiSlider::Discrete, -12.0, 12.0);
    semiToneSlider1->setRange(-12.0, 12.0, 1.0);

    semiToneSlider2->setup(getProcessor(), WaveSynth::SpecialParameters::SemiTones2, "SemiTones 2");
    semiToneSlider2->setMode(HiSlider::Discrete, -12.0, 12.0);
    semiToneSlider2->setRange(-12.0, 12.0, 1.0);

    voiceAmountEditor->setFont(GLOBAL_FONT());
    voiceAmountLabel->setFont(GLOBAL_FONT());
    fadeTimeEditor->setFont(GLOBAL_FONT());
    fadeTimeLabel->setFont(GLOBAL_FONT());

    //[/UserPreSize]

    setSize (800, 200);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
	updateGui();
    //[/Constructor]
}

WaveSynthBody::~WaveSynthBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    octaveSlider = nullptr;
    waveFormSelector = nullptr;
    waveformDisplay = nullptr;
    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    voiceAmountEditor = nullptr;
    fadeTimeEditor = nullptr;
    octaveSlider2 = nullptr;
    waveFormSelector2 = nullptr;
    waveformDisplay2 = nullptr;
    mixSlider = nullptr;
    panSlider = nullptr;
    panSlider2 = nullptr;
    detuneSlider2 = nullptr;
    detuneSlider = nullptr;
    enableSecondButton = nullptr;
    pulseSlider1 = nullptr;
    pulseSlider2 = nullptr;
    semiToneSlider1 = nullptr;
    semiToneSlider2 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void WaveSynthBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 20.00f, Font::bold));
    g.drawText (TRANS("SYNTHESISER"),
                (getWidth() / 2) - (152 / 2), 95, 152, 30,
                Justification::centred, true);

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (16.0f, 6.0f, 278.0f, static_cast<float> (getHeight() - 16), 1.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (16.0f, 6.0f, 278.0f, static_cast<float> (getHeight() - 16), 1.000f, 1.000f);

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> (getWidth() - 16 - 278), 6.0f, 278.0f, static_cast<float> (getHeight() - 16), 1.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> (getWidth() - 16 - 278), 6.0f, 278.0f, static_cast<float> (getHeight() - 16), 1.000f, 1.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void WaveSynthBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    octaveSlider->setBounds (160, 17, 128, 48);
    waveFormSelector->setBounds (25, 42, 128, 24);
    waveformDisplay->setBounds (25, 15, 128, 24);
    fadeTimeLabel->setBounds ((getWidth() / 2) + 7, 61, 79, 24);
    voiceAmountLabel->setBounds ((getWidth() / 2) + -69, 62, 79, 24);
    voiceAmountEditor->setBounds ((getWidth() / 2) + -30 - (68 / 2), 80, 68, 16);
    fadeTimeEditor->setBounds ((getWidth() / 2) + 12, 80, 51, 16);
    octaveSlider2->setBounds (getWidth() - 161 - 128, 17, 128, 48);
    waveFormSelector2->setBounds (getWidth() - 26 - 128, 42, 128, 24);
    waveformDisplay2->setBounds (getWidth() - 26 - 128, 15, 128, 24);
    mixSlider->setBounds ((getWidth() / 2) - (128 / 2), 13, 128, 48);

    panSlider->setBounds (25, 134, 128, 48);
    panSlider2->setBounds (getWidth() - 26 - 128, 134, 128, 48);
    detuneSlider2->setBounds (getWidth() - 161 - 128, 134, 128, 48);
    detuneSlider->setBounds (160, 134, 128, 48);
    enableSecondButton->setBounds ((getWidth() / 2) + -64, 136, 128, 28);
	enableSyncButton->setBounds((getWidth() / 2) + -64, 166, 128, 28);
    pulseSlider1->setBounds (25, 77, 128, 48);
    pulseSlider2->setBounds (getWidth() - 26 - 128, 77, 128, 48);
    semiToneSlider1->setBounds (160, 77, 128, 48);
    semiToneSlider2->setBounds (getWidth() - 161 - 128, 77, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void WaveSynthBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == octaveSlider)
    {
        //[UserSliderCode_octaveSlider] -- add your slider handling code here..
        //[/UserSliderCode_octaveSlider]
    }
    else if (sliderThatWasMoved == octaveSlider2)
    {
        //[UserSliderCode_octaveSlider2] -- add your slider handling code here..
        //[/UserSliderCode_octaveSlider2]
    }
    else if (sliderThatWasMoved == mixSlider)
    {
        //[UserSliderCode_mixSlider] -- add your slider handling code here..
        //[/UserSliderCode_mixSlider]
    }
    else if (sliderThatWasMoved == panSlider)
    {
        //[UserSliderCode_panSlider] -- add your slider handling code here..
        //[/UserSliderCode_panSlider]
    }
    else if (sliderThatWasMoved == panSlider2)
    {
        //[UserSliderCode_panSlider2] -- add your slider handling code here..
        //[/UserSliderCode_panSlider2]
    }
    else if (sliderThatWasMoved == detuneSlider2)
    {
        //[UserSliderCode_detuneSlider2] -- add your slider handling code here..
        //[/UserSliderCode_detuneSlider2]
    }
    else if (sliderThatWasMoved == detuneSlider)
    {
        //[UserSliderCode_detuneSlider] -- add your slider handling code here..
        //[/UserSliderCode_detuneSlider]
    }
    else if (sliderThatWasMoved == pulseSlider1)
    {
        //[UserSliderCode_pulseSlider1] -- add your slider handling code here..
        //[/UserSliderCode_pulseSlider1]
    }
    else if (sliderThatWasMoved == pulseSlider2)
    {
        //[UserSliderCode_pulseSlider2] -- add your slider handling code here..
        //[/UserSliderCode_pulseSlider2]
    }
    else if (sliderThatWasMoved == semiToneSlider1)
    {
        //[UserSliderCode_semiToneSlider1] -- add your slider handling code here..
        //[/UserSliderCode_semiToneSlider1]
    }
    else if (sliderThatWasMoved == semiToneSlider2)
    {
        //[UserSliderCode_semiToneSlider2] -- add your slider handling code here..
        //[/UserSliderCode_semiToneSlider2]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void WaveSynthBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == waveFormSelector)
    {
        //[UserComboBoxCode_waveFormSelector] -- add your combo box handling code here..
                
        //[/UserComboBoxCode_waveFormSelector]
    }
    else if (comboBoxThatHasChanged == waveFormSelector2)
    {
        //[UserComboBoxCode_waveFormSelector2] -- add your combo box handling code here..
                
        //[/UserComboBoxCode_waveFormSelector2]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void WaveSynthBody::labelTextChanged (Label* labelThatHasChanged)
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

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void WaveSynthBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == enableSecondButton)
    {
        //[UserButtonCode_enableSecondButton] -- add your button handler code here..
        //[/UserButtonCode_enableSecondButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="WaveSynthBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="200">
  <BACKGROUND backgroundColour="ffffff">
    <TEXT pos="0Cc 95 152 30" fill="solid: 52ffffff" hasStroke="0" text="SYNTHESISER"
          fontname="Arial" fontsize="20" bold="1" italic="0" justification="36"/>
    <ROUNDRECT pos="16 6 278 16M" cornerSize="1" fill="solid: 30000000" hasStroke="1"
               stroke="1, mitered, butt" strokeColour="solid: 25ffffff"/>
    <ROUNDRECT pos="16Rr 6 278 16M" cornerSize="1" fill="solid: 30000000" hasStroke="1"
               stroke="1, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Octave 1" id="baa9524f7348f05" memberName="octaveSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="158 15 128 48"
          min="-5" max="5" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <COMBOBOX name="new combo box" id="223afd792a25b6b" memberName="waveFormSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="26 65 128 24"
            tooltip="Selects the synthesiser's waveform" editable="0" layout="33"
            items="Sine&#10;Triangle&#10;Saw&#10;Square&#10;Noise&#10;Triangle 2&#10;Square 2&#10;Trapezoid 1&#10;Trapezoid 2&#10;"
            textWhenNonSelected="Select Waveform" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="5bdd135efdbc6b85" memberName="waveformDisplay"
                    virtualName="" explicitFocusOrder="0" pos="26 15 128 48" class="WaveformComponent"
                    params="getProcessor(), 0"/>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="7C 61 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Fade Time" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="-69C 62 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Voice Amount" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="-30Cc 80 68 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="12C 80 51 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <SLIDER name="Octave 2" id="509320d2b1d2c148" memberName="octaveSlider2"
          virtualName="HiSlider" explicitFocusOrder="0" pos="161Rr 15 128 48"
          min="-5" max="5" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <COMBOBOX name="new combo box" id="749f547d3ce33f2c" memberName="waveFormSelector2"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="26Rr 65 128 24"
            tooltip="Selects the synthesiser's waveform" editable="0" layout="33"
            items="Sine&#10;Triangle&#10;Saw&#10;Square&#10;Noise&#10;Triangle 2&#10;Square 2&#10;Trapezoid 1&#10;Trapezoid 2&#10;"
            textWhenNonSelected="Select Waveform" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="82267d619093f456" memberName="waveformDisplay2"
                    virtualName="" explicitFocusOrder="0" pos="26Rr 15 128 48" class="WaveformComponent"
                    params="getProcessor(), 1"/>
  <SLIDER name="Mix" id="3ef4da35a0bb5ce1" memberName="mixSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="0Cc 13 128 48" min="0" max="100"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Pan 1" id="18299b70f4f7493" memberName="panSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="160 73 128 48" min="-100" max="100"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Pan 2" id="5fb8e21c6e3ef898" memberName="panSlider2" virtualName="HiSlider"
          explicitFocusOrder="0" pos="161Rr 73 128 48" min="-100" max="100"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Detune 2" id="bb37c0e3877e65e3" memberName="detuneSlider2"
          virtualName="HiSlider" explicitFocusOrder="0" pos="26Rr 94 128 48"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Detune" id="5539f75b4ea811c1" memberName="detuneSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="25 96 128 48"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <TOGGLEBUTTON name="enableSecondButton" id="d5fb0bb61f179f07" memberName="enableSecondButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="-64C 136 128 28"
                buttonText="Enable 2nd Osc" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="Pulse 1" id="cc96ecee717343f8" memberName="pulseSlider1"
          virtualName="HiSlider" explicitFocusOrder="0" pos="160 132 128 48"
          tooltip="Select the pulse width if possible" min="0" max="1"
          int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Pulse 2" id="2925a0b39ba3d5b8" memberName="pulseSlider2"
          virtualName="HiSlider" explicitFocusOrder="0" pos="161Rr 132 128 48"
          tooltip="Select the pulse width if possible" min="0" max="1"
          int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="SemiTones 1" id="c990fe6c1167b9cb" memberName="semiToneSlider1"
          virtualName="HiSlider" explicitFocusOrder="0" pos="158 73 128 48"
          tooltip="The semitone detune for oscillator 1" min="-12" max="12" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="SemiTones 2" id="321ac674521d8e28" memberName="semiToneSlider2"
          virtualName="HiSlider" explicitFocusOrder="0" pos="161Rr 73 128 48"
          tooltip="The semitone detune for oscillator 2" min="-12" max="12" int="1"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
