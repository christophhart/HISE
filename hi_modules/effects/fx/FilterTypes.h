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

#ifndef FILTER_TYPES_H_INCLUDED
#define FILTER_TYPES_H_INCLUDED

namespace hise {
using namespace juce;

#ifndef MIN_FILTER_FREQ
#define MIN_FILTER_FREQ 20.0
#endif

#define SET_FILTER_TYPE(x) static FilterHelpers::FilterSubType getFilterType() { return x; };

class FilterHelpers
{
public:

	enum BankType
	{
		Poly,
		Mono,
		numBankTypes
	};

	enum class FilterSubType
	{
		MoogFilterSubType,
		LadderSubType,
		StateVariableFilterSubType,
		StaticBiquadSubType,
		SimpleOnePoleSubType,
		PhaseAllpassSubType,
		RingmodFilterSubType,
		numFilterSubTypes
	};

	struct RenderData
	{
		RenderData(AudioSampleBuffer& b_, int start_, int size_) :
			b(b_),
			startSample(start_),
			numSamples(size_)
		{
		}

		AudioSampleBuffer& b;
		int voiceIndex = -1;

		int startSample = 0;
		int numSamples = -1;
		double freqModValue = 1.0;
		double gainModValue = 1.0;
	};
};

/** A base class for filters with multiple channels.
*
*   It exposes an interface for different filter types which have common methods for
*   setting their parameters, initialisation etc.
*/
template <class FilterSubType> class MultiChannelFilter : public FilterSubType
{
public:

	MultiChannelFilter():
		frequency(1000.0),
		q(1.0),
		gain(1.0)
	{

	}

	~MultiChannelFilter() {}

	/** Set the filter type. The type is just a plain integer so it's up to your
	*   filter implementation to decide what is happening with the value. */
	void setType(int newType)
	{
		if (type != newType)
		{
			type = newType;
			FilterSubType::setType(type);
			clearCoefficients();
		}
	}

	/** Sets the samplerate. This will be automatically called whenever the sample rate changes. */
	void setSampleRate(double newSampleRate)
	{
		sampleRate = newSampleRate;

		frequency.reset(newSampleRate / 64.0, 0.03);
		q.reset(newSampleRate / 64.0, 0.03);
		gain.reset(newSampleRate / 64.0, 0.03);
		

		reset();
		clearCoefficients();
	}

	/** Sets the amount of channels. */
	void setNumChannels(int newNumChannels)
	{
		numChannels = jlimit<int>(0, NUM_MAX_CHANNELS, newNumChannels);
		reset();
		clearCoefficients();
	}

	void setFrequency(double newFrequency)
	{
		frequency.setValue(FilterLimits::limitFrequency(newFrequency));
	}

	void setQ(double newQ)
	{
		q.setValue(FilterLimits::limitQ(newQ));
	}

	void setGain(double newGain)
	{
		gain.setValue(FilterLimits::limitGain(newGain));
	}

	void reset()
	{
		frequency.setValueWithoutSmoothing(frequency.getTargetValue());
		gain.setValueWithoutSmoothing(gain.getTargetValue());
		q.setValueWithoutSmoothing(q.getTargetValue());

		FilterSubType::reset(numChannels);
	}
	
	void render(FilterHelpers::RenderData& r)
	{
		update(r);

		if (numChannels != r.b.getNumChannels())
		{
			setNumChannels(r.b.getNumChannels());
		}

		FilterSubType::processSamples(r.b, r.startSample, r.numSamples);
	}

private:

	inline double limit(double value, double minValue, double maxValue)
	{
		// Branchless SSE clamp.
		// return minss( maxss(val,minval), maxval );

#if HISE_IOS
		return jlimit<double>(minValue, maxValue, value);
#else
		_mm_store_sd(&value, _mm_min_sd(_mm_max_sd(_mm_set_sd(value), _mm_set_sd(minValue)), _mm_set_sd(maxValue)));
		return value;
#endif
	}

	inline bool compareAndSet(double& value, double newValue) noexcept
	{
		bool unEqual = value != newValue;
		value = newValue;
		return unEqual;
	}

	void clearCoefficients()
	{
		dirty = true;
	}

	void update(FilterHelpers::RenderData& renderData)
	{
		auto thisFreq = FilterLimits::limitFrequency(renderData.freqModValue * frequency.getNextValue());
		auto thisGain = renderData.gainModValue * gain.getNextValue();
		auto thisQ = q.getNextValue();

		dirty |= compareAndSet(currentFreq, thisFreq);
		dirty |= compareAndSet(currentGain, thisGain);
		dirty |= q.isSmoothing();

		if (dirty)
		{
			FilterSubType::updateCoefficients(sampleRate, thisFreq, thisQ, thisGain);
			dirty = false;
		}
	}

	bool dirty = false;

	double sampleRate = 44100.0;
	LinearSmoothedValue<double> frequency = 10000.0;
	LinearSmoothedValue<double> q = 1.0;
	LinearSmoothedValue<double> gain = 1.0;

	double currentFreq;
	double currentGain;

	int type = -1;
	int numChannels = 2;
};


class MoogFilterSubType
{
public:

	enum Mode
	{
		OnePole = 0,
		TwoPole,
		FourPole,
		numModes
	};

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::MoogFilterSubType);

	MoogFilterSubType();
	~MoogFilterSubType() {};

protected:

