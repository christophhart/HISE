/*
  ==============================================================================

    HiseLosslessAudioFormat.cpp
    Created: 12 Apr 2017 10:15:39pm
    Author:  Christoph

  ==============================================================================
*/


#include "HiseLosslessAudioFormat.h"

void HiseLosslessAudioFormat::compress(AudioSampleBuffer& source, OutputStream& output)
{
	reset();

	//auto bdms = getBitReductionAmountForMSEncoding(source);

	//blockOffset = 40960;

	uint32 numBytesUncompressed = source.getNumSamples() * 2;

	numBytesWritten = 0;

	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);

	while (blockOffset < source.getNumSamples() - COMPRESSION_BLOCK_SIZE)
	{
		auto block = CompressionHelpers::getPart(source, blockOffset, COMPRESSION_BLOCK_SIZE);

		compressBlock(block, output);
		blockOffset += COMPRESSION_BLOCK_SIZE;
		//break;
	}

	auto lastPart = CompressionHelpers::getPart(source, blockOffset, source.getNumSamples() - blockOffset);

	writeUncompressed(lastPart, output);

	ratio = (float)(numBytesWritten) / (float)(numBytesUncompressed);
}

void HiseLosslessAudioFormat::setupForDecompression()
{
	reset();

	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	currentCycle = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	readBuffer.setSize(COMPRESSION_BLOCK_SIZE * 2);

	decompressionSpeed = 0.0;

}

void HiseLosslessAudioFormat::decompress(AudioSampleBuffer& destination, InputStream& input)
{
	double start = Time::getMillisecondCounterHiRes();

	readIndex = 0;

	while (!input.isExhausted())
	{
		auto header = readCycleHeader(input);

		jassert(header.numSamples <= COMPRESSION_BLOCK_SIZE);

		auto compressor = collection.getSuitableCompressorForBitRate(header.getBitRate());
		auto numBytesToRead = compressor->getByteAmount(header.numSamples);

		input.read(readBuffer.getData(), numBytesToRead);

		auto dst = destination.getWritePointer(0, readIndex);

		if (header.isTemplate())
		{
			compressor->decompress(currentCycle.getWritePointer(), (const uint8*)readBuffer.getData(), header.numSamples);
			AudioDataConverters::convertInt16LEToFloat(currentCycle.getReadPointer(), dst, header.numSamples);
		}
		else
		{
			compressor->decompress(workBuffer.getWritePointer(), (const uint8*)readBuffer.getData(), header.numSamples);

			CompressionHelpers::IntVectorOperations::add(workBuffer.getWritePointer(), currentCycle.getReadPointer(), header.numSamples);
			AudioDataConverters::convertInt16LEToFloat(workBuffer.getReadPointer(), dst, header.numSamples);
		}

		readIndex += header.numSamples;
	}

	double stop = Time::getMillisecondCounterHiRes();


	double sampleLength = (double)destination.getNumSamples() / 44100.0;

	double delta = (stop - start) / 1000.0;

	decompressionSpeed = sampleLength / delta;

	Logger::writeToLog("HLAC Decoding Performance: " + String(decompressionSpeed, 1) + "x realtime");
}

void HiseLosslessAudioFormat::reset()
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
}

