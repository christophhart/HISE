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

#include "AhdsrEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...




//[/MiscUserDefs]

//==============================================================================
AhdsrEnvelopeEditor::AhdsrEnvelopeEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("ahdsr")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (attackSlider = new HiSlider ("Attack"));
    attackSlider->setRange (1, 20000, 1);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    attackSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackSlider->addListener (this);
    attackSlider->setSkewFactor (0.3);

    addAndMakeVisible (releaseSlider = new HiSlider ("Release"));
    releaseSlider->setRange (1, 20000, 1);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    releaseSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    releaseSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    releaseSlider->addListener (this);
    releaseSlider->setSkewFactor (0.3);

    addAndMakeVisible (attackLevelSlider = new HiSlider ("Attack Level"));
    attackLevelSlider->setTooltip (TRANS("The attack peak level."));
    attackLevelSlider->setRange (-100, 0, 1);
    attackLevelSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackLevelSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackLevelSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    attackLevelSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackLevelSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackLevelSlider->addListener (this);

    addAndMakeVisible (holdSlider = new HiSlider ("Hold"));
    holdSlider->setRange (0, 20000, 1);
    holdSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    holdSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    holdSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    holdSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    holdSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    holdSlider->addListener (this);
    holdSlider->setSkewFactor (0.3);

    addAndMakeVisible (decaySlider = new HiSlider ("Decay"));
    decaySlider->setRange (1, 20000, 1);
    decaySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    decaySlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    decaySlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    decaySlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    decaySlider->setColour (Slider::textBoxTextColourId, Colours::white);
    decaySlider->addListener (this);
    decaySlider->setSkewFactor (0.3);

    addAndMakeVisible (sustainSlider = new HiSlider ("Sustain"));
    sustainSlider->setRange (-100, 0, 1);
    sustainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    sustainSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    sustainSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    sustainSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    sustainSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    sustainSlider->addListener (this);

	auto b = dynamic_cast<scriptnode::data::base*>(getProcessor());
	auto rb = dynamic_cast<SimpleRingBuffer*>(b->externalData.obj);

    addAndMakeVisible (ahdsrGraph = new AhdsrGraph());
	ahdsrGraph->setComplexDataUIBase(rb);

    ahdsrGraph->setName ("new component");

    addAndMakeVisible (attackCurveSlider = new HiSlider ("Decay"));
    attackCurveSlider->setRange (1, 20000, 1);
    attackCurveSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackCurveSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackCurveSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    attackCurveSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackCurveSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackCurveSlider->addListener (this);
    attackCurveSlider->setSkewFactor (0.3);

    addAndMakeVisible (decayCurveSlider = new HiSlider ("Sustain"));
    decayCurveSlider->setRange (-100, 0, 1);
    decayCurveSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    decayCurveSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    decayCurveSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    decayCurveSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    decayCurveSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    decayCurveSlider->addListener (this);


    //[UserPreSize]

	attackSlider->setup(getProcessor(), AhdsrEnvelope::Attack, "Attack Time");
	attackSlider->setMode(HiSlider::Time);

	attackLevelSlider->setup(getProcessor(), AhdsrEnvelope::AttackLevel, "Attack Level");
	attackLevelSlider->setMode(HiSlider::Decibel, -100.0, 0.0, -12.0);

	sustainSlider->setup(getProcessor(), AhdsrEnvelope::Sustain, "Sustain Level");
	sustainSlider->setMode(HiSlider::Decibel, -100.0, 0.0, -12.0);

	holdSlider->setup(getProcessor(), AhdsrEnvelope::Hold, "Hold Time");
	holdSlider->setMode(HiSlider::Time);

	decaySlider->setup(getProcessor(), AhdsrEnvelope::Decay, "Decay Time");
	decaySlider->setMode(HiSlider::Time);

	releaseSlider->setup(getProcessor(), AhdsrEnvelope::Release, "Release Time");
	releaseSlider->setMode(HiSlider::Time);

    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));

	attackCurveSlider->setup(getProcessor(), AhdsrEnvelope::AttackCurve, "Attack Curve");
	attackCurveSlider->setMode(HiSlider::NormalizedPercentage);

	decayCurveSlider->setup(getProcessor(), AhdsrEnvelope::DecayCurve, "Decay Curve");
	decayCurveSlider->setMode(HiSlider::NormalizedPercentage);



    //[/UserPreSize]

    setSize (850, 170);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

	startTimer(50);

	attackSlider->setIsUsingModulatedRing(true);
	attackLevelSlider->setIsUsingModulatedRing(true);
	decaySlider->setIsUsingModulatedRing(true);
	releaseSlider->setIsUsingModulatedRing(true);
	sustainSlider->setIsUsingModulatedRing(true);

	ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    //[/Constructor]
}

