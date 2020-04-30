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

void tempo_sync::initialise(NodeBase* n)
{
	useFreqDomain.init(n, this);

	mc = n->getScriptProcessor()->getMainController_();
	mc->addTempoListener(this);
}

tempo_sync::tempo_sync():
	useFreqDomain(PropertyIds::UseFreqDomain, false)
{

}

tempo_sync::~tempo_sync()
{
	mc->removeTempoListener(this);
}

struct TempoDisplay: public ModulationSourceBaseComponent
{
	using ObjectType = tempo_sync;

	TempoDisplay(PooledUIUpdater* updater, tempo_sync* p_) :
		ModulationSourceBaseComponent(updater),
		p(p_)
	{
		setSize(256, 40);
	}

	static Component* createExtraComponent(tempo_sync*p, PooledUIUpdater* updater)
	{
		return new TempoDisplay(updater, p);
	}

	void timerCallback() override
	{
		if (p == nullptr)
			return;

		auto thisValue = p->currentTempoMilliseconds;

		if (thisValue != lastValue)
		{
			lastValue = thisValue;
			repaint();
		}
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_FONT());

		String n = String((int)lastValue) + " ms";

		g.drawText(n, getLocalBounds().toFloat(), Justification::centred);
	}

	double lastValue = 0.0;
	WeakReference<tempo_sync> p;
};





void tempo_sync::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(tempo_sync, Tempo);
		p.setParameterValueNames(TempoSyncer::getTempoNames());
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(tempo_sync, Multiplier);
		p.range = { 1, 16, 1.0 };
		p.defaultValue = 1.0;
		data.add(std::move(p));
	}
}

void tempo_sync::tempoChanged(double newTempo)
{
	bpm = newTempo;
	setTempo((double)(int)currentTempo);
}

void tempo_sync::setTempo(double newTempoIndex)
{
	currentTempo = (TempoSyncer::Tempo)jlimit<int>(0, TempoSyncer::numTempos-1, (int)newTempoIndex);
	currentTempoMilliseconds = TempoSyncer::getTempoInMilliSeconds(this->bpm, currentTempo) * multiplier;
}

bool tempo_sync::handleModulation(double& max)
{
	if (lastTempoMs != currentTempoMilliseconds)
	{
		lastTempoMs = currentTempoMilliseconds;
		max = currentTempoMilliseconds;

		if (useFreqDomain.getValue() && max > 0.0)
			max = 1.0 / (max * 0.001);

		return true;
	}

	return false;
}

void tempo_sync::setMultiplier(double newMultiplier)
{
	multiplier = jlimit(1.0, 32.0, newMultiplier);
	setTempo((double)(int)currentTempo);
}

template <int NV>
void ramp_impl<NV>::setPeriodTime(double periodTimeMs)
{
	periodTime = periodTimeMs;

	if (sr > 0.0)
	{
		auto s = periodTime * 0.001;
		auto inv = 1.0 / jmax(0.00001, s);

		auto newUptimeDelta = jmax(0.0000001, inv / sr);

		if (state.isMonophonicOrInsideVoiceRendering())
		{
			state.get().uptimeDelta = newUptimeDelta;
		}
		else
		{
			for (auto& s : state)
				s.uptimeDelta = newUptimeDelta;
		}
	}
}

template <int NV>
void ramp_impl<NV>::setLoopStart(double newLoopStart)
{
	auto v = jlimit(0.0, 1.0, newLoopStart);

	if (loopStart.isMonophonicOrInsideVoiceRendering())
		loopStart.get() = v;
	else
	{
		for (auto& d : loopStart)
			d = v;
	}
}


template <int NV>
void ramp_impl<NV>::initialise(NodeBase* b)
{
	useMidi.init(b, this);
}

template <int NV>
void ramp_impl<NV>::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(ramp_impl, PeriodTime);
		p.range = { 0.1, 1000.0, 0.1 };
		p.defaultValue = 100.0;
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(ramp_impl, LoopStart);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0;
		data.add(std::move(p));
	}

	useMidi.init(nullptr, nullptr);
}


template <int NV>
void ramp_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (useMidi.getValue() && e.isNoteOn())
		state.get().reset();
}

template <int NV>
bool ramp_impl<NV>::handleModulation(double& v)
{
	v = currentValue.get();
	return true;
}

