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

    
    namespace FilterLimitValues
    {
        constexpr double lowFrequency = 20.0f;
        constexpr double highFrequency = 20000.0f;
        
        constexpr double lowQ = 0.3;
        constexpr double highQ = 9.999;
        
        constexpr double lowGain = -18.0;
        constexpr double highGain = 18.0;
    }
    
    struct FilterLimits
    {
        
        static double limit(double minValue, double maxValue, double value)
        {
#if HISE_IOS
            return jlimit<double>(minValue, maxValue, value);
#else
            _mm_store_sd(&value, _mm_min_sd(_mm_max_sd(_mm_set_sd(value), _mm_set_sd(minValue)), _mm_set_sd(maxValue)));
            return value;
#endif
        }
        
        static double limitFrequency(double freq)
        {
            return limit(FilterLimitValues::lowFrequency, FilterLimitValues::highFrequency, freq);
        }
        
        static double limitQ(double q)
        {
            return limit(FilterLimitValues::lowQ, FilterLimitValues::highQ, q);
        }
        
        static double limitGain(double gain)
        {
            return limit(FilterLimitValues::lowGain, FilterLimitValues::highGain, gain);
        }
    };
    

    
class FilterHelpers
{
public:

	enum BankType
	{
		Poly,
		Mono,
		numBankTypes
	};

	enum CoefficientType
	{
		LowPass,
		LowPassReso,
		HighPass,
		BandPass,
		Peak,
		LowShelf,
		HighShelf,
		AllPass,
		numCoefficientTypes
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
		LinkwitzRiley,
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
		double qModValue = 1.0;
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
		gain(1.0),
		currentFreq(1000.0),
		currentQ(1.0),
		currentGain(1.0)
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

		frequency.reset(newSampleRate / 64.0, smoothingTimeSeconds);
		q.reset(newSampleRate / 64.0, smoothingTimeSeconds);
		gain.reset(newSampleRate / 64.0, smoothingTimeSeconds);

		reset();
		clearCoefficients();
	}

	void setSmoothingTime(double newSmoothingTimeSeconds)
	{
		smoothingTimeSeconds = newSmoothingTimeSeconds;
		setSampleRate(sampleRate);
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
		targetFreq = FilterLimits::limitFrequency(newFrequency);
		frequency.setValue(targetFreq);
	}

	void setQ(double newQ)
	{
		targetQ = FilterLimits::limitQ(newQ);
		q.setValue(targetQ);
	}

	void setGain(double newGain)
	{
		targetGain = FilterLimits::limitGain(newGain);
		gain.setValue(targetGain);
	}

	static Identifier getFilterTypeId()
	{
		return FilterSubType::getStaticId();
	}

	IIRCoefficients getApproximateCoefficients() const
	{
		auto cType = FilterSubType::getCoefficientTypeList();

		auto m = cType[type];

		auto f_ = (double)frequency.getCurrentValue();
		auto q_ = (double)q.getCurrentValue();
		auto g_ = (float)gain.getCurrentValue();

		switch (m)
		{
		case FilterHelpers::AllPass: return IIRCoefficients::makeAllPass(sampleRate, f_, q_);
		case FilterHelpers::LowPassReso: return IIRCoefficients::makeLowPass(sampleRate, f_, q_);
		case FilterHelpers::LowPass: return IIRCoefficients::makeLowPass(sampleRate, f_);
		case FilterHelpers::BandPass: return IIRCoefficients::makeBandPass(sampleRate, f_);
		case FilterHelpers::LowShelf: return IIRCoefficients::makeLowShelf(sampleRate, f_, q_, g_);
		case FilterHelpers::HighShelf: return IIRCoefficients::makeHighShelf(sampleRate, f_, q_, g_);
		case FilterHelpers::Peak: return IIRCoefficients::makePeakFilter(sampleRate, f_, q_, g_);
		case FilterHelpers::HighPass: return IIRCoefficients::makeHighPass(sampleRate, f_, q_);
		default:	return IIRCoefficients::makeLowPass(sampleRate, f_);
		}
	}

	void reset(int unused=0)
	{
		ignoreUnused(unused);
		frequency.setValueWithoutSmoothing(targetFreq);
		gain.setValueWithoutSmoothing(targetGain);
		q.setValueWithoutSmoothing(targetQ);

		FilterSubType::reset(numChannels);
	}
	
	void processSingle(float* frameData, int channels)
	{
		jassert(channels == numChannels);

		if (--frameCounter <= 0)
		{
			frameCounter = 64;
			updateEvery64Frame();
		}

		FilterSubType::processSingle(frameData, channels);
	}

