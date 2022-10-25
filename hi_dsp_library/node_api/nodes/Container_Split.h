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

namespace container
{

namespace splitprocessor
{

template <class T> static constexpr bool isSingleElement(const T& t) { return T::NumElements == 1; };

template <class T> static void copy(T& dst, const T& source)
{
	for (int i = 0; i < dst.size(); i++)
	{
		dst[i] = source[i];
	}
}

template <class ProcessDataType, int N> struct Block
{
	using BufferType = snex::Types::heap<float>;
	using RefType = snex::Types::dyn<float>;

	static constexpr int NumElements = N;
	static constexpr int NumChannels = ProcessDataType::NumChannels;

	Block(ProcessDataType& d_, BufferType& splitBuffer_, BufferType& workBuffer_) :
		originalBuffer(splitBuffer_),
		workBuffer(workBuffer_),
		d(d_)
	{
		if (!isSingleElement(*this))
		{
			ProcessDataHelpers<NumChannels>::copyTo(d, originalBuffer);
		}
	}

	template <class T> void operator()(T& t)
	{
		if (isSingleElement(*this) || channelCounter++ == 0)
			t.process(d);
		else
		{
			originalBuffer.copyTo(workBuffer);
			auto wcd = snex::Types::ProcessDataHelpers<NumChannels>::makeChannelData(workBuffer, d.getNumSamples());

			ProcessData<NumChannels> wd(wcd.begin(), d.getNumSamples());
			wd.copyNonAudioDataFrom(d);

			t.process(wd);

			auto dPtr = d.getRawDataPointers();
			const auto wPtr = wd.getRawDataPointers();

			for (int i = 0; i < d.getNumChannels(); i++)
				FloatVectorOperations::add(dPtr[i], wPtr[i], d.getNumSamples());
		}
	}

	ProcessDataType& d;
	RefType originalBuffer;
	RefType workBuffer;
	
	int channelCounter = 0;
};

template <typename FrameType, int N> struct Frame
{
	static constexpr int NumElements = N;

	Frame(FrameType& d_) :
		d(d_),
		channelCounter(0)
	{
		if (!isSingleElement(*this))
			copy(original, d);
	};

	template <class T> void operator()(T& t)
	{
		if (isSingleElement(*this) || channelCounter++ == 0)
			t.processFrame(d);
		else
		{
			FrameType wb;
			copy(wb, original);
			t.processFrame(wb);

			for (int i = 0; i < d.size(); i++)
				d[i] += wb[i];
		}
	}

	FrameType original;
	FrameType& d;

	int channelCounter;
};
}

template <class ParameterClass, typename... Processors> struct split : public container_base<ParameterClass, Processors...>
{
    using Type = container_base<ParameterClass, Processors...>;
    
	SN_GET_SELF_AS_OBJECT(split);
	static constexpr int N = sizeof...(Processors);

	static constexpr int NumChannels = Helpers::getNumChannelsOfFirstElement<Processors...>();
	static constexpr int getNumChannels() { return NumChannels; }

	using BlockType = snex::Types::ProcessData<NumChannels>;
	using BlockProcessor = splitprocessor::Block<BlockType, N>;

	using FrameType = snex::Types::span<float, NumChannels>;
	using FrameProcessor = splitprocessor::Frame<FrameType, N>;

	using BufferType = snex::Types::heap<float>;
	
	void prepare(PrepareSpecs ps)
	{
		call_tuple_iterator1(prepare, ps);

		if (N > 1)
		{
            snex::Types::FrameConverters::increaseBuffer(originalBuffer, ps);
			snex::Types::FrameConverters::increaseBuffer(workBuffer, ps);
		}
	}

	template <class ProcessDataType> void process(ProcessDataType& d)
	{
		if (N > 1)
		{
			// If this fires, you don't have called prepare yet...
			jassert(!originalBuffer.isEmpty());
			jassert(!workBuffer.isEmpty());
		}

		BlockProcessor p(d, originalBuffer, workBuffer);
		call_tuple_iterator1(process, p);
	}

	void processFrame(FrameType& d)
	{
		FrameProcessor p(d);
		call_tuple_iterator1(processFrame, p);
	}

	void handleHiseEvent(HiseEvent& e)
	{
        HiseEvent copy(e);
		call_tuple_iterator1(handleHiseEvent, copy);
	}

private:

	tuple_iterator_op(process, BlockProcessor);
	tuple_iterator_op(processFrame, FrameProcessor);

	BufferType originalBuffer;
	BufferType workBuffer;
};

}

}
