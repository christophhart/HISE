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

#if INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION
namespace container
{

template <class P, typename... Ts> using frame1_block = wrap::frame<1, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame2_block = wrap::frame<2, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame4_block = wrap::frame<4, container::chain<P, Ts...>>;
template <class P, typename... Ts> using framex_block = wrap::frame_x< container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample2x = wrap::oversample<2,   container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample4x = wrap::oversample<4,   container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample8x = wrap::oversample<8,   container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample16x = wrap::oversample<16, container::chain<P, Ts...>>;
template <class P, typename... Ts> using modchain = wrap::control_rate<chain<P, Ts...>>;

}
#endif

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
	HISE_EMPTY_PROCESS_SINGLE;

	tempo_sync();
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

	NodePropertyT<bool> useFreqDomain;

	JUCE_DECLARE_WEAK_REFERENCEABLE(tempo_sync);
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
	void processFrame(float* frameData, int numChannels);

	// This is no state variable, so we don't need it to be polyphonic...
	double max = 0.0;
};

class mono2stereo : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("mono2stereo");
	GET_SELF_AS_OBJECT(mono2stereo);
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_RESET;
	HISE_EMPTY_MOD;
	
	void process(ProcessData& data);
	void processFrame(float* frameData, int numChannels);
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
	void processFrame(float* frameData, int numChannels);
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

class hise_mod : public HiseDspBase
{
public:

	enum Index
	{
		Pitch,
		Extra1,
		Extra2,
		numIndexes
	};

	SET_HISE_NODE_ID("hise_mod");
	GET_SELF_AS_OBJECT(hise_mod);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_HEIGHT(30);

	

	hise_mod();

	bool isPolyphonic() const override { return true; }

	void initialise(NodeBase* b);
	void prepare(PrepareSpecs ps);
	
	void process(ProcessData& d);
	bool handleModulation(double& v);;
	void processFrame(float* frameData, int numChannels);

	void handleHiseEvent(HiseEvent& e) final override;

	Component* createExtraComponent(PooledUIUpdater* updater)
	{
		return new ModulationSourceBaseComponent(updater);
	}
	
	void createParameters(Array<ParameterData>& data) override;

	void setIndex(double index);

	void reset();

private:

	WeakReference<ModulationSourceNode> parentNode;
	int modIndex = -1;

	PolyData<ModValue, NUM_POLYPHONIC_VOICES> modValues;
	PolyData<double, NUM_POLYPHONIC_VOICES> uptime;
	
	double uptimeDelta = 0.0;
	double synthBlockSize = 0.0;

	WeakReference<JavascriptSynthesiser> parentProcessor;
};




DEFINE_EXTERN_NODE_TEMPLATE(ramp, ramp_poly, ramp_impl);


template <int NV> class oscillator_impl: public HiseDspBase
{
public:

	enum class Mode
	{
		Sine,
		Saw,
		Triangle,
		Square,
		Noise,
		numModes
	};

	constexpr static int NumVoices = NV;

	SET_HISE_NODE_EXTRA_HEIGHT(50);
	SET_HISE_POLY_NODE_ID("oscillator");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(oscillator_impl);

	oscillator_impl();

	void createParameters(Array<HiseDspBase::ParameterData>& data);
	
	void initialise(NodeBase* n) override;
	void reset() noexcept;;
	void prepare(PrepareSpecs ps);
	bool handleModulation(double&) noexcept { return false; };
	void process(ProcessData& data);
	void processFrame(float* data, int numChannels);
	void handleHiseEvent(HiseEvent& e) override;

	float tickSine(OscData& d);
	float tickTriangle(OscData& d);
	float tickSaw(OscData& d);
	float tickNoise(OscData& d);
	float tickSquare(OscData& d);

	Component* createExtraComponent(PooledUIUpdater* updater);

	void setMode(double newMode);
	void setFrequency(double newFrequency);
	void setPitchMultiplier(double newMultiplier);

	StringArray modes;
	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;
	Random r;

	SharedResourcePointer<SineLookupTable<2048>> sinTable;

	NodePropertyT<bool> useMidi;

	Mode currentMode = Mode::Sine;
	double freqValue = 220.0;
};



DEFINE_EXTERN_NODE_TEMPLATE(oscillator, oscillator_poly, oscillator_impl);

class fm : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("fm");
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	GET_SELF_AS_OBJECT(fm);

	bool isPolyphonic() const { return true; }

	void prepare(PrepareSpecs ps);
	void reset();
	bool handleModulation(double& ) { return false; }
	void process(ProcessData& d);

	template <typename FD> void processFrame(FD& d)
	{
		auto& od = oscData.get();
		double modValue = (double)d[0];
		d[0] = sinTable->getInterpolatedValue(od.tick());
		od.uptime += modGain.get() * modValue;
	}

	void createParameters(Array<ParameterData>& data) override;

	void handleHiseEvent(HiseEvent& e);
	
private:

	void setFreqMultiplier(double input);
	void setModulatorGain(double newGain);
	void setFrequency(double newFrequency);

	double sr = 0.0;
	double freqMultiplier = 1.0;

	PolyData<OscData, NUM_POLYPHONIC_VOICES> oscData;
	PolyData<double, NUM_POLYPHONIC_VOICES> modGain;

	SharedResourcePointer<SineLookupTable<2048>> sinTable;
};

template <int V> class gain_impl : public HiseDspBase
{
public:

	enum Parameter
	{
		Gain,
		SmoothingTime
	};

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
	void processFrame(float* numFrames, int numChannels);
	bool handleModulation(double&) noexcept { return false; };
	void createParameters(Array<ParameterData>& data) override;

	HISE_STATIC_PARAMETER_TEMPLATE
	{
		TEMPLATE_PARAMETER_CALLBACK(Gain, setGain);
		TEMPLATE_PARAMETER_CALLBACK(SmoothingTime, setSmoothingTime);
	}

	void setGain(double newValue);
	void setSmoothingTime(double smoothingTimeMs);

	double gainValue = 1.0;
	double sr = 0.0;
	double smoothingTime = 0.02;

	NodePropertyT<float> resetValue;
	NodePropertyT<float> useResetValue;
	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
	AudioSampleBuffer gainBuffer;
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
	void processFrame(float* data, int numChannels);
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
	void processFrame(float* data, int numChannels);
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






} // namespace core





}
