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

#include "MdaLimiterEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MdaLimiterEditor::MdaLimiterEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (attackSlider = new MdaSlider ("Attack"));
    attackSlider->setRange (0, 1, 0.01);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    attackSlider->addListener (this);

    addAndMakeVisible (outputSlider = new MdaSlider ("Gain"));
    outputSlider->setRange (0, 1, 0.01);
    outputSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    outputSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    outputSlider->addListener (this);

    addAndMakeVisible (treshSlider = new MdaSlider ("Treshhold"));
    treshSlider->setRange (0, 1, 0.01);
    treshSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    treshSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    treshSlider->addListener (this);

    addAndMakeVisible (releaseSlider = new MdaSlider ("Release"));
    releaseSlider->setRange (0, 1, 0.01);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    releaseSlider->addListener (this);

    addAndMakeVisible (limiterGraph = new LimiterGraph (dynamic_cast<MdaEffectWrapper*>(getProcessor())));
    limiterGraph->setName ("Limiter Graph");

    addAndMakeVisible (softKneeButton = new ToggleButton ("new toggle button"));
    softKneeButton->setButtonText (TRANS("Soft Knee"));
    softKneeButton->addListener (this);
    softKneeButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (inMeter = new VuMeter());
    inMeter->setName ("new component");

    addAndMakeVisible (outMeter = new VuMeter());
    outMeter->setName ("new component");

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("limiter")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	inMeter->setType(VuMeter::StereoVertical);
	outMeter->setType(VuMeter::StereoVertical);

	outMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	outMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	outMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));

	inMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	inMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	inMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));


	for(int i = 0; i < MdaLimiterEffect::numParameters; i++)
	{
		if(getSlider(i) != nullptr)
		{
			//p->getMainController()->skin(*getSlider(i));
			getSlider(i)->setupSlider(dynamic_cast<MdaEffectWrapper*>(getProcessor())->getEffect(), getProcessor(), i);
		}
		else getProcessor()->getMainController()->skin(*softKneeButton);
	}

#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

    //[/UserPreSize]

    setSize (900, 210);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

MdaLimiterEditor::~MdaLimiterEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    attackSlider = nullptr;
    outputSlider = nullptr;
    treshSlider = nullptr;
    releaseSlider = nullptr;
    limiterGraph = nullptr;
    softKneeButton = nullptr;
    inMeter = nullptr;
    outMeter = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void MdaLimiterEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - (600 / 2)), 18.0f, 600.0f, 180.0f, 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - (600 / 2)), 18.0f, 600.0f, 180.0f, 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void MdaLimiterEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    attackSlider->setBounds (((getWidth() / 2) - (200 / 2)) + 200 - -48, 85, 128, 48);
    outputSlider->setBounds (((getWidth() / 2) - (200 / 2)) + -48 - 128, 95, 128, 48);
    treshSlider->setBounds (((getWidth() / 2) - (200 / 2)) + -48 - 128, 39, 128, 48);
    releaseSlider->setBounds (((getWidth() / 2) - (200 / 2)) + 200 - -48, 139, 128, 48);
    limiterGraph->setBounds ((getWidth() / 2) - (200 / 2), 39, 200, 145);
    softKneeButton->setBounds (((getWidth() / 2) - (200 / 2)) + -48 - 128, 150, 128, 32);
    inMeter->setBounds (((getWidth() / 2) - (200 / 2)) + -8 - 32, 40, 32, 144);
    outMeter->setBounds (((getWidth() / 2) - (200 / 2)) + 200 - -8, 40, 32, 144);
    label->setBounds (getWidth() - 157 - 264, 22, 264, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void MdaLimiterEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
	for(int i = 0; i < MdaLimiterEffect::numParameters; i++)
	{
		if(sliderThatWasMoved == getSlider(i)) SET_ATTRIBUTE_FROM_SLIDER(i);
	}

	limiterGraph->refreshGraph();

    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == attackSlider)
    {
        //[UserSliderCode_attackSlider] -- add your slider handling code here..
        //[/UserSliderCode_attackSlider]
    }
    else if (sliderThatWasMoved == outputSlider)
    {
        //[UserSliderCode_outputSlider] -- add your slider handling code here..
        //[/UserSliderCode_outputSlider]
    }
    else if (sliderThatWasMoved == treshSlider)
    {
        //[UserSliderCode_treshSlider] -- add your slider handling code here..
        //[/UserSliderCode_treshSlider]
    }
    else if (sliderThatWasMoved == releaseSlider)
    {
        //[UserSliderCode_releaseSlider] -- add your slider handling code here..
        //[/UserSliderCode_releaseSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void MdaLimiterEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == softKneeButton)
    {
        //[UserButtonCode_softKneeButton] -- add your button handler code here..
		getProcessor()->setAttribute(MdaLimiterEffect::Knee, softKneeButton->getToggleState(), dontSendNotification);
		limiterGraph->refreshGraph();
        //[/UserButtonCode_softKneeButton]
    }

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

<JUCER_COMPONENT documentType="Component" className="MdaLimiterEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="210">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 18 600 180" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Attack" id="350c324d3e462faa" memberName="attackSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="-48R 85 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Gain" id="109abf6dc0fb35f3" memberName="outputSlider" virtualName="MdaSlider"
          explicitFocusOrder="0" pos="-48r 95 128 48" posRelativeX="410a230ddaa2f2e8"
          min="0" max="1" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Treshhold" id="89cc5b4c20e221e" memberName="treshSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="-48r 39 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Release" id="3e971c1f41cadff" memberName="releaseSlider"
          virtualName="MdaSlider" explicitFocusOrder="0" pos="-48R 139 128 48"
          posRelativeX="410a230ddaa2f2e8" min="0" max="1" int="0.01" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="Limiter Graph" id="410a230ddaa2f2e8" memberName="limiterGraph"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 39 200 145" class="LimiterGraph"
                    params="dynamic_cast&lt;MdaEffectWrapper*&gt;(getProcessor())"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="softKneeButton"
                virtualName="" explicitFocusOrder="0" pos="-48r 150 128 32" posRelativeX="410a230ddaa2f2e8"
                txtcol="ffffffff" buttonText="Soft Knee" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="520732f14dd5e616" memberName="inMeter"
                    virtualName="" explicitFocusOrder="0" pos="-8r 40 32 144" posRelativeX="410a230ddaa2f2e8"
                    class="VuMeter" params=""/>
  <GENERICCOMPONENT name="new component" id="4b74183968d581c6" memberName="outMeter"
                    virtualName="" explicitFocusOrder="0" pos="-8R 40 32 144" posRelativeX="410a230ddaa2f2e8"
                    class="VuMeter" params=""/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="157Rr 22 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="limiter" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise

//[/EndFile]
