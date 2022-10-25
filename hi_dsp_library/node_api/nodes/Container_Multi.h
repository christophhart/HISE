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
using namespace snex::Types;

namespace container
{

namespace multiprocessor
{
template <typename ProcessDataType> struct Block
{
	Block(ProcessDataType& d_):
		d(d_)
	{}

	template <class T> void operator()(T& obj)
	{
		constexpr int NumChannelsThisTime = T::NumChannels;

		ProcessData<NumChannelsThisTime> thisData(d.getRawDataPointers() + channelIndex, d.getNumSamples());
		thisData.copyNonAudioDataFrom(d);

		obj.process(thisData);
		channelIndex += NumChannelsThisTime;
	}

	ProcessDataType& d;
	int channelIndex = 0;
};

template <typename FrameType> struct Frame
{
	Frame(FrameType& frameData_):
		frameData(frameData_)
	{};

	template <class T> void operator()(T& obj)
	{
		constexpr int NumChannelsThisTime = T::NumChannels;

		using ThisFrameType = span<float, NumChannelsThisTime>;

		auto d = reinterpret_cast<ThisFrameType*>(frameData.begin() + channelIndex);

		obj.processFrame(*d);

		channelIndex += NumChannelsThisTime;
	}
	
	int channelIndex = 0;
	FrameType& frameData;
};
}

template <class ParameterClass, typename... Processors> struct multi: public container_base<ParameterClass, Processors...>
{
    using Type = container_base<ParameterClass, Processors...>;
    
	SN_GET_SELF_AS_OBJECT(multi);

	constexpr static int NumChannels = Helpers::getSummedChannels<Processors...>();

	using BlockType = snex::Types::ProcessData<NumChannels>;
	using BlockProcessor = multiprocessor::Block<BlockType>;
	using FrameType = snex::Types::span<float, NumChannels>;
	using FrameProcessor = multiprocessor::Frame<FrameType>;

	static constexpr int getNumChannels()
	{
		return NumChannels;
	}

	void prepare(PrepareSpecs ps)
	{
		call_tuple_iterator1(prepare, ps);
	}

	void process(BlockType& d)
	{
		BlockProcessor p(d);
		call_tuple_iterator1(process, p);
	}

	void processFrame(FrameType& data)
	{
		FrameProcessor p(data);
		call_tuple_iterator1(processFrame, p);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		HiseEvent copy(e);
		call_tuple_iterator1(handleHiseEvent, copy);
	}

	

private:

	tuple_iterator_op (process, BlockProcessor);
	tuple_iterator_op (processFrame, FrameProcessor);
	
};

}

}
