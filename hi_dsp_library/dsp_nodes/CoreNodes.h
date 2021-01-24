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

struct table
{
	SET_HISE_NODE_ID("table");
	SN_GET_SELF_AS_OBJECT(table);

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_SET_PARAMETER;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_CREATE_PARAM;

	using TableSpanType = span<float, SAMPLE_LOOKUP_TABLE_SIZE>;

	~table()
	{
		int x = 5;
	}

	bool isPolyphonic() const { return false; };
	constexpr bool isNormalisedModulation() const { return true; }

	bool handleModulation(double& value)
	{
		return currentValue.getChangedValue(value);
	}

	void prepare(PrepareSpecs ps)
	{
		smoothedValue.prepare(ps.sampleRate, 20.0);
	}

	void reset() noexcept
	{
		currentValue.setModValue(0.0);
		smoothedValue.reset();
	}

	TableSpanType* getTableData()
	{
		return reinterpret_cast<TableSpanType*>(tableData.data);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		if (auto td = getTableData())
		{
			block b;
			tableData.referBlockTo(b, 0);

			float v = 0.0f;

			for (auto& s : data[0])
				v = processFloat(s);

			tableData.setDisplayedValue(v);
		}
	}

	float processFloat(float& s)
	{
		auto& data = *getTableData();

		TableSpanType::wrapped i1, i2;

		auto peakValue = s;
		auto floatIndex = peakValue * (float)SAMPLE_LOOKUP_TABLE_SIZE;

		i1 = (int)floatIndex & (SAMPLE_LOOKUP_TABLE_SIZE - 1);
		i2 = i1 + 1;

		auto alpha = floatIndex - (float)i1;

		s = Interpolator::interpolateLinear(data[i1], data[i2], hmath::fmod(floatIndex, 1.0f));

		return peakValue;
	}

	void setExternalData(const ExternalData& d, int)
	{
		auto t = reinterpret_cast<uint64_t>(this);
		auto s = reinterpret_cast<uint64_t>(d.data);

		constexpr int z = sizeof(table);

		auto diff = s - t - z;

		if (d.numSamples == SAMPLE_LOOKUP_TABLE_SIZE)
			tableData = d;
			
		else
			jassertfalse;
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (auto td = getTableData())
		{
			processFloat(data[0]);
		}
	}

	ExternalData tableData;

	sfloat smoothedValue;

	ModValue currentValue;

