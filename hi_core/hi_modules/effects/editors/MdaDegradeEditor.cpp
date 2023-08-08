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

#include "MdaDegradeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MdaDegradeEditor::MdaDegradeEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (quantSlider = new MdaSlider ("Bit Crusher"));
    quantSlider->setRange (0, 1, 0.01);
    quantSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    quantSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    quantSlider->addListener (this);

    addAndMakeVisible (outputSlider = new MdaSlider ("Gain"));
    outputSlider->setRange (0, 1, 0.01);
    outputSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    outputSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    outputSlider->addListener (this);

    addAndMakeVisible (rateSlider = new MdaSlider ("SampleRate"));
    rateSlider->setRange (0, 1, 0.01);
    rateSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rateSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rateSlider->addListener (this);

    addAndMakeVisible (postFilterSlider = new MdaSlider ("PostFilter"));
    postFilterSlider->setRange (0, 1, 0.01);
    postFilterSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    postFilterSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    postFilterSlider->addListener (this);

    addAndMakeVisible (nonLinSlider = new MdaSlider ("Harmonics"));
    nonLinSlider->setRange (0, 1, 0.01);
    nonLinSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    nonLinSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    nonLinSlider->addListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (900, 150);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();



	for(int i = 0; i < MdaDegradeEffect::numParameters; i++)
	{
		if(getSlider(i) != nullptr)
		{
			getProcessor()->getMainController()->skin(*getSlider(i));
			getSlider(i)->setupSlider(dynamic_cast<MdaEffectWrapper*>(getProcessor())->getEffect(), getProcessor(), i);
		}
	}

	rateSlider->setRange(0.5, 0.7);

    //[/Constructor]
}

MdaDegradeEditor::~MdaDegradeEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    quantSlider = nullptr;
    outputSlider = nullptr;
    rateSlider = nullptr;
    postFilterSlider = nullptr;
    nonLinSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MdaDegradeEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("degrader"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MdaDegradeEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    quantSlider->setBounds ((getWidth() / 2) - (128 / 2), 24, 128, 48);
    outputSlider->setBounds ((getWidth() / 2) + -80 - 128, 72, 128, 48);
    rateSlider->setBounds ((getWidth() / 2) - (128 / 2), 72, 128, 48);
    postFilterSlider->setBounds ((getWidth() / 2) + 86, 72, 128, 48);
    nonLinSlider->setBounds ((getWidth() / 2) + -80 - 128, 24, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MdaDegradeEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
	for(int i = 0; i < MdaDegradeEffect::numParameters; i++)
	{
		if(sliderThatWasMoved == getSlider(i))
		{
			SET_ATTRIBUTE_FROM_SLIDER(i);
		}
	}

    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == quantSlider)
    {
        //[UserSliderCode_quantSlider] -- add your slider handling code here..
        //[/UserSliderCode_quantSlider]
    }
    else if (sliderThatWasMoved == outputSlider)
    {
        //[UserSliderCode_outputSlider] -- add your slider handling code here..
        //[/UserSliderCode_outputSlider]
    }
    else if (sliderThatWasMoved == rateSlider)
    {
        //[UserSliderCode_rateSlider] -- add your slider handling code here..
        //[/UserSliderCode_rateSlider]
    }
    else if (sliderThatWasMoved == postFilterSlider)
    {
        //[UserSliderCode_postFilterSlider] -- add your slider handling code here..
        //[/UserSliderCode_postFilterSlider]
    }
    else if (sliderThatWasMoved == nonLinSlider)
    {
        //[UserSliderCode_nonLinSlider] -- add your slider handling code here..
        //[/UserSliderCode_nonLinSlider]
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

<JUCER_COMPONENT documentType="Component" className="MdaDegradeEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="150">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="degrader"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Bit Crusher" id="350c324d3e462faa" memberName="quantSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="0Cc 24 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Gain" id="109abf6dc0fb35f3" memberName="outputSlider" virtualName="MdaSlider"
          explicitFocusOrder="0" pos="-80Cr 72 128 48" posRelativeX="410a230ddaa2f2e8"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="SampleRate" id="3e971c1f41cadff" memberName="rateSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="0Cc 72 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="PostFilter" id="23bb468e95071a96" memberName="postFilterSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="86C 72 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Harmonics" id="a32e5583e03352ed" memberName="nonLinSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="-80Cr 24 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise

//[/EndFile]
