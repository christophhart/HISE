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

#include "FileNamePartComponent.h"

MARKDOWN_CHAPTER(FileParserHelp)
START_MARKDOWN(Properties)
ML("## Token Property Selector");
ML("This column specifies the sample property (or properties) that should be set with the given token.");
ML("| Property Name | Description | Expected Data type |");
ML("| --- | ----- | ----- |");
ML("| Velocity Value | Sets the velocity range to a single velocity value `(LoVel = x, HiVel = x + 1)`. | A single number. |");
ML("| Velocity Range | Sets the velocity range. | A string in the format `63-127` (use the **NumericRange** data type). |");
ML("| Velocity Spread | Spreads the velocity over the complete range | A **Custom** list or a **Number** x for (1-x). |");
ML("| LowVelocity | The lower velocity limit. | A single number. |");
ML("| HighVelocity | The upper velocity limit. | A single number. |");
ML("| Single Key | maps the value to all note properties (`RootNote`, `LoKey` and `HiKey`). | A single number or the MIDI note name. |");
ML("| RR Group | Sets the group index to be used for Round Robin (or custom logic). | A **Custom** list or a **Number** x for (1-x). |");
ML("| Multimic | Sets the multimic index. | A **Custom** list of mic positions. |");
ML("| Ignore | Do nothing with this token (default). Use this for every token that does not contain special information. | nothing |");
END_MARKDOWN()
START_MARKDOWN(DataType)
ML("## Token Data Type Selector");
ML("This column specifies how the token should be interpreted for setting the property (or properties) for the given token.");
ML("| Data type | Expected Input | Items Example | Values Example | Result |");
ML("| -- | ---- | -- | -- | ----- |");
ML("| Number | A raw number. | - | - | the token property is set to the single value. |");
ML("| Number with range | A number inside a range (starting with 1) | 4 | - | values `1, 2, 3, 4` are distributed across the property range. |");
ML("| Numeric Range | a range in the format `1-63` | - | - | Velocity Range is set to LoVel: 1, HiVel: 63. |");
ML("| Note name | a note name in the format `D#3` (middle octave is 3). | - | - | the token property is set to 60. |");
ML("| Custom | a key-value pair | `pp, mp, ff` | 1, 2, 3 | pp is mapped to the lower, mp to the middle and ff to the upper third of the property range. |");
ML("| Fixed value | a fixed value (discards the token value) | 5 | - | the token property is set to 5. |");
ML("| Ignored | Ignores the token (default). | - | - | - |");
END_MARKDOWN()
START_MARKDOWN(Separator)
ML("## Setting the token separator");
ML("The file name of each sample will be divided into multiple tokens based on the separator character that you can enter here.");
ML("### Example");
ML("Let's assume we have two samples with the filenames");
ML("`Cello_stacc_RR1_D#2_mp.wav`");
ML("`Cello_stacc_RR2_C3_f.wav`");
ML("The character that separates the tokens is obviously the `_` character (which is by far the best character for separation and far superior to (`-` or `.`). It will divide the filename into 5 tokens and create 5 rows for each token that can be set to these values:")
ML("- `Cello`: can be ignored");
ML("- `stacc`: can be ignored");
ML("- RR1, RR2: the RR group of the sample. Use the Custom data type and create the mapping [RR1 -> 1, RR2 -> 2]");
ML("- D#2, C3: the note number of the sample. Use Single-Key & Notename to map the sample to the given note.");
ML("- mp, f: the velocity range. Use the Custom data type and create the mapping [mp -> 1, f -> 2].");
END_MARKDOWN()
END_MARKDOWN_CHAPTER()

