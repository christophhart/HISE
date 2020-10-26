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


/** The ChannelPtr is just a lightweight wrapper around the start of the channel data of a ProcessDataFix object. 

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

	void setResetFlag() noexcept
	{
		shouldReset = 1;
	}

	int getResetFlag() const noexcept
	{
		return shouldReset;
	}

	void setEventBuffer(const HiseEventBuffer& b)
	{
		events = b.begin();
		numEvents = b.getNumUsed();
	}

	template <typename T> T& as()
	{
		return *static_cast<T*>(this);
	}

	void copyNonAudioDataFrom(const InternalData& other)
	{
		events = other.events;
		numEvents = other.numEvents;
		shouldReset = other.shouldReset;
	}

	AudioSampleBuffer toAudioSampleBuffer() const
	{
		return { data, numChannels, numSamples };
	}

	juce::dsp::AudioBlock<float> toAudioBlock() const
	{
		return { data, (size_t)numChannels, (size_t)numSamples };
	}

protected:

	float** data;						// 8 bytes
	HiseEvent* events = nullptr;;		// 8 bytes

	int numSamples = 0;					// 4 bytes
	int numEvents = 0;					// 4 bytes
	int numChannels = 0;				// 4 byte
	int shouldReset = 0;				// 4 byte

	JUCE_DECLARE_NON_COPYABLE(InternalData);
};


/** A data structure that contains the processing context for a DSP algorithm. 

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

	/** Gets the number of channels. */
	static constexpr int getNumChannels();
	
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
	/** Creates a ProcessDataFix object from the given data pointer. */
	ProcessDataDyn(float** d, int numSamples_, int numChannels_) :
		InternalData(d, numSamples_)
	{
		numChannels = numChannels_;

		jassert(numChannels < NUM_MAX_CHANNELS);

	}


	int getNumChannels() const { return numChannels; };
	
	template <int NumChannels> FrameProcessor<NumChannels> toFrameData()
	{
		return FrameProcessor<NumChannels>(data, numSamples);
	}

	dyn<float> operator[](int channelIndex)
	{
		jassert(isPositiveAndBelow(channelIndex, numChannels));
		return dyn<float>(data[channelIndex], getNumSamples());
	}

	

	ChannelPtr* begin() const
	{
		return reinterpret_cast<ChannelPtr*>(data);
	}

	ChannelPtr* end() const
	{
		return reinterpret_cast<ChannelPtr*>(data + numChannels);
	}

	JUCE_DECLARE_NON_COPYABLE(ProcessDataDyn);
};

}
}