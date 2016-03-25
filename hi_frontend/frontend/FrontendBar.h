#ifndef __JUCE_HEADER_59561CDF26DED86E__
#define __JUCE_HEADER_59561CDF26DED86E__

/** The bar that is displayed for every FrontendProcessorEditor */
class FrontendBar  : public Component,
                     public Timer,
                     public ButtonListener
{
public:
    
    FrontendBar (FrontendProcessor *p);
    ~FrontendBar();

	void buttonClicked(Button *b) override;

	void timerCallback()
	{
		const float l = fp->getMainSynthChain()->getDisplayValues().outL;
		const float r = fp->getMainSynthChain()->getDisplayValues().outR;

		outMeter->setPeak(l, r);

		cpuPeak = jmax<int>(fp->getCpuUsage(), cpuPeak);



		if(cpuUpdater.shouldUpdate())
		{

			numVoices = "Voices: " + String(fp->getNumActiveVoices());
			voiceLabel->setText(numVoices, dontSendNotification);
			cpuSlider->setPeak((float)cpuPeak / 100.0f);
			cpuPeak = 0;
		}
	}

    void paint (Graphics& g);
    void resized();



private:

	FrontendProcessor *fp;

	ScopedPointer<VuMeter> cpuSlider;

	ScopedPointer<Label> voiceLabel;

	ScopedPointer<ShapeButton> panicButton;
	ScopedPointer<ShapeButton> infoButton;

	int cpuPeak;

	String numVoices;

	UpdateMerger cpuUpdater;

	ScopedPointer<TooltipBar> tooltipBar;

    //==============================================================================
    ScopedPointer<VuMeter> outMeter;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrontendBar)
};


#endif  
