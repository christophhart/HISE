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
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

#include <math.h>

#define USE_STATE_VARIABLE_FILTERS 1

/** A base class for filters with multiple channels. 
*
*   It exposes an interface for different filter types which have common methods for
*   setting their parameters, initialisation etc.
*/
class MultiChannelFilter
{
public:

	MultiChannelFilter()
	{
		
	}

	virtual ~MultiChannelFilter() {}

    /** Set the filter type. The type is just a plain integer so it's up to your
    *   filter implementation to decide what is happening with the value. */
	void setType(int newType)
	{
		type = newType;
		updateCoefficients();
	}

    /** Set the new frequency (real values from 20Hz to Fs/2). */
	void setFrequency(double newFrequency)
	{
		frequency = newFrequency;
		updateCoefficients();
	}

	void setFreqAndQ(double newFrequency, double newQ)
	{
		frequency = newFrequency;
		q = newQ;
		updateCoefficients();
	}

    /** Set the resonance. */
	void setQ(double newQ)
	{
		q = newQ;
		updateCoefficients();
	}

    /** Sets the samplerate. This will be automatically called whenever the sample rate changes. */
	void setSampleRate(double newSampleRate)
	{
		sampleRate = newSampleRate;
		reset();
		updateCoefficients();
	}

    /** Sets the amount of channels. */
	void setNumChannels(int newNumChannels)
	{
		numChannels = jlimit<int>(0, NUM_MAX_CHANNELS, newNumChannels);
	}

    /** Overwrite this method and reset all internal variables like previous values etc. This will be called
    *   when a new voice is started to prevent clicks in the processing. */
	virtual void reset() = 0;

    /** this will be called whenever the frequency, q, or samplerate changes. */
	virtual void updateCoefficients() = 0;

    /** Implement the filter algorithm here. */
	virtual void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) = 0;

protected:

	double sampleRate = 44100.0;
	double frequency = 10000.0;
	double q = 1.0;
	int type = 0;
	int numChannels = 2;

};


class MoogFilter: public MultiChannelFilter
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
		in1 = data;
		in2 = in1 + NUM_MAX_CHANNELS;
		in3 = in2 + NUM_MAX_CHANNELS;
		in4 = in3 + NUM_MAX_CHANNELS;
		out1 = in4 + NUM_MAX_CHANNELS;
		out2 = out1 + NUM_MAX_CHANNELS;
		out3 = out2 + NUM_MAX_CHANNELS;
		out4 = out3 + NUM_MAX_CHANNELS;


		reset();

		setFreqAndQ(20000.0, 1.0);
	}

	void reset()
	{
		memset(data, 0, sizeof(double)*NUM_MAX_CHANNELS * 8);
	}

	void setMode(int newMode)
	{
		mode = (Mode)newMode;
	};

	

	void updateCoefficients() override
	{
		fc = frequency / (0.5 *sampleRate);
		res = q / 2.0;
		if (res > 4.0) res = 4.0;
		f = fc * 1.16;
		fss = (f * f) * (f * f);
		invF = 1.0 - f;
		fb = res * (1.0 - 0.15 * f * f);
	}

	
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (numChannels != buffer.getNumChannels())
		{
			setNumChannels(buffer.getNumChannels());
		}

		for (int c = 0; c < numChannels; c++)
		{
			float* d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				double input = (double)d[i];

				input -= out4[c] * fb;
				input *= 0.35013 * fss;
				out1[c] = input + 0.3 * in1[c] + invF * out1[c]; 
				in1[c] = input;
				out2[c] = out1[c] + 0.3 * in2[c] + invF * out2[c]; 
				in2[c] = out1[c];
				out3[c] = out2[c] + 0.3 * in3[c] + invF * out3[c]; 
				in3[c] = out2[c];
				out4[c] = out3[c] + 0.3 * in4[c] + invF * out4[c]; 
				in4[c] = out3[c];
				d[i] = 2.0f * (float)out4[c];
			}
		}
	}

