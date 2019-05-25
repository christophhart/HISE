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
		size(s)
	{};

	ProcessData() {};

	float** data = nullptr;
	int numChannels = 0;
	int size = 0;

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
			frame[i] = *data[i];
	}

	template <int NumChannels> void copyFromFrameAndAdvance(const float* frame)
	{
		jassert(isPositiveAndBelow(NumChannels, numChannels + 1));

		for (int i = 0; i < NumChannels; i++)
			*data[i]++ = frame[i];
	}

	ProcessData copyTo(AudioSampleBuffer& buffer, int index);
	ProcessData& operator+=(const ProcessData& other);
	ProcessData referTo(AudioSampleBuffer& buffer, int index);
};


struct DspHelpers
{
	/** Increases the buffer size to match the process specs. */
	static void increaseBuffer(AudioSampleBuffer& b, int numChannels, int numSamples);

	using ConverterFunction = std::function<double(double)>;
	using ParameterCallback = std::function<void(double)>;

	/** Returns a ParameterCallback with the given range. */
	static ParameterCallback getFunctionFrom0To1ForRange(NormalisableRange<double> range, bool inverted, const ParameterCallback& originalFunction);

	/** Wraps the ParameterCallback into a conversion function based on the converterId. */
	static ParameterCallback wrapIntoConversionLambda(const Identifier& converterId,
		const ParameterCallback& originalFunction,
		NormalisableRange<double> range,
		bool inverted);

	static double findPeak(ProcessData& data);
};



}
