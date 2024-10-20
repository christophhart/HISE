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

#ifndef __JUCE_HEADER_51F4849208819A4C__
#define __JUCE_HEADER_51F4849208819A4C__

//[Headers]     -- You can add your own extra header files here --

namespace hise { using namespace juce;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.
	\cond HIDDEN_SYMBOLS
    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class DelayEditor  : public ProcessorEditorBody,
                     public SliderListener,
                     public ButtonListener
{
public:
    //==============================================================================
    DelayEditor (ProcessorEditor *p);
    ~DelayEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void updateGui()
	{
		const bool shouldBeSynced = tempoSyncButton->getToggleState();

        
        leftSyncTimeSlider->setVisible(shouldBeSynced);
        rightSyncTimeSlider->setVisible(shouldBeSynced);

        leftTimeSlider->setVisible(!shouldBeSynced);
        rightTimeSlider->setVisible(!shouldBeSynced);
	}

	int getBodyHeight() const
	{
		return h;
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    ProcessorEditorBodyUpdater updater;

    //==============================================================================
    ScopedPointer<HiSlider> leftTimeSlider;
    ScopedPointer<HiSlider> rightTimeSlider;
    ScopedPointer<HiSlider> leftSyncTimeSlider;
    ScopedPointer<HiSlider> rightSyncTimeSlider;
    ScopedPointer<HiSlider> leftFeedbackSlider;
    ScopedPointer<HiSlider> rightFeedbackSlider;
    ScopedPointer<HiSlider> mixSlider;
    ScopedPointer<HiToggleButton> tempoSyncButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_51F4849208819A4C__
