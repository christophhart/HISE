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

namespace parameter
{

template <int N> struct branch_index
{
	static constexpr int NumElements = N;

	PARAMETER_SPECS(parameter::ParameterType::Single, 0);

	void call(double v)
	{
		currentIndex = jlimit<uint8>(0, N-1, roundToInt(v));
	}

	static void callStatic(void*, double)
	{
		
	};

	bool isConnected() const { return true; }

	void addToList(ParameterDataList& d)
	{
		data p("Index");
		p.callback.referTo(this, callStatic);
		d.add(p);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	template <int P> NormalisableRange<double> createParameterRange()
	{
		static_assert(P == 0, "not zero");
		return NormalisableRange<double>();
	}

	uint8 currentIndex = 0;
};
}

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

#define CASE(idx, function, arg) case idx: this->template get<jmin(NumElements-1, idx)>().function(arg); break
#define CASE_16(function, arg) CASE(0, function, arg); \
							   CASE(1, function, arg); \
							   CASE(2, function, arg); \
							   CASE(3, function, arg); \
							   CASE(4, function, arg); \
							   CASE(5, function, arg); \
							   CASE(6, function, arg); \
							   CASE(7, function, arg); \
							   CASE(8, function, arg); \
							   CASE(9, function, arg); \
							   CASE(10, function, arg); \
							   CASE(11, function, arg); \
							   CASE(12, function, arg); \
							   CASE(13, function, arg); \
							   CASE(14, function, arg); \
							   CASE(15, function, arg); \
							   CASE(16, function, arg);

template <typename Unused, typename... Processors> struct branch: public container_base<parameter::branch_index<sizeof...(Processors)>, Processors...>
{
    using Type = container_base<parameter::branch_index<sizeof...(Processors)>, Processors...>;
    
    static constexpr int NumElements = sizeof...(Processors);
    static constexpr int NumChannels = Helpers::getNumChannelsOfFirstElement<Processors...>();
    static constexpr int getNumChannels() { return NumChannels; }

    using BlockType = snex::Types::ProcessData<NumChannels>;
    using FrameType = snex::Types::span<float, NumChannels>;

    SN_GET_SELF_AS_OBJECT(branch);

	int getCurrentIndex() const noexcept
	{
		return this->parameters.currentIndex;
	}

    branch() = default;
    branch(const branch& other) = default;
    ~branch() override {};
    
    /** prepares all child nodes with the same specs. */
    void prepare(PrepareSpecs ps)
    {
		static_assert(NumElements <= 16, "max branch child node amount is 16");

        call_tuple_iterator1(prepare, ps);
    }

    /** Processes all child nodes. */
    void process(BlockType& d)
    {
		switch(getCurrentIndex())
		{
			CASE_16(process, d);
		}
    }

    /** Process all child nodes one frame at a time. */
    void processFrame(FrameType& d)
    {
		switch(getCurrentIndex())
		{
			CASE_16(processFrame, d)	
		}
    }

	SN_EMPTY_SET_EXTERNAL_DATA;

    /** Calls `handleHiseEvent` for all child nodes. */
    void handleHiseEvent(HiseEvent& e)
    {
		switch(getCurrentIndex())
		{
			CASE_16(handleHiseEvent, e);
		}
    }
};

#undef CASE
#undef CASE_16

}

}
