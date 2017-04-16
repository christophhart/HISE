/*
  ==============================================================================

    HlacDecoder.cpp
    Created: 16 Apr 2017 10:18:47am
    Author:  Christoph

  ==============================================================================
*/

#include "HlacDecoder.h"



void HlacDecoder::setupForDecompression()
{
	reset();

	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	currentCycle = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	readBuffer.setSize(COMPRESSION_BLOCK_SIZE * 2);

	decompressionSpeed = 0.0;

}


void HlacDecoder::reset()
{
	indexInBlock = 0;
	currentCycle = CompressionHelpers::AudioBufferInt16(0);
	workBuffer = CompressionHelpers::AudioBufferInt16(0);
	blockOffset = 0;
	numBytesWritten = 0;
	numTemplates = 0;
	numDeltas = 0;
	blockOffset = 0;
	bitRateForCurrentCycle = 0;
	firstCycleLength = -1;
	ratio = 0.0f;
	readIndex = 0;
}


void HlacDecoder::decode(AudioSampleBuffer& destination, InputStream& input)
{
	double start = Time::getMillisecondCounterHiRes();

	

	while (!input.isExhausted())
	{
		auto header = readCycleHeader(input);

		if (header.isDiff())
			decodeDiff(header, destination, input);
		else
			decodeCycle(header, destination, input);
	}

	double stop = Time::getMillisecondCounterHiRes();
	double sampleLength = (double)destination.getNumSamples() / 44100.0;
	double delta = (stop - start) / 1000.0;

	decompressionSpeed = sampleLength / delta;

	Logger::writeToLog("HLAC Decoding Performance: " + String(decompressionSpeed, 1) + "x realtime");
}

void HlacDecoder::decodeDiff(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input)
{
	uint16 blockSize = header.getNumSamples();

	uint8 fullBitRate = header.getBitRate(true);
	auto compressorFull = collection.getSuitableCompressorForBitRate(fullBitRate);
	auto numFullValues = CompressionHelpers::Diff::getNumFullValues(blockSize);
	auto numFullBytes = compressorFull->getByteAmount(numFullValues);

	input.read(readBuffer.getData(), numFullBytes);

	compressorFull->decompress(workBuffer.getWritePointer(), (uint8*)readBuffer.getData(), numFullValues);

	CompressionHelpers::Diff::distributeFullSamples(currentCycle, (const uint16*)workBuffer.getReadPointer(), numFullValues);

	uint8 errorBitRate = header.getBitRate(false);
	auto compressorError = collection.getSuitableCompressorForBitRate(errorBitRate);
	auto numErrorValues = CompressionHelpers::Diff::getNumErrorValues(blockSize);
	auto numErrorBytes = compressorError->getByteAmount(numErrorValues);

	input.read(readBuffer.getData(), numErrorBytes);

	compressorError->decompress(workBuffer.getWritePointer(), (uint8*)readBuffer.getData(), numErrorValues);

	CompressionHelpers::Diff::addErrorSignal(currentCycle, (const uint16*)workBuffer.getReadPointer(), numErrorValues);

	auto dst = destination.getWritePointer(0, readIndex);
	AudioDataConverters::convertInt16LEToFloat(currentCycle.getReadPointer(), dst, blockSize);

	readIndex += blockSize;
}




void HlacDecoder::decodeCycle(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input)
{
	uint8 br = header.getBitRate();

	jassert(header.getBitRate() <= 16);
	jassert(header.getNumSamples() <= COMPRESSION_BLOCK_SIZE);

	uint16 numSamples = header.getNumSamples();

	auto compressor = collection.getSuitableCompressorForBitRate(header.getBitRate());
	auto numBytesToRead = compressor->getByteAmount(numSamples);

	if (numBytesToRead > 0)
		input.read(readBuffer.getData(), numBytesToRead);

	auto dst = destination.getWritePointer(0, readIndex);

	if (header.isTemplate())
	{
		if (true && compressor->getAllowedBitRange() != 0)
		{
			compressor->decompress(currentCycle.getWritePointer(), (const uint8*)readBuffer.getData(), numSamples);
			AudioDataConverters::convertInt16LEToFloat(currentCycle.getReadPointer(), dst, numSamples);
		}
		else
		{
			FloatVectorOperations::clear(dst, numSamples);
		}
	}
	else
	{
		compressor->decompress(workBuffer.getWritePointer(), (const uint8*)readBuffer.getData(), numSamples);

		CompressionHelpers::IntVectorOperations::add(workBuffer.getWritePointer(), currentCycle.getReadPointer(), numSamples);
		AudioDataConverters::convertInt16LEToFloat(workBuffer.getReadPointer(), dst, numSamples);
	}

	readIndex += numSamples;
}

HlacDecoder::CycleHeader HlacDecoder::readCycleHeader(InputStream& input)
{
	uint8 h = input.readByte();
	uint16 s = input.readShort();

	CycleHeader header(h, s);

	return header;
}


bool HlacDecoder::CycleHeader::isTemplate() const
{
	return headerInfo & 1 > 0;
}

uint8 HlacDecoder::CycleHeader::getBitRate(bool getFullBitRate) const
{
	if (isDiff())
	{
		uint16 b = (numSamples & 0xFF00) >> 8;

		if (getFullBitRate)
			return ((b & 0b11110000) >> 4) + 1;
		else
			return ((b & 0b00001111) + 1);
	}
	else
	{
		return (headerInfo >> 1);
	}
}

bool HlacDecoder::CycleHeader::isDiff() const
{
	return headerInfo == 0b11100000;
}

uint16 HlacDecoder::CycleHeader::getNumSamples() const
{
	if (isDiff())
	{
		uint16 numSamplesLog = numSamples & 0xFF;
		return 1 << numSamplesLog;
	}
	else
	{
		return numSamples;
	}

}
