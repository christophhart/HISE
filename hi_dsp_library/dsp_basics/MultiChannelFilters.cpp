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

namespace hise
{
using namespace juce;


double FilterLimits::limit(double minValue, double maxValue, double value)
{
#if HISE_IOS
	return jlimit<double>(minValue, maxValue, value);
#else
	_mm_store_sd(&value, _mm_min_sd(_mm_max_sd(_mm_set_sd(value), _mm_set_sd(minValue)), _mm_set_sd(maxValue)));
	return value;
#endif
}

double FilterLimits::limitFrequency(double freq)
{
	return limit(FilterLimitValues::lowFrequency, FilterLimitValues::highFrequency, freq);
}

double FilterLimits::limitQ(double q)
{
	return limit(FilterLimitValues::lowQ, FilterLimitValues::highQ, q);
}

double FilterLimits::limitGain(double gain)
{
	return limit(FilterLimitValues::lowGain, FilterLimitValues::highGain, gain);
}


template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setType(int newType)
{
	if (type != newType)
	{
		type = newType;
		internalFilter.setType(type);
		clearCoefficients();
	}
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setSampleRate(double newSampleRate)
{
	sampleRate = newSampleRate;

	frequency.reset(newSampleRate / 64.0, smoothingTimeSeconds);
	q.reset(newSampleRate / 64.0, smoothingTimeSeconds);
	gain.reset(newSampleRate / 64.0, smoothingTimeSeconds);

	reset();
	clearCoefficients();
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setSmoothingTime(double newSmoothingTimeSeconds)
{
	smoothingTimeSeconds = newSmoothingTimeSeconds;

	if(sampleRate > 0.0)
		setSampleRate(sampleRate);
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setNumChannels(int newNumChannels)
{
	numChannels = jlimit<int>(0, NUM_MAX_CHANNELS, newNumChannels);
	reset();
	clearCoefficients();
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setFrequency(double newFrequency)
{
	targetFreq = FilterLimits::limitFrequency(newFrequency);
	frequency.setValue(targetFreq);
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setQ(double newQ)
{
	targetQ = FilterLimits::limitQ(newQ);
	q.setValue(targetQ);
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::setGain(double newGain)
{
	targetGain = FilterLimits::limitGain(newGain);
	gain.setValue(targetGain);
}

template <class FilterSubType>
double MultiChannelFilter<FilterSubType>::getQ() const
{
	return targetQ;
}

template <class FilterSubType>
double MultiChannelFilter<FilterSubType>::getFrequency() const
{
	return targetFreq;
}

template <class FilterSubType>
double MultiChannelFilter<FilterSubType>::getGain() const
{
	return targetGain;
}

template <class FilterSubType>
int MultiChannelFilter<FilterSubType>::getType() const
{
	return type;
}

template <class FilterSubType>
Identifier MultiChannelFilter<FilterSubType>::getFilterTypeId()
{
	return FilterSubType::getStaticId();
}

template <class FilterSubType>
IIRCoefficients MultiChannelFilter<FilterSubType>::getApproximateCoefficients() const
{
	auto cType = internalFilter.getCoefficientTypeList();

	auto m = cType[type];

	auto f_ = (double)targetFreq; //(double)frequency.getCurrentValue();
	auto q_ = (double)targetQ; //(double)q.getCurrentValue();
	auto g_ = (float)targetGain; //(float)gain.getCurrentValue();

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

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::reset(int unused/*=0*/)
{
	ignoreUnused(unused);
	frequency.setValueWithoutSmoothing(targetFreq);
	gain.setValueWithoutSmoothing(targetGain);
	q.setValueWithoutSmoothing(targetQ);

	internalFilter.reset(numChannels);
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::processFrame(float* frameData, int channels)
{
	jassert(channels == numChannels);

	if (--frameCounter <= 0)
	{
		frameCounter = 64;
		updateEvery64Frame();
	}

	internalFilter.processFrame(frameData, channels);
}

template <class FilterSubType>
StringArray MultiChannelFilter<FilterSubType>::getModes() const
{
	return internalFilter.getModes();
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::render(FilterHelpers::RenderData& r)
{
	update(r);

	if (numChannels != r.b.getNumChannels())
	{
		setNumChannels(r.b.getNumChannels());
	}

	internalFilter.processSamples(r.b, r.startSample, r.numSamples);
}

template <class FilterSubType>
double MultiChannelFilter<FilterSubType>::limit(double value, double minValue, double maxValue)
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

template <class FilterSubType>
bool MultiChannelFilter<FilterSubType>::compareAndSet(double& value, double newValue) noexcept
{
	bool unEqual = value != newValue;
	value = newValue;
	return unEqual;
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::clearCoefficients()
{
	dirty = true;
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::updateEvery64Frame()
{
	auto thisFreq = FilterLimits::limitFrequency(frequency.getNextValue());
	auto thisGain = gain.getNextValue();
	auto thisQ = FilterLimits::limitQ(q.getNextValue());

	dirty |= compareAndSet(currentFreq, thisFreq);
	dirty |= compareAndSet(currentGain, thisGain);
	dirty |= compareAndSet(currentQ, thisQ);

	if (dirty)
	{
		internalFilter.updateCoefficients(sampleRate, thisFreq, thisQ, thisGain);
		dirty = false;
	}
}

template <class FilterSubType>
void MultiChannelFilter<FilterSubType>::update(FilterHelpers::RenderData& renderData)
{
	

	const auto f = renderData.applyModValue(frequency.getNextValue());

	auto thisFreq = FilterLimits::limitFrequency(f);
	auto thisGain = renderData.gainModValue * gain.getNextValue();
	auto thisQ = FilterLimits::limitQ(q.getNextValue() * renderData.qModValue);

	dirty |= compareAndSet(currentFreq, thisFreq);
	dirty |= compareAndSet(currentGain, thisGain);
	dirty |= compareAndSet(currentQ, thisQ);

	if (dirty)
	{
		internalFilter.updateCoefficients(sampleRate, thisFreq, thisQ, thisGain);
		dirty = false;
	}
}

template <class FilterSubType>
MultiChannelFilter<FilterSubType>::MultiChannelFilter() :
	frequency(1000.0),
	q(1.0),
	gain(1.0),
	currentFreq(1000.0),
	currentQ(1.0),
	currentGain(1.0)
{

}

MoogFilterSubType::MoogFilterSubType()
{
	in1 = data;
	in2 = in1 + NUM_MAX_CHANNELS;
	in3 = in2 + NUM_MAX_CHANNELS;
	in4 = in3 + NUM_MAX_CHANNELS;
	out1 = in4 + NUM_MAX_CHANNELS;
	out2 = out1 + NUM_MAX_CHANNELS;
	out3 = out2 + NUM_MAX_CHANNELS;
	out4 = out3 + NUM_MAX_CHANNELS;

	reset(NUM_MAX_CHANNELS);

	updateCoefficients(44100.0, 20000.0, 1.0, 0.0);
}


hise::FilterHelpers::FilterSubType MoogFilterSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::MoogFilterSubType;
}

juce::Identifier MoogFilterSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("moog");
}

MoogFilterSubType::~MoogFilterSubType()
{

}

juce::StringArray MoogFilterSubType::getModes() const
{
	return { "One Pole", "Two Poles", "Four Poles" };
}

Array<hise::FilterHelpers::CoefficientType> MoogFilterSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::LowPassReso, FilterHelpers::LowPassReso, FilterHelpers::LowPassReso };
}

void MoogFilterSubType::reset(int numChannels)
{
	memset(data, 0, sizeof(double)*numChannels * 8);
}

void MoogFilterSubType::setMode(int newMode)
{
	mode = (Mode)newMode;
}

void MoogFilterSubType::setType(int /*t*/)
{

}

void MoogFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	auto lFrequency = FilterLimits::limitFrequency(frequency);

	fc = lFrequency / (0.5 *sampleRate);
	res = q / 2.0;
	if (res > 4.0) res = 4.0;
	f = fc * 1.16;
	fss = (f * f) * (f * f);
	invF = 1.0 - f;
	fb = res * (1.0 - 0.15 * f * f);
}

void MoogFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	for (int c = 0; c < buffer.getNumChannels(); c++)
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

void MoogFilterSubType::processFrame(float* frameData, int numChannels)
{
	for (int c = 0; c < numChannels; c++)
	{
		double input = (double)frameData[c];

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
		frameData[c] = 2.0f * (float)out4[c];
	}
}

DEFINE_MULTI_CHANNEL_FILTER(MoogFilterSubType);

hise::FilterHelpers::FilterSubType SimpleOnePoleSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::SimpleOnePoleSubType;
}

juce::Identifier SimpleOnePoleSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("one_pole");
}

juce::StringArray SimpleOnePoleSubType::getModes() const
{
	return { "LP", "HP" };
}

Array<hise::FilterHelpers::CoefficientType> SimpleOnePoleSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::LowPass, FilterHelpers::HighPass };
}

SimpleOnePoleSubType::SimpleOnePoleSubType()
{
	onePoleType = SimpleOnePoleSubType::LP;
	memset(lastValues, 0, sizeof(float)*NUM_MAX_CHANNELS);
}

void SimpleOnePoleSubType::setType(int t)
{
	onePoleType = (FilterType)t;
}

void SimpleOnePoleSubType::reset(int numChannels)
{
	memset(lastValues, 0, sizeof(float)*numChannels);
}

void SimpleOnePoleSubType::updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/)
{
	const double x = exp(-2.0*double_Pi*frequency / sampleRate);

	a0 = (float)(1.0 - x);
	b1 = (float)-x;
}

void SimpleOnePoleSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	lastChannelAmount = buffer.getNumChannels();

	switch (onePoleType)
	{
	case FilterType::HP:
	{
		for (int c = 0; c < lastChannelAmount; c++)
		{
			float *d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				const float tmp = a0 * d[i] - b1 * lastValues[c];
				lastValues[c] = tmp;
				d[i] = d[i] - tmp;

			}
		}

		break;
	}
	case FilterType::LP:
	{
		for (int c = 0; c < lastChannelAmount; c++)
		{
			float *d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; i++)
			{
				d[i] = a0 * d[i] - b1 * lastValues[c];
				lastValues[c] = d[i];
			}
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

void SimpleOnePoleSubType::processFrame(float* d, int numChannels)
{
	switch (onePoleType)
	{
	case FilterType::HP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			const float tmp = a0 * d[c] - b1 * lastValues[c];
			lastValues[c] = tmp;
			d[c] = d[c] - tmp;
		}

		break;
	}
	case FilterType::LP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			d[c] = a0 * d[c] - b1 * lastValues[c];
			lastValues[c] = d[c];
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

DEFINE_MULTI_CHANNEL_FILTER(SimpleOnePoleSubType);

hise::FilterHelpers::FilterSubType RingmodFilterSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::RingmodFilterSubType;
}

juce::Identifier RingmodFilterSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("ring_mod");
}

juce::StringArray RingmodFilterSubType::getModes() const
{
	return { "RingMod" };
}

Array<hise::FilterHelpers::CoefficientType> RingmodFilterSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::AllPass };
}

void RingmodFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	uptimeDelta = frequency / sampleRate * 2.0 * double_Pi;
	oscGain = jmap<float>((float)q, 0.3f, 9.9f, 0.0f, 1.0f);
	oscGain = jlimit<float>(0.0f, 1.0f, oscGain);
}

void RingmodFilterSubType::reset(int /*numChannels*/)
{
	uptime = 0.0;
}

void RingmodFilterSubType::setType(int /*t*/)
{

}

void RingmodFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	const float invGain = 1.0f - oscGain;

	int numChannels = buffer.getNumChannels();

	for (int i = 0; i < numSamples; i++)
	{
		const float oscValue = oscGain * (float)std::sin(uptime);;

		for (int c = 0; c < numChannels; c++)
		{
			const float input = buffer.getSample(c, i + startSample);
			buffer.setSample(c, i + startSample, invGain * input + input * oscValue);
		}

		uptime += uptimeDelta;
	}
}

void RingmodFilterSubType::processFrame(float* d, int numChannels)
{
	const float invGain = 1.0f - oscGain;
	const float oscValue = oscGain * (float)std::sin(uptime);;

	for (int c = 0; c < numChannels; c++)
	{
		const float input = d[c];
		d[c] = invGain * input + input * oscValue;
	}

	uptime += uptimeDelta;
}

