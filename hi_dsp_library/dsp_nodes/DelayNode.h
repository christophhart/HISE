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

namespace core 
{

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
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_NODE_ID("fix_delay");
	SN_GET_SELF_AS_OBJECT(fix_delay);
	SN_DESCRIPTION("a non-interpolating delay line");

	fix_delay() {};

	fix_delay& operator=(const fix_delay& other)
	{
		delayTimeSeconds = other.delayTimeSeconds;

        for (int i = 0; i < other.delayLines.size(); i++)
			delayLines.add(new DelayLine<>());

		return *this;
	}

	SN_EMPTY_HANDLE_EVENT;

	void prepare(PrepareSpecs ps);
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
