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
#if 0
#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{

void tempo_sync::initialise(NodeBase* n)
{
	if (auto modSource = dynamic_cast<ModulationSourceNode*>(n))
		modSource->setScaleModulationValue(false);

	useFreqDomain.init(n, this);

	mc = n->getScriptProcessor()->getMainController_();
	mc->addTempoListener(this);
}

tempo_sync::tempo_sync():
	no_mod_normalisation(getStaticId()),
	useFreqDomain(PropertyIds::UseFreqDomain, false)
{

}

tempo_sync::~tempo_sync()
{
	mc->removeTempoListener(this);
}

struct tempo_sync::TempoDisplay: public ModulationSourceBaseComponent
{
	TempoDisplay(PooledUIUpdater* updater, tempo_sync* p_) :
		ModulationSourceBaseComponent(updater),
		p(p_)
	{

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


Component* tempo_sync::createExtraComponent(PooledUIUpdater* updater)
{
	return new TempoDisplay(updater, this);
}

void tempo_sync::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Tempo");

		StringArray sa;

		for (int i = 0; i < TempoSyncer::numTempos; i++)
			sa.add(TempoSyncer::getTempoName(i));

		p.setParameterValueNames(sa);
		p.db = BIND_MEMBER_FUNCTION_1(tempo_sync::updateTempo);

		data.add(std::move(p));
	}
	{
		ParameterData d("Multiplier");
		d.range = { 1, 16, 1.0 };
		d.defaultValue = 1.0;
		d.db = BIND_MEMBER_FUNCTION_1(tempo_sync::setMultiplier);

		data.add(std::move(d));
	}
}

void tempo_sync::tempoChanged(double newTempo)
{
	bpm = newTempo;
	updateTempo((double)(int)currentTempo);
}

void tempo_sync::updateTempo(double newTempoIndex)
{
	currentTempo = (TempoSyncer::Tempo)(int)newTempoIndex;
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
			state.forEachVoice([newUptimeDelta](OscData& od)
			{
				od.uptimeDelta = newUptimeDelta;
			});
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
		loopStart.forEachVoice([v](double& d) { d = v; });
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
		ParameterData p("PeriodTime");

		p.range = { 0.1, 1000.0, 0.1 };
		p.defaultValue = 100.0;

		p.db = BIND_MEMBER_FUNCTION_1(ramp_impl::setPeriodTime);

		data.add(std::move(p));
	}

	{
		ParameterData p("Loop");

		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0;

		p.db = BIND_MEMBER_FUNCTION_1(ramp_impl::setLoopStart);

		data.add(std::move(p));
	}

	useMidi.init(nullptr, nullptr);
}

template <int NV>
Component* ramp_impl<NV>::createExtraComponent(PooledUIUpdater* updater)
{
	return new ModulationSourcePlotter(updater);
}


template <int NV>
void ramp_impl<NV>::handleHiseEvent(HiseEvent& e)
{
	if (useMidi.getValue() && e.isNoteOn())
		state.get().reset();
}

template <int NV>
void ramp_impl<NV>::processSingle(float* frameData, int numChannels)
{
	auto newValue = state.get().tick();
	
	if (newValue > 1.0)
	{
		newValue = loopStart.get();
		state.get().uptime = newValue;
	}
		
	for (int i = 0; i < numChannels; i++)
		frameData[i] += (float)newValue;

	currentValue.get() = newValue;
}

template <int NV>
bool ramp_impl<NV>::handleModulation(double& v)
{
	v = currentValue.get();
	return true;
}