//==============================================================================
FileNamePartComponent::FileNamePartComponent (const String &token)
    : tokenName(token)
{
    addAndMakeVisible (separatorLabel = new Label ("new label",
                                                   TRANS("String")));
    separatorLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    separatorLabel->setJustificationType (Justification::centredLeft);
    separatorLabel->setEditable (false, false, false);
    separatorLabel->setColour (Label::textColourId, Colours::white);
    separatorLabel->setColour (TextEditor::textColourId, Colours::black);
    separatorLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (partName = new Label ("new label",
                                             TRANS("PART")));
    partName->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    partName->setJustificationType (Justification::centred);
    partName->setEditable (false, false, false);
    partName->setColour (Label::backgroundColourId, Colour (0x88ffffff));
    partName->setColour (Label::outlineColourId, Colour (0x38ffffff));
    partName->setColour (TextEditor::textColourId, Colours::black);
    partName->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    partName->setColour (TextEditor::highlightColourId, Colour (0x407a0000));

    addAndMakeVisible (displayGroupLabel = new Label ("new label",
                                                      TRANS("Property")));
    displayGroupLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    displayGroupLabel->setJustificationType (Justification::centredLeft);
    displayGroupLabel->setEditable (false, false, false);
    displayGroupLabel->setColour (Label::textColourId, Colours::white);
    displayGroupLabel->setColour (TextEditor::textColourId, Colours::black);
    displayGroupLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (propertyLabel = new PopupLabel ("new label",
                                                       String()));
    propertyLabel->setTooltip (TRANS("A selection of useful properties that can be set via the filename token."));
    propertyLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    propertyLabel->setJustificationType (Justification::centred);
    propertyLabel->setEditable (true, true, false);
    propertyLabel->setColour (Label::backgroundColourId, Colour (0x88ffffff));
    propertyLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    propertyLabel->setColour (TextEditor::textColourId, Colours::black);
    propertyLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    propertyLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    propertyLabel->addListener (this);

    addAndMakeVisible (displayGroupLabel2 = new Label ("new label",
                                                       TRANS("Data Type")));
    displayGroupLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    displayGroupLabel2->setJustificationType (Justification::centredLeft);
    displayGroupLabel2->setEditable (false, false, false);
    displayGroupLabel2->setColour (Label::textColourId, Colours::white);
    displayGroupLabel2->setColour (TextEditor::textColourId, Colours::black);
    displayGroupLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (dataLabel = new PopupLabel ("new label",
                                                   String()));
    dataLabel->setTooltip (TRANS("Specifies the type of data which this token contains."));
    dataLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    dataLabel->setJustificationType (Justification::centred);
    dataLabel->setEditable (true, true, false);
    dataLabel->setColour (Label::backgroundColourId, Colour (0x88ffffff));
    dataLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    dataLabel->setColour (TextEditor::textColourId, Colours::black);
    dataLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    dataLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    dataLabel->addListener (this);

    addAndMakeVisible (displayGroupLabel3 = new Label ("new label",
                                                       TRANS("Items")));
    displayGroupLabel3->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    displayGroupLabel3->setJustificationType (Justification::centredLeft);
    displayGroupLabel3->setEditable (false, false, false);
    displayGroupLabel3->setColour (Label::textColourId, Colours::white);
    displayGroupLabel3->setColour (TextEditor::textColourId, Colours::black);
    displayGroupLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (itemLabel = new Label ("new label",
                                              String()));
    itemLabel->setTooltip (TRANS("Enter all existing items"));
    itemLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    itemLabel->setJustificationType (Justification::centred);
    itemLabel->setEditable (true, true, false);
    itemLabel->setColour (Label::backgroundColourId, Colour (0x88ffffff));
    itemLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    itemLabel->setColour (TextEditor::textColourId, Colours::black);
    itemLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    itemLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    itemLabel->addListener (this);

    addAndMakeVisible (displayGroupLabel4 = new Label ("new label",
                                                       TRANS("Values")));
    displayGroupLabel4->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    displayGroupLabel4->setJustificationType (Justification::centredLeft);
    displayGroupLabel4->setEditable (false, false, false);
    displayGroupLabel4->setColour (Label::textColourId, Colours::white);
    displayGroupLabel4->setColour (TextEditor::textColourId, Colours::black);
    displayGroupLabel4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (valueLabel = new Label ("new label",
                                               String()));
    valueLabel->setTooltip (TRANS("enter the value list that are associated to the item list"));
    valueLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    valueLabel->setJustificationType (Justification::centred);
    valueLabel->setEditable (true, true, false);
    valueLabel->setColour (Label::backgroundColourId, Colour (0x88ffffff));
    valueLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    valueLabel->setColour (TextEditor::textColourId, Colours::black);
    valueLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    valueLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    valueLabel->addListener (this);

	

	separatorLabel->setFont (GLOBAL_BOLD_FONT());
	valueLabel->setFont (GLOBAL_BOLD_FONT());
	displayGroupLabel4->setFont (GLOBAL_BOLD_FONT());
	itemLabel->setFont (GLOBAL_BOLD_FONT());
	displayGroupLabel3->setFont (GLOBAL_BOLD_FONT());
	dataLabel->setFont (GLOBAL_BOLD_FONT());
	displayGroupLabel2->setFont (GLOBAL_BOLD_FONT());
	propertyLabel->setFont (GLOBAL_BOLD_FONT());
	displayGroupLabel->setFont (GLOBAL_BOLD_FONT());
	partName->setFont (GLOBAL_BOLD_FONT());

	partName->setText(tokenName, dontSendNotification);

	auto propertyButton = new MarkdownHelpButton();
	propertyButton->setHelpText(FileParserHelp::Properties());
	propertyButton->attachTo(propertyLabel, MarkdownHelpButton::TopRight);
	propertyButton->setPopupWidth(750);

	auto dataTypeButton = new MarkdownHelpButton();
	dataTypeButton->setHelpText(FileParserHelp::DataType());
	dataTypeButton->attachTo(dataLabel, MarkdownHelpButton::TopRight);
	dataTypeButton->setPopupWidth(750);



	propertyLabel->addOption("Velocity Value", "Sets the velocity range to (VALUE,VALUE+1).");
	propertyLabel->addOption("Velocity Range", "Use this with 'NumericRange' for velocity information like '63-127'");
	propertyLabel->addOption("Velocity Spread", "Spreads the velocity over the complete range. Use 'Number' or 'Custom' as Datatype for this.");

	propertyLabel->addOption("LowVelocity", "The lower velocity limit.");
	propertyLabel->addOption("HighVelocity", "The upper velocity limit.");

	propertyLabel->addOption("Single Key", "maps the value to RootNote, KeyLow and KeyHigh.");
	propertyLabel->addOption("RR Group", "Puts the sound into the specified group.");
	propertyLabel->addOption("Multi Mic", "The Multimic index. Using this property to merge multimics will not perform any sanity checks.");
	propertyLabel->addOption("Ignore", "Do nothing with this token (default). Use this for every token that does not contain special information");

	dataLabel->addOption("Number", "A simple integer number that can be directly read without further processing.");
	dataLabel->addOption("Number with range", "Use this for numbers that indicate a range (starting with 1) and enter the upper limit into the item list.");
	dataLabel->addOption("Numeric Range", "a range in the format '1-63'");
	dataLabel->addOption("Note name", "If the token is a note name (format: 'D#3'), use this data type to get the midi note number (middle octave is 3).");
	dataLabel->addOption("Custom", "If the token is a custom string, fill in all possible items into the item list (seperated by space) and all values (as integer) into the value list.");
	dataLabel->addOption("Fixed value", "discard all information and directly set a value to the fixed number that is entered into the item list.");
	dataLabel->addOption("Ignored", "Ignores the token (default).");

	dataLabel->setItemIndex(Datatype::Ignored);

	propertyLabel->setItemIndex(TokenProperties::Ignore);

	dataLabel->setEditable(false);
	propertyLabel->setEditable(false);

    setSize (600, 40);
}