DEFINE_MULTI_CHANNEL_FILTER(RingmodFilterSubType);

hise::FilterHelpers::FilterSubType StaticBiquadSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::StaticBiquadSubType;
}

juce::Identifier StaticBiquadSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("biquad");
}

juce::StringArray StaticBiquadSubType::getModes() const
{
	return { "LowPass", "High Pass", "Low Shelf", "High Shelf", "Peak", "Reso Low" };
}

Array<hise::FilterHelpers::CoefficientType> StaticBiquadSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::LowPass, FilterHelpers::HighPass,
			 FilterHelpers::LowShelf, FilterHelpers::HighShelf,
			 FilterHelpers::Peak, FilterHelpers::LowPassReso };
}

void StaticBiquadSubType::updateCoefficients(double sampleRate, double frequency, double q, double gain)
{
	switch (biquadType)
	{
	case LowPass:		currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
	case HighPass:		currentCoefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
	case LowShelf:		currentCoefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, (float)gain); break;
	case HighShelf:		currentCoefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, (float)gain); break;
	case Peak:			currentCoefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, (float)gain); break;
	case ResoLow:		currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, frequency, q); break;
	default:							jassertfalse; break;
	}

	for (int i = 0; i < numChannels; i++)
	{
		filters[i].setCoefficients(currentCoefficients);
	}
}

