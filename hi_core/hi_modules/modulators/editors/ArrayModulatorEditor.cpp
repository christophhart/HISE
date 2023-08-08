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

#include "ArrayModulatorEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ArrayModulatorEditor::ArrayModulatorEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (sliderPackMix = new SliderPack (dynamic_cast<ArrayModulator*>(getProcessor())->getSliderPack(0)));
    sliderPackMix->setName ("new component");


    //[UserPreSize]
    //[/UserPreSize]

    setSize (900, 150);


    //[Constructor] You can add your own custom stuff here..

	

	h = getHeight();
    //[/Constructor]
}

ArrayModulatorEditor::~ArrayModulatorEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    sliderPackMix = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ArrayModulatorEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

	ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ArrayModulatorEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    sliderPackMix->setBounds ((getWidth() / 2) - (768 / 2), 24, 768, 104);
    //[UserResized] Add your own custom resize handling here..

	const int width = getWidth();

	int w = 0;

	if (width > 768 + 24)
	{
		w = 768;
	}
	else
	{
		w = 512;
	}


	sliderPackMix->setBounds((getWidth() / 2) - (512 / 2), 24, 512, 104);

    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ArrayModulatorEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="150">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="9768772bb12c50e7" memberName="sliderPackMix"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 24 768 104" class="SliderPack"
                    params="dynamic_cast&lt;ArrayModulator*&gt;(getProcessor())-&gt;getSliderPackData()"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
