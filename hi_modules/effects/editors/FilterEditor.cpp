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

#include "FilterEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
FilterEditor::FilterEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (freqSlider = new HiSlider ("Frequency"));
    freqSlider->setRange (20, 20000, 1);
    freqSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freqSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freqSlider->addListener (this);

    addAndMakeVisible (qSlider = new HiSlider ("Q"));
    qSlider->setRange (0.3, 8, 0.1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setRange (-24, 24, 0.1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);
    
    addAndMakeVisible (keytrackSlider = new HiSlider ("Keytrack"));
    keytrackSlider->setRange (0.0, 1.0, 0.1);
    keytrackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    keytrackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    keytrackSlider->addListener (this);

    addAndMakeVisible (modeSelector = new ComboBox ("new combo box"));
    modeSelector->setEditableText (false);
    modeSelector->setJustificationType (Justification::centredLeft);
    modeSelector->setTextWhenNothingSelected (TRANS("Filter mode"));
    modeSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    modeSelector->setColour(MacroControlledObject::HiBackgroundColours::textColour, Colours::white);
	modeSelector->addItem(TRANS("1 Pole LP"), MonoFilterEffect::FilterMode::OnePoleLowPass + 1);
	modeSelector->addItem(TRANS("1 Pole HP"), MonoFilterEffect::FilterMode::OnePoleHighPass + 1);
	modeSelector->addItem(TRANS("SVF LP"), MonoFilterEffect::FilterMode::StateVariableLP + 1);
	modeSelector->addItem(TRANS("SVF HP"), MonoFilterEffect::FilterMode::StateVariableHP + 1);
	modeSelector->addItem(TRANS("Moog LP"), MonoFilterEffect::FilterMode::MoogLP + 1);
	modeSelector->addItem (TRANS("Biquad LP"), MonoFilterEffect::FilterMode::LowPass + 1);
    modeSelector->addItem (TRANS("Biquad HP"), MonoFilterEffect::FilterMode::HighPass + 1);
	modeSelector->addItem(TRANS("Biquad LP Rez"), MonoFilterEffect::FilterMode::ResoLow + 1);
    modeSelector->addItem (TRANS("Low Shelf EQ"), MonoFilterEffect::FilterMode::LowShelf + 1);
    modeSelector->addItem (TRANS("High Shelf EQ"), MonoFilterEffect::FilterMode::HighShelf + 1);
    modeSelector->addItem (TRANS("Peak EQ"), MonoFilterEffect::FilterMode::Peak + 1);
    modeSelector->addListener (this);

    addAndMakeVisible (filterGraph = new FilterGraph (1));
    filterGraph->setName ("new component");

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("filter")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	gainSlider->setup(getProcessor(), MonoFilterEffect::Gain, "Gain");
	gainSlider->setMode(HiSlider::Decibel, -18.0, 18.0, 0.0);

	qSlider->setup(getProcessor(), MonoFilterEffect::Q, "Q");
	qSlider->setMode(HiSlider::Linear, 0.3, 8.0, 1.0);

	freqSlider->setup(getProcessor(), MonoFilterEffect::Frequency, "Frequency");
    freqSlider->setMode(HiSlider::Frequency, 20.0, 20000.0, 1500.0);
    
    keytrackSlider->setup(getProcessor(), PolyFilterEffect::Keytrack, "Keytrack");
    keytrackSlider->setMode(HiSlider::Linear, 0.0, 1.0, 0.01);

	getProcessor()->getMainController()->skin(*modeSelector);

    filterGraph->addFilter(FilterType::LowPass);

#if JUCE_DEBUG
	startTimer(150);
#else
	startTimer(30);
#endif

    //[/UserPreSize]

    setSize (800, 180);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

    ProcessorEditorLookAndFeel::setupEditorNameLabel(label);
    
	freqSlider->setIsUsingModulatedRing(true);

    //[/Constructor]
}

FilterEditor::~FilterEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    freqSlider = nullptr;
    qSlider = nullptr;
    gainSlider = nullptr;
    keytrackSlider = nullptr;
    modeSelector = nullptr;
    filterGraph = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void FilterEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    
    //[UserPaint] Add your own custom painting code here..

    ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(g, this, 600);
    
	KnobLookAndFeel::drawHiBackground(g, filterGraph->getX() - 5, filterGraph->getY() - 5, filterGraph->getWidth() + 10, filterGraph->getHeight() + 10, nullptr, false);

    //[/UserPaint]
}

void FilterEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    freqSlider->setBounds ((getWidth() / 2) + -73 - (128 / 2), 118, 128, 48);
    qSlider->setBounds (((getWidth() / 2) + -73 - (128 / 2)) + 128 - -16, 118, 128, 48);
    gainSlider->setBounds (((getWidth() / 2) + -73 - (128 / 2)) + -16 - 128, 118, 128, 48);
    keytrackSlider->setBounds (((getWidth() / 2) + -73 - (128 / 2)) + -128 - 128, 118, 128, 48);
    modeSelector->setBounds ((((getWidth() / 2) + -73 - (128 / 2)) + 128 - -16) + 128 - -23, 82, 128, 28);
    filterGraph->setBounds ((getWidth() / 2) + -69 - (proportionOfWidth (0.5075f) / 2), 16, proportionOfWidth (0.5075f), 88);
    label->setBounds ((getWidth() / 2) + 242 - (100 / 2), 7, 100, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void FilterEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == freqSlider)
    {
        //[UserSliderCode_freqSlider] -- add your slider handling code here..


        //[/UserSliderCode_freqSlider]
    }
    else if (sliderThatWasMoved == qSlider)
    {
        //[UserSliderCode_qSlider] -- add your slider handling code here..


        //[/UserSliderCode_qSlider]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..


        //[/UserSliderCode_gainSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void FilterEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == modeSelector)
    {
        //[UserComboBoxCode_modeSelector] -- add your combo box handling code here..

		getProcessor()->setAttribute(MonoFilterEffect::Mode, (float)modeSelector->getSelectedId() - 1, dontSendNotification);

		updateGui();

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

<JUCER_COMPONENT documentType="Component" className="FilterEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="180">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 600 168" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="Frequency" id="f930000f86c6c8b6" memberName="freqSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-73Cc 118 128 48"
          min="20" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Q" id="3b242d8d6cab6cc3" memberName="qSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-16R 118 128 48" posRelativeX="f930000f86c6c8b6"
          min="0.29999999999999999" max="8" int="0.10000000000000001" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Gain" id="89cc5b4c20e221e" memberName="gainSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="-16r 118 128 48" posRelativeX="f930000f86c6c8b6"
          min="-24" max="24" int="0.10000000000000001" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="modeSelector"
            virtualName="" explicitFocusOrder="0" pos="-23R 82 128 24" posRelativeX="3b242d8d6cab6cc3"
            editable="0" layout="33" items="Low Pass&#10;High Pass&#10;Low Shelf&#10;High Shelf&#10;Peak&#10;Resonant LP&#10;State Variable LP&#10;State Variable HP&#10;Moog LP"
            textWhenNonSelected="Filter mode" textWhenNoItems="(no choices)"/>
  <GENERICCOMPONENT name="new component" id="664814e01ba3b705" memberName="filterGraph"
                    virtualName="" explicitFocusOrder="0" pos="-69Cc 16 50.75% 88"
                    class="FilterGraph" params="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="242Cc 7 100 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="filter" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="26" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