template <int NV>
void ramp_impl<NV>::process(ProcessData& d)
{
	auto& thisState = state.get();

	double thisUptime = thisState.uptime;
	double thisDelta = thisState.uptimeDelta;

	for (int c = 0; c < d.numChannels; c++)
	{
		thisUptime = thisState.uptime;

		for (int i = 0; i < d.size; i++)
		{
			if (thisUptime > 1.0)
				thisUptime = loopStart.get();

			d.data[c][i] += (float)thisUptime;

			thisUptime += thisDelta;
		}

		
	}

	thisState.uptime = thisUptime;


	currentValue.get() = thisState.uptime;
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
		state.forEachVoice([](OscData& s)
		{
			s.reset();
		});

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

template <int NV> struct OscDisplay : public HiseDspBase::ExtraComponent<oscillator_impl<NV>>
{
	OscDisplay(oscillator_impl<NV>* n, PooledUIUpdater* updater) :
		HiseDspBase::ExtraComponent<oscillator_impl<NV>>(n, updater)
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

	void timerCallback() override
	{
		if (this->getObject() == nullptr)
			return;

		auto thisMode = (int)this->getObject()->currentMode;

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
Component* oscillator_impl<NV>::createExtraComponent(PooledUIUpdater* updater)
{
	return new OscDisplay<NV>(this, updater);
}


template <int NV>
void scriptnode::core::oscillator_impl<NV>::reset() noexcept
{
	if (voiceData.isMonophonicOrInsideVoiceRendering())
		voiceData.get().reset();
	else
		voiceData.forEachVoice([](OscData& s) { s.reset(); });
}


template <int NV>
void oscillator_impl<NV>::processSingle(float* data, int )
{
	auto& d = voiceData.get();

	switch (currentMode)
	{
	case Mode::Sine: *data += tickSine(d); break;
	case Mode::Triangle: *data += tickTriangle(d); break;
	case Mode::Saw: *data += tickSaw(d); break;
	case Mode::Square: *data += tickSquare(d); break;
        default: jassertfalse; break;
	}

	
}

template <int NV>
void oscillator_impl<NV>::process(ProcessData& data)
{
	int numSamples = data.size;

	auto& thisData = voiceData.get();
	auto* ptr = data.data[0];

	switch (currentMode)
	{
	case Mode::Sine:
	{
		while (--numSamples >= 0)
			*ptr++ += tickSine(thisData);

		break;
	}
	case Mode::Triangle:
	{
		while (--numSamples >= 0)
			*ptr++ += tickTriangle(thisData);

		break;
	}
	case Mode::Saw:
	{
		while (--numSamples >= 0)
			*ptr++ += tickSaw(thisData);

		break;
	}
	case Mode::Noise:
	{
		while (--numSamples >= 0)
			*ptr++ += tickNoise(thisData);

		break;
	}
	case Mode::Square:
	{
		while (--numSamples >= 0)
			*ptr++ += tickSquare(thisData);

		break;
	}
    default: jassertfalse; break;
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
	return 2.0f * std::fmod(d.tick() / (float)sinTable->getTableSize(), 1.0f) - 1.0f;
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
		voiceData.forEachVoice([pitchMultiplier](OscData& d)
		{
			d.multiplier = pitchMultiplier;
		});
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
		voiceData.forEachVoice([newUptimeDelta](OscData& d)
		{
			d.uptimeDelta = newUptimeDelta;
		});
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
	modes = { "Sine", "Saw", "Triangle", "Square", "Noise"};
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
		HiseDspBase::ParameterData p("Mode");
		p.setParameterValueNames(modes);
		p.db = std::bind(&oscillator_impl::setMode, this, std::placeholders::_1);
		data.add(std::move(p));
	}
	{
		HiseDspBase::ParameterData p("Frequency");
		p.range = { 20.0, 20000.0, 0.1 };
		p.defaultValue = 220.0;
		p.range.setSkewForCentre(1000.0);
		p.db = std::bind(&oscillator_impl::setFrequency, this, std::placeholders::_1);
		data.add(std::move(p));
	}
	{
		HiseDspBase::ParameterData p("Freq Ratio");
		p.range = { 1.0, 16.0, 1.0 };
		p.defaultValue = 1.0;
		p.db = std::bind(&oscillator_impl::setPitchMultiplier, this, std::placeholders::_1);
		data.add(std::move(p));
	}

	useMidi.init(nullptr, nullptr);
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
void gain_impl<V>::setSmoothingTime(double smoothingTimeMs)
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
		gainer.forEachVoice([sm, sr_](LinearSmoothedValue<float>& f)
		{
			f.reset(sr_, sm);
		});
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
		gainer.forEachVoice([gf](LinearSmoothedValue<float>& g)
		{
			g.setValue(gf);
		});
	}
}

template <int V>
void gain_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Gain");

		p.range = { -100.0, 0.0, 0.1 };
		p.range.setSkewForCentre(-12.0);
		p.defaultValue = 0.0;
		p.db = std::bind(&gain_impl::setGain, this, std::placeholders::_1);

		data.add(std::move(p));
	}
	{
		ParameterData p("Smoothing");
		p.range = { 0.0, 1000.0, 0.1 };
		p.range.setSkewForCentre(100.0);
		p.defaultValue = 20.0;
		p.db = std::bind(&gain_impl::setSmoothingTime, this, std::placeholders::_1);

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
void gain_impl<V>::processSingle(float* numFrames, int numChannels)
{
	auto nextValue = gainer.get().getNextValue();
	FloatVectorOperations::multiply(numFrames, nextValue, numChannels);
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
		auto s = sr;
		auto t = smoothingTime;
		auto gf = gainValue;

		gainer.forEachVoice([s, t, gf](LinearSmoothedValue<float>& g)
		{
			g.reset((float)s, (float)t);
			g.setValueWithoutSmoothing((float)gf);
		});
	}
}

template <int V>
void gain_impl<V>::process(ProcessData& d)
{
	auto& thisGainer = gainer.get();

	if (thisGainer.isSmoothing())
	{
		jassert(d.size <= gainBuffer.getNumSamples());
		auto gainData = gainBuffer.getWritePointer(0);

		for (int i = 0; i < d.size; i++)
			gainData[i] = thisGainer.getNextValue();

		for (auto ch : d)
			FloatVectorOperations::multiply(ch, gainData, d.size);
	}
	else
	{
		auto gainFactor = thisGainer.getCurrentValue();

		for (auto ch : d)
			FloatVectorOperations::multiply(ch, gainFactor, d.size);
	}
}

template <int V>
void gain_impl<V>::prepare(PrepareSpecs ps)
{
	gainer.prepare(ps);
	sr = ps.sampleRate;

	DspHelpers::increaseBuffer(gainBuffer, ps);

	setSmoothingTime(smoothingTime * 1000.0);
}

DEFINE_EXTERN_NODE_TEMPIMPL(gain_impl);


template <int NV>
Component* smoother_impl<NV>::createExtraComponent(PooledUIUpdater* updater)
{
	return new ModulationSourceBaseComponent(updater);
}

template <int NV>
void smoother_impl<NV>::setDefaultValue(double newDefaultValue)
{
	defaultValue = (float)newDefaultValue;

	auto d = defaultValue; auto sm = smoothingTimeMs;

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().resetToValue((float)d, (float)sm);
	else
		smoother.forEachVoice([d, sm](Smoother& s) {s.resetToValue((float)d, (float)sm); });
}

template <int NV>
void smoother_impl<NV>::setSmoothingTime(double newSmoothingTime)
{
	smoothingTimeMs = newSmoothingTime;

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().setSmoothingTime((float)smoothingTimeMs);
	else
		smoother.forEachVoice([newSmoothingTime](Smoother& s) {s.setSmoothingTime((float)newSmoothingTime); });
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
void smoother_impl<NV>::process(ProcessData& d)
{
	smoother.get().smoothBuffer(d.data[0], d.size);
	modValue.get().setModValue(smoother.get().getDefaultValue());
}

template <int NV>
void smoother_impl<NV>::processSingle(float* data, int)
{
	*data = smoother.get().smooth(*data);
	modValue.get().setModValue(smoother.get().getDefaultValue());
}

template <int NV>
void smoother_impl<NV>::reset()
{
	auto d = defaultValue;

	if (smoother.isMonophonicOrInsideVoiceRendering())
		smoother.get().resetToValue(defaultValue, 0.0);
	else
		smoother.forEachVoice([d](Smoother& s) { s.resetToValue(d, 0.0); });
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
	smoother.forEachVoice([sr, sm](hise::Smoother& s)
	{
		s.prepareToPlay(sr);
		s.setSmoothingTime((float)sm);
	});
}

template <int NV>
void smoother_impl<NV>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("DefaultValue");
		p.range = { 0.0, 1.0, 0.01 };
		p.defaultValue = 0.0;
		p.db = BIND_MEMBER_FUNCTION_1(smoother_impl::setDefaultValue);
		data.add(std::move(p));
	}
	{
		ParameterData p("SmoothingTime");
		p.range = { 0.0, 2000.0, 0.1 };
		p.range.setSkewForCentre(100.0);
		p.defaultValue = 100.0;
		p.db = BIND_MEMBER_FUNCTION_1(smoother_impl::setSmoothingTime);
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
		ParameterData p("Gate");
		p.setParameterValueNames({ "On", "Off" });
		p.db = BIND_MEMBER_FUNCTION_1(ramp_envelope_impl::setGate);

		data.add(std::move(p));
	}

	{
		ParameterData p("Ramp Time");
		p.range = { 0.0, 2000.0, 0.1 };
		p.db = BIND_MEMBER_FUNCTION_1(ramp_envelope_impl::setRampTime);

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
		gainer.forEachVoice([](LinearSmoothedValue<float>& v)
		{
			v.setValueWithoutSmoothing(0.0f);
		});
	}
}

