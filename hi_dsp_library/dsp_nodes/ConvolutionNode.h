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

namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace filters
{

struct convolution : public data::base,
	public ConvolutionEffectBase
{
	enum Parameters
	{
		Gate,
		Predelay,
		Damping,
		HiCut,
		Multithread,
		numParameters
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gate, convolution);
		DEF_PARAMETER(Damping, convolution);
		DEF_PARAMETER(HiCut, convolution);
		DEF_PARAMETER(Predelay, convolution);
		DEF_PARAMETER(Multithread, convolution);
	};
	SN_PARAMETER_MEMBER_FUNCTION;

	SN_NODE_ID("convolution");
	SN_GET_SELF_AS_OBJECT(convolution);
	SN_DESCRIPTION("A convolution reverb node");

	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_INITIALISE;
	SN_EMPTY_MOD;

	convolution() = default;
	
	static constexpr bool isPolyphonic() { return false; }

	MultiChannelAudioBuffer& getImpulseBufferBase() override;

	const MultiChannelAudioBuffer& getImpulseBufferBase() const override;

	void setExternalData(const snex::ExternalData& d, int index) override;

	void prepare(PrepareSpecs specs);
	void reset();

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& dynData = data.template as<ProcessDataDyn>();
		processBase(dynData);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		jassertfalse;
	}

	void setMultithread(double shouldBeMultithreaded);
	void setDamping(double targetSustainDb);
	void setHiCut(double targetFreq);
	void setPredelay(double newDelay);
	void setGate(double shouldBeEnabled);
	void createParameters(ParameterDataList& data);
};

}
}




