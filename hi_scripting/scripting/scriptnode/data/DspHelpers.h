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

struct VoiceData
{
	bool operator==(const VoiceData& other) const noexcept
	{
		return voiceIndex == other.voiceIndex && noteOnEvent == other.noteOnEvent;
	}

	int voiceIndex = -1;
	HiseEvent noteOnEvent;
};

struct PrepareSpecs
{
	int numChannels = 0;
	int blockSize = 0;
	double sampleRate = -1.0;
	int* voiceIndex = nullptr;
};

template <typename T, int NumVoices> struct PolyData
{
	PolyData(T initValue)
	{
		setAll(initValue);
	}

	PolyData()
	{
		setAll(T());
	}

	void prepare(PrepareSpecs sp)
	{
		jassert(sp.voiceIndex != nullptr);
		jassert(isPowerOfTwo(NumVoices));
		voicePtr = sp.voiceIndex;
	}

	static constexpr bool isPolyphonic() { return NumVoices > 1; }

	void setAll(const T& value)
	{
		if (!isPolyphonic())
		{
			*data = value;
		}
		else
		{
			for (int i = 0; i < NumVoices; i++)
				getWithIndex(i) = value;
		}
	}

	T& get()
	{
		if (!isPolyphonic())
			return *data;
		else
			return getWithIndex(getCurrentVoiceIndex());
	}

	const T& get() const
	{
		if (!isPolyphonic())
			return *data;
		else
			return getWithIndex(getCurrentVoiceIndex());
	}

	void forEachVoice(const std::function<void(T& v)>& f)
	{
		if (!isPolyphonic())
			f(*data);
		else
		{
			for (int i = 0; i < NumVoices; i++)
				f(getWithIndex(i));
		}
	}

	bool isVoiceRenderingActive() const
	{
		return isPolyphonic() && voicePtr != nullptr && *voicePtr != -1;
	}

	T& getCurrentOrFirst()
	{
		if (isVoiceRenderingActive())
			return get();
		else
			return *data;
	}

private:

	int getCurrentVoiceIndex() const
	{
		jassert(isPolyphonic());
		jassert(voicePtr != nullptr);
		jassert(isPositiveAndBelow(*voicePtr, NumVoices));
		return *voicePtr;
	}

	int getVoiceIndex(int index)
	{
		auto rv = index & (NumVoices - 1);
		return rv;
	}

	T& getWithIndex(int index)
	{
		return *(data + getVoiceIndex(index));
	}

	const T& getWithIndex(int index) const
	{
		return *(data + getVoiceIndex(index));
	}

	T data[NumVoices];
	int* voicePtr = nullptr;
};

/** A lightweight object containing the data. */
struct ProcessData
{
	struct SampleIterator
	{
		SampleIterator() :
			data(nullptr),
			size(0)
		{};

		SampleIterator(float* d, int s) :
			data(d),
			size(s)
		{};

		float* begin() const noexcept { return data; }
		float* end() const noexcept { return data + size; }
		float& operator[](int index) noexcept
		{
			jassert(isPositiveAndBelow(index, size));
			return *(data + index);
		}

		const float& operator[](int index) const noexcept
		{
			jassert(isPositiveAndBelow(index, size));
			return *(data + index);
		}

		int getSize() const noexcept { return size; }

	private:

		float* data;
		int size;
	};

	struct ChannelIterator
	{
		ChannelIterator(ProcessData& d):
			numChannels(d.numChannels)
		{
			for (int i = 0; i < d.numChannels; i++)
				iterators[i] = { d.data[i], d.size };
		};

		SampleIterator* begin() const { return const_cast<SampleIterator*>(iterators); }
		SampleIterator* end() const { return begin() + numChannels; }

		SampleIterator iterators[NUM_MAX_CHANNELS];
		int numChannels;
	};

	ProcessData(float** d, int c, int s) :
		data(d),
		numChannels(c),
		size(s),
		eventBuffer(nullptr)
	{};

	ProcessData(const ProcessData& other):
		data(other.data),
		numChannels (other.numChannels),
		size(other.size),
		eventBuffer(other.eventBuffer)
	{}
	
	ProcessData() {};

	float** data = nullptr;
	int numChannels = 0;
	int size = 0;
	HiseEventBuffer* eventBuffer = nullptr;
	bool shouldReset = false;

	ChannelIterator channels()
	{
		return ChannelIterator(*this);
	}

	/** Iterates over the channels. */
	float** begin() const { return data; }
	float** end() const { return data + numChannels; }

	template <int NumChannels> void copyToFrame(float* frame) const
	{
		jassert(isPositiveAndBelow(NumChannels, numChannels+1));

		for (int i = 0; i < NumChannels; i++)
			frame[i] = data[i][0];
	}

	template <int NumChannels> void copyFromFrameAndAdvance(const float* frame)
	{
		jassert(isPositiveAndBelow(NumChannels, numChannels + 1));

		for (int i = 0; i < NumChannels; i++)
		{
			data[i][0] = frame[i];
			data[i] += 1;
		}
	}

	void copyToFrameDynamic(float* frame) const
	{
		for (int i = 0; i < numChannels; i++)
			frame[i] = *data[i];
	}

	void copyFromFrameAndAdvanceDynamic(const float* frame)
	{
		for (int i = 0; i < numChannels; i++)
			*data[i]++ = frame[i];
	}

	void advanceChannelPointers(int sampleAmount=1)
	{
		for (int i = 0; i < numChannels; i++)
            data[i] += sampleAmount;
	}

	ProcessData copyToRawArray(float** channelData, float* uninitialisedData, bool clearArray=true)
	{
		for (int i = 0; i < numChannels; i++)
		{
			channelData[i] = uninitialisedData;
			
			if(clearArray)
				memset(channelData[i], 0, sizeof(float)*size);

			uninitialisedData += size;
		}

		ProcessData rd(channelData, numChannels, size);
		rd.eventBuffer = eventBuffer;
		return rd;
	}

	ProcessData copyTo(AudioSampleBuffer& buffer, int index);
	ProcessData& operator+=(const ProcessData& other);
	ProcessData referTo(AudioSampleBuffer& buffer, int index) const;
};

struct OscData
{
	void reset()
	{
		uptime = 0.0;
	}

	double tick()
	{
		auto rv = uptime;
		uptime += uptimeDelta;
		return rv;
	}

	double uptime = 0.0;
	double uptimeDelta = 0.0;
};

struct ModValue
{
	bool getChangedValue(double& d)
	{
		if (changed)
		{
			changed = false;
			d = modValue;
			return true;
		}

		return false;
	}

	double getModValue() const noexcept { return modValue; }

	void setModValue(double newValue)
	{
		modValue = newValue;
		changed = true;
	}

private:

	bool changed = false;
	double modValue = 0.0;
};

struct DspHelpers
{
	/** Increases the buffer size to match the process specs. */
	static void increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& ps);

	using ConverterFunction = std::function<double(double)>;
	using ParameterCallback = std::function<void(double)>;

	/** Returns a ParameterCallback with the given range. */
	static ParameterCallback getFunctionFrom0To1ForRange(NormalisableRange<double> range, bool inverted, const ParameterCallback& originalFunction);

	/** Wraps the ParameterCallback into a conversion function based on the converterId. */
	static ParameterCallback wrapIntoConversionLambda(const Identifier& converterId,
		const ParameterCallback& originalFunction,
		NormalisableRange<double> range,
		bool inverted);

	static double findPeak(const ProcessData& data);
};



}
