/*
  ==============================================================================

    HlacEncoder.h
    Created: 16 Apr 2017 10:18:36am
    Author:  Christoph

  ==============================================================================
*/

#ifndef HLACENCODER_H_INCLUDED
#define HLACENCODER_H_INCLUDED

#include "JuceHeader.h"

#include "CompressionHelpers.h"

class HlacEncoder
{
public:

	HlacEncoder():
		currentCycle(0),
		workBuffer(0)
	{
		reset();
	};

	struct CompressorOptions
	{
		bool useDeltaEncoding = true;
		int16 fixedBlockWidth = -1;
		bool reuseFirstCycleLengthForBlock = true;
		bool removeDcOffset = true;
		float deltaCycleThreshhold = 0.2f;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;
	};


	void compress(AudioSampleBuffer& source, OutputStream& output);
	
	void reset();

	void setOptions(CompressorOptions& newOptions)
	{
		options = newOptions;
	}

	float getCompressionRatio() const;

private:

	bool encodeBlock(AudioSampleBuffer& block, OutputStream& output);

	bool encodeBlock(CompressionHelpers::AudioBufferInt16& block, OutputStream& output);

	uint8 getBitReductionAmountForMSEncoding(AudioSampleBuffer& block);

	bool isBlockExhausted() const
	{
		return indexInBlock >= COMPRESSION_BLOCK_SIZE;
	}



	bool encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output);
	void  writeUncompressed(AudioSampleBuffer& block, OutputStream& output);

	bool writeCycleHeader(bool isTemplate, int bitDepth, int numSamples, OutputStream& output);
	bool writeDiffHeader(int fullBitRate, int errorBitRate, int blockSize, OutputStream& output);

	int getCycleLength(CompressionHelpers::AudioBufferInt16& block);
	int getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	int indexInBlock;

	uint32 numBytesWritten = 0;
	uint32 numBytesUncompressed = 0;

	uint32 numTemplates = 0;
	uint32 numDeltas = 0;

	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int firstCycleLength = -1;

	MemoryBlock readBuffer;

	CompressorOptions options;

	float ratio = 0.0f;

	uint64 readIndex = 0;

	double decompressionSpeed = 0.0;
};


#endif  // HLACENCODER_H_INCLUDED
