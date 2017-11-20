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

#include "LfoEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
LfoEditorBody::LfoEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (frequencySlider = new HiSlider ("Frequency"));
    frequencySlider->setTooltip (TRANS("Adjust the LFO Frequency"));
    frequencySlider->setRange (0.5, 40, 0.01);
    frequencySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    frequencySlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    frequencySlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    frequencySlider->setColour (Slider::textBoxTextColourId, Colours::white);
    frequencySlider->addListener (this);

    addAndMakeVisible (fadeInSlider = new HiSlider ("Fadein"));
    fadeInSlider->setTooltip (TRANS("The Fade in time after each key press"));
    fadeInSlider->setRange (0, 5000, 1);
    fadeInSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    fadeInSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    fadeInSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    fadeInSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    fadeInSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("lfo")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (waveFormSelector = new HiComboBox ("Waveform Selection"));
    waveFormSelector->setTooltip (TRANS("Selects the synthesiser\'s waveform"));
    waveFormSelector->setEditableText (false);
    waveFormSelector->setJustificationType (Justification::centredLeft);
    waveFormSelector->setTextWhenNothingSelected (TRANS("Select Waveform"));
    waveFormSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    waveFormSelector->addItem (TRANS("Sine"), 1);
    waveFormSelector->addItem (TRANS("Triangle"), 2);
    waveFormSelector->addItem (TRANS("Saw"), 3);
    waveFormSelector->addItem (TRANS("Square"), 4);
    waveFormSelector->addItem (TRANS("Random"), 5);
    waveFormSelector->addItem (TRANS("Custom"), 6);
	waveFormSelector->addItem(TRANS("Steps"), 7);
    waveFormSelector->addListener (this);

    addAndMakeVisible (waveformDisplay = new WaveformComponent());
    waveformDisplay->setName ("new component");

    addAndMakeVisible (tempoSyncButton = new HiToggleButton ("Tempo Sync"));
    tempoSyncButton->setTooltip (TRANS("Enables sync to Host Tempo"));
    tempoSyncButton->addListener (this);
    tempoSyncButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (retriggerButton = new HiToggleButton ("Legato"));
    retriggerButton->setTooltip (TRANS("Disables retriggering of the LFO if multiple keys are pressed."));
    retriggerButton->addListener (this);
    retriggerButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (waveformTable = new TableEditor (static_cast<LfoModulator*>(getProcessor())->getTable()));
    waveformTable->setName ("new component");

    addAndMakeVisible (smoothTimeSlider = new HiSlider ("Smooth Time"));
    smoothTimeSlider->setTooltip (TRANS("The smoothing factor for the oscillator"));
    smoothTimeSlider->setRange (0, 5000, 1);
    smoothTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothTimeSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    smoothTimeSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    smoothTimeSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    smoothTimeSlider->addListener (this);


    //[UserPreSize]

	waveformDisplay->setSelector(waveFormSelector);

	waveFormSelector->setup(getProcessor(), LfoModulator::WaveFormType, "Waveform");

	tableUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Custom;

	frequencySlider->setup(getProcessor(), LfoModulator::Frequency, "Frequency");
	frequencySlider->setMode(HiSlider::Frequency, 0.1, 40.0, 10.0);

	frequencySlider->setIsUsingModulatedRing(true);

	smoothTimeSlider->setup(getProcessor(), LfoModulator::SmoothingTime, "Smoothing");
	smoothTimeSlider->setMode(HiSlider::Time, 0.0, 1000.0, 100.0);

	fadeInSlider->setup(getProcessor(), LfoModulator::FadeIn, "Fadein Time");
	fadeInSlider->setMode(HiSlider::Time, 0.0, 3000.0, 500.0);

	retriggerButton->setup(getProcessor(), LfoModulator::Legato, "Legato Mode");

	tempoSyncButton->setup(getProcessor(), LfoModulator::TempoSync, "Tempo Sync");

	tempoSyncButton->setNotificationType(sendNotification);

	waveformTable->connectToLookupTableProcessor(getProcessor());

    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));
    
    //[/UserPreSize]

	addAndMakeVisible(stepPanel = new SliderPack(dynamic_cast<SliderPackProcessor*>(getProcessor())->getSliderPackData(0)));

	stepPanel->setVisible(false);

    setSize (800, 255);


    //[Constructor] You can add your own custom stuff here..

	startTimer(150);

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

	h = getHeight();
    //[/Constructor]
}

