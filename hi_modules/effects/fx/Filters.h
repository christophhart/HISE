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

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

#include <math.h>

#define USE_STATE_VARIABLE_FILTERS 1


class MoogFilter
{
public:

	enum Mode
	{
		OnePole = 0,
		TwoPole,
		FourPole,
		numModes
	};

	MoogFilter()
	{
		sampleRate = 44100.0;
		reset();
		setCoefficients(20000, 1.0);
	}

	void reset()
	{
		in1 = in2 = in3 = in4 = 0.0;
		out1 = out2 = out3 = out4 = 0.0;
	}

	void setMode(int newMode)
	{
		mode = (Mode)newMode;
	};

	void setCoefficients(double frequency, double q)
	{
		

		fc = frequency / (0.5 *sampleRate);
		res = q / 2.0;
		if (res > 4.0) res = 4.0;
		f = fc * 1.16;
		fss = (f * f) * (f * f);
		invF = 1.0 - f;
		fb = res * (1.0 - 0.15 * f * f);
	}

	void setSampleRate(double newSampleRate)
	{
		sampleRate = newSampleRate;
		
	}

	void processSamples(float *buffer, int numSamples)
	{
		for (int i = 0; i < numSamples; i++)
		{
			double input = buffer[i];
			
			input -= out4 * fb;
			input *= 0.35013 * fss;
			out1 = input + 0.3 * in1 + invF * out1; // Pole 1
			in1 = input;
			out2 = out1 + 0.3 * in2 + invF * out2; // Pole 2
			in2 = out1;
			out3 = out2 + 0.3 * in3 + invF * out3; // Pole 3
			in3 = out2;
			out4 = out3 + 0.3 * in4 + invF * out4; // Pole 4
			in4 = out3;
			buffer[i] = 2.0f * (float)out4;
		}
	}

private:

	Mode mode;

	double in1, in2, in3, in4;
	double out1, out2, out3, out4;

	double f;
	double fss;
	double invF;
	double fb;
	double fc;
	double res;
	double sampleRate;

};


#if USE_STATE_VARIABLE_FILTERS

class StateVariableFilter
{

public:

	enum FilterType
	{ 
		LP= 0,
		HP,
		BP,
		NOTCH,
		numTypes
	};

	StateVariableFilter()
	{
		reset();
	}

	void reset() 
	{
		v0z = v1 = v2 = 0.0f;
	};

	void setType(int t) { type = t; }
	void setSamplerate(float sampleRate) { sample_rate = sampleRate; }
	void setCoefficients(float fc, float res)
	{
		float g = tan(float_Pi * fc / sample_rate);
		//float damping = 1.0f / res;
		//k = damping;
		k = 1.0f - 0.99f * res;
		float ginv = g / (1.0f + g * (g + k));
		g1 = ginv;
		g2 = 2.0f * (g + k) * ginv;
		g3 = g * ginv;
		g4 = 2.0f * ginv;
		
	}

	void processSamples(float* buffer, int samples)
	{
		switch (type) {
		case LP:
			for (int i = 0; i < samples; i++) 
			{	
				float v0 = buffer[i]; 
				float v1z = v1; 
				float v2z = v2;
				float v3 = v0 + v0z - 2.0f * v2z;
				v1 += g1 * v3 - g2 * v1z;
				v2 += g3 * v3 + g4 * v1z;
				v0z = v0;
				buffer[i] = v2; 
			}
			break;
		case BP:
			for (int i = 0; i < samples; i++)
			{
				float v0 = buffer[i];
				float v1z = v1;
				float v2z = v2;
				float v3 = v0 + v0z - 2.0f * v2z;
				v1 += g1 * v3 - g2 * v1z;
				v2 += g3 * v3 + g4 * v1z;
				v0z = v0;
				buffer[i] = v1;
			}
			
			break;
		case HP:
			for (int i = 0; i < samples; i++)
			{
				float v0 = buffer[i];
				float v1z = v1;
				float v2z = v2;
				float v3 = v0 + v0z - 2.0f * v2z;
				v1 += g1 * v3 - g2 * v1z;
				v2 += g3 * v3 + g4 * v1z;
				v0z = v0;
				buffer[i] = v0 - k * v1 - v2;
			}
			break;
		case NOTCH:
			for (int i = 0; i < samples; i++)
			{
				float v0 = buffer[i];
				float v1z = v1;
				float v2z = v2;
				float v3 = v0 + v0z - 2.0f * v2z;
				v1 += g1 * v3 - g2 * v1z;
				v2 += g3 * v3 + g4 * v1z;
				v0z = v0;
				buffer[i] = v0 - k * v1;
			}
			break;
		}
	}

private:
	float sample_rate;
	int type = 0;

