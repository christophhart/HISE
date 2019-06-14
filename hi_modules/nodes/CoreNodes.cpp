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
	if (auto modSource = dynamic_cast<ModulationSourceNode*>(n))
		modSource->setScaleModulationValue(false);

	mc = n->getScriptProcessor()->getMainController_();
	mc->addTempoListener(this);
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
	tempo_sync* p;
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

		if (state.isVoiceRenderingActive())
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

	if (loopStart.isVoiceRenderingActive())
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
		frameData[i] += newValue;

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

	for (int c = 0; c < d.numChannels; c++)
	{
		double thisUptime = thisState.uptime;
		double thisDelta = thisState.uptimeDelta;

		for (int i = 0; i < d.size; i++)
		{
			if (thisUptime > 1.0)
				thisUptime = loopStart.get();

			d.data[c][i] += (float)thisUptime;

			thisUptime += thisDelta;
		}

		thisState.uptime = thisUptime;
	}

	currentValue.get() = thisState.uptime;
}

template <int NV>
void ramp_impl<NV>::reset() noexcept
{
	if (state.isVoiceRenderingActive())
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
		auto thisMode = this->getObject()->currentMode;

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
void oscillator_impl<NV>::processSingle(float* data, int numChannels)
{
	auto newValue = sinTable->getInterpolatedValue(voiceData.get().tick());

	for (int i = 0; i < numChannels; i++)
		data[i] += newValue;
}

template <int NV>
void oscillator_impl<NV>::process(ProcessData& data)
{
	auto stackData = (float*)alloca(data.size * sizeof(float));
	auto signal = stackData;

	int numSamples = data.size;

	auto& thisData = voiceData.get();

	while (--numSamples >= 0)
	{
		*stackData++ = sinTable->getInterpolatedValue(thisData.tick());

		//*stackData++ = (float)std::sin(thisData.tick());
	}

	for (auto c : data)
	{
		FloatVectorOperations::add(c, signal, data.size);
	}
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

	if (voiceData.isVoiceRenderingActive())
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

	if (voiceData.isVoiceRenderingActive())
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
	currentMode = (int)newMode;
}

template <int NV>
oscillator_impl<NV>::oscillator_impl():
	useMidi(PropertyIds::UseMidi, false)
{
	modes = { "Sine", "Saw", "Noise", "Square" };
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
	useResetValue(PropertyIds::UseResetValue, false)
{

}


template <int V>
void gain_impl<V>::setSmoothingTime(double smoothingTimeMs)
{
	smoothingTime = smoothingTimeMs * 0.001;

	if (sr <= 0.0)
		return;

	auto sm = smoothingTime;

	if (gainer.isVoiceRenderingActive())
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

	if (gainer.isVoiceRenderingActive())
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

	if (gainer.isVoiceRenderingActive())
	{
		gainer.get().reset(sr, smoothingTime);

		if (useResetValue.getValue())
		{
			gainer.get().setValueWithoutSmoothing(rv);
			gainer.get().setValue(gainValue);
		}
		else
			gainer.get().setValueWithoutSmoothing(gainValue);
	}
	else
	{
		auto s = sr;
		auto t = smoothingTime;
		auto gf = gainValue;

		gainer.forEachVoice([s, t, gf](LinearSmoothedValue<float>& g)
		{
			g.reset(s, t);
			g.setValueWithoutSmoothing(gf);
		});
	}
}

template <int V>
void gain_impl<V>::process(ProcessData& d)
{
	auto& thisGainer = gainer.get();

	if (thisGainer.isSmoothing())
	{
		auto gainData = ALLOCA_FLOAT_ARRAY(d.size);

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

	if (smoother.isVoiceRenderingActive())
		smoother.get().resetToValue(d, sm);
	else
		smoother.forEachVoice([d, sm](Smoother& s) {s.resetToValue(d, sm); });
}

template <int NV>
void smoother_impl<NV>::setSmoothingTime(double newSmoothingTime)
{
	smoothingTimeMs = newSmoothingTime;

	if (smoother.isVoiceRenderingActive())
		smoother.get().setSmoothingTime(smoothingTimeMs);
	else
		smoother.forEachVoice([newSmoothingTime](Smoother& s) {s.setSmoothingTime(newSmoothingTime); });
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
void smoother_impl<NV>::processSingle(float* data, int numChannels)
{
	*data = smoother.get().smooth(*data);
	modValue.get().setModValue(smoother.get().getDefaultValue());
}

template <int NV>
void smoother_impl<NV>::reset()
{
	auto d = defaultValue;

	if (smoother.isVoiceRenderingActive())
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
		s.setSmoothingTime(sm);
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
	if (gainer.isVoiceRenderingActive())
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
void ramp_envelope_impl<V>::processSingle(float* data, int numChannels)
{

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
		if (gainer.isVoiceRenderingActive())
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
	max = 0.0;

	for (int i = 0; i < numChannels; i++)
		max = jmax(max, std::abs((double)frameData[i]));
}



} // namespace core

} // namespace scriptnode
