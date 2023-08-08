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

#include "ReverbEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ReverbEditor::ReverbEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (wetSlider = new HiSlider ("Dry/Wet"));
    wetSlider->setRange (0, 1, 0.01);
    wetSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    wetSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    wetSlider->addListener (this);

    addAndMakeVisible (roomSlider = new HiSlider ("RoomSize"));
    roomSlider->setRange (0, 1, 0.01);
    roomSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    roomSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    roomSlider->addListener (this);

    addAndMakeVisible (dampingSlider = new HiSlider ("Damping"));
    dampingSlider->setRange (0, 1, 0.01);
    dampingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    dampingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    dampingSlider->addListener (this);

    addAndMakeVisible (widthSlider = new HiSlider ("Width"));
    widthSlider->setRange (0, 1, 0.01);
    widthSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    widthSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    widthSlider->addListener (this);


    //[UserPreSize]

	wetSlider->setup(getProcessor(), SimpleReverbEffect::WetLevel, "Wet Level");
	wetSlider->setMode(HiSlider::NormalizedPercentage);

	roomSlider->setup(getProcessor(), SimpleReverbEffect::RoomSize, "Room Size");
	roomSlider->setMode(HiSlider::NormalizedPercentage);

	dampingSlider->setup(getProcessor(), SimpleReverbEffect::Damping, "Damping");
	dampingSlider->setMode(HiSlider::NormalizedPercentage);

	widthSlider->setup(getProcessor(), SimpleReverbEffect::Width, "Stereo Width");
	widthSlider->setMode(HiSlider::NormalizedPercentage);


    //[/UserPreSize]

    setSize (900, 80);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

ReverbEditor::~ReverbEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    wetSlider = nullptr;
    roomSlider = nullptr;
    dampingSlider = nullptr;
    widthSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ReverbEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("reverb"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ReverbEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    wetSlider->setBounds ((((getWidth() / 2) - 128) + 408 - 128) + -296 - 128, 16, 128, 48);
    roomSlider->setBounds (((getWidth() / 2) - 128) + 408 - 128, 16, 128, 48);
    dampingSlider->setBounds ((getWidth() / 2) - 128, 16, 128, 48);
    widthSlider->setBounds (((getWidth() / 2) - 128) + 206 - (128 / 2), 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ReverbEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == wetSlider)
    {
        //[UserSliderCode_wetSlider] -- add your slider handling code here..

		getProcessor()->setAttribute(SimpleReverbEffect::DryLevel, 1.0f - (float)sliderThatWasMoved->getValue(), dontSendNotification);
        //[/UserSliderCode_wetSlider]
    }
    else if (sliderThatWasMoved == roomSlider)
    {
        //[UserSliderCode_roomSlider] -- add your slider handling code here..


        //[/UserSliderCode_roomSlider]
    }
    else if (sliderThatWasMoved == dampingSlider)
    {
        //[UserSliderCode_dampingSlider] -- add your slider handling code here..

        //[/UserSliderCode_dampingSlider]
    }
    else if (sliderThatWasMoved == widthSlider)
    {
        //[UserSliderCode_widthSlider] -- add your slider handling code here..

        //[/UserSliderCode_widthSlider]
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

<JUCER_COMPONENT documentType="Component" className="ReverbEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="reverb"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Dry/Wet" id="89cc5b4c20e221e" memberName="wetSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-296r 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="RoomSize" id="109abf6dc0fb35f3" memberName="roomSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="408r 16 128 48"
          posRelativeX="350c324d3e462faa" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Damping" id="350c324d3e462faa" memberName="dampingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cr 16 128 48"
          posRelativeX="f930000f86c6c8b6" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Width" id="3e971c1f41cadff" memberName="widthSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="206c 16 128 48" posRelativeX="350c324d3e462faa"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise

//[/EndFile]
