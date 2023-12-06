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

#include "DelayEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
DelayEditor::DelayEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p),
    updater(*this)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (leftTimeSlider = new HiSlider ("Left Time"));
    leftTimeSlider->setRange (0, 3000, 1);
    leftTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    leftTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    leftTimeSlider->addListener (this);

    addAndMakeVisible (rightTimeSlider = new HiSlider ("Right Time"));
    rightTimeSlider->setRange (0, 3000, 1);
    rightTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rightTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rightTimeSlider->addListener (this);

    addAndMakeVisible (leftFeedbackSlider = new HiSlider ("Left Feedback"));
    leftFeedbackSlider->setRange (0, 100, 1);
    leftFeedbackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    leftFeedbackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    leftFeedbackSlider->addListener (this);

    addAndMakeVisible (rightFeedbackSlider = new HiSlider ("Right Feedback"));
    rightFeedbackSlider->setRange (0, 100, 1);
    rightFeedbackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rightFeedbackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rightFeedbackSlider->addListener (this);

    addAndMakeVisible (mixSlider = new HiSlider ("Mix"));
    mixSlider->setRange (0, 100, 1);
    mixSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    mixSlider->addListener (this);

    addAndMakeVisible (tempoSyncButton = new HiToggleButton ("new toggle button"));
    tempoSyncButton->setButtonText (TRANS("TempoSync"));
    tempoSyncButton->addListener (this);
    tempoSyncButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	tempoSyncButton->setup(getProcessor(), DelayEffect::TempoSync, "TempoSync");
	tempoSyncButton->setNotificationType(sendNotification);

	leftTimeSlider->setup(getProcessor(), DelayEffect::DelayTimeLeft, "Left Delay");
	leftTimeSlider->setMode(HiSlider::Time);

	rightTimeSlider->setup(getProcessor(), DelayEffect::DelayTimeRight, "Right Delay");
	rightTimeSlider->setMode(HiSlider::Time);


	leftFeedbackSlider->setup(getProcessor(), DelayEffect::FeedbackLeft, "Left Feedback");
	leftFeedbackSlider->setMode(HiSlider::NormalizedPercentage);

	rightFeedbackSlider->setup(getProcessor(), DelayEffect::FeedbackRight, "Right Feedback");
	rightFeedbackSlider->setMode(HiSlider::NormalizedPercentage);

	mixSlider->setup(getProcessor(), DelayEffect::Mix, "Mix");
	mixSlider->setMode(HiSlider::NormalizedPercentage);



    //[/UserPreSize]

    setSize (900, 170);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

DelayEditor::~DelayEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    leftTimeSlider = nullptr;
    rightTimeSlider = nullptr;
    leftFeedbackSlider = nullptr;
    rightFeedbackSlider = nullptr;
    mixSlider = nullptr;
    tempoSyncButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void DelayEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);
    g.setColour(Colour(0xAAFFFFFF));
    g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
    
    g.drawText (TRANS("delay"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void DelayEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    leftTimeSlider->setBounds ((getWidth() / 2) + -145 - 128, 32, 128, 48);
    rightTimeSlider->setBounds ((getWidth() / 2) + 6 - 128, 32, 128, 48);
    leftFeedbackSlider->setBounds ((getWidth() / 2) + -145 - 128, 96, 128, 48);
    rightFeedbackSlider->setBounds ((getWidth() / 2) + 6 - 128, 96, 128, 48);
    mixSlider->setBounds ((getWidth() / 2) + 102 - (128 / 2), 32, 128, 48);
    tempoSyncButton->setBounds ((getWidth() / 2) + 102 - (128 / 2), 96, 128, 32);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void DelayEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == leftTimeSlider)
    {
        //[UserSliderCode_leftTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_leftTimeSlider]
    }
    else if (sliderThatWasMoved == rightTimeSlider)
    {
        //[UserSliderCode_rightTimeSlider] -- add your slider handling code here..
        //[/UserSliderCode_rightTimeSlider]
    }
    else if (sliderThatWasMoved == leftFeedbackSlider)
    {
        //[UserSliderCode_leftFeedbackSlider] -- add your slider handling code here..
        //[/UserSliderCode_leftFeedbackSlider]
    }
    else if (sliderThatWasMoved == rightFeedbackSlider)
    {
        //[UserSliderCode_rightFeedbackSlider] -- add your slider handling code here..
        //[/UserSliderCode_rightFeedbackSlider]
    }
    else if (sliderThatWasMoved == mixSlider)
    {
        //[UserSliderCode_mixSlider] -- add your slider handling code here..
        //[/UserSliderCode_mixSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void DelayEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == tempoSyncButton)
    {
        //[UserButtonCode_tempoSyncButton] -- add your button handler code here..

		


        //[/UserButtonCode_tempoSyncButton]
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

<JUCER_COMPONENT documentType="Component" className="DelayEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="170">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="delay"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <SLIDER name="Left Time" id="89cc5b4c20e221e" memberName="leftTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-145Cr 32 128 48"
          posRelativeX="f930000f86c6c8b6" min="0" max="3000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Right Time" id="ae1646635cbce8fa" memberName="rightTimeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="6Cr 32 128 48"
          posRelativeX="f930000f86c6c8b6" min="0" max="3000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Left Feedback" id="cc35747a4515e5ae" memberName="leftFeedbackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-145Cr 96 128 48"
          posRelativeX="f930000f86c6c8b6" min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Right Feedback" id="c6f6406fdd87fc89" memberName="rightFeedbackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="6Cr 96 128 48"
          posRelativeX="f930000f86c6c8b6" min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Mix" id="9115805b4b27f781" memberName="mixSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="102Cc 32 128 48" posRelativeX="f930000f86c6c8b6"
          min="0" max="100" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="tempoSyncButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="102Cc 96 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="TempoSync"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
