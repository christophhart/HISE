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

namespace init
{
struct oversample;
}

using namespace snex::Types;



/** The wrap namespace contains templates that wrap around an existing node type and change the processing.

	Examples are:

	- fixed channel amount: wrap::fix
	- fixed upper block size: wrap::fix_block
	- frame based processing: wrap::frame_x

	Most of the templates just hold an object of the given type and forward most callbacks to its inner object
	while adding the additional functionality where required.
*/
namespace wrap
{



/** This namespace holds all the logic of custom node wrappers in static functions so it can be used
	by the SNEX jit compiler as well as the C++ classes.

*/
namespace static_functions
{

namespace prototypes
{
template <typename ProcessDataType> using process = void(*)(void*, ProcessDataType&);
typedef void(*handleHiseEvent)(void*, HiseEvent&);
}

template <int BlockSize> struct fix_block
{
	static void prepare(void* obj, void* functionPointer, PrepareSpecs ps)
	{
		auto typedFunction = (void(*)(void*, PrepareSpecs))functionPointer;
		ps.blockSize = BlockSize;
		typedFunction(obj, ps);
	}

	template <typename ProcessDataType> static void process(void* obj, void* functionPointer, ProcessDataType& data)
	{
		auto typedFunction = (void(*)(void*, ProcessDataType& d))(functionPointer);
		int numToDo = data.getNumSamples();

		if (numToDo < BlockSize)
			typedFunction(obj, data);
		else
		{
			// We need to forward the HiseEvents to the chunks as there might
			// be a event node sitting in there...
			static constexpr bool IncludeHiseEvents = true;

			ChunkableProcessData<ProcessDataType, IncludeHiseEvents> cpd(data);

			while (cpd)
			{
				int numThisTime = jmin(BlockSize, cpd.getNumLeft());
				auto c = cpd.getChunk(numThisTime);
				typedFunction(obj, c.toData());
			}

#if 0
			float* tmp[ProcessDataType::getNumFixedChannels()];

			for (int i = 0; i < data.getNumChannels(); i++)
				tmp[i] = data[i].data;

			while (numToDo > 0)
			{
				int numThisTime = jmin(BlockSize, numToDo);
				ProcessDataType copy(tmp, numThisTime, data.getNumChannels());
				copy.copyNonAudioDataFrom(data);
				typedFunction(obj, copy);

				for (int i = 0; i < data.getNumChannels(); i++)
					tmp[i] += numThisTime;

				numToDo -= numThisTime;
			}
#endif
		}
	}
};



/** Continue... */
struct event
{
	template <typename ProcessDataType> static void process(void* obj, prototypes::process<ProcessDataType> pf, prototypes::handleHiseEvent ef, ProcessDataType& d)
	{
		auto events = d.toEventData();

		if (events.size() > 0)
		{
			// We don't need the event in child nodes as it should call 
			// handleHiseEvent recursively from here...
			static constexpr bool IncludeHiseEvents = false;

			ChunkableProcessData<ProcessDataType, IncludeHiseEvents> aca(d);

			int lastPos = 0;

			for (auto& e : events)
			{
				if (e.isIgnored())
					continue;

				auto samplePos = e.getTimeStamp();

				const int numThisTime = jmin(aca.getNumLeft(), samplePos - lastPos);

				if (numThisTime > 0)
				{
					auto c = aca.getChunk(numThisTime);
					pf(obj, c.toData());
				}

				ef(obj, e);
				lastPos = samplePos;
			}

			if (aca)
			{
				auto c = aca.getRemainder();
				pf(obj, c.toData());
			}
		}
		else
		{
			pf(obj, d);
		}
	}
};

}



/** This wrapper template will create a compile-time channel configuration which might
	increase the performance and inlineability.

	Usage:

	@code
	using stereo_oscillator = fix<2, core::oscillator>
	@endcode

	If you wrap a node into this template, it will call the process functions with
	the fixed size process data arguments:

	void process(ProcessData<C>& data)

	void processFrame(span<float, C>& data)

	instead of the dynamic ones.
*/
template <int C, class T> class fix
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	static const int NumChannels = C;

	/** The process data type to use for this node. */
	using FixProcessType = snex::Types::ProcessData<NumChannels>;

	/** The frame data type to use for this node. */
	using FixFrameType = snex::Types::span<float, NumChannels>;

	fix() {};

	constexpr bool isPolyphonic() const { return this->obj.getWrappedObject().isPolyphonic(); }

	static Identifier getStaticId() { return T::getStaticId(); }

	/** Forwards the callback to its wrapped object. */
	void initialise(NodeBase* n)
	{
		this->obj.initialise(n);
	}

	/** Forwards the callback to its wrapped object, but it will change the channel amount to NumChannels. */
	void prepare(PrepareSpecs ps)
	{
		ps.numChannels = NumChannels;
		this->obj.prepare(ps);
	}

	/** Forwards the callback to its wrapped object. */
	forcedinline void reset() noexcept { obj.reset(); }

	/** Forwards the callback to its wrapped object. */
	void handleHiseEvent(HiseEvent& e)
	{
		this->obj.handleHiseEvent(e);
	}

	/** Processes the given data, but will cast it to the fixed channel version.

		You must not call this function with a ProcessDataType that has less channels
		or the results will be unpredictable.

		Leftover channels will not be processed.
	*/
	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		jassert(data.getNumChannels() >= NumChannels);
		auto& fd = data.as<FixProcessType>();
		this->obj.process(fd);
	}


	/** Processes the given data, but will cast it to the fixed channel version.

		You must not call this function with a FrameDataType that has less channels
		or the results will be unpredictable.

		Leftover channels will not be processed.
	*/
	template <typename FrameDataType> forcedinline void processFrame(FrameDataType& data) noexcept
	{
		jassert(data.size() >= NumChannels);
		auto& d = FixFrameType::as(data.begin());
		this->obj.processFrame(d);
	}

	/** Forwards the callback to its wrapped object. */
	void createParameters(ParameterDataList& data)
	{
		this->obj.createParameters(data);
	}

