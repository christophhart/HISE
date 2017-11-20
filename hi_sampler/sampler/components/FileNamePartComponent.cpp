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

#include "FileNamePartComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
FileNamePartComponent::FileNamePartComponent (const String &token)
    : tokenName(token)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

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


    //[UserPreSize]

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

	propertyLabel->addOption("Velocity Value", "Sets the velocity range to (VALUE,VALUE+1).");
	propertyLabel->addOption("Velocity Range", "Use this with 'NumericRange' for velocity information like '63-127'");
	propertyLabel->addOption("Velocity Spread", "Spreads the velocity over the complete range. Use 'Number' or 'Custom' as Datatype for this.");
	
    propertyLabel->addOption("LowVelocity", "The lower velocity limit.");
    propertyLabel->addOption("HighVelocity", "The upper velocity limit.");
    
	propertyLabel->addOption("Single Key", "maps the value to RootNote, KeyLow and KeyHigh.");
	propertyLabel->addOption("RR Group", "Puts the sound into the specified group.");
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

	addAndMakeVisible(propertyInfoButton = new ShapeButton("Info", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.6f), Colours::white));
	addAndMakeVisible(dataInfoButton = new ShapeButton("Info", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.6f), Colours::white));

	static const unsigned char pathData[] = { 110, 109, 0, 0, 12, 67, 46, 183, 84, 68, 98, 229, 174, 239, 66, 46, 183, 84, 68, 254, 255, 206, 66, 11, 205, 88, 68, 254, 255, 206, 66, 46, 215, 93, 68, 98, 254, 255, 206, 66, 82, 225, 98, 68, 228, 174, 239, 66, 46, 247, 102, 68, 0, 0, 12, 67, 46, 247, 102, 68, 98, 142, 40, 32, 67, 46, 247, 102, 68, 1, 128, 48, 67, 81, 225,
		98, 68, 1, 128, 48, 67, 46, 215, 93, 68, 98, 1, 128, 48, 67, 10, 205, 88, 68, 142, 40, 32, 67, 46, 183, 84, 68, 0, 0, 12, 67, 46, 183, 84, 68, 99, 109, 0, 0, 12, 67, 46, 65, 101, 68, 98, 31, 62, 247, 66, 46, 65, 101, 68, 255, 175, 220, 66, 106, 239, 97, 68, 255, 175, 220, 66, 46, 215, 93, 68, 98, 255, 175, 220, 66, 242, 190,
		89, 68, 31, 62, 247, 66, 46, 109, 86, 68, 0, 0, 12, 67, 46, 109, 86, 68, 98, 240, 96, 28, 67, 46, 109, 86, 68, 1, 168, 41, 67, 242, 190, 89, 68, 1, 168, 41, 67, 46, 215, 93, 68, 98, 1, 168, 41, 67, 106, 239, 97, 68, 241, 96, 28, 67, 46, 65, 101, 68, 0, 0, 12, 67, 46, 65, 101, 68, 99, 109, 0, 112, 7, 67, 46, 71, 89, 68, 108, 0, 144,
		16, 67, 46, 71, 89, 68, 108, 0, 144, 16, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 143, 91, 68, 108, 0, 112, 7, 67, 46, 71, 89, 68, 99, 109, 0, 32, 21, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 103, 98, 68, 108, 0, 224, 2, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 67, 97, 68, 108, 0, 112, 7, 67, 46, 215, 93, 68, 108, 0, 224,
		2, 67, 46, 215, 93, 68, 108, 0, 224, 2, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 179, 92, 68, 108, 0, 144, 16, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 67, 97, 68, 108, 0, 32, 21, 67, 46, 103, 98, 68, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));

	propertyInfoButton->setShape(path, false, true, true);
	dataInfoButton->setShape(path, false, true, true);

	propertyInfoButton->addListener(this);
	dataInfoButton->addListener(this);

    //[/UserPreSize]

    setSize (600, 40);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

FileNamePartComponent::~FileNamePartComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

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


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void FileNamePartComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setGradientFill (ColourGradient (Colour (0xff1b1b1b),
                                       240.0f, 0.0f,
                                       Colours::black,
                                       240.0f, 40.0f,
                                       false));
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (proportionOfWidth (1.0000f)), static_cast<float> (proportionOfHeight (1.0000f)), 5.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void FileNamePartComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

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
    //[UserResized] Add your own custom resize handling here..

	propertyInfoButton->setBounds(propertyLabel->getRight() - 14, 1, 16, 16);
	dataInfoButton->setBounds(dataLabel->getRight() - 14, 1, 16, 16);

    //[/UserResized]
}

