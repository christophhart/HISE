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

	if (data.isMonophonicOrInsideVoiceRendering())
		data.get().factor = factor;
	else
	{
		for (auto& d : data)
			d.factor = factor;
	}
}

template <int V>
void sampleandhold_impl<V>::createParameters(Array<ParameterData>& d)
{
	{
		ParameterData p("Counter");
		p.range = { 1, 64, 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(sampleandhold_impl::setFactor);

		d.add(std::move(p));
	}
}

template <int V>
void sampleandhold_impl<V>::reset() noexcept
{
	if (data.isMonophonicOrInsideVoiceRendering())
		data.get().clear(lastChannelAmount);
	else
	{
		for (auto& d : data)
			d.clear(lastChannelAmount);
	}
}

template <int V>
void sampleandhold_impl<V>::prepare(PrepareSpecs ps)
{
	data.prepare(ps);
	lastChannelAmount = ps.numChannels;
}

template <int V>
void sampleandhold_impl<V>::initialise(NodeBase* )
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

	if (bitDepth.isMonophonicOrInsideVoiceRendering())
		bitDepth.get() = v;
	else
		bitDepth.setAll(v);
}

template <int V>
void bitcrush_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(bitcrush_impl, BitDepth);
		p.range = { 4.0, 16.0, 0.1 };
		p.defaultValue = 16.0;
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

template <int V>
void bitcrush_impl<V>::prepare(PrepareSpecs ps)
{
	bitDepth.prepare(ps);
}

template <int V>
void bitcrush_impl<V>::initialise(NodeBase* )
{

}

DEFINE_EXTERN_NODE_TEMPIMPL(bitcrush_impl)

template <int V>
phase_delay_impl<V>::phase_delay_impl()
{

}

template <int V>
void phase_delay_impl<V>::initialise(NodeBase* )
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
void scriptnode::fx::phase_delay_impl<V>::reset() noexcept
{
	if (delays[0].isMonophonicOrInsideVoiceRendering())
	{
		delays[0].get().reset();
		delays[1].get().reset();
	}
	else
	{
		for (auto& ds : delays)
		{
			for (auto& d : ds)
				d.reset();
		}
	}
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

	auto coefficient = AllpassDelay::getDelayCoefficient((float)newFrequency);

	if (delays[0].isMonophonicOrInsideVoiceRendering())
	{
		delays[0].get().setDelay(coefficient);
		delays[1].get().setDelay(coefficient);
	}
	else
	{
		for (auto& outer : delays)
		{
			for (auto& d : outer)
				d.setDelay(coefficient);
		}
	}
		
}

DEFINE_EXTERN_NODE_TEMPIMPL(phase_delay_impl);

template <int V>
void haas_impl<V>::setPosition(double newValue)
{
	position = newValue;

	auto d = hmath::abs(position) * 0.02;

	auto& l = delay.get()[0];
	auto& r = delay.get()[1];

	if (delay.isMonophonicOrInsideVoiceRendering())
	{
		if (position == 0.0)
		{
			l.setDelayTimeSamples(0);
			r.setDelayTimeSamples(0);
		}
		else if (position > 0.0)
		{
			l.setDelayTimeSeconds(d);
			r.setDelayTimeSamples(0);
		}
		else if (position < 0.0)
		{
			l.setDelayTimeSamples(0);
			r.setDelayTimeSeconds(d);
		}
	}
	else
	{
		// We don't allow fade times in polyphonic effects because there is no constant flow of signal that
		// causes issues with the fade time logic...
		int fadeTime = NumVoices == 1 ? 2048 : 0;

		jassertfalse;

#if 0

		auto setToPos = [fadeTime, position](DelayType& t) 
		{ 
			auto& l = t[0];
			auto& r = t[0]

			if (position == 0.0)
			{
				l.forEachVoice(setZero);
				r.forEachVoice(setZero);
			}
			else if (position > 0.0)
			{
				l.forEachVoice(setSeconds);
				r.forEachVoice(setZero);
			}
			else if (position < 0.0)
			{
				l.forEachVoice(setZero);
				r.forEachVoice(setSeconds);
			}
		};

		auto setSeconds = [fadeTime, d](DelayType& t) 
		{
			for (auto& d : t)
			{
				d.setFadeTimeSamples(fadeTime);
				d.setDelayTimeSeconds(d);
			}
		};
		
		delay.forEachVoice(setToPos);

#endif
	}
}


template <int V>
bool haas_impl<V>::handleModulation(double&)
{
	return false;
}


template <int V>
void haas_impl<V>::processFrame(FrameType& data)
{
	data[0] = delay.get()[0].getDelayedValue(data[0]);
	data[1] = delay.get()[1].getDelayedValue(data[1]);
}


template <int V>
void haas_impl<V>::process(haas_impl<V>::ProcessType& d)
{
	delay.get()[0].processBlock(d[0]);
	delay.get()[1].processBlock(d[1]);
}


template <int V>
void haas_impl<V>::reset()
{
	if (delay.isMonophonicOrInsideVoiceRendering())
	{
		for (auto& d : delay.get())
		{
			d.setFadeTimeSamples(0);
			d.clear();
		}
	}
	else
	{
		for (auto& d : delay)
		{
			for (auto& inner : d)
				inner.clear();
		}
	}
}

template <int V>
void haas_impl<V>::prepare(PrepareSpecs ps)
{
	delay.prepare(ps);

	auto sr = ps.sampleRate;

	for(auto& d: delay)
	{
		for (auto& inner : d)
		{
			inner.prepareToPlay(sr);
			inner.setFadeTimeSamples(0);
		}
	};

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

void reverb::initialise(NodeBase* )
{
	
}

void reverb::prepare(PrepareSpecs ps)
{
	r.setSampleRate(ps.sampleRate);
}

void reverb::reset() noexcept
{
	r.reset();
}

bool reverb::handleModulation(double&) noexcept
{
	return false;
}

void reverb::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(reverb, Damping);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(reverb, Width);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(reverb, Size);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.5f;
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
