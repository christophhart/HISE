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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{


template <int NV> void oscillator<NV>::prepare(PrepareSpecs ps)
{
	voiceData.prepare(ps);
	sr = ps.sampleRate;
	setFrequency(freqValue);
	setPitchMultiplier(uiData.multiplier);
}




void fm::prepare(PrepareSpecs ps)
{
	oscData.prepare(ps);
	modGain.prepare(ps);
	sr = ps.sampleRate;
}

void fm::reset()
{
	for (auto& o : oscData)
		o.reset();
}

void fm::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(fm, Frequency);
		p.setRange({ 20.0, 20000.0, 0.1 });
		p.setDefaultValue(20.0);
		p.setSkewForCentre(1000.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(fm, Modulator);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(fm, FreqMultiplier);
		p.setRange({ 1.0, 12.0, 1.0 });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(fm, Gate);
		p.setRange({ 0.0, 1.0, 1.0 });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}
}

void fm::handleHiseEvent(HiseEvent& e)
{
	if(e.isNoteOn())
		setFrequency(e.getFrequency());
}

void fm::setFreqMultiplier(double input)
{
	for (auto& o : oscData)
		o.multiplier = input;
}

void fm::setModulator(double newGain)
{
	auto newValue = newGain * sinTable->getTableSize() * 0.05;

	for (auto& m : modGain)
		m = newValue;
}

void fm::setFrequency(double newFrequency)
{
	auto freqValue = newFrequency;
	auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

	for (auto& o : oscData)
		o.uptimeDelta = newUptimeDelta;
}



void fm::setGate(double v)
{
	auto shouldBeOn = (int)(v > 0.5);
	v = double(shouldBeOn);

	for (auto& d : oscData)
	{
		d.enabled = shouldBeOn;
		d.uptime *= v;
	}
}

} // namespace core



} // namespace scriptnode