bool HiseLosslessAudioFormat::compressBlock(AudioSampleBuffer& block, OutputStream& output)
{	
	firstCycleLength = -1;

	auto block16 = CompressionHelpers::AudioBufferInt16(block, false);

	auto maxBitDepth = CompressionHelpers::getPossibleBitReductionAmount(block16);

	LOG(String(blockOffset) + "\t\tNew Block with bit depth: " + String(maxBitDepth));

	if (maxBitDepth <= options.bitRateForWholeBlock)
	{
		return compressCycleAsTemplate(block16, output);
	}

	indexInBlock = 0;

	while (!isBlockExhausted())
	{
		const uint16 numRemaining = COMPRESSION_BLOCK_SIZE - indexInBlock;
		auto rest = CompressionHelpers::getPart(block16, indexInBlock, numRemaining);

		uint16 idealCycleLength;

		if (firstCycleLength < 0)
		{
			if (options.fixedBlockWidth > 0)
				idealCycleLength = options.fixedBlockWidth;
			else
				idealCycleLength = getCycleLength(rest) + 1;
			
			if(options.reuseFirstCycleLengthForBlock)
				firstCycleLength = idealCycleLength;
		}
		else
			idealCycleLength = firstCycleLength;

		uint16 cycleLength = idealCycleLength == 0 ? numRemaining : jmin<int>(numRemaining, idealCycleLength);

		currentCycle = CompressionHelpers::getPart(rest, 0, cycleLength);

		bitRateForCurrentCycle = CompressionHelpers::getPossibleBitReductionAmount(currentCycle);

		if (options.useDiffEncodingWithFixedBlocks)
		{
			auto byteAmount = CompressionHelpers::getByteAmountForDifferential(currentCycle);

			auto normalByteAmount = collection.getNumBytesForBitRate(bitRateForCurrentCycle, cycleLength);

			LOG("Size for diff encoding: " + String(byteAmount));
			LOG("Size for normal encoding: " + String(normalByteAmount));

			numBytesWritten += byteAmount + 3;

			indexInBlock += cycleLength;

			continue;
		}

		

		if (!compressCycleAsTemplate(currentCycle, output))
			return false;

		indexInBlock += cycleLength;

		while (options.useDeltaEncoding && !isBlockExhausted())
		{
			if (numRemaining > 2 * cycleLength)
			{
				auto nextCycle = CompressionHelpers::getPart(block16, indexInBlock, cycleLength);
				uint8 bitReductionDelta = CompressionHelpers::getBitReductionWithTemplate(currentCycle, nextCycle, options.removeDcOffset);

				//DBG("Bit rate reduction for delta storage: " + String(bitReductionDelta));

				//LOG("        Index: " + String(indexInBlock) + " BitReduction: " + String(bitReductionDelta));

				float factor = (float)bitReductionDelta / (float)bitRateForCurrentCycle;

				if (factor > options.deltaCycleThreshhold)
				{
					uint16 nextCycleSize = getCycleLengthFromTemplate(nextCycle, rest);
					nextCycle.size = nextCycleSize;

					compressCycleAsDelta(nextCycle, output);
					indexInBlock += nextCycle.size;
				}
				else
					break;

			}
			else
				break;
		}
	}
}

uint8 HiseLosslessAudioFormat::getBitReductionAmountForMSEncoding(AudioSampleBuffer& block)
{
	if (block.getNumChannels() == 1)
		return 0;

	float** channels = block.getArrayOfWritePointers();

	AudioSampleBuffer lb(channels, 1, block.getNumSamples());
	AudioSampleBuffer lr(channels+1, 1, block.getNumSamples());

	auto l = CompressionHelpers::AudioBufferInt16(lb, false);
	auto r = CompressionHelpers::AudioBufferInt16(lr, false);

	auto bdl = CompressionHelpers::getPossibleBitReductionAmount(l);
	auto bdr = CompressionHelpers::getPossibleBitReductionAmount(r);

	CompressionHelpers::AudioBufferInt16 workBuffer(block.getNumSamples());

	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), l.getReadPointer(), r.getReadPointer(), block.getNumSamples());

	auto bds = CompressionHelpers::getPossibleBitReductionAmount(workBuffer);
	
	if (bds < bdr)
	{
		return bdr - bds;
	}

	return 0;
}

bool HiseLosslessAudioFormat::compressCycleAsTemplate(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	if (cycle.size == 0)
		return true;

	++numTemplates;

	auto compressor = collection.getSuitableCompressorForData(cycle.getReadPointer(), cycle.size);
	uint16 numBytesToWrite = compressor->getByteAmount(cycle.size);

	LOG(String(blockOffset) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(cycle.size));

	if (!writeCycleHeader(true, compressor->getAllowedBitRange(), cycle.size, output))
		return false;

	if (numBytesToWrite > 0)
	{
		numBytesWritten += numBytesToWrite;

		MemoryBlock mb;
		mb.setSize(numBytesToWrite, true);
		compressor->compress((uint8*)mb.getData(), cycle.getReadPointer(), cycle.size);
		return output.write(mb.getData(), numBytesToWrite);
	}
}


