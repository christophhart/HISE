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

#include "PitchWheelEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PitchWheelEditorBody::PitchWheelEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("pitch wheel")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (midiTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), dynamic_cast<LookupTableProcessor*>(getProcessor())->getTable(0)));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new ToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a look up table to calculate the modulation value."));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (invertedButton = new ToggleButton ("new toggle button"));
    invertedButton->setTooltip (TRANS("Invert the range."));
    invertedButton->setButtonText (TRANS("Inverted"));
    invertedButton->addListener (this);
    invertedButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (smoothingSlider = new HiSlider ("Smoothing"));
    smoothingSlider->setTooltip (TRANS("Smoothing Value"));
    smoothingSlider->setRange (0, 2000, 0);
    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    smoothingSlider->addListener (this);


    //[UserPreSize]

	pm = static_cast<PitchwheelModulator*>(this->getProcessor());

    ProcessorHelpers::connectTableEditor(*midiTable, getProcessor());

	smoothingSlider->setup(getProcessor(), PitchwheelModulator::SmoothTime, "Smoothing");
	smoothingSlider->setMode(HiSlider::Mode::Time, 0, 1000.0, 100.0);




	getProcessor()->getMainController()->skin(*useTableButton);
	getProcessor()->getMainController()->skin(*invertedButton);

	tableUsed = pm->getAttribute(PitchwheelModulator::UseTable) == 1.0f;

    //[/UserPreSize]

    setSize (800, 230);


    //[Constructor] You can add your own custom stuff here..

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

	h = getHeight();

    //[/Constructor]
}

PitchWheelEditorBody::~PitchWheelEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    midiTable = nullptr;
    useTableButton = nullptr;
    invertedButton = nullptr;
    smoothingSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PitchWheelEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]

 
    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PitchWheelEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (((getWidth() / 2) - ((getWidth() - 112) / 2)) + (getWidth() - 112) - -7 - 264, 6, 264, 40);
    midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 89, getWidth() - 112, 120);
    useTableButton->setBounds (((getWidth() / 2) - ((getWidth() - 112) / 2)) + 0, 42, 128, 32);
    invertedButton->setBounds (getWidth() - -792 - 128, 85, 128, 32);
    smoothingSlider->setBounds ((((getWidth() / 2) - ((getWidth() - 112) / 2)) + 0) + 128 - -32, 29, 128, 48);
    //[UserResized] Add your own custom resize handling here..

	if(!tableUsed) midiTable->setTopLeftPosition(0, getHeight());

    //[/UserResized]
}

void PitchWheelEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();

		getProcessor()->setAttribute(PitchwheelModulator::UseTable, tableUsed ? 1.0f : 0.0f, dontSendNotification);

		refreshBodySize();
        //[/UserButtonCode_useTableButton]
    }
    else if (buttonThatWasClicked == invertedButton)
    {
        //[UserButtonCode_invertedButton] -- add your button handler code here..
		getProcessor()->setAttribute(PitchwheelModulator::Inverted, (float)buttonThatWasClicked->getToggleState(), dontSendNotification);
        //[/UserButtonCode_invertedButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void PitchWheelEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == smoothingSlider)
    {
        //[UserSliderCode_smoothingSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothingSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PitchWheelEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="230">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="-7Rr 6 264 40" posRelativeX="e2252e55bedecdc5"
         textCol="52ffffff" edTextCol="ff000000" edBkgCol="0" labelText="pitch wheel"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 89 112M 120" class="TableEditor"
                    params="dynamic_cast&lt;LookupTableProcessor*&gt;(getProcessor())-&gt;getTable(0)"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="0 42 128 32" posRelativeX="e2252e55bedecdc5"
                tooltip="Use a look up table to calculate the modulation value."
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="92a9b7af889d9ac3" memberName="invertedButton"
                virtualName="" explicitFocusOrder="0" pos="-792Rr 85 128 32"
                posRelativeX="5e7b9c6b5331d044" tooltip="Invert the range." txtcol="ffffffff"
                buttonText="Inverted" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
  <SLIDER name="Smoothing" id="9df027c7b43b6807" memberName="smoothingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-32R 29 128 48"
          posRelativeX="e77edc03c117de85" tooltip="Smoothing Value" min="0"
          max="2000" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