template <int NV>
void ramp_impl<NV>::reset() noexcept
{
	if (state.isMonophonicOrInsideVoiceRendering())
	{
		state.get().reset();
		currentValue.get() = 0.0;
	}
	else
	{
		for (auto& s : state)
			s.reset();

		currentValue.setAll(0.0);
	}
}

template <int NV>
void ramp_impl<NV>::prepare(PrepareSpecs ps)
{
	currentValue.prepare(ps);
	state.prepare(ps);
	loopStart.prepare(ps);

	sr = ps.sampleRate;
	setPeriodTime(periodTime);
}

template <int NV>
ramp_impl<NV>::ramp_impl():
	useMidi(PropertyIds::UseMidi, true)
{
	setPeriodTime(100.0);
}

DEFINE_EXTERN_NODE_TEMPIMPL(ramp_impl);

struct OscDisplay : public ScriptnodeExtraComponent<OscillatorDisplayProvider>
{
	OscDisplay(OscillatorDisplayProvider* n, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent(n, updater)
	{
		p = f.createPath("sine");
		this->setSize(0, 50);
	};

	void paint(Graphics& g) override
	{
		auto h = this->getHeight();
		auto b = this->getLocalBounds().withSizeKeepingCentre(h * 2, h).toFloat();
		p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), true);
		GlobalHiseLookAndFeel::fillPathHiStyle(g, p, h * 2, h, false);
	}

	static Component* createExtraComponent(ObjectType* obj, PooledUIUpdater* updater)
	{
		return new OscDisplay(obj, updater);
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
			return;

		auto thisMode = (int)getObject()->currentMode;

		if (currentMode != thisMode)
		{
			currentMode = thisMode;

			auto pId = MarkdownLink::Helpers::getSanitizedFilename(this->getObject()->modes[currentMode]);
			p = f.createPath(pId);
			this->repaint();
		}
	}

	int currentMode = 0;
	WaveformComponent::WaveformFactory f;
	Path p;
};

template <int NV>
void scriptnode::core::oscillator_impl<NV>::reset() noexcept
{
	if (voiceData.isMonophonicOrInsideVoiceRendering())
		voiceData.get().reset();
	else
	{
		for (auto& s : voiceData)
			s.reset();
	}
}


template <int NV>
float scriptnode::core::oscillator_impl<NV>::tickNoise(OscData& d)
{
	return r.nextFloat() * 2.0f - 1.0f;
}

template <int NV>
float scriptnode::core::oscillator_impl<NV>::tickSaw(OscData& d)
{
	return 2.0f * std::fmodf(d.tick() / sinTable->getTableSize(), 1.0f) - 1.0f;
}

template <int NV>
float scriptnode::core::oscillator_impl<NV>::tickTriangle(OscData& d)
{
	return (1.0f - std::abs(tickSaw(d))) * 2.0f - 1.0f;
}

template <int NV>
float scriptnode::core::oscillator_impl<NV>::tickSine(OscData& d)
{
	return sinTable->getInterpolatedValue(d.tick());
}


template <int NV>
float scriptnode::core::oscillator_impl<NV>::tickSquare(OscData& d)
{
	return (float)(1 - (int)std::signbit(tickSaw(d))) * 2.0f - 1.0f;
}

template <int NV>
void oscillator_impl<NV>::initialise(NodeBase* n)
{
	useMidi.init(n, this);
}


template <int NV>
void oscillator_impl<NV>::prepare(PrepareSpecs ps)
{
	voiceData.prepare(ps);

	sr = ps.sampleRate;

	setFrequency(freqValue);
	useMidi.init(nullptr, nullptr);
}

template <int NV>
void oscillator_impl<NV>::setPitchMultiplier(double newMultiplier)
{
	auto pitchMultiplier = jlimit(0.01, 100.0, newMultiplier);

	if (voiceData.isMonophonicOrInsideVoiceRendering())
	{
		voiceData.get().multiplier = pitchMultiplier;
	}
	else
	{
		for(auto& d: voiceData)
			d.multiplier = pitchMultiplier;
	}
}

template <int NV>
void oscillator_impl<NV>::setFrequency(double newFrequency)
{
	freqValue = newFrequency;
	auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

	if (voiceData.isMonophonicOrInsideVoiceRendering())
	{
		voiceData.get().uptimeDelta = newUptimeDelta;
	}
	else
	{
		for(auto& d: voiceData)
			d.uptimeDelta = newUptimeDelta;
	}
}

