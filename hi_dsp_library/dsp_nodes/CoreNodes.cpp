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

template <int NV>
void scriptnode::core::oscillator_impl<NV>::prepare(PrepareSpecs ps)
{
	voiceData.prepare(ps);
	sr = ps.sampleRate;
	setFrequency(freqValue);
}

template <int NV>
void scriptnode::core::oscillator_impl<NV>::reset() noexcept
{
	for (auto& s : voiceData)
		s.reset();
}


template <int NV>
void scriptnode::core::oscillator_impl<NV>::setMode(double newMode)
{
	currentMode = (Mode)(int)newMode;
}


template <int NV>
void scriptnode::core::oscillator_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (useMidi && e.isNoteOn())
	{
		setFrequency(e.getFrequency());
	}
}

template <int NV>
void scriptnode::core::oscillator_impl<NV>::setFrequency(double newFrequency)
{
	freqValue = newFrequency;

	if (sr > 0.0)
	{
		auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

		for (auto& d : voiceData)
			d.uptimeDelta = newUptimeDelta;
	}
}

template <int NV>
void scriptnode::core::oscillator_impl<NV>::setPitchMultiplier(double newMultiplier)
{
	auto pitchMultiplier = jlimit(0.01, 100.0, newMultiplier);

	for (auto& d : voiceData)
		d.multiplier = pitchMultiplier;
}

template <int NV>
scriptnode::core::oscillator_impl<NV>::oscillator_impl()
{

}


template <int NV>
void scriptnode::core::oscillator_impl<NV>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(oscillator_impl, Mode);
		p.setParameterValueNames(modes);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(oscillator_impl, Frequency);
		p.setRange({ 20.0, 20000.0, 0.1 });
		p.setDefaultValue(220.0);
		p.setSkewForCentre(1000.0);
		data.add(std::move(p));
	}
	{
		parameter::data p("Freq Ratio");
		p.setRange({ 1.0, 16.0, 1.0 });
		p.setDefaultValue(1.0);
		p.callback = parameter::inner<oscillator_impl, (int)Parameters::PitchMultiplier>(*this);
		data.add(std::move(p));
	}
}



template <int V>
void gain_impl<V>::setSmoothing(double smoothingTimeMs)
{
	smoothingTime = smoothingTimeMs;

	if (sr <= 0.0)
		return;

	for (auto& g : gainer)
		g.prepare(sr, smoothingTime);
}

template <int V>
void gain_impl<V>::setGain(double newValue)
{
	gainValue = Decibels::decibelsToGain(newValue);

	float gf = (float)gainValue;

	for (auto& g : gainer)
		g.set(gf);
}


template <int V>
void scriptnode::core::gain_impl<V>::setResetValue(double newResetValue)
{
	resetValue = newResetValue;
}


template <int V>
void gain_impl<V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(gain_impl, Gain);
		p.setRange({ -100.0, 0.0, 0.1 });
		p.setSkewForCentre(-12.0);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(gain_impl, Smoothing);
		p.setRange({ 0.0, 1000.0, 0.1 });
		p.setSkewForCentre(100.0);
		p.setDefaultValue(20.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(gain_impl, ResetValue);
		p.setRange({ -100.0, 0.0, 0.1 });
		p.setSkewForCentre(-12.0);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
}

template <int V>
void gain_impl<V>::reset() noexcept
{
	if (sr == 0.0)
		return;

	for (auto& g : gainer)
	{
		g.set(resetValue);
		g.reset();
		g.set((float)gainValue);
	}
}


template <int V>
void scriptnode::core::gain_impl<V>::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOn())
		reset();
}

template <int V>
void gain_impl<V>::prepare(PrepareSpecs ps)
{
	gainer.prepare(ps);
	sr = ps.sampleRate;

	setSmoothing(smoothingTime);
}

DEFINE_EXTERN_NODE_TEMPIMPL(gain_impl);


