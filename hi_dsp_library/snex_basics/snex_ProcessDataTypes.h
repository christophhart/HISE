/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace Types {
using namespace juce;


/** The ChannelPtr is just a lightweight wrapper around the start of the channel data of a ProcessData object.
    @ingroup snex_data_structures

	It is returned by the iterator and can be used to create a dyn<float> object to iterate over the samples:
	
	@code{.cpp}
	void process(ProcessDataFix<2>& data)
	{
		for(auto & ch: data)
		{
			// pass the `ch` variable to the toChannelData() call and it will create
			// a proper dyn<float> (using the sample amount from the data object).
			for(auto& s: data.toChannelData(ch))
				s *= 0.5f;
		}
	}
	@endcode	
*/
struct ChannelPtr
{
	float* getRawWritePointer() { return d; }
	const float* getRawReadPointer() const { return d; };

private:

	/** @internal. The entire class can only be used internally by the ProcessDataFix class to create iteratable channels. */
	template <int C> friend struct ProcessData;
	friend struct InternalData;
	
	/** @internal. */
	float* get() const { return d; };

	/** @internal. */
	ChannelPtr(float* data) : d(data) {};

	/** @internal. */
	float* d = nullptr;
};


struct InternalData
{
protected:

	InternalData(float** d_, int size_) :
		data(d_),
		numSamples(size_)
	{};

public:

	/** Returns the amount of samples for this processing block. 
	    This value is guaranteed to be less than the `blockSize` value passed into the
		last `prepare()`
	*/
	int getNumSamples() const { return numSamples; }

	int getNumEvents() const { return numEvents; }

	/** Converts a ChannelPtr to a block. */
	block toChannelData(const ChannelPtr& channelStart) const
	{
		return dyn<float>(channelStart.get(), getNumSamples());
	}

	/** Creates a buffer of HiseEvents for the given chunk. */
	dyn<HiseEvent> toEventData() const
	{
		return { events, (size_t)numEvents };
	}

	const float** getRawDataPointers() const
	{
		return const_cast<const float**>(data);
	}

	float** getRawDataPointers()
	{
		return data;
	}

	template <typename ContainerType> void setEventBuffer(const ContainerType& b)
	{
		events = b.begin();
		numEvents = b.size();
	}

	template <typename T> T& as()
	{
		if constexpr (T::hasCompileTimeSize())
		{
			jassert(numChannels >= T::getNumFixedChannels());
		}

		return *static_cast<T*>(this);
	}

	void copyNonAudioDataFrom(const InternalData& other)
	{
		events = other.events;
		numEvents = other.numEvents;
	}

	AudioSampleBuffer toAudioSampleBuffer() const
	{
		return { data, numChannels, numSamples };
	}

	juce::dsp::AudioBlock<float> toAudioBlock() const
	{
		return { data, (size_t)numChannels, (size_t)numSamples };
	}

    float** getRawChannelPointers()
    {
        return data;
    }
    
protected:

	float** data;						// 8 bytes
	HiseEvent* events = nullptr;;		// 8 bytes

	int numSamples = 0;					// 4 bytes
	int numEvents = 0;					// 4 bytes
	int numChannels = 0;				// 4 byte
	
	JUCE_DECLARE_NON_COPYABLE(InternalData);
};


/** A data structure that contains the processing context for a DSP algorithm.
    @ingroup snex_data_structures

	It has a compile-time channel amount to that you can use channel loop iterators
	without performance overhead (because it will be most likely unrolled).

	This class is extremely lightweight to fit into 32 bytes and contains:

	- a pointer to the audio data
	- a pointer to the event data
	- the number of samples / events
	- a reset flag that can be used to control the execution of polyphonic voices

	It also provides a few tools to make the code as lean as possible by providing
	access to its data using one of the three toXXXData functions:

	- toChannelData() for channel based processing
	- toFrameData() that creates a FrameProcessor object for frame based processing
	- toEventData() that creates a dyn<HiseEvent> so you can process the events
*/
template <int C> struct ProcessData: public InternalData
{
	/** The number of channels. */
	constexpr static int NumChannels = C;

	/** @internal The internal channel data type.*/
	using ChannelDataType = Types::span<float*, NumChannels>;

	static constexpr bool hasCompileTimeSize() { return true; }

	/** Creates a ProcessDataFix object from the given data pointer. */
	ProcessData(float** d, int numSamples_, int numChannels_=NumChannels) :
		InternalData(d, numSamples_)
	{
		ignoreUnused(numChannels_);
		numChannels = NumChannels;
	}

	/** Allows iteration over the channel data. 
	
		The ChannelPtr return type can be passed into the `toChannelData()` method in order
		to create a `dyn<float>` object that can be used to iterate over the channel's
		sample data:

		@code
		ProcessData<2> data;

		for(auto& ch: data)
		{
			// Pass the `ch` iterator into the `toChannelData` function
			// in order to iterator over the float samples...
			for(float& s: data.toChannelData(ch))
			{
				s *= 0.5f;
			}
		}
		@endcode


	*/
	ChannelPtr* begin() const
	{
		return reinterpret_cast<ChannelPtr*>(data);
	}

	/** see begin(). */
	ChannelPtr* end() const
	{
		return reinterpret_cast<ChannelPtr*>(data + NumChannels);
	}
	
	/** creates a iteratable channel object for the . */
	dyn<float> operator[](int channelIndex)
	{
		jassert(isPositiveAndBelow(channelIndex, NumChannels));
		return dyn<float>(data[channelIndex], getNumSamples());
	}

	/** Generates a frame processor that allows frame-based iteration over all given channels. */
	FrameProcessor<C> toFrameData();

	bool isSilent() const
	{
		static_assert(getNumChannels() <= 2, "only allowed with stereo channels");

		if (numSamples == 0)
			return true;

		float* l = data[0];
		float* r = getNumChannels() == 2 ? data[1] : data[0];

		using SSEFloat = dsp::SIMDRegister<float>;

		auto alignedL = SSEFloat::getNextSIMDAlignedPtr(l);
		auto alignedR = SSEFloat::getNextSIMDAlignedPtr(r);

		auto numUnaligned = alignedL - l;
		auto numAligned = numSamples - numUnaligned;

		static const auto gain90dB = Decibels::decibelsToGain(-90.0f);

		while (--numUnaligned >= 0)
		{
			if (std::abs(*l++) > gain90dB || std::abs(*r++) > gain90dB)
				return false;
		}

		constexpr int sseSize = SSEFloat::SIMDRegisterSize / sizeof(float);

		while (numAligned >= sseSize)
		{
			auto l_ = SSEFloat::fromRawArray(alignedL);
			auto r_ = SSEFloat::fromRawArray(alignedR);

			auto sqL = SSEFloat::abs(l_);
			auto sqR = SSEFloat::abs(r_);

			auto max = SSEFloat::max(sqL, sqR).sum();

			if (max > gain90dB)
				return false;

			alignedL += sseSize;
			alignedR += sseSize;
			numAligned -= sseSize;
		}

		return true;
	}

	/** Gets the number of channels. */
	static constexpr int getNumChannels();
	
	constexpr static int getNumFixedChannels();

	ProcessData& toFixChannel()
	{
		return *this;
	}

    
    
protected:

	ChannelDataType& getChannelDataType()
	{
		return *reinterpret_cast<ChannelDataType*>(data);
	}

	ChannelPtr getChannelPtr(int index);

	JUCE_DECLARE_NON_COPYABLE(ProcessData);
};



struct ProcessDataDyn: public InternalData
{
	static constexpr bool hasCompileTimeSize() { return false; }

	constexpr static int getNumFixedChannels()
	{
		return NUM_MAX_CHANNELS;
	}

	/** Creates a ProcessDataFix object from the given data pointer. */
	ProcessDataDyn(float** d, int numSamples_, int numChannels_) :
		InternalData(d, numSamples_)
	{
		numChannels = numChannels_;

		jassert(numChannels <= NUM_MAX_CHANNELS);

	}


	int getNumChannels() const { return numChannels; };
	
	bool isSilent()
	{
		bool silent = true;

		if (getNumChannels() == 1)
		{
			return reinterpret_cast<ProcessData<1>*>(this)->isSilent();
		}
		else
		{
			for (int i = 0; i < getNumChannels() - 1; i += 2)
			{
				ProcessData<2> c(data + i, numSamples);

				silent &= c.isSilent();

				if (!silent)
					return false;
			}
		}

		

		return silent;
	}

	template <int NumChannels> FrameProcessor<NumChannels> toFrameData()
	{
		return FrameProcessor<NumChannels>(data, numSamples);
	}

	dyn<float> operator[](int channelIndex)
	{
		jassert(isPositiveAndBelow(channelIndex, numChannels));
		return dyn<float>(data[channelIndex], getNumSamples());
	}

	void referTo(float** newData, int newChannels, int newSamples)
	{
		data = newData;
		numChannels = newChannels;
		numSamples = newSamples;
	}

	ChannelPtr* begin() const
	{
		return reinterpret_cast<ChannelPtr*>(data);
	}

	ChannelPtr* end() const
	{
		return reinterpret_cast<ChannelPtr*>(data + numChannels);
	}
};



/** @internal This helper class takes a process data type and allows chunk-wise processing.

	Just create one of these from any process data type, then call
*/
template <typename ProcessDataType, bool IncludeEvents=true> struct ChunkableProcessData
{
	ChunkableProcessData(ProcessDataType& d) :
		numChannels(d.getNumChannels()),
		numSamples(d.getNumSamples()),
		events(d.toEventData())
	{
		memcpy(ptrs.begin(), d.getRawDataPointers(), sizeof(float*) * numChannels);
	}

	~ChunkableProcessData()
	{
		// You haven't used all samples. Don't forget to call getRemainder() at the end
		jassert(numSamples == 0);
	}

	/** Checks whether there are samples left. */
	operator bool() const { return getNumLeft() > 0; }

	/** Returns the amount of samples left. */
	int getNumLeft() const { return numSamples; }

	/** Returns the amount of samples already processed. */
	int getNumProcessed() const { return numProcessed; }

	/** @internal This helper method will create a ProcessDataType of the given size and 
		bump the pointers when it goes out of scope. 
		
		It also will refer to events in that range and adjust the timestamps 
		accordingly IF the IncludeEvents template parameter is true. 
	
	*/
	struct ScopedChunk
	{
		ScopedChunk(ChunkableProcessData& parent_, int numSamplesToSplice) :
			parent(parent_),
			sliced(parent.ptrs.begin(), numSamplesToSplice, parent.numChannels)
		{
			if (IncludeEvents && !parent.events.isEmpty())
			{
				int firstIndex = 0;
				int lastIndex = 0;

				int firstTimestamp = parent.numProcessed;
				int lastTimestamp = firstTimestamp + numSamplesToSplice;

				for (auto& e : parent.events)
				{
					auto eventTimestamp = e.getTimeStamp();

					if (eventTimestamp < firstTimestamp)
						firstIndex++;

					if (eventTimestamp < lastTimestamp)
						lastIndex++;
					else
						break;
				}

				int numEventsThisTime = lastIndex - firstIndex;

				if (numEventsThisTime != 0)
				{
					slicedEvents.referToRawData(parent.events.begin() + firstIndex, numEventsThisTime);

					sliced.setEventBuffer(slicedEvents);

					for (auto& e : slicedEvents)
						e.addToTimeStamp(-parent.numProcessed);
				}
			}

			jassert(isPositiveAndBelow(numSamplesToSplice-1, parent.numSamples));
		}

		~ScopedChunk()
		{
			if constexpr (ProcessDataType::hasCompileTimeSize())
			{
				for (auto& p : parent.ptrs)
					p += sliced.getNumSamples();
			}
			else
			{
				for (int i = 0; i < parent.numChannels; i++)
					parent.ptrs[i] += sliced.getNumSamples();
			}

			if (IncludeEvents)
			{
				for (auto& e : slicedEvents)
					e.addToTimeStamp(parent.numProcessed);
			}

			parent.numSamples -= sliced.getNumSamples();
			parent.numProcessed += sliced.getNumSamples();
		}

		ProcessDataType& toData()
		{
			return sliced;
		}

		ScopedChunk(ScopedChunk&& other):
			parent(other.parent),
			sliced(other.sliced.getRawDataPointers(), other.sliced.getNumSamples(), other.sliced.getNumChannels())
		{
			
		}

		

	private:

		ChunkableProcessData& parent;
		ProcessDataType sliced;
		dyn<HiseEvent> slicedEvents;
	};

	ScopedChunk getRemainder()
	{
		return ScopedChunk(*this, numSamples);
	}

	ScopedChunk getChunk(int numSamplesToSplice)
	{
		return ScopedChunk(*this, numSamplesToSplice);
	}

private:

	span<float*, ProcessDataType::getNumFixedChannels()> ptrs;

	dyn<HiseEvent> events;
	int numSamples = 0;
	int numProcessed = 0;
	const int numChannels = 0;
};

struct FrameConverters
{
	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrameMono(DspClass* ptr, ProcessDataType& data)
	{
		processFix<1>(ptr, data);
	}

    static void increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& specs)
    {
        auto numChannels = specs.numChannels;
        auto numSamples = specs.blockSize;
        
        if (numChannels != b.getNumChannels() ||
            b.getNumSamples() < numSamples)
        {
            b.setSize(numChannels, numSamples);
        }
    }
    
    static void increaseBuffer(snex::Types::heap<float>& b, const PrepareSpecs& specs)
    {
        auto numChannels = specs.numChannels;
        auto numSamples = specs.blockSize;
        auto numElements = numChannels * numSamples;
        
        if (numElements > b.size())
            b.setSize(numElements);
    }
    
	template <int NumChannels, typename DspClass, typename ProcessDataType> static void processFix(DspClass* ptr, ProcessDataType& data)
	{
		//jassert(data.getNumEvents() == 0);

		auto& obj = *static_cast<DspClass*>(ptr);
        auto& fixData = data.template as<snex::Types::ProcessData<NumChannels>>();

		auto fd = fixData.toFrameData();

		while (fd.next())
			obj.processFrame(fd.toSpan());
	}

	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrameStereo(DspClass* ptr, ProcessDataType& data)
	{
		switch (data.getNumChannels())
		{
		case 1:   processFix<1>(ptr, data); break;
		case 2:   processFix<2>(ptr, data); break;
        case 4:   processFix<4>(ptr, data); break;
		}
	}

	template <typename DspClass, typename FrameDataType> static forcedinline void forwardToFixFrame16(DspClass* ptr, FrameDataType& data)
	{
		static_assert(Helpers::isRefArrayType<FrameDataType>(), "unneeded call to forwardToFrameFix");

		switch (data.size())
		{
		case 1:		ptr->processFrame(span<float, 1>::as(data.begin())); break;
		case 2:		ptr->processFrame(span<float, 2>::as(data.begin())); break;
#if 0
		case 3:		ptr->processFrame(span<float, 3>::as(data.begin())); break;
		case 4:		ptr->processFrame(span<float, 4>::as(data.begin())); break;
		case 5:		ptr->processFrame(span<float, 5>::as(data.begin())); break;
		case 6:		ptr->processFrame(span<float, 6>::as(data.begin())); break;
		case 7:		ptr->processFrame(span<float, 7>::as(data.begin())); break;
		case 8:		ptr->processFrame(span<float, 8>::as(data.begin())); break;
		case 9:		ptr->processFrame(span<float, 9>::as(data.begin())); break;
		case 10:	ptr->processFrame(span<float, 10>::as(data.begin())); break;
		case 11:	ptr->processFrame(span<float, 11>::as(data.begin())); break;
		case 12:	ptr->processFrame(span<float, 12>::as(data.begin())); break;
		case 13:	ptr->processFrame(span<float, 13>::as(data.begin())); break;
		case 14:	ptr->processFrame(span<float, 14>::as(data.begin())); break;
		case 15:	ptr->processFrame(span<float, 15>::as(data.begin())); break;
		case 16:	ptr->processFrame(span<float, 16>::as(data.begin())); break;
#endif
		}
	}

	template <typename DspClass, typename ProcessDataType> static forcedinline void forwardToFrame16(DspClass* ptr, ProcessDataType& data)
	{
		switch (data.getNumChannels())
		{
		case 1:   processFix<1>(ptr, data); break;
		case 2:   processFix<2>(ptr, data); break;
#if 0
		case 3:   processFix<3>(ptr, data); break;
		case 4:   processFix<4>(ptr, data); break;
		case 5:   processFix<5>(ptr, data); break;
		case 6:   processFix<6>(ptr, data); break;
		case 7:   processFix<7>(ptr, data); break;
		case 8:   processFix<8>(ptr, data); break;
		case 9:   processFix<9>(ptr, data); break;
		case 10: processFix<10>(ptr, data); break;
		case 11: processFix<11>(ptr, data); break;
		case 12: processFix<12>(ptr, data); break;
		case 13: processFix<13>(ptr, data); break;
		case 14: processFix<14>(ptr, data); break;
		case 15: processFix<15>(ptr, data); break;
		case 16: processFix<16>(ptr, data); break;
#endif
		}
	}
};


}
}
