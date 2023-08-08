/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "StereoEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
StereoEditor::StereoEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (widthSlider = new HiSlider ("Stereo Width"));
    widthSlider->setRange (0, 200, 1);
    widthSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    widthSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    widthSlider->addListener (this);

    addAndMakeVisible (panSlider = new HiSlider ("Pan"));
    panSlider->setRange (-100, 100, 1);
    panSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    panSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    panSlider->addListener (this);


    //[UserPreSize]

	widthSlider->setup(getProcessor(), StereoEffect::Width, "Stereo Width");
	widthSlider->setMode(HiSlider::Discrete, 0.0, 200.0, 100.0, 1.0);

	panSlider->setup(getProcessor(), StereoEffect::Pan, "Max Pan");
	panSlider->setMode(HiSlider::Pan);
	panSlider->setIsUsingModulatedRing(true);
	panSlider->setEnabled(false);
    //[/UserPreSize]

    setSize (800, 80);


    //[Constructor] You can add your own custom stuff here..

	START_TIMER();

	h = getHeight();
    //[/Constructor]
}

StereoEditor::~StereoEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    widthSlider = nullptr;
    panSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void StereoEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("stereo fx"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void StereoEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    widthSlider->setBounds ((getWidth() / 2), 17, 128, 48);
    panSlider->setBounds (((getWidth() / 2)) + -14 - 128, 17, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void StereoEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == widthSlider)
    {
        //[UserSliderCode_widthSlider] -- add your slider handling code here..
        //[/UserSliderCode_widthSlider]
    }
    else if (sliderThatWasMoved == panSlider)
    {
        //[UserSliderCode_panSlider] -- add your slider handling code here..
        //[/UserSliderCode_panSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="StereoEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="stereo fx"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Stereo Width" id="f930000f86c6c8b6" memberName="widthSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0C 17 128 48"
          min="0" max="200" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Pan" id="89cc5b4c20e221e" memberName="panSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-14r 17 128 48" posRelativeX="f930000f86c6c8b6"
          min="-100" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
