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

namespace midi_logic
{

struct gate
{
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_INITIALISE;

	bool getMidiValue(HiseEvent& e, double& v)
	{
		if (e.isNoteOnOrOff())
		{
			v = (double)e.isNoteOn();
			return true;
		}

		return false;
	}
};

struct velocity
{
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_INITIALISE;

	bool getMidiValue(HiseEvent& e, double& v)
	{
		if (e.isNoteOn())
		{
			v = e.getFloatVelocity();
			return true;
		}

		return false;
	}
};

struct notenumber
{
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_INITIALISE;

	bool getMidiValue(HiseEvent& e, double& v)
	{
		if (e.isNoteOn())
		{
			v = (double)e.getNoteNumber() / 127.0;
			return true;
		}

		return false;
	}
};

struct frequency
{
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_INITIALISE;

	bool getMidiValue(HiseEvent& e, double& v)
	{
		if (e.isNoteOn())
		{
			v = (e.getFrequency() - 20.0) / 19980.0;
			return true;
		}

		return false;
	}
};
}

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
template <typename MidiType> class midi
{
public:

	SET_HISE_NODE_ID("midi");
	SN_GET_SELF_AS_OBJECT(midi);

	
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_CREATE_PARAM;

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

template <int NV, typename TimerType> class timer_impl : public timer_base<TimerType>
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

		if (P > 1)
		{
			auto typed = static_cast<timer_impl*>(obj);
			typed->tType.setParameter<P - 2>(value);
		}
	}
	PARAMETER_MEMBER_FUNCTION;

	static constexpr bool isNormalisedModulation() { return false; }
	constexpr static int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("timer");
	SN_GET_SELF_AS_OBJECT(timer_impl);

	timer_impl()
	{

	}

	HISE_EMPTY_HANDLE_EVENT;

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

template <typename TimerType> using timer = timer_impl<1, TimerType>;
template <typename TimerType> using timer_poly = timer_impl<NUM_POLYPHONIC_VOICES, TimerType>;

}


} 