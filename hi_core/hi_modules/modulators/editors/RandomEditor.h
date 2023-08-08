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

#ifndef __JUCE_HEADER_101C7792A4FC7C8E__
#define __JUCE_HEADER_101C7792A4FC7C8E__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

#define RandomEditor(x) ModulatorEditor(x, new RandomEditorBody(x))
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class RandomEditorBody  : public ProcessorEditorBody,
                          public ButtonListener
{
public:
    //==============================================================================
    RandomEditorBody (ProcessorEditor *p);
    ~RandomEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		tableUsed = rm->getAttribute(RandomModulator::UseTable) == 1.0f;

		useTableButton->setToggleState(tableUsed, dontSendNotification);

	};

	int getBodyHeight() const override
	{
		return tableUsed ? h : 70;
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	bool tableUsed;
	RandomModulator *rm;

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RandomEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_101C7792A4FC7C8E__
