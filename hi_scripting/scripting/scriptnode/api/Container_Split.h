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

template <typename... Processors> struct split : public container_base<Processors...>
{
	static constexpr bool isModulationSource = false;

	void process(ProcessData& d)
	{
		auto original = d.copyTo(splitBuffer, 0);
		int channelCounter = 0;

		process_split_each(d, original, channelCounter, this->indexes);
	}

	void processSingle(float* data, int numChannels)
	{
		float original[NUM_MAX_CHANNELS];
		memcpy(original, data, sizeof(float)*numChannels);
		int channelCounter = 0;

		process_split_single_each(data, original, numChannels, channelCounter, this->indexes);
	}

	void prepare(PrepareSpecs ps)
	{
		this->prepare_each(ps, this->indexes);

		ps.numChannels *= 2;
		DspHelpers::increaseBuffer(splitBuffer, ps);
	}

	bool handleModulation(double& value)
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
        HiseEvent copy(e);
        
		handle_event_each_copy(copy, this->indexes);
	}

	auto& getObject() { return *this; };
	const auto& getObject() const { return *this; };

private:

	template <std::size_t ...Ns>
	void handle_event_each_copy(HiseEvent& e, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(this->processors).handleHiseEvent(e), void(), int{})...
		};
	}

	template <class T> void process_node(T& t, float* frameData, float* original, int numChannels, int& channelCounter)
	{
		if (channelCounter++ == 0)
			t.processSingle(frameData, numChannels);
		else
		{
			float wb[NUM_MAX_CHANNELS];
			memcpy(wb, original, sizeof(float)*numChannels);
			t.processSingle(wb, numChannels);
			FloatVectorOperations::add(frameData, wb, numChannels);
		}
	}

	template <class T> void process_node(T& t, ProcessData& d, ProcessData& original, int& channelCounter)
	{
		if (channelCounter++ == 0)
			t.process(d);
		else
		{
			auto wd = original.copyTo(splitBuffer, 1);
			t.process(wd);
			d += wd;
		}
	}

	template <std::size_t ...Ns>
	void process_split_each(ProcessData& d, ProcessData& original, int& channelCounter, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (process_node(std::get<Ns>(this->processors), d, original, channelCounter), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void process_split_single_each(float* frameData, float* original, int numChannels, int& channelCounter, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (process_node(std::get<Ns>(this->processors), frameData, original, numChannels, channelCounter), void(), int{})...
		};
	}

	AudioSampleBuffer splitBuffer;
};


#if OLD_IMPL

namespace impl {

template <bool IsFirst, typename Processor, typename Subclass>
struct SplitElement
{
	using P = Processor;

	virtual ~SplitElement() {};

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
		processor.handleHiseEvent(e);
	}

	void handleHiseEventWithCopy(HiseEvent& e)
	{
		HiseEvent c(e);
		processor.handleHiseEvent(c);
	}

	void process(ProcessData& data) noexcept
	{
		processor.process(data);
	}

	void processWithCopy(ProcessData& data, ProcessData& original, AudioSampleBuffer& splitBuffer)
	{
		auto wd = original.copyTo(splitBuffer, 1);
		processor.process(wd);
		data += wd;
	}

	void processSingle(float* frameData, int numChannels)
	{
		processor.processSingle(frameData, numChannels);
	}

	void processSingleWithOriginal(float* frameData, float* originalData, int numChannels)
	{
		float wb[NUM_MAX_CHANNELS];
		memcpy(wb, originalData, sizeof(float)*numChannels);

		processor.processSingle(wb, numChannels);

		FloatVectorOperations::add(frameData, wb, numChannels);
	}

	SplitElement& getObject() { return *this; };
	const SplitElement& getObject() const { return *this; };

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

template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct SplitBase : public SplitElement<IsFirst, FirstProcessor, SplitBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = SplitElement<IsFirst, FirstProcessor, SplitBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

	void processWithCopy(ProcessData& data, ProcessData& original, AudioSampleBuffer& splitBuffer) noexcept
	{
		if (IsFirst)
		{
			Base::process(data);
			processors.processWithCopy(data, original, splitBuffer);
		}
		else
		{
			auto wd = original.copyTo(splitBuffer, 1);

			Base::process(wd);

			data += wd;
			processors.processWithCopy(data, original, splitBuffer);
		}
	}

	void prepare(PrepareSpecs ps)
	{
		Base::prepare(ps);
		processors.prepare(ps);
	}

	void handleHiseEventWithCopy(HiseEvent& e)
	{
		Base::handleHiseEventWithCopy(e);
		processors.handleHiseEventWithCopy(e);
	}

	void initialise(NodeBase* n)
	{
		Base::initialise(n);
		processors.initialise(n);
	}

	void processSingleWithOriginal(float* frameData, float* originalData, int numChannels) noexcept
	{
		if (IsFirst)
		{
			Base::processSingle(frameData, numChannels);
		}
		else
		{
			float wb[NUM_MAX_CHANNELS];
			memcpy(wb, originalData, sizeof(float)*numChannels);

			Base::processSingle(wb, numChannels);
			FloatVectorOperations::add(frameData, wb, numChannels);
		}

		processors.processSingleWithOriginal(frameData, originalData, numChannels);
	}

	bool handleModulation(double& value) noexcept
	{
		auto b = Base::handleModulation(value);
		return processors.handleModulation(value) || b;
	}

	void reset() { Base::reset(); processors.reset(); }

	SplitBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct SplitBase<IsFirst, ProcessorType> : public SplitElement<IsFirst, ProcessorType, SplitBase<IsFirst, ProcessorType>> {};


}

template <typename... Processors> struct split2
{
	using Type = impl::SplitBase<true, Processors...>;

	static constexpr bool isModulationSource = Type::isModulationSource;
	static constexpr bool isSingleNode = sizeof...(Processors) == 1;

	void initialise(NodeBase* n)
	{
		static_assert(!isSingleNode, "no single split allowed");
		obj.initialise(n);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEventWithCopy(e);
	}

	void process(ProcessData& data)
	{
		auto original = data.copyTo(splitBuffer, 0);
		obj.processWithCopy(data, original, splitBuffer);
	}

	void processSingle(float* frameData, int numChannels)
	{
		float originalData[NUM_MAX_CHANNELS];
		memcpy(originalData, frameData, sizeof(float)*numChannels);

		obj.processSingleWithOriginal(frameData, originalData, numChannels);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
		ps.numChannels *= 2;

		if (!isSingleNode)
			DspHelpers::increaseBuffer(splitBuffer, ps);
	}

	void reset()
	{
		obj.reset();
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	Type obj;
	AudioSampleBuffer splitBuffer;
};
#endif



}

}
