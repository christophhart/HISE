/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_2F150CA0CBAA7764__
#define __JUCE_HEADER_2F150CA0CBAA7764__

//[Headers]     -- You can add your own extra header files here --
 
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class GainMatcherEditor  : public ProcessorEditorBody,
                           public ButtonListener,
                           public ComboBoxListener
{
public:
    //==============================================================================
    GainMatcherEditor (ProcessorEditor *pe);
    ~GainMatcherEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		tableUsed = getProcessor()->getAttribute(GlobalModulator::UseTable) == 1.0f;

		useTableButton->updateValue();

		setItemEntry();

	};

	int getBodyHeight() const override
	{
		return tableUsed ? h : 60;
	};

	void setItemEntry();

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	bool tableUsed;
	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<HiToggleButton> useTableButton;
    ScopedPointer<ComboBox> collectorSelector;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainMatcherEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_2F150CA0CBAA7764__
