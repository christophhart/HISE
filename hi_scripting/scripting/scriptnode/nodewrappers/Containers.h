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
template <bool IsFirst, typename Processor, typename Subclass>
struct ChainElement
{
	using P = Processor;

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		processor.prepare(numChannels, sampleRate, blockSize);
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

	void processWithCopy(ProcessData& data, ProcessData& original, AudioSampleBuffer& splitBuffer)
	{
		auto wd = original.copyTo(splitBuffer, 1);
		processor.process(wd);
		data += wd;
	}

	void processWithFixedChannels(ProcessData& d)
	{
		ProcessData copy(d);

		constexpr int c = Processor::getFixChannelAmount();

		copy.numChannels = c;
		d.numChannels -= c;
		d.data += c;

		jassert(d.numChannels >= 0);

		processor.process(copy);
	}

	void processSingleWithFixedChannels(float** frameData)
	{
		constexpr int c = Processor::getFixChannelAmount();
		processor.processSingle(*frameData, c);
		*frameData += c;
	}

	void process(ProcessData& data) noexcept
	{
		processor.process(data);
	}

	void processSingle(float* frameData, int numChannels)
	{
		processor.processSingle(frameData, numChannels);
	}

	void processSingleWithOriginal(float* frameData, float* originalData, int numChannels)
	{
		float wb[NUM_MAX_CHANNELS];
		memcpy(wb, originalData, sizeof(float)*numChannels);

		processor.processSingle(frameData, numChannels);

		FloatVectorOperations::add(frameData, wb, numChannels);		
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

	ChainBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct ChainBase<IsFirst, ProcessorType> : public ChainElement<IsFirst, ProcessorType, ChainBase<IsFirst, ProcessorType>> {};

//==============================================================================
template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct SplitBase : public ChainElement<IsFirst, FirstProcessor, SplitBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = ChainElement<IsFirst, FirstProcessor, SplitBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

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
			processors.processWithCopy(data, original, splitBuffer);

			data += wd;
		}
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		Base::prepare(numChannels, sampleRate, blockSize);
		processors.prepare(numChannels, sampleRate, blockSize);
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

	SplitBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct SplitBase<IsFirst, ProcessorType> : public ChainElement<IsFirst, ProcessorType, SplitBase<IsFirst, ProcessorType>> {};





//==============================================================================
template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct MultiBase : public ChainElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = ChainElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		Base::prepare(numChannels, sampleRate, blockSize);
		processors.prepare(numChannels, sampleRate, blockSize);
	}


	void processWithFixedChannels(ProcessData& data) noexcept
	{
		Base::processWithFixedChannels(data);
		processors.processWithFixedChannels(data);
	}

	void processSingleWithFixedChannels(float** frameData) noexcept
	{
		Base::processSingleWithFixedChannels(frameData);
		processors.processSingleWithFixedChannels(frameData);
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

template <bool IsFirst, typename ProcessorType> struct MultiBase<IsFirst, ProcessorType> : public ChainElement<IsFirst, ProcessorType, MultiBase<IsFirst, ProcessorType>> {};





}

template <typename... Processors> using chain = impl::ChainBase<true, Processors...>;

template <typename... Processors> struct split
{
	static constexpr bool isModulationSource = impl::SplitBase<true, Processors...>::isModulationSource;

	void initialise(NodeBase* n)
	{
		chain.initialise(n);
	}

	bool handleModulation(double& value)
	{
		return chain.handleModulation(value);
	}

	void process(ProcessData& data)
	{
		auto original = data.copyTo(splitBuffer, 0);
		chain.processWithCopy(data, original, splitBuffer);
	}

	void processSingle(float* frameData, int numChannels)
	{
		float originalData[NUM_MAX_CHANNELS];
		memcpy(originalData, frameData, sizeof(float)*numChannels);

		chain.processSingleWithOriginal(frameData, originalData, numChannels);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		DspHelpers::increaseBuffer(splitBuffer, numChannels * 2, blockSize);
		chain.prepare(numChannels, sampleRate, blockSize);
	}

	void reset()
	{
		chain.reset();
	}

	auto& getObject() { return chain.getObject(); }
	const auto& getObject() const { return chain.getObject(); }
	
	impl::SplitBase<true, Processors...> chain;
	AudioSampleBuffer splitBuffer;
};

template <typename... Processors> struct multi
{
	using Type = impl::MultiBase<true, Processors...>;
	static constexpr bool isModulationSource = Type::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void process(ProcessData& data)
	{
		ProcessData copy(data);

		obj.processWithFixedChannels(copy);
	}

	void processSingle(float* frameData, int)
	{
		obj.processSingleWithFixedChannels(&frameData);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		obj.prepare(numChannels, sampleRate, blockSize);
	}

	void reset()
	{
		obj.reset();
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	Type obj;
};

template <int T> struct IsZero
{
	static constexpr bool value = T == 0;
};

template <class SignalPath, class FeedbackPath> class feedback
{
public:

	static constexpr bool isModulationSource = false;

	void initialise(NodeBase* n)
	{
		s.initialise(n);
		f.initialise(n);
	}

	void prepare(int numChannelsToProcess, double sampleRate, int blockSize)
	{
		channelAmount = numChannelsToProcess;

		if (blockSize > 1)
			DspHelpers::increaseBuffer(feedbackBuffer, numChannelsToProcess, blockSize);
		
		s.prepare(numChannelsToProcess, sampleRate, blockSize);
		f.prepare(numChannelsToProcess, sampleRate, blockSize);

		reset();
	}

	void reset()
	{
		feedbackBuffer.clear();
		memset(singleData, 0, sizeof(float)* channelAmount);

		s.reset();
		f.reset();
	}

	void process(ProcessData& data)
	{
		auto fb = data.referTo(feedbackBuffer, 0);
		data += fb;
		s.process(data);
		data.copyTo(feedbackBuffer, 0);
		f.process(fb);
	}

	void processSingle(float* frameData, int numChannels)
	{
		for (int i = 0; i < numChannels; i++)
			frameData[i] += singleData[i];

		s.processSingle(frameData, numChannels);

		for (int i = 0; i < numChannels; i++)
			singleData[i] = frameData[i];

		f.processSingle(singleData, numChannels);
	}

	bool handleModulation(double& value)
	{
		return false;
	}

	auto& getObject() { return *this; }
	const auto& getObject() const { return *this; }

	auto& getNode(std::true_type)  { return s; }
	auto& getNode(std::false_type) { return f; }

	template <int arg> auto& get() noexcept 
	{ 
		return getNode(std::integral_constant<bool, IsZero<arg>::value>{});
	}

	SignalPath s;
	FeedbackPath f;

	int channelAmount;
	AudioSampleBuffer feedbackBuffer;
	float singleData[NUM_MAX_CHANNELS];
};


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
		modData.eventBuffer = data.eventBuffer;
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