	float v0z, v1, v2;
	float k, g1, g2, g3, g4;

};

#endif

class FilterEffect
{
public:
    
    
    void setRenderQuality(int powerOfTwo)
    {
        if(powerOfTwo != 0 && isPowerOfTwo(powerOfTwo))
        {
            quality = powerOfTwo;
        }
        
    }
    
    int getSampleAmountForRenderQuality() const
    {
        return quality;
    }
    
    virtual ~FilterEffect() {};

	virtual IIRCoefficients getCurrentCoefficients() const {return currentCoefficients;};

protected:

	IIRCoefficients currentCoefficients;
    
    int quality;

};

class MonoFilterEffect: public MonophonicEffectProcessor,
						public FilterEffect
{
public:

	SET_PROCESSOR_NAME("MonophonicFilter", "Monophonic Filter");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
		Frequency,
		Q,
		Mode,
        Quality,
		numEffectParameters
	};

	enum FilterMode
	{
		LowPass,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		ResoLow,
		StateVariableLP,
		StateVariableHP,
		MoogLP
	};

	MonoFilterEffect(MainController *mc, const String &id);;

	void setUseInternalChains(bool shouldBeUsed) { useInternalChains = shouldBeUsed; };
	void setUseFixedFrequency(bool shouldUseFixedFrequency) { useFixedFrequency = shouldUseFixedFrequency; }

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
    
    void processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples);
	
	bool hasTail() const override {return false; };

	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };
	Processor *getChildProcessor(int processorIndex) override { return processorIndex == FrequencyChain ? freqChain : gainChain; };
	const Processor *getChildProcessor(int processorIndex) const override { return processorIndex == FrequencyChain ? freqChain : gainChain; };
	AudioSampleBuffer &getBufferForChain(int chainIndex) { return chainIndex == FrequencyChain ? freqBuffer : gainBuffer; };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;
	
	static IIRCoefficients makeResoLowPass(double sampleRate, double cutoff, double q);;

private:

	void setMode(int filterMode);

	void calcCoefficients();

	bool useInternalChains;
	bool useFixedFrequency;

	ScopedPointer<ModulatorChain> freqChain;
	ScopedPointer<ModulatorChain> gainChain;

	AudioSampleBuffer freqBuffer;
	AudioSampleBuffer gainBuffer;

	friend class PolyFilterEffect;
	friend class HarmonicFilter;

	bool changeFlag;

	double currentFreq;
	float currentGain;

	double freq;
	double q;
	float gain;
	FilterMode mode;

	bool useStateVariableFilters;

	StateVariableFilter stateFilterL;
	StateVariableFilter stateFilterR;

	IIRFilter filterL;
	IIRFilter filterR;

	MoogFilter moogL;
	MoogFilter moogR;

};



class PolyFilterEffect: public VoiceEffectProcessor,
						public FilterEffect
{
public:

	SET_PROCESSOR_NAME("PolyphonicFilter", "Polyphonic Filter");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		numEditorStates
	};

	PolyFilterEffect(MainController *mc, const String &uid, int numVoices);;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };
	Processor *getChildProcessor(int processorIndex) override { return processorIndex == FrequencyChain ? freqChain : gainChain; };
	const Processor *getChildProcessor(int processorIndex) const override { return processorIndex == FrequencyChain ? freqChain : gainChain; };
	AudioSampleBuffer & getBufferForChain(int index) { return index == FrequencyChain ? timeVariantFreqModulatorBuffer : timeVariantGainModulatorBuffer; };

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void renderNextBlock(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSample*/) { }
	/** Calculates the frequency chain and sets the q to the current value. */
	void preVoiceRendering(int voiceIndex, int startSample, int numSamples);
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;
	/** Resets the filter state if a new voice is started. */
	void startVoice(int voiceIndex, int noteNumber) override;
	bool hasTail() const override { return true; };
	
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	IIRCoefficients getCurrentCoefficients() const override {return voiceFilters[0]->getCurrentCoefficients();};

private:

	friend class HarmonicFilter;

	bool changeFlag;

	double currentFreq;
	double freq;
	double q;
	float gain;
	float currentGain;

	MonoFilterEffect::FilterMode mode;

	OwnedArray<MonoFilterEffect> voiceFilters;

	ScopedPointer<ModulatorChain> freqChain;
	ScopedPointer<ModulatorChain> gainChain;

	AudioSampleBuffer timeVariantFreqModulatorBuffer;
	AudioSampleBuffer timeVariantGainModulatorBuffer;

};






#endif  // FILTERS_H_INCLUDED
