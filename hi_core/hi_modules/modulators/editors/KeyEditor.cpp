/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "KeyEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
KeyEditor::KeyEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (midiTable = new TableEditor());
    midiTable->setName ("new component");

    //[UserPreSize]
	
    ProcessorHelpers::connectTableEditor(*midiTable, getProcessor());

    //[/UserPreSize]

    setSize (900, 320);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

    //[/Constructor]
}

KeyEditor::~KeyEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    midiTable = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void KeyEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 96) / 2)), 6.0f, static_cast<float> (getWidth() - 96), static_cast<float> (getHeight() - 32), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 96) / 2)), 6.0f, static_cast<float> (getWidth() - 96), static_cast<float> (getHeight() - 32), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void KeyEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    midiTable->setBounds ((getWidth() / 2) + -1 - ((getWidth() - 164) / 2), 32, getWidth() - 164, 203);
    
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void KeyEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void KeyEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    

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

<JUCER_COMPONENT documentType="Component" className="KeyEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="320">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 96M 32M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="-1Cc 32 164M 203"
                    class="TableEditor" params="static_cast&lt;KeyModulator*&gt;(getProcessor())-&gt;getTable(KeyModulator::NumberMode)"/>
  <GENERICCOMPONENT name="new component" id="cbc090f4af268c91" memberName="discreteTableEditor"
                    virtualName="" explicitFocusOrder="0" pos="-25Cc 126 212M 109"
                    class="DiscreteTableEditor" params="dynamic_cast&lt;KeyModulator*&gt;(getProcessor())-&gt;getTable(KeyModulator::KeyMode)"/>
  <GENERICCOMPONENT name="new component" id="b6ea720ca9d80007" memberName="keyGraph"
                    virtualName="" explicitFocusOrder="0" pos="-25Cc 32 212M 80"
                    class="KeyGraph" params="dynamic_cast&lt;KeyModulator*&gt;(getProcessor())-&gt;getTable(KeyModulator::KeyMode)"/>
  <SLIDER name="new slider" id="20bfcab3982bdab1" memberName="selectionSlider"
          virtualName="" explicitFocusOrder="0" pos="83Rr 32 32 203" thumbcol="59ffffff"
          rotaryslideroutline="47000000" textboxoutline="88808080" min="0"
          max="1" int="0" style="LinearBar" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="keyMode"
                virtualName="" explicitFocusOrder="0" pos="-6 247 128 32" posRelativeX="cbc090f4af268c91"
                tooltip="Toggle between Keymode with discrete values for each note or a simple look up table over the entire MIDI range."
                txtcol="ffffffff" buttonText="Key Mode" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="e28c5469abd3d68f" memberName="controlButton"
                virtualName="" explicitFocusOrder="0" pos="-8R 247 128 32" posRelativeX="e77edc03c117de85"
                tooltip="Enables selection of ranges by playing notes / intervals on the keyboard."
                txtcol="ffffffff" buttonText="MidiSelect" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="scrollButton"
                virtualName="" explicitFocusOrder="0" pos="-8R 247 128 32" posRelativeX="e28c5469abd3d68f"
                tooltip="Enables scrolling to the selected range." txtcol="ffffffff"
                buttonText="Autoscroll" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
