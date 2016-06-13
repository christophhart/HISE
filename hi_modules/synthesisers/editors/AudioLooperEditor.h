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

#ifndef __JUCE_HEADER_BFEB9E679CA60A44__
#define __JUCE_HEADER_BFEB9E679CA60A44__

//[Headers]     -- You can add your own extra header files here --



//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AudioLooperEditor  : public ProcessorEditorBody,
                           public Timer,
                           public AudioDisplayComponent::Listener,
                           public ComboBoxListener,
                           public ButtonListener,
                           public SliderListener
{
public:
    //==============================================================================
    AudioLooperEditor (ProcessorEditor *p);
    ~AudioLooperEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	int getBodyHeight() const override
	{

		return h;
	}

	void updateGui() override
	{
		syncToHost->updateValue();
		loopButton->updateValue();
		rootNote->updateValue();
		pitchButton->updateValue();

		rootNote->setEnabled(getProcessor()->getAttribute(AudioLooper::PitchTracking) > 0.5f);

		AudioSampleProcessor *envelope = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		if (sampleBufferContent->getSampleArea(0)->getSampleRange() != envelope->getRange())
		{
			sampleBufferContent->setRange(envelope->getRange());


		}

	}

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

	void timerCallback() override
	{
		sampleBufferContent->setPlaybackPosition(getProcessor()->getInputValue());
	};

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<AudioSampleBufferComponent> sampleBufferContent;
    ScopedPointer<Label> label;
    ScopedPointer<HiComboBox> syncToHost;
    ScopedPointer<HiToggleButton> pitchButton;
    ScopedPointer<HiToggleButton> loopButton;
    ScopedPointer<HiSlider> rootNote;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLooperEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_BFEB9E679CA60A44__
