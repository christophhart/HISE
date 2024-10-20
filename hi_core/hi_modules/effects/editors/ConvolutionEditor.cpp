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

#include "ConvolutionEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ConvolutionEditor::ConvolutionEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (drySlider = new HiSlider ("Dry Level"));
    drySlider->setRange (-100, 0, 0.1);
    drySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    drySlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    drySlider->addListener (this);

    addAndMakeVisible (wetSlider = new HiSlider ("Wet Level"));
    wetSlider->setRange (-100, 0, 0.1);
    wetSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    wetSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    wetSlider->addListener (this);

    addAndMakeVisible (dryMeter = new VuMeter());
    dryMeter->setName ("new component");

    addAndMakeVisible (wetMeter = new VuMeter());
    wetMeter->setName ("new component");

    addAndMakeVisible (impulseDisplay = new MultiChannelAudioBufferDisplay ());
    impulseDisplay->setName ("new component");

    addAndMakeVisible (resetButton = new HiToggleButton ("new toggle button"));
    resetButton->setButtonText (TRANS("Process Input"));
    resetButton->addListener (this);
    resetButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("convolution")));
    label->setFont (Font ("Arial", 26.00f, Font::plain).withTypefaceStyle ("Bold"));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (backgroundButton = new HiToggleButton ("new toggle button"));
    backgroundButton->setButtonText (TRANS("Multithread"));
    backgroundButton->addListener (this);
    backgroundButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (predelaySlider = new HiSlider ("Dry Level"));
    predelaySlider->setRange (-100, 0, 0.1);
    predelaySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    predelaySlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    predelaySlider->addListener (this);

    addAndMakeVisible (dampingSlider = new HiSlider ("Dry Level"));
    dampingSlider->setRange (-100, 0, 0.1);
    dampingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    dampingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    dampingSlider->addListener (this);

    addAndMakeVisible (hiCutSlider = new HiSlider ("Dry Level"));
    hiCutSlider->setRange (-100, 0, 0.1);
    hiCutSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    hiCutSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    hiCutSlider->addListener (this);


    //[UserPreSize]

	addAndMakeVisible(fadeoutDisplay = new FadeoutDisplay());

    label->setFont(GLOBAL_BOLD_FONT().withHeight(26.0f));

    drySlider->setMode(HiSlider::Decibel);
	drySlider->setup(getProcessor(), ConvolutionEffect::DryGain, "Dry Level");
	

    wetSlider->setMode(HiSlider::Decibel);
	wetSlider->setup(getProcessor(), ConvolutionEffect::WetGain, "Wet Level");
	

    dampingSlider->setMode(HiSlider::Decibel);
	dampingSlider->setup(getProcessor(), ConvolutionEffect::Damping, "Damping");
	

    predelaySlider->setMode(HiSlider::Time, 0.0, 200.0, 50.0, 0.1);
	predelaySlider->setup(getProcessor(), ConvolutionEffect::Predelay, "Predelay");
	

    hiCutSlider->setMode(HiSlider::Frequency);
	hiCutSlider->setup(getProcessor(), ConvolutionEffect::HiCut, "IR High Cut");
	

	dryMeter->setType(VuMeter::Type::StereoHorizontal);
	wetMeter->setType(VuMeter::Type::StereoHorizontal);

	dryMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	dryMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	dryMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));

	wetMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	wetMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	wetMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));

	resetButton->setup(getProcessor(), ConvolutionEffect::ProcessInput, "Process Input");

	backgroundButton->setup(getProcessor(), ConvolutionEffect::UseBackgroundThread, "Multithread");

	#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

	impulseDisplay->setAudioFile(&dynamic_cast<AudioSampleProcessor*>(getProcessor())->getBuffer());

	impulseDisplay->addAreaListener(this);

	impulseDisplay->setOpaque(false);
	impulseDisplay->setColour(MultiChannelAudioBufferDisplay::ColourIds::bgColour, Colour(0x11000000));

    ProcessorEditorLookAndFeel::setupEditorNameLabel(label);

    //[/UserPreSize]

    setSize (900, 300);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

