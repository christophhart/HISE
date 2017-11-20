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

#include "GainCollectorEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
GainCollectorEditor::GainCollectorEditor (ProcessorEditor *pe)
    : ProcessorEditorBody(pe)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (smoothSlider = new HiSlider ("Mix"));
    smoothSlider->setRange (0, 100, 1);
    smoothSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    smoothSlider->addListener (this);

    addAndMakeVisible (gainMeter = new VuMeter());
    gainMeter->setName ("new component");

    addAndMakeVisible (modeSelector = new HiComboBox ("new combo box"));
    modeSelector->setEditableText (false);
    modeSelector->setJustificationType (Justification::centredLeft);
    modeSelector->setTextWhenNothingSelected (TRANS("Filter mode"));
    modeSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    modeSelector->addItem (TRANS("Simple LP"), 1);
    modeSelector->addItem (TRANS("Attack / Release"), 2);
    modeSelector->addListener (this);

    addAndMakeVisible (attackSlider = new HiSlider ("Mix"));
    attackSlider->setRange (0, 100, 1);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    attackSlider->addListener (this);

    addAndMakeVisible (releaseSlider = new HiSlider ("Mix"));
    releaseSlider->setRange (0, 100, 1);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    releaseSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("gain collector")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	smoothSlider->setup(getProcessor(), GainCollector::Smoothing, "Smoothing Time");
	smoothSlider->setMode(HiSlider::Time);
	modeSelector->setup(getProcessor(), GainCollector::Mode, "Mode");
	attackSlider->setup(getProcessor(), GainCollector::Attack, "Attack Time");
	attackSlider->setMode(HiSlider::Time);
	releaseSlider->setup(getProcessor(), GainCollector::Release, "Release Time");
	releaseSlider->setMode(HiSlider::Time);

	gainMeter->setType(VuMeter::MonoVertical);

	gainMeter->setOpaque(false);

	gainMeter->setColour(VuMeter::ColourId::backgroundColour, Colours::transparentBlack);

	gainMeter->setColour(VuMeter::ColourId::outlineColour, Colours::white.withAlpha(0.6f));

	START_TIMER();

    //[/UserPreSize]

    setSize (900, 150);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

GainCollectorEditor::~GainCollectorEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    smoothSlider = nullptr;
    gainMeter = nullptr;
    modeSelector = nullptr;
    attackSlider = nullptr;
    releaseSlider = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void GainCollectorEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) + 1 - (600 / 2)), 8.0f, 600.0f, static_cast<float> (getHeight() - 16), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) + 1 - (600 / 2)), 8.0f, 600.0f, static_cast<float> (getHeight() - 16), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void GainCollectorEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    smoothSlider->setBounds ((getWidth() / 2) + -162 - (128 / 2), 72, 128, 48);
    gainMeter->setBounds ((getWidth() / 2) + -242 - 40, 24, 40, 104);
    modeSelector->setBounds ((getWidth() / 2) + -231, 24, 128, 24);
    attackSlider->setBounds ((getWidth() / 2) + -10 - (128 / 2), 72, 128, 48);
    releaseSlider->setBounds ((getWidth() / 2) + 134 - (128 / 2), 72, 128, 48);
    label->setBounds ((getWidth() / 2) + 299 - 264, 6, 264, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void GainCollectorEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == smoothSlider)
    {
        //[UserSliderCode_smoothSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothSlider]
    }
    else if (sliderThatWasMoved == attackSlider)
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

void GainCollectorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
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



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GainCollectorEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *pe"
                 variableInitialisers="ProcessorEditorBody(pe)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="150">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="1Cc 8 600 16M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Mix" id="9115805b4b27f781" memberName="smoothSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-162Cc 72 128 48" posRelativeX="f930000f86c6c8b6"
          min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="new component" id="9e5ade2b42bef7dc" memberName="gainMeter"
                    virtualName="" explicitFocusOrder="0" pos="-242Cr 24 40 104"
                    class="VuMeter" params=""/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="modeSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="-231C 24 128 24"
            posRelativeX="3b242d8d6cab6cc3" editable="0" layout="33" items="Simple LP&#10;Attack / Release"
            textWhenNonSelected="Filter mode" textWhenNoItems="(no choices)"/>
  <SLIDER name="Mix" id="43cfb0cd079133bb" memberName="attackSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-10Cc 72 128 48" posRelativeX="f930000f86c6c8b6"
          min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Mix" id="dcd3335f0b4d518e" memberName="releaseSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="134Cc 72 128 48" posRelativeX="f930000f86c6c8b6"
          min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="299Cr 6 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="gain collector"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="26" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