void StaticBiquadSubType::setType(int newType)
{
	biquadType = (FilterType)newType;
	reset(numChannels);
}

void StaticBiquadSubType::reset(int numNewChannels)
{
	numChannels = numNewChannels;

	for (int i = 0; i < numChannels; i++)
	{
		filters[i].reset();
	}
}

void StaticBiquadSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	int channelAmount = b.getNumChannels();

	for (int i = 0; i < channelAmount; i++)
	{
		float* d = b.getWritePointer(i, startSample);
		filters[i].processSamples(d, numSamples);
	}
}

void StaticBiquadSubType::processFrame(float* d, int channels)
{
	for (int i = 0; i < channels; i++)
	{
		d[i] = filters[i].processSingleSampleRaw(d[i]);
	}
}

DEFINE_MULTI_CHANNEL_FILTER(StaticBiquadSubType);

hise::FilterHelpers::FilterSubType PhaseAllpassSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::PhaseAllpassSubType;
}

juce::Identifier PhaseAllpassSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("allpass");
}

juce::StringArray PhaseAllpassSubType::getModes() const
{
	return { "All Pass" };
}

Array<hise::FilterHelpers::CoefficientType> PhaseAllpassSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::AllPass };
}

void PhaseAllpassSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	for (int c = 0; c < numFilters; c++)
	{
		for (int i = 0; i < numSamples; i++)
		{
			float* d = b.getWritePointer(c, i + startSample);
			*d = filters[c].getNextSample(*d);
		}
	}
}

void PhaseAllpassSubType::processFrame(float* d, int numChannels)
{
	for (int i = 0; i < numChannels; i++)
		d[i] = filters[i].getNextSample(d[i]);
}

void PhaseAllpassSubType::setType(int /**/)
{

}

void PhaseAllpassSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	if (sampleRate <= 0.0f)
	{
		return;
	}

	for (int i = 0; i < numFilters; i++)
	{
		filters[i].fMin = (float)frequency;
		filters[i].minDelay = (float)(frequency / (sampleRate / 2.0));
		filters[i].feedback = (float)jmap<double>(q, 0.3, 9.9, 0.0, 0.99);
	}
}

void PhaseAllpassSubType::reset(int newNumChannels)
{
	numFilters = newNumChannels;
}

PhaseAllpassSubType::InternalFilter::InternalFilter()
{
	reset();
}

void PhaseAllpassSubType::InternalFilter::reset()
{
	currentValue = 0.0f;
}

float PhaseAllpassSubType::InternalFilter::getNextSample(float input)
{
	float delayThisSample = minDelay;

	const float delayCoefficient = AllpassDelay::getDelayCoefficient(delayThisSample);

	allpassFilters[0].setDelay(delayCoefficient);
	allpassFilters[1].setDelay(delayCoefficient);
	allpassFilters[2].setDelay(delayCoefficient);
	allpassFilters[3].setDelay(delayCoefficient);
	allpassFilters[4].setDelay(delayCoefficient);
	allpassFilters[5].setDelay(delayCoefficient);

	float output = allpassFilters[0].getNextSample(
		allpassFilters[1].getNextSample(
			allpassFilters[2].getNextSample(
				allpassFilters[3].getNextSample(
					allpassFilters[4].getNextSample(
						allpassFilters[5].getNextSample(input + currentValue * feedback))))));

	currentValue = output;

	return input + output;
}

DEFINE_MULTI_CHANNEL_FILTER(PhaseAllpassSubType);

hise::FilterHelpers::FilterSubType LadderSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::LadderSubType;
}

juce::Identifier LadderSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("ladder");
}

Array<hise::FilterHelpers::CoefficientType> LadderSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::LowPassReso };
}

juce::StringArray LadderSubType::getModes() const
{
	return { "LP24" };
}

void LadderSubType::reset(int newNumChannels)
{
	memset(buf, 0, sizeof(float) * newNumChannels * 4);
}

