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
//[/Headers]

#include "PluginParameterEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PluginParameterEditorBody::PluginParameterEditorBody (BetterProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (valueSlider = new HiSlider ("Value"));
    valueSlider->setRange (0, 1, 0);
    valueSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    valueSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    valueSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    valueSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    valueSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("PluginParameter")));
    label->setFont (Font ("Impact", 47.30f, Font::plain));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x3f000000));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	valueSlider->setup(getProcessor(), PluginParameterModulator::Value, "Value");
	valueSlider->setMode(HiSlider::NormalizedPercentage);

    //[/UserPreSize]

    setSize (800, 70);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PluginParameterEditorBody::~PluginParameterEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    valueSlider = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PluginParameterEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 3.0f, static_cast<float> (getWidth() - 84), 56.0f, 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 3.0f, static_cast<float> (getWidth() - 84), 56.0f, 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PluginParameterEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    valueSlider->setBounds ((getWidth() / 2) - (128 / 2), 9, 128, 48);
    label->setBounds (getWidth() - 359, 11, 312, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PluginParameterEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == valueSlider)
    {
        //[UserSliderCode_valueSlider] -- add your slider handling code here..

		PluginParameterModulator *pm = static_cast<PluginParameterModulator*>(getProcessor());

		int slotIndex = pm->getSlotIndex();

		if(slotIndex != -1)
		{
			float value = (float)sliderThatWasMoved->getValue() / 100.0f;
			getProcessor()->getMainController()->setPluginParameter(pm->getSlotIndex(), value);
		}

        //[/UserSliderCode_valueSlider]
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

<JUCER_COMPONENT documentType="Component" className="PluginParameterEditorBody"
                 componentName="" parentClasses="public ProcessorEditorBody" constructorParams="BetterProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="70">
  <BACKGROUND backgroundColour="ae00bc">
    <ROUNDRECT pos="0Cc 3 84M 56" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Value" id="9ef32c38be6d2f66" memberName="valueSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="0Cc 9 128 48" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="1" int="0" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="359R 11 312 40" textCol="3f000000"
         edTextCol="ff000000" edBkgCol="0" labelText="PluginParameter"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Impact" fontsize="47.299999999999997" bold="0" italic="0"
         justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
