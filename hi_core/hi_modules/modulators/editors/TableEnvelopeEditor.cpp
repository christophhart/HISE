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

#include "TableEnvelopeEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TableEnvelopeEditorBody::TableEnvelopeEditorBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (attackSlider = new HiSlider ("Attack Time"));
    attackSlider->setRange (1, 20000, 1);
    attackSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    attackSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    attackSlider->setColour (Slider::backgroundColourId, Colour (0x00000000));
    attackSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    attackSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    attackSlider->addListener (this);
    attackSlider->setSkewFactor (0.3);

    addAndMakeVisible (releaseSlider = new HiSlider ("Release Time"));
    releaseSlider->setRange (3, 20000, 1);
    releaseSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    releaseSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
    releaseSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
    releaseSlider->setColour (Slider::textBoxTextColourId, Colours::white);
    releaseSlider->addListener (this);
    releaseSlider->setSkewFactor (0.3);

    addAndMakeVisible (attackTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<TableEnvelope*>(getProcessor())->getTable(0)));
    attackTable->setName ("new component");

    addAndMakeVisible (releaseTable = new TableEditor (getProcessor()->getMainController()->getControlUndoManager(), static_cast<TableEnvelope*>(getProcessor())->getTable(1)));
    releaseTable->setName ("new component");


    //[UserPreSize]

	attackSlider->setup(getProcessor(), SimpleEnvelope::Attack, "Attack Time");
	attackSlider->setMode(HiSlider::Time, 1.0, 20000.0, 2000.0);

	releaseSlider->setup(getProcessor(), SimpleEnvelope::Release, "Release Time");
	releaseSlider->setMode(HiSlider::Time, 1.0, 20000.0, 2000.0);

	attackSlider->setIsUsingModulatedRing(true);
	releaseSlider->setIsUsingModulatedRing(true);

    ProcessorHelpers::connectTableEditor(*attackTable, getProcessor(), 0);
    ProcessorHelpers::connectTableEditor(*releaseTable, getProcessor(), 1);
    
    //[/UserPreSize]

    setSize (800, 200);


    //[Constructor] You can add your own custom stuff here..

	startTimer(30);

	h = getHeight();


    //[/Constructor]
}

TableEnvelopeEditorBody::~TableEnvelopeEditorBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..


    //[/Destructor_pre]

    attackSlider = nullptr;
    releaseSlider = nullptr;
    attackTable = nullptr;
    releaseTable = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TableEnvelopeEditorBody::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (32.0f, 6.0f, static_cast<float> (proportionOfWidth (0.4463f)), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (32.0f, 6.0f, static_cast<float> (proportionOfWidth (0.4463f)), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> (getWidth() - 32 - proportionOfWidth (0.4463f)), 6.0f, static_cast<float> (proportionOfWidth (0.4463f)), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x23ffffff));
    g.drawRoundedRectangle (static_cast<float> (getWidth() - 32 - proportionOfWidth (0.4463f)), 6.0f, static_cast<float> (proportionOfWidth (0.4463f)), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TableEnvelopeEditorBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    attackSlider->setBounds (51 + proportionOfWidth (0.4000f) / 2 - (160 / 2), 142, 160, 48);
    releaseSlider->setBounds ((getWidth() - 51 - proportionOfWidth (0.4000f)) + proportionOfWidth (0.4000f) / 2 - (160 / 2), 142, 160, 48);
    attackTable->setBounds (51, 17, proportionOfWidth (0.4000f), 120);
    releaseTable->setBounds (getWidth() - 51 - proportionOfWidth (0.4000f), 16, proportionOfWidth (0.4000f), 120);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TableEnvelopeEditorBody::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == attackSlider)
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



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TableEnvelopeEditorBody"
                 componentName="" parentClasses="public ProcessorEditorBody, public Timer"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="200">
  <BACKGROUND backgroundColour="291538">
    <ROUNDRECT pos="32 6 44.625% 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <ROUNDRECT pos="32Rr 6 44.625% 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 23ffffff"/>
  </BACKGROUND>
  <SLIDER name="Attack Time" id="9ef32c38be6d2f66" memberName="attackSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 142 160 48"
          posRelativeX="e2252e55bedecdc5" bkgcol="0" thumbcol="80666666"
          textboxtext="ffffffff" min="1" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <SLIDER name="Release Time" id="b3d59ac44c48ffc2" memberName="releaseSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="0Cc 142 160 48"
          posRelativeX="b388001c00dd7bae" thumbcol="80666666" textboxtext="ffffffff"
          min="3" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="0.29999999999999999"/>
  <GENERICCOMPONENT name="new component" id="e2252e55bedecdc5" memberName="attackTable"
                    virtualName="" explicitFocusOrder="0" pos="51 17 40% 120" class="TableEditor"
                    params="static_cast&lt;TableEnvelope*&gt;(getProcessor())-&gt;getTable(TableEnvelope::Attack)"/>
  <GENERICCOMPONENT name="new component" id="b388001c00dd7bae" memberName="releaseTable"
                    virtualName="" explicitFocusOrder="0" pos="51Rr 16 40% 120" class="TableEditor"
                    params="static_cast&lt;TableEnvelope*&gt;(getProcessor())-&gt;getTable(TableEnvelope::Release)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
