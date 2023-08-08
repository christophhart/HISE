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

#ifndef __JUCE_HEADER_7FE6E8987AD0E636__
#define __JUCE_HEADER_7FE6E8987AD0E636__

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
class PitchWheelEditorBody  : public ProcessorEditorBody,
                              public ButtonListener,
                              public SliderListener
{
public:
    //==============================================================================
    PitchWheelEditorBody (ProcessorEditor *p);
    ~PitchWheelEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		smoothingSlider->updateValue();
		useTableButton->setToggleState(pm->getAttribute(PitchwheelModulator::UseTable) == 1.0f, dontSendNotification);
		invertedButton->setToggleState(pm->getAttribute(PitchwheelModulator::Inverted) == 1.0f, dontSendNotification);
	};

	int getBodyHeight() const override
	{
		return tableUsed ? h : 110;
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	PitchwheelModulator *pm;

	int h;
	bool tableUsed;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<ToggleButton> invertedButton;
    ScopedPointer<HiSlider> smoothingSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchWheelEditorBody)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_7FE6E8987AD0E636__
