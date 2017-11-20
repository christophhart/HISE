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

#ifndef __JUCE_HEADER_B8935CA349EC4DF8__
#define __JUCE_HEADER_B8935CA349EC4DF8__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
	\cond HIDDEN_SYMBOLS
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
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
    //[UserVariables]   -- You can add your own custom variables in this section.

	MidiProcessor *processor;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> intensitySlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransposerEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_B8935CA349EC4DF8__
