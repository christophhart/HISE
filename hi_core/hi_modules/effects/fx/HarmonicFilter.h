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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HARMONICFILTER_H_INCLUDED
#define HARMONICFILTER_H_INCLUDED

namespace hise { using namespace juce;

#define NUM_MAX_FILTER_BANDS 16

class PeakFilterBand
{
	using FloatType = float;

#if USE_SSE
	using SSEType = dsp::SIMDRegister<FloatType>;
#else
	using SSEType = FloatType;
#endif

	struct State
	{
		State()
		{
			setGain(0.0);
		}

		SSEType _a1, _a2, _a3;
		SSEType _m1;

		FloatType _A;
		FloatType _ASqrt;

		SSEType _ic1eq;
		SSEType _ic2eq;
		

		FloatType g;
		FloatType k;
		FloatType q;

		FloatType gain;

		bool dirty = false;

		bool compareAndSet(FloatType& old, FloatType newValue)
		{
			bool equal = old != newValue;
			old = newValue;
			return equal;
		}

		void setGain(FloatType gainDb)
		{
			dirty = compareAndSet(gain, gainDb);

			if (dirty)
			{
				_A = pow((FloatType)10.0, gainDb / (FloatType)40.0);
				_ASqrt = sqrt(_A);

				updateGainInternal();
			}
		}

#if USE_SSE

		void process(SSEType& input)
		{
			const auto _v3 = input - _ic2eq;
			const auto _v1 = _v3 * _a2 + _ic1eq*_a1;
			const auto _v2 = _v3*_a3 + _ic1eq*_a2 + _ic2eq;

			_ic1eq = _v1 * 2.0 - _ic1eq;
			_ic2eq = _v2 * 2.0 - _ic2eq;

			input += _v1 * _m1;
		}
#else
		void process(FloatType& left, FloatType& right)
		{
			const FloatType _v3 = left - _ic2eq;
			const FloatType _v1 = _a1 * _ic1eq + _a2 * _v3;
			const FloatType _v2 = _ic2eq + _a2 * _ic1eq + _a3 * _v3;

			_ic1eq = 2 * _v1 - _ic1eq;
			_ic2eq = 2 * _v2 - _ic2eq;

			left += _m1 * _v1;
			right = left;
		}

#endif


		void reset()
		{
			_ic1eq = _ic2eq = 0.0;
		}

		void calculateFrequency(FloatType frequency, FloatType q_, FloatType sampleRate)
		{
			g = tanf((frequency / sampleRate) * static_cast<FloatType>(double_Pi));
			q = q_;

			updateGainInternal();
		}

	private:

		void updateGainInternal()
		{
			k = 1.f / (q*_A);

			_a1 = 1 / (1 + g * (g + k));
			_a2 = _a1 * g;
			_a3 = _a2 * g;
			_m1 = k * (_A*_A - 1);
		}
	};

public:

	PeakFilterBand():
		numBands(NUM_MAX_FILTER_BANDS),
		numBandsToUse(NUM_MAX_FILTER_BANDS)
	{
		q = 1.0;
		reset();
	}

	void setNumBands(int newNumBands)
	{
		numBands = jlimit<int>(1, NUM_MAX_FILTER_BANDS, newNumBands);
		numBandsToUse = numBands;
		reset();
	}

	int numBands;
	int numBandsToUse;
	double q;

	void updateBaseFrequency(double newBaseFrequency)
	{
		baseFrequency = newBaseFrequency;
		auto limit = (sampleRate * 0.4);

		auto bandsUntilLimit = jlimit<int>(1, NUM_MAX_FILTER_BANDS, roundToInt(limit / baseFrequency));

		numBandsToUse = jmin<int>(numBands, bandsUntilLimit);

		double freqToUse = baseFrequency;

		for (int i = 0; i < numBandsToUse; i++)
		{
			states[i].calculateFrequency((float)freqToUse, (float)q, (float)sampleRate);
			freqToUse += baseFrequency;
		}
	}

	void setQ(double newQ)
	{
		q = newQ;
	}

	void reset()
	{
		for (auto& s : *this)
			s.reset();
	}

	

	void updateBand(int bandIndex, float newGain)
	{
		if (bandIndex < numBandsToUse)
		{
			states[bandIndex].setGain((FloatType)newGain);
		}
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
	{
		auto l = buffer.getWritePointer(0, startSample);
		auto r = buffer.getWritePointer(1, startSample);

		for (int i = 0; i < numSamples; i++)
		{
#if USE_SSE
			SSEType d = { (FloatType)*l, (FloatType)*r };

			for (auto& s : *this)
				s.process(d);

			*l++ = (float)d[0];
			*r++ = (float)d[1];
#else

			for (auto& s : *this)
			{
				s.process(*l, *r);
			}

			l++;
			r++;
#endif
		}
	}

	inline State* begin() const noexcept
	{
		State* d = const_cast<State*>(states);
		return d;
	}

	inline State* end() const noexcept
	{
		State* d = const_cast<State*>(states);
		return d + numBandsToUse;
	}

	void setSampleRate(double sr)
	{
		sampleRate = sr;
	}

private:

	double sampleRate;

	double baseFrequency;

	State states[NUM_MAX_FILTER_BANDS];

	JUCE_DECLARE_NON_COPYABLE(PeakFilterBand);
};

class BaseHarmonicFilter: public SliderPackProcessor
{
public:

