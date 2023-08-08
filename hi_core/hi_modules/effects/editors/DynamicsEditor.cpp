/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...

namespace hise { using namespace juce;

//[/Headers]

#include "DynamicsEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
DynamicsEditor::DynamicsEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (gateEnabled = new HiToggleButton ("new toggle button"));
    gateEnabled->setButtonText (TRANS("Gate Enabled"));
    gateEnabled->addListener (this);
    gateEnabled->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (gateMeter = new VuMeter());
    gateMeter->setName ("new component");

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("dynamics")));
    label->setFont (Font ("Arial", 24.00f, Font::plain).withTypefaceStyle ("Bold"));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x70ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (gateThreshold = new HiSlider ("Gate Threshold"));
    gateThreshold->setRange (0, 1, 0.01);
    gateThreshold->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gateThreshold->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gateThreshold->addListener (this);

    addAndMakeVisible (gateAttack = new HiSlider ("Gate Attack"));
    gateAttack->setRange (0, 1, 0.01);
    gateAttack->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gateAttack->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gateAttack->addListener (this);

    addAndMakeVisible (gateRelease = new HiSlider ("Gate Release"));
    gateRelease->setRange (0, 1, 0.01);
    gateRelease->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gateRelease->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gateRelease->addListener (this);

    addAndMakeVisible (compEnabled = new HiToggleButton ("new toggle button"));
    compEnabled->setButtonText (TRANS("Comp Enabled"));
    compEnabled->addListener (this);
    compEnabled->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (compMeter = new VuMeter());
    compMeter->setName ("new component");

    addAndMakeVisible (compThreshold = new HiSlider ("Comp Threshold"));
    compThreshold->setRange (0, 1, 0.01);
    compThreshold->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    compThreshold->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    compThreshold->addListener (this);

    addAndMakeVisible (compAttack = new HiSlider ("Comp Attack"));
    compAttack->setRange (0, 1, 0.01);
    compAttack->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    compAttack->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    compAttack->addListener (this);

    addAndMakeVisible (compRelease = new HiSlider ("compRelease"));
    compRelease->setRange (0, 1, 0.01);
    compRelease->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    compRelease->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    compRelease->addListener (this);

    addAndMakeVisible (compRatio = new HiSlider ("Comp Ratio"));
    compRatio->setRange (0, 1, 0.01);
    compRatio->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    compRatio->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    compRatio->addListener (this);

    addAndMakeVisible (limiterEnabled = new HiToggleButton ("new toggle button"));
    limiterEnabled->setButtonText (TRANS("Limiter Enabled"));
    limiterEnabled->addListener (this);
    limiterEnabled->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (limiterMeter = new VuMeter());
    limiterMeter->setName ("new component");

    addAndMakeVisible (limiterThreshold = new HiSlider ("Limiter Threshold"));
    limiterThreshold->setRange (0, 1, 0.01);
    limiterThreshold->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    limiterThreshold->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    limiterThreshold->addListener (this);

    addAndMakeVisible (limiterAttack = new HiSlider ("Limiter Attack"));
    limiterAttack->setRange (0, 1, 0.01);
    limiterAttack->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    limiterAttack->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    limiterAttack->addListener (this);

    addAndMakeVisible (limiterRelease = new HiSlider ("Limiter Release"));
    limiterRelease->setRange (0, 1, 0.01);
    limiterRelease->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    limiterRelease->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    limiterRelease->addListener (this);

    addAndMakeVisible (compMakeup = new HiToggleButton ("new toggle button"));
    compMakeup->setButtonText (TRANS("Comp Makeup"));
    compMakeup->addListener (this);
    compMakeup->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (limiterMakeup = new HiToggleButton ("new toggle button"));
    limiterMakeup->setButtonText (TRANS("Limiter Makeup"));
    limiterMakeup->addListener (this);
    limiterMakeup->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	gateAttack->setup(getProcessor(), DynamicsEffect::Parameters::GateAttack, "Attack");
	gateRelease->setup(getProcessor(), DynamicsEffect::Parameters::GateRelease, "Release");
	gateThreshold->setup(getProcessor(), DynamicsEffect::Parameters::GateThreshold, "Threshold");
	gateEnabled->setup(getProcessor(), DynamicsEffect::Parameters::GateEnabled, "Gate On");
	gateAttack->setMode(HiSlider::Mode::Time, 0.0, 100.0, 10.0, 0.01);
	gateRelease->setMode(HiSlider::Mode::Time, 0.0, 300.0, 10.0, 0.01);
	gateThreshold->setMode(HiSlider::Mode::Decibel, -100, 0.0, -40.0, 0.1);

	limiterAttack->setup(getProcessor(), DynamicsEffect::Parameters::LimiterAttack, "Attack");
	limiterRelease->setup(getProcessor(), DynamicsEffect::Parameters::LimiterRelease, "Release");
	limiterThreshold->setup(getProcessor(), DynamicsEffect::Parameters::LimiterThreshold, "Threshold");
	limiterEnabled->setup(getProcessor(), DynamicsEffect::Parameters::LimiterEnabled, "Gate Enabled");
	limiterAttack->setMode(HiSlider::Mode::Time, 0.0, 100.0, 10.0, 0.01);
	limiterRelease->setMode(HiSlider::Mode::Time, 0.0, 300.0, 10.0, 0.01);
	limiterThreshold->setMode(HiSlider::Mode::Decibel, -100, 0.0, -40.0, 0.1);
	limiterMakeup->setup(getProcessor(), DynamicsEffect::Parameters::LimiterMakeup, "Limiter Makeup");

	compAttack->setup(getProcessor(), DynamicsEffect::Parameters::CompressorAttack, "Attack");
	compRelease->setup(getProcessor(), DynamicsEffect::Parameters::CompressorRelease, "Release");
	compThreshold->setup(getProcessor(), DynamicsEffect::Parameters::CompressorThreshold, "Threshold");
	compEnabled->setup(getProcessor(), DynamicsEffect::Parameters::CompressorEnabled, "Compressor On");
	compRatio->setup(getProcessor(), DynamicsEffect::Parameters::CompressorRatio, "Ratio");
	compAttack->setMode(HiSlider::Mode::Time, 0.0, 100.0, 10.0, 0.01);
	compRelease->setMode(HiSlider::Mode::Time, 0.0, 300.0, 10.0, 0.01);
	compThreshold->setMode(HiSlider::Mode::Decibel, -100, 0.0, -40.0, 0.1);
	compRatio->setMode(HiSlider::Mode::Linear, 1.0, 32.0, 4.0, 0.1);
	compMakeup->setup(getProcessor(), DynamicsEffect::Parameters::CompressorMakeup, "Comp Makeup");

	gateMeter->setType(VuMeter::MonoVertical);
	gateMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	gateMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	gateMeter->setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));
	compMeter->setType(VuMeter::MonoVertical);
	compMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	compMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	compMeter->setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));
	limiterMeter->setType(VuMeter::MonoVertical);
	limiterMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	limiterMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	limiterMeter->setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));

	label->setFont(GLOBAL_BOLD_FONT().withHeight(24.0f));

	limiterMeter->setPeak(0.0f, 0.0f);
	gateMeter->setPeak(0.0f, 0.0f);
	compMeter->setPeak(0.0f, 0.0f);

	START_TIMER();

    //[/UserPreSize]

    setSize (800, 340);


    //[Constructor] You can add your own custom stuff here..



	h = getHeight();
    //[/Constructor]
}

