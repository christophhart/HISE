/*
  ==============================================================================

    GainEffect.h
    Created: 30 Oct 2014 8:57:01pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef GAINEFFECT_H_INCLUDED
#define GAINEFFECT_H_INCLUDED

/** A simple gain effect that allows time variant modulation. */
class GainEffect: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SimpleGain", "Simple Gain")

	enum InternalChains
	{
		GainChain = 0,
        DelayChain,
        WidthChain,
		numInternalChains
	};

	enum EditorStates
	{
		GainChainShown = Processor::numEditorStates,
        DelayChainShown,
        WidthChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
        Delay,
        Width,
		numParameters
	};

	GainEffect(MainController *mc, const String &uid);;

	~GainEffect()
	{};

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	
	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int processorIndex) override
    {
        switch(processorIndex)
        {
            case GainChain: return gainChain;
            case DelayChain: return delayChain;
            case WidthChain: return widthChain;
        }
    };
    
	const Processor *getChildProcessor(int processorIndex) const override
    {
        switch(processorIndex)
        {
            case GainChain: return gainChain;
            case DelayChain: return delayChain;
            case WidthChain: return widthChain;
        }
    };
    
	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples);;
	void prepareToPlay(double sampleRate, int samplesPerBlock);
	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override {};

    void setDelayTime(float newDelayInMilliseconds)
    {
        delay = newDelayInMilliseconds;
        leftDelay.setDelayTimeSeconds(delay/1000.0f);
        rightDelay.setDelayTimeSeconds(delay/1000.0f);
    }
    
private:

	float gain;
    float delay;
	
	ScopedPointer<ModulatorChain> gainChain;
    ScopedPointer<ModulatorChain> delayChain;
    ScopedPointer<ModulatorChain> widthChain;

	Smoother smoother;

	AudioSampleBuffer gainBuffer;
    AudioSampleBuffer delayBuffer;
    AudioSampleBuffer widthBuffer;
    
    MidSideDecoder msDecoder;
    
    DelayLine leftDelay;
    DelayLine rightDelay;
    
};


#endif  // GAINEFFECT_H_INCLUDED
