/*
  ==============================================================================

    HlacEncoder.cpp
    Created: 16 Apr 2017 10:18:36am
    Author:  Christoph

  ==============================================================================
*/

#include "HlacEncoder.h"



void HlacEncoder::compress(AudioSampleBuffer& source, OutputStream& output)
{
	reset();

	//auto bdms = getBitReductionAmountForMSEncoding(source);

	//blockOffset = 40960;

	uint32 numBytesUncompressed = source.getNumSamples() * 2;

	numBytesWritten = 0;

	uint32 numSamplesRemaining = 0;

	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);

	while (blockOffset <= source.getNumSamples() - COMPRESSION_BLOCK_SIZE)
	{
		auto block = CompressionHelpers::getPart(source, blockOffset, COMPRESSION_BLOCK_SIZE);

		encodeBlock(block, output);
		blockOffset += COMPRESSION_BLOCK_SIZE;
	}

	if (source.getNumSamples() - blockOffset > 0)
	{
		auto lastPart = CompressionHelpers::getPart(source, blockOffset, source.getNumSamples() - blockOffset);

		writeUncompressed(lastPart, output);
	}


	ratio = (float)(numBytesWritten) / (float)(numBytesUncompressed);
}


void HlacEncoder::reset()
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


bool HlacEncoder::encodeBlock(AudioSampleBuffer& block, OutputStream& output)
{
	firstCycleLength = -1;

	auto block16 = CompressionHelpers::AudioBufferInt16(block, false);

	auto maxBitDepth = CompressionHelpers::getPossibleBitReductionAmount(block16);

	LOG(String(blockOffset) + "\t\tNew Block with bit depth: " + String(maxBitDepth));

	if (maxBitDepth <= options.bitRateForWholeBlock)
	{
		indexInBlock = 0;
		return encodeCycle(block16, output);
	}

	indexInBlock = 0;

	while (!isBlockExhausted())
	{
		const uint16 numRemaining = COMPRESSION_BLOCK_SIZE - indexInBlock;
		auto rest = CompressionHelpers::getPart(block16, indexInBlock, numRemaining);

		if (numRemaining <= 4)
		{
			if (!encodeCycleDelta(rest, output))
				return false;

			indexInBlock += numRemaining;

			continue;
		}

		uint16 idealCycleLength;

		if (firstCycleLength < 0)
		{
			if (options.fixedBlockWidth > 0)
				idealCycleLength = options.fixedBlockWidth;
			else
				idealCycleLength = getCycleLength(rest) + 1;

			if (options.reuseFirstCycleLengthForBlock)
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

			indexInBlock += cycleLength;

			if (byteAmount == normalByteAmount)
				encodeCycle(currentCycle, output);
			else
				encodeDiff(currentCycle, output);

			continue;
		}

		indexInBlock += cycleLength;

		if (!encodeCycle(currentCycle, output))
			return false;


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

					indexInBlock += nextCycle.size;

					encodeCycleDelta(nextCycle, output);

				}
				else
					break;

			}
			else
				break;
		}
	}
}

uint8 HlacEncoder::getBitReductionAmountForMSEncoding(AudioSampleBuffer& block)
{
	if (block.getNumChannels() == 1)
		return 0;

	float** channels = block.getArrayOfWritePointers();

	AudioSampleBuffer lb(channels, 1, block.getNumSamples());
	AudioSampleBuffer lr(channels + 1, 1, block.getNumSamples());

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

bool HlacEncoder::encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	if (cycle.size == 0)
		return true;

	++numTemplates;

	auto compressor = collection.getSuitableCompressorForData(cycle.getReadPointer(), cycle.size);

	auto br = compressor->getAllowedBitRange();

	uint16 numBytesToWrite = compressor->getByteAmount(cycle.size);

	LOG(" " + String(blockOffset + indexInBlock) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(cycle.size));

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


bool HlacEncoder::encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	jassert(cycle.size % 4 == 0);

	auto pb = cycle.getWritePointer(0);

	uint16 numFullValues = CompressionHelpers::Diff::getNumFullValues(cycle.size);

	auto packedBuffer = CompressionHelpers::Diff::createBufferWithFullValues(cycle);

	jassert(packedBuffer.size == numFullValues);

	auto compressorFull = collection.getSuitableCompressorForData(packedBuffer.getReadPointer(), packedBuffer.size);
	auto numBytesForFull = compressorFull->getByteAmount(numFullValues);
	auto bitRateFull = compressorFull->getAllowedBitRange();

	uint16 numErrorValues = CompressionHelpers::Diff::getNumErrorValues(cycle.size);
	auto packedErrorBuffer = CompressionHelpers::Diff::createBufferWithErrorValues(cycle, packedBuffer);

	auto compressorError = collection.getSuitableCompressorForData(packedErrorBuffer.getReadPointer(), packedErrorBuffer.size);
	auto numBytesForError = compressorError->getByteAmount(numErrorValues);
	auto bitRateError = compressorError->getAllowedBitRange();



	if (!writeDiffHeader(bitRateFull, bitRateError, cycle.size, output))
		return false;

	MemoryBlock mbFull;
	mbFull.setSize(numBytesForFull);
	compressorFull->compress((uint8*)mbFull.getData(), packedBuffer.getReadPointer(), numFullValues);

	if (!output.write(mbFull.getData(), numBytesForFull))
		return false;

	numBytesWritten += numBytesForFull;

	MemoryBlock mbError;
	mbError.setSize(numBytesForError);
	compressorError->compress((uint8*)mbError.getData(), packedErrorBuffer.getReadPointer(), numErrorValues);

	if (!output.write(mbError.getData(), numBytesForError))
		return false;

	numBytesWritten += numBytesForError;

	LOG(" " + String(blockOffset + indexInBlock) + "\t\t\tNew Diff block bit depth " + String(compressorFull->getAllowedBitRange()) + " -> " + String(compressorError->getAllowedBitRange()) + ": " + String(cycle.size));

	return true;
}

