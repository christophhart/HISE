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


template <int NumChannels, class T> class fix
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = getFixChannelAmount();
		obj.prepare(ps);
	}

	forcedinline void reset() noexcept { obj.reset(); }

	forcedinline void process(ProcessData& data) noexcept
	{
		auto dCopy = ProcessData(data);
		dCopy.numChannels = getFixChannelAmount();

		obj.process(dCopy);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	constexpr static int getFixChannelAmount()
	{
		return NumChannels;
	}

	forcedinline void processSingle(float* frameData, int ) noexcept
	{
		obj.processSingle(frameData, getFixChannelAmount());
	}

	forcedinline bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};

template <class T> class skip
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

	void initialise(NodeBase* n)
	{
		obj.initialise(n);
	}

	void prepare(PrepareSpecs )
	{
		
	}

	forcedinline void reset() noexcept {  }

	forcedinline void process(ProcessData& ) noexcept
	{
		
	}

	void handleHiseEvent(HiseEvent& )
	{
		
	}

	forcedinline void processSingle(float* , int) noexcept
	{
		
	}

	forcedinline bool handleModulation(double& ) noexcept
	{
		return false;
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};


namespace wrap
{

template <class T> class event
{
public:

	static constexpr bool isModulationSource = false;

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

	void process(ProcessData& d)
	{
		if (d.eventBuffer != nullptr && !d.eventBuffer->isEmpty())
		{
			float* ptrs[NUM_MAX_CHANNELS];
			int numChannels = d.numChannels;
			memcpy(ptrs, d.data, sizeof(float*) * numChannels);

			auto advancePtrs = [numChannels](float** dt, int numSamples)
			{
				for (int i = 0; i < numChannels; i++)
					dt[i] += numSamples;
			};

			HiseEventBuffer::Iterator iter(*d.eventBuffer);

			int lastPos = 0;
			int numLeft = d.size;
			int samplePos;
			HiseEvent e;

			while (iter.getNextEvent(e, samplePos, true, false))
			{
				int numThisTime = jmin(numLeft, samplePos - lastPos);

				obj.handleHiseEvent(e);

				if (numThisTime > 0)
				{
					ProcessData part(ptrs, numChannels, numThisTime);
					obj.process(part);
					advancePtrs(ptrs, numThisTime);
					numLeft = jmax(0, numLeft - numThisTime);
					lastPos = samplePos;
				}
			}

			if (numLeft > 0)
			{
				ProcessData part(ptrs, numChannels, numLeft);
				obj.process(part);
			}
		}
		else
			obj.process(d);
	}

	void processSingle(float* frameData, int numChannels)
	{
		obj.processSingle(frameData, numChannels);
	}

	bool handleModulation(double& value) noexcept { return false; }

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

	T obj;
};


template <class T> class frame_x
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

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

	forcedinline void process(ProcessData& data) noexcept
	{
		int numToDo = data.size;
		float* frame = (float*)alloca(data.numChannels * sizeof(float));
		float** frameData = (float**)alloca(data.numChannels * sizeof(float*));
		memcpy(frameData, data.data, sizeof(float*)*data.numChannels);
		ProcessData copy(frameData, data.numChannels, data.size);

		while (--numToDo >= 0)
		{
			copy.copyToFrameDynamic(frame);
			processSingle(frame, data.numChannels);
			copy.copyFromFrameAndAdvanceDynamic(frame);
		}
	}

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		obj.processSingle(frameData, numChannels);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};

template <int NumChannels, class T> class frame
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

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

	forcedinline void process(ProcessData& data) noexcept
	{
		jassert(data.numChannels == NumChannels);

		int numToDo = data.size;
		float frame[NumChannels];
		float* frameData[NumChannels];
		memcpy(frameData, data.data, sizeof(float*)*NumChannels);
		ProcessData copy(frameData, NumChannels, data.size);
		copy.allowPointerModification();

		while (--numToDo >= 0)
		{
			copy.copyToFrame<NumChannels>(frame);
			processSingle(frame, NumChannels);
			copy.copyFromFrameAndAdvance<NumChannels>(frame);
		}
	}

	forcedinline void processSingle(float* frameData, int ) noexcept
	{
		obj.processSingle(frameData, NumChannels);
	}

	bool handleModulation(double& value)
	{
		return obj.handleModulation(value);
	}

	auto& getObject() { return obj.getObject(); }
	const auto& getObject() const { return obj.getObject(); }

private:

	T obj;
};




template <int OversamplingFactor, class T> class oversample
{
public:

	static constexpr bool isModulationSource = T::isModulationSource;

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

	forcedinline void processSingle(float* frameData, int numChannels) noexcept
	{
		// Applying oversampling on frame basis is stupid.
		jassertfalse;
	}

	forcedinline void process(ProcessData& d) noexcept
	{
		if (oversampler == nullptr)
			return;

		juce::dsp::AudioBlock<float> input(d.data, d.numChannels, d.size);

		auto output = oversampler->processSamplesUp(input);

		float* data[NUM_MAX_CHANNELS];

		for (int i = 0; i < d.numChannels; i++)
			data[i] = output.getChannelPointer(i);

		ProcessData od;
		od.data = data;
		od.numChannels = d.numChannels;
		od.size = d.size * OversamplingFactor;

		obj.process(od);

		oversampler->processSamplesDown(input);
	}

	bool handleModulation(double& value) noexcept
	{
		return obj.handleModulation(value);
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

template <class T> class control_rate
{
public:

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
		this->obj.prepare(ps);
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

	void processSingle(float* , int )
	{
		if (--singleCounter <= 0)
		{
			singleCounter = HISE_EVENT_RASTER;
			float lastValue = 0.0f;
			obj.processSingle(&lastValue, 1);
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
};


template <class T> struct mod: public SingleWrapper<T>
{
	GET_SELF_AS_OBJECT(mod);

	constexpr static bool isModulationSource = false;

	inline void process(ProcessData& data) noexcept
	{
		this->obj.process(data);

		if (this->obj.handleModulation(modValue))
			db(modValue);
	}

	forcedinline void reset() noexcept 
	{
		modValue = 0.0;
		this->obj.reset();
	}

	inline void processSingle(float* frameData, int numChannels) noexcept
	{
		this->obj.processSingle(frameData, numChannels);

		if (this->obj.handleModulation(modValue))
			db(modValue);
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data) override
	{
		this->obj.createParameters(data);
	}

	void prepare(PrepareSpecs ps)
	{
		this->obj.prepare(ps);
	}

	bool handleModulation(double& value) noexcept
	{
		return false;
	}

	void setCallback(const DspHelpers::ParameterCallback& c)
	{
		db = c;
	}

	//auto& getObject() { return obj.getObject(); }
	//const auto& getObject() const { return obj.getObject(); }

	DspHelpers::ParameterCallback db;
	double modValue = 0.0;
};

}





}