void LadderSubType::setType(int /*t*/)
{

}

void LadderSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	for (int c = 0; c < b.getNumChannels(); c++)
	{
		for (int i = 0; i < numSamples; i++)
		{
			float* d = b.getWritePointer(c, i + startSample);
			*d = processSample(*d, c);
		}
	}
}

void LadderSubType::processFrame(float* d, int numChannels)
{
	for (int c = 0; c < numChannels; c++)
	{
		d[c] = processSample(d[c], c);
	}
}

void LadderSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	float inFreq = (float)FilterLimits::limitFrequency(frequency);

	const float x = 2.0f * float_Pi*inFreq / (float)sampleRate;

	cut = jlimit<float>(0.0f, 0.8f, x);
	res = jlimit<float>(0.3f, 4.0f, (float)q / 2.0f);
}

float LadderSubType::processSample(float input, int channel)
{
	float* buffer = buf[channel];

	float resoclip = buffer[3];

	const float in = input - (resoclip * res);
	buffer[0] = ((in - buffer[0]) * cut) + buffer[0];
	buffer[1] = ((buffer[0] - buffer[1]) * cut) + buffer[1];
	buffer[2] = ((buffer[1] - buffer[2]) * cut) + buffer[2];
	buffer[3] = ((buffer[2] - buffer[3]) * cut) + buffer[3];
	return 2.0f * buffer[3];
}

DEFINE_MULTI_CHANNEL_FILTER(LadderSubType);

hise::FilterHelpers::FilterSubType StateVariableFilterSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::StateVariableFilterSubType;
}

juce::Identifier StateVariableFilterSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("svf");
}

juce::StringArray StateVariableFilterSubType::getModes() const
{
	return { "LP", "HP", "BP", "Notch", "Allpass" };
}

Array<hise::FilterHelpers::CoefficientType> StateVariableFilterSubType::getCoefficientTypeList() const
{
	return { FilterHelpers::LowPassReso, FilterHelpers::HighPass,
			 FilterHelpers::BandPass, FilterHelpers::BandPass,
			 FilterHelpers::AllPass };
}

StateVariableFilterSubType::StateVariableFilterSubType()
{
	memset(v0z, 0, sizeof(float)*NUM_MAX_CHANNELS);
	memset(z1_A, 0, sizeof(float)*NUM_MAX_CHANNELS);
	memset(v2, 0, sizeof(float)*NUM_MAX_CHANNELS);
}

void StateVariableFilterSubType::reset(int numChannels)
{
	memset(v0z, 0, sizeof(float)*numChannels);
	memset(z1_A, 0, sizeof(float)*numChannels);
	memset(v2, 0, sizeof(float)*numChannels);
}

void StateVariableFilterSubType::setType(int t)
{
	type = (FilterType)t;
}

void StateVariableFilterSubType::updateCoefficients(double sampleRate, double frequency, double q, double /*gain*/)
{
	const float scaledQ = jlimit<float>(0.0f, 9.999f, (float)q * 0.1f);

	if (type == FilterType::ALLPASS)
	{
		// pre-warp the cutoff (for bilinear-transform filters)
		float wd = static_cast<float>(frequency * 2.0f * float_Pi);
		float T = 1.0f / (float)sampleRate;
		float wa = (2.0f / T) * tan(wd * T / 2.0f);

		// Calculate g (gain element of integrator)
		gCoeff = wa * T / 2.0f;			// Calculate g (gain element of integrator)

										// Calculate Zavalishin's R from Q (referred to as damping parameter)
		RCoeff = 1.0f / (2.0f * (float)q);

		x1 = (2.0f * RCoeff + gCoeff);
		x2 = 1.0f / (1.0f + (2.0f * RCoeff * gCoeff) + gCoeff * gCoeff);
	}
	else
	{
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
}

void StateVariableFilterSubType::processSamples(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	auto numChannels = buffer.getNumChannels();

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
				float v1z = z1_A[c];
				float v2z = v2[c];
				float v3 = v0 + v0z[c] - 2.0f * v2z;
				z1_A[c] += g1 * v3 - g2 * v1z;
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
				float v1z = z1_A[c];
				float v2z = v2[c];
				float v3 = v0 + v0z[c] - 2.0f * v2z;
				z1_A[c] += g1 * v3 - g2 * v1z;
				v2[c] += g3 * v3 + g4 * v1z;
				v0z[c] = v0;
				d[i] = z1_A[c];
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
				float v1z = z1_A[c];
				float v2z = v2[c];
				float v3 = v0 + v0z[c] - 2.0f * v2z;
				z1_A[c] += g1 * v3 - g2 * v1z;
				v2[c] += g3 * v3 + g4 * v1z;
				v0z[c] = v0;
				d[i] = v0 - k * z1_A[c] - v2[c];
			}
		}

		break;
	}
	case FilterType::ALLPASS:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float* d = buffer.getWritePointer(c, startSample);

			for (int i = 0; i < numSamples; ++i)
			{
				const float input = d[i];
				const float HP = (input - x1 * z1_A[c] - v2[c]) / x2;
				const float BP = HP * gCoeff + z1_A[c];
				const float LP = BP * gCoeff + v2[c];

				z1_A[c] = gCoeff * HP + BP;
				v2[c] = gCoeff * BP + LP;

				const float AP = input - (4.0f * RCoeff * BP);
				d[i] = AP;
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
				float v1z = z1_A[c];
				float v2z = v2[c];
				float v3 = v0 + v0z[c] - 2.0f * v2z;
				z1_A[c] += g1 * v3 - g2 * v1z;
				v2[c] += g3 * v3 + g4 * v1z;
				v0z[c] = v0;
				d[i] = v0 - k * z1_A[c];
			}
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

