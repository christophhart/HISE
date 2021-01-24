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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;


namespace core
{



struct SnexEventProcessor: public SnexSource
{
	enum class Mode
	{
		Gate = 0,
		Velocity,
		NoteNumber,
		Frequency,
		Custom
	};

	static StringArray getModes()
	{
		return { "Gate", "Velocity", "NoteNumber", "Frequency", "Custom" };
	}

	String getEmptyText() const override
	{
		String c;

		c << "void prepare(PrepareSpecs ps)\n";
		c << "{\n\n    \n}\n\n";
		c << "int getMidiValue(HiseEvent& e, double& value)\n";
		c << "{\n    return 0;\n}\n";

		return c;
	}

	SnexEventProcessor() :
		SnexSource(),
		mode(PropertyIds::Mode, "Gate")
	{
		
	};

	void prepare(PrepareSpecs ps);

	void initialise(NodeBase* n)
	{
		SnexSource::initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(SnexEventProcessor::setMode));
		mode.initialise(n);
		modeValue.referTo(mode.getPropertyTree().getPropertyAsValue(PropertyIds::Value, n->getUndoManager()));
	}

	void codeCompiled() override
	{
		f = obj["getMidiValue"];
	}

	void setMode(Identifier id, var newValue)
	{
		if (id == PropertyIds::Value)
		{
			auto v = newValue.toString();

			auto idx = getModes().indexOf(v);

			if (idx != -1)
				currentMode = (Mode)idx;
		}
	}

	bool getMidiValue(HiseEvent& e, double& v)
	{
		switch (currentMode)
		{
		case Mode::Gate:
		{
			if (e.isNoteOnOrOff())
			{
				v = e.isNoteOn() ? 1.0 : 0.0;
				lastValue = v;
				return true;
			}

			break;
		}
		case Mode::Velocity:
		{
			if (e.isNoteOn())
			{
				v = (double)e.getVelocity() / 127.0;
				lastValue = v;
				return true;
			}

			break;
		}
		case Mode::NoteNumber:
		{
			if (e.isNoteOn())
			{
				v = (double)e.getNoteNumber() / 127.0;
				lastValue = v;
				return true;
			}

			break;
		}
		case Mode::Frequency:
		{
			if (e.isNoteOn())
			{
				v = (e.getFrequency() - 20.0) / 19980.0;
				lastValue = v;
				return true;
			}
			break;
		}
		case Mode::Custom:
		{
			HiseEvent* eptr = &e;
			double* s = &v;

			if (f.function != nullptr)
			{
				auto ok = (bool)f.call<int>(eptr, s);
				lastValue = v;
				return ok;
			}
				
			break;
		}
		}

		return false;
	}

	double lastValue = 0.0;
	Mode currentMode = Mode::Gate;
	NodePropertyT<String> mode;
	Value modeValue;
	snex::jit::FunctionData f;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexEventProcessor);
};


template <typename MidiType> class midi: public HiseDspBase 
{
public:

	SET_HISE_NODE_ID("midi");
	SN_GET_SELF_AS_OBJECT(midi);

	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_PROCESS;

	static constexpr bool isNormalisedModulation() { return true; }

	constexpr bool isPolyphonic() const { return false; }

	void initialise(NodeBase* n)
	{
		mType.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		mType.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		double thisModValue = 0.0;
		auto thisChanged = mType.getMidiValue(e, thisModValue);
		if (thisChanged)
			v.setModValueIfChanged(thisModValue);
	}

	bool handleModulation(double& value)
	{
		return v.getChangedValue(value);
	}

	void createParameters(ParameterDataList& data)
	{
	}

	MidiType mType;
	ModValue v;
};

struct TimerInfo
{
	bool active = false;
	int samplesBetweenCallbacks = 22050;
	int samplesLeft = 22050;
	double lastValue = 0.0f;
	
	bool getChangedValue(double& v)
	{
		if (samplesBetweenCallbacks >= samplesLeft)
		{
			v = lastValue;
			return true;
		}

		return false;
	}

	bool tick()
	{
		return --samplesLeft <= 0;
	}

