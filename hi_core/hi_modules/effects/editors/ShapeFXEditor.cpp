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
namespace hise {
using namespace juce;
//[/Headers]

#include "ShapeFXEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ShapeFXEditor::ShapeFXEditor (ProcessorEditor* p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
	auto sfx = dynamic_cast<ShapeFX*>(getProcessor());

#if HI_USE_SHAPE_FX_SCRIPTING
	
	tokeniser = new JavascriptTokeniser();
#endif
    //[/Constructor_pre]

    addAndMakeVisible (shapeDisplay = new WaveformComponent (dynamic_cast<ShapeFX*>(getProcessor())));
    shapeDisplay->setName ("WavetableDisplayComponent");

    addAndMakeVisible (biasLeft = new HiSlider ("Bias Left"));
    biasLeft->setTooltip (TRANS("The DC offset for the left channel"));
    biasLeft->setRange (1, 20000, 1);
    biasLeft->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    biasLeft->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    biasLeft->setColour (Slider::backgroundColourId, Colour (0x00000000));
    biasLeft->setColour (Slider::thumbColourId, Colour (0x80666666));
    biasLeft->setColour (Slider::textBoxTextColourId, Colours::white);
    biasLeft->addListener (this);
    biasLeft->setSkewFactor (0.3);

    addAndMakeVisible (outMeter = new VuMeter());
    outMeter->setName ("new component");

    addAndMakeVisible (inMeter = new VuMeter());
    inMeter->setName ("new component");

    addAndMakeVisible (modeSelector = new HiComboBox ("new combo box"));
    modeSelector->setTooltip (TRANS("Choose the waveshape function."));
    modeSelector->setEditableText (false);
    modeSelector->setJustificationType (Justification::centredLeft);
    modeSelector->setTextWhenNothingSelected (TRANS("Function"));
    modeSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    modeSelector->addItem (TRANS("Linear"), 1);
    modeSelector->addItem (TRANS("Atan"), 2);
    modeSelector->addItem (TRANS("Tanh"), 3);
    modeSelector->addItem (TRANS("Saturate"), 4);
    modeSelector->addItem (TRANS("Square"), 5);
    modeSelector->addItem (TRANS("Square Root"), 6);
    modeSelector->addItem (TRANS("Curve"), 7);
#if HI_USE_SHAPE_FX_SCRIPTING
    modeSelector->addItem (TRANS("Script"), 8);
    modeSelector->addItem (TRANS("Cached Script"), 9);
#endif
    modeSelector->addListener (this);

    addAndMakeVisible (biasRight = new HiSlider ("Bias Right"));
    biasRight->setTooltip (TRANS("The DC offset for the right channel"));
    biasRight->setRange (1, 20000, 1);
    biasRight->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    biasRight->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    biasRight->setColour (Slider::backgroundColourId, Colour (0x00000000));
    biasRight->setColour (Slider::thumbColourId, Colour (0x80666666));
    biasRight->setColour (Slider::textBoxTextColourId, Colours::white);
    biasRight->addListener (this);
    biasRight->setSkewFactor (0.3);

    addAndMakeVisible (highPass = new HiSlider ("High Pass"));
    highPass->setTooltip (TRANS("Applies a high pass before the shaper"));
    highPass->setRange (1, 20000, 1);
    highPass->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    highPass->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    highPass->setColour (Slider::backgroundColourId, Colour (0x00000000));
    highPass->setColour (Slider::thumbColourId, Colour (0x80666666));
    highPass->setColour (Slider::textBoxTextColourId, Colours::white);
    highPass->addListener (this);
    highPass->setSkewFactor (0.3);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setRange (1, 20000, 1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    gainSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    gainSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    gainSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    gainSlider->addListener (this);
    gainSlider->setSkewFactor (0.3);

    addAndMakeVisible (reduceSlider = new HiSlider ("Reduce"));
    reduceSlider->setTooltip (TRANS("Applies a bit reduction effect"));
    reduceSlider->setRange (1, 20000, 1);
    reduceSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    reduceSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    reduceSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    reduceSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    reduceSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    reduceSlider->addListener (this);
    reduceSlider->setSkewFactor (0.3);

    addAndMakeVisible (mixSlider = new HiSlider ("Mix"));
    mixSlider->setRange (1, 20000, 1);
    mixSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    mixSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    mixSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    mixSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    mixSlider->addListener (this);
    mixSlider->setSkewFactor (0.3);

    addAndMakeVisible (oversampling = new HiComboBox ("new combo box"));
    oversampling->setTooltip (TRANS("Set the oversampling factor"));
    oversampling->setEditableText (false);
    oversampling->setJustificationType (Justification::centredLeft);
    oversampling->setTextWhenNothingSelected (TRANS("Oversampling"));
    oversampling->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    oversampling->addItem (TRANS("1x"), 1);
    oversampling->addItem (TRANS("2x"), 2);
    oversampling->addItem (TRANS("4x"), 3);
    oversampling->addItem (TRANS("8x"), 4);
    oversampling->addItem (TRANS("16x"), 5);
    oversampling->addListener (this);

    addAndMakeVisible (autoGain = new HiToggleButton ("Auto Gain"));
    autoGain->setTooltip (TRANS("Automatically turns down the volume depending on the gain factor"));
    autoGain->setButtonText (TRANS("Autogain"));
    autoGain->addListener (this);
    autoGain->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (lowPass = new HiSlider ("High Pass"));
    lowPass->setTooltip (TRANS("Applies a Low Pass before the shaper"));
    lowPass->setRange (1, 20000, 1);
    lowPass->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    lowPass->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    lowPass->setColour (Slider::backgroundColourId, Colour (0x00000000));
    lowPass->setColour (Slider::thumbColourId, Colour (0x80666666));
    lowPass->setColour (Slider::textBoxTextColourId, Colours::white);
    lowPass->addListener (this);
    lowPass->setSkewFactor (0.3);

    addAndMakeVisible (table = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<ShapeFX*>(getProcessor())->getTable(0)));
    table->setName ("new component");


#if HI_USE_SHAPE_FX_SCRIPTING
    addAndMakeVisible (editor = new JavascriptCodeEditor (*sfx->getSnippet(0), tokeniser, sfx, "shape"));
    editor->setName ("new component");
#endif

