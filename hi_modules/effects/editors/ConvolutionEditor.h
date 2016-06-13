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

#ifndef __JUCE_HEADER_7DB53C0A003C421A__
#define __JUCE_HEADER_7DB53C0A003C421A__

//[Headers]     -- You can add your own extra header files here --
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

	\cond HIDDEN_SYMBOLS

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ConvolutionEditor  : public ProcessorEditorBody,
                           public Timer,
                           public AudioDisplayComponent::Listener,
                           public SliderListener,
                           public ButtonListener
{
public:
    //==============================================================================
    ConvolutionEditor (ProcessorEditor *p);
    ~ConvolutionEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

#pragma warning( push )
#pragma warning( disable: 4100 )

	void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
	{
		jassert(broadcaster == impulseDisplay);

		Range<int> newRange = impulseDisplay->getSampleArea(changedArea)->getSampleRange();

		AudioSampleProcessor *sampleProcessor = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		sampleProcessor->setLoadedFile(impulseDisplay->getCurrentlyLoadedFileName());
		sampleProcessor->setRange(newRange);
	};

#pragma warning( pop )

	void updateGui() override
	{
		drySlider->updateValue();
		wetSlider->updateValue();

		resetButton->updateValue();

		AudioSampleProcessor *sampleProcessor = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		if(impulseDisplay->getSampleArea(0)->getSampleRange() != sampleProcessor->getRange())
		{
			impulseDisplay->setRange(sampleProcessor->getRange());
		}
	};

	void timerCallback()
	{
		EffectProcessor::DisplayValues d = dynamic_cast<EffectProcessor*>(getProcessor())->getDisplayValues();

		dryMeter->setPeak(d.inL, d.inR);
		wetMeter->setPeak(d.outL, d.outR);
	}

	int getBodyHeight() const override
	{
		return h;
	}


    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> drySlider;
    ScopedPointer<HiSlider> wetSlider;
    ScopedPointer<VuMeter> dryMeter;
    ScopedPointer<VuMeter> wetMeter;
    ScopedPointer<AudioSampleBufferComponent> impulseDisplay;
    ScopedPointer<HiToggleButton> resetButton;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolutionEditor)
};

//[EndFile] You can add extra defines here...

/** \endcond */
//[/EndFile]

#endif   // __JUCE_HEADER_7DB53C0A003C421A__
