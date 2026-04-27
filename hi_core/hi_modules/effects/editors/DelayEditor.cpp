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
    leftTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    leftTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    leftTimeSlider->addListener (this);

    addAndMakeVisible (rightTimeSlider = new HiSlider ("Right Time"));
    rightTimeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rightTimeSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rightTimeSlider->addListener (this);

    addAndMakeVisible (leftFeedbackSlider = new HiSlider ("Left Feedback"));
    leftFeedbackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    leftFeedbackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    leftFeedbackSlider->addListener (this);

    addAndMakeVisible (rightFeedbackSlider = new HiSlider ("Right Feedback"));
    rightFeedbackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    rightFeedbackSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    rightFeedbackSlider->addListener (this);

    addAndMakeVisible (mixSlider = new HiSlider ("Mix"));
    mixSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    mixSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    mixSlider->addListener (this);

    addAndMakeVisible (tempoSyncButton = new HiToggleButton ("new toggle button"));
    tempoSyncButton->setButtonText (TRANS("TempoSync"));
    tempoSyncButton->addListener (this);
    tempoSyncButton->setColour (ToggleButton::textColourId, Colours::white);


    //[UserPreSize]

	auto md = getProcessor()->getMetadata();
	md.setup(*tempoSyncButton, getProcessor(), DelayEffect::TempoSync);
	tempoSyncButton->setNotificationType(sendNotification);

	md.setup(*leftTimeSlider, getProcessor(), DelayEffect::DelayTimeLeft);
	md.setup(*rightTimeSlider, getProcessor(), DelayEffect::DelayTimeRight);
	md.setup(*leftFeedbackSlider, getProcessor(), DelayEffect::FeedbackLeft);
	md.setup(*rightFeedbackSlider, getProcessor(), DelayEffect::FeedbackRight);
	md.setup(*mixSlider, getProcessor(), DelayEffect::Mix);

    setSize (900, 170);
	h = getHeight();
}

DelayEditor::~DelayEditor()
{
    leftTimeSlider = nullptr;
    rightTimeSlider = nullptr;
    leftFeedbackSlider = nullptr;
    rightFeedbackSlider = nullptr;
    mixSlider = nullptr;
    tempoSyncButton = nullptr;
}

//==============================================================================
void DelayEditor::paint (Graphics& g)
{
    ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);
    g.setColour(Colour(0xAAFFFFFF));
    g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
    
    g.drawText (TRANS("delay"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);
}

void DelayEditor::resized()
{
    leftTimeSlider->setBounds ((getWidth() / 2) + -145 - 128, 32, 128, 48);
    rightTimeSlider->setBounds ((getWidth() / 2) + 6 - 128, 32, 128, 48);
    leftFeedbackSlider->setBounds ((getWidth() / 2) + -145 - 128, 96, 128, 48);
    rightFeedbackSlider->setBounds ((getWidth() / 2) + 6 - 128, 96, 128, 48);
    mixSlider->setBounds ((getWidth() / 2) + 102 - (128 / 2), 32, 128, 48);
    tempoSyncButton->setBounds ((getWidth() / 2) + 102 - (128 / 2), 96, 128, 32);
}

void DelayEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    
}

void DelayEditor::buttonClicked (Button* buttonThatWasClicked)
{
   
}



} // namespace hise
