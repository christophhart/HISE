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

#ifndef __JUCE_HEADER_5F604E9692DC5E1A__
#define __JUCE_HEADER_5F604E9692DC5E1A__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
	\cond HIDDEN_SYMBOLS
    A editor component for the VelocityModulator
                                                                    //[/Comments]
*/
class VelocityEditorBody  : public ProcessorEditorBody,
                            public ButtonListener
{
public:
    //==============================================================================
    VelocityEditorBody (ProcessorEditor *p);
    ~VelocityEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		//intensitySlider->setValue(vm->getIntensity(), dontSendNotification);

		tableUsed = vm->getAttribute(VelocityModulator::UseTable) == 1.0f;

        decibelButton->updateValue();
		useTableButton->setToggleState(tableUsed, dontSendNotification);
		invertedButton->setToggleState(vm->getAttribute(VelocityModulator::Inverted) == 1.0f, dontSendNotification);
	};

	int getBodyHeight() const override
	{
		return tableUsed ? h : 70;
	};



    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	VelocityModulator *vm;

	bool tableUsed;

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<ToggleButton> invertedButton;
    ScopedPointer<Label> label;
    ScopedPointer<HiToggleButton> decibelButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityEditorBody)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_5F604E9692DC5E1A__