    addAndMakeVisible (limitButton = new HiToggleButton ("Auto Gain"));
    limitButton->setTooltip (TRANS("Apply a soft limiter before the input to avoid hard clipping"));
    limitButton->setButtonText (TRANS("Limit Input"));
    limitButton->addListener (this);
    limitButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	START_TIMER();

	oversampling->setup(getProcessor(), ShapeFX::SpecialParameters::Oversampling, "Oversampling");
	mixSlider->setup(getProcessor(), ShapeFX::SpecialParameters::Mix, "Mix");
	mixSlider->setMode(HiSlider::NormalizedPercentage);

	autoGain->setup(getProcessor(), ShapeFX::SpecialParameters::Autogain, "Autogain");



	//driveSlider->setup(getProcessor(), ShapeFX::SpecialParameters::Drive, "Drive");
	//driveSlider->setMode(HiSlider::NormalizedPercentage);

	reduceSlider->setup(getProcessor(), ShapeFX::SpecialParameters::Reduce, "Reduce");
	reduceSlider->setMode(HiSlider::Discrete, 0.0, 14.0, 7, 1.0);
	reduceSlider->setTextValueSuffix(" bits");

	gainSlider->setup(getProcessor(), ShapeFX::SpecialParameters::Gain, "Gain");
	gainSlider->setMode(HiSlider::Decibel, 0.0, 60.0, 24.0, 0.1);

	highPass->setup(getProcessor(), ShapeFX::SpecialParameters::HighPass, "High Pass");
	highPass->setMode(HiSlider::Frequency, 20.0, 8000.0, 200.0, 1.0);

	lowPass->setup(getProcessor(), ShapeFX::SpecialParameters::LowPass, "Low Pass");
	lowPass->setMode(HiSlider::Frequency, 200.0, 20000.0, 1500.0, 1.0);

	modeSelector->setup(getProcessor(), ShapeFX::SpecialParameters::Mode , "Mode");

	biasLeft->setup(getProcessor(), ShapeFX::SpecialParameters::BiasLeft, "Bias Left");
	biasLeft->setMode(HiSlider::Linear, -1.0, 1.0, 0.0, 0.01);

	biasRight->setup(getProcessor(), ShapeFX::SpecialParameters::BiasRight, "Bias Right");
	biasRight->setMode(HiSlider::Linear, -1.0, 1.0, 0.0, 0.01);

	limitButton->setup(getProcessor(), ShapeFX::SpecialParameters::LimitInput, "Limit Input");

	inMeter->setType(VuMeter::Type::StereoVertical);
	outMeter->setType(VuMeter::Type::StereoVertical);

	inMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	inMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	inMeter->setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));

	outMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	outMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	outMeter->setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));

	modeSelector->clear(dontSendNotification);

	auto sa = sfx->getShapeNames();

	for (int i = 0; i < sa.size(); i++)
	{
		if (sa[i] != "unused")
		{
			modeSelector->addItem(sa[i], i);
		}
	}

    //[/UserPreSize]

    setSize (800, 600);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

ShapeFXEditor::~ShapeFXEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    shapeDisplay = nullptr;
    biasLeft = nullptr;
    outMeter = nullptr;
    inMeter = nullptr;
    modeSelector = nullptr;
    biasRight = nullptr;
    highPass = nullptr;
    gainSlider = nullptr;
    reduceSlider = nullptr;
    mixSlider = nullptr;
    oversampling = nullptr;
    autoGain = nullptr;
    lowPass = nullptr;
    table = nullptr;

