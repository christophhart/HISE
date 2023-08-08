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

#include "PolyShapeFXEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PolyShapeFXEditor::PolyShapeFXEditor (ProcessorEditor* p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
	auto sfx = dynamic_cast<PolyshapeFX*>(getProcessor());
    //[/Constructor_pre]

    addAndMakeVisible (shapeDisplay = new WaveformComponent (dynamic_cast<PolyshapeFX*>(getProcessor())));
    shapeDisplay->setName ("WavetableDisplayComponent");

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
    modeSelector->addItem (TRANS("Script"), 8);
    modeSelector->addItem (TRANS("Cached Script"), 9);
    modeSelector->addListener (this);

    addAndMakeVisible (driveSlider = new HiSlider ("Drive"));
    driveSlider->setRange (1, 20000, 1);
    driveSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    driveSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    driveSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    driveSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    driveSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    driveSlider->addListener (this);
    driveSlider->setSkewFactor (0.3);

    addAndMakeVisible (overSampling = new HiToggleButton ("Auto Gain"));
    overSampling->setTooltip (TRANS("Applies 4x Oversampling to the shaper"));
    overSampling->setButtonText (TRANS("Oversample"));
    overSampling->addListener (this);
    overSampling->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (table = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<PolyshapeFX*>(getProcessor())->getTable(0)));
    table->setName ("new component");

    addAndMakeVisible (bias = new HiSlider ("Bias"));
    bias->setRange (1, 20000, 1);
    bias->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    bias->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    bias->setColour (Slider::backgroundColourId, Colour (0x00000000));
    bias->setColour (Slider::thumbColourId, Colour (0x80666666));
    bias->setColour (Slider::textBoxTextColourId, Colours::white);
    bias->addListener (this);
    bias->setSkewFactor (0.3);

    addAndMakeVisible (table2 = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<PolyshapeFX*>(getProcessor())->getTable(1)));
    table2->setName ("new component");


    //[UserPreSize]

	driveSlider->setup(getProcessor(), PolyshapeFX::SpecialParameters::Drive, "Drive");
	driveSlider->setMode(HiSlider::Decibel, 0.0, 60.0, 24.0, 0.1);
	driveSlider->setIsUsingModulatedRing(true);

	modeSelector->setup(getProcessor(), PolyshapeFX::SpecialParameters::Mode, "Mode");
	modeSelector->clear(dontSendNotification);
	auto sa = sfx->getShapeNames();



	for (int i = 0; i < sa.size(); i++)
	{
		if (sa[i] != "unused")
		{
			modeSelector->addItem(sa[i], i);
		}
	}

	overSampling->setup(getProcessor(), PolyshapeFX::SpecialParameters::Oversampling, "Oversampling");

	bias->setup(getProcessor(), PolyshapeFX::SpecialParameters::Bias, "Bias");
	bias->setMode(HiSlider::NormalizedPercentage);

	

    //[/UserPreSize]

    setSize (800, 200);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
	startTimer(50);
    //[/Constructor]
}

PolyShapeFXEditor::~PolyShapeFXEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    shapeDisplay = nullptr;
    modeSelector = nullptr;
    driveSlider = nullptr;
    overSampling = nullptr;
    table = nullptr;
    bias = nullptr;
    table2 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void PolyShapeFXEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    {
        float x = static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), y = 6.0f, width = static_cast<float> (getWidth() - 84), height = static_cast<float> (getHeight() - 12);
        Colour fillColour = Colour (0x30000000);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRoundedRectangle (x, y, width, height, 6.000f);
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

void PolyShapeFXEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    shapeDisplay->setBounds ((getWidth() / 2) + -136 - (128 / 2), 48, 128, 128);
    modeSelector->setBounds ((getWidth() / 2) + -136 - (128 / 2), 8, 128, 24);
    driveSlider->setBounds ((getWidth() / 2) + -280 - (128 / 2), 24, 128, 48);
    overSampling->setBounds (((getWidth() / 2) + -280 - (128 / 2)) + 128 / 2 + 288 - (128 / 2), 8, 128, 32);
    table->setBounds ((getWidth() / 2) + 140 - ((getWidth() - 408) / 2), 48, getWidth() - 408, 128);
    bias->setBounds ((getWidth() / 2) + -280 - (128 / 2), 88, 128, 48);
    table2->setBounds ((getWidth() / 2) + 140 - ((getWidth() - 408) / 2), 48, getWidth() - 408, 128);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void PolyShapeFXEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == modeSelector)
    {
        //[UserComboBoxCode_modeSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_modeSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void PolyShapeFXEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == driveSlider)
    {
        //[UserSliderCode_driveSlider] -- add your slider handling code here..
        //[/UserSliderCode_driveSlider]
    }
    else if (sliderThatWasMoved == bias)
    {
        //[UserSliderCode_bias] -- add your slider handling code here..
        //[/UserSliderCode_bias]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void PolyShapeFXEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == overSampling)
    {
        //[UserButtonCode_overSampling] -- add your button handler code here..
        //[/UserButtonCode_overSampling]
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

<JUCER_COMPONENT documentType="Component" className="PolyShapeFXEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor* p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="200">
  <BACKGROUND backgroundColour="323e44">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="0"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="shape fx"
          fontname="Arial" fontsize="24" kerning="0" bold="1" italic="0"
          justification="34" typefaceStyle="Bold"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="WavetableDisplayComponent" id="9694b8a8ec6e9c20" memberName="shapeDisplay"
                    virtualName="" explicitFocusOrder="0" pos="-136Cc 48 128 128"
                    class="WaveformComponent" params="dynamic_cast&lt;PolyshapeFX*&gt;(getProcessor())"/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="modeSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="-136Cc 8 128 24"
            posRelativeX="3b242d8d6cab6cc3" tooltip="Choose the waveshape function."
            editable="0" layout="33" items="Linear&#10;Atan&#10;Tanh&#10;Saturate&#10;Square&#10;Square Root&#10;Curve&#10;Script&#10;Cached Script"
            textWhenNonSelected="Function" textWhenNoItems="(no choices)"/>
  <SLIDER name="Drive" id="905c9d3b1d279d06" memberName="driveSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-280Cc 24 128 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <TOGGLEBUTTON name="Auto Gain" id="e6345feaa3cb5bea" memberName="overSampling"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="288Cc 8 128 32"
                posRelativeX="905c9d3b1d279d06" tooltip="Applies 4x Oversampling to the shaper"
                txtcol="ffffffff" buttonText="Oversample" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="table"
                    virtualName="" explicitFocusOrder="0" pos="140Cc 48 408M 128"
                    class="TableEditor" params="getProcessor()-&gt;getMainController()-&gt;getControlUndoManager(), static_cast&lt;PolyshapeFX*&gt;(getProcessor())-&gt;getTable(0)"/>
  <SLIDER name="Bias" id="148cb90a5e344e47" memberName="bias" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-280Cc 88 128 48" posRelativeX="557420bb82cec3a9"
          bkgcol="0" thumbcol="80666666" textboxtext="ffffffff" min="1"
          max="20000" int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="0.2999999999999999889"
          needsCallback="1"/>
  <GENERICCOMPONENT name="new component" id="ff55275c5ebb746b" memberName="table2"
                    virtualName="" explicitFocusOrder="0" pos="140Cc 48 408M 128"
                    class="TableEditor" params="getProcessor()-&gt;getMainController()-&gt;getControlUndoManager(), static_cast&lt;PolyshapeFX*&gt;(getProcessor())-&gt;getTable(1)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
}
//[/EndFile]
