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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::Types;

namespace fx
{

template <int V>
sampleandhold<V>::sampleandhold():
	polyphonic_base(getStaticId(), false)
{

}

template <int V>
void sampleandhold<V>::setCounter(double value)
{
	auto factor = jlimit(1, 44100, roundToInt(value));

	for (auto& d : data)
		d.factor = factor;
}

template <int V>
void sampleandhold<V>::createParameters(ParameterDataList& d)
{
	{
		DEFINE_PARAMETERDATA(sampleandhold, Counter);
		p.setRange({ 1.0, 64, 1.0 });
		p.setDefaultValue(1.0);
		d.add(std::move(p));
	}
}

template <int V>
void sampleandhold<V>::reset() noexcept
{
	for (auto& d : data)
		d.clear(lastChannelAmount);
}

template <int V>
void sampleandhold<V>::prepare(PrepareSpecs ps)
{
	data.prepare(ps);
	lastChannelAmount = ps.numChannels;
}

template <int V>
void sampleandhold<V>::initialise(NodeBase*)
{

}


template <int V>
bitcrush<V>::bitcrush():
	polyphonic_base(getStaticId(), false)
{
	bitDepth.setAll(16.0f);
}

template <int V>
void bitcrush<V>::setBitDepth(double newBitDepth)
{
	auto v = jlimit(1.0f, 16.0f, (float)newBitDepth);

	for (auto& b : bitDepth)
		b = v;
}

template <int V>
void bitcrush<V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(bitcrush, BitDepth);
		p.setRange({ 4.0, 16.0, 0.1 });
		p.setDefaultValue(16.0);
		data.add(std::move(p));
	}
}

template <int V>
bool bitcrush<V>::handleModulation(double&) noexcept
{
	return false;
}

template <int V>
void bitcrush<V>::reset() noexcept
{

}

template <int V>
void bitcrush<V>::prepare(PrepareSpecs ps)
{
	bitDepth.prepare(ps);
}

template <int V>
void bitcrush<V>::initialise(NodeBase*)
{

}



template <int V>
phase_delay<V>::phase_delay():
	polyphonic_base(getStaticId(), false)
{

}

template <int V>
void phase_delay<V>::initialise(NodeBase*)
{

}

template <int V>
void phase_delay<V>::prepare(PrepareSpecs ps)
{
	sr = ps.sampleRate * 0.5;
	delays[0].prepare(ps);
	delays[1].prepare(ps);
}

template <int V>
void phase_delay<V>::reset() noexcept
{
	for (auto& ds : delays)
	{
		for (auto& d : ds)
			d.reset();
	}
}



template <int V>
bool phase_delay<V>::handleModulation(double&) noexcept
{
	return false;
}

template <int V>
void phase_delay<V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(phase_delay, Frequency);
		p.setRange({ 20.0, 20000.0, 0.1 });
		p.setSkewForCentre(1000.0);
		p.setDefaultValue(400.0);
		data.add(std::move(p));
	}
}

template <int V>
void phase_delay<V>::setFrequency(double newFrequency)
{
	newFrequency /= sr;

	auto coefficient = AllpassDelay::getDelayCoefficient((float)newFrequency);

	for (auto& outer : delays)
	{
		for (auto& d : outer)
			d.setDelay(coefficient);
	}

}

template <int V>
void haas<V>::setPosition(double newValue)
{
	position = newValue;

	auto d = hmath::abs(position) * 0.02;

	for (auto& d_ : delay)
	{
		auto& l = d_[0];
		auto& r = d_[1];

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
}


template <int V>
bool haas<V>::handleModulation(double&)
{
	return false;
}


template <int V>
void haas<V>::processFrame(FrameType& data)
{
	data[0] = delay.get()[0].getDelayedValue(data[0]);
	data[1] = delay.get()[1].getDelayedValue(data[1]);
}


template <int V>
void haas<V>::process(haas<V>::ProcessType& d)
{
	delay.get()[0].processBlock(d.getRawDataPointers()[0], d.getNumSamples());
	delay.get()[1].processBlock(d.getRawDataPointers()[1], d.getNumSamples());
}


template <int V>
void haas<V>::reset()
{
	for (auto& d : delay)
	{
		for (auto& inner : d)
		{
			inner.setFadeTimeSamples(0);
			inner.clear();
		}
	}
}

template <int V>
void haas<V>::prepare(PrepareSpecs ps)
{
	delay.prepare(ps);

	auto sr = ps.sampleRate;

	for (auto& d : delay)
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
void haas<V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(haas, Position);
		p.setRange({ -1.0, 1.0, 0.1 });
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
}



}

}