#if HI_USE_SHAPE_FX_SCRIPTING
    editor = nullptr;
#endif
    limitButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ShapeFXEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    {
        float x = static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), y = 6.0f, width = static_cast<float> (getWidth() - 84), height = static_cast<float> (getHeight() - 12);
        Colour fillColour = Colour (0x30000000);
        Colour strokeColour = Colour (0x25ffffff);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRoundedRectangle (x, y, width, height, 6.000f);
        g.setColour (strokeColour);
        g.drawRoundedRectangle (x, y, width, height, 6.000f, 2.000f);
    }

    {
        int x = getWidth() - 53 - 200, y = 6, width = 200, height = 40;
        String text (TRANS("shape fx"));
        Colour fillColour = Colour (0x52ffffff);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (Font ("Arial", 24.00f, Font::plain).withTypefaceStyle ("Bold"));
        g.drawText (text, x, y, width, height,
                    Justification::centredRight, true);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ShapeFXEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    shapeDisplay->setBounds ((getWidth() / 2) - (256 / 2), 48, 256, 256);
    biasLeft->setBounds ((getWidth() / 2) + -264 - (128 / 2), 56, 128, 48);
    outMeter->setBounds ((getWidth() / 2) + 160 - (20 / 2), 48, 20, 256);
    inMeter->setBounds ((getWidth() / 2) + -158 - (20 / 2), 48, 20, 256);
    modeSelector->setBounds ((getWidth() / 2) + -104 - (128 / 2), 16, 128, 24);
    biasRight->setBounds (((getWidth() / 2) + -264 - (128 / 2)) + 0, 120, 128, 48);
    highPass->setBounds (((getWidth() / 2) + -264 - (128 / 2)) + 0, 248, 128, 48);
    gainSlider->setBounds ((getWidth() / 2) + 264 - (128 / 2), 48, 128, 48);
    reduceSlider->setBounds (((getWidth() / 2) + -264 - (128 / 2)) + 528, 112, 128, 48);
    mixSlider->setBounds ((getWidth() / 2) + 272 - (128 / 2), 256, 128, 48);
    oversampling->setBounds ((getWidth() / 2) + 64 - (128 / 2), 16, 128, 24);
    autoGain->setBounds (((getWidth() / 2) + 264 - (128 / 2)) + 128 / 2 + 8 - (128 / 2), 177, 128, 32);
    lowPass->setBounds (((getWidth() / 2) + -264 - (128 / 2)) + 0, 184, 128, 48);
    table->setBounds ((getWidth() / 2) + -2 - ((getWidth() - 109) / 2), 319, getWidth() - 109, 257);

#if HI_USE_SHAPE_FX_SCRIPTING
    editor->setBounds ((getWidth() / 2) + 2 - ((getWidth() - 109) / 2), 320, getWidth() - 109, 257);
#endif

    limitButton->setBounds (((getWidth() / 2) + 264 - (128 / 2)) + 128 / 2 + 8 - (128 / 2), 224, 128, 32);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ShapeFXEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == biasLeft)
    {
        //[UserSliderCode_biasLeft] -- add your slider handling code here..
        //[/UserSliderCode_biasLeft]
    }
    else if (sliderThatWasMoved == biasRight)
    {
        //[UserSliderCode_biasRight] -- add your slider handling code here..
        //[/UserSliderCode_biasRight]
    }
    else if (sliderThatWasMoved == highPass)
    {
        //[UserSliderCode_highPass] -- add your slider handling code here..
        //[/UserSliderCode_highPass]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == reduceSlider)
    {
        //[UserSliderCode_reduceSlider] -- add your slider handling code here..
        //[/UserSliderCode_reduceSlider]
    }
    else if (sliderThatWasMoved == mixSlider)
    {
        //[UserSliderCode_mixSlider] -- add your slider handling code here..
        //[/UserSliderCode_mixSlider]
    }
    else if (sliderThatWasMoved == lowPass)
    {
        //[UserSliderCode_lowPass] -- add your slider handling code here..
        //[/UserSliderCode_lowPass]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void ShapeFXEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == modeSelector)
    {
        //[UserComboBoxCode_modeSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_modeSelector]
    }
    else if (comboBoxThatHasChanged == oversampling)
    {
        //[UserComboBoxCode_oversampling] -- add your combo box handling code here..
        //[/UserComboBoxCode_oversampling]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void ShapeFXEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == autoGain)
    {
        //[UserButtonCode_autoGain] -- add your button handler code here..
        //[/UserButtonCode_autoGain]
    }
    else if (buttonThatWasClicked == limitButton)
    {
        //[UserButtonCode_limitButton] -- add your button handler code here..
        //[/UserButtonCode_limitButton]
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

<JUCER_COMPONENT documentType="Component" className="ShapeFXEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor* p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="600">
  <BACKGROUND backgroundColour="323e44">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="shape fx"
          fontname="Arial" fontsize="24" kerning="0" bold="1" italic="0"
          justification="34" typefaceStyle="Bold"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="WavetableDisplayComponent" id="9694b8a8ec6e9c20" memberName="shapeDisplay"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 48 256 256" class="WaveformComponent"
                    params="dynamic_cast&lt;ShapeFX*&gt;(getProcessor())"/>
  <SLIDER name="Bias Left" id="a7c54198d4a84cc" memberName="biasLeft" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-264Cc 56 128 48" posRelativeX="557420bb82cec3a9"
          tooltip="The DC offset for the left channel" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.2999999999999999889" needsCallback="1"/>
  <GENERICCOMPONENT name="new component" id="124338752f9a014" memberName="outMeter"
                    virtualName="" explicitFocusOrder="0" pos="160Cc 48 20 256" class="VuMeter"
                    params=""/>
  <GENERICCOMPONENT name="new component" id="49cb22e36f56f4d" memberName="inMeter"
                    virtualName="" explicitFocusOrder="0" pos="-158Cc 48 20 256"
                    class="VuMeter" params=""/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="modeSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="-104Cc 16 128 24"
            posRelativeX="3b242d8d6cab6cc3" tooltip="Choose the waveshape function."
            editable="0" layout="33" items="Linear&#10;Atan&#10;Tanh&#10;Saturate&#10;Square&#10;Square Root&#10;Curve&#10;Script&#10;Cached Script"
            textWhenNonSelected="Function" textWhenNoItems="(no choices)"/>
  <SLIDER name="Bias Right" id="5fbcc7b448500dac" memberName="biasRight"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0 120 128 48"
          posRelativeX="a7c54198d4a84cc" tooltip="The DC offset for the right channel"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <SLIDER name="High Pass" id="95de38dc8041d32c" memberName="highPass"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0 248 128 48"
          posRelativeX="a7c54198d4a84cc" tooltip="Applies a high pass before the shaper"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <SLIDER name="Gain" id="905c9d3b1d279d06" memberName="gainSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="264Cc 48 128 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <SLIDER name="Reduce" id="62246e674bc79ce8" memberName="reduceSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="528 112 128 48"
          posRelativeX="a7c54198d4a84cc" tooltip="Applies a bit reduction effect"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <SLIDER name="Mix" id="27eb3f7c71cca4dc" memberName="mixSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="272Cc 256 128 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <COMBOBOX name="new combo box" id="c2dee3e04a7ede9b" memberName="oversampling"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="64Cc 16 128 24"
            posRelativeX="3b242d8d6cab6cc3" tooltip="Set the oversampling factor"
            editable="0" layout="33" items="1x&#10;2x&#10;4x&#10;8x&#10;16x"
            textWhenNonSelected="Oversampling" textWhenNoItems="(no choices)"/>
  <TOGGLEBUTTON name="Auto Gain" id="e6345feaa3cb5bea" memberName="autoGain"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="8Cc 177 128 32"
                posRelativeX="905c9d3b1d279d06" tooltip="Automatically turns down the volume depending on the gain factor"
                txtcol="ffffffff" buttonText="Autogain" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="High Pass" id="bb6d2cf7c7745092" memberName="lowPass" virtualName="HiSlider"
          explicitFocusOrder="0" pos="0 184 128 48" posRelativeX="a7c54198d4a84cc"
          tooltip="Applies a Low Pass before the shaper" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.2999999999999999889" needsCallback="1"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="table"
                    virtualName="" explicitFocusOrder="0" pos="-1.5Cc 319 109M 257"
                    class="TableEditor" params="getProcessor()-&gt;getMainController()-&gt;getControlUndoManager(), static_cast&lt;ShapeFX*&gt;(getProcessor())-&gt;getTable(0)"/>
  <GENERICCOMPONENT name="new component" id="ed787baea400e650" memberName="editor"
                    virtualName="" explicitFocusOrder="0" pos="1.5Cc 320 109M 257"
                    class="JavascriptCodeEditor" params="*sfx-&gt;getSnippet(0), tokeniser, sfx, &quot;shape&quot;"/>
  <TOGGLEBUTTON name="Auto Gain" id="71e2a24e9e8fdf6b" memberName="limitButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="8Cc 224 128 32"
                posRelativeX="905c9d3b1d279d06" tooltip="Apply a soft limiter before the input to avoid hard clipping"
                txtcol="ffffffff" buttonText="Limit Input" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
}
//[/EndFile]
