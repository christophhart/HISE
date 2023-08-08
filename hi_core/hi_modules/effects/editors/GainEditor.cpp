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

#include "GainEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GainEditor::GainEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (widthSlider = new HiSlider ("Gain"));
    widthSlider->setRange (-100, 36, 1);
    widthSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    widthSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    widthSlider->addListener (this);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setRange (-100, 36, 1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);

    addAndMakeVisible (delaySlider = new HiSlider ("Gain"));
    delaySlider->setRange (-100, 36, 1);
    delaySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    delaySlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    delaySlider->addListener (this);

    addAndMakeVisible (balanceSlider = new HiSlider ("Balance"));
    balanceSlider->setRange (-100, 36, 1);
    balanceSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    balanceSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    balanceSlider->addListener (this);


    //[UserPreSize]
	gainSlider->setup(getProcessor(), GainEffect::Gain, "Gain");
	gainSlider->setMode(HiSlider::Decibel, -100.0, 36.0, 0.0);
    gainSlider->setIsUsingModulatedRing(true);

    delaySlider->setup(getProcessor(), GainEffect::Delay, "Delay");
    delaySlider->setMode(HiSlider::Time, 0, 500, 100);
    delaySlider->setIsUsingModulatedRing(true);

    widthSlider->setup(getProcessor(), GainEffect::Width, "Width");
	widthSlider->setMode(HiSlider::Discrete, 0.0, 200.0, 100.0);
    widthSlider->setIsUsingModulatedRing(true);

	balanceSlider->setup(getProcessor(), GainEffect::Balance, "Balance");
	balanceSlider->setMode(HiSlider::Pan);
	balanceSlider->setIsUsingModulatedRing(true);

    START_TIMER();

    //[/UserPreSize]

    setSize (800, 80);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

GainEditor::~GainEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    widthSlider = nullptr;
    gainSlider = nullptr;
    delaySlider = nullptr;
    balanceSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void GainEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("gain"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void GainEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    widthSlider->setBounds ((getWidth() / 2) + 40 - (128 / 2), 16, 128, 48);
    gainSlider->setBounds ((getWidth() / 2) + -280 - (128 / 2), 16, 128, 48);
    delaySlider->setBounds ((getWidth() / 2) + -120 - (128 / 2), 16, 128, 48);
    balanceSlider->setBounds ((getWidth() / 2) + 208 - (128 / 2), 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void GainEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == widthSlider)
    {
        //[UserSliderCode_widthSlider] -- add your slider handling code here..
        //[/UserSliderCode_widthSlider]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == delaySlider)
    {
        //[UserSliderCode_delaySlider] -- add your slider handling code here..
        //[/UserSliderCode_delaySlider]
    }
    else if (sliderThatWasMoved == balanceSlider)
    {
        //[UserSliderCode_balanceSlider] -- add your slider handling code here..
        //[/UserSliderCode_balanceSlider]
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

<JUCER_COMPONENT documentType="Component" className="GainEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="gain"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Gain" id="89cc5b4c20e221e" memberName="widthSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="40Cc 16 128 48" posRelativeX="f930000f86c6c8b6"
          min="-100" max="36" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Gain" id="4dc41660965e265c" memberName="gainSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-280Cc 16 128 48" posRelativeX="f930000f86c6c8b6"
          min="-100" max="36" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Gain" id="5cd4c50ac19c9c3c" memberName="delaySlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-120Cc 16 128 48" posRelativeX="f930000f86c6c8b6"
          min="-100" max="36" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Balance" id="53bf35a7c21709e9" memberName="balanceSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="208Cc 16 128 48"
          posRelativeX="f930000f86c6c8b6" min="-100" max="36" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
