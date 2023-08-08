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

#include "RandomEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
RandomEditorBody::RandomEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (midiTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<RandomModulator*>(getProcessor())->getTable(0)));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new ToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a lookup table to massage the probability."));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("randomizer")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	rm = static_cast<RandomModulator*>(getProcessor());

	getProcessor()->getMainController()->skin(*useTableButton);

	tableUsed = rm->getAttribute(RandomModulator::UseTable) == 1.0f;

    ProcessorHelpers::connectTableEditor(*midiTable, rm);

    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));


    //[/UserPreSize]

    setSize (800, 200);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    //[/Constructor]
}

RandomEditorBody::~RandomEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    midiTable = nullptr;
    useTableButton = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void RandomEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	
	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void RandomEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 61, getWidth() - 112, 120);
    useTableButton->setBounds ((getWidth() / 2) - (128 / 2), 16, 128, 32);
    label->setBounds (getWidth() - 315, 2, 264, 40);
    //[UserResized] Add your own custom resize handling here..
	if(!tableUsed) midiTable->setTopLeftPosition(0, getHeight());
    //[/UserResized]
}

void RandomEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();

		getProcessor()->setAttribute(RandomModulator::UseTable, tableUsed ? 1.0f : 0.0f, dontSendNotification);
		refreshBodySize();


        //[/UserButtonCode_useTableButton]
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

<JUCER_COMPONENT documentType="Component" className="RandomEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="200">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 4 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 61 112M 120" class="TableEditor"
                    params="static_cast&lt;RandomModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="0Cc 16 128 32" tooltip="Use a lookup table to massage the probability."
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="315R 2 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="randomizer" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
