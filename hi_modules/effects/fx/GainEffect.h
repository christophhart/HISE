/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
