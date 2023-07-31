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

	void timerCallback();

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


	void updateGui();;

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

    void attachLabel(Graphics& g, Component& c, const String& text);

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	ModulatorSampler *sampler;
	int currentChannel;
	int currentChannelSize;

	ScopedPointer<ModulatorSampler::Documentation> docs;

	int h;
    //[/UserVariables]

    Rectangle<float> columns[3];

    //==============================================================================
    ScopedPointer<Label> bufferSizeEditor;
    ScopedPointer<Label> preloadBufferEditor;
    ScopedPointer<PopupLabel> purgeChannelLabel;

    ScopedPointer<Label> memoryUsageLabel;
    ScopedPointer<Slider> diskSlider;
    ScopedPointer<PopupLabel> purgeSampleEditor;

    ScopedPointer<Label> voiceAmountEditor;
    ScopedPointer<Label> voiceLimitEditor;
    ScopedPointer<Label> fadeTimeEditor;
    
    ScopedPointer<Label> rrGroupEditor;
    ScopedPointer<PopupLabel> crossfadeGroupEditor;
    ScopedPointer<PopupLabel> showCrossfadeLabel;

    ScopedPointer<PopupLabel> pitchTrackingEditor;
    ScopedPointer<PopupLabel> retriggerEditor;
    ScopedPointer<PopupLabel> playbackEditor;

    ScopedPointer<PopupLabel> timestretchEditor;
    ScopedPointer<Slider> stretchRatioSlider;

    ScopedPointer<TableEditor> crossfadeEditor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSettings)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_8BC78B263CB93F86__
