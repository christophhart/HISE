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

namespace hise { using namespace juce;

#include "FileNameImporterDialog.h"


//==============================================================================
FileNameImporterDialog::FileNameImporterDialog (ModulatorSampler *sampler_, const StringArray &fileNames_)
    : fileNames(fileNames_),
      sampler(sampler_)
{

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
    separatorEditor->setColour (Label::backgroundColourId, Colour (0x66ffffff));
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
    fileNameEditor->setColour (Label::backgroundColourId, Colour (0x66ffffff));
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
    propertiesEditor->setColour (Label::backgroundColourId, Colour (0x66ffffff));
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
    filesAmountEditor->setColour (Label::backgroundColourId, Colour (0x66ffffff));
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

	addAndMakeVisible(clearButton = new TextButton("new button"));
	clearButton->setButtonText(TRANS("Clear Settings"));
	clearButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	clearButton->addListener(this);
	clearButton->setColour(TextButton::buttonColourId, Colour(0xffababab));
	clearButton->setColour(TextButton::buttonOnColourId, Colours::brown);

	auto separatorButton = new MarkdownHelpButton();
	separatorButton->setHelpText(FileParserHelp::Separator());
	separatorButton->attachTo(separatorEditor, MarkdownHelpButton::Left);

	separatorLabel->setFont (GLOBAL_BOLD_FONT());
	separatorEditor->setFont (GLOBAL_BOLD_FONT());
	separatorLabel2->setFont (GLOBAL_BOLD_FONT());
	fileNameEditor->setFont (GLOBAL_BOLD_FONT());
	separatorLabel3->setFont (GLOBAL_BOLD_FONT());
	filesAmountEditor->setFont (GLOBAL_BOLD_FONT());
	separatorLabel4->setFont (GLOBAL_BOLD_FONT());
	propertiesEditor->setFont (GLOBAL_BOLD_FONT());

	filesAmountEditor->setText(String(fileNames.size()), dontSendNotification);

	for(int i = 0; i < fileNames.size(); i++)
		relativeFileNames.add(File(fileNames[i]).getFileNameWithoutExtension());

	setSeparator("_");
    setSize (600, 400);

	File recentSettingsFile = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("FileNameParserSettings.xml");

	if (recentSettingsFile.existsAsFile())
	{
		const String recentSettings = recentSettingsFile.loadFileAsString();
		restoreFromXml(recentSettings);
	}
}

FileNameImporterDialog::~FileNameImporterDialog()
{
	File recentSettingsFile = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("FileNameParserSettings.xml");

	ScopedPointer<XmlElement> settings = saveAsXml();

	recentSettingsFile.replaceWithText(settings->createDocument(""));

	settings = nullptr;

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
}

//==============================================================================
void FileNameImporterDialog::paint (Graphics& g)
{
	g.fillAll(Colours::transparentBlack);
}

void FileNameImporterDialog::resized()
{
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
}

void FileNameImporterDialog::labelTextChanged (Label* labelThatHasChanged)
{
    if (labelThatHasChanged == separatorEditor)
    {
		setSeparator(separatorEditor->getText());
    }
}

void FileNameImporterDialog::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == copyButton)
    {
		ScopedPointer<XmlElement> settings = saveAsXml();
		SystemClipboard::copyTextToClipboard(settings->createDocument(""));
    }
    else if (buttonThatWasClicked == pasteButton)
    {
		restoreFromXml(SystemClipboard::getTextFromClipboard());
    }
    else if (buttonThatWasClicked == saveButton)
    {
		ScopedPointer<XmlElement> settings = saveAsXml();
        PresetHandler::saveFile(settings->createDocument(""), "*.xml");
    }
    else if (buttonThatWasClicked == loadButton)
    {
		auto content = PresetHandler::loadFile("*.xml").loadFileAsString();
		restoreFromXml(content);
    }
}



void FileNameImporterDialog::restoreFromXml(const String& xmlData)
{
	if (auto settings = XmlDocument::parse(xmlData))
	{
		separatorEditor->setText(settings->getStringAttribute("Separator", "_"), sendNotification);

		if (tokenPanels.size() == settings->getNumChildElements())
		{
			for (int i = 0; i < settings->getNumChildElements(); i++)
				tokenPanels[i]->importSettings(*settings->getChildElement(i));
		}
		else
		{
			if (PresetHandler::showYesNoWindow("Token amount mismatch", "The settings you are about to load have a different amount of tokens. Press OK to load it anyway."))
			{
				for (int i = 0; i < tokenPanels.size(); i++)
				{
					if (i < settings->getNumChildElements())
					{
						tokenPanels[i]->importSettings(*settings->getChildElement(i));
					}
				}
			}
		}
	}
	else
	{
		PresetHandler::showMessageWindow("Parsing Error", "The XML settings file could not be parsed", PresetHandler::IconType::Warning);
	}
}

XmlElement* FileNameImporterDialog::saveAsXml()
{
	ScopedPointer<XmlElement> settings = new XmlElement("settings");

	settings->setAttribute("Separator", currentSeparator);

	for (int i = 0; i < tokenPanels.size(); i++)
	{
		settings->addChildElement(tokenPanels[i]->exportSettings());
	}

	return settings.release();
}

} // namespace hise
