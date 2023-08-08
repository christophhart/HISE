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

#define setMod(x, y) (getProcessor()->setAttribute(x, y, dontSendNotification) )

//[/Headers]

#include "SimpleEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SimpleEnvelopeEditorBody::SimpleEnvelopeEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (attackSlider = new HiSlider ("Attack"));
    attackSlider->setRange (0, 20000, 1);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    attackSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackSlider->addListener (this);
    attackSlider->setSkewFactor (0.3);

    addAndMakeVisible (releaseSlider = new HiSlider ("Release"));
    releaseSlider->setRange (3, 20000, 1);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    releaseSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    releaseSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    releaseSlider->addListener (this);
    releaseSlider->setSkewFactor (0.3);

    addAndMakeVisible (useLinearMode = new HiToggleButton ("new toggle button"));
    useLinearMode->setTooltip (TRANS("Use a look up table to calculate the modulation value."));
    useLinearMode->setButtonText (TRANS("Linear Mode"));
    useLinearMode->addListener (this);
    useLinearMode->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]



	attackSlider->setup(getProcessor(), SimpleEnvelope::Attack, "Attack Time");
	attackSlider->setMode(HiSlider::Time);

	releaseSlider->setup(getProcessor(), SimpleEnvelope::Release, "Release Time");
	releaseSlider->setMode(HiSlider::Time);

	useLinearMode->setup(getProcessor(), SimpleEnvelope::LinearMode, "Linear Mode");

    //[/UserPreSize]

    setSize (800, 80);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

	attackSlider->setIsUsingModulatedRing(true);

	

	sm = dynamic_cast<SimpleEnvelope*>(getProcessor());

	startTimer(30);

    //[/Constructor]
}

SimpleEnvelopeEditorBody::~SimpleEnvelopeEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    attackSlider = nullptr;
    releaseSlider = nullptr;
    useLinearMode = nullptr;


    //[Destructor]. You can add your own custom destruction code here..

    //[/Destructor]
}

//==============================================================================
void SimpleEnvelopeEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]



    //[UserPaint] Add your own custom painting code here..
   
	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    g.setColour (Colour (0xAAFFFFFF));
    
    g.setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));
    
    g.drawText (getProcessor()->getName().toLowerCase(),
                getWidth() - 53 - 500, 6, 500, 40,
                Justification::centredRight, true);
    
    //[/UserPaint]
}

void SimpleEnvelopeEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    attackSlider->setBounds (336 + 128 / 2 + -72 - 128, 16, 128, 48);
    releaseSlider->setBounds (336, 16, 128, 48);
    useLinearMode->setBounds (56, 24, 128, 32);
    //[UserResized] Add your own custom resize handling here..

    //[/UserResized]
}

void SimpleEnvelopeEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == attackSlider)
    {
        //[UserSliderCode_attackSlider] -- add your slider handling code here..

        //[/UserSliderCode_attackSlider]
    }
    else if (sliderThatWasMoved == releaseSlider)
    {
        //[UserSliderCode_releaseSlider] -- add your slider handling code here..

        //[/UserSliderCode_releaseSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void SimpleEnvelopeEditorBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useLinearMode)
    {
        //[UserButtonCode_useLinearMode] -- add your button handler code here..
        //[/UserButtonCode_useLinearMode]
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

<JUCER_COMPONENT documentType="Component" className="SimpleEnvelopeEditorBody"
                 componentName="" parentClasses="public ProcessorEditorBody, public Timer"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="80">
  <BACKGROUND backgroundColour="9a2424">
  </BACKGROUND>
  <SLIDER name="Attack" id="9ef32c38be6d2f66" memberName="attackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-72Cr 16 128 48"
          posRelativeX="b3d59ac44c48ffc2" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Release" id="b3d59ac44c48ffc2" memberName="releaseSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="336 16 128 48"
          posRelativeX="bd1d8d6ad6d04bdc" thumbcol="80666666" textboxtext="ffffffff"
          min="3" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useLinearMode"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="56 24 128 32"
                posRelativeX="9df027c7b43b6807" tooltip="Use a look up table to calculate the modulation value."
                txtcol="ffffffff" buttonText="Linear Mode" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
