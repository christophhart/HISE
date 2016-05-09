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

#include "CCEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
CCEnvelopeEditor::CCEnvelopeEditor (BetterProcessorEditor *pe)
    : ProcessorEditorBody(pe)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("cc envelope")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x3fffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (midiTable = new TableEditor (static_cast<CCEnvelope*>(getProcessor())->getTable()));
    midiTable->setName ("new component");

    addAndMakeVisible (useTableButton = new ToggleButton ("new toggle button"));
    useTableButton->setTooltip (TRANS("Use a look up table to calculate the modulation value."));
    useTableButton->setButtonText (TRANS("UseTable"));
    useTableButton->addListener (this);
    useTableButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (controllerNumberSlider = new HiSlider ("CC Nr."));
    controllerNumberSlider->setTooltip (TRANS("The CC number"));
    controllerNumberSlider->setRange (1, 128, 1);
    controllerNumberSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    controllerNumberSlider->setTextBoxStyle (Slider::TextBoxRight, false, 30, 20);
    controllerNumberSlider->addListener (this);

    addAndMakeVisible (smoothingSlider = new HiSlider ("Smoothing"));
    smoothingSlider->setTooltip (TRANS("Smoothing Value"));
    smoothingSlider->setRange (0, 2000, 0);
    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    smoothingSlider->addListener (this);

    addAndMakeVisible (learnButton = new ToggleButton ("new toggle button"));
    learnButton->setButtonText (TRANS("MIDI Learn"));
    learnButton->addListener (this);
    learnButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (defaultSlider = new HiSlider ("Default"));
    defaultSlider->setTooltip (TRANS("Smoothing Value"));
    defaultSlider->setRange (0, 127, 0);
    defaultSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    defaultSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    defaultSlider->addListener (this);

    addAndMakeVisible (decayTimeSlider = new HiSlider ("Default"));
    decayTimeSlider->setTooltip (TRANS("Smoothing Value"));
    decayTimeSlider->setRange (0, 127, 0);
    decayTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    decayTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    decayTimeSlider->addListener (this);

    addAndMakeVisible (holdTimeSlider = new HiSlider ("Default"));
    holdTimeSlider->setTooltip (TRANS("Smoothing Value"));
    holdTimeSlider->setRange (0, 127, 0);
    holdTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    holdTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    holdTimeSlider->addListener (this);

    addAndMakeVisible (display = new CCEnvelopeDisplay (dynamic_cast<CCEnvelope*>(getProcessor())));
    display->setName ("new component");

    addAndMakeVisible (fixedNoteOffButton = new HiToggleButton ("new toggle button"));
    fixedNoteOffButton->setTooltip (TRANS("Don\'t alter the output after the note off"));
    fixedNoteOffButton->setButtonText (TRANS("Fixed Note Off"));
    fixedNoteOffButton->addListener (this);
    fixedNoteOffButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (startLevelSlider = new HiSlider ("Default"));
    startLevelSlider->setTooltip (TRANS("Smoothing Value"));
    startLevelSlider->setRange (0, 127, 0);
    startLevelSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    startLevelSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    startLevelSlider->addListener (this);

    addAndMakeVisible (endLevelSlider = new HiSlider ("Default"));
    endLevelSlider->setTooltip (TRANS("Smoothing Value"));
    endLevelSlider->setRange (0, 127, 0);
    endLevelSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    endLevelSlider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    endLevelSlider->addListener (this);


    //[UserPreSize]

	CCEnvelope *cm = static_cast<CCEnvelope*>(this->getProcessor());

	smoothingSlider->setup(getProcessor(), CCEnvelope::SmoothTime, "Smoothing");
	smoothingSlider->setMode(HiSlider::Mode::Time, 0, 1000.0, 100.0);

	controllerNumberSlider->setup(getProcessor(), CCEnvelope::ControllerNumber, "CC Number");
	controllerNumberSlider->setMode(HiSlider::Discrete, 0, 129.0, 64.0);

	defaultSlider->setup(getProcessor(), CCEnvelope::DefaultValue, "Default");
	defaultSlider->setMode(HiSlider::Discrete, 0.0, 127.0);
	defaultSlider->setRange(0.0, 127.0, 1.0);

	holdTimeSlider->setup(getProcessor(), CCEnvelope::HoldTime, "Hold Time");
	holdTimeSlider->setMode(HiSlider::Time);
	holdTimeSlider->setIsUsingModulatedRing(true);

	decayTimeSlider->setup(getProcessor(), CCEnvelope::DecayTime, "Decay Time");
	decayTimeSlider->setMode(HiSlider::Time);
	decayTimeSlider->setIsUsingModulatedRing(true);

	startLevelSlider->setup(getProcessor(), CCEnvelope::StartLevel, "Start Level");
	startLevelSlider->setMode(HiSlider::Decibel);
	startLevelSlider->setIsUsingModulatedRing(true);

	endLevelSlider->setup(getProcessor(), CCEnvelope::EndLevel, "EndLevel");
	endLevelSlider->setMode(HiSlider::Decibel);
	endLevelSlider->setIsUsingModulatedRing(true);

	fixedNoteOffButton->setup(getProcessor(), CCEnvelope::FixedNoteOff, "Fixed Note Off");


    label->setFont (GLOBAL_BOLD_FONT().withHeight(26.0f));
    
	getProcessor()->getMainController()->skin(*useTableButton);
	getProcessor()->getMainController()->skin(*learnButton);

	tableUsed = cm->getAttribute(CCEnvelope::UseTable) == 1.0f;

	midiTable->connectToLookupTableProcessor(cm);

	startTimer(30);

    //[/UserPreSize]

    setSize (900, 340);


    //[Constructor] You can add your own custom stuff here..

	h = getHeight();

    //[/Constructor]
}

