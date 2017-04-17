/*
  ==============================================================================

    HlacDecoder.h
    Created: 16 Apr 2017 10:18:47am
    Author:  Christoph

  ==============================================================================
*/

#ifndef HLACDECODER_H_INCLUDED
#define HLACDECODER_H_INCLUDED


#include "JuceHeader.h"

#include "CompressionHelpers.h"


class HlacDecoder
{
public:


	HlacDecoder():
	currentCycle(0),
	workBuffer(0)
	{};

	void decode(AudioSampleBuffer& destination, InputStream& input);

	void setupForDecompression();

	double getDecompressionPerformance() const { return decompressionSpeed; }

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
	
	bool decodeBlock(AudioSampleBuffer& destination, InputStream& input, int channelIndex);

	void decodeDiff(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input, int channelIndex);

	void decodeCycle(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input, int channelIndex);

	CycleHeader readCycleHeader(InputStream& input);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	uint16 indexInBlock = 0;

	uint32 numBytesWritten = 0;

	uint32 numTemplates = 0;
	uint32 numDeltas = 0;

	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int16 firstCycleLength = -1;

	MemoryBlock readBuffer;

	float ratio = 0.0f;

	int readIndex = 0;

	double decompressionSpeed = 0.0;
};



#endif  // HLACDECODER_H_INCLUDED
