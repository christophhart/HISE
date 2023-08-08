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
namespace hise { using namespace juce;
//[/Headers]

#include "SineSynthBody.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SineSynthBody::SineSynthBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (octaveSlider = new HiSlider ("Octave"));
    octaveSlider->setRange (-5, 5, 1);
    octaveSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    octaveSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    octaveSlider->addListener (this);

    addAndMakeVisible (fadeTimeLabel = new Label ("new label",
                                                  TRANS("Fade Time")));
    fadeTimeLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    fadeTimeLabel->setJustificationType (Justification::centredLeft);
    fadeTimeLabel->setEditable (false, false, false);
    fadeTimeLabel->setColour (Label::textColourId, Colours::white);
    fadeTimeLabel->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountLabel = new Label ("new label",
                                                     TRANS("Voices")));
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

    addAndMakeVisible (semiToneSlider = new HiSlider ("Semitones"));
    semiToneSlider->setTooltip (TRANS("The semitone detune"));
    semiToneSlider->setRange (-12, 12, 1);
    semiToneSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    semiToneSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    semiToneSlider->addListener (this);

    addAndMakeVisible (musicalRatio = new HiToggleButton ("Musical Ratio"));
    musicalRatio->setTooltip (TRANS("Enables FM Modulation\n"));
    musicalRatio->addListener (this);
    musicalRatio->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (saturationSlider = new HiSlider ("Semitones"));
    saturationSlider->setTooltip (TRANS("The Saturation of the Waveshaper"));
    saturationSlider->setRange (0, 1, 0.01);
    saturationSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    saturationSlider->setTextBoxStyle (Slider::TextBoxRight, false, 40, 20);
    saturationSlider->addListener (this);

    addAndMakeVisible (waveDisplay = new WaveformComponent (getProcessor()));
    waveDisplay->setName ("Waveform");

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("SINE WAVE")));
    label->setFont (GLOBAL_BOLD_FONT().withHeight(28.0f));
    label->setJustificationType (Justification::centred);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	fadeTimeLabel->setFont (GLOBAL_FONT());
    voiceAmountLabel->setFont (GLOBAL_FONT());
    voiceAmountEditor->setFont (GLOBAL_FONT());
    fadeTimeEditor->setFont (GLOBAL_FONT());

	octaveSlider->setup(getProcessor(), SineSynth::OctaveTranspose, "Octave");
	octaveSlider->setMode(HiSlider::Discrete, -5.0, 5.0, 0.0);

	musicalRatio->setup(getProcessor(), SineSynth::UseFreqRatio, "Use Freq Ratio");

	

	semiToneSlider->setup(getProcessor(), SineSynth::SemiTones, "Fine Tune");
	semiToneSlider->setMode(HiSlider::Discrete, -12.0, 12.0, 0.0);

	saturationSlider->setup(getProcessor(), SineSynth::SaturationAmount, "Saturation");
	saturationSlider->setMode(HiSlider::NormalizedPercentage);

    //[/UserPreSize]

    setSize (800, 140);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