bool HlacEncoder::encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output)
{
	++numDeltas;

	CompressionHelpers::AudioBufferInt16 workBuffer(nextCycle.size);
	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), nextCycle.getReadPointer(), currentCycle.getReadPointer(), nextCycle.size);
	auto compressor = collection.getSuitableCompressorForData(workBuffer.getReadPointer(), nextCycle.size);
	uint16 numBytesToWrite = compressor->getByteAmount(nextCycle.size);

	LOG("  " + String(blockOffset + indexInBlock) + "\t\t\t\tSave delta with bit rate " + String(compressor->getAllowedBitRange()) + ": " + String(nextCycle.size));

	if (!writeCycleHeader(false, compressor->getAllowedBitRange(), nextCycle.size, output))
		return false;

	numBytesWritten += numBytesToWrite;

	MemoryBlock mb;
	mb.setSize(numBytesToWrite, true);
	compressor->compress((uint8*)mb.getData(), workBuffer.getReadPointer(), nextCycle.size);
	return output.write(mb.getData(), numBytesToWrite);
}

bool HlacEncoder::writeCycleHeader(bool isTemplate, uint8 bitDepth, uint16 numSamples, OutputStream& output)
{
	if (bitDepth == 0)
	{
		if (!output.writeByte(1))
			return false;
	}
	else if (bitDepth == 1)
	{
		if (!output.writeByte(3))
			return false;
	}
	else if (bitDepth == 2)
	{
		if (!output.writeByte(5))
			return false;
	}
	else
	{
		uint8 header = isTemplate ? 1 : 0;

		jassert(numSamples <= COMPRESSION_BLOCK_SIZE);

		uint8 bitDepthMoved = ((bitDepth - 1) & 15) << 1;
		header |= bitDepthMoved;

		jassert(header != 0b11100000); // Reserved for diff

		if (!output.writeByte(header))
			return false;
	}

	numBytesWritten += 3;

	return output.writeShort(numSamples);
}


bool HlacEncoder::writeDiffHeader(uint8 fullBitRate, uint8 errorBitRate, uint16 blockSize, OutputStream& output)
{
	if (!output.writeByte(0b11100000))
		return false;

	uint8 bitRates = (fullBitRate - 1) << 4;
	bitRates |= (errorBitRate - 1);

	uint8 blockSizeLog = log2(blockSize);

	uint16 shortPacked = bitRates << 8;
	shortPacked |= blockSizeLog;

	output.writeShort(shortPacked);

	numBytesWritten += 3;

	return true;
}


void HlacEncoder::writeUncompressed(AudioSampleBuffer& block, OutputStream& output)
{
	CompressionHelpers::AudioBufferInt16 a(block, false);

	encodeCycle(a, output);
}


/** Returns the best length for the cycle template.
*
* It calculates the perfect length as fractional number and then returns the upper ceiling value.
* This is so that subsequent cycles can use the additional sample or not depending on the bit reduction amount.
*
* If it can't reduce the bit range it will return the block size
*/
uint16 HlacEncoder::getCycleLength(CompressionHelpers::AudioBufferInt16& block)
{
	uint8 unused;
	return CompressionHelpers::getCycleLengthWithLowestBitRate(block, unused, workBuffer);
}

uint16 HlacEncoder::getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest)
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