	void reset()
	{
		samplesLeft = samplesBetweenCallbacks;
		lastValue = 0.0;
	}
};


struct SnexEventTimer: public SnexSource
{
	SnexEventTimer():
		SnexSource()
	{}

	void codeCompiled() override
	{
		f = obj["getTimerValue"];
	}

	String getEmptyText() const override
	{
		String s;

		s << "double getTimerValue()\n";
		s << "{\n    return 0.0;\n}\n";
		return s;
	}

	double getTimerValue()
	{
		ui_led = true;
		if (f.function != nullptr)
		{
			lastValue = f.call<double>();
			return lastValue;
		}
			
		return 0.0;
	}

	double lastValue = 0.0;
	bool ui_led = false;
	snex::jit::FunctionData f;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexEventTimer);
};

template <typename TimerType> struct timer_base
{
	enum class Parameters
	{
		Active,
		Interval
	};

	void initialise(NodeBase* n)
	{
		tType.initialise(n);
	}

	TimerType tType;
	double sr = 44100.0;
};

template <int NV, typename TimerType> class timer_impl: public timer_base<TimerType>
{
public:

	enum Parameters
	{
		Active,
		Interval,
		numParameters
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Active, timer_impl);
		DEF_PARAMETER(Interval, timer_impl);
	}

	constexpr bool isNormalisedModulation() const { return false; }
	constexpr static int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("timer");
	SN_GET_SELF_AS_OBJECT(timer_impl);

	timer_impl()
	{
		
	}

	void prepare(PrepareSpecs ps)
	{
		this->sr = ps.sampleRate;
		this->tType.prepare(ps);
		t.prepare(ps);
	}

	void reset()
	{
		auto v = this->tType.getTimerValue();

		for (auto& ti : t)
		{
			ti.reset();
			ti.lastValue = v;
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& thisInfo = t.get();

		if (!thisInfo.active)
			return;

		if (thisInfo.tick())
		{
			thisInfo.reset();
			thisInfo.lastValue = this->tType.getTimerValue();
		}
	}

	void handleHiseEvent(HiseEvent& e) {};

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& thisInfo = t.get();

		if (!thisInfo.active)
			return;

		const int numSamples = d.getNumSamples();

		if (numSamples < thisInfo.samplesLeft)
		{
			thisInfo.samplesLeft -= numSamples;
		}
		else
		{
			const int numRemaining = numSamples - thisInfo.samplesLeft;

			t.get().lastValue = this->tType.getTimerValue();
			const int numAfter = numSamples - numRemaining;
			thisInfo.samplesLeft = thisInfo.samplesBetweenCallbacks + numRemaining;
		}
	}

	bool handleModulation(double& value)
	{
		bool ok = false;

		for (auto& ti : t)
			ok |= ti.getChangedValue(value);

		return ok;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(timer_impl, Active);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(timer_impl, Interval);
			p.setRange({ 0.0, 2000.0, 0.1 });
			p.setDefaultValue(500.0);
			data.add(std::move(p));
		}
	}

	void setActive(double value)
	{
		bool thisActive = value > 0.5;

		for (auto& ti : t)
		{
			if (ti.active != thisActive)
			{
				ti.active = thisActive;
				ti.reset();
			}
		}
	}

	void setInterval(double timeMs)
	{
		auto newTime = roundToInt(timeMs * 0.001 * this->sr);

		for (auto& ti : t)
			ti.samplesBetweenCallbacks = newTime;
	}

private:

	PolyData<TimerInfo, NumVoices> t;
};



struct SnexOscillator : public SnexSource
{
	SnexOscillator()
	{

	}

	void codeCompiled() override
	{
		ok = false;
		tickFunction = obj["tick"];
		processFunction = obj["process"];

		auto tickMatches = tickFunction.returnType == Types::ID::Float && tickFunction.args[0].typeInfo.getType() == Types::ID::Double;
		auto processMatches = processFunction.returnType == Types::ID::Void;

		ok = processFunction.function != nullptr && tickMatches && processMatches && processFunction.function != nullptr;
	}

