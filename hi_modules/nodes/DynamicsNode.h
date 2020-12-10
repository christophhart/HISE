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

namespace dynamics
{
    
template <class DynamicProcessorType> class dynamics_wrapper : public HiseDspBase
{
public:

	enum class Parameters
	{
		Threshhold,
		Attack,
		Release,
		Ratio
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Threshhold, dynamics_wrapper);
		DEF_PARAMETER(Attack, dynamics_wrapper);
		DEF_PARAMETER(Release, dynamics_wrapper);
		DEF_PARAMETER(Ratio, dynamics_wrapper);
	}

	static Identifier getStaticId();

	GET_SELF_AS_OBJECT();

	dynamics_wrapper();

	void createParameters(ParameterDataList& data);
	bool handleModulation(double& max) noexcept;;
	void prepare(PrepareSpecs ps);
	void reset() noexcept;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrameStereo(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		if (data.size() == 2)
		{
			double values[2] = { data[0], data[1] };
			obj.process(values[0], values[1]);
			data[0] = (float)values[0];
			data[1] = (float)values[1];
		}
		else
		{
			jassert(data.size() == 1);

			double values[2] = { data[0], data[0] };
			obj.process(values[0], values[1]);
			data[0] = (float)values[0];
		}
	}

	void setThreshhold(double v);
	void setAttack(double v);
	void setRelease(double v);
	void setRatio(double v);

	DynamicProcessorType obj;
};


class envelope_follower : public HiseDspBase
{
public:

	enum class Parameters
	{
		Attack,
		Release
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Attack, envelope_follower);
		DEF_PARAMETER(Release, envelope_follower);
	}

	SET_HISE_NODE_ID("envelope_follower");
	GET_SELF_AS_OBJECT();

	envelope_follower();

	bool handleModulation(double&) noexcept { return false; };
	void prepare(PrepareSpecs ps) override;
	void reset() noexcept;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrameStereo(this, data);
	}

	template <typename FrameType> void processFrame(FrameType& data)
	{
		float input = 0.0;

		for (auto& s: data)
			input = Math.max(Math.abs(s), input);

		input = envelope.calculateValue(input);
		
		for (auto& s : data)
			s = input;
	}

	void setAttack(double v);
	void setRelease(double v);

	void createParameters(ParameterDataList& data);

	EnvelopeFollower::AttackRelease envelope;
	hmath Math;
};


DEFINE_EXTERN_MONO_TEMPLATE(gate, dynamics_wrapper<chunkware_simple::SimpleGate>);
DEFINE_EXTERN_MONO_TEMPLATE(comp, dynamics_wrapper<chunkware_simple::SimpleComp>);
DEFINE_EXTERN_MONO_TEMPLATE(limiter, dynamics_wrapper<chunkware_simple::SimpleLimit>);
;
}


}
