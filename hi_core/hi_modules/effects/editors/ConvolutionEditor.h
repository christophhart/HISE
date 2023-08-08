/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --

namespace hise { using namespace juce;


class FadeoutDisplay : public Component
{
public:

	FadeoutDisplay()
	{
		setInterceptsMouseClicks(false, false);

		setFadeoutValue(0.4f, {0.0f, 1.0f});
	}

	void setFadeoutValue(float newValue, Range<float> normalizedSampleRange)
	{
		damping = jlimit<float>(0.0f, 1.0f, newValue);
		range = normalizedSampleRange;

		p.clear();
		p.startNewSubPath(0.0f, 0.0f);


		p.quadraticTo(0.0f, 1.0f - damping, 1.0f, 1.0f - damping);
		p.lineTo(1.0f, 1.0f);
		p.lineTo(0.0f, 1.0f);
		p.closeSubPath();

		auto x = (float)getWidth() * range.getStart();
		auto w = (float)getWidth() * range.getLength();

		auto b = p.getBounds();

		if(b.getWidth() > 0 && b.getHeight() > 0)
			p.scaleToFit(x, 0.0f, w, (float)getHeight(), false);
	}

	void resized() override
	{
		setFadeoutValue(damping, range);
	}

	void paint(Graphics& g)
	{
		g.setColour(Colours::white.withAlpha(0.08f));
		g.fillPath(p);
	}

private:

	float damping = 0.4f;
	Range<float> range;

	Path p;

};

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
                           public Slider::Listener,
                           public Button::Listener
{
public:
    //==============================================================================
    ConvolutionEditor (ProcessorEditor *p);
    ~ConvolutionEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.


	void rangeChanged(AudioDisplayComponent *broadcaster, int changedArea)
	{
		
	};



	void updateGui() override
	{
		drySlider->updateValue();
		wetSlider->updateValue();
		predelaySlider->updateValue();
		dampingSlider->updateValue();
		hiCutSlider->updateValue();


		resetButton->updateValue();
		backgroundButton->updateValue();

		AudioSampleProcessor *sampleProcessor = dynamic_cast<AudioSampleProcessor*>(getProcessor());

		const float numSamples = (float)sampleProcessor->getAudioSampleBuffer().getNumSamples();
		const float dampingValue = Decibels::decibelsToGain(getProcessor()->getAttribute(ConvolutionEffect::Damping));

		auto numSamplesToUse = jmax(numSamples, 1.0f);

		auto sRange = sampleProcessor->getBuffer().getCurrentRange();

		const auto range = Range<float>((float)sRange.getStart() / numSamplesToUse, (float)sRange.getEnd() / numSamplesToUse);

		fadeoutDisplay->setFadeoutValue(dampingValue, range);
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

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;

	ScopedPointer<FadeoutDisplay> fadeoutDisplay;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> drySlider;
    ScopedPointer<HiSlider> wetSlider;
    ScopedPointer<VuMeter> dryMeter;
    ScopedPointer<VuMeter> wetMeter;
    ScopedPointer<MultiChannelAudioBufferDisplay> impulseDisplay;
    ScopedPointer<HiToggleButton> resetButton;
    ScopedPointer<Label> label;
    ScopedPointer<HiToggleButton> backgroundButton;
    ScopedPointer<HiSlider> predelaySlider;
    ScopedPointer<HiSlider> dampingSlider;
    ScopedPointer<HiSlider> hiCutSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolutionEditor)
};

//[EndFile] You can add extra defines here...

/** \endcond */
} // namespace hise
//[/EndFile]
