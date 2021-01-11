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

struct NodeBase;

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

class peak
{
public:

	SET_HISE_NODE_ID("peak");
	SN_GET_SELF_AS_OBJECT(peak);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;

	bool isPolyphonic() const { return false; }

	bool handleModulation(double& value);
	void reset() noexcept;;

	constexpr bool isNormalisedModulation() const { return true; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		snex::hmath Math;

		max = 0.0f;

		for (auto& ch : data)
		{
			auto range = FloatVectorOperations::findMinAndMax(data.toChannelData(ch).begin(), data.getNumSamples());
			max = jmax<float>(max, Math.abs(range.getStart()), Math.abs(range.getEnd()));
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		snex::hmath Math;

		max = 0.0;

		for (auto& s : data)
			max = Math.max(max, Math.abs((double)s));
	}

	// This is no state variable, so we don't need it to be polyphonic...
	double max = 0.0;
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
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_SET_PARAMETER;
	
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
	HISE_EMPTY_HANDLE_EVENT;
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
	void handleHiseEvent(HiseEvent& e);

	void setPeriodTime(double periodTimeMs);
	void setLoopStart(double loopStart);

private:

	double sr = 44100.0;
	double periodTime = 500.0;
	PolyData<OscData, NumVoices> state;
	PolyData<double, NumVoices> currentValue;
	PolyData<double, NumVoices> loopStart;
};


DEFINE_EXTERN_NODE_TEMPLATE(ramp, ramp_poly, ramp_impl);






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
	
	void process(snex::Types::ProcessData<1>& data)
	{
		auto f = data.toFrameData();

		currentVoiceData = &voiceData.get();

		while (f.next())
			processFrame(f.toSpan());
	}

	void processFrame(snex::Types::span<float, 1>& data)
	{
		if (currentVoiceData == nullptr)
			currentVoiceData = &voiceData.get();

		auto& s = data[0];

		switch (currentMode)
		{
		case Mode::Sine:	 s += tickSine(*currentVoiceData); break;
		case Mode::Triangle: s += tickTriangle(*currentVoiceData); break;
		case Mode::Saw:		 s += tickSaw(*currentVoiceData); break;
		case Mode::Square:	 s += tickSquare(*currentVoiceData); break;
		case Mode::Noise:	 s += Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
		}
	}

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

	PARAMETER_MEMBER_FUNCTION;

	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;

	OscData* currentVoiceData = nullptr;

	double freqValue = 220.0;
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
	
	void setFreqMultiplier(double input);
	void setModulator(double newGain);
	void setFrequency(double newFrequency);

private:

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
		Smoothing,
		ResetValue
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gain, gain_impl);
		DEF_PARAMETER(Smoothing, gain_impl);
		DEF_PARAMETER(ResetValue, gain_impl);
	}

	static constexpr int NumVoices = V;

	SET_HISE_POLY_NODE_ID("gain");
	SN_GET_SELF_AS_OBJECT(gain_impl);

	void prepare(PrepareSpecs ps);

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto gainFactor = gainer.get().advance();

		for (auto& s : data)
			s *= gainFactor;
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& thisGainer = gainer.get();

		if (thisGainer.isActive())
			DspHelpers::forwardToFrame16(this, data);
		else
		{
			auto gainFactor = thisGainer.get();

			for (auto ch : data)
				data.toChannelData(ch) *= gainFactor;
		}
	}

	void reset() noexcept;;

	bool handleModulation(double&) noexcept { return false; };
	void createParameters(ParameterDataList& data) override;

	void handleHiseEvent(HiseEvent& e);

	void setGain(double newValue);
	void setSmoothing(double smoothingTimeMs);
	void setResetValue(double newResetValue);

	double gainValue = 1.0;
	double sr = 0.0;
	double smoothingTime = 0.02;
	double resetValue = 0.0;

	PolyData<sfloat, NumVoices> gainer;
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
	void handleHiseEvent(HiseEvent& e);
	
	void setSmoothingTime(double newSmoothingTime);
	void setDefaultValue(double newDefaultValue);

	double smoothingTimeMs = 100.0;
	float defaultValue = 0.0f;
	PolyData<hise::Smoother, NumVoices> smoother;
	PolyData<ModValue, NumVoices> modValue;
};

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
	void handleHiseEvent(HiseEvent& e);
	void setGate(double newValue);
	void setRampTime(double newAttackTimeMs);

	double attackTimeSeconds = 0.01;
	double sr = 0.0;

	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
};

DEFINE_EXTERN_NODE_TEMPLATE(ramp_envelope, ramp_envelope_poly, ramp_envelope_impl);

} // namespace core


extern template class scriptnode::wrap::fix<1, core::oscillator_impl<1>>;
extern template class scriptnode::wrap::fix<1, core::oscillator_impl<NUM_POLYPHONIC_VOICES>>;


}
