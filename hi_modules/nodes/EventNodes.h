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

struct MidiDisplayProviderBase
{
	virtual ~MidiDisplayProviderBase() {};

	virtual float getDisplayValue() const = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiDisplayProviderBase);
};

template <int V> class MidiSourceNode : public HiseDspBase,
							            public MidiDisplayProviderBase
{
public:

	static constexpr int NumVoices = V;

	enum class Mode
	{
		Gate = 0,
		Velocity,
		NoteNumber,
		Frequency
	};

	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_POLY_NODE_ID("midi");
	GET_SELF_AS_OBJECT(MidiSourceNode);

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(40);
	SET_HISE_NODE_EXTRA_WIDTH(256);
#endif

	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_PROCESS;

	void initialise(NodeBase* n);

	void prepare(PrepareSpecs sp);;
	void handleHiseEvent(HiseEvent& e);
	bool handleModulation(double& value);
	void createParameters(Array<ParameterData>& data) override;

	float getDisplayValue() const override;

	void setMode(double newMode)
	{
		currentMode = (Mode)(int)newMode;
	}

	Mode currentMode;
	ScriptFunctionManager scriptFunction;
	
	PolyData<ModValue, NumVoices> modValue;
};

struct TimerInfo
{
	bool active = false;
	int samplesBetweenCallbacks = 22050;
	int samplesLeft = 22050;

	bool tick()
	{
		return --samplesLeft <= 0;
	}

	void reset()
	{
		samplesLeft = samplesBetweenCallbacks;
	}
};

struct TimerDisplayProviderBase
{
	virtual ~TimerDisplayProviderBase() {}

	virtual bool& getActiveFlag() = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TimerDisplayProviderBase)
};

template <int NV> class TimerNode : public HiseDspBase,
									public TimerDisplayProviderBase
{
public:

	using FirstChannelType = snex::Types::ProcessDataFix<1>;

	constexpr static int NumVoices = NV;

	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_POLY_NODE_ID("timer");
	GET_SELF_AS_OBJECT(TimerNode);

#if RE
	SET_HISE_NODE_EXTRA_HEIGHT(40);
	SET_HISE_NODE_EXTRA_WIDTH(256);
#endif

	TimerNode();

	void initialise(NodeBase* n);

	void prepare(PrepareSpecs ps) override;
	void reset();

	bool& getActiveFlag() override;

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& thisInfo = t.get();

		if (!thisInfo.active)
			return;

		if (thisInfo.tick())
		{
			auto newValue = scriptFunction.callWithDouble(0.0);

			modValue.get().setModValue(newValue);

			ui_led = true;
			thisInfo.reset();
		}

		if (fillMode.getValue())
		{
			auto value = (float)modValue.get().getModValue();

			for (auto& s : d)
				s = value;
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& fd = d.as<FirstChannelType>();

		processFirstChannel(fd);

		auto ptrs = d.getRawDataPointers();

		for (int i = 1; i < d.getNumChannels(); i++)
			FloatVectorOperations::copy(ptrs[i], ptrs[0], d.getNumSamples());
	}

	void processFirstChannel(FirstChannelType& d);
	bool handleModulation(double& value);
	void createParameters(Array<ParameterData>& data) override;

	void setActive(double value);
	void setInterval(double timeMs);

	bool ui_led = false;

private:

	double sr = 44100.0;

	PolyData<TimerInfo, NumVoices> t;

	ScriptFunctionManager scriptFunction;
	NodePropertyT<bool> fillMode;
	
	PolyData<ModValue, NumVoices> modValue;	
};

DEFINE_EXTERN_NODE_TEMPLATE(midi, midi_poly, MidiSourceNode);
DEFINE_EXTERN_NODE_TEMPLATE(timer, timer_poly, TimerNode);
}



}
