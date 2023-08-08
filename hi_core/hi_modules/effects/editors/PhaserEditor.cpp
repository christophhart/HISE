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

#include "PhaserEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PhaserEditor::PhaserEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (freq1Slider = new HiSlider ("Speed"));
    freq1Slider->setRange (0, 1, 0.01);
    freq1Slider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freq1Slider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freq1Slider->addListener (this);

    addAndMakeVisible (freq2Slider = new HiSlider ("Range"));
    freq2Slider->setRange (0, 1, 0.01);
    freq2Slider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freq2Slider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freq2Slider->addListener (this);

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
    
    freq1Slider->setup(getProcessor(), PhaseFX::Attributes::Frequency1, "Frequency1");
    freq1Slider->setMode(HiSlider::Mode::Frequency);
    freq2Slider->setup(getProcessor(), PhaseFX::Attributes::Frequency2, "Frequency2");
    freq2Slider->setMode(HiSlider::Mode::Frequency);
    feedBackSlider->setup(getProcessor(), PhaseFX::Attributes::Feedback, "Feedback");
    feedBackSlider->setMode(HiSlider::Mode::NormalizedPercentage);
    wetSlider->setup(getProcessor(), PhaseFX::Attributes::Mix, "Mix");
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

    freq1Slider = nullptr;
    freq2Slider = nullptr;
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

    freq1Slider->setBounds ((getWidth() / 2) + -165 - 128, 16, 128, 48);
    freq2Slider->setBounds ((getWidth() / 2) + -14 - 128, 16, 128, 48);
    feedBackSlider->setBounds ((getWidth() / 2) + 130 - 128, 16, 128, 48);
    wetSlider->setBounds ((getWidth() / 2) + 274 - 128, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PhaserEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == freq1Slider)
    {
        //[UserSliderCode_speedSlider] -- add your slider handling code here..
        //[/UserSliderCode_speedSlider]
    }
    else if (sliderThatWasMoved == freq2Slider)
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
                 parentClasses="public Component" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="phaser"
          fontname="Oxygen" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Speed" id="89cc5b4c20e221e" memberName="freq1Slider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-165Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Range" id="13e15d71142cf03b" memberName="freq2Slider" virtualName="HiSlider"
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
} // namespace hise

//[/EndFile]
