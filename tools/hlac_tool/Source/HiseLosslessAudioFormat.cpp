/*
  ==============================================================================

    HiseLosslessAudioFormat.cpp
    Created: 12 Apr 2017 10:15:39pm
    Author:  Christoph

  ==============================================================================
*/


#include "HiseLosslessAudioFormat.h"

bool HiseLosslessAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	bool isStereo = destSamples[1] != nullptr;

	if (isStereo)
	{
		jassertfalse;
	}
	else
	{
		float* destinationFloat = reinterpret_cast<float*>(destSamples[0]);

		AudioSampleBuffer b = AudioSampleBuffer(&destinationFloat, 1, numSamples);

		decoder.decode(b, *input);
	}

	return true;
}

bool HiseLosslessAudioFormatWriter::write(const int** samplesToWrite, int numSamples)
{
	bool isStereo = samplesToWrite[1] != nullptr;

	if (isStereo)
	{
		jassertfalse;
	}
	else
	{
		float* r = const_cast<float*>(reinterpret_cast<const float*>(samplesToWrite[0]));

		AudioSampleBuffer b = AudioSampleBuffer(&r, 1, numSamples);

		encoder.compress(b, *output);
	}

	return true;
}

HiseLosslessAudioFormat::HiseLosslessAudioFormat() :
	AudioFormat("HLAC", StringArray(".hlac"))
{

}

bool HiseLosslessAudioFormat::canHandleFile(const File& fileToTest)
{
	return fileToTest.getFileExtension() == ".hlac";
}

Array<int> HiseLosslessAudioFormat::getPossibleSampleRates()
{
	Array<int> sampleRates;

	sampleRates.add(11025);
	sampleRates.add(22050);
	sampleRates.add(32000);
	sampleRates.add(44100);
	sampleRates.add(48000);
	sampleRates.add(88200);
	sampleRates.add(96000);

	return sampleRates;
}

Array<int> HiseLosslessAudioFormat::getPossibleBitDepths()
{
	Array<int> bitRates;

	bitRates.add(16); // that's it folks :)

	return bitRates;
}


bool HiseLosslessAudioFormat::canDoMono()
{
	return true;
}

bool HiseLosslessAudioFormat::canDoStereo()
{
	return true;
}

bool HiseLosslessAudioFormat::isCompressed()
{
	return true;
}

StringArray HiseLosslessAudioFormat::getQualityOptions()
{
	StringArray options;

	options.add("EncodeMode");

	return options;
}

AudioFormatReader* HiseLosslessAudioFormat::createReaderFor(InputStream* sourceStream, bool deleteStreamIfOpeningFails)
{
	return new HiseLosslessAudioFormatReader(sourceStream);
}

AudioFormatWriter* HiseLosslessAudioFormat::createWriterFor(OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const StringPairArray& metadataValues, int /*qualityOptionIndex*/)
{
	HiseLosslessAudioFormatWriter::EncodeMode mode = metadataValues.getValue("EncodeMode", "Diff") == "Diff" ?
		HiseLosslessAudioFormatWriter::EncodeMode::Diff :
		HiseLosslessAudioFormatWriter::EncodeMode::Block;

	return new HiseLosslessAudioFormatWriter(mode, streamToWriteTo, sampleRateToUse, numberOfChannels);
}
