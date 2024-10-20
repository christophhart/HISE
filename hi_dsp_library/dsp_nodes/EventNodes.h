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




namespace control
{


/** A midi processing node with a customizable logic. Supply a class as template that has a method with the prototype

	bool getMidiValue(HiseEvent& e, double& value)

	which sets the value to 0...1 according to the input and returns true if the event should
	cause a signal update:

		struct NoteNumberProcessor
		{
			bool getMidiValue(HiseEvent& e, double& value)
			{
				if(e.isNoteOn())
				{
					value = (double)e.getNoteNumber() / 127.0;
					return true;
				}

				return false;
			}
		};

		using MyMidi = core::midi<NoteNumberProcessor>;

	Take a look at the default classes defined in the
*/
template <typename MidiType> class midi: public control::pimpl::templated_mode
{
public:

	SN_NODE_ID("midi");
	SN_GET_SELF_AS_OBJECT(midi);
	SN_DESCRIPTION("Create a modulation signal from MIDI input");

	midi() :
		templated_mode(getStaticId(), "midi_logic")
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsProcessingHiseEvent);
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
	};
	
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_PROCESS;
	SN_EMPTY_CREATE_PARAM;

	constexpr bool isProcessingHiseEvent() const { return true; }
	static constexpr bool isNormalisedModulation() { return true; }

	constexpr bool isPolyphonic() const { return false; }

	void initialise(NodeBase* n)
	{
		if constexpr (prototypes::check::initialise<MidiType>::value)
			mType.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		mType.prepare(ps);
	}

	void reset()
	{
		v.reset();
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

	MidiType mType;
	ModValue v;
};


struct TimerInfo
{
	bool active = false;
	int samplesBetweenCallbacks = 22050;
	int samplesLeft = 22050;
	ModValue lastValue;

	bool getChangedValue(double& v)
	{
		return lastValue.getChangedValue(v);
	}

	bool tick(int numSamples)
	{
		samplesLeft -= numSamples;
		return samplesLeft <= 0;
	}

	bool tick()
	{
		return --samplesLeft <= 0;
	}

	void reset()
	{
		samplesLeft = samplesBetweenCallbacks;
		lastValue.setModValue(0.0);
	}
};

template <typename TimerType> struct timer_base: public mothernode
{
	enum class Parameters
	{
		Active,
		Interval
	};

	virtual ~timer_base() {};

	void initialise(NodeBase* n)
	{
		if constexpr (prototypes::check::initialise<TimerType>::value)
			tType.initialise(n);
	}

	TimerType tType;
	double sr = 44100.0;
};

template <int NV, typename TimerType> class timer : public timer_base<TimerType>,
													public polyphonic_base,
													public control::pimpl::templated_mode
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
		DEF_PARAMETER(Active, timer);
		DEF_PARAMETER(Interval, timer);

#if HISE_INCLUDE_SNEX
		if (P > 1)
		{
			auto typed = static_cast<timer*>(obj);
			typed->tType.template setParameter<P - 2>(value);
		}
#endif
	}

	SN_PARAMETER_MEMBER_FUNCTION;

	static constexpr bool isNormalisedModulation() { return true; }
	constexpr static int NumVoices = NV;

	SN_POLY_NODE_ID("timer");
	SN_GET_SELF_AS_OBJECT(timer);
	SN_DESCRIPTION("Create a periodic modulation signal if active");

	timer():
		polyphonic_base(getStaticId(), false),
		templated_mode(getStaticId(), "timer_logic")
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsOptionalSnexNode);
	}

	SN_EMPTY_HANDLE_EVENT;

	void prepare(PrepareSpecs ps)
	{
		this->sr = ps.sampleRate;

		if constexpr (prototypes::check::prepare<TimerType>::value)
			this->tType.prepare(ps);

		t.prepare(ps);
	}

	void reset()
	{
		if constexpr (prototypes::check::reset<TimerType>::value)
			this->tType.reset();

		auto v = this->tType.getTimerValue();

		for (auto& ti : t)
		{
			ti.reset();
			ti.lastValue.setModValue(v);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& thisInfo = t.get();

		if (!thisInfo.active)
			return;

		if (thisInfo.tick())
		{
			thisInfo.lastValue.setModValue(this->tType.getTimerValue());
			thisInfo.samplesLeft += thisInfo.samplesBetweenCallbacks;
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& thisInfo = t.get();

		if (!thisInfo.active)
			return;

		const int numSamples = d.getNumSamples();

		if (thisInfo.tick(numSamples))
		{
			thisInfo.lastValue.setModValue(this->tType.getTimerValue());
			thisInfo.samplesLeft += thisInfo.samplesBetweenCallbacks;
		}
	}

	bool handleModulation(double& value)
	{
		return t.begin()->getChangedValue(value);
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(timer, Active);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(timer, Interval);
			p.setRange({ 0.0, 2000.0, 0.1 });
			p.setDefaultValue(500.0);
			data.add(std::move(p));
		}
	}

	void setActive(double value)
	{
		bool thisActive = value > 0.5;
		auto v = this->tType.getTimerValue();

		for (auto& ti : t)
		{
			if (ti.active != thisActive)
			{
				ti.active = thisActive;
				ti.samplesLeft = ti.samplesBetweenCallbacks;
				ti.lastValue.setModValue(v);
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


}


} 