template <int V>
void ramp_envelope_impl<V>::processSingle(float* , int )
{
	jassertfalse;
}

template <int V>
void ramp_envelope_impl<V>::process(ProcessData& d)
{
	auto& thisG = gainer.get();

	if (thisG.isSmoothing())
	{
		for (int i = 0; i < d.size; i++)
		{
			auto v = thisG.getNextValue();

			for (int c = 0; c < d.numChannels; c++)
				d.data[c][i] *= v;
		}

		d.shouldReset = false;
	}
	else
	{
		for (int c = 0; c < d.numChannels; c++)
		{
			FloatVectorOperations::multiply(d.data[c], thisG.getTargetValue(), d.size);
		}

		d.shouldReset = thisG.getCurrentValue() == 0.0;
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
			gainer.forEachVoice([sr_, as](LinearSmoothedValue<float>& v)
			{
				v.reset(sr_, as);
			});
		}
	}
}

DEFINE_EXTERN_NODE_TEMPIMPL(ramp_envelope_impl);

bool peak::handleModulation(double& value)
{
	value = max;
	return true;
}

juce::Component* peak::createExtraComponent(PooledUIUpdater* updater)
{

	return new ModulationSourcePlotter(updater);
}

void peak::reset() noexcept
{
	max = 0.0;
}