AhdsrEnvelopeEditor::~AhdsrEnvelopeEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
	stopTimer();

    //[/Destructor_pre]

    label = nullptr;
    attackSlider = nullptr;
    releaseSlider = nullptr;
    attackLevelSlider = nullptr;
    holdSlider = nullptr;
    decaySlider = nullptr;
    sustainSlider = nullptr;
    ahdsrGraph = nullptr;
    attackCurveSlider = nullptr;
    decayCurveSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AhdsrEnvelopeEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - (710 / 2)), 6.0f, 710.0f, static_cast<float> (getHeight() - 12), 3.000f);

    
    //[UserPaint] Add your own custom painting code here..

	//KnobLookAndFeel::drawHiBackground(g, ahdsrGraph->getX() - 16, ahdsrGraph->getY() - 8, ahdsrGraph->getWidth() + 32, ahdsrGraph->getHeight() + 16, nullptr, false);

    //[/UserPaint]
}

void AhdsrEnvelopeEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds ((getWidth() / 2) + 348 - 290, 8, 290, 40);
    attackSlider->setBounds ((getWidth() / 2) + -340, 112, 128, 48);
    releaseSlider->setBounds ((getWidth() / 2) + 341 - 128, 112, 128, 48);
    attackLevelSlider->setBounds ((getWidth() / 2) + 341 - 128, 42, 128, 48);
    holdSlider->setBounds ((getWidth() / 2) + -202, 112, 128, 48);
    decaySlider->setBounds (((getWidth() / 2) + -202) + 128 / 2 + 75, 112, 128, 48);
    sustainSlider->setBounds ((((getWidth() / 2) + -202) + 128 / 2 + 75) + 128 - -10, 112, 128, 48);
    ahdsrGraph->setBounds ((getWidth() / 2) + -73 - 264, 32, 264, 64);
    attackCurveSlider->setBounds (((getWidth() / 2) + -202) + 128 - -10, 42, 128, 48);
    decayCurveSlider->setBounds ((((getWidth() / 2) + -202) + 128 / 2 + 75) + 128 - -10, 42, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AhdsrEnvelopeEditor::sliderValueChanged (Slider* sliderThatWasMoved)
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
    else if (sliderThatWasMoved == attackLevelSlider)
    {
        //[UserSliderCode_attackLevelSlider] -- add your slider handling code here..



        //[/UserSliderCode_attackLevelSlider]
    }
    else if (sliderThatWasMoved == holdSlider)
    {
        //[UserSliderCode_holdSlider] -- add your slider handling code here..

        //[/UserSliderCode_holdSlider]
    }
    else if (sliderThatWasMoved == decaySlider)
    {
        //[UserSliderCode_decaySlider] -- add your slider handling code here..

        //[/UserSliderCode_decaySlider]
    }
    else if (sliderThatWasMoved == sustainSlider)
    {
        //[UserSliderCode_sustainSlider] -- add your slider handling code here..

        //[/UserSliderCode_sustainSlider]
    }
    else if (sliderThatWasMoved == attackCurveSlider)
    {
        //[UserSliderCode_attackCurveSlider] -- add your slider handling code here..
        //[/UserSliderCode_attackCurveSlider]
    }
    else if (sliderThatWasMoved == decayCurveSlider)
    {
        //[UserSliderCode_decayCurveSlider] -- add your slider handling code here..
        //[/UserSliderCode_decayCurveSlider]
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

<JUCER_COMPONENT documentType="Component" className="AhdsrEnvelopeEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="850" initialHeight="170">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 710 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="348Cr 8 290 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="ahdsr" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
  <SLIDER name="Attack" id="9ef32c38be6d2f66" memberName="attackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-340C 112 128 48"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"
          needsCallback="1"/>
  <SLIDER name="Release" id="b3d59ac44c48ffc2" memberName="releaseSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="341Cr 112 128 48"
          thumbcol="80666666" textboxtext="ffffffff" min="1" max="20000"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"
          needsCallback="1"/>
  <SLIDER name="Attack Level" id="d72131224c938370" memberName="attackLevelSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="341Cr 42 128 48"
          tooltip="The attack peak level." bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="-100" max="0" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Hold" id="557420bb82cec3a9" memberName="holdSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-202C 112 128 48" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="0" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999" needsCallback="1"/>
  <SLIDER name="Decay" id="4baa7ade743ecf2b" memberName="decaySlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="75C 112 128 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"
          needsCallback="1"/>
  <SLIDER name="Sustain" id="a564ecb868858bb9" memberName="sustainSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-10R 112 128 48"
          posRelativeX="4baa7ade743ecf2b" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="-100" max="0" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <GENERICCOMPONENT name="new component" id="9694b8a8ec6e9c20" memberName="ahdsrGraph"
                    virtualName="" explicitFocusOrder="0" pos="-73Cr 32 264 64" class="AhdsrGraph"
                    params="getProcessor()"/>
  <SLIDER name="Decay" id="a7c54198d4a84cc" memberName="attackCurveSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-10R 42 128 48"
          posRelativeX="557420bb82cec3a9" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999" needsCallback="1"/>
  <SLIDER name="Sustain" id="e6f50736c60dcaee" memberName="decayCurveSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-10R 42 128 48"
          posRelativeX="4baa7ade743ecf2b" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="-100" max="0" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