	String getEmptyText() const override
	{
		String s;
		s << "float tick(double uptime)\n";
		s << "{\n    return 0.0f;\n}\n";

		s << "void process(OscProcessData& d)\n";
		s << "{\n";
		s << "    for (auto& s : d.data)\n";
		s << "    {\n";
		s << "        s = tick(d.uptime);\n";
		s << "        d.uptime += d.delta;\n";
		s << "    }\n";
		s << "}\n";

		return s;
	}

	void initialise(NodeBase* n)
	{
		SnexSource::initialise(n);
	}

	float tick(double uptime)
	{
		jassert(isReady());
		return processFunction.call<float>(uptime);
	}

	void process(OscProcessData& d)
	{
		jassert(isReady());
		processFunction.callVoid(&d);
	}

	bool isReady() const { return ok; }

	bool ok = false;
	FunctionData tickFunction;
	FunctionData processFunction;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexOscillator);
};

template <typename T> struct snex_osc_base
{
	void initialise(NodeBase* n)
	{
		oscType.initialise(n);
	}

	T oscType;
};

template <int NV, typename T> struct snex_osc_impl: snex_osc_base<T>
{
	enum class Parameters
	{
		Frequency,
		PitchMultiplier,
		Extra1,
		Extra2
	};

	static constexpr int NumVoices = NV;

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, snex_osc_impl);
		DEF_PARAMETER(PitchMultiplier, snex_osc_impl);
		DEF_PARAMETER(Extra1, snex_osc_impl);
		DEF_PARAMETER(Extra2, snex_osc_impl);
	}

	SET_HISE_POLY_NODE_ID("snex_osc");
	SN_GET_SELF_AS_OBJECT(snex_osc_impl);

	void prepare(PrepareSpecs ps)
	{
		sampleRate = ps.sampleRate;
		voiceIndex = ps.voiceIndex;
		oscData.prepare(ps);
		reset();
	}

	void reset()
	{
		for (auto& o : oscData)
			o.reset();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (this->oscType.isReady())
		{
			auto& thisData = oscData.get();
			auto uptime = thisData.tick();
			data[0] += this->oscType.tick(thisData.tick());
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (this->oscType.isReady())
		{
			auto& thisData = oscData.get();

			OscProcessData op;

			op.data.referToRawData(data.getRawDataPointers()[0], data.getNumSamples());
			op.uptime = thisData.uptime;
			op.delta = thisData.uptimeDelta * thisData.multiplier;
			op.voiceIndex = voiceIndex->getVoiceIndex();
			
			this->oscType.process(op);
			thisData.uptime += op.delta * (double)data.getNumSamples();
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			setFrequency(e.getFrequency());
	}

	void setExtra1(double newValue)
	{
		
	}

	void setExtra2(double newValue)
	{

	}

	void setFrequency(double newValue)
	{
		if (sampleRate > 0.0)
		{
			auto cyclesPerSecond = newValue;
			auto cyclesPerSample = cyclesPerSecond / sampleRate;

			for (auto& o : oscData)
				o.uptimeDelta = cyclesPerSample;
		}
	}

	void setPitchMultiplier(double newMultiplier)
	{
		newMultiplier = jlimit(0.01, 100.0, newMultiplier);

		for (auto& o : oscData)
			o.multiplier = newMultiplier;
	}

	double sampleRate = 0.0;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(snex_osc_impl, Frequency);
			p.setRange({ 20.0, 20000.0, 0.1 });
			p.setSkewForCentre(1000.0);
			p.setDefaultValue(220.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(snex_osc_impl, PitchMultiplier);
			p.setRange({ 1.0, 16.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(snex_osc_impl, Extra1);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(snex_osc_impl, Extra2);
			data.add(std::move(p));
		}
	}


	PolyHandler* voiceIndex = nullptr;
	PolyData<OscData, NumVoices> oscData;
};

template <typename OscType> using snex_osc = snex_osc_impl<1, OscType>;
template <typename OscType> using snex_osc_poly = snex_osc_impl<NUM_POLYPHONIC_VOICES, OscType>;


template <typename TimerType> using timer = timer_impl<1, TimerType>;
template <typename TimerType> using timer_poly = timer_impl<NUM_POLYPHONIC_VOICES, TimerType>;

}



}
