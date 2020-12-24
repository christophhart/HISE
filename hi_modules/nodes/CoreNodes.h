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
using namespace snex;
using namespace snex::Types;

#if INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION
namespace container
{

template <class P, typename... Ts> using frame1_block = wrap::frame<1, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame2_block = wrap::frame<2, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame4_block = wrap::frame<4, container::chain<P, Ts...>>;
template <class P, typename... Ts> using framex_block = wrap::frame_x< container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample2x = wrap::oversample<2,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample4x = wrap::oversample<4,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample8x = wrap::oversample<8,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample16x = wrap::oversample<16, container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using modchain = wrap::control_rate<chain<P, Ts...>>;

}
#endif

namespace core
{

class tempo_sync : public HiseDspBase,
	public TempoListener
{
public:

	enum class Parameters
	{
		Tempo,
		Multiplier
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Tempo, tempo_sync);
		DEF_PARAMETER(Multiplier, tempo_sync);
	}

	bool isPolyphonic() const { return false; }

	SET_HISE_NODE_ID("tempo_sync");
	SN_GET_SELF_AS_OBJECT(tempo_sync);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;

	tempo_sync();
	~tempo_sync();

	void initialise(NodeBase* n) override;
	void createParameters(ParameterDataList& data) override;
	void tempoChanged(double newTempo) override;
	void setTempo(double newTempoIndex);
	bool handleModulation(double& max);

	constexpr bool isNormalisedModulation() const { return false; }

	void setMultiplier(double newMultiplier);

	

	double currentTempoMilliseconds = 500.0;
	double lastTempoMs = 0.0;
	double bpm = 120.0;

	double multiplier = 1.0;
	TempoSyncer::Tempo currentTempo = TempoSyncer::Tempo::Eighth;

	MainController* mc = nullptr;

	NodePropertyT<bool> useFreqDomain;

	JUCE_DECLARE_WEAK_REFERENCEABLE(tempo_sync);
};



template <class ParameterType> struct pma: public combined_parameter_base
{
	SET_HISE_NODE_ID("pma");
	SN_GET_SELF_AS_OBJECT(pma);

	enum class Parameters
	{
		Value,
		Multiply,
		Add
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Value, pma);
		DEF_PARAMETER(Multiply, pma);
		DEF_PARAMETER(Add, pma);
	};

	void setValue(double v)
	{
		data.value = v;
		sendParameterChange();
	}

	void setAdd(double v)
	{
		data.addValue = v;
		sendParameterChange();
	}

	HISE_EMPTY_RESET;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_HANDLE_EVENT;

	void setMultiply(double v)
	{
		data.mulValue = v;
		sendParameterChange();
	}

	void sendParameterChange()
	{
		p.call(data.getPmaValue());
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(pma, Value);
			p.range = { 0.0, 1.0 };
			p.defaultValue = 0.0;
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(pma, Multiply);
			p.defaultValue = 1.0;
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(pma, Add);
			p.defaultValue = 0.0;
			data.add(std::move(p));
		}
	}

	ParameterType p;
};

class peak : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("peak");
	SN_GET_SELF_AS_OBJECT(peak);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;

	bool isPolyphonic() const { return false; }

	bool handleModulation(double& value);
	void reset() noexcept;;

	constexpr bool isNormalisedModulation() const { return true; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		max = DspHelpers::findPeak(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		max = 0.0;

		for (auto& s : data)
			max = Math.max(max, Math.abs((double)s));
	}

	// This is no state variable, so we don't need it to be polyphonic...
	double max = 0.0;

	snex::hmath Math;
};

class mono2stereo : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("mono2stereo");
	SN_GET_SELF_AS_OBJECT(mono2stereo);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_RESET;
	HISE_EMPTY_MOD;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (data.getNumChannels() >= 2)
			Math.vcopy(data[1], data[0]);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() >= 2)
			data[1] = data[0];
	}

	hmath Math;
};

