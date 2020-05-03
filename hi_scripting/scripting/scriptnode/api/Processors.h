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


template <int C, class T> class fix
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	static const int NumChannels = C;

	using FixProcessType = snex::Types::ProcessDataFix<NumChannels>;
	using FixFrameType = snex::Types::span<float, NumChannels>;

	fix() {};

	constexpr bool isPolyphonic() const { return obj.getWrappedObject().isPolyphonic(); }

	static Identifier getStaticId() { return T::getStaticId(); }

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = NumChannels;
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		jassert(data.getNumChannels() >= NumChannels);
		auto& fd = data.as<FixProcessType>();
		this->obj.process(fd);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		this->obj.handleHiseEvent(e);
	}

	template <typename FrameDataType> forcedinline void processFrame(FrameDataType& data) noexcept
	{
		jassert(data.size() >= NumChannels);
		auto& d = FixFrameType::as(data.begin());
		this->obj.processFrame(d);
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data)
	{
		this->obj.createParameters(data);
	}

private:

	T obj;
};




template <class T> class skip
{
public:

	GET_SELF_OBJECT(obj);
	GET_WRAPPED_OBJECT(obj);

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs )
	{
		
	}

	void reset() noexcept {  }

	template <typename ProcessDataType> void process(ProcessDataType& ) noexcept
	{
		
	}

	void handleHiseEvent(HiseEvent& )
	{
		
	}

	template <typename FrameDataType> void processFrame(FrameDataType& ) noexcept
	{
		
	}

private:

	T obj;
};


