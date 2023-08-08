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

#include "FileImportDialog.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
FileImportDialog::FileImportDialog (Processor *p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (metadataButton = new ToggleButton ("new toggle button"));
    metadataButton->setButtonText (TRANS("Extract Metadata for mapping"));
    metadataButton->addListener (this);
    metadataButton->setToggleState (true, dontSendNotification);
    metadataButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (fileNameButton = new ToggleButton ("new toggle button"));
    fileNameButton->setButtonText (TRANS("Filename Token Parser"));
    fileNameButton->setRadioGroupId (1);
    fileNameButton->addListener (this);
    fileNameButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (pitchDetectionButton = new ToggleButton ("new toggle button"));
    pitchDetectionButton->setButtonText (TRANS("Root Note Pitch Detection"));
    pitchDetectionButton->setRadioGroupId (1);
    pitchDetectionButton->addListener (this);
    pitchDetectionButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (dropPointButton = new ToggleButton ("new toggle button"));
    dropPointButton->setButtonText (TRANS("Drop Point"));
    dropPointButton->setRadioGroupId (1);
    dropPointButton->addListener (this);
    dropPointButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (descriptionLabel = new Label ("new label",
                                                     TRANS("Automap Type")));
    descriptionLabel->setFont (GLOBAL_BOLD_FONT());
    descriptionLabel->setJustificationType (Justification::centred);
    descriptionLabel->setEditable (false, false, false);
    descriptionLabel->setColour (Label::textColourId, Colours::white);
    descriptionLabel->setColour (TextEditor::textColourId, Colours::black);
    descriptionLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (descriptionLabel2 = new Label ("new label",
                                                      TRANS("Extract Metadata")));
    descriptionLabel2->setFont (GLOBAL_BOLD_FONT());
    descriptionLabel2->setJustificationType (Justification::centred);
    descriptionLabel2->setEditable (false, false, false);
    descriptionLabel2->setColour (Label::textColourId, Colours::white);
    descriptionLabel2->setColour (TextEditor::textColourId, Colours::black);
    descriptionLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

	setLookAndFeel(&laf);

	descriptionLabel->setFont (GLOBAL_BOLD_FONT());
	descriptionLabel2->setFont (GLOBAL_BOLD_FONT());

	p->getMainController()->skin(*metadataButton);
	p->getMainController()->skin(*pitchDetectionButton);
	p->getMainController()->skin(*dropPointButton);
	p->getMainController()->skin(*fileNameButton);

	// Prevent the return key to activate any buttons
	for (int i = 0; i < getNumChildComponents(); i++)
		getChildComponent(i)->setWantsKeyboardFocus(false);

    //[/UserPreSize]

    setSize (500, 210);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

FileImportDialog::~FileImportDialog()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    metadataButton = nullptr;
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

    metadataButton->setBounds (136, 160, 256, 32);
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

    if (buttonThatWasClicked == metadataButton)
    {
        //[UserButtonCode_velocityButton] -- add your button handler code here..
		useMetadataForMapping = metadataButton->getToggleState();
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


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
