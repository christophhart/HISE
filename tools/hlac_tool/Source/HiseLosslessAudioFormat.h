/*
  ==============================================================================

    HiseLosslessAudioFormat.h
    Created: 12 Apr 2017 10:15:39pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef HISELOSSLESSAUDIOFORMAT_H_INCLUDED
#define HISELOSSLESSAUDIOFORMAT_H_INCLUDED

#include "CompressionHelpers.h"


#define LOG_OUTPUT 0


#if LOG_OUTPUT
#define LOG(x) DBG(x)
#else
#define LOG(x)
#endif

class HiseLosslessAudioFormat
{
public:

	struct CompressorOptions
	{
		bool useDeltaEncoding = true;
		int fixedBlockWidth = -1;
		bool reuseFirstCycleLengthForBlock = true;
		bool removeDcOffset = true;
		float deltaCycleThreshhold = 0.2f;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;
	};

	HiseLosslessAudioFormat():
		currentCycle(0),
		workBuffer(0)
	{};

	struct CycleHeader
	{
		bool isTemplate() const;
		uint8 getBitRate();

		uint8 headerInfo;
		uint16 numSamples;
	};

	void compress(AudioSampleBuffer& source, OutputStream& output);

	void setupForDecompression();

	void decompress(AudioSampleBuffer& destination, InputStream& input);

	void setOptions(CompressorOptions& newOptions)
	{
		options = newOptions;
	}

	void reset();

	float getCompressionRatio() const { return ratio; }

	double getDecompressionPerformance() const { return decompressionSpeed; }

private:

	bool compressBlock(AudioSampleBuffer& block, OutputStream& output);

	uint8 getBitReductionAmountForMSEncoding(AudioSampleBuffer& block);

	/** compresses the current cycle with the min bit depth
	* and stores it to the current cycle slot. */
	bool compressCycleAsTemplate(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);

	/** Writes the 2 byte cycle header.
	*
	*	Bit structure:
	*	AAAA AAAA AAAB BBBC
	*
	*	C: isTemplate
	*	B: bit depth (1 - 16)
	*	A: number of samples(1 - 4096)
	*/
	bool writeCycleHeader(bool isTemplate, uint8 bitDepth, uint16 numSamples, OutputStream& output);

	CycleHeader readCycleHeader(InputStream& input);

	void writeUncompressed(AudioSampleBuffer& block, OutputStream& output);

	bool isBlockExhausted() const;

	/** Returns the best length for the cycle template.
	*
	* It calculates the perfect length as fractional number and then returns the upper ceiling value.
	* This is so that subsequent cycles can use the additional sample or not depending on the bit reduction amount.
	*
	* If it can't reduce the bit range it will return the block size
	*/
	uint16 getCycleLength(CompressionHelpers::AudioBufferInt16& block);


	/** Checks if the next cycle has a lower bit depth if the shortened template is used. */
	uint16 getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest);

	/** calculates the diff with the last cycle and stores the delta with the min bit depth. */
	bool compressCycleAsDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	uint16 indexInBlock;

	uint32 numBytesWritten = 0;

	uint32 numTemplates = 0;
	uint32 numDeltas = 0;

	uint32 blockOffset = 0;

	uint8 bitRateForCurrentCycle;

	int16 firstCycleLength = -1;

	MemoryBlock readBuffer;

	CompressorOptions options;

	float ratio = 0.0f;
	
	uint64 readIndex = 0;

	double decompressionSpeed = 0.0;

};





#endif  // HISELOSSLESSAUDIOFORMAT_H_INCLUDED
