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


template <typename... Processors> struct multi: public container_base<Processors...>
{
	static constexpr bool isModulationSource = false;

	void initialise(NodeBase* b)
	{
		init_each(b, indexes);
	}

	template <class T> void process_multi(T& obj, ProcessData& d, int& channelIndex)
	{
		constexpr int numChannelsThisTime = T::getFixChannelAmount();

		float* currentChannelData[numChannelsThisTime];
		memcpy(currentChannelData, d.data + channelIndex, sizeof(float*)*numChannelsThisTime);

		ProcessData thisData(currentChannelData, numChannelsThisTime, d.size);
		obj.process(thisData);

		channelIndex += numChannelsThisTime;
	}

	template <class T> void process_single_multi(T& obj, float* frameData, int& channelIndex)
	{
		constexpr int numChannelsThisTime = T::getFixChannelAmount();

		float* d = frameData + channelIndex;
		obj.processSingle(d, numChannelsThisTime);

		channelIndex += numChannelsThisTime;
	}

	template <std::size_t ...Ns>
	void process_each_multi(ProcessData& d, int& channelIndex, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (process_multi(std::get<Ns>(processors), d, channelIndex), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void process_single_each_multi(float* data, int& channelIndex, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (process_single_multi(std::get<Ns>(processors), data, channelIndex), void(), int{})...
		};
	}

	void process(ProcessData& d)
	{
		int channelIndex = 0;
		process_each_multi(d, channelIndex, indexes);
	}

	void processSingle(float* data, int )
	{
		int channelIndex = 0;
		process_single_each_multi(data, channelIndex, indexes);
	}

	void reset()
	{
		reset_each(indexes);
	}

	void prepare(PrepareSpecs ps)
	{
		prepare_each(ps, indexes);
	}

	bool handleModulation(double& value)
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		handle_event_each(e, indexes);
	}
};


#if OLD_IMPL

namespace impl {

//==============================================================================
template <bool IsFirst, typename Processor, typename Subclass>
struct MultiElement
{
	using P = Processor;

	virtual ~MultiElement() {};

	void prepare(PrepareSpecs ps)
	{
		processor.prepare(ps);
	}

	void initialise(NodeBase* n)
	{
		processor.initialise(n);
	}

	bool handleModulation(double& value) noexcept
	{
		if (processor.isModulationSource)
			return processor.handleModulation(value);

		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		HiseEvent c(e);
		processor.handleHiseEvent(c);
	}


	void process(ProcessData& d)
	{
		ProcessData copy(d);

		constexpr int c = Processor::getFixChannelAmount();

		copy.numChannels = c;
		d.numChannels -= c;
		d.data += c;

		jassert(d.numChannels >= 0);

		processor.process(copy);
	}

	void processSingle(float* frameData, int)
	{
		constexpr int c = Processor::getFixChannelAmount();
		processor.processSingle(frameData, c);
	}

	MultiElement& getObject() { return *this; };
	const MultiElement& getObject() const { return *this; };

	void reset() { processor.reset(); }

	Processor processor;

	static constexpr bool isModulationSource = Processor::isModulationSource;

	Processor& getProcessor() noexcept { return processor; }
	const Processor& getProcessor() const noexcept { return processor; }
	Subclass& getThis() noexcept { return *static_cast<Subclass*> (this); }
	const Subclass& getThis() const noexcept { return *static_cast<const Subclass*> (this); }

	template <int arg> auto& get() noexcept { return AccessHelper<arg>::get(getThis()); }
	template <int arg> const auto& get() const noexcept { return AccessHelper<arg>::get(getThis()); }
};


//==============================================================================
template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct MultiBase : public MultiElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = MultiElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

	void prepare(PrepareSpecs ps)
	{
		Base::prepare(ps);
		processors.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		Base::handleHiseEvent(e);
		processors.handleHiseEvent(e);
	}

	void process(ProcessData& data) noexcept
	{
		if (IsFirst)
		{
			ProcessData copy(data);

			Base::process(copy);
			processors.process(copy);
		}
		else
		{
			Base::process(data);
			processors.process(data);
		}
	}

	void processSingle(float* frameData, int) noexcept
	{
		Base::processSingle(frameData, 0);

		auto c = FirstProcessor::getFixChannelAmount();

		processors.processSingle(frameData + c, 0);
	}

	void initialise(NodeBase* n)
	{
		Base::initialise(n);
		processors.initialise(n);
	}

	bool handleModulation(double& value) noexcept
	{
		auto b = Base::handleModulation(value);
		return processors.handleModulation(value) || b;
	}

	void reset() { Base::reset(); processors.reset(); }

	MultiBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct MultiBase<IsFirst, ProcessorType> : public MultiElement<IsFirst, ProcessorType, MultiBase<IsFirst, ProcessorType>> {};


}

template <typename... Processors> struct multi : public impl::MultiBase<true, Processors...>
{
#if 0
	using Type =
		static constexpr bool isModulationSource = Type::isModulationSource;

	static constexpr bool isSingleNode = sizeof...(Processors) == 1;

	void initialise(NodeBase* n)
	{
		static_assert(!isSingleNode, "no single multi allowed");

		obj.initialise(n);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	void process(ProcessData& data)
	{


		obj.processWithFixedChannels(copy);
	}

	void processSingle(float* frameData, int)
	{
		obj.processSingleWithFixedChannels(&frameData);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	Type obj;
#endif
};

#endif

}

}