void peak::process(ProcessData& data)
{
	max = DspHelpers::findPeak(data);
}

void peak::processSingle(float* frameData, int numChannels)
{
    if(numChannels == 1)
        max = frameData[0];
    else if (numChannels == 2)
    {
        max = jmax(frameData[0], frameData[1]);
    }
    else
    {
        max = 0.0;
        
        for (int i = 0; i < numChannels; i++)
            max = jmax(max, std::abs((double)frameData[i]));
    }
	
}



void mono2stereo::process(ProcessData& data)
{
	if(data.numChannels >= 2)
		FloatVectorOperations::copy(data.data[1], data.data[0], data.size);
}

void mono2stereo::processSingle(float* frameData, int numChannels)
{
	if (numChannels >= 2)
		frameData[1] = frameData[0];
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

	uptime.forEachVoice([](double& d) { d = 0.0; });

	if (parentProcessor != nullptr && ps.sampleRate > 0.0)
	{
		synthBlockSize = (double)parentProcessor->getLargestBlockSize();
		uptimeDelta = parentProcessor->getSampleRate() / ps.sampleRate;
	}
	
}

void hise_mod::process(ProcessData& d)
{
	if (parentProcessor != nullptr)
	{
		auto numToDo = (double)d.size;
		auto& u = uptime.get();

		
		modValues.get().setModValueIfChanged(parentProcessor->getModValueForNode(modIndex, roundToInt(u)));
		u = fmod(u + numToDo * uptimeDelta, synthBlockSize);
	}
}