bool HiseLosslessAudioFormat::compressCycleAsDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output)
{
	++numDeltas;

	CompressionHelpers::AudioBufferInt16 workBuffer(nextCycle.size);
	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), nextCycle.getReadPointer(), currentCycle.getReadPointer(), nextCycle.size);
	auto compressor = collection.getSuitableCompressorForData(workBuffer.getReadPointer(), nextCycle.size);
	uint16 numBytesToWrite = compressor->getByteAmount(nextCycle.size);

	LOG(String(blockOffset) + "\t\t\t\tSave delta with bit rate " + String(compressor->getAllowedBitRange()) + ": " + String(nextCycle.size));

	if (!writeCycleHeader(false, compressor->getAllowedBitRange(), nextCycle.size, output))
		return false;

	numBytesWritten += numBytesToWrite;

	MemoryBlock mb;
	mb.setSize(numBytesToWrite, true);
	compressor->compress((uint8*)mb.getData(), workBuffer.getReadPointer(), nextCycle.size);
	return output.write(mb.getData(), numBytesToWrite);
}

bool HiseLosslessAudioFormat::writeCycleHeader(bool isTemplate, uint8 bitDepth, uint16 numSamples, OutputStream& output)
{
	uint8 header = isTemplate ? 1 : 0;

	jassert(numSamples <= COMPRESSION_BLOCK_SIZE);

	uint8 bitDepthMoved = ((bitDepth-1) & 15) << 1;
	header |= bitDepthMoved;
	
	if (!output.writeByte(header))
		return false;

	numBytesWritten += 3;

	return output.writeShort(numSamples);
}

HiseLosslessAudioFormat::CycleHeader HiseLosslessAudioFormat::readCycleHeader(InputStream& input)
{
	CycleHeader h;
	h.headerInfo = input.readByte();
	h.numSamples = input.readShort();

	return h;
}

void HiseLosslessAudioFormat::writeUncompressed(AudioSampleBuffer& block, OutputStream& output)
{
	writeCycleHeader(true, 16, block.getNumSamples(), output);

	numBytesWritten += block.getNumSamples() * 2;

	CompressionHelpers::AudioBufferInt16 a(block, false);

	output.write(a.getReadPointer(), sizeof(int16)*a.size);
}


bool HiseLosslessAudioFormat::isBlockExhausted() const
{
	return indexInBlock >= COMPRESSION_BLOCK_SIZE;
}

/** Returns the best length for the cycle template.
*
* It calculates the perfect length as fractional number and then returns the upper ceiling value.
* This is so that subsequent cycles can use the additional sample or not depending on the bit reduction amount.
*
* If it can't reduce the bit range it will return the block size
*/
uint16 HiseLosslessAudioFormat::getCycleLength(CompressionHelpers::AudioBufferInt16& block)
{
	uint8 unused;
	return CompressionHelpers::getCycleLengthWithLowestBitRate(block, unused, workBuffer);
}

uint16 HiseLosslessAudioFormat::getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest)
{
	auto candidate1 = CompressionHelpers::getPart(rest, newCycle.size - 1, newCycle.size);
	auto candidate2 = CompressionHelpers::getPart(rest, newCycle.size - 2, newCycle.size);

	auto br1 = CompressionHelpers::getBitReductionWithTemplate(currentCycle, candidate1, options.removeDcOffset);
	auto br2 = CompressionHelpers::getBitReductionWithTemplate(currentCycle, candidate2, options.removeDcOffset);

	if (br1 > br2)
	{
		return newCycle.size - 1;
	}
	else
	{
		return newCycle.size;
	}
}

bool HiseLosslessAudioFormat::CycleHeader::isTemplate() const
{
	return headerInfo & 1 > 0;
}

uint8 HiseLosslessAudioFormat::CycleHeader::getBitRate()
{
	return headerInfo >> 1;
}
