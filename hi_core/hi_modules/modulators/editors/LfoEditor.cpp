/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.3

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

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

    frequencySlider.reset (new HiSlider ("Frequency"));
    addAndMakeVisible (frequencySlider.get());
    frequencySlider->setTooltip (TRANS("Adjust the LFO Frequency"));
    frequencySlider->setRange (0.5, 40, 0.01);
    frequencySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    frequencySlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    frequencySlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    frequencySlider->setColour (Slider::textBoxTextColourId, Colours::white);
    frequencySlider->addListener (this);

    fadeInSlider.reset (new HiSlider ("Fadein"));
    addAndMakeVisible (fadeInSlider.get());
    fadeInSlider->setTooltip (TRANS("The Fade in time after each key press"));
    fadeInSlider->setRange (0, 5000, 1);
    fadeInSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    fadeInSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    fadeInSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    fadeInSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    fadeInSlider->addListener (this);

    label.reset (new Label ("new label",
                            TRANS("lfo")));
    addAndMakeVisible (label.get());
    label->setFont (Font ("Arial", 24.00f, Font::plain).withTypefaceStyle ("Bold"));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    waveFormSelector.reset (new HiComboBox ("Waveform Selection"));
    addAndMakeVisible (waveFormSelector.get());
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

    waveFormSelector->setBounds (59, 68, 128, 28);

    waveformDisplay.reset (new WaveformComponent (getProcessor()));
    addAndMakeVisible (waveformDisplay.get());
    waveformDisplay->setName ("new component");

    waveformDisplay->setBounds (59, 17, 128, 48);

    tempoSyncButton.reset (new HiToggleButton ("Tempo Sync"));
    addAndMakeVisible (tempoSyncButton.get());
    tempoSyncButton->setTooltip (TRANS("Enables sync to Host Tempo"));
    tempoSyncButton->addListener (this);
    tempoSyncButton->setColour (ToggleButton::textColourId, Colours::white);

	clockSyncButton.reset(new HiToggleButton("Clock Sync"));
	addAndMakeVisible(clockSyncButton.get());
	clockSyncButton->setTooltip(TRANS("Enables sync to Master Clock"));
	clockSyncButton->addListener(this);
	clockSyncButton->setColour(ToggleButton::textColourId, Colours::white);

    retriggerButton.reset (new HiToggleButton ("Legato"));
    addAndMakeVisible (retriggerButton.get());
    retriggerButton->setTooltip (TRANS("Disables retriggering of the LFO if multiple keys are pressed."));
    retriggerButton->addListener (this);
    retriggerButton->setColour (ToggleButton::textColourId, Colours::white);

    waveformTable.reset (new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<LfoModulator*>(getProcessor())->getTable(0)));
    addAndMakeVisible (waveformTable.get());
    waveformTable->setName ("new component");

    smoothTimeSlider.reset (new HiSlider ("Smooth Time"));
    addAndMakeVisible (smoothTimeSlider.get());
    smoothTimeSlider->setTooltip (TRANS("The smoothing factor for the oscillator"));
    smoothTimeSlider->setRange (0, 5000, 1);
    smoothTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothTimeSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    smoothTimeSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    smoothTimeSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    smoothTimeSlider->addListener (this);

    loopButton.reset (new HiToggleButton ("Loop"));
    addAndMakeVisible (loopButton.get());
    loopButton->setTooltip (TRANS("Disables looping of the Oscillator"));
    loopButton->addListener (this);
    loopButton->setColour (ToggleButton::textColourId, Colours::white);

    phaseSlider.reset (new HiSlider ("Phase"));
    addAndMakeVisible (phaseSlider.get());
    phaseSlider->setTooltip (TRANS("The phase offset"));
    phaseSlider->setRange (0, 5000, 1);
    phaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    phaseSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    phaseSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    phaseSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    phaseSlider->addListener (this);


    //[UserPreSize]



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

	clockSyncButton->setup(getProcessor(), LfoModulator::SyncToMasterClock, "Clock Sync");

    ProcessorHelpers::connectTableEditor(*waveformTable, getProcessor());

    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));

	loopButton->setup(getProcessor(), LfoModulator::Parameters::LoopEnabled, "Loop On");

	addAndMakeVisible(stepPanel = new SliderPack());
	stepPanel->setSliderPackData(dynamic_cast<ExternalDataHolder*>(getProcessor())->getSliderPack(0));

	stepPanel->setVisible(false);
	stepPanel->setStepSize(0.01);

	phaseSlider->setup(getProcessor(), LfoModulator::Parameters::PhaseOffset, "Phase Offset");
	phaseSlider->setMode(HiSlider::NormalizedPercentage);

    tableUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Custom;
    stepsUsed = getProcessor()->getAttribute(LfoModulator::WaveFormType) == LfoModulator::Steps;
    
    //[/UserPreSize]

    setSize (800, 255);


    //[Constructor] You can add your own custom stuff here..

	startTimer(150);

	//ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    
    
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
    loopButton = nullptr;
    phaseSlider = nullptr;
	clockSyncButton = nullptr;


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
    fadeInSlider->setBounds ((getWidth() / 2) + -55, 16, 120, 48);
    label->setBounds (getWidth() - 50 - 264, 7, 264, 40);
    tempoSyncButton->setBounds ((getWidth() / 2) + -73 - 128, 68, 128, 32);

	clockSyncButton->setBounds((getWidth() / 2) + -55, 68, 128, 32);

    retriggerButton->setBounds ((getWidth() / 2) + 84, 68, 128, 32);
    waveformTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 111, getWidth() - 112, 121);
    smoothTimeSlider->setBounds ((getWidth() / 2) + 85, 16, 120, 48);
    loopButton->setBounds ((getWidth() / 2) + 213, 68, 120, 32);
    phaseSlider->setBounds ((getWidth() / 2) + 213, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..

	waveformTable->setVisible(tableUsed && !stepsUsed);



	stepPanel->setVisible(!tableUsed && stepsUsed);

	int maxWidth = waveformTable->getWidth();

	int numSteps = stepPanel->getNumSliders();

	if (numSteps > 0)
	{
		int stepWidth = numSteps * (maxWidth / numSteps);
		int stepX = (getWidth() - stepWidth) / 2;

		stepPanel->setBounds(stepX, waveformTable->getY(), stepWidth, waveformTable->getHeight());
	}

    //[/UserResized]
}

void LfoEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == frequencySlider.get())
    {
        //[UserSliderCode_frequencySlider] -- add your slider handling code here..

        //[/UserSliderCode_frequencySlider]
    }
    else if (sliderThatWasMoved == fadeInSlider.get())
    {
        //[UserSliderCode_fadeInSlider] -- add your slider handling code here..

        //[/UserSliderCode_fadeInSlider]
    }
    else if (sliderThatWasMoved == smoothTimeSlider.get())
    {
        //[UserSliderCode_smoothTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothTimeSlider]
    }
    else if (sliderThatWasMoved == phaseSlider.get())
    {
        //[UserSliderCode_phaseSlider] -- add your slider handling code here..
        //[/UserSliderCode_phaseSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void LfoEditorBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == waveFormSelector.get())
    {
        //[UserComboBoxCode_waveFormSelector] -- add your combo box handling code here..



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

    if (buttonThatWasClicked == tempoSyncButton.get())
    {
        //[UserButtonCode_tempoSyncButton] -- add your button handler code here..





        //[/UserButtonCode_tempoSyncButton]
    }
    else if (buttonThatWasClicked == retriggerButton.get())
    {
        //[UserButtonCode_retriggerButton] -- add your button handler code here..
        //[/UserButtonCode_retriggerButton]
    }
    else if (buttonThatWasClicked == loopButton.get())
    {
        //[UserButtonCode_loopButton] -- add your button handler code here..
        //[/UserButtonCode_loopButton]
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

<JUCER_COMPONENT documentType="Component" className="LfoEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="255">
  <BACKGROUND backgroundColour="ffffff"/>
  <SLIDER name="Frequency" id="9ef32c38be6d2f66" memberName="frequencySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-73Cr 16 128 48"
          tooltip="Adjust the LFO Frequency" thumbcol="80666666" textboxtext="ffffffff"
          min="0.5" max="40.0" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <SLIDER name="Fadein" id="1ef615987ac878bd" memberName="fadeInSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-55C 16 120 48"
          tooltip="The Fade in time after each key press" thumbcol="80666666"
          textboxtext="ffffffff" min="0.0" max="5000.0" int="1.0" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="50Rr 7 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="lfo" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24.0" kerning="0.0" bold="1" italic="0" justification="34"
         typefaceStyle="Bold"/>
  <COMBOBOX name="Waveform Selection" id="223afd792a25b6b" memberName="waveFormSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="59 68 128 28"
            tooltip="Selects the synthesiser's waveform" editable="0" layout="33"
            items="Sine&#10;Triangle&#10;Saw&#10;Square&#10;Random&#10;Custom"
            textWhenNonSelected="Select Waveform" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="5bdd135efdbc6b85" memberName="waveformDisplay"
                    virtualName="" explicitFocusOrder="0" pos="59 17 128 48" class="WaveformComponent"
                    params="getProcessor()"/>
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
                    class="TableEditor" params="getProcessor()-&gt;getMainController()-&gt;getControlUndoManager(), static_cast&lt;LfoModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <SLIDER name="Smooth Time" id="74a9e7103fec3764" memberName="smoothTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="85C 16 120 48"
          tooltip="The smoothing factor for the oscillator" thumbcol="80666666"
          textboxtext="ffffffff" min="0.0" max="5000.0" int="1.0" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="1"/>
  <TOGGLEBUTTON name="Loop" id="e2c1c8f56fcae3d2" memberName="loopButton" virtualName="HiToggleButton"
                explicitFocusOrder="0" pos="84C 68 120 32" tooltip="Disables looping of the Oscillator"
                txtcol="ffffffff" buttonText="Loop" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="Phase" id="d2994a891ecf4f27" memberName="phaseSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="213C 56 128 48" tooltip="The phase offset"
          thumbcol="80666666" textboxtext="ffffffff" min="0.0" max="5000.0"
          int="1.0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