	JUCE_DECLARE_WEAK_REFERENCEABLE(table);
};

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

	bool handleModulation(double& value)
	{
		value = max;
		return true;
	}

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
        {
            auto dst = data[1];
            Math.vcopy(dst, data[0]);
        }
			
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

	ramp_impl()
	{
		setPeriodTime(100.0);
	}

	void prepare(PrepareSpecs ps)
	{
		currentValue.prepare(ps);
		state.prepare(ps);
		loopStart.prepare(ps);

		sr = ps.sampleRate;
		setPeriodTime(periodTime);
	}

	void reset() noexcept
	{
		for (auto& s : state)
			s.reset();

		currentValue.setAll(0.0);
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(PeriodTime, ramp_impl);
		DEF_PARAMETER(LoopStart, ramp_impl);
	}
	PARAMETER_MEMBER_FUNCTION;


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

	bool handleModulation(double& v)
	{
		v = currentValue.get();
		return true;
	}

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

	void createParameters(ParameterDataList& data) override
	{
		{
			DEFINE_PARAMETERDATA(ramp_impl, PeriodTime);
			p.setRange({ 0.1, 1000.0, 0.1 });
			p.setDefaultValue(100.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(ramp_impl, LoopStart);
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			state.get().reset();
	}

	void setPeriodTime(double periodTimeMs)
	{
		periodTime = periodTimeMs;

		if (sr > 0.0)
		{
			auto s = periodTime * 0.001;
			auto inv = 1.0 / jmax(0.00001, s);

			auto newUptimeDelta = jmax(0.0000001, inv / sr);

			for (auto& s : state)
				s.uptimeDelta = newUptimeDelta;
		}
	}

	void setLoopStart(double newLoopStart)
	{
		auto v = jlimit(0.0, 1.0, newLoopStart);

		for (auto& d : loopStart)
			d = v;
	}

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

	oscillator_impl() = default;

	HISE_EMPTY_INITIALISE;

	void reset()
	{
		for (auto& s : voiceData)
			s.reset();
	}

	void prepare(PrepareSpecs ps)
	{
		voiceData.prepare(ps);
		sr = ps.sampleRate;
		setFrequency(freqValue);
	}
	
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

	void handleHiseEvent(HiseEvent& e)
	{
		if (useMidi && e.isNoteOn())
		{
			setFrequency(e.getFrequency());
		}
	}


	void createParameters(ParameterDataList& data)
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

	void setMode(double newMode)
	{
		currentMode = (Mode)(int)newMode;
	}

	void setFrequency(double newFrequency)
	{
		freqValue = newFrequency;

		if (sr > 0.0)
		{
			auto newUptimeDelta = (double)(freqValue / sr * (double)sinTable->getTableSize());

			for (auto& d : voiceData)
				d.uptimeDelta = newUptimeDelta;
		}
	}

	void setPitchMultiplier(double newMultiplier)
	{
		auto pitchMultiplier = jlimit(0.01, 100.0, newMultiplier);

		for (auto& d : voiceData)
			d.multiplier = pitchMultiplier;
	}

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
		FrameConverters::forwardToFrameMono(this, data);
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

	HISE_EMPTY_MOD;

	void prepare(PrepareSpecs ps)
	{
		gainer.prepare(ps);
		sr = ps.sampleRate;

		setSmoothing(smoothingTime);
	}

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
			FrameConverters::forwardToFrame16(this, data);
		else
		{
			auto gainFactor = thisGainer.get();

			for (auto ch : data)
				data.toChannelData(ch) *= gainFactor;
		}
	}

	void reset() noexcept
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

	void createParameters(ParameterDataList& data) override
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

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			reset();
	}


	void setGain(double newValue)
	{
		gainValue = Decibels::decibelsToGain(newValue);

		float gf = (float)gainValue;

		for (auto& g : gainer)
			g.set(gf);
	}

	void setSmoothing(double smoothingTimeMs)
	{
		smoothingTime = smoothingTimeMs;

		if (sr <= 0.0)
			return;

		for (auto& g : gainer)
			g.prepare(sr, smoothingTime);
	}

	void setResetValue(double newResetValue)
	{
		resetValue = newResetValue;
	}

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

	HISE_EMPTY_INITIALISE;
    
	void createParameters(ParameterDataList& data) override
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

	void prepare(PrepareSpecs ps) 
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

	void reset()
	{
		auto d = defaultValue;

		for (auto& s : smoother)
			s.resetToValue(d, 0.0);
	}

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

	bool handleModulation(double& value)
	{
		return modValue.get().getChangedValue(value);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			reset();
	}

	void setSmoothingTime(double newSmoothingTime)
	{
		smoothingTimeMs = newSmoothingTime;

		for (auto& s : smoother)
			s.setSmoothingTime((float)newSmoothingTime);
	}

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = (float)newDefaultValue;

		auto d = defaultValue; auto sm = smoothingTimeMs;

		for (auto& s : smoother)
			s.resetToValue((float)d, (float)sm);
	}

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

	/* ============================================================================ ramp_impl */
	ramp_envelope_impl()
	{

	}

	void createParameters(ParameterDataList& data) override
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

	void prepare(PrepareSpecs ps)
	{
		gainer.prepare(ps);
		sr = ps.sampleRate;
		setRampTime(attackTimeSeconds * 1000.0);
	}

	void reset()
	{
		for (auto& g : gainer)
			g.setValueWithoutSmoothing(0.0f);
	}

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
			snex::Types::FrameConverters::forwardToFrame16(this, data);
		else
		{
			auto v = thisG.getTargetValue();

			for (auto ch : data)
				hmath::vmuls(data.toChannelData(ch), v);

			if (thisG.getCurrentValue() == 0.0)
				data.setResetFlag();
		}
	}

	bool handleModulation(double&)
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOnOrOff())
			gainer.get().setTargetValue(e.isNoteOn() ? 1.0f : 0.0f);
	}

	void setGate(double newValue)
	{
		for (auto& g : gainer)
			g.setTargetValue(newValue > 0.5 ? 1.0f : 0.0f);
	}

	void setRampTime(double newAttackTimeMs)
	{
		attackTimeSeconds = newAttackTimeMs * 0.001;

		if (sr > 0.0)
		{
			for (auto& g : gainer)
				g.reset(sr, attackTimeSeconds);
		}
	}

	double attackTimeSeconds = 0.01;
	double sr = 0.0;

	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
};

DEFINE_EXTERN_NODE_TEMPLATE(ramp_envelope, ramp_envelope_poly, ramp_envelope_impl);

} // namespace core


extern template class scriptnode::wrap::fix<1, core::oscillator_impl<1>>;
extern template class scriptnode::wrap::fix<1, core::oscillator_impl<NUM_POLYPHONIC_VOICES>>;


}