void FileNamePartComponent::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == propertyLabel)
    {
        //[UserLabelCode_propertyLabel] -- add your label text handling code here..
		tokenProperty = (TokenProperties) propertyLabel->getCurrentIndex();

        tokenDataType = getPreferedDataTypeFor(tokenProperty);
        
		const bool enableFields = tokenProperty != TokenProperties::Ignore;

		dataLabel->setEnabled(enableFields);
		itemLabel->setEnabled(enableFields);
		valueLabel->setEnabled(enableFields);

		dataLabel->setItemIndex(tokenDataType, sendNotification);

        //[/UserLabelCode_propertyLabel]
    }
    else if (labelThatHasChanged == dataLabel)
    {
        //[UserLabelCode_dataLabel] -- add your label text handling code here..
		tokenDataType = (Datatype) dataLabel->getCurrentIndex();

		if (tokenDataType == Custom)
		{
			fillCustomList();
		}

        //[/UserLabelCode_dataLabel]
    }
    else if (labelThatHasChanged == itemLabel)
    {
        //[UserLabelCode_itemLabel] -- add your label text handling code here..

		customList.clear();
		customList.addTokens(itemLabel->getText(), " ", "");
        //[/UserLabelCode_itemLabel]
    }
    else if (labelThatHasChanged == valueLabel)
    {
        //[UserLabelCode_valueLabel] -- add your label text handling code here..

		StringArray valueStrings;
		valueStrings.addTokens(valueLabel->getText(), " ", "");

		valueList.clear();

		for(int i = 0; i < valueStrings.size(); i++)
		{
			valueList.add(valueStrings[i].getIntValue());
		}

		if(valueList.size() == 2)
		{
			valueRange = Range<int>(valueList[0], valueList[1]);
		}
		else
		{
			valueRange = Range<int>();
		}

        //[/UserLabelCode_valueLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
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

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

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

void FileNamePartComponent::buttonClicked(Button* b)
{
	if (b == dataInfoButton)
	{
		PresetHandler::showMessageWindow("Data Type Help", dataLabel->getOptionDescription(), PresetHandler::IconType::Info);
	}
	else if (b == propertyInfoButton)
	{
		PresetHandler::showMessageWindow("Property Type Help", propertyLabel->getOptionDescription(), PresetHandler::IconType::Info);
	}

}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="FileNamePartComponent" componentName=""
                 parentClasses="public Component" constructorParams="const String &amp;token"
                 variableInitialisers="tokenName(token)" snapPixels="8" snapActive="1"
                 snapShown="0" overlayOpacity="0.330" fixedSize="1" initialWidth="500"
                 initialHeight="40">
  <BACKGROUND backgroundColour="0">
    <ROUNDRECT pos="0 0 100% 100%" cornerSize="5" fill="linear: 240 0, 240 40, 0=ff1b1b1b, 1=ff000000"
               hasStroke="0"/>
  </BACKGROUND>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="separatorLabel"
         virtualName="" explicitFocusOrder="0" pos="24 -2 39 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="String" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="partName"
         virtualName="" explicitFocusOrder="0" pos="13 18 64 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="PART" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="45354151fdeccf85" memberName="displayGroupLabel"
         virtualName="" explicitFocusOrder="0" pos="100 -1 57 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Property" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="3ca481f4230f2188" memberName="propertyLabel"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="88 18 80 16"
         tooltip="A selection of useful properties that can be set via the filename token."
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="e51ef85c31600193" memberName="displayGroupLabel2"
         virtualName="" explicitFocusOrder="0" pos="189 -1 57 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Data Type" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="4a5730ac1fdfeba5" memberName="dataLabel"
         virtualName="PopupLabel" explicitFocusOrder="0" pos="177 18 80 16"
         tooltip="Specifies the type of data which this token contains."
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="a117394962b4825d" memberName="displayGroupLabel3"
         virtualName="" explicitFocusOrder="0" pos="295 0 57 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Items" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="ac4700c36967320e" memberName="itemLabel"
         virtualName="Label" explicitFocusOrder="0" pos="265 18 108 16"
         tooltip="Enter all existing items" bkgCol="38ffffff" outlineCol="38ffffff"
         edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000" labelText=""
         editableSingleClick="1" editableDoubleClick="1" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="14" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="2b581b4ac7cedcbe" memberName="displayGroupLabel4"
         virtualName="" explicitFocusOrder="0" pos="409 0 57 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Values" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="1a77f50564fb64c" memberName="valueLabel"
         virtualName="Label" explicitFocusOrder="0" pos="379 18 108 16"
         tooltip="enter the value list that are associated to the item list"
         bkgCol="38ffffff" outlineCol="38ffffff" edTextCol="ff000000"
         edBkgCol="0" hiliteCol="407a0000" labelText="" editableSingleClick="1"
         editableDoubleClick="1" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="14" bold="0" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