CCEnvelopeEditor::~CCEnvelopeEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    midiTable = nullptr;
    useTableButton = nullptr;
    controllerNumberSlider = nullptr;
    smoothingSlider = nullptr;
    learnButton = nullptr;
    defaultSlider = nullptr;
    decayTimeSlider = nullptr;
    holdTimeSlider = nullptr;
    display = nullptr;
    fixedNoteOffButton = nullptr;
    startLevelSlider = nullptr;
    endLevelSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CCEnvelopeEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CCEnvelopeEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds (getWidth() - 51 - 264, 13, 264, 40);
    midiTable->setBounds ((getWidth() / 2) - ((getWidth() - 112) / 2), 200, getWidth() - 112, 121);
    useTableButton->setBounds ((59 + 0) + 0, 110, 128, 32);
    controllerNumberSlider->setBounds (59, 17, 128, 48);
    smoothingSlider->setBounds (59 + 128 - -16, 17, 128, 48);
    learnButton->setBounds (59 + 0, 71, 128, 32);
    defaultSlider->setBounds ((59 + 128 - -16) + 128 - -15, 17, 128, 48);
    decayTimeSlider->setBounds ((59 + 128 - -16) + 128 - -277, 144, 128, 48);
    holdTimeSlider->setBounds ((59 + 128 - -16) + 128 - -5, 144, 128, 48);
    display->setBounds (((59 + 128 - -16) + 128 - -15) + 128 - 274, 80, getWidth() - 364, 56);
    fixedNoteOffButton->setBounds ((59 + 0) + 0, 152, 128, 32);
    startLevelSlider->setBounds ((59 + 128 - -16) + 128 - 131, 144, 128, 48);
    endLevelSlider->setBounds ((59 + 128 - -16) + 128 - -141, 144, 128, 48);
    //[UserResized] Add your own custom resize handling here..

	if (!tableUsed) midiTable->setTopLeftPosition(0, getHeight());

    //[/UserResized]
}

void CCEnvelopeEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == useTableButton)
    {
        //[UserButtonCode_useTableButton] -- add your button handler code here..

		tableUsed = useTableButton->getToggleState();

		getProcessor()->setAttribute(CCEnvelope::UseTable, tableUsed ? 1.0f : 0.0f, dontSendNotification);

		refreshBodySize();

        //[/UserButtonCode_useTableButton]
    }
    else if (buttonThatWasClicked == learnButton)
    {
        //[UserButtonCode_learnButton] -- add your button handler code here..
		dynamic_cast<CCEnvelope*>(getProcessor())->enableLearnMode();
        //[/UserButtonCode_learnButton]
    }
    else if (buttonThatWasClicked == fixedNoteOffButton)
    {
        //[UserButtonCode_fixedNoteOffButton] -- add your button handler code here..
        //[/UserButtonCode_fixedNoteOffButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void CCEnvelopeEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == controllerNumberSlider)
    {
        //[UserSliderCode_controllerNumberSlider] -- add your slider handling code here..
        //[/UserSliderCode_controllerNumberSlider]
    }
    else if (sliderThatWasMoved == smoothingSlider)
    {
        //[UserSliderCode_smoothingSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothingSlider]
    }
    else if (sliderThatWasMoved == defaultSlider)
    {
        //[UserSliderCode_defaultSlider] -- add your slider handling code here..
        //[/UserSliderCode_defaultSlider]
    }
    else if (sliderThatWasMoved == decayTimeSlider)
    {
        //[UserSliderCode_decayTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_decayTimeSlider]
    }
    else if (sliderThatWasMoved == holdTimeSlider)
    {
        //[UserSliderCode_holdTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_holdTimeSlider]
    }
    else if (sliderThatWasMoved == startLevelSlider)
    {
        //[UserSliderCode_startLevelSlider] -- add your slider handling code here..
        //[/UserSliderCode_startLevelSlider]
    }
    else if (sliderThatWasMoved == endLevelSlider)
    {
        //[UserSliderCode_endLevelSlider] -- add your slider handling code here..
        //[/UserSliderCode_endLevelSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...


void CCEnvelopeDisplay::timerCallback()
{

	const float thisHold = processor->getAttribute(CCEnvelope::HoldTime);
	const float thisDecay = processor->getAttribute(CCEnvelope::DecayTime);
	const float thisValue = processor->getCurrentCCValue();

	const float thisStartLevel = CONSTRAIN_TO_0_1((1.0f - Decibels::decibelsToGain(processor->getAttribute(CCEnvelope::StartLevel))));
	const float thisEndLevel = CONSTRAIN_TO_0_1(1.0f - Decibels::decibelsToGain(processor->getAttribute(CCEnvelope::EndLevel)));

	const float thisRulerIndex = processor->getDisplayValues().inR;

	if (thisHold != hold || thisDecay != decay || thisValue != targetValue || thisRulerIndex != currentRulerValue || thisStartLevel != startLevel || thisEndLevel != endLevel)
	{
		hold = thisHold;
		decay = thisDecay;
		targetValue = thisValue;
		currentRulerValue = thisRulerIndex;
		startLevel = thisStartLevel;
		endLevel = thisEndLevel;

		repaint();
	}
}


void CCEnvelopeDisplay::paint(Graphics &g)
{

	const float sum = hold + decay;

	const float w = (float)getWidth();
	const float h = (float)getHeight();

	const float targetHeight = (1.0f - targetValue) * h;

	Path p;

	if (sum == 0.0f)
	{
		p.startNewSubPath(0.0f, targetHeight);

		p.lineTo(w, targetHeight);
		p.lineTo(w, h);
		p.lineTo(0.0f, h);

	}
	else
	{
		const float holdRatio = CONSTRAIN_TO_0_1(hold / sum);

        p.startNewSubPath(0.0f, startLevel*h);

		p.lineTo(holdRatio * w, endLevel*h);

		p.lineTo(w, targetHeight);

		p.lineTo(w, h);

		p.lineTo(0, h);

	}


	p.closeSubPath();

	KnobLookAndFeel::fillPathHiStyle(g, p, getWidth(), getHeight(), true);

	if (sum != 0.0f)
	{
		g.setColour(Colours::lightgrey.withAlpha(0.05f));
		g.fillRect(jmax(0.0f, currentRulerValue * (float)getWidth() - 5.0f), 0.0f, currentRulerValue == 0.0f ? 5.0f : 10.0f, (float)getHeight());
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawLine(Line<float>(currentRulerValue * (float)getWidth(), 0.0f, currentRulerValue * (float)getWidth(), (float)getHeight()), 0.5f);
	}
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="CCEnvelopeEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="BetterProcessorEditor *pe"
                 variableInitialisers="ProcessorEditorBody(pe)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="340">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="51Rr 13 264 40" textCol="3fffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="cc envelope" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="24" bold="1" italic="0" justification="34"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="midiTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 200 112M 121"
                    class="TableEditor" params="static_cast&lt;CCEnvelope*&gt;(getProcessor())-&gt;getTable()"/>
  <TOGGLEBUTTON name="new toggle button" id="e77edc03c117de85" memberName="useTableButton"
                virtualName="" explicitFocusOrder="0" pos="0 110 128 32" posRelativeX="7755a3174f755061"
                tooltip="Use a look up table to calculate the modulation value."
                txtcol="ffffffff" buttonText="UseTable" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <SLIDER name="CC Nr." id="6462473d097e8027" memberName="controllerNumberSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="59 17 128 48"
          tooltip="The CC number" min="1" max="128" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="30"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Smoothing" id="9df027c7b43b6807" memberName="smoothingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-16R 17 128 48"
          posRelativeX="6462473d097e8027" tooltip="Smoothing Value" min="0"
          max="2000" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="new toggle button" id="7755a3174f755061" memberName="learnButton"
                virtualName="" explicitFocusOrder="0" pos="0 71 128 32" posRelativeX="6462473d097e8027"
                txtcol="ffffffff" buttonText="MIDI Learn" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Default" id="5e7b9c6b5331d044" memberName="defaultSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-15R 17 128 48"
          posRelativeX="9df027c7b43b6807" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Default" id="2b2dd3bd55c7978" memberName="decayTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-277R 144 128 48"
          posRelativeX="9df027c7b43b6807" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Default" id="3fd364be26295a41" memberName="holdTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-5R 144 128 48"
          posRelativeX="9df027c7b43b6807" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <GENERICCOMPONENT name="new component" id="506d36f65b94c9ab" memberName="display"
                    virtualName="" explicitFocusOrder="0" pos="274R 80 364M 56" posRelativeX="5e7b9c6b5331d044"
                    class="CCEnvelopeDisplay" params="dynamic_cast&lt;CCEnvelope*&gt;(getProcessor())"/>
  <TOGGLEBUTTON name="new toggle button" id="bf22557198cb7ec" memberName="fixedNoteOffButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="0 152 128 32"
                posRelativeX="7755a3174f755061" tooltip="Don't alter the output after the note off"
                txtcol="ffffffff" buttonText="Fixed Note Off" connectedEdges="0"
                needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Default" id="974176989277ed73" memberName="startLevelSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="131R 144 128 48"
          posRelativeX="9df027c7b43b6807" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Default" id="13587b7aed3de4f3" memberName="endLevelSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-141R 144 128 48"
          posRelativeX="9df027c7b43b6807" tooltip="Smoothing Value" min="0"
          max="127" int="0" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
