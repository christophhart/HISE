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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

struct combined_parameter_base
{
	struct Data
	{
		double getPmaValue() const { return value * mulValue + addValue; }

		double getPamValue() const { return (value + addValue) * mulValue; }

		double value = 0.0;
		double mulValue = 1.0;
		double addValue = 0.0;
	};

	virtual Data getUIData() const = 0;

	NormalisableRange<double> currentRange;

	JUCE_DECLARE_WEAK_REFERENCEABLE(combined_parameter_base);
};

template <int NumVoices> struct combined_parameter : public combined_parameter_base
{
	Data getUIData() const override {
		return data.getFirst();
	}

	PolyData<Data, NumVoices> data;
};

namespace core
{



template <class ParameterType, int NumVoices=1> struct pma : public combined_parameter<NumVoices>
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
		for (auto& s : this->data)
		{
			s.value = v;
			sendParameterChange(s);
		}
	}

	void setAdd(double v)
	{
		for (auto& s : this->data)
		{
			s.addValue = v;
			sendParameterChange(s);
		}
	}

	void prepare(PrepareSpecs ps)
	{
		this->data.prepare(ps);
	}

	/** This method can be used to connect a target to the combined output of this
	    node.
	*/
	template <int I, class T> void connect(T& t)
	{
		p.getParameter<0>().connect<I>(t);
	}

	HISE_EMPTY_RESET;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_HANDLE_EVENT;

	void setMultiply(double v)
	{
		for (auto& s : this->data)
		{
			s.mulValue = v;
			sendParameterChange(s);
		}
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

private:

	void sendParameterChange(combined_parameter_base::Data& d)
	{
		p.call(d.getPmaValue());
	}
};

struct fix_delay : public HiseDspBase
{
	enum class Parameters
	{
		DelayTime,
		FadeTime
};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(DelayTime, fix_delay);
		DEF_PARAMETER(FadeTime, fix_delay);
	}

	SET_HISE_NODE_ID("fix_delay");
	SN_GET_SELF_AS_OBJECT(fix_delay);

	fix_delay() {};

	void prepare(PrepareSpecs ps);
	bool handleModulation(double&) noexcept { return false; };
	void reset() noexcept;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		jassert(data.getNumChannels() == delayLines.size());
		int index = 0;

		for(auto c: data)
			delayLines[index++]->processBlock(c.getRawWritePointer(), data.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		jassert(data.size() == delayLines.size());

		int index = 0;

		for (auto& s: data)
			s = delayLines[index++]->getDelayedValue(s);
	}

	void setDelayTime(double newValue);
	void setFadeTime(double newValue);
	void createParameters(ParameterDataList& data) override;

	OwnedArray<DelayLine<>> delayLines;
	
	double delayTimeSeconds = 0.1;
};
}



}
