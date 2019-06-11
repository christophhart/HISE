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


namespace container
{

template <typename... Ts> using frame1_block = wrap::frame<1, container::chain<Ts...>>;
template <typename... Ts> using frame2_block = wrap::frame<2, container::chain<Ts...>>;
template <typename... Ts> using frame3_block = wrap::frame<3, container::chain<Ts...>>;
template <typename... Ts> using frame4_block = wrap::frame<4, container::chain<Ts...>>;
template <typename... Ts> using frame6_block = wrap::frame<6, container::chain<Ts...>>;
template <typename... Ts> using frame8_block = wrap::frame<8, container::chain<Ts...>>;
template <typename... Ts> using frame16_block = wrap::frame<16, container::chain<Ts...>>;

template <typename... Ts> using oversample2x = wrap::oversample<2, container::chain<Ts...>>;
template <typename... Ts> using oversample4x = wrap::oversample<4, container::chain<Ts...>>;
template <typename... Ts> using oversample8x = wrap::oversample<8, container::chain<Ts...>>;
template <typename... Ts> using oversample16x = wrap::oversample<16, container::chain<Ts...>>;

template <typename... Processors> using modchain = wrap::mod<chain<Processors...>>;

}

namespace core
{


class tempo_sync : public HiseDspBase,
				   public TempoListener
{
public:

	SET_HISE_NODE_ID("tempo_sync");
	GET_SELF_AS_OBJECT(tempo_sync);
	SET_HISE_NODE_EXTRA_HEIGHT(24);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE

	~tempo_sync();

	struct TempoDisplay;

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void initialise(NodeBase* n) override;
	void createParameters(Array<ParameterData>& data) override;
	void tempoChanged(double newTempo) override;
	void updateTempo(double newTempoIndex);
	bool handleModulation(double& max);

	void setMultiplier(double newMultiplier)
	{
		multiplier = jlimit(1.0, 32.0, newMultiplier);
		updateTempo((double)(int)currentTempo);
	}

	double currentTempoMilliseconds = 500.0;
	double lastTempoMs = 0.0;
	double bpm = 120.0;

	double multiplier = 1.0;
	TempoSyncer::Tempo currentTempo = TempoSyncer::Tempo::Eighth;

	MainController* mc = nullptr;
};

class peak : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("peak");
	GET_SELF_AS_OBJECT(peak);
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;

	bool handleModulation(double& value);
	Component* createExtraComponent(PooledUIUpdater* updater);;
	void reset() noexcept;;
	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);

	// This is no state variable, so we don't need it to be polyphonic...
	double max = 0.0;
};

class empty : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("empty");
	GET_SELF_AS_OBJECT(empty);
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_RESET;
	HISE_EMPTY_MOD;
};

template <int NV> class ramp_impl : public HiseDspBase
{
public:
	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("ramp");
	GET_SELF_AS_OBJECT(ramp_impl);
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	ramp_impl();

	void initialise(NodeBase* b);
	void prepare(PrepareSpecs ps);
	void reset() noexcept;;
	void process(ProcessData& d);
	bool handleModulation(double& v);;
	void processSingle(float* frameData, int numChannels);
	Component* createExtraComponent(PooledUIUpdater* updater);;
	void createParameters(Array<ParameterData>& data) override;
	void handleHiseEvent(HiseEvent& e) final override;

	void setPeriodTime(double periodTimeMs);
	void setLoopStart(double loopStart);

private:

	double sr = 44100.0;
	double periodTime = 500.0;
	PolyData<OscData, NumVoices> state;
	PolyData<double, NumVoices> currentValue;
	PolyData<double, NumVoices> loopStart;

	NodePropertyT<bool> useMidi;
};


DEFINE_EXTERN_NODE_TEMPLATE(ramp, ramp_poly, ramp_impl);


template <int NV> class oscillator_impl: public HiseDspBase
{
public:

	constexpr static int NumVoices = NV;

	SET_HISE_NODE_EXTRA_HEIGHT(50);
	SET_HISE_POLY_NODE_ID("oscillator");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(oscillator_impl);

	oscillator_impl();

	void createParameters(Array<HiseDspBase::ParameterData>& data);
	
	void initialise(NodeBase* n) override;
	void reset() noexcept { voiceData.get().reset(); };
	void prepare(PrepareSpecs ps);
	bool handleModulation(double&) noexcept { return false; };
	void process(ProcessData& data);
	void processSingle(float* data, int numChannels);
	void handleHiseEvent(HiseEvent& e) override;

	Component* createExtraComponent(PooledUIUpdater* updater);

	void setMode(double newMode);
	void setFrequency(double newFrequency);
	void setPitchMultiplier(double newMultiplier);

	StringArray modes;
	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;

	SharedResourcePointer<SineLookupTable<2048>> sinTable;

	NodePropertyT<bool> useMidi;

	int currentMode = 0;
	double freqValue = 220.0;
};

DEFINE_EXTERN_NODE_TEMPLATE(oscillator, oscillator_poly, oscillator_impl);

template <int V> class gain_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("gain");
	GET_SELF_AS_OBJECT(gain_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	gain_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);
	void process(ProcessData& d);
	void reset() noexcept;;
	void processSingle(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept { return false; };
	void createParameters(Array<ParameterData>& data) override;

	void setGain(double newValue);
	void setSmoothingTime(double smoothingTimeMs);

	double gainValue = 1.0;
	double sr = 0.0;
	double smoothingTime = 0.02;

	NodePropertyT<float> resetValue;
	NodePropertyT<float> useResetValue;
	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
};

DEFINE_EXTERN_NODE_TEMPLATE(gain, gain_poly, gain_impl);