class empty : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("empty");
	SN_GET_SELF_AS_OBJECT(empty);

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

	enum class Parameters
	{
		PeriodTime,
		LoopStart,
		numParameters
	};

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("ramp");
	SN_GET_SELF_AS_OBJECT(ramp_impl);

	ramp_impl();

	void initialise(NodeBase* b);
	void prepare(PrepareSpecs ps);
	void reset() noexcept;;

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(PeriodTime, ramp_impl);
		DEF_PARAMETER(LoopStart, ramp_impl);
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& thisState = state.get();

		double thisUptime = thisState.uptime;
		double thisDelta = thisState.uptimeDelta;

		for (auto c : d)
		{
			thisUptime = thisState.uptime;

			for (auto& s : d.toChannelData(c))
			{
				if (thisUptime > 1.0)
					thisUptime = loopStart.get();

				s += (float)thisUptime;

				thisUptime += thisDelta;
			}
		}

		thisState.uptime = thisUptime;
		currentValue.get() = thisState.uptime;
	}

	bool handleModulation(double& v);;

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto newValue = state.get().tick();

		if (newValue > 1.0)
		{
			newValue = loopStart.get();
			state.get().uptime = newValue;
		}

		for (auto& s : data)
			s += (float)newValue;

		currentValue.get() = newValue;
	}

	void createParameters(ParameterDataList& data) override;
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

	enum class Parameters
	{
		Index, 
		numParameters
	};

	enum Index
	{
		Pitch,
		Extra1,
		Extra2,
		numIndexes
	};

	SET_HISE_NODE_ID("hise_mod");
	SN_GET_SELF_AS_OBJECT(hise_mod);

	hise_mod();

	constexpr bool isPolyphonic() const { return true; }

	bool isNormalisedModulation() const { return modIndex != ModulatorSynth::BasicChains::PitchChain; }

	void initialise(NodeBase* b);
	void prepare(PrepareSpecs ps);
	
	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (parentProcessor != nullptr)
		{
			auto numToDo = (double)d.getNumSamples();
			auto& u = uptime.get();

			modValues.get().setModValueIfChanged(parentProcessor->getModValueForNode(modIndex, roundToInt(u)));
			u = fmod(u + numToDo * uptimeDelta, synthBlockSize);
		}
	}

	bool handleModulation(double& v);;

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto numToDo = 1.0;
		auto& u = uptime.get();

		modValues.get().setModValueIfChanged(parentProcessor->getModValueForNode(modIndex, roundToInt(u)));
		u = Math.fmod(u + 1.0 * uptimeDelta, synthBlockSize);
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Index, hise_mod);
	}

	void handleHiseEvent(HiseEvent& e) override;
	
	void createParameters(ParameterDataList& data) override;

	void setIndex(double index);

	void reset();

private:

	WeakReference<ModulationSourceNode> parentNode;
	int modIndex = -1;

	PolyData<ModValue, NUM_POLYPHONIC_VOICES> modValues;
	PolyData<double, NUM_POLYPHONIC_VOICES> uptime;
	
	double uptimeDelta = 0.0;
	double synthBlockSize = 0.0;

	hmath Math;

	WeakReference<JavascriptSynthesiser> parentProcessor;
};


DEFINE_EXTERN_NODE_TEMPLATE(ramp, ramp_poly, ramp_impl);


struct OscillatorDisplayProvider
{
	struct UseMidi
	{
		DECLARE_SNEX_NATIVE_PROPERTY(UseMidi, bool, true);

		void set(OscillatorDisplayProvider& obj, bool newValue)
		{
			obj.useMidi = newValue;
		}
	};

	using UseMidiProperty = properties::native<UseMidi>;

	enum class Mode
	{
		Sine,
		Saw,
		Triangle,
		Square,
		Noise,
		numModes
	};

	OscillatorDisplayProvider()
	{
		modes = { "Sine", "Saw", "Triangle", "Square", "Noise" };
	}

	virtual ~OscillatorDisplayProvider() {};

	float tickNoise(OscData& d)
	{
		return r.nextFloat() * 2.0f - 1.0f;
	}

	float tickSaw(OscData& d)
	{
		return 2.0f * std::fmodf(d.tick() / sinTable->getTableSize(), 1.0f) - 1.0f;
	}

	float tickTriangle(OscData& d)
	{
		return (1.0f - std::abs(tickSaw(d))) * 2.0f - 1.0f;
	}

	float tickSine(OscData& d)
	{
		return sinTable->getInterpolatedValue(d.tick());
	}

	float tickSquare(OscData& d)
	{
		return (float)(1 - (int)std::signbit(tickSaw(d))) * 2.0f - 1.0f;
	}

	bool useMidi = false;

	Random r;
	SharedResourcePointer<SineLookupTable<2048>> sinTable;
	StringArray modes;
	Mode currentMode = Mode::Sine;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorDisplayProvider);
};



template <int NV> class oscillator_impl: public OscillatorDisplayProvider
{
public:

	enum class Parameters
	{
		Mode,
		Frequency,
		PitchMultiplier,
		numParameters
	};

	constexpr static int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("oscillator");
	SN_GET_SELF_AS_OBJECT(oscillator_impl);

	oscillator_impl();

	void initialise(NodeBase* n) {};
	void reset() noexcept;
	void prepare(PrepareSpecs ps);
	void process(snex::Types::ProcessData<1>& data);
	void processFrame(snex::Types::span<float, 1>& data);
	void handleHiseEvent(HiseEvent& e);
	void createParameters(ParameterDataList& data);
	void setMode(double newMode);
	void setFrequency(double newFrequency);
	void setPitchMultiplier(double newMultiplier);

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Mode, oscillator_impl);
		DEF_PARAMETER(Frequency, oscillator_impl);
		DEF_PARAMETER(PitchMultiplier, oscillator_impl);
	}

	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;

	OscData* currentVoiceData = nullptr;

	double freqValue = 220.0;
};

