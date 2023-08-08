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

#include "RouteFXEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
RouteFXEditor::RouteFXEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (router = new RouterComponent (&dynamic_cast<RoutableProcessor*>(p->getProcessor())->getMatrix()));
    router->setName ("new component");


    //[UserPreSize]
    //[/UserPreSize]

    setSize (800, 180);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

RouteFXEditor::~RouteFXEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    router = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void RouteFXEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - ((getWidth() - 84) / 2)), 6.0f, static_cast<float> (getWidth() - 84), static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    g.setColour (Colour (0x52ffffff));
    g.setFont (Font ("Arial", 24.00f, Font::bold));
    g.drawText (TRANS("routing"),
                getWidth() - 53 - 200, 6, 200, 40,
                Justification::centredRight, true);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void RouteFXEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    router->setBounds ((getWidth() / 2) + -38 - ((getWidth() - 768) / 2), 24, 768, 72);
    //[UserResized] Add your own custom resize handling here..

	router->setBounds((getWidth() / 2) - (640 / 2), 32, 640, 128);

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

<JUCER_COMPONENT documentType="Component" className="RouteFXEditor" componentName=""
                 parentClasses="public ProcessorEditorBody" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="180">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="-0.5Cc 6 84M 12M" cornerSize="6" fill="solid: 30000000"
               hasStroke="1" stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
    <TEXT pos="53Rr 6 200 40" fill="solid: 52ffffff" hasStroke="0" text="routing"
          fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="e05c0b1da9fdff72" memberName="router"
                    virtualName="RouterComponent" explicitFocusOrder="0" pos="-38Cc 24 768M 72"
                    class="Component" params="&amp;dynamic_cast&lt;RoutableProcessor*&gt;(p-&gt;getProcessor())-&gt;getMatrix()"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
