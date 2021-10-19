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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SAMPLEBUFFER_H_INCLUDED
#define SAMPLEBUFFER_H_INCLUDED

namespace hlac {

typedef CompressionHelpers::AudioBufferInt16 FixedSampleBuffer;

template <typename ElementType, int PreallocatedSize> class OptionalDynamicArray
{
public:

	OptionalDynamicArray()
	{
		dataPtr = preallocated;
	};

	void ensureStorageAllocated(int minNumElements)
	{
		if (numAllocated >= minNumElements)
			return;

		if (minNumElements <= PreallocatedSize)
		{
			allocatedData.free();
			numAllocated = minNumElements;
			dataPtr = preallocated;
		}
		else
		{
			if (minNumElements != numAllocated)
				allocatedData.realloc(minNumElements);

			numAllocated = minNumElements;
			dataPtr = allocatedData;
		}
	}

	void clearQuick()
	{
		numUsed = 0;
	}

	void clear()
	{
		for (auto& element : *this)
			element = ElementType();

		numUsed = 0;
	}

	const ElementType& operator[](int index) const
	{
		jassert(juce::isPositiveAndBelow(index, numUsed));

		return *(begin() + index);
	}

	void add(ElementType&& newElement, bool joinIfPossible=false)
	{
		if (joinIfPossible)
		{
			for (auto& e : *this)
			{
				if (newElement.canBeJoined(e))
				{
					e.join(std::move(newElement));
					return;
				}
			}
		}
		
		jassert(numUsed + 1 <= numAllocated);
		ElementType* ptr = begin() + numUsed;
		*ptr = newElement;
		numUsed++;
	}

	void remove(int indexToRemove)
	{
		if (juce::isPositiveAndBelow(indexToRemove, numUsed))
		{
			--numUsed;
			ElementType* const e = begin() + indexToRemove;
			e->~ElementType();
			const int numberToShift = numUsed - indexToRemove;

			if (numberToShift > 0)
				memmove(e, e + 1, ((size_t)numberToShift) * sizeof(ElementType));
		}
		else
		{
			jassertfalse;
		}
	}

	ElementType* begin() const noexcept
	{
		return const_cast<ElementType*>(dataPtr);
	}

	ElementType* end() const noexcept
	{
		return begin() + numUsed;
	}

	int size() const noexcept { return numUsed; }

	int maxSize() const noexcept { return numAllocated; }

	bool isDynamic() const noexcept { return allocatedData != nullptr; }

	ElementType& getReference(int index)
	{
		jassert(juce::isPositiveAndBelow(index, numUsed));

		return *(begin() + index);
	}

private:

	int numUsed = 0;
	int numAllocated = PreallocatedSize;

	ElementType* dataPtr;
	ElementType preallocated[PreallocatedSize];
    juce::HeapBlock<ElementType> allocatedData;
};


/** A buffer for audio signals with two storage types: 32bit float and 16bit integer. 
*
*	It mirrors the functionality of JUCE's AudioSampleBuffer, but uses two different data types internally.
*	This is used to reduce the memory consumption by 50% in case that the samples are compressed with HLAC (or uncompressed 16 bit).
*/
class HiseSampleBuffer
{
public:

	/** The normaliser object takes care of applying the normalisation from the HLAC codec to
		the a given audiosample buffer.
		
	*/
	struct Normaliser
	{
		Normaliser()
		{};

		void allocate(int numSamples)
		{
			auto minNumToUse = juce::jmax<int>(16, numSamples / 1024 + 3);
			infos.ensureStorageAllocated(minNumToUse);
		}

		void clear(juce::Range<int> rangeToClear = juce::Range<int>());

		void apply(float* dataLWithoutOffset, float* dataRWithoutOffset, juce::Range<int> rangeInData) const;

		/** Copies the normalisation ranges from the source than intersect with the given range. */
		void copyFrom(const Normaliser& source, juce::Range<int> srcRange, juce::Range<int> dstRange);

		struct NormalisationInfo
		{
			uint8_t leftNormalisation = 0;
			uint8_t rightNormalisation = 0;
            juce::Range<int> range;

			bool canBeJoined(const NormalisationInfo& other) const;

			void join(NormalisationInfo&& other);

			void apply(float* dataLWithoutOffset, float* dataRWithoutOffset, juce::Range<int> rangeInData) const;
		};

		OptionalDynamicArray<NormalisationInfo, 16> infos;
	};

	HiseSampleBuffer() :
        useOneMap(false),
		numChannels(0),
		size(0),
        isFloat(true)
	{}

	HiseSampleBuffer(bool isFloat_, int numChannels_, int numSamples) :
        useOneMap(numChannels_ == 1),
        numChannels(numChannels_),
        size(numSamples),
		isFloat(isFloat_),
		floatBuffer(numChannels, isFloat ? numSamples : 0),
		leftIntBuffer(isFloat ? 0 : numSamples),
		rightIntBuffer(isFloat ? 0 : numSamples)
    {}

