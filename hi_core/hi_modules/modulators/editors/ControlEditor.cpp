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

#include "hi_core/hi_modules/modulators/mods/ControlModulator.h"

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ControlEditorBody::ControlEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("midi controller")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (midiTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<ControlModulator*>(getProcessor())->getTable(0)));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new HiToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a look up table to calculate the modulation value."));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (invertedButton = new HiToggleButton ("new toggle button"));
    invertedButton->setTooltip (TRANS("Invert the range."));
    invertedButton->setButtonText (TRANS("Inverted"));
    invertedButton->addListener (this);
    invertedButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (controllerNumberSlider = new HiSlider ("CC Nr."));
    controllerNumberSlider->setTooltip (TRANS("The CC number"));
    controllerNumberSlider->setRange (1, 128, 1);
    controllerNumberSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    controllerNumberSlider->setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
    controllerNumberSlider->addListener (this);

    addAndMakeVisible (smoothingSlider = new HiSlider ("Smoothing"));
    smoothingSlider->setTooltip (TRANS("Smoothing Value"));
    smoothingSlider->setRange (0, 2000, 0);
    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    smoothingSlider->addListener (this);

    addAndMakeVisible (learnButton = new ToggleButton ("new toggle button"));
    learnButton->setButtonText (TRANS("MIDI Learn"));
    learnButton->addListener (this);
    learnButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (defaultSlider = new HiSlider ("Default"));
    defaultSlider->setTooltip (TRANS("Smoothing Value"));
    defaultSlider->setRange (0, 127, 0);
    defaultSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    defaultSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    defaultSlider->addListener (this);


    //[UserPreSize]

	cm = static_cast<ControlModulator*>(this->getProcessor());

	smoothingSlider->setup(getProcessor(), ControlModulator::SmoothTime, "Smoothing");
	smoothingSlider->setMode(HiSlider::Mode::Time, 0, 1000.0, 100.0);

	controllerNumberSlider->setup(getProcessor(), ControlModulator::ControllerNumber, "CC Number");
	controllerNumberSlider->setMode(HiSlider::Discrete, 0, 129.0, 64.0);

	defaultSlider->setup(getProcessor(), ControlModulator::DefaultValue, "Default");
	defaultSlider->setMode(HiSlider::Discrete, 0.0, 127.0);
	defaultSlider->setRange(0.0, 127.0, 1.0);

    useTableButton->setup(getProcessor(), ControlModulator::Parameters::UseTable, "UseTable");
    invertedButton->setup(getProcessor(), ControlModulator::Parameters::Inverted, "Inverted");

	getProcessor()->getMainController()->skin(*learnButton);

    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));

	tableUsed = cm->getAttribute(ControlModulator::UseTable) == 1.0f;

    ProcessorHelpers::connectTableEditor(*midiTable, cm);

    //[/UserPreSize]

    setSize (800, 245);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    //[/Constructor]
}

ControlEditorBody::~ControlEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    midiTable = nullptr;
    useTableButton = nullptr;
    invertedButton = nullptr;
    controllerNumberSlider = nullptr;
    smoothingSlider = nullptr;
    learnButton = nullptr;
    defaultSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ControlEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	
	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[/UserPrePaint]



    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ControlEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (getWidth() - 312, 9, 264, 40);
    midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 107, getWidth() - 112, 121);
    useTableButton->setBounds ((58 + 128 - -16) + 0, 65, 128, 32);
    invertedButton->setBounds ((58 + 128 - -174) + 128 - 128, 65, 128, 32);
    controllerNumberSlider->setBounds (58, 16, 128, 48);
    smoothingSlider->setBounds (58 + 128 - -16, 16, 128, 48);
    learnButton->setBounds (58 + 0, 65, 128, 32);
    defaultSlider->setBounds (58 + 128 - -174, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..

	if(!tableUsed) midiTable->setTopLeftPosition(0, getHeight());

    //[/UserResized]
}

void ControlEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..
		tableUsed = useTableButton->getToggleState();
        
		refreshBodySize();
        //[/UserButtonCode_useTableButton]
    }
    else if (buttonThatWasClicked == invertedButton)
    {
        //[UserButtonCode_invertedButton] -- add your button handler code here..
		
        //[/UserButtonCode_invertedButton]
    }
    else if (buttonThatWasClicked == learnButton)
    {
        //[UserButtonCode_learnButton] -- add your button handler code here..
		dynamic_cast<ControlModulator*>(getProcessor())->enableLearnMode();
        //[/UserButtonCode_learnButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void ControlEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == controllerNumberSlider)
    {
        //[UserSliderCode_controllerNumberSlider] -- add your slider handling code here..

        //[/UserSliderCode_controllerNumberSlider]
    }
    else if (sliderThatWasMoved == smoothingSlider)
    {
        //[UserSliderCode_smoothingSlider] -- add your slider handling code here..

        //[/UserSliderCode_smoothingSlider]
    }
    else if (sliderThatWasMoved == defaultSlider)
    {
        //[UserSliderCode_defaultSlider] -- add your slider handling code here..
        //[/UserSliderCode_defaultSlider]
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

<JUCER_COMPONENT documentType="Component" className="ControlEditorBody" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="245">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="312R 9 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="midi controller"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 107 112M 121"
                    class="TableEditor" params="static_cast&lt;ControlModulator*&gt;(getProcessor())-&gt;getTable()"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="0 65 128 32" posRelativeX="9df027c7b43b6807"
                tooltip="Use a look up table to calculate the modulation value."
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="92a9b7af889d9ac3" memberName="invertedButton"
                virtualName="" explicitFocusOrder="0" pos="0Rr 65 128 32" posRelativeX="5e7b9c6b5331d044"
                tooltip="Invert the range." txtcol="ffffffff" buttonText="Inverted"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="CC Nr." id="6462473d097e8027" memberName="controllerNumberSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="58 16 128 48"
          tooltip="The CC number" min="1" max="128" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="30"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Smoothing" id="9df027c7b43b6807" memberName="smoothingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-16R 16 128 48"
          posRelativeX="6462473d097e8027" tooltip="Smoothing Value" min="0"
          max="2000" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="new toggle button" id="7755a3174f755061" memberName="learnButton"
                virtualName="" explicitFocusOrder="0" pos="0 65 128 32" posRelativeX="6462473d097e8027"
                txtcol="ffffffff" buttonText="MIDI Learn" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Default" id="5e7b9c6b5331d044" memberName="defaultSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-174R 16 128 48"
          posRelativeX="6462473d097e8027" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
