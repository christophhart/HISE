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

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MacroControlModulatorEditorBody::MacroControlModulatorEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("macro controller")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (valueTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<MacroModulator*>(getProcessor())->getTable(0)));
    valueTable->setName ("new component");

    addAndMakeVisible (useTableButton = new ToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a look up table to calculate the modulation value."));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (smoothingSlider = new HiSlider ("Smoothing"));
    smoothingSlider->setTooltip (TRANS("Smoothing Value"));
    smoothingSlider->setRange (0, 2000, 0);
    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    smoothingSlider->addListener (this);

    addAndMakeVisible (macroSelector = new ComboBox ("new combo box"));
    macroSelector->setEditableText (false);
    macroSelector->setJustificationType (Justification::centredLeft);
    macroSelector->setTextWhenNothingSelected (String());
    macroSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    macroSelector->addItem (TRANS("Not connected"), 1);

	for (int i = 0; i < HISE_NUM_MACROS; i++)
		macroSelector->addItem("Macro " + String(i + 1), i + 1);

    macroSelector->addListener (this);


    //[UserPreSize]
	tableUsed = getProcessor()->getAttribute(MacroModulator::UseTable) == 1.0f;

	getProcessor()->getMainController()->skin(*macroSelector);
	getProcessor()->getMainController()->skin(*useTableButton);

	smoothingSlider->setup(getProcessor(), MacroModulator::SmoothTime, "Smoothing");
    smoothingSlider->setMode(HiSlider::Time, 0.0, 1000.0, 100.0);

    ProcessorHelpers::connectTableEditor(*valueTable, getProcessor());
    
    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));
    
    //[/UserPreSize]

    setSize (800, 210);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    //[/Constructor]
}

MacroControlModulatorEditorBody::~MacroControlModulatorEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    valueTable = nullptr;
    useTableButton = nullptr;
    smoothingSlider = nullptr;
    macroSelector = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MacroControlModulatorEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..

	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MacroControlModulatorEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (getWidth() - 267, 2, 218, 40);
    valueTable->setBounds ((getWidth() / 2) + 2 - ((getWidth() - 112) / 2), 73, getWidth() - 112, 120);
    useTableButton->setBounds (((getWidth() / 2) + 2 - ((getWidth() - 112) / 2)) + 134, 21, 128, 32);
    smoothingSlider->setBounds ((getWidth() / 2) - (128 / 2), 13, 128, 48);
    macroSelector->setBounds (((getWidth() / 2) + 2 - ((getWidth() - 112) / 2)) + 0, 21, 120, 28);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MacroControlModulatorEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();

		getProcessor()->setAttribute(MacroModulator::UseTable, tableUsed, dontSendNotification);

		refreshBodySize();
        //[/UserButtonCode_useTableButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void MacroControlModulatorEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
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

void MacroControlModulatorEditorBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == macroSelector)
    {
        //[UserComboBoxCode_macroSelector] -- add your combo box handling code here..

		const int id =  macroSelector->getSelectedId() - 2;

		getProcessor()->setAttribute(MacroModulator::MacroIndex, (float)id, dontSendNotification);
        //[/UserComboBoxCode_macroSelector]
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

<JUCER_COMPONENT documentType="Component" className="MacroControlModulatorEditorBody"
                 componentName="" parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="210">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 4 84M 8M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="267R 2 218 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="macro controller"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="valueTable"
                    virtualName="" explicitFocusOrder="0" pos="2Cc 73 112M 120" class="TableEditor"
                    params="static_cast&lt;MacroModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="134 21 128 32" posRelativeX="e2252e55bedecdc5"
                tooltip="Use a look up table to calculate the modulation value."
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="Smoothing" id="9df027c7b43b6807" memberName="smoothingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 13 128 48"
          tooltip="Smoothing Value" min="0" max="2000" int="0" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="1"/>
  <COMBOBOX name="new combo box" id="462a3679f705065f" memberName="macroSelector"
            virtualName="" explicitFocusOrder="0" pos="0 21 120 28" posRelativeX="e2252e55bedecdc5"
            editable="0" layout="33" items="Not connected&#10;Macro 1&#10;Macro 2&#10;Macro 3&#10;Macro 4&#10;Macro 5&#10;Macro 6&#10;Macro 7&#10;Macro 8"
            textWhenNonSelected="" textWhenNoItems="(no choices)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