private:

	enum Index
	{
		In1 = 0,
		In2,
		In3,
		In4,
		Out1,
		Out2,
		Out3,
		Out4
	};

	Mode mode;

	double data[NUM_MAX_CHANNELS * 8];

	double* in1;
	double* in2;
	double* in3;
	double* in4;

	double* out1;
	double* out2;
	double* out3;
	double* out4;
	
	double f;
	double fss;
	double invF;
	double fb;
	double fc;
	double res;
	
};


class SimpleOnePole: public MultiChannelFilter
{
public:

	enum FilterType
	{
		LP=0,
		HP,
		numTypes
	};

	SimpleOnePole()
	{
		memset(lastValues, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

	void reset() override
	{
		memset(lastValues, 0, sizeof(float)*numChannels);
	}

	void updateCoefficients() override
	{
		const double x = exp(-2.0*double_Pi*frequency / sampleRate);

		a0 = (float)(1.0 - x);
		b1 = (float)-x;
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (buffer.getNumSamples() != numChannels)
		{
			setNumChannels(buffer.getNumChannels());
		}
		
		switch (type)
		{
		case FilterType::HP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float *d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					const float tmp = a0*d[i] - b1*lastValues[c];
					lastValues[c] = tmp;
					d[i] = d[i] - tmp;

				}
			}

			break;
		}
		case FilterType::LP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float *d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					d[i] = a0*d[i] - b1*lastValues[c];
					lastValues[c] = d[i];
				}
			}

			break;
		}

		}

		
	}

private:

	float lastValues[NUM_MAX_CHANNELS];

	float a0;
	float b1;
};

class StaticBiquad : public MultiChannelFilter
{
public:

	enum FilterType
	{
		LowPass,
		HighPass,
		HighShelf,
		LowShelf,
		Peak,
		ResoLow,
		numFilterTypes
	};

	void setGain(double newGain)
	{
		gain = newGain;
	}

	void reset() override
	{
		for (int i = 0; i < numChannels; i++)
		{
			filters[i].reset();
		}
	}

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
	{
		if (numChannels != b.getNumChannels())
		{
			setNumChannels(b.getNumChannels());
		}

		for (int i = 0; i < numChannels; i++)
		{
			float* d = b.getWritePointer(i, startSample);
			filters[i].processSamples(d, numSamples);
		}
	}

	void updateCoefficients() override;

	IIRCoefficients currentCoefficients;

	double gain = 1.0;

	IIRFilter filters[NUM_MAX_CHANNELS];
};


#if USE_STATE_VARIABLE_FILTERS

class StateVariableFilter: public MultiChannelFilter
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
		memset(v0z, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(v1, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(v2, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

	void reset() 
	{
		memset(v0z, 0, sizeof(float)*numChannels);
		memset(v1, 0, sizeof(float)*numChannels);
		memset(v2, 0, sizeof(float)*numChannels);
	};

	void updateCoefficients() override
	{
		const float scaledQ = (float)q * 0.1f;

		float g = (float)tan(double_Pi * frequency / sampleRate);
		//float damping = 1.0f / res;
		//k = damping;
		k = 1.0f - 0.99f * scaledQ;
		float ginv = g / (1.0f + g * (g + k));
		g1 = ginv;
		g2 = 2.0f * (g + k) * ginv;
		g3 = g * ginv;
		g4 = 2.0f * ginv;
		
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples) override
	{
		if (numChannels != buffer.getNumChannels())
		{
			setNumChannels(buffer.getNumChannels());
		}

		switch (type)
		{
		case LP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = v1[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					v1[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v2[c];
				}
			}

			break;
		}
		case BP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = v1[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					v1[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v1[c];
				}
			}

			break;
		}

		case HP:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = v1[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					v1[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v0 - k * v1[c] - v2[c];
				}
			}

			break;
		}
		case NOTCH:
		{
			for (int c = 0; c < numChannels; c++)
			{
				float* d = buffer.getWritePointer(c, startSample);

				for (int i = 0; i < numSamples; i++)
				{
					float v0 = d[i];
					float v1z = v1[c];
					float v2z = v2[c];
					float v3 = v0 + v0z[c] - 2.0f * v2z;
					v1[c] += g1 * v3 - g2 * v1z;
					v2[c] += g3 * v3 + g4 * v1z;
					v0z[c] = v0;
					d[i] = v0 - k * v1[c];
				}
			}

			break;
		}
		}
	}

