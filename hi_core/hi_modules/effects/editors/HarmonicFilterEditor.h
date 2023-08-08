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

#ifndef __JUCE_HEADER_1CFD59AA7124C2BC__
#define __JUCE_HEADER_1CFD59AA7124C2BC__

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
class HarmonicFilterEditor  : public ProcessorEditorBody,
                              public Timer,
                              public ComboBoxListener,
                              public SliderListener
{
public:
    //==============================================================================
    HarmonicFilterEditor (ProcessorEditor *p);
    ~HarmonicFilterEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override
	{
		return h;
	}

	void updateGui() override
	{
		filterNumbers->updateValue();

		qSlider->updateValue();

		semiToneTranspose->updateValue();

		const double sliderValue = (float)getProcessor()->getAttribute(HarmonicFilter::Crossfade) * 2.0 - 1.0;

		crossfadeSlider->setValue(sliderValue, dontSendNotification);
	};

	void timerCallback() override;

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

	BiPolarSliderLookAndFeel laf;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SliderPack> sliderPackA;
    ScopedPointer<SliderPack> sliderPackB;
    ScopedPointer<SliderPack> sliderPackMix;
    ScopedPointer<Label> label2;
    ScopedPointer<Label> label3;
    ScopedPointer<Label> label4;
    ScopedPointer<HiComboBox> filterNumbers;
    ScopedPointer<HiSlider> qSlider;
    ScopedPointer<Slider> crossfadeSlider;
    ScopedPointer<HiSlider> semiToneTranspose;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicFilterEditor)
};

//[EndFile] You can add extra defines here...

} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_1CFD59AA7124C2BC__
