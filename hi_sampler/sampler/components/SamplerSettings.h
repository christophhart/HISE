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

#ifndef __JUCE_HEADER_8BC78B263CB93F86__
#define __JUCE_HEADER_8BC78B263CB93F86__

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
class SamplerSettings  : public Component,
                         public Timer,
                         public LabelListener,
                         public SliderListener
{
public:
    //==============================================================================
    SamplerSettings (ModulatorSampler *s);
    ~SamplerSettings();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void timerCallback()
	{
		const double usage = sampler->getDiskUsage();
		diskSlider->setValue(usage, dontSendNotification);
	}

	int getPanelHeight() const
	{
		const bool crossFadeShown = sampler->getAttribute(ModulatorSampler::Parameters::CrossfadeGroups) > 0.5f;

		crossfadeEditor->setVisible(crossFadeShown);
		showCrossfadeLabel->setEnabled(crossFadeShown);

		return crossFadeShown ? h + 125 : h;
	}

	void updateInterface()
	{
		updateGui();
	}


	void updateGui()
	{
		if(bufferSizeEditor->getCurrentTextEditor() == nullptr)
        {
			bufferSizeEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::BufferSize)), dontSendNotification);
        }

        if(preloadBufferEditor->getCurrentTextEditor() == nullptr)
        {
            preloadBufferEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::PreloadSize)), dontSendNotification);
        }

		memoryUsageLabel->setText(sampler->getMemoryUsage(), dontSendNotification);

        if(voiceLimitEditor->getCurrentTextEditor() == nullptr)
        {
            voiceLimitEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::VoiceLimit)), dontSendNotification);
        }

        if(fadeTimeEditor->getCurrentTextEditor() == nullptr)
        {
            fadeTimeEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::KillFadeTime)), dontSendNotification);
        }

        if(voiceAmountEditor->getCurrentTextEditor() == nullptr)
        {
		    voiceAmountEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::VoiceAmount)), dontSendNotification);
        }


		if(retriggerEditor->getCurrentTextEditor() == nullptr)
        {
		    retriggerEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::SamplerRepeatMode), dontSendNotification);
        }

        if(playbackEditor->getCurrentTextEditor() == nullptr)
        {
			const int oneShot = (int)sampler->getAttribute(ModulatorSampler::OneShot);
			const int reversed = (int)sampler->getAttribute(ModulatorSampler::Reversed);

		    playbackEditor->setItemIndex(reversed * 2 + oneShot, dontSendNotification);
        }

		if (showCrossfadeLabel->getCurrentTextEditor() == nullptr)
		{
			showCrossfadeLabel->setItemIndex(sampler->getEditorState(ModulatorSampler::EditorStates::CrossfadeTableShown));
		}

		if (crossfadeGroupEditor->getCurrentTextEditor() == nullptr)
		{
			crossfadeGroupEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::CrossfadeGroups), dontSendNotification);
		}

        if(pitchTrackingEditor->getCurrentTextEditor() == nullptr)
        {
		    pitchTrackingEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::PitchTracking), dontSendNotification);
        }

        if(rrGroupEditor->getCurrentTextEditor() == nullptr)
        {
		    rrGroupEditor->setText(String((int)sampler->getAttribute(ModulatorSampler::RRGroupAmount)), dontSendNotification);
        }

		if (purgeSampleEditor->getCurrentTextEditor() == nullptr)
		{
			purgeSampleEditor->setItemIndex((int)sampler->getAttribute(ModulatorSampler::Purged), dontSendNotification);
		}

		refreshMicAmount();
	};

	void refreshMicAmount();

	void refreshTickStatesForPurgeChannel();

	ModulatorSampler *getProcessor()
	{
		return sampler;
	}


    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	ModulatorSampler *sampler;
	int currentChannel;
	int currentChannelSize;

	ScopedPointer<ModulatorSampler::Documentation> docs;

	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> fadeTimeLabel;
    ScopedPointer<Label> voiceAmountLabel;
    ScopedPointer<Label> label2;
    ScopedPointer<Label> label;
    ScopedPointer<Label> label4;
    ScopedPointer<Label> label3;
    ScopedPointer<Label> fadeTimeLabel2;
    ScopedPointer<Label> voiceLimitLabel;
    ScopedPointer<Label> bufferSizeEditor;
    ScopedPointer<Label> preloadBufferEditor;
    ScopedPointer<Label> memoryUsageLabel;
    ScopedPointer<Slider> diskSlider;
    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> voiceLimitEditor;
    ScopedPointer<Label> fadeTimeEditor;
    ScopedPointer<PopupLabel> retriggerEditor;
    ScopedPointer<Label> voiceAmountLabel2;
    ScopedPointer<Label> rrGroupEditor;
    ScopedPointer<Label> playbackModeDescription;
    ScopedPointer<PopupLabel> playbackEditor;
    ScopedPointer<Label> playbackModeDescription2;
    ScopedPointer<PopupLabel> pitchTrackingEditor;
    ScopedPointer<Label> voiceLimitLabel2;
    ScopedPointer<PopupLabel> crossfadeGroupEditor;
    ScopedPointer<TableEditor> crossfadeEditor;
    ScopedPointer<Label> voiceLimitLabel3;
    ScopedPointer<PopupLabel> showCrossfadeLabel;
    ScopedPointer<Label> voiceLimitLabel4;
    ScopedPointer<PopupLabel> purgeSampleEditor;
    ScopedPointer<Label> channelAmountLabel3;
    ScopedPointer<PopupLabel> purgeChannelLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSettings)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_8BC78B263CB93F86__
