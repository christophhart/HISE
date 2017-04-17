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


bool HlacDecoder::decodeBlock(AudioSampleBuffer& destination, InputStream& input, int channelIndex)
{
    
	indexInBlock = 0;

	int numTodo = jmin<int>(destination.getNumSamples() - readIndex, COMPRESSION_BLOCK_SIZE);

    
    
    LOG("DEC " + String(readIndex) + "\t\tNew Block");
    
	while (indexInBlock < numTodo)
	{
		auto header = readCycleHeader(input);

		if (header.isDiff())
			decodeDiff(header, destination, input, channelIndex);
		else
			decodeCycle(header, destination, input, channelIndex);
        
        jassert(indexInBlock <= 4096);
	}

	if(channelIndex == destination.getNumChannels() - 1)
		readIndex += indexInBlock;

    
    
	return numTodo != 0;
}

void HlacDecoder::decode(AudioSampleBuffer& destination, InputStream& input)
{
	double start = Time::getMillisecondCounterHiRes();

	int channelIndex = 0;
	bool decodeStereo = destination.getNumChannels() == 2;

	while (!input.isExhausted())
	{
		if (!decodeBlock(destination, input, channelIndex))
			break;

		if (decodeStereo)
			channelIndex = 1 - channelIndex;
	}

	double stop = Time::getMillisecondCounterHiRes();
	double sampleLength = (double)destination.getNumSamples() / 44100.0;
	double delta = (stop - start) / 1000.0;

	decompressionSpeed = sampleLength / delta;

	Logger::writeToLog("HLAC Decoding Performance: " + String(decompressionSpeed, 1) + "x realtime");
}

void HlacDecoder::decodeDiff(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input, int channelIndex)
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

	auto dst = destination.getWritePointer(channelIndex, readIndex + indexInBlock);
	AudioDataConverters::convertInt16LEToFloat(currentCycle.getReadPointer(), dst, blockSize);

	indexInBlock += blockSize;
}




void HlacDecoder::decodeCycle(const CycleHeader& header, AudioSampleBuffer& destination, InputStream& input, int channelIndex)
{
	uint8 br = header.getBitRate();

	jassert(header.getBitRate() <= 16);
	jassert(header.getNumSamples() <= COMPRESSION_BLOCK_SIZE);

	uint16 numSamples = header.getNumSamples();

    jassert(indexInBlock+numSamples <= COMPRESSION_BLOCK_SIZE);
    
	auto compressor = collection.getSuitableCompressorForBitRate(br);
	auto numBytesToRead = compressor->getByteAmount(numSamples);

	if (numBytesToRead > 0)
		input.read(readBuffer.getData(), numBytesToRead);

	auto dst = destination.getWritePointer(channelIndex, readIndex + indexInBlock);

	if (header.isTemplate())
	{
        LOG("DEC  " + String(readIndex + indexInBlock) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(numSamples));

        
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
        LOG("DEC  " + String(readIndex + indexInBlock) + "\t\t\t\tNew Delta with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(numSamples) + " Index in Block:" + String(indexInBlock));
        
        
        
		compressor->decompress(workBuffer.getWritePointer(), (const uint8*)readBuffer.getData(), numSamples);

		CompressionHelpers::IntVectorOperations::add(workBuffer.getWritePointer(), currentCycle.getReadPointer(), numSamples);
		AudioDataConverters::convertInt16LEToFloat(workBuffer.getReadPointer(), dst, numSamples);
	}

	indexInBlock += numSamples;
    
    
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
	return (headerInfo & 1) > 0;
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
