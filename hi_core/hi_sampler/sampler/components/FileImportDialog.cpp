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
//[/Headers]

#include "FileImportDialog.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
FileImportDialog::FileImportDialog (Processor *p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (velocityButton = new ToggleButton ("new toggle button"));
    velocityButton->setButtonText (TRANS("Automap velocity based on volume"));
    velocityButton->addListener (this);
    velocityButton->setToggleState (true, dontSendNotification);
    velocityButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (fileNameButton = new ToggleButton ("new toggle button"));
    fileNameButton->setButtonText (TRANS("Automap root based on file name"));
    fileNameButton->setRadioGroupId (1);
    fileNameButton->addListener (this);
    fileNameButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (pitchDetectionButton = new ToggleButton ("new toggle button"));
    pitchDetectionButton->setButtonText (TRANS("Automap root based on pitch detection"));
    pitchDetectionButton->setRadioGroupId (1);
    pitchDetectionButton->addListener (this);
    pitchDetectionButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (dropPointButton = new ToggleButton ("new toggle button"));
    dropPointButton->setButtonText (TRANS("Automap root based on drop point"));
    dropPointButton->setRadioGroupId (1);
    dropPointButton->addListener (this);
    dropPointButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (descriptionLabel = new Label ("new label",
                                                     TRANS("Note automap")));
    descriptionLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    descriptionLabel->setJustificationType (Justification::centred);
    descriptionLabel->setEditable (false, false, false);
    descriptionLabel->setColour (Label::textColourId, Colours::white);
    descriptionLabel->setColour (TextEditor::textColourId, Colours::black);
    descriptionLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (descriptionLabel2 = new Label ("new label",
                                                      TRANS("Velocity Automap")));
    descriptionLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    descriptionLabel2->setJustificationType (Justification::centred);
    descriptionLabel2->setEditable (false, false, false);
    descriptionLabel2->setColour (Label::textColourId, Colours::white);
    descriptionLabel2->setColour (TextEditor::textColourId, Colours::black);
    descriptionLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	setLookAndFeel(&laf);

	descriptionLabel->setFont (GLOBAL_FONT());
	descriptionLabel2->setFont (GLOBAL_FONT());

	p->getMainController()->skin(*velocityButton);
	p->getMainController()->skin(*pitchDetectionButton);
	p->getMainController()->skin(*dropPointButton);
	p->getMainController()->skin(*fileNameButton);


    //[/UserPreSize]

    setSize (500, 210);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

FileImportDialog::~FileImportDialog()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    velocityButton = nullptr;
    fileNameButton = nullptr;
    pitchDetectionButton = nullptr;
    dropPointButton = nullptr;
    descriptionLabel = nullptr;
    descriptionLabel2 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void FileImportDialog::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void FileImportDialog::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    velocityButton->setBounds (136, 160, 256, 32);
    fileNameButton->setBounds (136, 32, 256, 32);
    pitchDetectionButton->setBounds (136, 64, 256, 32);
    dropPointButton->setBounds (136, 96, 256, 32);
    descriptionLabel->setBounds ((getWidth() / 2) - (proportionOfWidth (1.0000f) / 2), 0, proportionOfWidth (1.0000f), 24);
    descriptionLabel2->setBounds ((getWidth() / 2) - (proportionOfWidth (1.0000f) / 2), 130, proportionOfWidth (1.0000f), 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void FileImportDialog::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == velocityButton)
    {
        //[UserButtonCode_velocityButton] -- add your button handler code here..
		autoMapVelocity = velocityButton->getToggleState();
        //[/UserButtonCode_velocityButton]
    }
    else if (buttonThatWasClicked == fileNameButton)
    {
        //[UserButtonCode_fileNameButton] -- add your button handler code here..
		mode = FileName;
        //[/UserButtonCode_fileNameButton]
    }
    else if (buttonThatWasClicked == pitchDetectionButton)
    {
        //[UserButtonCode_pitchDetectionButton] -- add your button handler code here..
		mode = PitchDetection;
        //[/UserButtonCode_pitchDetectionButton]
    }
    else if (buttonThatWasClicked == dropPointButton)
    {
        //[UserButtonCode_dropPointButton] -- add your button handler code here..
		mode = DropPoint;
        //[/UserButtonCode_dropPointButton]
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

<JUCER_COMPONENT documentType="Component" className="FileImportDialog" componentName=""
                 parentClasses="public Component" constructorParams="Processor *p"
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="1" initialWidth="500" initialHeight="210">
  <BACKGROUND backgroundColour="222222"/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="velocityButton"
                virtualName="" explicitFocusOrder="0" pos="136 160 256 32" posRelativeX="410a230ddaa2f2e8"
                txtcol="ffffffff" buttonText="Automap velocity based on volume"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="1"/>
  <TOGGLEBUTTON name="new toggle button" id="2b8888708307f364" memberName="fileNameButton"
                virtualName="" explicitFocusOrder="0" pos="136 32 256 32" posRelativeX="410a230ddaa2f2e8"
                txtcol="ffffffff" buttonText="Automap root based on file name"
                connectedEdges="0" needsCallback="1" radioGroupId="1" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="a454d36c9fd5b599" memberName="pitchDetectionButton"
                virtualName="" explicitFocusOrder="0" pos="136 64 256 32" posRelativeX="410a230ddaa2f2e8"
                txtcol="ffffffff" buttonText="Automap root based on pitch detection"
                connectedEdges="0" needsCallback="1" radioGroupId="1" state="0"/>
  <TOGGLEBUTTON name="new toggle button" id="a2fde6f5c299e4a6" memberName="dropPointButton"
                virtualName="" explicitFocusOrder="0" pos="136 96 256 32" posRelativeX="410a230ddaa2f2e8"
                txtcol="ffffffff" buttonText="Automap root based on drop point"
                connectedEdges="0" needsCallback="1" radioGroupId="1" state="0"/>
  <LABEL name="new label" id="7be2ed43072326c4" memberName="descriptionLabel"
         virtualName="" explicitFocusOrder="0" pos="0Cc 0 100% 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Note automap" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="1df0f7d610f33029" memberName="descriptionLabel2"
         virtualName="" explicitFocusOrder="0" pos="0Cc 130 100% 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Velocity Automap"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" bold="0" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
