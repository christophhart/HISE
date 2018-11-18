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


#ifndef HLACDECODER_H_INCLUDED
#define HLACDECODER_H_INCLUDED

namespace hlac { using namespace juce; 

class HlacDecoder
{
public:


	HlacDecoder():
	currentCycle(0),
	workBuffer(0)
	{};

	void setHlacVersion(int version)
	{
		hlacVersion = version;
	}

	void decode(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int offsetInSource=0, int numSamples=-1);

	void setupForDecompression();

	double getDecompressionPerformance() const;

	uint32 getCurrentReadPosition() const
	{
		return readOffset;
	}

	void seekToPosition(InputStream& input, uint32 samplePosition, uint32 byteOffset);

private:

	struct CycleHeader
	{
		CycleHeader(uint8 headerInfo_, uint16 numSamples_) :
			headerInfo(headerInfo_),
			numSamples(numSamples_)
		{}

		bool isTemplate() const;
		uint8 getBitRate(bool getFullBitRate = true) const;
		bool isDiff() const;

		uint16 getNumSamples() const;

	private:

		uint8 headerInfo;
		uint16 numSamples;
	};

	void reset();
	
	

	bool decodeBlock(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int channelIndex);

	void decodeDiff(const CycleHeader& header, bool decodeStereo, HiseSampleBuffer& destination, InputStream& input, int channelIndex);

	void decodeCycle(const CycleHeader& header, bool decodeStereo, HiseSampleBuffer& destination, InputStream& input, int channelIndex);

	enum class FloatWriteMode
	{
		Copy,
		Add,
		Clear,
		numWriteModes
	};

	void writeToFloatArray(bool shouldCopy, bool useTempBuffer, HiseSampleBuffer& destination, int channelIndex, int numSamples);

	CycleHeader readCycleHeader(InputStream& input);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	uint16 indexInBlock = 0;
	int leftFloatIndex = 0;
	int rightFloatIndex = 0;
	
	int leftNumToSkip = 0;
	int rightNumToSkip = 0;
	
	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int16 firstCycleLength = -1;

	MemoryBlock readBuffer;

	float ratio = 0.0f;

	int readOffset = 0;

	int readIndex = 0;

	Array<double> decompressionSpeeds;

	double decompressionSpeed = 0.0;

	int hlacVersion = HLAC_VERSION;
};

} // namespace hlac

#endif  // HLACDECODER_H_INCLUDED