ConvolutionEditor::~ConvolutionEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    drySlider = nullptr;
    wetSlider = nullptr;
    dryMeter = nullptr;
    wetMeter = nullptr;
    impulseDisplay = nullptr;
    resetButton = nullptr;
    label = nullptr;
    backgroundButton = nullptr;
    predelaySlider = nullptr;
    dampingSlider = nullptr;
    hiCutSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ConvolutionEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    {
        float x = static_cast<float> ((getWidth() / 2) - (600 / 2)), y = 8.0f, width = 600.0f, height = static_cast<float> (getHeight() - 16);
        Colour fillColour = Colour (0x23000000);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRoundedRectangle (x, y, width, height, 6.000f);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ConvolutionEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    drySlider->setBounds ((getWidth() / 2) + 56 - 136, 32, 136, 48);
    wetSlider->setBounds ((getWidth() / 2) + -118 - 136, 32, 136, 48);
    dryMeter->setBounds (((getWidth() / 2) + 56 - 136) + 1, 86, 136, 20);
    wetMeter->setBounds (((getWidth() / 2) + -118 - 136) + 1, 86, 136, 20);
    impulseDisplay->setBounds ((getWidth() / 2) + -282, 129, 368, 144);
    resetButton->setBounds ((getWidth() / 2) + 195 - (128 / 2), 215, 128, 32);
    label->setBounds ((getWidth() / 2) + 284 - 264, 15, 264, 40);
    backgroundButton->setBounds ((getWidth() / 2) + 195 - (128 / 2), 253, 128, 32);
    predelaySlider->setBounds ((getWidth() / 2) + 264 - 136, 50, 136, 48);
    dampingSlider->setBounds ((getWidth() / 2) + 265 - 136, 160, 136, 48);
    hiCutSlider->setBounds ((getWidth() / 2) + 265 - 136, 105, 136, 48);
    //[UserResized] Add your own custom resize handling here..

	fadeoutDisplay->setBounds(impulseDisplay->getBounds());

    //[/UserResized]
}

void ConvolutionEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == drySlider)
    {
        //[UserSliderCode_drySlider] -- add your slider handling code here..


        //[/UserSliderCode_drySlider]
    }
    else if (sliderThatWasMoved == wetSlider)
    {
        //[UserSliderCode_wetSlider] -- add your slider handling code here..

        //[/UserSliderCode_wetSlider]
    }
    else if (sliderThatWasMoved == predelaySlider)
    {
        //[UserSliderCode_predelaySlider] -- add your slider handling code here..
        //[/UserSliderCode_predelaySlider]
    }
    else if (sliderThatWasMoved == dampingSlider)
    {
        //[UserSliderCode_dampingSlider] -- add your slider handling code here..
        //[/UserSliderCode_dampingSlider]
    }
    else if (sliderThatWasMoved == hiCutSlider)
    {
        //[UserSliderCode_hiCutSlider] -- add your slider handling code here..
        //[/UserSliderCode_hiCutSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void ConvolutionEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == resetButton)
    {
        //[UserButtonCode_resetButton] -- add your button handler code here..
        //[/UserButtonCode_resetButton]
    }
    else if (buttonThatWasClicked == backgroundButton)
    {
        //[UserButtonCode_backgroundButton] -- add your button handler code here..
        //[/UserButtonCode_backgroundButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ConvolutionEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer, public AudioDisplayComponent::Listener"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)&#10;"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="900" initialHeight="300">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 8 600 16M" cornerSize="6" fill="solid: 23000000" hasStroke="0"/>
  </BACKGROUND>
  <SLIDER name="Dry Level" id="109abf6dc0fb35f3" memberName="drySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="56Cr 32 136 48"
          posRelativeX="350c324d3e462faa" min="-100" max="0" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Wet Level" id="89cc5b4c20e221e" memberName="wetSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-118Cr 32 136 48"
          min="-100" max="0" int="0.10000000000000000555" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1" needsCallback="1"/>
  <GENERICCOMPONENT name="new component" id="2d1a3b35c3a9be67" memberName="dryMeter"
                    virtualName="" explicitFocusOrder="0" pos="1 86 136 20" posRelativeX="109abf6dc0fb35f3"
                    class="VuMeter" params=""/>
  <GENERICCOMPONENT name="new component" id="1099df2918c1e835" memberName="wetMeter"
                    virtualName="" explicitFocusOrder="0" pos="1 86 136 20" posRelativeX="89cc5b4c20e221e"
                    class="VuMeter" params=""/>
  <GENERICCOMPONENT name="new component" id="ca07ce0dc9de3398" memberName="impulseDisplay"
                    virtualName="" explicitFocusOrder="0" pos="-282C 129 368 144"
                    class="AudioSampleBufferComponent" params="getProcessor()"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="resetButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="195Cc 215 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Process Input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="284Cr 15 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="convolution" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="26" kerning="0" bold="1" italic="0" justification="34"
         typefaceStyle="Bold"/>
  <TOGGLEBUTTON name="new toggle button" id="f46df9985675be44" memberName="backgroundButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="195Cc 253 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Multithread"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Dry Level" id="6680c278e08a4932" memberName="predelaySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="264Cr 50 136 48"
          posRelativeX="350c324d3e462faa" min="-100" max="0" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Dry Level" id="9360a5b175bde664" memberName="dampingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="265Cr 160 136 48"
          posRelativeX="350c324d3e462faa" min="-100" max="0" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="Dry Level" id="15499c3c9d8b550f" memberName="hiCutSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="265Cr 105 136 48"
          posRelativeX="350c324d3e462faa" min="-100" max="0" int="0.10000000000000000555"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