void StateVariableFilterSubType::processFrame(float* d, int numChannels)
{
	switch (type)
	{
	case LP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v2[c];
		}

		break;
	}
	case BP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = z1_A[c];
		}

		break;
	}

	case HP:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v0 - k * z1_A[c] - v2[c];
		}

		break;
	}
	case FilterType::ALLPASS:
	{
		for (int c = 0; c < numChannels; c++)
		{
			const float input = d[c];
			const float HP = (input - x1 * z1_A[c] - v2[c]) / x2;
			const float BP = HP * gCoeff + z1_A[c];
			const float LP = BP * gCoeff + v2[c];

			z1_A[c] = gCoeff * HP + BP;
			v2[c] = gCoeff * BP + LP;

			const float AP = input - (4.0f * RCoeff * BP);
			d[c] = AP;
		}
		break;
	}

	case NOTCH:
	{
		for (int c = 0; c < numChannels; c++)
		{
			float v0 = d[c];
			float v1z = z1_A[c];
			float v2z = v2[c];
			float v3 = v0 + v0z[c] - 2.0f * v2z;
			z1_A[c] += g1 * v3 - g2 * v1z;
			v2[c] += g3 * v3 + g4 * v1z;
			v0z[c] = v0;
			d[c] = v0 - k * z1_A[c];
		}

		break;
	}
	default:
		jassertfalse;
		break;
	}
}

DEFINE_MULTI_CHANNEL_FILTER(StateVariableFilterSubType);

hise::FilterHelpers::FilterSubType LinkwitzRiley::getFilterType()
{
	return FilterHelpers::FilterSubType::LinkwitzRiley;
}

juce::Identifier LinkwitzRiley::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("linkwitzriley");
}

juce::StringArray LinkwitzRiley::getModes() const
{
	return { "LP", "HP", "AP" };
}

Array<hise::FilterHelpers::CoefficientType> LinkwitzRiley::getCoefficientTypeList() const
{

	return { FilterHelpers::LowPass, FilterHelpers::HighPass, FilterHelpers::AllPass };
}

void LinkwitzRiley::setType(int type)
{
	mode = type;
}

void LinkwitzRiley::reset(int numChannels)
{
	memset(hpData, 0, sizeof(Data)*numChannels);
	memset(lpData, 0, sizeof(Data)*numChannels);
}

void LinkwitzRiley::processSamples(AudioSampleBuffer& buffer, int startSample, int)
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

void LinkwitzRiley::processFrame(float* frameData, int numChannels)
{
	for (int i = 0; i < numChannels; i++)
	{
		frameData[i] = process(frameData[i], i);
	}
}

void LinkwitzRiley::updateCoefficients(double sampleRate, double frequency, double /*q*/, double /*gain*/)
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

	SpinLock::ScopedLockType sl(lock);

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

