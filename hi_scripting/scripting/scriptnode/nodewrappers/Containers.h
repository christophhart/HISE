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

namespace impl { 

template <int arg>
struct AccessHelper
{
	template <typename ProcessorType>
	static auto& get(ProcessorType& a) noexcept { return AccessHelper<arg - 1>::get(a.processors); }

	template <typename ProcessorType>
	static const auto& get(const ProcessorType& a) noexcept { return AccessHelper<arg - 1>::get(a.processors); }
};

template <>
struct AccessHelper<0>
{
	template <typename ProcessorType>
	static auto& get(ProcessorType& a) noexcept { return a.getProcessor(); }

	template <typename ProcessorType>
	static const auto& get(const ProcessorType& a) noexcept { return a.getProcessor(); }
};

//==============================================================================
template <typename Processor, typename Subclass>
struct ChainElement
{
	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		processor.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data) noexcept
	{
		processor.process(data);
	}

	void processSingle(float* frameData, int numChannels)
	{
		processor.processSingle(frameData, numChannels);
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

	void reset() { processor.reset(); }

	Processor processor;

	static constexpr bool isModulationSource = Processor::isModulationSource;

	Processor& getProcessor() noexcept { return processor; }
	const Processor& getProcessor() const noexcept { return processor; }
	Subclass& getThis() noexcept { return *static_cast<Subclass*> (this); }
	const Subclass& getThis() const noexcept { return *static_cast<const Subclass*> (this); }

	ChainElement& getObject() { return *this; };
	const ChainElement& getObject() const { return *this; };

	template <int arg> auto& get() noexcept { return AccessHelper<arg>::get(getThis()); }
	template <int arg> const auto& get() const noexcept { return AccessHelper<arg>::get(getThis()); }
};

//==============================================================================
template <typename FirstProcessor, typename... SubsequentProcessors>
struct ChainBase : public ChainElement<FirstProcessor, ChainBase<FirstProcessor, SubsequentProcessors...>>
{
	using Base = ChainElement<FirstProcessor, ChainBase<FirstProcessor, SubsequentProcessors...>>;

	void process(ProcessData& data) noexcept
	{
		Base::process(data);
		processors.process(data);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		Base::prepare(numChannels, sampleRate, blockSize);
		processors.prepare(numChannels, sampleRate, blockSize);
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

	ChainBase<SubsequentProcessors...> processors;
};

template <typename ProcessorType> struct ChainBase<ProcessorType> : public ChainElement<ProcessorType, ChainBase<ProcessorType>> {};

}

template <typename... Processors> using chain = impl::ChainBase<Processors...>;


template <class T> class mod
{
public:

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(int numChannelsToProcess, double sampleRate, int blockSize)
	{
		obj.prepare(numChannelsToProcess, sampleRate / (double)HISE_EVENT_RASTER, blockSize / HISE_EVENT_RASTER);
	}

	void reset()
	{
		obj.reset();
		singleCounter = 0;
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData = { &d, 1, numToProcess };

		obj.process(modData);
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (--singleCounter > 0) return;

		singleCounter = HISE_EVENT_RASTER;
		float value = 0.0f;

		obj.processSingle(&value, 1);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	T& getObject() { return obj.getObject(); }
	const T& getObject() const { return obj.getObject(); }

	T obj;
	int singleCounter = 0;
};


}

}
