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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise { using namespace juce;



#ifndef MIN_FILTER_FREQ
#define MIN_FILTER_FREQ 20.0
#endif

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
	static double limit(double minValue, double maxValue, double value);
	static double limitFrequency(double freq);
	static double limitQ(double q);
	static double limitGain(double gain);
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
		StateVariableEqSubType,
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

		double applyModValue(double f) const;

		AudioSampleBuffer& b;
		int voiceIndex = -1;

		int startSample = 0;
		int numSamples = -1;
		double freqModValue = 1.0;
		double bipolarDelta = 0.0;
		double gainModValue = 1.0;
		double qModValue = 1.0;
	};
};

/** A base class for filters with multiple channels.
*
*   It exposes an interface for different filter types which have common methods for
*   setting their parameters, initialisation etc.
*/
template <class FilterSubType> class MultiChannelFilter
{
public:

	using SubType = FilterSubType;

	MultiChannelFilter();
	~MultiChannelFilter() {}

	/** Set the filter type. The type is just a plain integer so it's up to your
	*   filter implementation to decide what is happening with the value. */
	void setType(int newType);

	void setSampleRate(double newSampleRate);
	void setSmoothingTime(double newSmoothingTimeSeconds);
	void setNumChannels(int newNumChannels);
	void setFrequency(double newFrequency);
	void setQ(double newQ);
	void setGain(double newGain);

	double getGain() const;
	double getFrequency() const;
	double getQ() const;
	int getType() const;

	static Identifier getFilterTypeId();
	IIRCoefficients getApproximateCoefficients() const;

	void reset(int unused = 0);
	void processFrame(float* frameData, int channels);
	StringArray getModes() const;
	void render(FilterHelpers::RenderData& r);

private:

	FilterSubType internalFilter;

	double limit(double value, double minValue, double maxValue);
	bool compareAndSet(double& value, double newValue) noexcept;
	void clearCoefficients();
	void updateEvery64Frame();
	void update(FilterHelpers::RenderData& renderData);

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


#define FORWARD_DECLARE_MULTI_CHANNEL_FILTER(className) extern template class MultiChannelFilter<className>;
#define DEFINE_MULTI_CHANNEL_FILTER(className) template class MultiChannelFilter<className>;

class LinkwitzRiley
{
public:

	enum Mode
	{
		LP,
		HP,
		Allpass
	};

	static FilterHelpers::FilterSubType getFilterType();;
	static Identifier getStaticId();;

	StringArray getModes() const;;

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	void setType(int type);
	void reset(int numChannels);
	void processSamples(AudioSampleBuffer& buffer, int startSample, int);
	void processFrame(float* frameData, int numChannels);
	double b1co, b2co, b3co, b4co;

	void updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/);

private:


	SpinLock lock;

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

	float process(float input, int channel);

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

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(LinkwitzRiley);

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

	static FilterHelpers::FilterSubType getFilterType();;
	static Identifier getStaticId();

	MoogFilterSubType();
	~MoogFilterSubType();;

	StringArray getModes() const;

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	void reset(int numChannels);
	void setMode(int newMode);;
	void setType(int /*t*/);;
	void updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/);
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);

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

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(MoogFilterSubType);

class SimpleOnePoleSubType
{
public:

	enum FilterType
	{
		LP = 0,
		HP,
		numTypes
	};

	static FilterHelpers::FilterSubType getFilterType();
	static Identifier getStaticId();

	StringArray getModes() const;

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;


	SimpleOnePoleSubType();

	void setType(int t);;
	void reset(int numChannels);
	void updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/);
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);

private:

	FilterType onePoleType;
	size_t lastChannelAmount = NUM_MAX_CHANNELS;
	float lastValues[NUM_MAX_CHANNELS];
	float a0;
	float b1;
};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(SimpleOnePoleSubType);
using SimpleOnePole = MultiChannelFilter<SimpleOnePoleSubType>;

class RingmodFilterSubType
{
public:

	static FilterHelpers::FilterSubType getFilterType();
	static Identifier getStaticId();

	StringArray getModes() const;
	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	void updateCoefficients(double sampleRate, double frequency, double q, double gain);
	void reset(int /*numChannels*/);
	void setType(int /*t*/);;
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);

private:

	double uptimeDelta = 0.0;
	double uptime = 0.0;
	float oscGain = 1.0f;
};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(RingmodFilterSubType);
using RingmodFilter = MultiChannelFilter<RingmodFilterSubType>;


