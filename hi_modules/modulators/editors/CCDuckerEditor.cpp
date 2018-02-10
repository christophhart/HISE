/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "CCDuckerEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
CCDuckerEditor::CCDuckerEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (ccSlider = new HiSlider ("CC Number"));
    ccSlider->setRange (1, 127, 1);
    ccSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    ccSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    ccSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    ccSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    ccSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    ccSlider->addListener (this);
    ccSlider->setSkewFactor (0.3);

    addAndMakeVisible (duckingTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), dynamic_cast<LookupTableProcessor*>(getProcessor())->getTable(0)));
    duckingTable->setName ("new component");

    addAndMakeVisible (timeSlider = new HiSlider ("Ducking Time"));
    timeSlider->setRange (1, 20000, 1);
    timeSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    timeSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    timeSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    timeSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    timeSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    timeSlider->addListener (this);
    timeSlider->setSkewFactor (0.3);

    addAndMakeVisible (smoothingSlider = new HiSlider ("Smoothing Slider"));
    smoothingSlider->setRange (1, 1000, 1);
    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    smoothingSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    smoothingSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    smoothingSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    smoothingSlider->addListener (this);
    smoothingSlider->setSkewFactor (0.3);


    //[UserPreSize]

	duckingTable->connectToLookupTableProcessor(getProcessor());

	ccSlider->setup(getProcessor(), CCDucker::CCNumber, "CC Number");
	ccSlider->setMode(HiSlider::Mode::Discrete, 0, 128);

	smoothingSlider->setup(getProcessor(), CCDucker::SmoothingTime, "Smoothing Time");
	smoothingSlider->setMode(HiSlider::Time, 0.0, 1000.0, 100.0);

	timeSlider->setup(getProcessor(), CCDucker::DuckingTime, "Ducking Time");
	timeSlider->setMode(HiSlider::Time);

        
    //[/UserPreSize]

    setSize (850, 200);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

CCDuckerEditor::~CCDuckerEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    ccSlider = nullptr;
    duckingTable = nullptr;
    timeSlider = nullptr;
    smoothingSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CCDuckerEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - (600 / 2)), 6.0f, 600.0f, static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - (600 / 2)), 6.0f, 600.0f, static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void CCDuckerEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    ccSlider->setBounds (((getWidth() / 2) - (340 / 2)) + 340 / 2 + -202 - (160 / 2), 144, 160, 48);
    duckingTable->setBounds ((getWidth() / 2) - (340 / 2), 16, 340, 120);
    timeSlider->setBounds (((getWidth() / 2) - (340 / 2)) + 340 / 2 - (160 / 2), 144, 160, 48);
    smoothingSlider->setBounds (((getWidth() / 2) - (340 / 2)) + 340 / 2 + 196 - (160 / 2), 144, 160, 48);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void CCDuckerEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == ccSlider)
    {
        //[UserSliderCode_ccSlider] -- add your slider handling code here..
        //[/UserSliderCode_ccSlider]
    }
    else if (sliderThatWasMoved == timeSlider)
    {
        //[UserSliderCode_timeSlider] -- add your slider handling code here..
        //[/UserSliderCode_timeSlider]
    }
    else if (sliderThatWasMoved == smoothingSlider)
    {
        //[UserSliderCode_smoothingSlider] -- add your slider handling code here..
        //[/UserSliderCode_smoothingSlider]
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

<JUCER_COMPONENT documentType="Component" className="CCDuckerEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="850" initialHeight="200">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 600 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <SLIDER name="CC Number" id="9ef32c38be6d2f66" memberName="ccSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="-202Cc 144 160 48"
          posRelativeX="e2252e55bedecdc5" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="127" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="duckingTable"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 16 340 120" class="TableEditor"
                    params="dynamic_cast&lt;LookupTableProcessor*&gt;(getProcessor())-&gt;getTable(0)"/>
  <SLIDER name="Ducking Time" id="cd5ba0ab4ad6dcab" memberName="timeSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 144 160 48"
          posRelativeX="e2252e55bedecdc5" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Smoothing Slider" id="a8603f7ecdf9c8ff" memberName="smoothingSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="196Cc 144 160 48"
          posRelativeX="e2252e55bedecdc5" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="1000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