template <int NV>
void oscillator_impl<NV>::setMode(double newMode)
{
	currentMode = (Mode)(int)newMode;
}

template <int NV>
oscillator_impl<NV>::oscillator_impl():
	useMidi(PropertyIds::UseMidi, false)
{
	
}

template <int V>
void oscillator_impl<V>::handleHiseEvent(HiseEvent& e)
{
    if (useMidi.getValue() && e.isNoteOn())
    {
        setFrequency(e.getFrequency());
    }
}

template <int NV>
void oscillator_impl<NV>::createParameters(Array<HiseDspBase::ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(oscillator_impl, Mode);
		p.setParameterValueNames(modes);
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(oscillator_impl, Frequency);
		p.range = { 20.0, 20000.0, 0.1 };
		p.defaultValue = 220.0;
		p.range.setSkewForCentre(1000.0);
		data.add(std::move(p));
	}
	{
		HiseDspBase::ParameterData p("Freq Ratio");
		p.range = { 1.0, 16.0, 1.0 };
		p.defaultValue = 1.0;
		p.dbNew = parameter::inner<oscillator_impl, (int)Parameters::PitchMultiplier>(*this);
		data.add(std::move(p));
	}

	useMidi.init(nullptr, nullptr);
}

template <int NV>
void oscillator_impl<NV>::processFrame(snex::Types::span<float, 1>& data)
{
	auto& d = voiceData.get();
	auto& s = data[0];

	switch (currentMode)
	{
	case Mode::Sine:	 s += tickSine(d); break;
	case Mode::Triangle: s += tickTriangle(d); break;
	case Mode::Saw:		 s += tickSaw(d); break;
	case Mode::Square:	 s += tickSquare(d); break;
	case Mode::Noise:	 s += Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
	}
}

template <int NV>
void oscillator_impl<NV>::process(snex::Types::ProcessDataFix<1>& data)
{
	auto f = data.toFrameData();

	while (f.next())
		processFrame(f.toSpan());
}



DEFINE_EXTERN_NODE_TEMPIMPL(oscillator_impl);


template <int V>
gain_impl<V>::gain_impl():
	resetValue(PropertyIds::ResetValue, 0.0f),
	useResetValue(PropertyIds::UseResetValue, false),
	gainBuffer(1, 0)
{

}


template <int V>
void gain_impl<V>::setSmoothing(double smoothingTimeMs)
{
	smoothingTime = smoothingTimeMs * 0.001;

	if (sr <= 0.0)
		return;

	auto sm = smoothingTime;

	if (gainer.isMonophonicOrInsideVoiceRendering())
	{
		gainer.get().reset(sr, sm);
	}
	else
	{
		auto sr_ = sr;

		for (auto& g : gainer)
			g.reset(sr_, sm);
	}
}

template <int V>
void gain_impl<V>::setGain(double newValue)
{
	gainValue = Decibels::decibelsToGain(newValue);

	if (gainer.isMonophonicOrInsideVoiceRendering())
		gainer.get().setValue((float)gainValue);
	else
	{
		float gf = (float)gainValue;

		for (auto& g : gainer)
			g.setValue(gf);
	}
}

template <int V>
void gain_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(gain_impl, Gain);
		p.range = { -100.0, 0.0, 0.1 };
		p.range.setSkewForCentre(-12.0);
		p.defaultValue = 0.0;
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(gain_impl, Smoothing);
		p.range = { 0.0, 1000.0, 0.1 };
		p.range.setSkewForCentre(100.0);
		p.defaultValue = 20.0;
		data.add(std::move(p));
	}

	resetValue.init(nullptr, nullptr);
	useResetValue.init(nullptr, nullptr);
}




template <int V>
void gain_impl<V>::initialise(NodeBase* n)
{
	resetValue.init(n, this);
	useResetValue.init(n, this);
}

