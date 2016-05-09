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

#include "AhdsrEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

AhdsrGraph::AhdsrGraph(Processor *p):
	processor(p)
{}

void AhdsrGraph::paint(Graphics &g)
{
	const float attack = processor->getAttribute(AhdsrEnvelope::Attack);
	const float attackLevel = processor->getAttribute(AhdsrEnvelope::AttackLevel);
	const float hold = processor->getAttribute(AhdsrEnvelope::Hold);
	const float decay = processor->getAttribute(AhdsrEnvelope::Decay);
	const float sustain = processor->getAttribute(AhdsrEnvelope::Sustain);
	const float release = processor->getAttribute(AhdsrEnvelope::Release);

	float aln = pow((1.0f - (attackLevel + 100.0f) / 100.0f), 0.6f);
	const float sn =  pow((1.0f - (sustain     + 100.0f) / 100.0f), 0.6f);

	aln = sn < aln ? sn : aln;


	const float width = (float) getWidth();
	const float height = (float) getHeight();

	const float an = pow((attack / 20000.0f), 0.2f) * (0.2f * width );
	const float hn = pow((hold / 20000.0f), 0.2f) * (0.2f * width );
	const float dn = pow((decay / 20000.0f), 0.2f) * (0.2f * width );
	const float rn = pow((release / 20000.0f), 0.2f) * (0.2f * width );

	float x = 0.0f;
	float lastX = x;

	Path envelopePath;

	envelopePath.startNewSubPath(0.0f, (float)getHeight());

	// Attack Curve

	lastX = x;
	x += an;

	envelopePath.quadraticTo((lastX + x) / 2, aln * height, x, aln * height);

	x += hn;

	envelopePath.lineTo(x, aln * height);

	lastX = x;
	x += dn;

	envelopePath.quadraticTo(lastX, sn * height, x, sn * height);

	x = 0.8f * width;

	envelopePath.lineTo(x, sn*height);

	lastX = x;
	x += rn;

	envelopePath.quadraticTo(lastX, height, x, height);
	envelopePath.closeSubPath();

	KnobLookAndFeel::fillPathHiStyle(g, envelopePath, getWidth(), getHeight());

	g.setColour(Colours::lightgrey.withAlpha(0.3f));
	g.strokePath (envelopePath, PathStrokeType(1.0f));

	g.setColour(Colours::lightgrey.withAlpha(0.1f));
	g.drawRect(getLocalBounds(), 1);

    g.setGradientFill (ColourGradient (Colour (0x77999999),
                                       384.0f, 144.0f,
                                       Colour (0x11777777),
                                       408.0f, 520.0f,
                                       false));

	g.setGradientFill (ColourGradient (Colour (0x95c6c6c6),
                                    0.0f, 0.0f,
                                    Colour (0x976c6c6c),
                                    0.0f, static_cast<float> (proportionOfHeight (1.0000f)),
                                    false));

	//g.fillPath(envelopePath);


}



//[/MiscUserDefs]

//==============================================================================
AhdsrEnvelopeEditor::AhdsrEnvelopeEditor (BetterProcessorEditor *p)
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

    addAndMakeVisible (ahdsrGraph = new AhdsrGraph (getProcessor()));
    ahdsrGraph->setName ("new component");


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


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AhdsrEnvelopeEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..

	KnobLookAndFeel::drawHiBackground(g, ahdsrGraph->getX() - 16, ahdsrGraph->getY() - 8, ahdsrGraph->getWidth() + 32, ahdsrGraph->getHeight() + 16, nullptr, false);

    //[/UserPaint]
}

void AhdsrEnvelopeEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (getWidth() - 52 - 290, 21, 290, 40);
    attackSlider->setBounds (56, 112, proportionOfWidth (0.1506f), 48);
    releaseSlider->setBounds (getWidth() - 56 - proportionOfWidth (0.1506f), 112, proportionOfWidth (0.1506f), 48);
    attackLevelSlider->setBounds (getWidth() - 56 - proportionOfWidth (0.1506f), 59, proportionOfWidth (0.1506f), 48);
    holdSlider->setBounds (proportionOfWidth (0.2447f), 112, proportionOfWidth (0.1506f), 48);
    decaySlider->setBounds (proportionOfWidth (0.2447f) + proportionOfWidth (0.1506f) - -25, 112, proportionOfWidth (0.1506f), 48);
    sustainSlider->setBounds ((proportionOfWidth (0.2447f) + proportionOfWidth (0.1506f) - -25) + proportionOfWidth (0.1506f) - -25, 112, proportionOfWidth (0.1506f), 48);
    ahdsrGraph->setBounds (88, 32, proportionOfWidth (0.6224f), 64);
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

    //[UsersliderValueChanged_Post]

	ahdsrGraph->repaint();

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
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="BetterProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="850" initialHeight="170">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="52Rr 21 290 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="ahdsr" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
  <SLIDER name="Attack" id="9ef32c38be6d2f66" memberName="attackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="56 112 15.059% 48"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Release" id="b3d59ac44c48ffc2" memberName="releaseSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="56Rr 112 15.059% 48"
          thumbcol="80666666" textboxtext="ffffffff" min="1" max="20000"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Attack Level" id="d72131224c938370" memberName="attackLevelSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="56Rr 59 15.059% 48"
          tooltip="The attack peak level." bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="-100" max="0" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Hold" id="557420bb82cec3a9" memberName="holdSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="24.471% 112 15.059% 48" bkgcol="0"
          thumbcol="80666666" textboxtext="ffffffff" min="0" max="20000"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Decay" id="4baa7ade743ecf2b" memberName="decaySlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-25R 112 15.059% 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Sustain" id="a564ecb868858bb9" memberName="sustainSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-25R 112 15.059% 48"
          posRelativeX="4baa7ade743ecf2b" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="-100" max="0" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="new component" id="9694b8a8ec6e9c20" memberName="ahdsrGraph"
                    virtualName="" explicitFocusOrder="0" pos="88 32 62.235% 64"
                    class="AhdsrGraph" params="getProcessor()"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