FileNamePartComponent::~FileNamePartComponent()
{
    separatorLabel = nullptr;
    partName = nullptr;
    displayGroupLabel = nullptr;
    propertyLabel = nullptr;
    displayGroupLabel2 = nullptr;
    dataLabel = nullptr;
    displayGroupLabel3 = nullptr;
    itemLabel = nullptr;
    displayGroupLabel4 = nullptr;
    valueLabel = nullptr;

}

//==============================================================================
void FileNamePartComponent::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colour (0xff1b1b1b),
                                       240.0f, 0.0f,
                                       Colours::black,
                                       240.0f, 40.0f,
                                       false));
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (proportionOfWidth (1.0000f)), static_cast<float> (proportionOfHeight (1.0000f)), 5.000f);
}

void FileNamePartComponent::resized()
{
    separatorLabel->setBounds (24, -2, 39, 24);
    partName->setBounds (13, 18, 64, 16);
    displayGroupLabel->setBounds (100, -1, 57, 24);
    propertyLabel->setBounds (88, 18, 80, 16);
    displayGroupLabel2->setBounds (189, -1, 57, 24);
    dataLabel->setBounds (177, 18, 80, 16);
    displayGroupLabel3->setBounds (295, 0, 57, 24);
    itemLabel->setBounds (265, 18, 108, 16);
    displayGroupLabel4->setBounds (409, 0, 57, 24);
    valueLabel->setBounds (379, 18, 108, 16);
}