private:
	
	float v0z[NUM_MAX_CHANNELS];
	float v1[NUM_MAX_CHANNELS];
	float v2[NUM_MAX_CHANNELS];

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

	virtual IIRCoefficients getCurrentCoefficients() const = 0;

protected:

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
        Keytrack,
		numEffectParameters
	};

	enum FilterMode
	{
		LowPass = 0,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		ResoLow,
		StateVariableLP,
		StateVariableHP,
		MoogLP,
		OnePoleLowPass,
		OnePoleHighPass,
		numFilterModes
    };
    
    double fractionalMidiNoteInHz(double note)
    {
        // Like MidiMessage::getMidiNoteInHertz(), but with a float note.
        note -= 69;
        // Now 0 = A
        return 440 * pow(2.0, note / 12.0);
    }
    float KTRatio = 0.0;
    int FilterNote[128];

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

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
	static IIRCoefficients makeResoLowPass(double sampleRate, double cutoff, double q);;

	IIRCoefficients getCurrentCoefficients() const override
	{
		switch (mode)
		{
		case MonoFilterEffect::OnePoleLowPass:  return IIRCoefficients::makeLowPass(getSampleRate(), freq);
		case MonoFilterEffect::OnePoleHighPass:  return IIRCoefficients::makeHighPass(getSampleRate(), freq);
		case MonoFilterEffect::LowPass:			return IIRCoefficients::makeLowPass(getSampleRate(), freq);
		case MonoFilterEffect::HighPass:		return IIRCoefficients::makeHighPass(getSampleRate(), freq);
		case MonoFilterEffect::LowShelf:		return IIRCoefficients::makeLowShelf(getSampleRate(), freq, q, currentGain);
		case MonoFilterEffect::HighShelf:		return IIRCoefficients::makeHighShelf(getSampleRate(), freq, q, currentGain);
		case MonoFilterEffect::Peak:			return IIRCoefficients::makePeakFilter(getSampleRate(), freq, q, currentGain);
		case MonoFilterEffect::ResoLow:			return makeResoLowPass(getSampleRate(), freq, q);
		case MonoFilterEffect::StateVariableLP: return makeResoLowPass(getSampleRate(), freq, q);
		case MonoFilterEffect::StateVariableHP: return IIRCoefficients::makeHighPass(getSampleRate(), freq);
		case MonoFilterEffect::MoogLP:			return makeResoLowPass(getSampleRate(), freq, q);
		default:								return IIRCoefficients();
		}
	}

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
	friend class HarmonicMonophonicFilter;

	bool changeFlag;

	bool calculateGainModValue = false;

	double currentFreq;
	float currentGain;

	double freq;
	double q;
	float gain;
	FilterMode mode;

	StaticBiquad staticBiquadFilter;
	MoogFilter moogFilter;
	StateVariableFilter stateFilter;
	SimpleOnePole simpleFilter;
	
	MultiChannelFilter* currentFilter = nullptr;

	double lastSampleRate = 0.0;

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
    
    enum Parameters
    {
        Keytrack = 5
    };
    
    double fractionalMidiNoteInHz(double note)
    {
        // Like MidiMessage::getMidiNoteInHertz(), but with a float note.
        note -= 69;
        // Now 0 = A
        return 440 * pow(2.0, note / 12.0);
    }
    float KTRatio = 0;

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
	
    ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
    
    int FilterNote[128];

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
