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

/** A buffer for audio signals with two storage types: 32bit float and 16bit integer. 
*
*	It mirrors the functionality of JUCE's AudioSampleBuffer, but uses two different data types internally.
*	This is used to reduce the memory consumption by 50% in case that the samples are compressed with HLAC (or uncompressed 16 bit).
*/
class HiseSampleBuffer
{
public:

	HiseSampleBuffer() :
		isFloat(true),
		leftIntBuffer(0),
		rightIntBuffer(0),
		numChannels(0),
		size(0)
	{};

	HiseSampleBuffer(bool isFloat_, int numChannels_, int numSamples) :
		isFloat(isFloat_),
		floatBuffer(numChannels_, isFloat_ ? numSamples : 0),
		leftIntBuffer(isFloat_ ? 0 : numSamples),
		rightIntBuffer(isFloat_ ? 0 : numSamples),
		numChannels(numChannels_),
		size(numSamples)
	{

	}

	HiseSampleBuffer(HiseSampleBuffer&& otherBuffer) :
		floatBuffer(otherBuffer.floatBuffer),
		isFloat(otherBuffer.isFloat),
		leftIntBuffer(std::move(otherBuffer.leftIntBuffer)),
		rightIntBuffer(std::move(otherBuffer.rightIntBuffer)),
		numChannels(otherBuffer.numChannels),
		size(otherBuffer.size)
	{};

	/** Creates an HiseSampleBuffer from an array of data pointers. */
	HiseSampleBuffer(int16** sampleData, int numChannels_, int numSamples):
		leftIntBuffer(sampleData[0], numSamples),
		rightIntBuffer(numChannels_ > 1 ? sampleData[0] : nullptr, numSamples),
		isFloat(false),
		size(numSamples),
		numChannels(numChannels_)
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

		return *this;
	}

	

	HiseSampleBuffer(HiseSampleBuffer& otherBuffer, int offset);

	/** Creates a HiseSampleBuffer from an existing AudioSampleBuffer. */
	HiseSampleBuffer(AudioSampleBuffer& floatBuffer_):
		isFloat(true),
		floatBuffer(floatBuffer_),
		leftIntBuffer(0),
		rightIntBuffer(0),
		numChannels(floatBuffer_.getNumChannels()),
		size(floatBuffer_.getNumSamples())
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
	AudioSampleBuffer* getFloatBufferForFileReader();

	

private:

	int numChannels = 0;
	int size = 0;

	bool hasSecondChannel() const { return numChannels == 2; }

	bool isFloat = false;

	AudioSampleBuffer floatBuffer;

	FixedSampleBuffer leftIntBuffer;
	FixedSampleBuffer rightIntBuffer;

};

} // namespace hlac

#endif  // SAMPLEBUFFER_H_INCLUDED