template <int NV>
void smoother_impl<NV>::setDefaultValue(double newDefaultValue)
{
	defaultValue = (float)newDefaultValue;

	auto d = defaultValue; auto sm = smoothingTimeMs;

	for (auto& s : smoother)
		s.resetToValue((float)d, (float)sm);
}

template <int NV>
void smoother_impl<NV>::setSmoothingTime(double newSmoothingTime)
{
	smoothingTimeMs = newSmoothingTime;

	for (auto& s : smoother)
		s.setSmoothingTime((float)newSmoothingTime);
}

template <int NV>
void smoother_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOn())
		reset();
}

template <int NV>
bool smoother_impl<NV>::handleModulation(double& value)
{
	return modValue.get().getChangedValue(value);
}

template <int NV>
void smoother_impl<NV>::reset()
{
	auto d = defaultValue;

	for (auto& s : smoother)
		s.resetToValue(d, 0.0);
}

template <int NV>
void scriptnode::core::smoother_impl<NV>::prepare(PrepareSpecs ps)
{
	modValue.prepare(ps);
	smoother.prepare(ps);
	auto sr = ps.sampleRate;
	auto sm = smoothingTimeMs;

	for (auto& s : smoother)
	{
		s.prepareToPlay(sr);
		s.setSmoothingTime((float)sm);
	}
}

template <int NV>
void smoother_impl<NV>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(smoother_impl, DefaultValue);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(smoother_impl, SmoothingTime);
		p.setRange({ 0.0, 2000.0, 0.1 });
		p.setSkewForCentre(100.0);
		p.setDefaultValue(100.0);
		data.add(std::move(p));
	}
}

DEFINE_EXTERN_NODE_TEMPIMPL(smoother_impl); 

/* ============================================================================ ramp_impl */

template <int V>
ramp_envelope_impl<V>::ramp_envelope_impl()
{
}

template <int V>
void ramp_envelope_impl<V>::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(ramp_envelope_impl, Gate);
		p.setParameterValueNames({ "On", "Off" });
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(ramp_envelope_impl, RampTime);
		p.setRange({ 0.0, 2000.0, 0.1 });
		data.add(std::move(p));
	}
}

template <int V>
void ramp_envelope_impl<V>::prepare(PrepareSpecs ps)
{
	gainer.prepare(ps);
	sr = ps.sampleRate;
	setRampTime(attackTimeSeconds * 1000.0);
}

template <int V>
void ramp_envelope_impl<V>::reset()
{
	for (auto& g : gainer)
		g.setValueWithoutSmoothing(0.0f);
}

template <int V>
bool ramp_envelope_impl<V>::handleModulation(double&)
{
	return false;
}

template <int V>
void ramp_envelope_impl<V>::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOnOrOff())
		gainer.get().setTargetValue(e.isNoteOn() ? 1.0f : 0.0f);
}

template <int V>
void ramp_envelope_impl<V>::setGate(double newValue)
{
	for(auto& g: gainer)
		g.setTargetValue(newValue > 0.5 ? 1.0f : 0.0f);
}

template <int V>
void ramp_envelope_impl<V>::setRampTime(double newAttackTimeMs)
{
	attackTimeSeconds = newAttackTimeMs * 0.001;

	if (sr > 0.0)
	{
		for (auto& g : gainer)
			g.reset(sr, attackTimeSeconds);
	}
}

DEFINE_EXTERN_NODE_TEMPIMPL(ramp_envelope_impl);

bool peak::handleModulation(double& value)
{
	value = max;
	return true;
}

void peak::reset() noexcept
{
	max = 0.0;
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
}

void fm::handleHiseEvent(HiseEvent& e)
{
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

} // namespace core


template class wrap::fix<1, core::oscillator_impl<1>>;
template class wrap::fix<1, core::oscillator_impl<NUM_POLYPHONIC_VOICES>>;

} // namespace scriptnode