	StringArray getModes() const
	{
		return FilterSubType::getModes();
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

	void updateEvery64Frame()
	{
		auto thisFreq = FilterLimits::limitFrequency(frequency.getNextValue());
		auto thisGain = gain.getNextValue();
		auto thisQ = FilterLimits::limitQ(q.getNextValue());

		dirty |= compareAndSet(currentFreq, thisFreq);
		dirty |= compareAndSet(currentGain, thisGain);
		dirty |= compareAndSet(currentQ, thisQ);

		if (dirty)
		{
			FilterSubType::updateCoefficients(sampleRate, thisFreq, thisQ, thisGain);
			dirty = false;
		}
	}

	void update(FilterHelpers::RenderData& renderData)
	{
		auto thisFreq = FilterLimits::limitFrequency(renderData.freqModValue * frequency.getNextValue());
		auto thisGain = renderData.gainModValue * gain.getNextValue();
		auto thisQ = FilterLimits::limitQ(q.getNextValue() * renderData.qModValue);

		dirty |= compareAndSet(currentFreq, thisFreq);
		dirty |= compareAndSet(currentGain, thisGain);
		dirty |= compareAndSet(currentQ, thisQ);

		if (dirty)
		{
			FilterSubType::updateCoefficients(sampleRate, thisFreq, thisQ, thisGain);
			dirty = false;
		}
	}

	bool dirty = false;

	double smoothingTimeSeconds = 0.03;
	double sampleRate = 44100.0;
	LinearSmoothedValue<double> frequency = 10000.0;
	LinearSmoothedValue<double> q = 1.0;
	LinearSmoothedValue<double> gain = 1.0;

	double currentFreq;
	double currentGain;
	double currentQ;

	double targetFreq = 1000.0;
	double targetQ = 1.0;
	double targetGain = 1.0;

	int frameCounter = 0;
	int type = -1;
	int numChannels = 2;
};


class LinkwitzRiley
{
public:

	enum Mode
	{
		LP,
		HP
	};

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::LinkwitzRiley);

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("linkwitzriley"); };

	StringArray getModes() const { return { "LP", "HP" }; };

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const 
	{ 
		return { FilterHelpers::LowPass, FilterHelpers::HighPass };
	}

protected:

	void setType(int type)
	{
		mode = type;
	}

	void reset(int numChannels)
	{
		memset(hpData, 0, sizeof(Data)*numChannels);
		memset(lpData, 0, sizeof(Data)*numChannels);
	}

	void processSamples(AudioSampleBuffer& buffer, int startSample, int)
	{
		for (int c = 0; c < buffer.getNumChannels(); c++)
		{
			auto ptr = buffer.getWritePointer(c + startSample);

			for (int i = 0; i < buffer.getNumSamples(); i++)
			{
                ptr[i] = process(ptr[i], c);
			}
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		for (int i = 0; i < numChannels; i++)
		{
			frameData[i] = process(frameData[i], i);
		}
	}

	double b1co, b2co, b3co, b4co;

	void updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/)
	{
		const double pi = double_Pi;
		const double cowc = 2.0 * pi*frequency;
		const double cowc2 = cowc * cowc;
		const double cowc3 = cowc2 * cowc;
		const double cowc4 = cowc2 * cowc2;
		const double cok = cowc / std::tan(pi*frequency / sampleRate);
		const double cok2 = cok * cok;
		const double cok3 = cok2 * cok;
		const double cok4 = cok2 * cok2;
		const double sqrt2 = std::sqrt(2.0);
		const double sq_tmp1 = sqrt2 * cowc3 * cok;
		const double sq_tmp2 = sqrt2 * cowc * cok3;
		const double a_tmp = 4.0 * cowc2*cok2 + 2.0 * sq_tmp1 + cok4 + 2.0 * sq_tmp2 + cowc4;

		b1co = (4.0 * (cowc4 + sq_tmp1 - cok4 - sq_tmp2)) / a_tmp;
		b2co = (6.0 * cowc4 - 8.0 * cowc2*cok2 + 6.0 * cok4) / a_tmp;
		b3co = (4.0 * (cowc4 - sq_tmp1 + sq_tmp2 - cok4)) / a_tmp;
		b4co = (cok4 - 2.0 * sq_tmp1 + cowc4 - 2.0 * sq_tmp2 + 4.0 * cowc2*cok2) / a_tmp;

		//================================================
		// low-pass
		//================================================
		lpco.coefficients[0] = cowc4 / a_tmp;
		lpco.coefficients[1] = 4.0 * cowc4 / a_tmp;
		lpco.coefficients[2] = 6.0 * cowc4 / a_tmp;
		lpco.coefficients[3] = lpco.coefficients[1];
		lpco.coefficients[4] = lpco.coefficients[0];

		//=====================================================
		// high-pass
		//=====================================================
		hpco.coefficients[0] = cok4 / a_tmp;
		hpco.coefficients[1] = -4.0 * cok4 / a_tmp;
		hpco.coefficients[2] = 6.0 * cok4 / a_tmp;
		hpco.coefficients[3] = hpco.coefficients[1];
		hpco.coefficients[4] = hpco.coefficients[0];

	}

