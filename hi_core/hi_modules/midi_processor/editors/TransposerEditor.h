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

#pragma once

namespace hise { using namespace juce;

class TransposerEditor  : public ProcessorEditorBody,
                          public SliderListener
{
public:
    //==============================================================================
    TransposerEditor (ProcessorEditor *p);
    ~TransposerEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		intensitySlider->updateValue();
	};

	int getBodyHeight() const override
	{
		return 50;

	};


	void sliderDragStarted(Slider *)
	{
		MidiProcessorChain* chain = dynamic_cast<MidiProcessorChain*>(ProcessorHelpers::findParentProcessor(getProcessor(), false));

		if (chain != nullptr)
		{
			chain->sendAllNoteOffEvent();
		}
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
	MidiProcessor *processor;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> intensitySlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransposerEditor)
};

class ChokeGroupEditor : public ProcessorEditorBody
{
public:
	ChokeGroupEditor(ProcessorEditor *p);
	
	void updateGui() override
	{
		groupSlider->updateValue();
		loSlider->updateValue();
		hiSlider->updateValue();
		killButton->updateValue();
	};

	int getBodyHeight() const override
	{
		return 50;
	};

	
	void resized();

private:
	MidiProcessor *processor;
	ScopedPointer<HiSlider> groupSlider;
	ScopedPointer<HiSlider> loSlider;
	ScopedPointer<HiSlider> hiSlider;
	ScopedPointer<HiToggleButton> killButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChokeGroupEditor)
};

} // namespace hise