class StaticBiquadSubType
{
public:

	static FilterHelpers::FilterSubType getFilterType();
	static Identifier getStaticId();

	enum FilterType
	{
		LowPass = 0,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		ResoLow,
		numFilterTypes
	};

	StringArray getModes() const;

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	void setType(int newType);
	void reset(int numNewChannels);
	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);
	void processFrame(float* frameData, int numChannels);

private:

	int numChannels = NUM_MAX_CHANNELS;

	IIRCoefficients currentCoefficients;
	IIRFilter filters[NUM_MAX_CHANNELS];
	FilterType biquadType;
};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(StaticBiquadSubType);
using StaticBiquad = MultiChannelFilter<StaticBiquadSubType>;


class PhaseAllpassSubType
{
public:

	static FilterHelpers::FilterSubType getFilterType();
	static Identifier getStaticId();
	StringArray getModes() const;
	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);
	void setType(int /**/);;
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);
	void reset(int newNumChannels);

private:

	struct InternalFilter
	{

		class AllpassDelay
		{
		public:

			AllpassDelay();

			static float getDelayCoefficient(float delaySamples);
			void setDelay(float newDelay) noexcept;;
			float getNextSample(float input) noexcept;;
			float delay, currentValue;
		};

		InternalFilter();

		void reset();

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

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(PhaseAllpassSubType);
using PhaseAllpass = MultiChannelFilter<PhaseAllpassSubType>;

class LadderSubType
{
public:

	static Identifier getStaticId();
	static FilterHelpers::FilterSubType getFilterType();

	enum FilterType
	{
		LP24 = 0,
		numTypes
	};

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	StringArray getModes() const;

	void reset(int newNumChannels);
	void setType(int /*t*/);;
	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);
	void updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/);

private:

	float processSample(float input, int channel);
	float buf[NUM_MAX_CHANNELS][4];

	float cut;
	float res;
};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(LadderSubType);
using Ladder = MultiChannelFilter<LadderSubType>;




/** A SVF Filter replacement for the eq filters. */
class StateVariableEqSubType
{
public:

	enum Mode
	{
		LowPass,
		HighPass,
		LowShelf,
		HighShelf,
		Peak,
		numModes
	};

	static Identifier getStaticId();
	static FilterHelpers::FilterSubType getFilterType();
	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;;
	StringArray getModes() const;

	void reset(int newNumChannels);
	void setType(int newType);
	void processSamples(AudioSampleBuffer& b, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);

private:

	Mode t = Mode::LowPass;

	struct Coefficients
	{
		Coefficients();

		void update(double freq, double q, Mode type, double samplerate);
		void setGain(double gainDb);
		double computeK(double q, bool useGain = false);
		void computeA(double g, double k);
		void tick();

	private:

		double gain;
		double gain_sqrt;
		double m[4];
		double a[4];

	public:

		double mp[4];
		double ap[4];

	} coefficients;

	struct State
	{
		State();
		void reset();
		float tick(float inp, const Coefficients& _coef);

	private:

		double _ic1eq;
		double _ic2eq;

		double v[4];
	};

	State states[NUM_MAX_CHANNELS];
};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(StateVariableEqSubType);


class StateVariableFilterSubType
{
public:

	static FilterHelpers::FilterSubType getFilterType();
	static Identifier getStaticId();

	enum FilterType
	{
		LP = 0,
		HP,
		BP,
		NOTCH,
		ALLPASS,
		numTypes
	};

	StringArray getModes() const;

	Array<FilterHelpers::CoefficientType> getCoefficientTypeList() const;

	StateVariableFilterSubType();

	void reset(int numChannels);;
	void setType(int t);
	void updateCoefficients(double sampleRate, double frequency, double q, double gain);
	void processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples);
	void processFrame(float* frameData, int numChannels);

private:

	FilterType type;

	float v0z[NUM_MAX_CHANNELS];
	float z1_A[NUM_MAX_CHANNELS];
	float v2[NUM_MAX_CHANNELS];

	float k, g1, g2, g3, g4, x1, x2, gCoeff, RCoeff;

};

FORWARD_DECLARE_MULTI_CHANNEL_FILTER(StateVariableFilterSubType);
using StateVariableFilter = MultiChannelFilter<StateVariableFilterSubType>;


} 