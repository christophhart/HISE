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

#include "ChorusEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ChorusEditor::ChorusEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (rateSlider = new HiSlider ("Rate"));
    rateSlider->setRange (0, 1, 0.01);
    rateSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rateSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rateSlider->addListener (this);

    addAndMakeVisible (widthSlider = new HiSlider ("Width"));
    widthSlider->setRange (0, 1, 0.01);
    widthSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    widthSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    widthSlider->addListener (this);

    addAndMakeVisible (feedBackSlider = new HiSlider ("Feedback"));
    feedBackSlider->setRange (0, 1, 0.01);
    feedBackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    feedBackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    feedBackSlider->addListener (this);

    addAndMakeVisible (delaySlider = new HiSlider ("Delay"));
    delaySlider->setRange (0, 1, 0.1);
    delaySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    delaySlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    delaySlider->addListener (this);


    //[UserPreSize]

	rateSlider->setup(getProcessor(), ChorusEffect::Parameters::Rate, "Rate");
	rateSlider->setMode(HiSlider::NormalizedPercentage);
	widthSlider->setup(getProcessor(), ChorusEffect::Width, "Width");
	widthSlider->setMode(HiSlider::NormalizedPercentage);
	feedBackSlider->setup(getProcessor(), ChorusEffect::Feedback, "Feedback");
	feedBackSlider->setMode(HiSlider::NormalizedPercentage);
	delaySlider->setup(getProcessor(), ChorusEffect::Delay, "Delay");

    
    
    //[/UserPreSize]

    setSize (900, 80);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

ChorusEditor::~ChorusEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    rateSlider = nullptr;
    widthSlider = nullptr;
    feedBackSlider = nullptr;
    delaySlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ChorusEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    g.setColour (Colour (0xAAffffff));
    g.setFont (GLOBAL_BOLD_FONT().withHeight(22.0f));
    g.drawText (TRANS("chorus"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ChorusEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    rateSlider->setBounds ((getWidth() / 2) + -197 - 128, 16, 128, 48);
    widthSlider->setBounds ((getWidth() / 2) + -46 - 128, 16, 128, 48);
    feedBackSlider->setBounds ((getWidth() / 2) + 98 - 128, 16, 128, 48);
    delaySlider->setBounds ((getWidth() / 2) + 242 - 128, 16, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ChorusEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == rateSlider)
    {
        //[UserSliderCode_rateSlider] -- add your slider handling code here..
        //[/UserSliderCode_rateSlider]
    }
    else if (sliderThatWasMoved == widthSlider)
    {
        //[UserSliderCode_widthSlider] -- add your slider handling code here..
        //[/UserSliderCode_widthSlider]
    }
    else if (sliderThatWasMoved == feedBackSlider)
    {
        //[UserSliderCode_feedBackSlider] -- add your slider handling code here..
        //[/UserSliderCode_feedBackSlider]
    }
    else if (sliderThatWasMoved == delaySlider)
    {
        //[UserSliderCode_delaySlider] -- add your slider handling code here..
        //[/UserSliderCode_delaySlider]
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

<JUCER_COMPONENT documentType="Component" className="ChorusEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="80">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="chorus"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Rate" id="89cc5b4c20e221e" memberName="rateSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-197Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Width" id="13e15d71142cf03b" memberName="widthSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-46Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Feedback" id="2b5afac2b8aa9df6" memberName="feedBackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="98Cr 16 128 48"
          posRelativeX="109abf6dc0fb35f3" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Delay" id="7006b2021dbdcadd" memberName="delaySlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="242Cr 16 128 48" posRelativeX="109abf6dc0fb35f3"
          min="0" max="1" int="0.10000000000000000555" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
