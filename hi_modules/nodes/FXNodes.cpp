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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace fx
{



template <int V>
sampleandhold_impl<V>::sampleandhold_impl()
{

}

template <int V>
void sampleandhold_impl<V>::setFactor(double value)
{
	auto factor = jlimit(1, 44100, roundToInt(value));

	if (data.isVoiceRenderingActive())
		data.get().factor = factor;
	else
		data.forEachVoice([factor](Data& d_) {d_.factor = factor; });
}

template <int V>
void sampleandhold_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Counter");
		p.range = { 1, 64, 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(sampleandhold_impl::setFactor);

		data.add(std::move(p));
	}
}

template <int V>
void sampleandhold_impl<V>::processSingle(float* numFrames, int numChannels)
{
	auto& v = data.get();

	if (v.counter == 0)
	{
		FloatVectorOperations::copy(v.currentValues, numFrames, numChannels);
		v.counter = v.factor;
	}
	else
	{
		FloatVectorOperations::copy(numFrames, v.currentValues, numChannels);
		v.counter--;
	}
}

template <int V>
void sampleandhold_impl<V>::reset() noexcept
{
	if (data.isVoiceRenderingActive())
		data.get().clear(lastChannelAmount);
	else
		data.forEachVoice([this](Data& d_) {d_.clear(lastChannelAmount); });
}

template <int V>
void sampleandhold_impl<V>::process(ProcessData& d)
{
	Data& v = data.get();

	if (v.counter > d.size)
	{
		for (int i = 0; i < d.numChannels; i++)
			FloatVectorOperations::fill(d.data[i], v.currentValues[i], d.size);

		v.counter -= d.size;
	}
	else
	{
		for (int i = 0; i < d.size; i++)
		{
			if (v.counter == 0)
			{
				for (int c = 0; c < d.numChannels; c++)
				{
					v.currentValues[c] = d.data[c][i];
					v.counter = v.factor + 1;
				}
			}

			for (int c = 0; c < d.numChannels; c++)
				d.data[c][i] = v.currentValues[c];

			v.counter--;
		}
	}
}

template <int V>
void sampleandhold_impl<V>::prepare(PrepareSpecs ps)
{
	data.prepare(ps);
	lastChannelAmount = ps.numChannels;
}

template <int V>
void sampleandhold_impl<V>::initialise(NodeBase* n)
{

}

DEFINE_EXTERN_NODE_TEMPIMPL(sampleandhold_impl);



template <int V>
bitcrush_impl<V>::bitcrush_impl()
{
	bitDepth.setAll(16.0f);
}

template <int V>
void bitcrush_impl<V>::setBitDepth(double newBitDepth)
{
	auto v = jlimit(1.0f, 16.0f, (float)newBitDepth);

	if (bitDepth.isVoiceRenderingActive())
		bitDepth.get() = v;
	else
		bitDepth.setAll(v);
}

template <int V>
void bitcrush_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Bit Depth");

		p.range = { 4.0, 16.0, 0.1 };
		p.defaultValue = 16.0;
		p.db = BIND_MEMBER_FUNCTION_1(bitcrush_impl::setBitDepth);

		data.add(std::move(p));
	}
}

template <int V>
bool bitcrush_impl<V>::handleModulation(double&) noexcept
{
	return false;
}

template <int V>
void bitcrush_impl<V>::reset() noexcept
{

}



// ======================================================================================================

void getBitcrushedValue(float* data, int numSamples, float bitDepth)
{
	const float invStepSize = pow(2.0f, bitDepth);
	const float stepSize = 1.0f / invStepSize;

	for (int i = 0; i < numSamples; i++)
		data[i] = (stepSize * ceil(data[i] * invStepSize) - 0.5f * stepSize);
}

// ======================================================================================================

template <int V>
void bitcrush_impl<V>::process(ProcessData& d)
{
	for (auto ch : d)
		getBitcrushedValue(ch, d.size, bitDepth.get());
}


template <int V>
void bitcrush_impl<V>::processSingle(float* numFrames, int numChannels)
{
	getBitcrushedValue(numFrames, numChannels, bitDepth.get());
}