bool hise_mod::handleModulation(double& v)
{
	if (modValues.isMonophonicOrInsideVoiceRendering())
		return modValues.get().getChangedValue(v);
	else
		return false;
}

void hise_mod::processSingle(float* , int )
{
	jassertfalse;
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
		ParameterData p("Index");
		p.setParameterValueNames({ "Pitch", "Extra 1", "Extra 2" });
		p.db = BIND_MEMBER_FUNCTION_1(hise_mod::setIndex);
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

	if (parentNode != nullptr)
		parentNode->setScaleModulationValue(inp != 0);
}

void hise_mod::reset()
{
	if (modValues.isMonophonicOrInsideVoiceRendering())
	{
		modValues.get().setModValue(parentProcessor->getModValueAtVoiceStart(modIndex));
	}
	else
	{
		modValues.forEachVoice([](ModValue& v)
		{
			v.setModValue(1.0);
		});
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
		oscData.forEachVoice([](OscData& d) {d.reset(); });
}

void fm::process(ProcessData& d)
{
	auto& od = oscData.get();

	for (int i = 0; i < d.size; i++)
	{
		double modValue = (double)d.data[0][i];
		d.data[0][i] = sinTable->getInterpolatedValue(od.tick());
		od.uptime += modGain.get() * modValue;
	}
}

void fm::processSingle(float* frameData, int )
{
	auto& od = oscData.get();
	double modValue = (double)*frameData;
	*frameData = sinTable->getInterpolatedValue(od.tick());
	od.uptime += modGain.get() * modValue;
}

void fm::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Frequency");
		p.range = { 20.0, 20000.0, 0.1 };
		p.setDefaultValue(20.0);
		p.range.setSkewForCentre(1000.0);
		p.db = BIND_MEMBER_FUNCTION_1(fm::setFrequency);

		data.add(std::move(p));
	}

	{
		ParameterData p("Modulator");
		p.setDefaultValue(0.0);
		p.db = BIND_MEMBER_FUNCTION_1(fm::setModulatorGain);
		data.add(std::move(p));
	}

	{
		ParameterData p("FreqMultiplier");
		p.range = { 1.0, 12.0, 1.0 };
		p.defaultValue = 1.0;
		p.db = BIND_MEMBER_FUNCTION_1(fm::setFreqMultiplier);
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
		oscData.forEachVoice([input](OscData& d) { d.multiplier = input; });
}

void fm::setModulatorGain(double newGain)
{
	auto newValue = newGain * sinTable->getTableSize() * 0.05;

	if (modGain.isMonophonicOrInsideVoiceRendering())
		modGain.get() = newValue;
	else
		modGain.forEachVoice([newValue](double& d) {d = newValue; });
}

void fm::setFrequency(double newFrequency)
{
	auto freqValue = newFrequency;
	auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

	if (oscData.isMonophonicOrInsideVoiceRendering())
		oscData.get().uptimeDelta = newUptimeDelta;
	else
		oscData.forEachVoice([newUptimeDelta](OscData& d) {d.uptimeDelta = newUptimeDelta; });
}

} // namespace core

} // namespace scriptnode
#endif