float LinkwitzRiley::process(float input, int channel)
{
	SpinLock::ScopedLockType sl(lock);

	double tempx, hp, lp;
	tempx = (double)input;

	auto& hptemp = hpData[channel];
	auto& lptemp = lpData[channel];

	// High pass
	hp = hpco.coefficients[0] * tempx +
		hpco.coefficients[1] * hptemp.xm1 +
		hpco.coefficients[2] * hptemp.xm2 +
		hpco.coefficients[3] * hptemp.xm3 +
		hpco.coefficients[4] * hptemp.xm4 -
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
	hptemp.ym1 = hp;

	// Low pass

	lp =
		lpco.coefficients[0] * tempx +
		lpco.coefficients[1] * lptemp.xm1 +
		lpco.coefficients[2] * lptemp.xm2 +
		lpco.coefficients[3] * lptemp.xm3 +
		lpco.coefficients[4] * lptemp.xm4 -
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
	lptemp.ym1 = lp;

	switch (mode)
	{
	case LP: return static_cast<float>(lp);
	case HP: return static_cast<float>(hp);
	case Allpass: return static_cast<float>(lp + hp);
	default: return 0.0f;
	}
}

DEFINE_MULTI_CHANNEL_FILTER(LinkwitzRiley);

PhaseAllpassSubType::InternalFilter::AllpassDelay::AllpassDelay() :
	delay(0.f),
	currentValue(0.f)
{

}

float PhaseAllpassSubType::InternalFilter::AllpassDelay::getDelayCoefficient(float delaySamples)
{
	return (1.f - delaySamples) / (1.f + delaySamples);
}

void PhaseAllpassSubType::InternalFilter::AllpassDelay::setDelay(float newDelay) noexcept
{
	delay = newDelay;
}

float PhaseAllpassSubType::InternalFilter::AllpassDelay::getNextSample(float input) noexcept
{
	float y = input * -delay + currentValue;
	currentValue = y * delay + input;

	return y;
}

juce::Identifier StateVariableEqSubType::getStaticId()
{
	RETURN_STATIC_IDENTIFIER("svf_eq");
}

hise::FilterHelpers::FilterSubType StateVariableEqSubType::getFilterType()
{
	return FilterHelpers::FilterSubType::StateVariableEqSubType;
}

juce::Array<hise::FilterHelpers::CoefficientType> StateVariableEqSubType::getCoefficientTypeList() const
{
	return
	{
		FilterHelpers::LowPassReso,
		FilterHelpers::HighPass,
		FilterHelpers::LowShelf,
		FilterHelpers::HighShelf,
		FilterHelpers::Peak,
	};
}

juce::StringArray StateVariableEqSubType::getModes() const
{
	return
	{
		"LowPass",
		"HighPass",
		"LowShelf",
		"HighShelf",
		"Peak"
	};
}

void StateVariableEqSubType::reset(int newNumChannels)
{
	for (int i = 0; i < newNumChannels; i++)
		states[i].reset();
}

void StateVariableEqSubType::setType(int newType)
{
	t = (Mode)newType;
}

void StateVariableEqSubType::processSamples(AudioSampleBuffer& b, int startSample, int numSamples)
{
	auto numChannels = b.getNumChannels();
	auto ptrs = b.getArrayOfWritePointers();

	for (int i = startSample; i < startSample + numSamples; i++)
	{
		coefficients.tick();

		for (int c = 0; c < numChannels; c++)
		{
			auto v = states[c].tick(ptrs[c][i], coefficients);
			ptrs[c][i] = v;
		}
	}
}

void StateVariableEqSubType::processFrame(float* frameData, int numChannels)
{
	coefficients.tick();

	for (int c = 0; c < numChannels; c++)
	{
		frameData[c] = states[c].tick(frameData[c], coefficients);
	}
}

void StateVariableEqSubType::updateCoefficients(double sampleRate, double frequency, double q, double gain)
{
	coefficients.setGain(Decibels::gainToDecibels(gain));
	coefficients.update(frequency, q, t, sampleRate);
}

StateVariableEqSubType::Coefficients::Coefficients()
{
	memset(a, 0, sizeof(double) * 4);
	memset(ap, 0, sizeof(double) * 4);
	memset(m, 0, sizeof(double) * 4);
	memset(mp, 0, sizeof(double) * 4);
	gain = gain_sqrt = 1;
}

