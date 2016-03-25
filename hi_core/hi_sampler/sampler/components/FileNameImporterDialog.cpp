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

#include "FileNameImporterDialog.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
FileNameImporterDialog::FileNameImporterDialog (ModulatorSampler *sampler_, const StringArray &fileNames_)
    : fileNames(fileNames_),
      sampler(sampler_)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (separatorLabel = new Label ("new label",
                                                   TRANS("Separator")));
    separatorLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    separatorLabel->setJustificationType (Justification::centredLeft);
    separatorLabel->setEditable (false, false, false);
    separatorLabel->setColour (Label::textColourId, Colours::white);
    separatorLabel->setColour (TextEditor::textColourId, Colours::black);
    separatorLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (separatorEditor = new Label ("new label",
                                                    TRANS("_")));
    separatorEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    separatorEditor->setJustificationType (Justification::centred);
    separatorEditor->setEditable (true, true, false);
    separatorEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    separatorEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    separatorEditor->setColour (TextEditor::textColourId, Colours::black);
    separatorEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    separatorEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    separatorEditor->addListener (this);

    addAndMakeVisible (separatorLabel2 = new Label ("new label",
                                                    TRANS("First Filename")));
    separatorLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    separatorLabel2->setJustificationType (Justification::centredLeft);
    separatorLabel2->setEditable (false, false, false);
    separatorLabel2->setColour (Label::textColourId, Colours::white);
    separatorLabel2->setColour (TextEditor::textColourId, Colours::black);
    separatorLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (fileNameEditor = new Label ("new label",
                                                   TRANS("piano_rr1_50_127_d2")));
    fileNameEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    fileNameEditor->setJustificationType (Justification::centred);
    fileNameEditor->setEditable (false, false, false);
    fileNameEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    fileNameEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    fileNameEditor->setColour (TextEditor::textColourId, Colours::black);
    fileNameEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fileNameEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));

    addAndMakeVisible (separatorLabel3 = new Label ("new label",
                                                    TRANS("Tokens found")));
    separatorLabel3->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    separatorLabel3->setJustificationType (Justification::centredLeft);
    separatorLabel3->setEditable (false, false, false);
    separatorLabel3->setColour (Label::textColourId, Colours::white);
    separatorLabel3->setColour (TextEditor::textColourId, Colours::black);
    separatorLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (propertiesEditor = new Label ("new label",
                                                     TRANS("7")));
    propertiesEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    propertiesEditor->setJustificationType (Justification::centred);
    propertiesEditor->setEditable (false, false, false);
    propertiesEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    propertiesEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    propertiesEditor->setColour (TextEditor::textColourId, Colours::black);
    propertiesEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    propertiesEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));

    addAndMakeVisible (separatorLabel4 = new Label ("new label",
                                                    TRANS("Files to import")));
    separatorLabel4->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    separatorLabel4->setJustificationType (Justification::centredLeft);
    separatorLabel4->setEditable (false, false, false);
    separatorLabel4->setColour (Label::textColourId, Colours::white);
    separatorLabel4->setColour (TextEditor::textColourId, Colours::black);
    separatorLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (filesAmountEditor = new Label ("new label",
                                                      TRANS("7")));
    filesAmountEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    filesAmountEditor->setJustificationType (Justification::centred);
    filesAmountEditor->setEditable (false, false, false);
    filesAmountEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    filesAmountEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    filesAmountEditor->setColour (TextEditor::textColourId, Colours::black);
    filesAmountEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    filesAmountEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));

    addAndMakeVisible (copyButton = new TextButton ("new button"));
    copyButton->setButtonText (TRANS("Copy Settings"));
    copyButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    copyButton->addListener (this);
    copyButton->setColour (TextButton::buttonColourId, Colour (0xffababab));
    copyButton->setColour (TextButton::buttonOnColourId, Colours::brown);

    addAndMakeVisible (pasteButton = new TextButton ("new button"));
    pasteButton->setButtonText (TRANS("Paste Settings"));
    pasteButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    pasteButton->addListener (this);
    pasteButton->setColour (TextButton::buttonColourId, Colour (0xffababab));
    pasteButton->setColour (TextButton::buttonOnColourId, Colours::brown);

    addAndMakeVisible (saveButton = new TextButton ("new button"));
    saveButton->setButtonText (TRANS("Save Settings"));
    saveButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    saveButton->addListener (this);
    saveButton->setColour (TextButton::buttonColourId, Colour (0xffababab));
    saveButton->setColour (TextButton::buttonOnColourId, Colours::brown);

    addAndMakeVisible (loadButton = new TextButton ("new button"));
    loadButton->setButtonText (TRANS("Load Settings"));
    loadButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    loadButton->addListener (this);
    loadButton->setColour (TextButton::buttonColourId, Colour (0xffababab));
    loadButton->setColour (TextButton::buttonOnColourId, Colours::brown);


    //[UserPreSize]

	separatorLabel->setFont (GLOBAL_FONT());
	separatorEditor->setFont (GLOBAL_FONT());
	separatorLabel2->setFont (GLOBAL_FONT());
	fileNameEditor->setFont (GLOBAL_FONT());
	separatorLabel3->setFont (GLOBAL_FONT());
	filesAmountEditor->setFont (GLOBAL_FONT());
	separatorLabel4->setFont (GLOBAL_FONT());
	propertiesEditor->setFont (GLOBAL_FONT());

	filesAmountEditor->setText(String(fileNames.size()), dontSendNotification);

	for(int i = 0; i < fileNames.size(); i++)
	{
		relativeFileNames.add(File(fileNames[i]).getFileNameWithoutExtension());
	}

	setSeparator("_");



    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

