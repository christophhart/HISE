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

#ifndef __JUCE_HEADER_BBD5AE674E02BB6A__
#define __JUCE_HEADER_BBD5AE674E02BB6A__

//[Headers]     -- You can add your own extra header files here --

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AudioFileEnvelopeEditor  : public ProcessorEditorBody,
                                 public Timer,
                                 public AudioDisplayComponent::Listener,
                                 public SliderListener,
                                 public ButtonListener,
                                 public ComboBoxListener
{
public:
    //==============================================================================
    AudioFileEnvelopeEditor (BetterProcessorEditor *p);
    ~AudioFileEnvelopeEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override {return h;};

#pragma warning( push )
#pragma warning( disable: 4100 )

	void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
	{
		jassert(broadcaster == sampleBufferContent);

		Range<int> newRange = sampleBufferContent->getSampleArea(changedArea)->getSampleRange();

		AudioSampleProcessor *envelope = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		envelope->setLoadedFile(sampleBufferContent->getCurrentlyLoadedFileName());
		envelope->setRange(newRange);
	};

#pragma warning( pop )

	void updateGui() override
	{
		AudioFileEnvelope::EnvelopeFollowerMode mode = (AudioFileEnvelope::EnvelopeFollowerMode)(int)getProcessor()->getAttribute(AudioFileEnvelope::Mode);

		attackSlider->updateValue();
		releaseSlider->updateValue();
		modeSelector->updateValue();
		retriggerButton->updateValue();
		gainSlider->updateValue();
		smoothSlider->updateValue();
		offsetSlider->updateValue();
		rampSlider->updateValue();

		AudioFileEnvelope *envelope = dynamic_cast<AudioFileEnvelope*>(getProcessor());

		if(sampleBufferContent->getSampleArea(0)->getSampleRange() != envelope->getRange())
		{
			sampleBufferContent->setRange(envelope->getRange());


		}

		switch(mode)
		{
		case AudioFileEnvelope::SimpleLP:	attackSlider->setVisible(false);
											releaseSlider->setVisible(false);
											rampSlider->setVisible(false);
											smoothSlider->setVisible(true);
											gainSlider->setVisible(true);
											break;
		case AudioFileEnvelope::RampedAverage:	attackSlider->setVisible(false);
											releaseSlider->setVisible(false);
											rampSlider->setVisible(true);
											break;
		case AudioFileEnvelope::AttackRelease:	attackSlider->setVisible(true);
											releaseSlider->setVisible(true);
											rampSlider->setVisible(false);
											break;


		}
	}

	void timerCallback() override
	{
		sampleBufferContent->setPlaybackPosition(getProcessor()->getInputValue());
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> label;
    ScopedPointer<HiSlider> smoothSlider;
    ScopedPointer<HiToggleButton> retriggerButton;
    ScopedPointer<AudioSampleBufferComponent> sampleBufferContent;
    ScopedPointer<HiComboBox> modeSelector;
    ScopedPointer<HiSlider> gainSlider;
    ScopedPointer<HiSlider> attackSlider;
    ScopedPointer<HiSlider> releaseSlider;
    ScopedPointer<HiSlider> offsetSlider;
    ScopedPointer<HiSlider> rampSlider;
    ScopedPointer<HiComboBox> syncToHost;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileEnvelopeEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_BBD5AE674E02BB6A__