template <int V>
void gain_impl<V>::reset() noexcept
{

	if (sr == 0.0)
		return;

	auto rv = resetValue.getValue();

	if (gainer.isMonophonicOrInsideVoiceRendering())
	{
		gainer.get().reset(sr, smoothingTime);

		if (useResetValue.getValue())
		{
			gainer.get().setValueWithoutSmoothing(rv);
			gainer.get().setValue((float)gainValue);
		}
		else
			gainer.get().setValueWithoutSmoothing((float)gainValue);
	}
	else
	{
		for (auto& g : gainer)
		{
			g.reset((float)sr, (float)smoothingTime);
			g.setValueWithoutSmoothing((float)gainValue);
		}
	}
}

template <int V>
void gain_impl<V>::prepare(PrepareSpecs ps)
{
	gainer.prepare(ps);
	sr = ps.sampleRate;

	DspHelpers::increaseBuffer(gainBuffer, ps);

	setSmoothing(smoothingTime * 1000.0);
}

DEFINE_EXTERN_NODE_TEMPIMPL(gain_impl);


template <int NV>
void smoother_impl<NV>::setDefaultValue(double newDefaultValue)
{
	defaultValue = (float)newDefaultValue;

	auto d = defaultValue; auto sm = smoothingTimeMs;

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().resetToValue((float)d, (float)sm);
	else
	{
		for (auto& s : smoother)
			s.resetToValue((float)d, (float)sm);
	}
}

template <int NV>
void smoother_impl<NV>::setSmoothingTime(double newSmoothingTime)
{
	smoothingTimeMs = newSmoothingTime;

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().setSmoothingTime((float)smoothingTimeMs);
	else
	{
		for(auto& s: smoother)
			s.setSmoothingTime((float)newSmoothingTime);
	}
}

template <int NV>
void smoother_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (useMidi.getValue() && e.isNoteOn())
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

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().resetToValue(defaultValue, 0.0);
	else
	{
		for (auto& s : smoother)
			s.resetToValue(d, 0.0);
	}
}

template <int NV>
void smoother_impl<NV>::initialise(NodeBase* n)
{
	useMidi.init(n, this);
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
void smoother_impl<NV>::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(smoother_impl, DefaultValue);
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0;
		data.add(std::move(p));
	}
	{
		DEFINE_PARAMETERDATA(smoother_impl, SmoothingTime);
		p.range = { 0.0, 2000.0, 0.1 };
		p.range.setSkewForCentre(100.0);
		p.defaultValue = 100.0;
		data.add(std::move(p));
	}

	useMidi.init(nullptr, nullptr);
}

DEFINE_EXTERN_NODE_TEMPIMPL(smoother_impl); 

/* ============================================================================ ramp_impl */

template <int V>
ramp_envelope_impl<V>::ramp_envelope_impl():
	useMidi(PropertyIds::UseMidi, true)
{
}

template <int V>
void ramp_envelope_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(ramp_envelope_impl, Gate);
		p.setParameterValueNames({ "On", "Off" });
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(ramp_envelope_impl, RampTime);
		p.range = { 0.0, 2000.0, 0.1 };
		data.add(std::move(p));
	}
}

template <int V>
void ramp_envelope_impl<V>::initialise(NodeBase* n)
{
	useMidi.init(n, this);
}

template <int V>
void ramp_envelope_impl<V>::prepare(PrepareSpecs ps)
{
	gainer.prepare(ps);
	sr = ps.sampleRate;
	setRampTime(attackTimeSeconds * 1000.0);
	useMidi.init(nullptr, nullptr);
}

template <int V>
void ramp_envelope_impl<V>::reset()
{
	if (gainer.isMonophonicOrInsideVoiceRendering())
		gainer.get().setValueWithoutSmoothing(0.0f);
	else
	{
		for(auto& g: gainer)
			g.setValueWithoutSmoothing(0.0f);
	}
}

template <int V>
bool ramp_envelope_impl<V>::handleModulation(double&)
{
	return false;
}

template <int V>
void ramp_envelope_impl<V>::handleHiseEvent(HiseEvent& e)
{
	if (useMidi.getValue() && e.isNoteOnOrOff())
		gainer.get().setTargetValue(e.isNoteOn() ? 1.0f : 0.0f);
}

template <int V>
void ramp_envelope_impl<V>::setGate(double newValue)
{
	gainer.getCurrentOrFirst().setTargetValue(newValue > 0.5 ? 1.0f : 0.0f);
}