LfoEditorBody::~LfoEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    frequencySlider = nullptr;
    fadeInSlider = nullptr;
    label = nullptr;
    waveFormSelector = nullptr;
    waveformDisplay = nullptr;
    tempoSyncButton = nullptr;
    retriggerButton = nullptr;
    waveformTable = nullptr;
    smoothTimeSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void LfoEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void LfoEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    frequencySlider->setBounds ((getWidth() / 2) + -73 - 128, 16, 128, 48);
    fadeInSlider->setBounds ((getWidth() / 2) + -55, 16, 128, 48);
    label->setBounds (getWidth() - 50 - 264, 7, 264, 40);
    waveFormSelector->setBounds (59, 68, 128, 28);
    waveformDisplay->setBounds (59, 17, 128, 48);
    tempoSyncButton->setBounds ((getWidth() / 2) + -73 - 128, 68, 128, 32);
    retriggerButton->setBounds ((getWidth() / 2) + -55, 68, 128, 32);
    waveformTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 111, getWidth() - 112, 121);
    smoothTimeSlider->setBounds ((getWidth() / 2) + 88, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..

	waveformTable->setVisible(tableUsed && !stepsUsed);



	stepPanel->setVisible(!tableUsed && stepsUsed);

	int maxWidth = waveformTable->getWidth();

	int numSteps = stepPanel->getNumSliders();

	int stepWidth = numSteps * (maxWidth / numSteps);
	int stepX = (getWidth() - stepWidth) / 2;

	stepPanel->setBounds(stepX, waveformTable->getY(), stepWidth, waveformTable->getHeight());

    //[/UserResized]
}

void LfoEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == frequencySlider)
    {
        //[UserSliderCode_frequencySlider] -- add your slider handling code here..

        //[/UserSliderCode_frequencySlider]
    }
    else if (sliderThatWasMoved == fadeInSlider)
    {
        //[UserSliderCode_fadeInSlider] -- add your slider handling code here..

        //[/UserSliderCode_fadeInSlider]
    }
    else if (sliderThatWasMoved == smoothTimeSlider)
    {
        //[UserSliderCode_smoothTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothTimeSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void LfoEditorBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == waveFormSelector)
    {
        //[UserComboBoxCode_waveFormSelector] -- add your combo box handling code here..

		waveformDisplay->setType(waveFormSelector->getSelectedId());

		const bool newTableUsed = waveFormSelector->getSelectedId() == LfoModulator::Custom;
		const bool newStepsUsed = waveFormSelector->getSelectedId() == LfoModulator::Steps;

		if (newTableUsed != tableUsed || newStepsUsed != stepsUsed)
		{
			tableUsed = newTableUsed;
			stepsUsed = newStepsUsed;
			refreshBodySize();
			resized();
		}

        //[/UserComboBoxCode_waveFormSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void LfoEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == tempoSyncButton)
    {
        //[UserButtonCode_tempoSyncButton] -- add your button handler code here..

		



        //[/UserButtonCode_tempoSyncButton]
    }
    else if (buttonThatWasClicked == retriggerButton)
    {
        //[UserButtonCode_retriggerButton] -- add your button handler code here..
        //[/UserButtonCode_retriggerButton]
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

<JUCER_COMPONENT documentType="Component" className="LfoEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="255">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 16M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Frequency" id="9ef32c38be6d2f66" memberName="frequencySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-73Cr 16 128 48"
          tooltip="Adjust the LFO Frequency" thumbcol="80666666" textboxtext="ffffffff"
          min="0.5" max="40" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Fadein" id="1ef615987ac878bd" memberName="fadeInSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-55C 16 128 48"
          tooltip="The Fade in time after each key press" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="5000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="50Rr 7 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="lfo" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
  <COMBOBOX name="Waveform Selection" id="223afd792a25b6b" memberName="waveFormSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="59 68 128 28"
            tooltip="Selects the synthesiser's waveform" editable="0" layout="33"
            items="Sine&#10;Triangle&#10;Saw&#10;Square&#10;Random&#10;Custom"
            textWhenNonSelected="Select Waveform" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="5bdd135efdbc6b85" memberName="waveformDisplay"
                    virtualName="" explicitFocusOrder="0" pos="59 17 128 48" class="WaveformComponent"
                    params=""/>
  <TOGGLEBUTTON name="Tempo Sync" id="e77edc03c117de85" memberName="tempoSyncButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="-73Cr 68 128 32"
                tooltip="Enables sync to Host Tempo" txtcol="ffffffff" buttonText="Tempo Sync"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="Legato" id="dfdc6e861a38fb62" memberName="retriggerButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="-55C 68 128 32"
                tooltip="Disables retriggering of the LFO if multiple keys are pressed."
                txtcol="ffffffff" buttonText="Legato" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="waveformTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 111 112M 121"
                    class="TableEditor" params="static_cast&lt;LfoModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <SLIDER name="Smooth Time" id="74a9e7103fec3764" memberName="smoothTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="88C 16 128 48"
          tooltip="The smoothing factor for the oscillator" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="5000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