namespace wrap
{

template <class T> class event
{
public:

	static constexpr bool isModulationSource = false;

	bool isPolyphonic() const { return obj.isPolyphonic(); }

	Component* createExtraComponent(PooledUIUpdater* updater) { return nullptr; };
	int getExtraWidth() const { return 0; };
	int getExtraHeight() const { return 0; };

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto events = d.toEventData();

		if (events.size() > 0)
		{
			float* ptrs[NUM_MAX_CHANNELS];
			int numChannels = d.getNumChannels();
			memcpy(ptrs, d.getRawDataPointers(), sizeof(float*) * numChannels);

			int lastPos = 0;
			int numLeft = d.getNumSamples();
			
			for (auto& e : events)
			{
				if (e.isIgnored())
					continue;

				auto samplePos = e.getTimeStamp();
				const int numThisTime = jmin(numLeft, samplePos - lastPos);

				obj.handleHiseEvent(e);

				if (numThisTime > 0)
				{
					ProcessDataType part(ptrs, numThisTime, d.getNumChannels());
					obj.process(part);
					
					for (int i = 0; i < d.getNumChannels(); i++)
						ptrs[i] += numThisTime;
				}

				numLeft = jmax(0, numLeft - numThisTime);
				lastPos = samplePos;
			}
			
			if (numLeft > 0)
			{
				ProcessDataType part(ptrs, numLeft, numChannels);
				obj.process(part);
			}
		}
		else
		{
			obj.process(d);
		}

		


		
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

	bool handleModulation(double& value) noexcept { return false; }

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	T obj;
};


template <class T> class frame_x
{
public:

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		DspHelpers::forwardToFrame16(&obj, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};

template <int NumChannels, class T> class frame
{
public:

	GET_SELF_OBJECT(obj);
	GET_WRAPPED_OBJECT(obj);

	using FixProcessType = snex::Types::ProcessDataFix<NumChannels>;
	using FrameType = snex::Types::span<float, NumChannels>;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = NumChannels;
		obj.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		snex::Types::ProcessDataHelpers<NumChannels>::processFix(&obj, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

private:

	T obj;
};




template <int OversamplingFactor, class T> class oversample
{
public:

	using Oversampler = juce::dsp::Oversampling<float>;

	void prepare(PrepareSpecs ps)
	{
		jassert(lock != nullptr);

		ScopedPointer<Oversampler> newOverSampler;

		auto originalBlockSize = ps.blockSize;

		ps.sampleRate *= (double)OversamplingFactor;
		ps.blockSize *= OversamplingFactor;

		obj.prepare(ps);

		newOverSampler = new Oversampler(ps.numChannels, (int)std::log2(OversamplingFactor), Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		if (originalBlockSize > 0)
			newOverSampler->initProcessing(originalBlockSize);

		{
			ScopedLock sl(*lock);
			oversampler.swapWith(newOverSampler);
		}
	}

	forcedinline void reset() noexcept 
	{
		if (oversampler != nullptr)
			oversampler->reset();

		obj.reset(); 
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		// shouldn't be called since the oversampling always has to happen on block level...
		jassertfalse;
	}

	template <typename ProcessDataType>  void process(ProcessDataType& data)
	{
		if (oversampler == nullptr)
			return;

		auto bl = data.toAudioBlock();
		auto output = oversampler->processSamplesUp(bl);

		float* tmp[NUM_MAX_CHANNELS];

		for (int i = 0; i < data.getNumChannels(); i++)
			tmp[i] = output.getChannelPointer(i);

		ProcessDataType od(tmp, data.getNumSamples() * OversamplingFactor, data.getNumChannels());

		od.copyNonAudioDataFrom(data);
		obj.process(od);

		oversampler->processSamplesDown(bl);
	}

	void initialise(NodeBase* n)
	{
		lock = &n->getRootNetwork()->getConnectionLock();
		obj.initialise(n);
	}

	const auto& getObject() const { return obj.getObject(); }
	auto& getObject() { return obj.getObject(); }

private:

	CriticalSection* lock = nullptr;

	ScopedPointer<Oversampler> oversampler;
	T obj;
};

template <class T, int BlockSize> class fix_block
{
public:

	constexpr static bool isModulationSource = false;

	const auto& getObject() const { return obj.getObject(); }
	auto& getObject() { return obj.getObject(); }

	fix_block() {};

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.blockSize = BlockSize;
		obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		int numToDo = data.getNumSamples();

		if (numToDo < BlockSize)
		{
			this->obj.process(data);
		}
		else
		{
			float* tmp[NUM_MAX_CHANNELS];

			for (int i = 0; i < data.getNumChannels(); i++)
				tmp[i] = data[i].data;

			while (numToDo > 0)
			{
				int numThisTime = jmin(BlockSize, numToDo);

				ProcessDataType copy(tmp, numThisTime, data.getNumChannels());
				copy.copyNonAudioDataFrom(data);

				obj.process(copy);

				for (int i = 0; i < data.getNumChannels(); i++)
					tmp[i] += numThisTime;

				numToDo -= numThisTime;
			}
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		// this node should never be called per frame...
		jassertfalse;
	}

	bool handleModulation(double& v)
	{
		return obj.handleModulation(v);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

private:

	T obj;
};


template <class T> class control_rate
{
public:

	using FrameType = snex::Types::span<float, 1>;
	using ProcessType = snex::Types::ProcessDataFix<1>;

	constexpr static bool isModulationSource = false;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.sampleRate /= (double)HISE_EVENT_RASTER;
		ps.blockSize /= HISE_EVENT_RASTER;
		ps.numChannels = 1;

		DspHelpers::increaseBuffer(controlBuffer, ps);

		this->obj.prepare(ps);
	}

	void reset()
	{
		obj.reset();
		singleCounter = 0;
	}

	void process(ProcessType& data)
	{
		int numToProcess = data.getNumSamples() / HISE_EVENT_RASTER;

		jassert(numToProcess <= controlBuffer.size());

		FloatVectorOperations::clear(controlBuffer.begin(), numToProcess);
		
		float* d[1] = { controlBuffer.begin() };

		ProcessDataFix<1> md(d, numToProcess, 1);
		md.copyNonAudioDataFrom(data);

		obj.process(md);
	}

	// must always be wrapped into a fix<1> node...
	void processFrame(FrameType& d)
	{
		if (--singleCounter <= 0)
		{
			singleCounter = HISE_EVENT_RASTER;
			float lastValue = 0.0f;
			obj.processFrame(d);
		}
	}

	bool handleModulation(double& )
	{
		return false;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	T obj;
	int singleCounter = 0;

	snex::Types::heap<float> controlBuffer;
};


template <class T, class ParameterClass> struct mod
{
	GET_SELF_OBJECT(*this);
	GET_WRAPPED_OBJECT(this->obj.getWrappedObject());

	constexpr static bool isModulationSource = false;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		this->obj.process(data);
		checkModValue();
	}

	void reset() noexcept 
	{
		this->obj.reset();
		checkModValue();
	}

	void checkModValue()
	{
		double modValue = 0.0;

		if (this->obj.handleModulation(modValue))
			p.call(modValue);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
		checkModValue();
	}

	inline void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
		checkModValue();
	}

	bool isPolyphonic() const
	{
		return obj.isPolyphonic();
	}

	void prepare(PrepareSpecs ps)
	{
		this->obj.prepare(ps);
	}

	bool handleModulation(double& value) noexcept
	{
		return false;
	}

	template <int I, class T> void connect(T& t)
	{
		p.getParameter<0>().connect<I>(t);
	}

	ParameterClass p;
	T obj;
};

}





}
