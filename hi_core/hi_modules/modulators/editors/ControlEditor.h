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

#ifndef __JUCE_HEADER_CEB145587AFA4A1E__
#define __JUCE_HEADER_CEB145587AFA4A1E__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;

#define ControlEditor(x) ProcessorEditor(x, new ControlEditorBody(x))
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ControlEditorBody  : public ProcessorEditorBody,
                           public ButtonListener,
                           public SliderListener
{
public:
    //==============================================================================
    ControlEditorBody (ProcessorEditor *p);
    ~ControlEditorBody();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		learnButton->setToggleState(dynamic_cast<ControlModulator*>(getProcessor())->learnModeActive(), dontSendNotification);

		controllerNumberSlider->updateValue();
		defaultSlider->updateValue();
		smoothingSlider->updateValue();
		useTableButton->setToggleState(cm->getAttribute(ControlModulator::UseTable) == 1.0f, dontSendNotification);
		invertedButton->setToggleState(cm->getAttribute(ControlModulator::Inverted) == 1.0f, dontSendNotification);
	};

	int getBodyHeight() const override
	{
		return tableUsed ? h : 105;
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	ControlModulator *cm;

	bool tableUsed;

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<TableEditor> midiTable;
    ScopedPointer<ToggleButton> useTableButton;
    ScopedPointer<ToggleButton> invertedButton;
    ScopedPointer<HiSlider> controllerNumberSlider;
    ScopedPointer<HiSlider> smoothingSlider;
    ScopedPointer<ToggleButton> learnButton;
    ScopedPointer<HiSlider> defaultSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlEditorBody)
};

//[EndFile] You can add extra defines here...

/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_CEB145587AFA4A1E__