	void reset(int numChannels)
	{
		memset(data, 0, sizeof(double)*numChannels * 8);
	}

	void setMode(int newMode)
	{
		mode = (Mode)newMode;
	};

	void setType(int /*t*/) {};

	void updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/);


	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);

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




class SimpleOnePoleSubType
{
public:

	enum FilterType
	{
		LP = 0,
		HP,
		numTypes
	};

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::SimpleOnePoleSubType);

	SimpleOnePoleSubType()
	{
		memset(lastValues, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

protected:


	void setType(int t)
	{
		onePoleType = (FilterType)t;
	};

	void reset(int numChannels)
	{
		memset(lastValues, 0, sizeof(float)*numChannels);
	}

	void updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/)
	{
		const double x = exp(-2.0*double_Pi*frequency / sampleRate);

		a0 = (float)(1.0 - x);
		b1 = (float)-x;
	}
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);

private:

	FilterType onePoleType;

	size_t lastChannelAmount = NUM_MAX_CHANNELS;

	float lastValues[NUM_MAX_CHANNELS];

	float a0;
	float b1;
};

using SimpleOnePole = MultiChannelFilter<SimpleOnePoleSubType>;

class RingmodFilterSubType
{
public:

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::RingmodFilterSubType);

protected:

	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

	void reset(int /*numChannels*/)
	{
		uptime = 0.0;
	}

	void setType(int /*t*/) {};

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);

private:

	double uptimeDelta = 0.0;
	double uptime = 0.0;
	float oscGain = 1.0f;
};

using RingmodFilter = MultiChannelFilter<RingmodFilterSubType>;


class StaticBiquadSubType
{
public:

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::StaticBiquadSubType);

	enum FilterType
	{
		LowPass = 0,
		HighPass,
		HighShelf,
		LowShelf,
		Peak,
		ResoLow,
		numFilterTypes
	};

protected:

	void setType(int newType)
	{
		biquadType = (FilterType)newType;
		reset(numChannels);
	}

	void reset(int numNewChannels);
	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

private:

	int numChannels = NUM_MAX_CHANNELS;

	IIRCoefficients currentCoefficients;
	IIRFilter filters[NUM_MAX_CHANNELS];
	FilterType biquadType;
};

using StaticBiquad = MultiChannelFilter<StaticBiquadSubType>;


class PhaseAllpassSubType
{
public:

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::PhaseAllpassSubType);

protected:

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);

	void setType(int /**/) {};

	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

	void reset(int newNumChannels)
	{
		numFilters = newNumChannels;

	}

private:

	struct InternalFilter
	{

		class AllpassDelay
		{
		public:
			AllpassDelay() :
				delay(0.f),
				currentValue(0.f)
			{}

			static float getDelayCoefficient(float delaySamples)
			{
				return (1.f - delaySamples) / (1.f + delaySamples);
			}

			void setDelay(float newDelay) noexcept { delay = newDelay; };

			float getNextSample(float input) noexcept
			{
				float y = input * -delay + currentValue;
				currentValue = y * delay + input;

				return y;
			};

			float delay, currentValue;
		};

		InternalFilter()
		{
			reset();
		}

		void reset()
		{
			currentValue = 0.0f;
		}

		float getNextSample(float input);

		AllpassDelay allpassFilters[6];

		float minDelay, maxDelay;
		float feedback;
		float sampleRate;
		float fMin;
		float fMax;
		float rate;

		float currentValue;

	};

	int numFilters = NUM_MAX_CHANNELS;

	InternalFilter filters[NUM_MAX_CHANNELS];

};

using PhaseAllpass = MultiChannelFilter<PhaseAllpassSubType>;

class LadderSubType
{
public:

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::LadderSubType);

	enum FilterType
	{
		LP24 = 0,
		numTypes
	};

protected:

	void reset(int newNumChannels)
	{
		memset(buf, 0, sizeof(float) * newNumChannels * 4);
	}

	void setType(int /*t*/) {};

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);

	void updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/);

private:

	float processSample(float input, int channel);

	float buf[NUM_MAX_CHANNELS][4];

	float cut;
	float res;
};

using Ladder = MultiChannelFilter<LadderSubType>;

class StateVariableFilterSubType
{


public:

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::StateVariableFilterSubType);

	enum FilterType
	{
		LP = 0,
		HP,
		BP,
		NOTCH,
		ALLPASS,
		numTypes
	};

	StateVariableFilterSubType()
	{
		memset(v0z, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(z1_A, 0, sizeof(float)*NUM_MAX_CHANNELS);
		memset(v2, 0, sizeof(float)*NUM_MAX_CHANNELS);
	}

protected:

	void reset(int numChannels)
	{
		memset(v0z, 0, sizeof(float)*numChannels);
		memset(z1_A, 0, sizeof(float)*numChannels);
		memset(v2, 0, sizeof(float)*numChannels);
	};

	void setType(int t)
	{
		type = (FilterType)t;
	}

	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);

private:

	FilterType type;

	float v0z[NUM_MAX_CHANNELS];
	float z1_A[NUM_MAX_CHANNELS];
	float v2[NUM_MAX_CHANNELS];

	float k, g1, g2, g3, g4, x1, x2, gCoeff, RCoeff;

};

using StateVariableFilter = MultiChannelFilter<StateVariableFilterSubType>;

}

#endif // FILTER_TYPES_H_INCLUDED