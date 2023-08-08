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

#include "SaturationEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SaturationEditor::SaturationEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (saturationSlider = new HiSlider ("Saturation"));
    saturationSlider->setRange (-24, 24, 0.1);
    saturationSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    saturationSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    saturationSlider->addListener (this);

    addAndMakeVisible (wetSlider = new HiSlider ("Wet"));
    wetSlider->setRange (-24, 24, 0.1);
    wetSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    wetSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    wetSlider->addListener (this);

    addAndMakeVisible (pregainSlider = new HiSlider ("Saturation"));
    pregainSlider->setRange (-24, 24, 0.1);
    pregainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    pregainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    pregainSlider->addListener (this);

    addAndMakeVisible (postGainSlider = new HiSlider ("Wet"));
    postGainSlider->setRange (-24, 24, 0.1);
    postGainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    postGainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    postGainSlider->addListener (this);


    //[UserPreSize]

	wetSlider->setup(getProcessor(), SaturatorEffect::WetAmount, "Wet Amount");
	saturationSlider->setup(getProcessor(), SaturatorEffect::Saturation, "Saturation");

	wetSlider->setMode(HiSlider::NormalizedPercentage);
	saturationSlider->setMode(HiSlider::NormalizedPercentage);

	pregainSlider->setup(getProcessor(), SaturatorEffect::PreGain, "Pre Gain");
	pregainSlider->setMode(HiSlider::Decibel, 0, 24.0, 12.0);
	postGainSlider->setup(getProcessor(), SaturatorEffect::PostGain, "Post Gain");
	postGainSlider->setMode(HiSlider::Decibel, -24.0, 0.0, -12.0);
    //[/UserPreSize]

    setSize (800, 80);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

SaturationEditor::~SaturationEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    saturationSlider = nullptr;
    wetSlider = nullptr;
    pregainSlider = nullptr;
    postGainSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SaturationEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("saturation"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SaturationEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    saturationSlider->setBounds ((getWidth() / 2) + -68 - 128, 18, 128, 48);
    wetSlider->setBounds ((getWidth() / 2) + -48, 18, 128, 48);
    pregainSlider->setBounds ((getWidth() / 2) + -212 - 128, 18, 128, 48);
    postGainSlider->setBounds ((getWidth() / 2) + 106, 18, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SaturationEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == saturationSlider)
    {
        //[UserSliderCode_saturationSlider] -- add your slider handling code here..
        //[/UserSliderCode_saturationSlider]
    }
    else if (sliderThatWasMoved == wetSlider)
    {
        //[UserSliderCode_wetSlider] -- add your slider handling code here..
        //[/UserSliderCode_wetSlider]
    }
    else if (sliderThatWasMoved == pregainSlider)
    {
        //[UserSliderCode_pregainSlider] -- add your slider handling code here..
        //[/UserSliderCode_pregainSlider]
    }
    else if (sliderThatWasMoved == postGainSlider)
    {
        //[UserSliderCode_postGainSlider] -- add your slider handling code here..
        //[/UserSliderCode_postGainSlider]
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

<JUCER_COMPONENT documentType="Component" className="SaturationEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="saturation"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Saturation" id="89cc5b4c20e221e" memberName="saturationSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-68Cr 18 128 48"
          posRelativeX="f930000f86c6c8b6" min="-24" max="24" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Wet" id="ad396a89472e7cfc" memberName="wetSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-48C 18 128 48" posRelativeX="f930000f86c6c8b6"
          min="-24" max="24" int="0.10000000000000000555" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Saturation" id="d36fabecc4e35a39" memberName="pregainSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-212Cr 18 128 48"
          posRelativeX="f930000f86c6c8b6" min="-24" max="24" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Wet" id="730b3bc71def5830" memberName="postGainSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="106C 18 128 48"
          posRelativeX="f930000f86c6c8b6" min="-24" max="24" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
