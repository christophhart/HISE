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


template <typename... Processors> struct chain: public container_base<Processors...>
{
	static constexpr bool isModulationSource = false;

	void process(ProcessData& d)
	{
		process_each(d, this->indexes);
	}

	void processSingle(float* data, int numChannels)
	{
		process_single_each(data, numChannels, this->indexes);
	}

	void prepare(PrepareSpecs ps)
	{
		this->prepare_each(ps, this->indexes);
	}

	bool handleModulation(double& value)
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		this->handle_event_each(e, this->indexes);
	}

private:

	template <std::size_t ...Ns>
	void process_each(ProcessData& d, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(this->processors).process(d), void(), int{})...
		};
	}

	template <std::size_t ...Ns>
	void process_single_each(float* d, int numChannels, std::index_sequence<Ns...>) {
		using swallow = int[];
		(void)swallow {
			1, (std::get<Ns>(this->processors).processSingle(d, numChannels), void(), int{})...
		};
	}
};

#if OLD_IMPL

namespace impl
{
//==============================================================================
template <bool IsFirst, typename Processor, typename Subclass>
struct ChainElement
{
	using P = Processor;

	virtual ~ChainElement() {};

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

	void process(ProcessData& data) noexcept
	{
		processor.process(data);
	}

	void processSingle(float* frameData, int numChannels)
	{
		processor.processSingle(frameData, numChannels);
	}

	ChainElement& getObject() { return *this; };
	const ChainElement& getObject() const { return *this; };

	void reset() { processor.reset(); }

	Processor processor;

	static constexpr bool isModulationSource = Processor::isModulationSource;

	Processor& getProcessor() noexcept { return processor; }
	const Processor& getProcessor() const noexcept { return processor; }
	Subclass& getThis() noexcept { return *static_cast<Subclass*> (this); }
	const Subclass& getThis() const noexcept { return *static_cast<const Subclass*> (this); }

	/** dummy overrides from HiseDspBase class. */

	template <int arg> auto& get() noexcept { return AccessHelper<arg>::get(getThis()); }
	template <int arg> const auto& get() const noexcept { return AccessHelper<arg>::get(getThis()); }
};


//==============================================================================
template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct ChainBase : public ChainElement<IsFirst, FirstProcessor, ChainBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = ChainElement<IsFirst, FirstProcessor, ChainBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

	void process(ProcessData& data) noexcept
	{
		Base::process(data);
		processors.process(data);
	}

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

	void processSingle(float* frameData, int numChannels) noexcept
	{
		Base::processSingle(frameData, numChannels);
		processors.processSingle(frameData, numChannels);
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

	ChainBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct ChainBase<IsFirst, ProcessorType> : public ChainElement<IsFirst, ProcessorType, ChainBase<IsFirst, ProcessorType>> {};

}

template <typename... Processors> struct chain2 : impl::ChainBase<true, Processors...> {};
#endif


}

}