template <int NV> class smoother_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = NV;

	SET_HISE_NODE_EXTRA_HEIGHT(25);
	SET_HISE_POLY_NODE_ID("smoother");
	GET_SELF_AS_OBJECT(smoother_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	smoother_impl();

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void initialise(NodeBase* n) override;
	void createParameters(Array<ParameterData>& data) override;
	void prepare(PrepareSpecs ps);
	void reset();
	void processSingle(float* data, int numChannels);
	void process(ProcessData& d);
	bool handleModulation(double&);
	void handleHiseEvent(HiseEvent& e) final override;
	
	void setSmoothingTime(double newSmoothingTime);
	void setDefaultValue(double newDefaultValue);

	NodePropertyT<bool> useMidi;
	double smoothingTimeMs = 100.0;
	float defaultValue = 0.0f;
	PolyData<hise::Smoother, NumVoices> smoother;
	PolyData<ModValue, NumVoices> modValue;
};

template <int NV>
scriptnode::core::smoother_impl<NV>::smoother_impl():
	useMidi(PropertyIds::UseMidi, true)
{

}

DEFINE_EXTERN_NODE_TEMPLATE(smoother, smoother_poly, smoother_impl);


template <int V> class ramp_envelope_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("ramp_envelope");
	GET_SELF_AS_OBJECT(ramp_envelope_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	ramp_envelope_impl();

	void initialise(NodeBase* n) override;
	void createParameters(Array<ParameterData>& data) override;
	void prepare(PrepareSpecs ps);
	void reset();
	void processSingle(float* data, int numChannels);
	void process(ProcessData& d);
	bool handleModulation(double&);
	void handleHiseEvent(HiseEvent& e) final override;
	void setGate(double newValue);
	void setRampTime(double newAttackTimeMs);

	NodePropertyT<bool> useMidi;

	double attackTimeSeconds = 0.01;
	double sr = 0.0;


	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
};

DEFINE_EXTERN_NODE_TEMPLATE(ramp_envelope, ramp_envelope_poly, ramp_envelope_impl);

template <int V> class haas_impl : public HiseDspBase
{
public:

	using DelayType = DelayLine<2048, DummyCriticalSection>;

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("haas");
	GET_SELF_AS_OBJECT(haas_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data) override;
	void prepare(PrepareSpecs ps);
	void reset();
	void processSingle(float* data, int numChannels);
	void process(ProcessData& d);
	bool handleModulation(double&);
	void setPosition(double newValue);
	
	double position = 0.0;
	PolyData<DelayType, NumVoices> delayL;
	PolyData<DelayType, NumVoices> delayR;
};

template <int V>
void scriptnode::core::haas_impl<V>::setPosition(double newValue)
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
		auto setZero = [](DelayType& t) { t.setFadeTimeSamples(2048); t.setDelayTimeSamples(0); };
		auto setSeconds = [d](DelayType& t) { t.setFadeTimeSamples(2048); t.setDelayTimeSeconds(d); };

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
bool scriptnode::core::haas_impl<V>::handleModulation(double&)
{
	return false;
}

template <int V>
void scriptnode::core::haas_impl<V>::process(ProcessData& d)
{
	if (d.numChannels == 2)
	{
		delayL.get().processBlock(d.data[0], d.size);
		delayR.get().processBlock(d.data[1], d.size);
	}
}

template <int V>
void scriptnode::core::haas_impl<V>::processSingle(float* data, int numChannels)
{
	if (numChannels == 2)
	{
		data[0] = delayL.get().getDelayedValue(data[0]);
		data[1] = delayR.get().getDelayedValue(data[1]);
	}
}

template <int V>
void scriptnode::core::haas_impl<V>::reset()
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
void scriptnode::core::haas_impl<V>::prepare(PrepareSpecs ps)
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
void scriptnode::core::haas_impl<V>::createParameters(Array<ParameterData>& data)
{
	{
		ParameterData p("Position");
		p.range = { -1.0, 1.0, 0.1 };
		p.defaultValue = 0.0;
		p.db = BIND_MEMBER_FUNCTION_1(haas_impl::setPosition);
		data.add(std::move(p));
	}
}

DEFINE_EXTERN_NODE_TEMPLATE(haas, haas_poly, haas_impl);
DEFINE_EXTERN_NODE_TEMPIMPL(haas_impl);



namespace panner_impl
{
// Template Alias Definition =======================================================

using panner_ = wrap::frame<2, container::multi<fix<1, core::gain_poly>, fix<1, core::gain_poly>>>;

template <int NV> struct instance : public hardcoded<panner_>
{
	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("panner");
	GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void createParameters(Array<ParameterData>& data)
	{
		// Node Registration ===============================================================
		registerNode(get<0>(obj), "gain2");
		registerNode(get<1>(obj), "gain3");

		// Parameter Initalisation =========================================================
		setParameterDefault("gain2.Gain", -6.0206);
		setParameterDefault("gain2.Smoothing", 20.0);
		setParameterDefault("gain3.Gain", -6.0206);
		setParameterDefault("gain3.Smoothing", 20.0);

		// Parameter Callbacks =============================================================
		{
			ParameterData p("Balance", { 0.0, 1.0, 0.01, 1.0 });

			auto param_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });
			auto param_target2 = getParameter("gain3.Gain", { -100.0, 0.0, 0.1, 5.42227 });

			param_target1.addConversion(ConverterIds::DryAmount);
			param_target2.addConversion(ConverterIds::WetAmount);

			p.db = [param_target1, param_target2, outer = p.range](double newValue)
			{
				auto normalised = outer.convertTo0to1(newValue);
				param_target1(normalised);
				param_target2(normalised);
			};

			data.add(std::move(p));
		}
	}

};

}

DEFINE_EXTERN_NODE_TEMPLATE(panner, panner_poly, panner_impl::instance);


} // namespace core





}
