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

#include "VelocityEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
VelocityEditorBody::VelocityEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (midiTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<VelocityModulator*>(getProcessor())->getTable(0)));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new ToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a table to calculate the value"));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (invertedButton = new ToggleButton ("new toggle button"));
    invertedButton->setTooltip (TRANS("Inverts the range (0..1) -> (1...0)"));
    invertedButton->setButtonText (TRANS("Inverted"));
    invertedButton->addListener (this);
    invertedButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("velocity")));
    
    addAndMakeVisible (decibelButton = new HiToggleButton ("new toggle button"));
    decibelButton->setTooltip (TRANS("Use a table to calculate the value"));
    decibelButton->setButtonText (TRANS("Decibel Mode"));
    decibelButton->addListener (this);
    decibelButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]



    
	vm = static_cast<VelocityModulator*>(getProcessor());

	getProcessor()->getMainController()->skin(*invertedButton);
	getProcessor()->getMainController()->skin(*useTableButton);

	tableUsed = vm->getAttribute(VelocityModulator::UseTable) == 1.0f;

    ProcessorHelpers::connectTableEditor(*midiTable, getProcessor());
    
    decibelButton->setup(getProcessor(), VelocityModulator::DecibelMode, "Decibel Mode");
    
    //[/UserPreSize]

    setSize (800, 190);




    //[Constructor] You can add your own custom stuff here..

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

	h = getHeight();

    //[/Constructor]
}

VelocityEditorBody::~VelocityEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    midiTable = nullptr;
    useTableButton = nullptr;
    invertedButton = nullptr;
    label = nullptr;
    decibelButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}



//==============================================================================
void VelocityEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    
	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]

    
    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void VelocityEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 55, getWidth() - 112, 120);
    useTableButton->setBounds ((getWidth() / 2) + -12 - 128, 12, 128, 32);
    invertedButton->setBounds ((getWidth() / 2) + 12, 12, 128, 32);
    label->setBounds (getWidth() - 315, 8, 264, 40);
    decibelButton->setBounds ((getWidth() / 2) + -160 - 128, 12, 128, 32);
    //[UserResized] Add your own custom resize handling here..

	if(!tableUsed) midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 84) / 2), getHeight(), getWidth() - 84, 184);


    //[/UserResized]
}

void VelocityEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]

    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();
		getProcessor()->setAttribute(VelocityModulator::UseTable, tableUsed ? 1.0f : 0.0f, dontSendNotification);

		refreshBodySize();
		//getEditor()->refreshSize();


        //[/UserButtonCode_useTableButton]
    }
    else if (buttonThatWasClicked == invertedButton)
    {
        //[UserButtonCode_invertedButton] -- add your button handler code here..
		getProcessor()->setAttribute(VelocityModulator::Inverted, (float)buttonThatWasClicked->getToggleState(), dontSendNotification);
        //[/UserButtonCode_invertedButton]
    }
    else if (buttonThatWasClicked == decibelButton)
    {
        //[UserButtonCode_decibelButton] -- add your button handler code here..
        //[/UserButtonCode_decibelButton]
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

<JUCER_COMPONENT documentType="Component" className="VelocityEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="0" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="190">
  <BACKGROUND backgroundColour="b2b2b2">
    <ROUNDRECT pos="0Cc 3 84M 6M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 55 112M 120" class="TableEditor"
                    params="static_cast&lt;VelocityModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="-12Cr 12 128 32" tooltip="Use a table to calculate the value"
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="92a9b7af889d9ac3" memberName="invertedButton"
                virtualName="" explicitFocusOrder="0" pos="12C 12 128 32" tooltip="Inverts the range (0..1) -&gt; (1...0)"
                txtcol="ffffffff" buttonText="Inverted" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="315R 8 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="velocity" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial Unicode MS"
         fontsize="24" bold="1" italic="0" justification="34"/>
  <TOGGLEBUTTON name="new toggle button" id="d21c61c589ffef85" memberName="decibelButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="-160Cr 12 128 32"
                tooltip="Use a table to calculate the value" txtcol="ffffffff"
                buttonText="Decibel Mode" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