template <int V>
void bitcrush_impl<V>::prepare(PrepareSpecs ps)
{
	bitDepth.prepare(ps);
}

template <int V>
void bitcrush_impl<V>::initialise(NodeBase* n)
{

}

DEFINE_EXTERN_NODE_TEMPIMPL(bitcrush_impl)

template <int V>
phase_delay_impl<V>::phase_delay_impl()
{

}

template <int V>
void phase_delay_impl<V>::initialise(NodeBase* n)
{
	
}

template <int V>
void phase_delay_impl<V>::prepare(PrepareSpecs ps)
{
	sr = ps.sampleRate * 0.5;
	delays[0].prepare(ps);
	delays[1].prepare(ps);
}

template <int V>
void phase_delay_impl<V>::process(ProcessData& d)
{
	int numChannels = jmin(2, d.numChannels);

	for (int c = 0; c < numChannels; c++)
	{
		auto ptr = d.data[c];
		auto& dl = delays[c].get();

		for (int i = 0; i < d.size; i++)
			*ptr++ = dl.getNextSample(*ptr);
	}
}

template <int V>
void scriptnode::fx::phase_delay_impl<V>::reset() noexcept
{
	if (delays[0].isVoiceRenderingActive())
	{
		delays[0].get().reset();
		delays[1].get().reset();
	}
	else
	{
		auto f = [](AllpassDelay& d) { d.reset(); };
		delays[0].forEachVoice(f);
		delays[1].forEachVoice(f);
	}
}

template <int V>
void phase_delay_impl<V>::processSingle(float* numFrames, int numChannels)
{
	numChannels = jmin(2, numChannels);

	for (int i = 0; i < numChannels; i++)
		numFrames[i] = delays[i].get().getNextSample(numFrames[i]);
}

template <int V>
bool phase_delay_impl<V>::handleModulation(double&) noexcept
{
	return false;
}

template <int V>
void phase_delay_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Frequency");
		p.range = { 20.0, 20000.0, 0.1 };
		p.range.setSkewForCentre(1000.0);
		p.defaultValue = 400.0;
		p.db = BIND_MEMBER_FUNCTION_1(phase_delay_impl::setFrequency);

		data.add(std::move(p));
	}
}

template <int V>
void phase_delay_impl<V>::setFrequency(double newFrequency)
{
	newFrequency /= sr;

	auto coefficient = AllpassDelay::getDelayCoefficient(newFrequency);

	if (delays[0].isVoiceRenderingActive())
	{
		delays[0].get().setDelay(coefficient);
		delays[1].get().setDelay(coefficient);
	}
		
	else
	{
		auto f = [coefficient](AllpassDelay& d) {d.setDelay(coefficient); };
		delays[0].forEachVoice(f);
		delays[1].forEachVoice(f);
	}
		
}

DEFINE_EXTERN_NODE_TEMPIMPL(phase_delay_impl);

template <int V>
void haas_impl<V>::setPosition(double newValue)
{
	position = newValue;

	auto d = std::abs(position) * 0.02;

	if (delayL.isVoiceRenderingActive())
	{
		if (position == 0.0)
		{
			delayL.get().setDelayTimeSamples(0);
			delayR.get().setDelayTimeSamples(0);
		}
		else if (position > 0.0)
		{
			delayL.get().setDelayTimeSeconds(d);
			delayR.get().setDelayTimeSamples(0);
		}
		else if (position < 0.0)
		{
			delayL.get().setDelayTimeSamples(0);
			delayR.get().setDelayTimeSeconds(d);
		}
	}
	else
	{
		// We don't allow fade times in polyphonic effects because there is no constant flow of signal that
		// causes issues with the fade time logic...
		int fadeTime = NumVoices == 1 ? 2048 : 0;

		auto setZero = [fadeTime](DelayType& t) { t.setFadeTimeSamples(fadeTime); t.setDelayTimeSamples(0); };
		auto setSeconds = [fadeTime, d](DelayType& t) { t.setFadeTimeSamples(fadeTime); t.setDelayTimeSeconds(d); };

		if (position == 0.0)
		{
			delayL.forEachVoice(setZero);
			delayR.forEachVoice(setZero);
		}
		else if (position > 0.0)
		{
			delayL.forEachVoice(setSeconds);
			delayR.forEachVoice(setZero);
		}
		else if (position < 0.0)
		{
			delayL.forEachVoice(setZero);
			delayR.forEachVoice(setSeconds);
		}
	}
}


