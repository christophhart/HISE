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

#ifndef __JUCE_HEADER_980D08636A357E76__
#define __JUCE_HEADER_980D08636A357E76__

//[Headers]     -- You can add your own extra header files here --

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class GainCollectorEditor  : public ProcessorEditorBody,
                             public Timer,
                             public SliderListener,
                             public ComboBoxListener
{
public:
    //==============================================================================
    GainCollectorEditor (ProcessorEditor *pe);
    ~GainCollectorEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{};

	int getBodyHeight() const override { return 150; }

	void timerCallback() override
	{
		gainMeter->setPeak(dynamic_cast<GainCollector*>(getProcessor())->getCurrentGain());
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> smoothSlider;
    ScopedPointer<VuMeter> gainMeter;
    ScopedPointer<HiComboBox> modeSelector;
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainCollectorEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_980D08636A357E76__
