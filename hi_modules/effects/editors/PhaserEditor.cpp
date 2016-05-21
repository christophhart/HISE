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
//[/Headers]

#include "PhaserEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PhaserEditor::PhaserEditor (BetterProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (speedSlider = new HiSlider ("Speed"));
    speedSlider->setRange (0, 1, 0.01);
    speedSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    speedSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    speedSlider->addListener (this);

    addAndMakeVisible (rangeSlider = new HiSlider ("Range"));
    rangeSlider->setRange (0, 1, 0.01);
    rangeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rangeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rangeSlider->addListener (this);

    addAndMakeVisible (feedBackSlider = new HiSlider ("Feedback"));
    feedBackSlider->setRange (0, 1, 0.01);
    feedBackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    feedBackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    feedBackSlider->addListener (this);

    addAndMakeVisible (wetSlider = new HiSlider ("Wet"));
    wetSlider->setRange (0, 1, 0.1);
    wetSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    wetSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    wetSlider->addListener (this);


    //[UserPreSize]
    
    speedSlider->setup(getProcessor(), PhaserEffect::Attributes::Speed, "Speed");
    speedSlider->setMode(HiSlider::Mode::NormalizedPercentage);
    rangeSlider->setup(getProcessor(), PhaserEffect::Attributes::Range, "Range");
    rangeSlider->setMode(HiSlider::Mode::NormalizedPercentage);
    feedBackSlider->setup(getProcessor(), PhaserEffect::Attributes::Feedback, "Feedback");
    feedBackSlider->setMode(HiSlider::Mode::NormalizedPercentage);
    wetSlider->setup(getProcessor(), PhaserEffect::Attributes::Mix, "Mix");
    wetSlider->setMode(HiSlider::Mode::NormalizedPercentage);

    
    //[/UserPreSize]

    setSize (900, 80);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

PhaserEditor::~PhaserEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    speedSlider = nullptr;
    rangeSlider = nullptr;
    feedBackSlider = nullptr;
    wetSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PhaserEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Oxygen", 24.00f, Font::bold));
    g.drawText (TRANS("phaser"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PhaserEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    speedSlider->setBounds ((getWidth() / 2) + -165 - 128, 16, 128, 48);
    rangeSlider->setBounds ((getWidth() / 2) + -14 - 128, 16, 128, 48);
    feedBackSlider->setBounds ((getWidth() / 2) + 130 - 128, 16, 128, 48);
    wetSlider->setBounds ((getWidth() / 2) + 274 - 128, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PhaserEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == speedSlider)
    {
        //[UserSliderCode_speedSlider] -- add your slider handling code here..
        //[/UserSliderCode_speedSlider]
    }
    else if (sliderThatWasMoved == rangeSlider)
    {
        //[UserSliderCode_rangeSlider] -- add your slider handling code here..
        //[/UserSliderCode_rangeSlider]
    }
    else if (sliderThatWasMoved == feedBackSlider)
    {
        //[UserSliderCode_feedBackSlider] -- add your slider handling code here..
        //[/UserSliderCode_feedBackSlider]
    }
    else if (sliderThatWasMoved == wetSlider)
    {
        //[UserSliderCode_wetSlider] -- add your slider handling code here..
        //[/UserSliderCode_wetSlider]
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

<JUCER_COMPONENT documentType="Component" className="PhaserEditor" componentName=""
                 parentClasses="public Component" constructorParams="BetterProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="phaser"
          fontname="Oxygen" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Speed" id="89cc5b4c20e221e" memberName="speedSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-165Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Range" id="13e15d71142cf03b" memberName="rangeSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-14Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Feedback" id="2b5afac2b8aa9df6" memberName="feedBackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="130Cr 16 128 48"
          posRelativeX="109abf6dc0fb35f3" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Wet" id="7006b2021dbdcadd" memberName="wetSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="274Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.10000000000000000555" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
