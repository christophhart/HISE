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


namespace chainprocessor
{
template <typename ProcessDataType> struct Block
{
	Block(ProcessDataType& d_) :
		d(d_)
	{}

	template <class T> void operator()(T& obj)
	{
		obj.process(d);
	}

	ProcessDataType& d;
};

template <typename FrameDataType> struct Frame
{
	Frame(FrameDataType& d_) :
		d(d_)
	{};

	template <class T> void operator()(T& obj)
	{
		obj.processFrame(d);
	}

	FrameDataType& d;
};
}

template <class ParameterClass, typename... Processors> struct chain: public container_base<ParameterClass, Processors...>
{
	static constexpr int NumChannels = Helpers::getNumChannelsOfFirstElement<Processors...>();
	static constexpr int getNumChannels() { return NumChannels; }

	using BlockType = snex::Types::ProcessDataFix<NumChannels>;
	using BlockProcessor = chainprocessor::Block<BlockType>;

	using FrameType = snex::Types::span<float, NumChannels>;
	using FrameProcessor = chainprocessor::Frame<FrameType>;

	GET_SELF_AS_OBJECT(chain);

	void prepare(PrepareSpecs ps)
	{
		call_tuple_iterator1(prepare, ps);
	}

	void process(BlockType& d)
	{
		BlockProcessor p(d);
		call_tuple_iterator1(process, p);
	}

	void processFrame(FrameType& d)
	{
		FrameProcessor p(d);
		call_tuple_iterator1(processFrame, p);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		call_tuple_iterator1(handleHiseEvent, e);
	}

private:

	tuple_iterator_op(process, BlockProcessor);
	tuple_iterator_op(processFrame, FrameProcessor);

};

}

}
