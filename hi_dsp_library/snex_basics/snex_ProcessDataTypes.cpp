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

namespace snex {
namespace Types {
using namespace juce;





template <int C>
snex::Types::FrameProcessor<C> snex::Types::ProcessData<C>::toFrameData()
{
	return FrameProcessor<C>(data, getNumSamples());
}

template <int C>
constexpr int snex::Types::ProcessData<C>::getNumChannels()
{
	return NumChannels;
}

template <int C>
constexpr int snex::Types::ProcessData<C>::getNumFixedChannels()
{
	return NumChannels;
}

template <int C>
ChannelPtr snex::Types::ProcessData<C>::getChannelPtr(int index)
{
	jassert(isPositiveAndBelow(index, NumChannels));
	return { data[index] };
}


template <int C> struct ProcessDataHelpers
{
	/** The number of channels. */
	constexpr static int NumChannels = C;

	/** @internal The internal channel data type.*/
	using ChannelDataType = Types::span<float*, NumChannels>;

	/** @internal. Used on the C++ side to create a ProcessData<> object from a consecutive float memory range. */
	template <typename T> static ChannelDataType makeChannelData(T& obj, int numSamples)
	{
		auto ptr = obj.begin();
		numSamples = getNumSamplesForConsequentData(obj, numSamples);

		ChannelDataType d;

		for (int i = 0; i < NumChannels; i++)
			d[i] = ptr + i * numSamples;

		return d;
	}

	/** @internal A helper function that creates the sample amount for a data array. */
	template <typename T> constexpr static int getNumSamplesForConsequentData(T& obj, int numSamples)
	{
		auto fullSamples = obj.size() / NumChannels;

		if (numSamples != -1)
		{
			jassert(isPositiveAndBelow(numSamples, fullSamples + 1));
			return numSamples;
		}
		
		return fullSamples;
	}

	template <typename OtherContainer> static void copyFrom(const ProcessData<C>& target, OtherContainer& s)
	{
		static_assert(std::is_same<float, typename OtherContainer::DataType>(), "target must be float array");

		auto src = target.begin();
		int numElements = target.getNumSamples();
		int numToCopy = numElements * NumChannels;

		auto targetPtrs = target.getRawDataPointers();

		for (int i = 0; i < NumChannels; i++)
		{
			auto offset = i * numElements;
			memcpy(targetPtrs[i], src + offset, sizeof(float)*numElements);
		}

		jassert(s.size() >= numToCopy); ignoreUnused(numToCopy);
	}

	template <typename OtherContainer> static void copyTo(const ProcessData<C>& source, OtherContainer& t)
	{
        static_assert(std::is_same<float, typename OtherContainer::DataType>(), "target must be float array");

		auto dst = t.begin();
		int numElements = source.getNumSamples();
		int numToCopy = numElements * NumChannels;

		const auto sourcePtrs = source.getRawDataPointers();

		for (int i = 0; i < NumChannels; i++)
		{
			auto offset = i * numElements;
			memcpy(dst + offset, sourcePtrs[i], sizeof(float)*numElements);
		}

        jassert(t.size() >= numToCopy); ignoreUnused(numToCopy);
	}
};

}
}
