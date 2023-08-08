/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "SfzGroupSelectorComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SfzGroupSelectorComponent::SfzGroupSelectorComponent ()
{
    addAndMakeVisible (rrGroupSelector = new ComboBox ("new combo box"));
    rrGroupSelector->setEditableText (false);
    rrGroupSelector->setJustificationType (Justification::centredLeft);
    rrGroupSelector->setTextWhenNothingSelected (String());
    rrGroupSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    rrGroupSelector->addListener (this);

    addAndMakeVisible (GroupName = new Label ("new label",
                                              TRANS("GroupName")));
    GroupName->setFont (Font ("Khmer UI", 15.00f, Font::plain));
    GroupName->setJustificationType (Justification::centredLeft);
    GroupName->setEditable (false, false, false);
    GroupName->setColour (Label::backgroundColourId, Colour (0x23ffffff));
    GroupName->setColour (Label::textColourId, Colours::white);
    GroupName->setColour (Label::outlineColourId, Colour (0x4effffff));
    GroupName->setColour (TextEditor::textColourId, Colours::black);
    GroupName->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	GroupName->setFont (GLOBAL_FONT());

	fixedReturnValue = false;

    //[/UserPreSize]

    setSize (400, 30);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

SfzGroupSelectorComponent::~SfzGroupSelectorComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    rrGroupSelector = nullptr;
    GroupName = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SfzGroupSelectorComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff474747));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SfzGroupSelectorComponent::resized()
{
    rrGroupSelector->setBounds (getWidth() - 3 - 72, 3, 72, 24);
    GroupName->setBounds (3, 3, getWidth() - 83, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SfzGroupSelectorComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == rrGroupSelector)
    {
        //[UserComboBoxCode_rrGroupSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_rrGroupSelector]
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

<JUCER_COMPONENT documentType="Component" className="SfzGroupSelectorComponent"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="1" initialWidth="400" initialHeight="30">
  <BACKGROUND backgroundColour="ff474747"/>
  <COMBOBOX name="new combo box" id="ec6a9eff46d60dd5" memberName="rrGroupSelector"
            virtualName="" explicitFocusOrder="0" pos="3Rr 3 72 24" editable="0"
            layout="33" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="f10fb70e52abe08" memberName="GroupName"
         virtualName="" explicitFocusOrder="0" pos="3 3 83M 24" bkgCol="23ffffff"
         textCol="ffffffff" outlineCol="4effffff" edTextCol="ff000000"
         edBkgCol="0" labelText="GroupName" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="15" bold="0"
         italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
