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

		processor.processSingle(wb, numChannels);

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

	/** dummy overrides from HiseDspBase class. */
	HardcodedNode* getAsHardcodedNode() { return processor.getAsHardcodedNode(); }
	Component* createExtraComponent(PooledUIUpdater*) { return nullptr; }
	void createParameters(Array<HiseDspBase::ParameterData>&) {};
	bool isPolyphonic() const { return false; }

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

	void handleHiseEvent(HiseEvent& e)
	{
		Base::handleHiseEvent(e);
		processors.handleHiseEvent(e);
	}

	void handleHiseEventWithCopy(HiseEvent& e)
	{
		Base::handleHiseEventWithCopy(e);
		processors.handleHiseEventWithCopy(e);
	}

	void processSingle(float* frameData, int numChannels) noexcept
	{
		Base::processSingle(frameData, numChannels);
		processors.processSingle(frameData, numChannels);
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

	ChainBase<false, SubsequentProcessors...> processors;
};

template <bool IsFirst, typename ProcessorType> struct ChainBase<IsFirst, ProcessorType> : public ChainElement<IsFirst, ProcessorType, ChainBase<IsFirst, ProcessorType>> {};

#if 0
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

	void prepare(PrepareSpecs ps)
	{
		Base::prepare(ps);
		processors.prepare(ps);
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

#endif


#if 0
//==============================================================================
template <bool IsFirst, typename FirstProcessor, typename... SubsequentProcessors>
struct MultiBase : public ChainElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>
{
	using Base = ChainElement<IsFirst, FirstProcessor, MultiBase<IsFirst, FirstProcessor, SubsequentProcessors...>>;

	void prepare(PrepareSpecs ps)
	{
		Base::prepare(ps);
		processors.prepare(ps);
	}

	

	void handleHiseEvent(HiseEvent& e)
	{
		HiseEvent c(e);
		Base::handleHiseEvent(e);
		processors.handleHiseEvent(c);
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
#endif

}

template <typename... Processors> using chain = impl::ChainBase<true, Processors...>;

template <typename... Processors> using synth = wrap::synth<chain<Processors...>>;



template <typename... Processors> struct split
{
	using Type = chain<Processors...>;

	static constexpr bool isModulationSource = Type::isModulationSource;

	static constexpr bool isSingleNode = sizeof...(Processors) == 1;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (isSingleNode)
			obj.handleHiseEvent(e);
		else
			obj.handleHiseEventWithCopy(e);
	}

	void process(ProcessData& data)
	{
		if (isSingleNode)
			obj.process(data);
		else
		{
			auto original = data.copyTo(splitBuffer, 0);
			obj.processWithCopy(data, original, splitBuffer);
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (isSingleNode)
			obj.processSingle(frameData, numChannels);
		else
		{
			float originalData[NUM_MAX_CHANNELS];
			memcpy(originalData, frameData, sizeof(float)*numChannels);

			obj.processSingleWithOriginal(frameData, originalData, numChannels);
		}
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
		ps.numChannels *= 2;
		
		if(!isSingleNode)
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

template <typename... Processors> struct multi
{
	using Type = chain<Processors...>;
	static constexpr bool isModulationSource = Type::isModulationSource;

	void initialise(NodeBase* n)
	{
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
		ProcessData copy(data);

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

	void prepare(PrepareSpecs ps)
	{
		channelAmount = ps.numChannels;

		if (ps.blockSize > 1)
			DspHelpers::increaseBuffer(feedbackBuffer, ps);
		
		s.prepare(ps);
		f.prepare(ps);

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

	void handleHiseEvent(HiseEvent& e)
	{
		s.handleHiseEvent(e);
		f.handleHiseEvent(e);
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


}

}
