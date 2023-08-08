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

#ifndef __JUCE_HEADER_2C75A49110BA0876__
#define __JUCE_HEADER_2C75A49110BA0876__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
class FileImportDialog  : public Component,
                          public ButtonListener
{
public:
    //==============================================================================
    FileImportDialog (Processor *p);
    ~FileImportDialog();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	enum ImportMode
	{
		FileName = 0,
		PitchDetection,
		DropPoint,
		numImportModes
	};

	ImportMode getImportMode() const
	{
		return mode;
	};

	void setMode(ImportMode m)
	{
		mode = m;

		switch(m)
		{
		case FileName:			fileNameButton->setToggleState(true, sendNotification); break;
		case DropPoint:			dropPointButton->setToggleState(true, sendNotification); break;
		case PitchDetection:	pitchDetectionButton->setToggleState(true, sendNotification); break;
        case numImportModes:   break;
		}
	};

	bool useMetadata() const
	{
		return useMetadataForMapping;
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	ImportMode mode;
	bool useMetadataForMapping;

	AlertWindowLookAndFeel laf;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ToggleButton> metadataButton;
    ScopedPointer<ToggleButton> fileNameButton;
    ScopedPointer<ToggleButton> pitchDetectionButton;
    ScopedPointer<ToggleButton> dropPointButton;
    ScopedPointer<Label> descriptionLabel;
    ScopedPointer<Label> descriptionLabel2;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileImportDialog)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_2C75A49110BA0876__