private:

	T obj;
};


/** Wrapping a node into this template will bypass all callbacks.

	You can use it to quickly deactivate a node (or the code generator might use this
	in order to generate nodes that are bypassed.

*/
template <class T> class skip
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	void initialise(NodeBase* n)
	{
		this->obj.initialise(n);
	}

	void prepare(PrepareSpecs) {}
	void reset() noexcept {}

	template <typename ProcessDataType> void process(ProcessDataType&) noexcept {}

	void handleHiseEvent(HiseEvent&) {}

	template <typename FrameDataType> void processFrame(FrameDataType&) noexcept {}

private:

	T obj;
};

#define HISE_DEFAULT_PREPARE(ObjectType) void prepare(PrepareSpecs ps) { obj.prepare(ps); }
#define HISE_DEFAULT_PROCESS_FRAME(ObjectType) template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept { this->obj.processFrame(data); }
#define INTERNAL_EVENT_FUNCTION(ObjectClass) static void handleHiseEventInternal(void* obj, HiseEvent& e) { auto& typed = *static_cast<ObjectClass*>(obj); typed.handleHiseEvent(e); }

template <class T> class event
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	bool isPolyphonic() const { return obj.isPolyphonic(); }

	HISE_DEFAULT_INIT(T);
	HISE_DEFAULT_PREPARE(T);
	HISE_DEFAULT_RESET(T);
	HISE_DEFAULT_HANDLE_EVENT(T);
	HISE_DEFAULT_PROCESS_FRAME(T);

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto p = [](void* obj, ProcessDataType& data) { static_cast<event*>(obj)->obj.process(data); };
		auto e = [](void* obj, HiseEvent& e) { static_cast<event*>(obj)->obj.handleHiseEvent(e); };
		static_functions::event::process<ProcessDataType>(this, p, e, data);
	}

	T obj;

private:
};


template <class T> class frame_x
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

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
		constexpr int C = ProcessDataType::getNumFixedChannels();

		if (ProcessDataType::isFixedChannel)
			FrameConverters::processFix<C>(&obj, data);
		else
			FrameConverters::forwardToFrame16(&obj, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		this->obj.processFrame(data);
	}

private:

	T obj;
};

/** Just a shortcut to the frame_x type. */
template <int C, typename T> using frame = fix<C, frame_x<T>>;

#if 0
template <int NumChannels, class T> class frame
{
public:

	GET_SELF_OBJECT(obj);
	GET_WRAPPED_OBJECT(obj);

	using FixProcessType = snex::Types::ProcessData<NumChannels>;
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
#endif



struct oversample_base
{
	using Oversampler = juce::dsp::Oversampling<float>;

	oversample_base(int factor) :
		oversamplingFactor(factor)
	{};

	void prepare(PrepareSpecs ps)
	{
		jassert(lock != nullptr);

		ScopedPointer<Oversampler> newOverSampler;

		auto originalBlockSize = ps.blockSize;

		ps.sampleRate *= (double)oversamplingFactor;
		ps.blockSize *= oversamplingFactor;

		if (pCallback)
			pCallback(ps);
		
		newOverSampler = new Oversampler(ps.numChannels, (int)std::log2(oversamplingFactor), Oversampler::FilterType::filterHalfBandPolyphaseIIR, false);

		if (originalBlockSize > 0)
			newOverSampler->initProcessing(originalBlockSize);

		{
			ScopedLock sl(*lock);
			oversampler.swapWith(newOverSampler);
		}
	}

	CriticalSection* lock = nullptr;

protected:

	const int oversamplingFactor = 0;
	std::function<void(PrepareSpecs)> pCallback;
	ScopedPointer<Oversampler> oversampler;
};





template <int OversamplingFactor, class T, class InitFunctionClass=init::oversample> class oversample: public oversample_base
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	oversample():
		oversample_base(OversamplingFactor)
	{
		pCallback = [this](PrepareSpecs ps)
		{
			obj.prepare(ps);
		};
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
		InitFunctionClass::initialise(this, n);
		obj.initialise(n);
	}

private:

	T obj;
};


template <int BlockSize, class T> class fix_block
{
public:

	GET_SELF_OBJECT(obj.getObject());
	GET_WRAPPED_OBJECT(obj.getWrappedObject());

	fix_block() {};

	void prepare(PrepareSpecs ps)
	{
		static_functions::fix_block<BlockSize>::prepare(this, fix_block::prepareInternal, ps);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		static_functions::fix_block<BlockSize>::process(this, fix_block::processInternal<ProcessDataType>, data);
	}

	HISE_DEFAULT_INIT(T);
	HISE_DEFAULT_RESET(T);
	HISE_DEFAULT_MOD(T);
	HISE_DEFAULT_HANDLE_EVENT(T);

private:

	INTERNAL_PREPARE_FUNCTION(T);
	INTERNAL_PROCESS_FUNCTION(T);

	T obj;
};


/** Downsamples the incoming signal with the HISE_EVENT_RASTER value
    (default is 8x) and processes a mono signal that can be used as
    modulation signal.
*/
template <class T> class control_rate
{
public:

	using FrameType = snex::Types::span<float, 1>;
	using ProcessType = snex::Types::ProcessData<1>;

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

		ProcessData<1> md(d, numToProcess, 1);
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