template <int V>
bool haas_impl<V>::handleModulation(double&)
{
	return false;
}

template <int V>
void haas_impl<V>::process(ProcessData& d)
{
	if (d.numChannels == 2)
	{
		delayL.get().processBlock(d.data[0], d.size);
		delayR.get().processBlock(d.data[1], d.size);
	}
}

template <int V>
void haas_impl<V>::processSingle(float* data, int numChannels)
{
	if (numChannels == 2)
	{
		data[0] = delayL.get().getDelayedValue(data[0]);
		data[1] = delayR.get().getDelayedValue(data[1]);
	}
}

template <int V>
void haas_impl<V>::reset()
{

	if (delayL.isVoiceRenderingActive())
	{
		jassert(delayR.isVoiceRenderingActive());

		delayL.get().setFadeTimeSamples(0);
		delayR.get().setFadeTimeSamples(0);

		delayL.get().clear();
		delayR.get().clear();
	}
	else
	{
		auto f = [](DelayType& d) {d.clear(); };
		delayL.forEachVoice(f);
		delayR.forEachVoice(f);
	}
}

template <int V>
void haas_impl<V>::prepare(PrepareSpecs ps)
{
	delayL.prepare(ps);
	delayR.prepare(ps);

	auto sr = ps.sampleRate;
	auto f = [sr](DelayType& d)
	{
		d.prepareToPlay(sr);
		d.setFadeTimeSamples(0);
	};

	delayL.forEachVoice(f);
	delayR.forEachVoice(f);

	setPosition(position);
}

template <int V>
void haas_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Position");
		p.range = { -1.0, 1.0, 0.1 };
		p.defaultValue = 0.0;
		p.db = BIND_MEMBER_FUNCTION_1(haas_impl::setPosition);
		data.add(std::move(p));
	}
}


DEFINE_EXTERN_NODE_TEMPIMPL(haas_impl);

reverb::reverb()
{
	auto p = r.getParameters();
	p.dryLevel = 0.0f;
	r.setParameters(p);
}

void reverb::initialise(NodeBase* n)
{
	
}

void reverb::prepare(PrepareSpecs ps)
{
	r.setSampleRate(ps.sampleRate);
}

void reverb::process(ProcessData& d)
{
	if (d.numChannels == 2)
		r.processStereo(d.data[0], d.data[1], d.size);
	else if (d.numChannels == 1)
		r.processMono(d.data[0], d.size);
}

void reverb::reset() noexcept
{
	r.reset();
}

void reverb::processSingle(float* numFrames, int numChannels)
{
	if (numChannels == 2)
		r.processStereo(numFrames, numFrames + 1, 1);
	else if (numChannels == 1)
		r.processMono(numFrames, 1);
}

bool reverb::handleModulation(double&) noexcept
{
	return false;
}

void reverb::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Size");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
		p.db = BIND_MEMBER_FUNCTION_1(reverb::setSize);
		data.add(std::move(p));
	}

	{
		ParameterData p("Damping");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
		p.db = BIND_MEMBER_FUNCTION_1(reverb::setDamping);
		data.add(std::move(p));
	}

	{
		ParameterData p("Width");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
		p.db = BIND_MEMBER_FUNCTION_1(reverb::setWidth);
		data.add(std::move(p));
	}
}

void reverb::setDamping(double newDamping)
{
	auto p = r.getParameters();
	p.damping = jlimit(0.0f, 1.0f, (float)newDamping);
	r.setParameters(p);
}

void reverb::setWidth(double width)
{
	auto p = r.getParameters();
	p.damping = jlimit(0.0f, 1.0f, (float)width);
	r.setParameters(p);
}

void reverb::setSize(double size)
{
	auto p = r.getParameters();
	p.roomSize = jlimit(0.0f, 1.0f, (float)size);
	r.setParameters(p);
}

}
}