template <class T, class PropertyType> struct BoringWrapper
{
	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	static Identifier getStaticId() { return T::getStaticId(); }

	constexpr bool isPolyphonic() const { return obj.isPolyphonic(); }

	void initialise(NodeBase* n)
	{
		props.initWithRoot(n, obj.getWrappedObject());
		obj.initialise(n);
	}

	void reset()
	{
		obj.reset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		obj.processFrame(data);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		obj.process(data);
	}

	void createParameters(ParameterDataList& data)
	{
		obj.createParameters(data);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	PropertyType props;
	T obj;
};



using oscillator = wrap::fix<1, oscillator_impl<1>>;
using oscillator_poly = wrap::fix<1, oscillator_impl<NUM_POLYPHONIC_VOICES>>;





class fm : public HiseDspBase
{
public:

	enum class Parameters
	{
		Frequency,
		Modulator,
		FreqMultiplier
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, fm);
		DEF_PARAMETER(Modulator, fm);
		DEF_PARAMETER(FreqMultiplier, fm);
	}

	SET_HISE_NODE_ID("fm");
	SN_GET_SELF_AS_OBJECT(fm);

	bool isPolyphonic() const { return true; }

	void prepare(PrepareSpecs ps);
	void reset();
	bool handleModulation(double& ) { return false; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrameMono(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& od = oscData.get();
		double modValue = (double)d[0];
		d[0] = sinTable->getInterpolatedValue(od.tick());
		od.uptime += modGain.get() * modValue;
	}

	void createParameters(ParameterDataList& data) override;

	void handleHiseEvent(HiseEvent& e);
	
private:

	void setFreqMultiplier(double input);
	void setModulator(double newGain);
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

	enum class Parameters
	{
		Gain,
		Smoothing
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gain, gain_impl);
		DEF_PARAMETER(Smoothing, gain_impl);
	}

	static constexpr int NumVoices = V;

	SET_HISE_POLY_NODE_ID("gain");
	SN_GET_SELF_AS_OBJECT(gain_impl);

	gain_impl();

	void initialise(NodeBase* n);
	void prepare(PrepareSpecs ps);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& thisGainer = gainer.get();

		if (thisGainer.isSmoothing())
			DspHelpers::forwardToFrame16(this, data);
		else
		{
			auto gainFactor = thisGainer.getCurrentValue();

			for (auto ch : data)
				hmath::vmuls(data.toChannelData(ch), gainFactor);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto nextValue = gainer.get().getNextValue();

		for (auto& s : data)
			s *= nextValue;
	}

	void reset() noexcept;;

	bool handleModulation(double&) noexcept { return false; };
	void createParameters(ParameterDataList& data) override;

	void setGain(double newValue);
	void setSmoothing(double smoothingTimeMs);

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

	enum class Parameters
	{
		SmoothingTime,
		DefaultValue
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SmoothingTime, smoother_impl);
		DEF_PARAMETER(DefaultValue, smoother_impl);
	}

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("smoother");
	SN_GET_SELF_AS_OBJECT(smoother_impl);

	smoother_impl();

	void initialise(NodeBase* n) override;
	void createParameters(ParameterDataList& data) override;
	void prepare(PrepareSpecs ps);
	void reset();

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		data[0] = smoother.get().smooth(data[0]);
		modValue.get().setModValue(smoother.get().getDefaultValue());
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		smoother.get().smoothBuffer(data[0].data, data.getNumSamples());
		modValue.get().setModValue(smoother.get().getDefaultValue());
	}

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

	enum class Parameters
	{
		Gate,
		RampTime
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gate, ramp_envelope_impl);
		DEF_PARAMETER(RampTime, ramp_envelope_impl);
	}

	static constexpr int NumVoices = V;

	SET_HISE_POLY_NODE_ID("ramp_envelope");
	SN_GET_SELF_AS_OBJECT(ramp_envelope_impl);

	ramp_envelope_impl();

	void initialise(NodeBase* n) override;
	void createParameters(ParameterDataList& data) override;
	void prepare(PrepareSpecs ps);
	void reset();

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto& thisG = gainer.get();
		auto v = thisG.getNextValue();

		for (auto& s : data)
			s *= v;
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& thisG = gainer.get();

		if (thisG.isSmoothing())
			DspHelpers::forwardToFrame16(this, data);
		else
		{
			auto v = thisG.getTargetValue();

			for (auto ch : data)
				hmath::vmuls(data.toChannelData(ch), v);

			if (thisG.getCurrentValue() == 0.0)
				data.setResetFlag();
		}
	}

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


extern template class scriptnode::wrap::fix<1, core::oscillator_impl<1>>;
extern template class scriptnode::wrap::fix<1, core::oscillator_impl<NUM_POLYPHONIC_VOICES>>;


}
