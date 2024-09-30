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

namespace hlac { using namespace juce; 

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
		jassert(isPositiveAndBelow(index, numUsed));

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
		if (isPositiveAndBelow(indexToRemove, numUsed))
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
		jassert(isPositiveAndBelow(index, numUsed));

		return *(begin() + index);
	}

private:

	int numUsed = 0;
	int numAllocated = PreallocatedSize;

	ElementType* dataPtr;
	ElementType preallocated[PreallocatedSize];
	HeapBlock<ElementType> allocatedData;
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
			auto minNumToUse = jmax<int>(16, numSamples / 1024 + 3);
			infos.ensureStorageAllocated(minNumToUse);
		}

		void clear(Range<int> rangeToClear = Range<int>());

		void apply(float* dataLWithoutOffset, float* dataRWithoutOffset, Range<int> rangeInData) const;

		/** Copies the normalisation ranges from the source than intersect with the given range. */
		void copyFrom(const Normaliser& source, Range<int> srcRange, Range<int> dstRange);

		struct NormalisationInfo
		{
			uint8 leftNormalisation = 0;
			uint8 rightNormalisation = 0;
			Range<int> range;

			bool canBeJoined(const NormalisationInfo& other) const;

			void join(NormalisationInfo&& other);

			void apply(float* dataLWithoutOffset, float* dataRWithoutOffset, Range<int> rangeInData) const;
		};

		OptionalDynamicArray<NormalisationInfo, 16> infos;
	};

	HiseSampleBuffer() :
		isFloat(true),
		leftIntBuffer(0),
		rightIntBuffer(0),
		numChannels(0),
		size(0),
		useOneMap(false)
	{};

	HiseSampleBuffer(bool isFloat_, int numChannels_, int numSamples) :
		isFloat(isFloat_),
		floatBuffer(numChannels_, isFloat_ ? numSamples : 0),
		leftIntBuffer(isFloat_ ? 0 : numSamples),
		rightIntBuffer(isFloat_ ? 0 : numSamples),
		numChannels(numChannels_),
		size(numSamples)
	{
		useOneMap = numChannels == 1;
	}

	HiseSampleBuffer(HiseSampleBuffer&& otherBuffer) :
		floatBuffer(otherBuffer.floatBuffer),
		isFloat(otherBuffer.isFloat),
		leftIntBuffer(std::move(otherBuffer.leftIntBuffer)),
		rightIntBuffer(std::move(otherBuffer.rightIntBuffer)),
		numChannels(otherBuffer.numChannels),
		size(otherBuffer.size),
		useOneMap(otherBuffer.useOneMap)
	{};

	/** Creates an HiseSampleBuffer from an array of data pointers. */
	HiseSampleBuffer(int16** sampleData, int numChannels_, int numSamples):
		leftIntBuffer(sampleData[0], numSamples),
		rightIntBuffer(numChannels_ > 1 ? sampleData[0] : nullptr, numSamples),
		isFloat(false),
		size(numSamples),
		numChannels(numChannels_),
		useOneMap(numChannels_ == 1)
	{
		
	}

	HiseSampleBuffer& operator= (HiseSampleBuffer&& other)
	{
		isFloat = other.isFloat;
		leftIntBuffer = std::move(other.leftIntBuffer);
		rightIntBuffer = std::move(other.rightIntBuffer);
		floatBuffer = other.floatBuffer;
		numChannels = other.numChannels;
		size = other.size;
		useOneMap = other.useOneMap;

		return *this;
	}

	HiseSampleBuffer(HiseSampleBuffer& otherBuffer, int offset);

	HiseSampleBuffer(FixedSampleBuffer&& intBuffer) :
		isFloat(false),
		size(intBuffer.size),
		floatBuffer(),
		numChannels(1),
		leftIntBuffer(std::move(intBuffer)),
		rightIntBuffer(0),
		useOneMap(true)
	{
		flushNormalisationInfo({ 0, size });
	}

	/** Creates a HiseSampleBuffer from an existing AudioSampleBuffer. */
	HiseSampleBuffer(AudioSampleBuffer& floatBuffer_):
		isFloat(true),
		floatBuffer(floatBuffer_),
		leftIntBuffer(0),
		rightIntBuffer(0),
		numChannels(floatBuffer_.getNumChannels()),
		size(floatBuffer_.getNumSamples()),
		useOneMap(floatBuffer_.getNumChannels() == 1)
	{
		
	};

	~HiseSampleBuffer() {};

	bool isFloatingPoint() const { return isFloat; }

	int getNumSamples() const { return isFloatingPoint() ? floatBuffer.getNumSamples() : leftIntBuffer.size; };

	void reverse(int startSample, int numSamples);

	void setSize(int numChannels, int numSamples);

	void clear();

	void clear(int startSample, int numSamples);

	int getNumChannels() const { return numChannels; }

	void allocateNormalisationTables(int offsetToUse);

	void flushNormalisationInfo(Range<int> rangeToFlush);

	FixedSampleBuffer& getFixedBuffer(int channelIndex);

	void convertToFloatWithNormalisation(float** data, int numChannels, int startSampleInSource, int numSamples) const;

	void clearNormalisation(Range<int> r);

	bool usesNormalisation() const noexcept;

	/** Bakes in the normalisation values. This is not lossless and the operation will allocate a temporary float buffer. */
	void burnNormalisation(bool useFloatBuffer=false);

	void copyNormalisationRanges(const HiseSampleBuffer& otherBuffset, int startOffsetInBuffer);

	/** Copies the samples from the source to the destination. The buffers must have the same data type. */
	static void copy(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples);

	/** Adds the samples from the source to the destination. The buffers must have the same data type. */
	static void add(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples);

	static void addWithGain(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples, float gainFactor);

	/** returns a write pointer to the given position. Unlike it's AudioSampleBuffer counterpart, it returns a void*, so you have to cast it yourself using isFloatingPoint(). */
	void* getWritePointer(int channel, int startSample);

	/** returns a read pointer to the given position. Unlike it's AudioSampleBuffer counterpart, it returns a void*, so you have to cast it yourself using isFloatingPoint(). */
	const void* getReadPointer(int channel, int startSample=0) const;

	void applyGainRamp(int channelIndex, int startOffset, int rampLength, float startGain, float endGain);

	void applyGainRampWithGamma(int channelIndex, int startOffset, int rampLength, float startGain, float endGain,
	                            float gamma)
	{
		if(gamma == 1.0f)
			applyGainRamp(channelIndex, startOffset, rampLength, startGain, endGain);
		else
		{
			jassert(getNumSamples() >= rampLength);
			jassert(getNumChannels() == 2);
			
			bool isFloat = isFloatingPoint();

			auto l = (float*)getWritePointer(0, 0);
			auto r = (float*)getWritePointer(1, 0);

			auto lInt = (int16*)getWritePointer(0, 0);
			auto rInt = (int16*)getWritePointer(1, 0);

			float gainFactor = 0.0f;

			for (int i = 0; i < rampLength; i++)
			{
				float indexToUse = (float)i / (float)rampLength;
				auto g = startGain + indexToUse * (endGain - startGain);

				gainFactor = std::pow(g, gamma);

				if (isFloat)
				{
					l[i] *= gainFactor;
					r[i] *= gainFactor;
				}
				else
				{
					lInt[i] = (int16)((float)lInt[i] * gainFactor);
					rInt[i] = (int16)((float)rInt[i] * gainFactor);
				}
			}
		}
	}

	/** Returns the internal AudioSampleBuffer for convenient usage with AudioFormatReader classes. */
	AudioSampleBuffer* getFloatBufferForFileReader();

	CompressionHelpers::NormaliseMap& getNormaliseMap(int channelIndex);

	const CompressionHelpers::NormaliseMap& getNormaliseMap(int channelIndex) const;

	void setUseOneMap(bool shouldUseOneMap) { useOneMap = shouldUseOneMap; };

	bool useOneMap = false;

	void minimizeNormalisationInfo();

	float getMagnitude(int startOffset, int numSamples) const
	{
		if(isFloatingPoint())
			return floatBuffer.getMagnitude(startOffset, numSamples);
		else
		{
			int maxValue = 0;

			auto l = leftIntBuffer.getReadPointer(0);
			auto r = rightIntBuffer.getReadPointer(0);

			for(int i = startOffset; i < numSamples; i++)
			{
				jmax<int>(maxValue, std::abs((int)l[i]), std::abs((int)r[i]));
			}

			auto g = (float)maxValue / (float)INT16_MAX;
			return g;
		}
	}

private:

	Normaliser normaliser;

	int numChannels = 0;
	int size = 0;

	bool useNormalisationMap = false;

	bool hasSecondChannel() const { return numChannels == 2; }

	bool isFloat = false;

	AudioSampleBuffer floatBuffer;

	FixedSampleBuffer leftIntBuffer;
	FixedSampleBuffer rightIntBuffer;

};

} // namespace hlac

#endif  // SAMPLEBUFFER_H_INCLUDED
