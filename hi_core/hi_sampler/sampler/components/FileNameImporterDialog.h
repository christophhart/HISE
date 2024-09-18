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

 namespace hise { using namespace juce;

//==============================================================================

class FileNameImporterDialog  : public Component,
                                public LabelListener,
                                public ButtonListener
{
public:
    //==============================================================================
    FileNameImporterDialog (ModulatorSampler *sampler_, const StringArray &fileNames_);
    ~FileNameImporterDialog();

    //==============================================================================

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

			PoolReference ref(sampler->getMainController(), fileNames[i], FileHandlerBase::Samples);

			data.index = startIndex + i;
			data.files.add(ref);
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

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);

private:

	void restoreFromXml(const String& xmlData);

	XmlElement* saveAsXml();

	StringArray fileNames;
	StringArray relativeFileNames;
	StringArray tokens;

	String currentSeparator;

	OwnedArray<FileNamePartComponent> tokenPanels;

	ModulatorSampler *sampler;
	AlertWindowLookAndFeel laf;


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
	ScopedPointer<TextButton> clearButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileNameImporterDialog)
};



/** \endcond */
} // namespace hise


#endif   // __JUCE_HEADER_44A2A0BCCC20A6FE__