	HiseSampleBuffer(HiseSampleBuffer&& otherBuffer) :
        useOneMap(otherBuffer.useOneMap),
        numChannels(otherBuffer.numChannels),
        size(otherBuffer.size),
        isFloat(otherBuffer.isFloat),
        floatBuffer(otherBuffer.floatBuffer),
		leftIntBuffer(std::move(otherBuffer.leftIntBuffer)),
		rightIntBuffer(std::move(otherBuffer.rightIntBuffer))
	{}

	/** Creates an HiseSampleBuffer from an array of data pointers. */
	HiseSampleBuffer(int16_t** sampleData, int numChannels_, int numSamples):
        useOneMap(numChannels_ == 1),
        numChannels(numChannels_),
        size(numSamples),
        leftIntBuffer(sampleData[0], numSamples),
        rightIntBuffer(numChannels > 1 ? sampleData[0] : nullptr, numSamples)
	{}

	HiseSampleBuffer& operator= (HiseSampleBuffer&& other)
	{
        useOneMap = other.useOneMap;
		isFloat = other.isFloat;
		leftIntBuffer = std::move(other.leftIntBuffer);
		rightIntBuffer = std::move(other.rightIntBuffer);
		floatBuffer = other.floatBuffer;
		numChannels = other.numChannels;
		size = other.size;

		return *this;
	}

	HiseSampleBuffer(HiseSampleBuffer& otherBuffer, int offset);

	HiseSampleBuffer(FixedSampleBuffer&& intBuffer) :
        useOneMap(true),
        numChannels(1),
        size(intBuffer.size),
        floatBuffer(),
        leftIntBuffer(std::move(intBuffer))
	{
        flushNormalisationInfo({ 0, size });
	}

	/** Creates a HiseSampleBuffer from an existing AudioSampleBuffer. */
	HiseSampleBuffer(juce::AudioSampleBuffer& floatBuffer_):
        useOneMap(floatBuffer_.getNumChannels() == 1),
        numChannels(floatBuffer_.getNumChannels()),
        size(floatBuffer_.getNumSamples()),
		isFloat(true),
		floatBuffer(floatBuffer_),
		leftIntBuffer(0),
		rightIntBuffer(0)
	{}

	~HiseSampleBuffer() {};

	bool isFloatingPoint() const { return isFloat; }

	int getNumSamples() const { return isFloatingPoint() ? floatBuffer.getNumSamples() : leftIntBuffer.size; };

	void reverse(int startSample, int numSamples);

	void setSize(int numChannels, int numSamples);

	void clear();

	void clear(int startSample, int numSamples);

	int getNumChannels() const { return numChannels; }

	void allocateNormalisationTables (int offsetToUse);

	void flushNormalisationInfo (juce::Range<int> rangeToFlush);

	FixedSampleBuffer& getFixedBuffer(int channelIndex);

	void convertToFloatWithNormalisation(float** data, int numChannels, int startSampleInSource, int numSamples) const;

	void clearNormalisation (juce::Range<int> r);

	bool usesNormalisation() const noexcept;

	/** Bakes in the normalisation values. This is not lossless and the operation will allocate a temporary float buffer. */
	void burnNormalisation();

	void copyNormalisationRanges(const HiseSampleBuffer& otherBuffset, int startOffsetInBuffer);

	/** Copies the samples from the source to the destination. The buffers must have the same data type. */
	static void copy(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples);

	/** Adds the samples from the source to the destination. The buffers must have the same data type. */
	static void add(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples);

	/** returns a write pointer to the given position. Unlike it's AudioSampleBuffer counterpart, it returns a void*, so you have to cast it yourself using isFloatingPoint(). */
	void* getWritePointer(int channel, int startSample);

	/** returns a read pointer to the given position. Unlike it's AudioSampleBuffer counterpart, it returns a void*, so you have to cast it yourself using isFloatingPoint(). */
	const void* getReadPointer(int channel, int startSample=0) const;

	void applyGainRamp(int channelIndex, int startOffset, int rampLength, float startGain, float endGain);

	/** Returns the internal AudioSampleBuffer for convenient usage with AudioFormatReader classes. */
    juce::AudioSampleBuffer* getFloatBufferForFileReader();

	CompressionHelpers::NormaliseMap& getNormaliseMap(int channelIndex);

	const CompressionHelpers::NormaliseMap& getNormaliseMap(int channelIndex) const;

	void setUseOneMap(bool shouldUseOneMap) { useOneMap = shouldUseOneMap; };

	bool useOneMap = false;

	void minimizeNormalisationInfo();

private:

	Normaliser normaliser;

	int numChannels = 0;
	int size = 0;

	bool hasSecondChannel() const { return numChannels == 2; }

	bool isFloat = false;

    juce::AudioSampleBuffer floatBuffer;

	FixedSampleBuffer leftIntBuffer;
	FixedSampleBuffer rightIntBuffer;

};

} // namespace hlac

#endif  // SAMPLEBUFFER_H_INCLUDED
