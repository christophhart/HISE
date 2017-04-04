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

    addAndMakeVisible (impulseDisplay = new AudioSampleBufferComponent (dynamic_cast<AudioSampleProcessor*>(getProcessor())->getCache()));
    impulseDisplay->setName ("new component");

    addAndMakeVisible (resetButton = new HiToggleButton ("new toggle button"));
    resetButton->setButtonText (TRANS("Process Input"));
    resetButton->addListener (this);
    resetButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("convolution")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

    label->setFont(GLOBAL_BOLD_FONT().withHeight(26.0f));
    
	drySlider->setup(getProcessor(), ConvolutionEffect::DryGain, "Dry Level");
	drySlider->setMode(HiSlider::Decibel);

	wetSlider->setup(getProcessor(), ConvolutionEffect::WetGain, "Wet Level");
	wetSlider->setMode(HiSlider::Decibel);

	dryMeter->setType(VuMeter::Type::StereoVertical);
	wetMeter->setType(VuMeter::Type::StereoVertical);

	dryMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	dryMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	dryMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));

	wetMeter->setColour (VuMeter::backgroundColour, Colour (0xFF333333));
	wetMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	wetMeter->setColour (VuMeter::outlineColour, Colour (0x45FFFFFF));

	resetButton->setup(getProcessor(), ConvolutionEffect::ProcessInput, "Process Input");

	#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

	AudioSampleProcessor *asp = dynamic_cast<AudioSampleProcessor*>(getProcessor());

	impulseDisplay->setAudioSampleBuffer(asp->getBuffer(), asp->getFileName());

	impulseDisplay->addChangeListener(asp);

	impulseDisplay->addAreaListener(this);

	impulseDisplay->setOpaque(false);

    ProcessorEditorLookAndFeel::setupEditorNameLabel(label);
    
    //[/UserPreSize]

    setSize (900, 230);


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


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ConvolutionEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    
    ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(g, this, 600);
    
    g.setColour (Colour (0xAAffffff));
    g.setFont (GLOBAL_BOLD_FONT().withHeight(22.0f));
    
    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ConvolutionEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    drySlider->setBounds ((getWidth() / 2) + 230 - 136, 123, 136, 48);
    wetSlider->setBounds ((getWidth() / 2) + 230 - 136, 67, 136, 48);
    dryMeter->setBounds ((getWidth() / 2) + 255, 123, 24, 48);
    wetMeter->setBounds ((getWidth() / 2) + 255, 67, 24, 48);
    impulseDisplay->setBounds ((getWidth() / 2) + -282, 24, 360, 184);
    resetButton->setBounds ((getWidth() / 2) + 165 - (128 / 2), 175, 128, 32);
    label->setBounds ((getWidth() / 2) + 284 - 264, 15, 264, 40);
    //[UserResized] Add your own custom resize handling here..
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

<JUCER_COMPONENT documentType="Component" className="ConvolutionEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer, public AudioDisplayComponent::Listener"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)&#10;"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="900" initialHeight="230">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 8 600 16M" cornerSize="6" fill="solid: 23000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Dry Level" id="109abf6dc0fb35f3" memberName="drySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="230Cr 123 136 48"
          posRelativeX="350c324d3e462faa" min="-100" max="0" int="0.10000000000000001"
          style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Wet Level" id="89cc5b4c20e221e" memberName="wetSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="230Cr 67 136 48"
          min="-100" max="0" int="0.10000000000000001" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="new component" id="2d1a3b35c3a9be67" memberName="dryMeter"
                    virtualName="" explicitFocusOrder="0" pos="255C 123 24 48" class="VuMeter"
                    params=""/>
  <GENERICCOMPONENT name="new component" id="1099df2918c1e835" memberName="wetMeter"
                    virtualName="" explicitFocusOrder="0" pos="255C 67 24 48" class="VuMeter"
                    params=""/>
  <GENERICCOMPONENT name="new component" id="ca07ce0dc9de3398" memberName="impulseDisplay"
                    virtualName="" explicitFocusOrder="0" pos="-282C 24 360 184"
                    class="AudioSampleBufferComponent" params="dynamic_cast&lt;AudioSampleProcessor*&gt;(getProcessor())-&gt;getCache()"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="resetButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="165Cc 175 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Process Input"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="284Cr 15 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="convolution" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="26" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
