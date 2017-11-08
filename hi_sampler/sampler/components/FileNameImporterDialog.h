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

#ifndef __JUCE_HEADER_44A2A0BCCC20A6FE__
#define __JUCE_HEADER_44A2A0BCCC20A6FE__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;
//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
*	@endinternal
*
*	@class FileNameImporterDialog
*	@ingroup components
*
*	@brief A component that allows to specify certain tokens within a file name pattern
*	and associate them with a ModulatorSamplerSound::Property.
*	This can be used to set the basic data (Everything that can be stored in SampleImporter::SamplerSoundBasicData) automatically
*	when new sounds are imported.
*
*	You have to specify a seperator character and it divides the file name and shows a FileNamePartComponent for all detected tokens.
*
*	You can import and export the panel settings (it copies an XML string into the clipboard so you can do whatever you need to do with it)
*	to save some time when importing multiple but similar file selections.
*
*	@internal
                                                                    //[/Comments]
*/
class FileNameImporterDialog  : public Component,
                                public LabelListener,
                                public ButtonListener
{
public:
    //==============================================================================
    FileNameImporterDialog (ModulatorSampler *sampler_, const StringArray &fileNames_);
    ~FileNameImporterDialog();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	/** set the separator character. Multiple characters are not allowed. */
	void setSeparator(String separator)
	{
		currentSeparator = separator;

		jassert(relativeFileNames.size() > 0);

		String firstName = relativeFileNames[0];

		tokens.clear();

		tokens.addTokens(firstName, separator, String());

		fileNameEditor->setText(firstName, dontSendNotification);
		propertiesEditor->setText(String(tokens.size()), dontSendNotification);

		tokenPanels.clear();

		int y = 64;

		for(int i = 0; i < tokens.size(); i++)
		{

			FileNamePartComponent *tp = new FileNamePartComponent(tokens[i]);

			addAndMakeVisible(tp);
			tp->setBounds(50, y, 500, 40);

			y += 50;

			tokenPanels.add(tp);



		}

	};

	String getSeparator() const { return currentSeparator; }

	int getTokenIndex(FileNamePartComponent *component) const
	{
		return tokenPanels.indexOf(component);
	}

	/** Fills the supplied list with the data. Call this function whenever you have set up all token panels.
	*
	*	@param dataList a reference to an array of SampleImporter::SamplerSoundBasicData which will be filled (it will be cleared automatically.)
	*	@param startIndex the offset for the indexes (they will be incremented with each file.
	*/
	void fillDataList(SampleImporter::SampleCollection &collection, const int startIndex) const
	{
		collection.dataList.clear();

		for(int i = 0; i < fileNames.size(); i++)
		{
			SampleImporter::SamplerSoundBasicData data;

			data.index = startIndex + i;
			data.fileNames.add(fileNames[i]);
			collection.dataList.add(data);

			StringArray currentTokens;

			currentTokens.addTokens(relativeFileNames[i], currentSeparator, String());

			for(int t = 0; t < tokenPanels.size(); t++)
			{
				tokenPanels[t]->fillDataWithTokenInformation(collection, i, currentTokens[t]);
			}

			

		}
	}

	const StringArray &getFileNameList() { return relativeFileNames; }

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	void restoreFromXml(const String& xmlData);

	XmlElement* saveAsXml();

	StringArray fileNames;
	StringArray relativeFileNames;
	StringArray tokens;

	String currentSeparator;

	OwnedArray<FileNamePartComponent> tokenPanels;

	ModulatorSampler *sampler;

	AlertWindowLookAndFeel laf;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> separatorLabel;
    ScopedPointer<Label> separatorEditor;
    ScopedPointer<Label> separatorLabel2;
    ScopedPointer<Label> fileNameEditor;
    ScopedPointer<Label> separatorLabel3;
    ScopedPointer<Label> propertiesEditor;
    ScopedPointer<Label> separatorLabel4;
    ScopedPointer<Label> filesAmountEditor;
    ScopedPointer<TextButton> copyButton;
    ScopedPointer<TextButton> pasteButton;
    ScopedPointer<TextButton> saveButton;
    ScopedPointer<TextButton> loadButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileNameImporterDialog)
};

//[EndFile] You can add extra defines here...


/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_44A2A0BCCC20A6FE__
