/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "TransposerEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TransposerEditor::TransposerEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (intensitySlider = new HiSlider ("Transpose"));
    intensitySlider->setRange (-24, 24, 1);
    intensitySlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    intensitySlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    intensitySlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    intensitySlider->setColour (Slider::textBoxTextColourId, Colours::white);
    intensitySlider->addListener (this);


    //[UserPreSize]

	intensitySlider->setup(getProcessor(), Transposer::TransposeAmount, "Transpose");

	intensitySlider->setMode(HiSlider::Discrete, -24.0, 24.0, 0.0, 1.0);
	intensitySlider->setTextValueSuffix(" st");	

    //[/UserPreSize]

    setSize (800, 40);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

TransposerEditor::~TransposerEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    intensitySlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TransposerEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TransposerEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    intensitySlider->setBounds ((getWidth() / 2) - (128 / 2), 0, 128, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TransposerEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == intensitySlider)
    {
        //[UserSliderCode_intensitySlider] -- add your slider handling code here..
		static_cast<Transposer*>(getProcessor())->setAttribute(Transposer::TransposeAmount, (float)(int)intensitySlider->getValue(), dontSendNotification);
        //[/UserSliderCode_intensitySlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TransposerEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="40">
  <BACKGROUND backgroundColour="ffffff"/>
  <SLIDER name="Transpose" id="9ef32c38be6d2f66" memberName="intensitySlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 0 128 48"
          thumbcol="80666666" textboxtext="ffffffff" min="-24" max="24"
          int="1" style="RotaryHorizontalVerticalDrag" textBoxPos="TextBoxRight"
          textBoxEditable="0" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...

ChokeGroupEditor::ChokeGroupEditor(ProcessorEditor *p) : ProcessorEditorBody(p)
{
	addAndMakeVisible(groupSlider = new HiSlider("Transpose"));
	groupSlider->setRange(-24, 24, 1);
	groupSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	groupSlider->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
	groupSlider->setColour(Slider::thumbColourId, Colour(0x80666666));
	groupSlider->setColour(Slider::textBoxTextColourId, Colours::white);
	groupSlider->setup(getProcessor(), ChokeGroupProcessor::SpecialParameters::ChokeGroup, "ChokeGroup");
	groupSlider->setMode(HiSlider::Discrete, 0, 16.0, DBL_MAX, 1.0);

	addAndMakeVisible(loSlider = new HiSlider("LoKey"));
	loSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	loSlider->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
	loSlider->setColour(Slider::thumbColourId, Colour(0x80666666));
	loSlider->setColour(Slider::textBoxTextColourId, Colours::white);
	loSlider->setup(getProcessor(), ChokeGroupProcessor::SpecialParameters::LoKey, "LoKey");
	loSlider->setMode(HiSlider::Discrete, 0, 127, DBL_MAX, 1.0);

	addAndMakeVisible(hiSlider = new HiSlider("HiKey"));
	hiSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	hiSlider->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
	hiSlider->setColour(Slider::thumbColourId, Colour(0x80666666));
	hiSlider->setColour(Slider::textBoxTextColourId, Colours::white);
	hiSlider->setup(getProcessor(), ChokeGroupProcessor::SpecialParameters::HiKey, "HiKey");
	hiSlider->setMode(HiSlider::Discrete, 0, 127, DBL_MAX, 1.0);

	addAndMakeVisible(killButton = new HiToggleButton("KillVoices"));
	killButton->setup(getProcessor(), ChokeGroupProcessor::SpecialParameters::KillVoice, "KillVoice");
	
	setSize(800, 40);
}

void ChokeGroupEditor::resized()
{
	auto b = getLocalBounds().withSizeKeepingCentre((128 + 10) * 4, 48);

	groupSlider->setBounds(b.removeFromLeft(128)); b.removeFromLeft(10);
	loSlider->setBounds(b.removeFromLeft(128)); b.removeFromLeft(10);
	hiSlider->setBounds(b.removeFromLeft(128)); b.removeFromLeft(10);
	killButton->setBounds(b.removeFromLeft(128).reduced(0, 8)); b.removeFromLeft(10);
}

} // namespace hise

//[/EndFile]
