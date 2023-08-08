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

#ifndef __JUCE_HEADER_BE66E0B8481CC7F6__
#define __JUCE_HEADER_BE66E0B8481CC7F6__

//[Headers]     -- You can add your own extra header files here --
namespace hise { using namespace juce;



class LimiterGraph: public Component
{
public:
	LimiterGraph(const MdaEffectWrapper *effect):
		limiterEffect(dynamic_cast<const MdaLimiterEffect*>(effect))
	{

	};

	void paint(Graphics &g)
	{
		const float w = (float)getWidth();
		const float h = (float)getHeight();

		GlobalHiseLookAndFeel::fillPathHiStyle(g, dynamicPath, (int)w, (int)h);

		g.setColour(Colours::lightgrey.withAlpha(0.1f));

		for(float i = 0.0f; i < w; i += w / 8.0f)
		{
			g.drawLine(i, 0.0f, i, h, 0.5f);


		}

		for(float i = 0.0f; i < h; i += h / 8.0f)
		{
			g.drawLine(0.0f, i, w, i);


		}




	};

	void refreshGraph()
	{
		const float gain = limiterEffect->getAttribute(MdaLimiterEffect::Output);
		const float thresh = limiterEffect->getAttribute(MdaLimiterEffect::Threshhold);

		const float threshLevel = 1.0f - (gain-0.5f) - thresh;

		const float w = (float)getWidth();
		const float h = (float)getHeight();

		const bool softKnee = limiterEffect->getAttribute(MdaLimiterEffect::Knee) == 1.0f;

		dynamicPath.clear();

		dynamicPath.startNewSubPath(0.0f, h);

		dynamicPath.lineTo(0.0f, (1.0f - (gain-0.5f)) * h);

		if(softKnee)
		{
			dynamicPath.quadraticTo(thresh*w, threshLevel * h, w, threshLevel * h);


		}
		else
		{
			dynamicPath.lineTo(thresh*w, threshLevel * h);
			dynamicPath.lineTo(w, threshLevel * h);
		}

		dynamicPath.lineTo(w, h);
		dynamicPath.closeSubPath();

		repaint();
	};

	void resized()
	{
		refreshGraph();
	}



private:

	Path dynamicPath;

	const MdaLimiterEffect *limiterEffect;
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
class MdaLimiterEditor  : public ProcessorEditorBody,
                          public Timer,
                          public SliderListener,
                          public ButtonListener
{
public:
    //==============================================================================
    MdaLimiterEditor (ProcessorEditor *p);
    ~MdaLimiterEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void updateGui() override
	{
		for(int i = 0; i < MdaLimiterEffect::numParameters; i++)
		{
			if(getSlider(i) != nullptr) getSlider(i)->updateValue();//setValue(getProcessor()->getAttribute(i), dontSendNotification);
			else softKneeButton->setToggleState(getProcessor()->getAttribute(i) == 1.0f, dontSendNotification);
		}





		limiterGraph->refreshGraph();

	}

	void timerCallback() override
	{
		MdaEffectWrapper::DisplayValues v = dynamic_cast<MdaEffectWrapper*>(getProcessor())->getDisplayValues();

		inMeter->setPeak(v.inL, v.inR);
		outMeter->setPeak(v.outL, v.outR);
	};

	int getBodyHeight() const override
	{
		return h;
	};

	MdaSlider *getSlider(int parameterIndex)
	{
		switch(parameterIndex)
		{
		case MdaLimiterEffect::Attack:	return attackSlider;
		case MdaLimiterEffect::Release:	return releaseSlider;
		case MdaLimiterEffect::Threshhold:	return treshSlider;
		case MdaLimiterEffect::Output:	return outputSlider;
		case MdaLimiterEffect::Knee:	return nullptr;
		default:						return nullptr;
		}
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
    ScopedPointer<MdaSlider> attackSlider;
    ScopedPointer<MdaSlider> outputSlider;
    ScopedPointer<MdaSlider> treshSlider;
    ScopedPointer<MdaSlider> releaseSlider;
    ScopedPointer<LimiterGraph> limiterGraph;
    ScopedPointer<ToggleButton> softKneeButton;
    ScopedPointer<VuMeter> inMeter;
    ScopedPointer<VuMeter> outMeter;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MdaLimiterEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_BE66E0B8481CC7F6__