void StateVariableEqSubType::Coefficients::update(double freq, double q, StateVariableEqSubType::Mode type, double sampleRate)
{
	double g = std::tan((freq / sampleRate) * double_Pi);
	double k = computeK(q, type == Peak);

	switch (type) {
	case LowPass:
		m[0] = 0.0;
		m[1] = 0.0;
		m[2] = 1.0;
		break;
	case HighPass:
		m[0] = 1.0;
		m[1] = -k;
		m[2] = -1.0;
		break;
	case Peak:
		m[0] = 1.0;
		m[1] = k * (gain*gain - 1.0);
		m[2] = 0.0;
		break;
	case LowShelf:
		g /= gain_sqrt;
		m[0] = 1.0;
		m[1] = k * (gain - 1.0);
		m[2] = gain * gain - 1.0;
		break;
	case HighShelf:
		g *= gain_sqrt;
		m[0] = gain * gain;
		m[1] = k * (1.0 - gain)*gain;
		m[2] = 1.0 - gain * gain;
		break;
	case numModes:
	default:
		jassertfalse;
	}

	computeA(g, k);
}

void StateVariableEqSubType::Coefficients::setGain(double gainDb)
{
	gain = std::pow(10.0, gainDb / 40.0);
	gain_sqrt = std::sqrt(gain);
}

double StateVariableEqSubType::Coefficients::computeK(double q, bool useGain /*= false*/)
{
	return 1.f / (useGain ? (q*gain) : q);
}

void StateVariableEqSubType::Coefficients::computeA(double g, double k)
{
	a[0] = 1 / (1 + g * (g + k));
	a[1] = g * a[0];
	a[2] = g * a[1];
}

void StateVariableEqSubType::Coefficients::tick()
{
	const double gain = 0.99;
	const double invGain = 1.0 - gain;

#if 1 || HI_ENABLE_LEGACY_CPU_SUPPORT || !JUCE_WINDOWS

	mp[0] *= gain;
	mp[1] *= gain;
	mp[2] *= gain;

	mp[0] += m[0] * invGain;
	mp[1] += m[1] * invGain;
	mp[2] += m[2] * invGain;

	ap[0] *= gain;
	ap[1] *= gain;
	ap[2] *= gain;

	ap[0] += a[0] * invGain;
	ap[1] += a[1] * invGain;
	ap[2] += a[2] * invGain;

#else
	const auto gain_ = _mm256_set1_pd(gain);
	const auto invGain_ = _mm256_set1_pd(invGain);

	auto _mp_ = _mm256_load_pd(mp);
	auto _m_ = _mm256_load_pd(m);
	auto _ap_ = _mm256_load_pd(ap);
	auto _a_ = _mm256_load_pd(a);

	_mp_ = _mm256_mul_pd(_mp_, gain_);
	_m_ = _mm256_mul_pd(_m_, invGain_);
	_mp_ = _mm256_add_pd(_mp_, _m_);
	_mm256_store_pd(mp, _mp_);

	_ap_ = _mm256_mul_pd(_ap_, gain_);
	_a_ = _mm256_mul_pd(_a_, invGain_);
	_ap_ = _mm256_add_pd(_ap_, _a_);
	_mm256_store_pd(ap, _ap_);
#endif
}

StateVariableEqSubType::State::State()
{
	reset();
}

void StateVariableEqSubType::State::reset()
{
	memset(v, 0, sizeof(double) * 4);
	_ic1eq = _ic2eq = 0.0;
}

float StateVariableEqSubType::State::tick(float inp, const Coefficients& c)
{
	v[0] = (double)inp;

	v[3] = v[0] - _ic2eq;
	v[1] = c.ap[0] * _ic1eq + c.ap[1] * v[3];
	v[2] = _ic2eq + c.ap[1] * _ic1eq + c.ap[2] * v[3];
	_ic1eq = 2 * v[1] - _ic1eq;
	_ic2eq = 2 * v[2] - _ic2eq;

	return (float)(c.mp[0] * v[0] + c.mp[1] * v[1] + c.mp[2] * v[2]);
}

DEFINE_MULTI_CHANNEL_FILTER(StateVariableEqSubType);


double FilterHelpers::RenderData::applyModValue(double f) const
{
	bool calcModulation = !HISE_LOG_FILTER_FREQMOD || ((1.0 + bipolarDelta) * freqModValue != 1.0);

	if (!calcModulation)
		return f;

	f -= 20.0;
	f *= 1.0 / 19980.0;

#if HISE_LOG_FILTER_FREQMOD
	const double skew = 0.2299045622348785;
	f = hmath::pow(f, skew);
#endif

	f += bipolarDelta;
	f *= freqModValue;

#if HISE_LOG_FILTER_FREQMOD
	f = hmath::pow(jmax(0.0, f), 1.0 / skew);
#endif

	f *= 19980.0;
	f += 20.0;
	
	return f;
}

}