SineSynthBody::~SineSynthBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    octaveSlider = nullptr;
    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    voiceAmountEditor = nullptr;
    fadeTimeEditor = nullptr;
    semiToneSlider = nullptr;
    musicalRatio = nullptr;
    saturationSlider = nullptr;
    waveDisplay = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SineSynthBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SineSynthBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..


    //[/UserPreResize]

    octaveSlider->setBounds (160 - (128 / 2), 21, 128, 48);
    fadeTimeLabel->setBounds ((getWidth() / 2) + 36 - (66 / 2), 10, 66, 24);
    voiceAmountLabel->setBounds ((getWidth() / 2) + -65, 11, 79, 24);
    voiceAmountEditor->setBounds ((getWidth() / 2) + -59, 29, 49, 16);
    fadeTimeEditor->setBounds ((getWidth() / 2) + 34 - (51 / 2), 29, 51, 16);
    semiToneSlider->setBounds (160 - (128 / 2), 78, 128, 48);
    musicalRatio->setBounds ((getWidth() / 2) - (128 / 2), 51, 128, 32);
    saturationSlider->setBounds (getWidth() - 148 - (128 / 2), 80, 128, 48);
    waveDisplay->setBounds (getWidth() - 149 - (136 / 2), 19, 136, 56);
    label->setBounds ((getWidth() / 2) - (128 / 2), 80, 128, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SineSynthBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == octaveSlider)
    {
        //[UserSliderCode_octaveSlider] -- add your slider handling code here..

        //[/UserSliderCode_octaveSlider]
    }
    else if (sliderThatWasMoved == semiToneSlider)
    {
        //[UserSliderCode_semiToneSlider] -- add your slider handling code here..
        //[/UserSliderCode_semiToneSlider]
    }
    else if (sliderThatWasMoved == saturationSlider)
    {
        //[UserSliderCode_saturationSlider] -- add your slider handling code here..

		
        //[/UserSliderCode_saturationSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void SineSynthBody::labelTextChanged (Label* labelThatHasChanged)
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

void SineSynthBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == musicalRatio)
    {
        //[UserButtonCode_musicalRatio] -- add your button handler code here..

		const bool useFreqRatio = getProcessor()->getAttribute(SineSynth::UseFreqRatio) >= 0.5f;

		if (!useFreqRatio)
		{
			octaveSlider->setup(getProcessor(), SineSynth::OctaveTranspose, "Octave");
			semiToneSlider->setup(getProcessor(), SineSynth::SemiTones, "Semi Tones");

			octaveSlider->setRange(-5.0, 5.0, 1.0);
			semiToneSlider->setMode(HiSlider::Discrete, -12.0, 12.0, 0.0);

		}
		else
		{
			octaveSlider->setup(getProcessor(), SineSynth::CoarseFreqRatio, "Coarse Ratio");
			semiToneSlider->setup(getProcessor(), SineSynth::FineFreqRatio, "Fine Ratio");

			octaveSlider->setRange(-5.0, 16.0, 1.0);
			semiToneSlider->setMode(HiSlider::Linear, 0.0, 1.0);
			semiToneSlider->setRange(0.0, 1.0, 0.01);

		}

		getProcessor()->sendChangeMessage();
        //[/UserButtonCode_musicalRatio]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SineSynthBody::updateGui()
{
	const bool useFreqRatio = getProcessor()->getAttribute(SineSynth::UseFreqRatio) >= 0.5f;

	if (!useFreqRatio)
	{
		octaveSlider->setup(getProcessor(), SineSynth::OctaveTranspose, "Octave");
		semiToneSlider->setup(getProcessor(), SineSynth::SemiTones, "Semi Tones");

		octaveSlider->setRange(-5.0, 5.0, 1.0);
		semiToneSlider->setMode(HiSlider::Discrete, -12.0, 12.0, 0.0);

	}
	else
	{
		octaveSlider->setup(getProcessor(), SineSynth::CoarseFreqRatio, "Coarse Ratio");
		semiToneSlider->setup(getProcessor(), SineSynth::FineFreqRatio, "Fine Ratio");

		octaveSlider->setRange(-5.0, 16.0, 1.0);
		semiToneSlider->setMode(HiSlider::Linear, 0.0, 1.0);
		semiToneSlider->setRange(0.0, 1.0, 0.01);

	}

	octaveSlider->updateValue();
	semiToneSlider->updateValue();
	musicalRatio->updateValue();
	saturationSlider->updateValue();
	
	fadeTimeEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::KillFadeTime)), dontSendNotification);
	voiceAmountEditor->setText(String((int)getProcessor()->getAttribute(ModulatorSynth::VoiceLimit)), dontSendNotification);

	
}



//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SineSynthBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="140">
  <BACKGROUND backgroundColour="8e0000"/>
  <SLIDER name="Octave" id="baa9524f7348f05" memberName="octaveSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="160c 21 128 48"
          min="-5" max="5" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="36Cc 10 66 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Fade Time" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="-65C 11 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Voices" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="-59C 29 49 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="33.5Cc 29 51 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="33"/>
  <SLIDER name="Semitones" id="c990fe6c1167b9cb" memberName="semiToneSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="160c 78 128 48"
          tooltip="The semitone detune" min="-12" max="12" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="40"
          textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="Musical Ratio" id="e77edc03c117de85" memberName="musicalRatio"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="0Cc 51 128 32"
                tooltip="Enables FM Modulation&#10;" txtcol="ffffffff" buttonText="Musical Ratio"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Semitones" id="321ac674521d8e28" memberName="saturationSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="148Rc 80 128 48"
          tooltip="The Saturation of the Waveshaper" min="0" max="1" int="0.01"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="40" textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="Waveform" id="59804142ae01ca30" memberName="waveDisplay"
                    virtualName="WavetableDisplayComponent" explicitFocusOrder="0"
                    pos="149Rc 19 136 56" class="Component" params="getProcessor()"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="0Cc 80 128 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="SINE WAVE" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