void FileNamePartComponent::labelTextChanged (Label* labelThatHasChanged)
{
    if (labelThatHasChanged == propertyLabel)
    {
		tokenProperty = (TokenProperties) propertyLabel->getCurrentIndex();

        tokenDataType = getPreferedDataTypeFor(tokenProperty);
        
		const bool enableFields = tokenProperty != TokenProperties::Ignore;

		dataLabel->setEnabled(enableFields);
		itemLabel->setEnabled(enableFields);
		valueLabel->setEnabled(enableFields);

		dataLabel->setItemIndex(tokenDataType, sendNotification);
    }
    else if (labelThatHasChanged == dataLabel)
    {
		tokenDataType = (Datatype) dataLabel->getCurrentIndex();

		if (tokenDataType == Custom)
		{
			fillCustomList();
		}
    }
    else if (labelThatHasChanged == itemLabel)
    {
		customList.clear();
		customList.addTokens(itemLabel->getText(), " ", "");
    }
    else if (labelThatHasChanged == valueLabel)
    {
		StringArray valueStrings;
		valueStrings.addTokens(valueLabel->getText(), " ", "");

		valueList.clear();

		for(int i = 0; i < valueStrings.size(); i++)
		{
			valueList.add(valueStrings[i].getIntValue());
		}

		if(valueList.size() == 2)
			valueRange = Range<int>(valueList[0], valueList[1]);
		else
			valueRange = Range<int>();
    }
}



void FileNamePartComponent::fillCustomList()
{
	FileNameImporterDialog *daddy = findParentComponentOfClass<FileNameImporterDialog>();

	if (daddy != nullptr)
	{
		const StringArray files = daddy->getFileNameList();
		const int tokenIndex = daddy->getTokenIndex(this);
		const String separator = daddy->getSeparator();

		customList.clear();

		for (int i = 0; i < files.size(); i++)
		{
			StringArray tokens = StringArray::fromTokens(files[i], separator, "");

			if (tokenIndex < tokens.size())
			{
				String listItem = tokens[tokenIndex];
				
				if (!customList.contains(listItem))
				{
					int insertIndex = -1;

					if (customList.size() == 0)
					{
						customList.add(listItem);
					}
					else
					{
						for (int j = 0; j < customList.size(); j++)
						{
							if (customList[j].compareNatural(listItem) > 0)
							{
								insertIndex = j;
								break;
								
							}
						}

						customList.insert(insertIndex, listItem);
					}
				}
			}
		}

		valueList.clear();

		String valueString;

		for (int i = 0; i < customList.size(); i++)
		{
			valueList.add(i+1);

			valueString << ((i == 0) ? "" : " ") << String(i+1);
		}

		itemLabel->setText(customList.joinIntoString(" "), dontSendNotification);
		valueLabel->setText(valueString, dontSendNotification);
	}
}

void FileNamePartComponent::importSettings(XmlElement &p)
{
	String propName = p.getStringAttribute("Property");

	for (int i = 0; i < numTokenProperties; i++)
	{
		if (propName == getSpecialPropertyName((TokenProperties)i))
		{
			propertyLabel->setItemIndex(i);
		}
	}

	String dataName = p.getStringAttribute("DataType");

	for (int i = 0; i < numDataTypes; i++)
	{
		if (dataName == getDataTypeName((Datatype)i))
		{
			dataLabel->setItemIndex(i);
		}
	}

	itemLabel->setText(p.getStringAttribute("Items"), sendNotification);
	valueLabel->setText(p.getStringAttribute("Values"), sendNotification);
}

} // namespace hise