private:

	struct Data
	{
		double xm1 = 0.0f;
		double xm2 = 0.0f;
		double xm3 = 0.0f;
		double xm4 = 0.0f;
		double ym1 = 0.0f;
		double ym2 = 0.0f;
		double ym3 = 0.0f;
		double ym4 = 0.0f;
	};

	Data hpData[NUM_MAX_CHANNELS];
	Data lpData[NUM_MAX_CHANNELS];

	float process(float input, int channel)
	{
		double tempx, tempy;
		tempx = (double)input;

		double returnValue;

		auto& hptemp = hpData[channel];
		auto& lptemp = lpData[channel];
			// High pass

		tempy = hpco.coefficients[0]*tempx +
		hpco.coefficients[1]*hptemp.xm1 +
		hpco.coefficients[2]*hptemp.xm2 +
		hpco.coefficients[3]*hptemp.xm3 +
		hpco.coefficients[4]*hptemp.xm4 -
		b1co * hptemp.ym1 -
		b2co * hptemp.ym2 -
		b3co * hptemp.ym3 -
		b4co * hptemp.ym4;

		hptemp.xm4 = hptemp.xm3;
		hptemp.xm3 = hptemp.xm2;
		hptemp.xm2 = hptemp.xm1;
		hptemp.xm1 = tempx;
		hptemp.ym4 = hptemp.ym3;
		hptemp.ym3 = hptemp.ym2;
		hptemp.ym2 = hptemp.ym1;
		hptemp.ym1 = tempy;
		returnValue = tempy;

		// Low pass

		tempy = 
		lpco.coefficients[0] *tempx +
		lpco.coefficients[1] *lptemp.xm1 +
		lpco.coefficients[2] *lptemp.xm2 +
		lpco.coefficients[3] *lptemp.xm3 +
		lpco.coefficients[4] *lptemp.xm4 -
		b1co * lptemp.ym1 -
		b2co * lptemp.ym2 -
		b3co * lptemp.ym3 -
		b4co * lptemp.ym4;

		lptemp.xm4 = lptemp.xm3; // these are the same as hptemp and could be optimised away
		lptemp.xm3 = lptemp.xm2;
		lptemp.xm2 = lptemp.xm1;
		lptemp.xm1 = tempx;
		lptemp.ym4 = lptemp.ym3;
		lptemp.ym3 = lptemp.ym2;
		lptemp.ym2 = lptemp.ym1;
		lptemp.ym1 = tempy;
		
		if(mode == 0)
			returnValue = tempy;

		return (float)returnValue;
	}

	struct Coefficients
	{
		Coefficients()
		{
			memset(coefficients, 0, sizeof(double) * 5);
		}

		double coefficients[5];
	};

	Coefficients lpco;
	Coefficients hpco;
	int mode;
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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("moog"); }

	MoogFilterSubType();
	~MoogFilterSubType() {};

	StringArray getModes() const
	{
		return { "One Pole", "Two Poles", "Four Poles" };
	}

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::LowPassReso, FilterHelpers::LowPassReso, FilterHelpers::LowPassReso };
	}

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

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("one_pole"); }
	
	StringArray getModes() const { return { "LP", "HP" }; }

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::LowPass, FilterHelpers::HighPass };
	}

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

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("ring_mod"); }

	StringArray getModes() const { return { "RingMod" }; }

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::AllPass };
	}

protected:

	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

	void reset(int /*numChannels*/)
	{
		uptime = 0.0;
	}

	void setType(int /*t*/) {};

	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("biquad"); }

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

	StringArray getModes() const { return { "LowPass", "High Pass", "High Shelf", "Low Shelf", "Peak", "Reso Low" }; }

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::LowPass, FilterHelpers::HighPass,
			     FilterHelpers::HighShelf, FilterHelpers::LowShelf,
			     FilterHelpers::Peak, FilterHelpers::LowPassReso };
	}

protected:

	void setType(int newType)
	{
		biquadType = (FilterType)newType;
		reset(numChannels);
	}

	void reset(int numNewChannels);
	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("allpass"); }

	StringArray getModes() const { return { "All Pass" }; }

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::AllPass };
	}

protected:

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("ladder"); }

	SET_FILTER_TYPE(FilterHelpers::FilterSubType::LadderSubType);

	enum FilterType
	{
		LP24 = 0,
		numTypes
	};

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::LowPassReso };
	}

	StringArray getModes() const { return { "LP24" }; }

protected:

	void reset(int newNumChannels)
	{
		memset(buf, 0, sizeof(float) * newNumChannels * 4);
	}

	void setType(int /*t*/) {};

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);

	void processSingle(float* frameData, int numChannels);

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

	static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("svf"); }

	enum FilterType
	{
		LP = 0,
		HP,
		BP,
		NOTCH,
		ALLPASS,
		numTypes
	};

	StringArray getModes() const { return { "LP", "HP", "BP", "Notch", "Allpass" }; }

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const
	{
		return { FilterHelpers::LowPassReso, FilterHelpers::HighPass,
				 FilterHelpers::BandPass, FilterHelpers::BandPass,
		         FilterHelpers::AllPass };
	}

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

	void processSingle(float* frameData, int numChannels);

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
