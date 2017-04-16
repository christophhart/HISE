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



class HiseLosslessAudioFormatReader : public AudioFormatReader
{
public:
	HiseLosslessAudioFormatReader(InputStream* input_):
		AudioFormatReader(input_, "HLAC")
	{
		numChannels = 1;
		usesFloatingPointData = true;

		decoder.setupForDecompression();
	}

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override;

	double getDecompressionPerformanceForLastFile() { return decoder.getDecompressionPerformance(); }

private:

	HlacDecoder decoder;

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

	HiseLosslessAudioFormatWriter(EncodeMode mode_, OutputStream* output, double sampleRate, int numChannels):
		AudioFormatWriter(output, "HLAC", sampleRate, numChannels, 16),
		mode(mode_)
	{
		usesFloatingPointData = true;
	}

	void setOptions(HlacEncoder::CompressorOptions& newOptions)
	{
		encoder.setOptions(newOptions);
	}

	bool write(const int** samplesToWrite, int numSamples) override;


	double getCompressionRatioForLastFile() { return encoder.getCompressionRatio(); }

private:

	HlacEncoder encoder;

	EncodeMode mode;
	HlacEncoder::CompressorOptions options;
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
	bool isCompressed();

	

	StringArray getQualityOptions() override;

	AudioFormatReader* createReaderFor(InputStream* sourceStream, bool deleteStreamIfOpeningFails) override;
	AudioFormatWriter* createWriterFor(OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const StringPairArray& metadataValues, int /*qualityOptionIndex*/) override;
};


#endif  // HISELOSSLESSAUDIOFORMAT_H_INCLUDED
