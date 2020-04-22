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

	static const int NumChannels = C;
	static constexpr bool isModulationSource = T::isModulationSource;

	using FixProcessType = snex::Types::ProcessDataFix<NumChannels>;
	using FixFrameType = snex::Types::span<float, NumChannels>;

	fix() {};

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
		jassert(data.getNumChannels() == NumChannels);
		auto& fd = data.as<FixProcessType>();
		this->obj.process(fd);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		this->obj.handleHiseEvent(e);
	}

	template <typename FrameDataType> forcedinline void processFrame(FrameDataType& data) noexcept
	{
		jassert(numSamples == NumChannels);
		auto& d = *reinterpret_cast<FixFrameType*>(frameData);
		this->obj.processFrame(d);
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

	template <typename ProcessDataType> void process(ProcessDataType& ) noexcept
	{
		
	}

	void handleHiseEvent(HiseEvent& )
	{
		
	}

	template <typename FrameDataType> void processFrame(FrameDataType& ) noexcept
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
		// neu denken, mit event iterator...
		jassertfalse;

#if 0
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
#endif
	}

	HardcodedNode* getAsHardcodedNode() { return obj.getAsHardcodedNode(); }

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

	FORWARD_PROCESSDATA2FIX

	template <int C> void processFix(snex::Types::ProcessDataFix<C>& d)
	{
		auto fd = d.toFrameData();

		while (fd.next())
			obj.processFrame(fd);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
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

	using ProcessDataType = snex::Types::ProcessDataFix<NumChannels>;
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

	void process(ProcessDataType& data)
	{
		auto fd = data.toFrameData();

		while (fd.next())
			processFrame(fd);
	}

	forcedinline void process(ProcessData& data) noexcept
	{
		jassert(data.getNumChannels() == NumChannels);

		auto fd = data.toFrameData<NumChannels>();

		while (fd.next())
			processFrame(fd);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
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

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		// shouldn't be called since the oversampling always has to happen on block level...
		jassertfalse;
	}

	FORWARD_PROCESSDATA2FIX();

	template <int C> void processFix(snex::Types::ProcessDataFix<C>& c)
	{
		if (oversampler == nullptr)
			return;

		using FixProcessType = snex::Types::ProcessDataFix<C>;

		auto bl = d.toAudioBlock();
		auto output = oversampler->processSamplesUp(bl);

		FixProcessType::ChannelDataType data;

		for (int i = 0; i < C; i++)
			data[i] = output.getChannelPointer(i);

		FixProcessType od(data, d.getNumSamples() * OversamplingFactor);
		od.copyNonAudioDataFrom(c);
		obj.process(od);

		oversampler->processSamplesDown(bl);
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

	FORWARD_PROCESSDATA2FIX();

	template <int NumChannels> void processFix(snex::Types::ProcessDataFix<NumChannels>& d)
	{
		using FixProcessData = snex::Types::ProcessDataFix<NumChannels>;

		int numToDo = d.getNumSamples();

		if (numToDo < BlockSize)
		{
			this->obj.process(d);
		}
		else
		{
			FixProcessData::ChannelDataType cp;

			for (int i = 0; i < NumChannels; i++)
				cp[i] = d[i].data;

			while (numToDo > 0)
			{
				int numThisTime = jmin(BlockSize, numToDo);

				FixProcessData copy(d, cp, numThisTime);
				obj.process(copy);

				for (int i = 0; i < NumChannels; i++)
					cp[i] += numThisTime;

				numToDo -= copy.getNumSamples();
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

		DspHelpers::increaseBuffer(controlBuffer);

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

		jassert(numToProcess == controlBuffer.size());

		FloatVectorOperations::clear(controlBuffer.begin(), numToProcess);
		
		ProcessType::ChannelDataType cd = { controlBuffer.begin() };
		ProcessType md(data, cd);

		obj.process(md);
	}

	template <typename OtherFrameType> void processFrame(OtherFrameType& d)
	{
		if (--singleCounter <= 0)
		{
			auto& fd = FrameType::as(d.begin());

			singleCounter = HISE_EVENT_RASTER;
			float lastValue = 0.0f;
			obj.processFrame(fd);
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


template <class T, class ParameterClass> struct mod: public SingleWrapper<T>
{
	GET_SELF_AS_OBJECT(mod);

	constexpr static bool isModulationSource = false;
	constexpr static int NumChannels = T::NumChannels;

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		this->obj.process(data);

		double modValue = 0.0;

		if (this->obj.handleModulation(modValue))
			p.call(modValue);
	}

	forcedinline void reset() noexcept 
	{
		this->obj.reset();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);

		double modValue = 0.0;

		if (this->obj.handleModulation(modValue))
			p.call(modValue);
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

	template <int I, class T> void connect(T& t)
	{
		p.getParameter<0>().connect<I>(t);
	}

	ParameterClass p;
};

}





}
