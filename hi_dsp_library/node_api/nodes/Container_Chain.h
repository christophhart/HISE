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

/** A chain processes all its child nodes serially:

	@code
	| |
	V V
    Inp
    ||
	P1
	||
	P2
	||
	[...]
	||
	Px
	||
	Out


	   | |
	   V V
	  _Inp___...__
	//     \\     \\
	||     ||     ||
	P1     P2     Px
	\\_   _//_..._//
	   |+|
	   Out
	    

	  | |
	  V V
	  Inp
	 /   \
	P1   P2
	 \   /
	  Out
	@endcode

	The `ParameterClass` template parameter can be used to define compile-time
	parameter connections:

	@code
	using MyParameterType = parameter::plain<core::osc, 0>;
	using MyChainType = container::chain<MyParameterType, core::osc>;

	MyChainType c;
	c.getParameter<0>().connect(c.get<0>());

	// forwards the parameter to the internal oscillator
	c.setParameter<0>(440.0f);
	@endcode
*/
template <class ParameterClass, typename... Processors> struct chain: public container_base<ParameterClass, Processors...>
{
    using Type = container_base<ParameterClass, Processors...>;
    
    static constexpr int NumChannels = Helpers::getNumChannelsOfFirstElement<Processors...>();
	static constexpr int getNumChannels() { return NumChannels; }

	using BlockType = snex::Types::ProcessData<NumChannels>;
	using BlockProcessor = chainprocessor::Block<BlockType>;

	using FrameType = snex::Types::span<float, NumChannels>;
	using FrameProcessor = chainprocessor::Frame<FrameType>;

	

	SN_GET_SELF_AS_OBJECT(chain);

	chain() = default;
	chain(const chain& other) = default;

    ~chain() {};
    
	/** prepares all child nodes with the same specs. */
	void prepare(PrepareSpecs ps)
	{
		call_tuple_iterator1(prepare, ps);
	}

	/** Processes all child nodes. */
	void process(BlockType& d)
	{
		BlockProcessor p(d);
		call_tuple_iterator1(process, p);
	}

	/** Process all child nodes one frame at a time. */
	void processFrame(FrameType& d)
	{
		FrameProcessor p(d);
		call_tuple_iterator1(processFrame, p);
	}

	/** Calls `handleHiseEvent` for all child nodes. */
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
