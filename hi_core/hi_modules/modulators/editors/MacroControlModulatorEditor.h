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

#ifndef __JUCE_HEADER_531DDB149D1700D4__
#define __JUCE_HEADER_531DDB149D1700D4__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MacroControlModulatorEditorBody  : public ProcessorEditorBody,
                                         public ButtonListener,
                                         public SliderListener,
                                         public ComboBoxListener
{
public:
    //==============================================================================
    MacroControlModulatorEditorBody (ProcessorEditor *p);
    ~MacroControlModulatorEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override
	{
		return tableUsed ? h : 72;
	}

	void updateGui() override
	{
		macroSelector->setSelectedId((int)getProcessor()->getAttribute(MacroModulator::MacroIndex) + 2, dontSendNotification);

		smoothingSlider->updateValue();

		tableUsed = getProcessor()->getAttribute(MacroModulator::UseTable) == 1.0f;
		useTableButton->setToggleState(tableUsed, dontSendNotification);



	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

	bool tableUsed;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<TableEditor> valueTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<HiSlider> smoothingSlider;
    ScopedPointer<ComboBox> macroSelector;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroControlModulatorEditorBody)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_531DDB149D1700D4__