template <int V>
void ramp_envelope_impl<V>::setRampTime(double newAttackTimeMs)
{
	attackTimeSeconds = newAttackTimeMs * 0.001;

	if (sr > 0.0)
	{
		if (gainer.isMonophonicOrInsideVoiceRendering())
		{
			gainer.get().reset(sr, attackTimeSeconds);
		}
		else
		{
			auto sr_ = sr;
			auto as = attackTimeSeconds;

			for (auto& g : gainer)
				g.reset(sr, attackTimeSeconds);
		}
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

hise_mod::hise_mod()
{
	
}

void hise_mod::initialise(NodeBase* b)
{
	parentProcessor = dynamic_cast<JavascriptSynthesiser*>(b->getScriptProcessor());
	parentNode = dynamic_cast<ModulationSourceNode*>(b);
}

void hise_mod::prepare(PrepareSpecs ps)
{
	modValues.prepare(ps);
	uptime.prepare(ps);

	for (auto& u : uptime)
		u = 0.0;

	if (parentProcessor != nullptr && ps.sampleRate > 0.0)
	{
		synthBlockSize = (double)parentProcessor->getLargestBlockSize();
		uptimeDelta = parentProcessor->getSampleRate() / ps.sampleRate;
	}
	
}

bool hise_mod::handleModulation(double& v)
{
	if (modValues.isMonophonicOrInsideVoiceRendering())
		return modValues.get().getChangedValue(v);
	else
		return false;
}


void hise_mod::handleHiseEvent(HiseEvent& e)
{
	if (e.isNoteOn())
	{
		uptime.get() = e.getTimeStamp() * uptimeDelta;
	}
}

void hise_mod::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(hise_mod, Index);
		p.setParameterValueNames({ "Pitch", "Extra 1", "Extra 2" });
		p.defaultValue = 0.0;
		data.add(std::move(p));
	}
}

void hise_mod::setIndex(double index)
{
	auto inp = roundToInt(index);

	if (inp == 0)
	{
		modIndex = ModulatorSynth::BasicChains::PitchChain;
	}

	else if (inp == 1)
		modIndex = JavascriptSynthesiser::ModChains::Extra1;

	else if (inp == 2)
		modIndex = JavascriptSynthesiser::ModChains::Extra2;
}

void hise_mod::reset()
{
	if (modValues.isMonophonicOrInsideVoiceRendering())
	{
		modValues.get().setModValue(parentProcessor->getModValueAtVoiceStart(modIndex));
	}
	else
	{
		for (auto& m : modValues)
			m.setModValue(1.0);
	}
}

void fm::prepare(PrepareSpecs ps)
{
	oscData.prepare(ps);
	modGain.prepare(ps);
	sr = ps.sampleRate;
}

void fm::reset()
{
	if (oscData.isMonophonicOrInsideVoiceRendering())
		oscData.get().reset();
	else
	{
		for (auto& o : oscData)
			o.reset();
	}
}

void fm::createParameters(Array<ParameterData>& data)
{
	{
		DEFINE_PARAMETERDATA(fm, Frequency);
		p.range = { 20.0, 20000.0, 0.1 };
		p.setDefaultValue(20.0);
		p.range.setSkewForCentre(1000.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(fm, Modulator);
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(fm, FreqMultiplier);
		p.range = { 1.0, 12.0, 1.0 };
		p.defaultValue = 1.0;
		data.add(std::move(p));
	}
}

void fm::handleHiseEvent(HiseEvent& e)
{
	setFrequency(e.getFrequency());
}

void fm::setFreqMultiplier(double input)
{
	if (oscData.isMonophonicOrInsideVoiceRendering())
		oscData.get().multiplier = input;
	else
	{
		for (auto& o : oscData)
			o.multiplier = input;
	}
}

void fm::setModulator(double newGain)
{
	auto newValue = newGain * sinTable->getTableSize() * 0.05;

	if (modGain.isMonophonicOrInsideVoiceRendering())
		modGain.get() = newValue;
	else
	{
		for (auto& m : modGain)
			m = newValue;
	}
}

void fm::setFrequency(double newFrequency)
{
	auto freqValue = newFrequency;
	auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

	if (oscData.isMonophonicOrInsideVoiceRendering())
		oscData.get().uptimeDelta = newUptimeDelta;
	else
	{
		for (auto& o : oscData)
			o.uptimeDelta = newUptimeDelta;
	}
}

} // namespace core

} // namespace scriptnode