	enum SliderPacks
	{
		A = 0,
		B,
		Mix,
		numPacks
	};

	BaseHarmonicFilter(MainController* mc) :
		SliderPackProcessor(mc, 3)
	{
		dataA = getSliderPackUnchecked(0);
		dataB = getSliderPackUnchecked(1);
		dataMix = getSliderPackUnchecked(2);
	}

    virtual ~BaseHarmonicFilter() {};
    
	virtual void setCrossfadeValue(double normalizedCrossfadeValue) = 0;

protected:

	SliderPackData* dataA;
	SliderPackData* dataB;
	SliderPackData* dataMix;
};

 
/** A set of tuned hi-resonant peak filters that are set to the root frequency and harmonics of each note
	@ingroup effectTypes
*/
class HarmonicFilter : public VoiceEffectProcessor,
					   public BaseHarmonicFilter
{
public:

	SET_PROCESSOR_NAME("HarmonicFilter", "Harmonic Filter", "A set of tuned hi-resonant peak filters that are set to the root frequency and harmonics of each note");

	enum InternalChains
	{
		XFadeChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		MixChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum SpecialParameters
	{
		NumFilterBands = 0,
		QFactor,
		Crossfade,
		SemiToneTranspose,
		numParameters
	};

	enum SliderPacks
	{
		A = 0,
		B,
		Mix,
		numPacks
	};

	enum FilterBandNumbers
	{
		OneBand = 0,
		TwoBands,
		FourBands,
		EightBands,
		SixteenBands,
		numFilterBandNumbers
	};

	HarmonicFilter(MainController *mc, const String &uid, int numVoices_);;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;;

	int getNumInternalChains() const override { return numInternalChains; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return modChains[XFadeChain].getChain(); };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return modChains[XFadeChain].getChain(); };
	int getNumChildProcessors() const override { return 1; };
	
	void setQ(float newQ);
	void setNumFilterBands(int numBands);
	void setSemitoneTranspose(float newValue);

	bool hasTail() const override { return true; };
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void renderNextBlock(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSample*/) {}
	/** Calculates the frequency chain and sets the q to the current value. */
	/** Resets the filter state if a new voice is started. */
	void startVoice(int voiceIndex, const HiseEvent& e) override;
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
    
	void setCrossfadeValue(double normalizedCrossfadeValue) override;

private:
	
	int getNumBandForFilterBandIndex(FilterBandNumbers number) const;

	int filterBandIndex;
	float currentCrossfadeValue;
	int semiToneTranspose;
	const int numVoices;
	double q;
	int numBands;

	FixedVoiceAmountArray<PeakFilterBand> filterBanks;
};




class HarmonicMonophonicFilter : public MonophonicEffectProcessor,
								 public BaseHarmonicFilter
{
public:

	SET_PROCESSOR_NAME("HarmonicFilterMono", "Harmonic Filter Monophonic", "A set of tuned hi-resonant peak filters that are set to the root frequency and harmonics of the last played note");

	enum InternalChains
	{
		XFadeChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		MixChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum SpecialParameters
	{
		NumFilterBands = 0,
		QFactor,
		Crossfade,
		SemiToneTranspose,
		numParameters
	};

	

	enum FilterBandNumbers
	{
		OneBand = 0,
		TwoBands,
		FourBands,
		EightBands,
		SixteenBands,
		numFilterBandNumbers
	};

	HarmonicMonophonicFilter(MainController *mc, const String &uid);

	void setInternalAttribute(int parameterIndex, float newValue) override;
	float getAttribute(int parameterIndex) const override;;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;;

	int getNumInternalChains() const override { return numInternalChains; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return modChains[XFadeChain].getChain(); };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return modChains[XFadeChain].getChain(); };
	int getNumChildProcessors() const override { return 1; };
	
	void setQ(float newQ);
	void setNumFilterBands(int numBands);
	void setSemitoneTranspose(float newValue);

	bool hasTail() const override { return true; };
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	
	/** Resets the filter state if a new voice is started. */
	void startMonophonicVoice(const HiseEvent& e) override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
    
	void setCrossfadeValue(double normalizedCrossfadeValue);

private:

	int getNumBandForFilterBandIndex(FilterBandNumbers number) const;

	int filterBandIndex;
	float currentCrossfadeValue;
	int semiToneTranspose;
	int numBands;
	double q;

	PeakFilterBand filterBank;
};




} // namespace hise

#endif  // HARMONICFILTER_H_INCLUDED