DynamicsEditor::~DynamicsEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    gateEnabled = nullptr;
    gateMeter = nullptr;
    label = nullptr;
    gateThreshold = nullptr;
    gateAttack = nullptr;
    gateRelease = nullptr;
    compEnabled = nullptr;
    compMeter = nullptr;
    compThreshold = nullptr;
    compAttack = nullptr;
    compRelease = nullptr;
    compRatio = nullptr;
    limiterEnabled = nullptr;
    limiterMeter = nullptr;
    limiterThreshold = nullptr;
    limiterAttack = nullptr;
    limiterRelease = nullptr;
    compMakeup = nullptr;
    limiterMakeup = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void DynamicsEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    {
        float x = static_cast<float> ((getWidth() / 2) - (700 / 2)), y = 12.0f, width = 700.0f, height = static_cast<float> (getHeight() - 24);
        Colour fillColour = Colour (0x30000000);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRoundedRectangle (x, y, width, height, 6.000f);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void DynamicsEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    gateEnabled->setBounds ((getWidth() / 2) + -208 - (128 / 2), 64, 128, 32);
    gateMeter->setBounds (((getWidth() / 2) + -208 - (128 / 2)) + 128 - -10, 64 + 50, 16, 132);
    label->setBounds ((getWidth() / 2) + 80, 16, 264, 40);
    gateThreshold->setBounds (((getWidth() / 2) + -208 - (128 / 2)) + 128 / 2 - (128 / 2), 64 + 40, 128, 48);
    gateAttack->setBounds ((((getWidth() / 2) + -208 - (128 / 2)) + 128 / 2 - (128 / 2)) + 0, 64 + 90, 128, 48);
    gateRelease->setBounds ((((getWidth() / 2) + -208 - (128 / 2)) + 128 / 2 - (128 / 2)) + 0, 64 + 140, 128, 48);
    compEnabled->setBounds ((getWidth() / 2) - (128 / 2), 40, 128, 32);
    compMeter->setBounds (((getWidth() / 2) - (128 / 2)) + 128 - -10, 40 + 50, 16, 182);
    compThreshold->setBounds (((getWidth() / 2) - (128 / 2)) + 128 / 2 - (128 / 2), 40 + 40, 128, 48);
    compAttack->setBounds (((getWidth() / 2) - (128 / 2)) + 128 / 2 - (128 / 2), 40 + 90, 128, 48);
    compRelease->setBounds ((getWidth() / 2) - (128 / 2), 40 + 140, 128, 48);
    compRatio->setBounds ((getWidth() / 2) - (128 / 2), 40 + 190, 128, 48);
    limiterEnabled->setBounds ((getWidth() / 2) + 208 - (128 / 2), 64, 128, 32);
    limiterMeter->setBounds (((getWidth() / 2) + 208 - (128 / 2)) + 128 - -10, 64 + 50, 16, 132);
    limiterThreshold->setBounds (((getWidth() / 2) + 208 - (128 / 2)) + 128 / 2 - (128 / 2), 64 + 40, 128, 48);
    limiterAttack->setBounds (((getWidth() / 2) + 208 - (128 / 2)) + 128 / 2 - (128 / 2), 64 + 90, 128, 48);
    limiterRelease->setBounds (((getWidth() / 2) + 208 - (128 / 2)) + 128 / 2 - (128 / 2), 64 + 140, 128, 48);
    compMakeup->setBounds ((getWidth() / 2) - (128 / 2), 288, 128, 32);
    limiterMakeup->setBounds ((getWidth() / 2) + 208 - (128 / 2), 288, 128, 32);
    //[UserResized] Add your own custom resize handling here..

	compMeter->setTransform(AffineTransform::rotation(float_Pi, (float)compMeter->getBounds().getCentreX(), (float)compMeter->getBounds().getCentreY()));
	limiterMeter->setTransform(AffineTransform::rotation(float_Pi, (float)limiterMeter->getBounds().getCentreX(), (float)limiterMeter->getBounds().getCentreY()));
	gateMeter->setTransform(AffineTransform::rotation(float_Pi, (float)gateMeter->getBounds().getCentreX(), (float)limiterMeter->getBounds().getCentreY()));
    //[/UserResized]
}

void DynamicsEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == gateEnabled)
    {
        //[UserButtonCode_gateEnabled] -- add your button handler code here..
        //[/UserButtonCode_gateEnabled]
    }
    else if (buttonThatWasClicked == compEnabled)
    {
        //[UserButtonCode_compEnabled] -- add your button handler code here..
        //[/UserButtonCode_compEnabled]
    }
    else if (buttonThatWasClicked == limiterEnabled)
    {
        //[UserButtonCode_limiterEnabled] -- add your button handler code here..
        //[/UserButtonCode_limiterEnabled]
    }
    else if (buttonThatWasClicked == compMakeup)
    {
        //[UserButtonCode_compMakeup] -- add your button handler code here..
        //[/UserButtonCode_compMakeup]
    }
    else if (buttonThatWasClicked == limiterMakeup)
    {
        //[UserButtonCode_limiterMakeup] -- add your button handler code here..
        //[/UserButtonCode_limiterMakeup]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void DynamicsEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == gateThreshold)
    {
        //[UserSliderCode_gateThreshold] -- add your slider handling code here..
        //[/UserSliderCode_gateThreshold]
    }
    else if (sliderThatWasMoved == gateAttack)
    {
        //[UserSliderCode_gateAttack] -- add your slider handling code here..
        //[/UserSliderCode_gateAttack]
    }
    else if (sliderThatWasMoved == gateRelease)
    {
        //[UserSliderCode_gateRelease] -- add your slider handling code here..
        //[/UserSliderCode_gateRelease]
    }
    else if (sliderThatWasMoved == compThreshold)
    {
        //[UserSliderCode_compThreshold] -- add your slider handling code here..
        //[/UserSliderCode_compThreshold]
    }
    else if (sliderThatWasMoved == compAttack)
    {
        //[UserSliderCode_compAttack] -- add your slider handling code here..
        //[/UserSliderCode_compAttack]
    }
    else if (sliderThatWasMoved == compRelease)
    {
        //[UserSliderCode_compRelease] -- add your slider handling code here..
        //[/UserSliderCode_compRelease]
    }
    else if (sliderThatWasMoved == compRatio)
    {
        //[UserSliderCode_compRatio] -- add your slider handling code here..
        //[/UserSliderCode_compRatio]
    }
    else if (sliderThatWasMoved == limiterThreshold)
    {
        //[UserSliderCode_limiterThreshold] -- add your slider handling code here..
        //[/UserSliderCode_limiterThreshold]
    }
    else if (sliderThatWasMoved == limiterAttack)
    {
        //[UserSliderCode_limiterAttack] -- add your slider handling code here..
        //[/UserSliderCode_limiterAttack]
    }
    else if (sliderThatWasMoved == limiterRelease)
    {
        //[UserSliderCode_limiterRelease] -- add your slider handling code here..
        //[/UserSliderCode_limiterRelease]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void DynamicsEditor::updateGui()
{
	gateEnabled->updateValue();

	gateThreshold->updateValue();
	gateAttack->updateValue();
	gateRelease->updateValue();

	compEnabled->updateValue();
	compThreshold->updateValue();
	compAttack->updateValue();
	compRelease->updateValue();
	compRatio->updateValue();
	compMakeup->updateValue();

	limiterEnabled->updateValue();
	limiterThreshold->updateValue();
	limiterAttack->updateValue();
	limiterRelease->updateValue();
	limiterMakeup->updateValue();
}



void DynamicsEditor::timerCallback()
{
	if (getProcessor()->getAttribute(DynamicsEffect::Parameters::GateEnabled))
	{
		float gp = 1.0f - getProcessor()->getAttribute(DynamicsEffect::Parameters::GateReduction);
		gateMeter->setPeak(gp, gp);
	}
	else
		gateMeter->setPeak(0.0f, 0.0f);

	if (getProcessor()->getAttribute(DynamicsEffect::Parameters::CompressorEnabled))
	{
		float cp = 1.0f - getProcessor()->getAttribute(DynamicsEffect::Parameters::CompressorReduction);
		compMeter->setPeak(cp, cp);
	}
	else
		compMeter->setPeak(0.0f, 0.0f);

	if (getProcessor()->getAttribute(DynamicsEffect::Parameters::LimiterEnabled))
	{
		float lp = 1.0f - getProcessor()->getAttribute(DynamicsEffect::Parameters::LimiterReduction);
		limiterMeter->setPeak(lp, lp);
	}
	else
		limiterMeter->setPeak(0.0f, 0.0f);

}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="DynamicsEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="340">
  <BACKGROUND backgroundColour="0">
    <ROUNDRECT pos="0Cc 12 700 24M" cornerSize="6" fill="solid: 30000000" hasStroke="0"/>
  </BACKGROUND>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="gateEnabled"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="-208Cc 64 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Gate Enabled"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="520732f14dd5e616" memberName="gateMeter"
                    virtualName="" explicitFocusOrder="0" pos="-10R 50 16 132" posRelativeX="e6345feaa3cb5bea"
                    posRelativeY="e6345feaa3cb5bea" class="VuMeter" params=""/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="80C 16 264 40" textCol="70ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="dynamics" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" kerning="0" bold="1" italic="0" justification="34"
         typefaceStyle="Bold"/>
  <SLIDER name="Gate Threshold" id="a66c2b8be13d8dd9" memberName="gateThreshold"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 40 128 48"
          posRelativeX="e6345feaa3cb5bea" posRelativeY="e6345feaa3cb5bea"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Gate Attack" id="ce952002c368c05a" memberName="gateAttack"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0 90 128 48"
          posRelativeX="a66c2b8be13d8dd9" posRelativeY="e6345feaa3cb5bea"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Gate Release" id="803f8b17de23043a" memberName="gateRelease"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0 140 128 48"
          posRelativeX="a66c2b8be13d8dd9" posRelativeY="e6345feaa3cb5bea"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <TOGGLEBUTTON name="new toggle button" id="b483624576310fc1" memberName="compEnabled"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="0Cc 40 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Comp Enabled"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="5253665d707918a9" memberName="compMeter"
                    virtualName="" explicitFocusOrder="0" pos="-10R 50 16 182" posRelativeX="b483624576310fc1"
                    posRelativeY="b483624576310fc1" class="VuMeter" params=""/>
  <SLIDER name="Comp Threshold" id="39b28ad3e255290e" memberName="compThreshold"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 40 128 48"
          posRelativeX="b483624576310fc1" posRelativeY="b483624576310fc1"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Comp Attack" id="55e5675505ddd74c" memberName="compAttack"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 90 128 48"
          posRelativeX="b483624576310fc1" posRelativeY="b483624576310fc1"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="compRelease" id="eb52ba269d7e672a" memberName="compRelease"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 140 128 48"
          posRelativeY="b483624576310fc1" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Comp Ratio" id="540dfae8d722726f" memberName="compRatio"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 190 128 48"
          posRelativeY="b483624576310fc1" min="0" max="1" int="0.010000000000000000208"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <TOGGLEBUTTON name="new toggle button" id="aeb418f9b0aaa817" memberName="limiterEnabled"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="208Cc 64 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Limiter Enabled"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="ddcb723eb137e866" memberName="limiterMeter"
                    virtualName="" explicitFocusOrder="0" pos="-10R 50 16 132" posRelativeX="aeb418f9b0aaa817"
                    posRelativeY="aeb418f9b0aaa817" class="VuMeter" params=""/>
  <SLIDER name="Limiter Threshold" id="7de1f3cae119a001" memberName="limiterThreshold"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 40 128 48"
          posRelativeX="aeb418f9b0aaa817" posRelativeY="aeb418f9b0aaa817"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Limiter Attack" id="669e095edd0d79bf" memberName="limiterAttack"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 90 128 48"
          posRelativeX="aeb418f9b0aaa817" posRelativeY="aeb418f9b0aaa817"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <SLIDER name="Limiter Release" id="e81b21eb195ccab5" memberName="limiterRelease"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 140 128 48"
          posRelativeX="aeb418f9b0aaa817" posRelativeY="aeb418f9b0aaa817"
          min="0" max="1" int="0.010000000000000000208" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <TOGGLEBUTTON name="new toggle button" id="6549ebd2506430d8" memberName="compMakeup"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="0Cc 288 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Comp Makeup"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="98e23aec76b01e5b" memberName="limiterMakeup"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="208Cc 288 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Limiter Makeup"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
