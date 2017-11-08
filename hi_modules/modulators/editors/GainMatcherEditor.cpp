/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "GainMatcherEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GainMatcherEditor::GainMatcherEditor (ProcessorEditor *pe)
    : ProcessorEditorBody(pe)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (midiTable = new TableEditor (dynamic_cast<GainMatcherModulator*>(getProcessor())->getTable(0)));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new HiToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a table to calculate the value"));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (collectorSelector = new ComboBox ("new combo box"));
    collectorSelector->setEditableText (false);
    collectorSelector->setJustificationType (Justification::centredLeft);
    collectorSelector->setTextWhenNothingSelected (TRANS("Select GainMatcher"));
    collectorSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    collectorSelector->addListener (this);


    //[UserPreSize]

	getProcessor()->getMainController()->skin(*useTableButton);

	tableUsed = getProcessor()->getAttribute(GlobalModulator::UseTable) == 1.0f;

	midiTable->connectToLookupTableProcessor(getProcessor());

	useTableButton->setup(getProcessor(), GlobalModulator::UseTable, "Use Table");

	getProcessor()->getMainController()->skin(*collectorSelector);

	setItemEntry();

    
    //[/UserPreSize]

    setSize (900, 200);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

    //[/Constructor]
}

GainMatcherEditor::~GainMatcherEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    midiTable = nullptr;
    useTableButton = nullptr;
    collectorSelector = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void GainMatcherEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 3.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 6), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 3.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 6), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void GainMatcherEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    midiTable->setBounds ((getWidth() / 2) + 2 - ((getWidth() - 116) / 2), 64, getWidth() - 116, 120);
    useTableButton->setBounds (60, 17, 128, 32);
    collectorSelector->setBounds (getWidth() - 56 - (getWidth() - 268), 17, getWidth() - 268, 32);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void GainMatcherEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();
		getProcessor()->setAttribute(GainMatcherModulator::UseTable, tableUsed ? 1.0f : 0.0f, dontSendNotification);

		refreshBodySize();
        //[/UserButtonCode_useTableButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void GainMatcherEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == collectorSelector)
    {
        //[UserComboBoxCode_collectorSelector] -- add your combo box handling code here..

		const String text = collectorSelector->getText();

		dynamic_cast<GainMatcherModulator*>(getProcessor())->setConnectedCollectorId(text);


        //[/UserComboBoxCode_collectorSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...


void GainMatcherEditor::setItemEntry()
{
	GainMatcherModulator *gm = dynamic_cast<GainMatcherModulator*>(getProcessor());

	StringArray itemList = gm->getListOfAllGainCollectors();

	String item = gm->getConnectedCollectorId();

	collectorSelector->clear();

	collectorSelector->addItemList(itemList, 1);

	const int index = itemList.indexOf(item);

	if (index != -1)
	{
		collectorSelector->setSelectedItemIndex(index, dontSendNotification);
	}
}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GainMatcherEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *pe"
                 variableInitialisers="ProcessorEditorBody(pe)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="200">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 3 84M 6M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="2Cc 64 116M 120" class="TableEditor"
                    params="dynamic_cast&lt;GainMatcherModulator*&gt;(getProcessor())-&gt;getTable(0)"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="60 17 128 32"
                tooltip="Use a table to calculate the value" txtcol="ffffffff"
                buttonText="UseTable" connectedEdges="0" needsCallback="1" radioGroupId="0"
                state="0"/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="collectorSelector"
            virtualName="ComboBox" explicitFocusOrder="0" pos="56Rr 17 268M 32"
            posRelativeX="3b242d8d6cab6cc3" editable="0" layout="33" items=""
            textWhenNonSelected="Select GainMatcher" textWhenNoItems="(no choices)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