FileNameImporterDialog::~FileNameImporterDialog()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    separatorLabel = nullptr;
    separatorEditor = nullptr;
    separatorLabel2 = nullptr;
    fileNameEditor = nullptr;
    separatorLabel3 = nullptr;
    propertiesEditor = nullptr;
    separatorLabel4 = nullptr;
    filesAmountEditor = nullptr;
    copyButton = nullptr;
    pasteButton = nullptr;
    saveButton = nullptr;
    loadButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void FileNameImporterDialog::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void FileNameImporterDialog::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    separatorLabel->setBounds (8, 11, 79, 24);
    separatorEditor->setBounds (24, 32, 29, 16);
    separatorLabel2->setBounds (124, 11, 79, 24);
    fileNameEditor->setBounds (83, 32, 157, 16);
    separatorLabel3->setBounds (332, 11, 100, 24);
    propertiesEditor->setBounds (361, 32, 29, 16);
    separatorLabel4->setBounds (240, 11, 100, 24);
    filesAmountEditor->setBounds (255, 32, 50, 16);
    copyButton->setBounds (512, 8, 78, 16);
    pasteButton->setBounds (512, 32, 78, 16);
    saveButton->setBounds (424, 8, 78, 16);
    loadButton->setBounds (424, 32, 78, 16);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void FileNameImporterDialog::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == separatorEditor)
    {
        //[UserLabelCode_separatorEditor] -- add your label text handling code here..
		setSeparator(separatorEditor->getText());
        //[/UserLabelCode_separatorEditor]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void FileNameImporterDialog::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == copyButton)
    {
        //[UserButtonCode_copyButton] -- add your button handler code here..

		ScopedPointer<XmlElement> settings = new XmlElement("settings");

		settings->setAttribute("Separator", currentSeparator);

		for(int i = 0; i < tokenPanels.size(); i++)
		{
			settings->addChildElement(tokenPanels[i]->exportSettings());
		}

		SystemClipboard::copyTextToClipboard(settings->createDocument(""));

        //[/UserButtonCode_copyButton]
    }
    else if (buttonThatWasClicked == pasteButton)
    {
        //[UserButtonCode_pasteButton] -- add your button handler code here..

		ScopedPointer<XmlElement> settings = XmlDocument::parse(SystemClipboard::getTextFromClipboard());

		if(settings != nullptr)
		{
			separatorEditor->setText(settings->getStringAttribute("Separator", "_"), sendNotification);

			if(tokenPanels.size() == settings->getNumChildElements())
			{
				for(int i = 0; i < settings->getNumChildElements(); i++)
				{
					tokenPanels[i]->importSettings(*settings->getChildElement(i));

				}
			}

		}

        //[/UserButtonCode_pasteButton]
    }
    else if (buttonThatWasClicked == saveButton)
    {
        //[UserButtonCode_saveButton] -- add your button handler code here..

        ScopedPointer<XmlElement> settings = new XmlElement("settings");

		settings->setAttribute("Separator", currentSeparator);

		for(int i = 0; i < tokenPanels.size(); i++)
		{
			settings->addChildElement(tokenPanels[i]->exportSettings());
		}

        PresetHandler::saveFile(settings->createDocument(""), "*.xml");

        //[/UserButtonCode_saveButton]
    }
    else if (buttonThatWasClicked == loadButton)
    {
        //[UserButtonCode_loadButton] -- add your button handler code here..

        ScopedPointer<XmlElement> settings = XmlDocument::parse(PresetHandler::loadFile("*.xml"));

		if(settings != nullptr)
		{
			separatorEditor->setText(settings->getStringAttribute("Separator", "_"), sendNotification);

			if(tokenPanels.size() == settings->getNumChildElements())
			{
				for(int i = 0; i < settings->getNumChildElements(); i++)
				{
					tokenPanels[i]->importSettings(*settings->getChildElement(i));

				}
			}

		}

        //[/UserButtonCode_loadButton]
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

<JUCER_COMPONENT documentType="Component" className="FileNameImporterDialog" componentName=""
                 parentClasses="public Component" constructorParams="ModulatorSampler *sampler_, const StringArray &amp;fileNames_"
                 variableInitialisers="fileNames(fileNames_),&#10;sampler(sampler_)"
                 snapPixels="8" snapActive="1" snapShown="0" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="2b2b2b"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="separatorLabel"
         virtualName="" explicitFocusOrder="0" pos="8 11 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Separator" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="separatorEditor"
         virtualName="" explicitFocusOrder="0" pos="24 32 29 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="_" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="c1f0dd0a2febbec8" memberName="separatorLabel2"
         virtualName="" explicitFocusOrder="0" pos="124 11 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="First Filename"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="64acc46d557be8" memberName="fileNameEditor"
         virtualName="" explicitFocusOrder="0" pos="83 32 157 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="piano_rr1_50_127_d2" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="553c052cb6647962" memberName="separatorLabel3"
         virtualName="" explicitFocusOrder="0" pos="332 11 100 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Tokens found" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="f4ea497bc1cfa22c" memberName="propertiesEditor"
         virtualName="" explicitFocusOrder="0" pos="361 32 29 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="7" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="86c089606859e6d3" memberName="separatorLabel4"
         virtualName="" explicitFocusOrder="0" pos="240 11 100 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Files to import"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="6856f1ed651aa59f" memberName="filesAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="255 32 50 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="7" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <TEXTBUTTON name="new button" id="63d3065c97ec2cb5" memberName="copyButton"
              virtualName="" explicitFocusOrder="0" pos="512 8 78 16" bgColOff="ffababab"
              bgColOn="ffa52a2a" buttonText="Copy Settings" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="1150dd015005f9fd" memberName="pasteButton"
              virtualName="" explicitFocusOrder="0" pos="512 32 78 16" bgColOff="ffababab"
              bgColOn="ffa52a2a" buttonText="Paste Settings" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="4d9786d21903c7ef" memberName="saveButton"
              virtualName="" explicitFocusOrder="0" pos="424 8 78 16" bgColOff="ffababab"
              bgColOn="ffa52a2a" buttonText="Save Settings" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="50ff3359b680c441" memberName="loadButton"
              virtualName="" explicitFocusOrder="0" pos="424 32 78 16" bgColOff="ffababab"
              bgColOn="ffa52a2a" buttonText="Load Settings" connectedEdges="3"
              needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
