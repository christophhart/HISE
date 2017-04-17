/*
  ==============================================================================

    HiseLosslessAudioFormat.h
    Created: 12 Apr 2017 10:15:39pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef HISELOSSLESSAUDIOFORMAT_H_INCLUDED
#define HISELOSSLESSAUDIOFORMAT_H_INCLUDED


#include "HlacEncoder.h"
#include "HlacDecoder.h"

#define HLAC_VERSION 0

struct HiseLosslessHeader
{
	HiseLosslessHeader(InputStream* input);

	HiseLosslessHeader(bool useEncryption, uint8 globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32 numBlocks);

	int getVersion() const
	{
		return (headerByte & 0x60) >> 5;
	}

	bool isEncrypted() const
	{
		return false;
	}

	int getBitShiftAmount() const
	{
		return (headerByte & 0x0F);
	}

	int getNumChannels() const 
	{
		return (sampleDataByte & 0x70) >> 4;
	}

	int getBitsPerSample() const
	{
		return (sampleDataByte & 0x02) != 0 ? 24 : 16;
	}

	bool usesCompression() const
	{
		return (sampleDataByte & 1) != 0;
	}

	double getSampleRate() const
	{
		const static double sampleRates[4] = { 44100.0, 48000.0, 88200.0, 96000.0 };
		const uint8 srIndex = (sampleDataByte & 0xC0) >> 6;
		return sampleRates[srIndex];
	}

	uint8 headerByte = 0;
	uint8 sampleDataByte = 0;
	uint32 blockAmount = 0;
	HeapBlock<uint32> blockOffsets;
	bool headerValid = false;

	bool write(OutputStream* output);
};

class HiseLosslessAudioFormatReader : public AudioFormatReader
{
public:
	HiseLosslessAudioFormatReader(InputStream* input_);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override;

	double getDecompressionPerformanceForLastFile() { return decoder.getDecompressionPerformance(); }

private:

	HlacDecoder decoder;
	
	HiseLosslessHeader header;

};

class HiseLosslessAudioFormatWriter : public AudioFormatWriter
{
public:

	enum class EncodeMode
	{
		Block,
		Delta,
		Diff,
		numEncodeModes
	};

	HiseLosslessAudioFormatWriter(EncodeMode mode_, OutputStream* output, double sampleRate, int numChannels, uint32* blockOffsetBuffer);

	~HiseLosslessAudioFormatWriter();

	/** You must call flush after you written all files so that it can create the block offset table and write the header. */
	bool flush() override;

	void setOptions(HlacEncoder::CompressorOptions& newOptions);

	bool write(const int** samplesToWrite, int numSamples) override;

	double getCompressionRatioForLastFile() { return encoder.getCompressionRatio(); }

	/** You can use a temporary file instead of the memory buffer if you encode large files. */
	void setTemporaryBufferType(bool shouldUseTemporaryFile);

private:

	bool writeHeader();
	bool writeDataFromTemp();

	void deleteTemp();

	ScopedPointer<TemporaryFile> tempFile;
	ScopedPointer<OutputStream> tempOutputStream;

	bool tempWasFlushed = true;
	bool usesTempFile = false;

	uint32* blockOffsets;

	HlacEncoder encoder;

	EncodeMode mode;
	HlacEncoder::CompressorOptions options;

	bool useEncryption = false;
	bool useCompression = true;

	uint8 globalBitShiftAmount = 0;
};

class HiseLosslessAudioFormat : public AudioFormat
{
public:

	

	HiseLosslessAudioFormat();;

	~HiseLosslessAudioFormat() {}

	bool canHandleFile(const File& fileToTest) override;

	Array<int> getPossibleSampleRates() override;
	Array<int> getPossibleBitDepths() override;

	bool canDoMono() override;
	bool canDoStereo() override;
	bool isCompressed() override;

	

	StringArray getQualityOptions() override;

	AudioFormatReader* createReaderFor(InputStream* sourceStream, bool deleteStreamIfOpeningFails) override;
	AudioFormatWriter* createWriterFor(OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const StringPairArray& metadataValues, int /*qualityOptionIndex*/) override;

	HeapBlock<uint32> blockOffsets;
};


#endif  // HISELOSSLESSAUDIOFORMAT_H_INCLUDED
