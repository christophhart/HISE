/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace hlac { using namespace juce; 

void HlacEncoder::compress(AudioSampleBuffer& source, OutputStream& output, uint32* blockOffsetData)
{
	bool compressStereo = source.getNumChannels() == 2;

	if (options.normalisationMode == CompressionHelpers::NormaliseMap::Mode::StaticNormalisation)
	{
		auto maxLevel = source.getMagnitude(0, source.getNumSamples());
		auto db = -1.0f * Decibels::gainToDecibels(maxLevel);
		currentNormaliseBitShiftAmount = jmin<int>(8, (int)(db / 6.0f));
	}
	else
		currentNormaliseBitShiftAmount = 0;

	if (source.getNumSamples() == COMPRESSION_BLOCK_SIZE)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		if (compressStereo)
		{
			auto l = CompressionHelpers::getPart(source, 0, 0, COMPRESSION_BLOCK_SIZE);
			auto r = CompressionHelpers::getPart(source, 1, 0, COMPRESSION_BLOCK_SIZE);

			encodeBlock(l, output);
			encodeBlock(r, output);
		}
		else
			encodeBlock(source, output);

		return;
	}

	blockOffset = 0;
	int32 numSamplesRemaining = source.getNumSamples();

	while (numSamplesRemaining >= COMPRESSION_BLOCK_SIZE)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		uint32 numTodo = jmin<int>(COMPRESSION_BLOCK_SIZE, source.getNumSamples());

		if (compressStereo)
		{
			auto l = CompressionHelpers::getPart(source, 0, blockOffset, numTodo);
			auto r = CompressionHelpers::getPart(source, 1, blockOffset, numTodo);

			encodeBlock(l, output);
			encodeBlock(r, output);
		}
		else
		{
			auto b = CompressionHelpers::getPart(source, blockOffset, numTodo);
			
			encodeBlock(b, output);
		}

		blockOffset += numTodo;

		numSamplesRemaining -= numTodo;
	}

	if (source.getNumSamples() - blockOffset > 0)
	{
		blockOffsetData[blockIndex] = numBytesWritten;
		++blockIndex;

		const int remaining = source.getNumSamples() - blockOffset;

		if (compressStereo)
		{
			auto l = CompressionHelpers::getPart(source, 0, blockOffset, remaining);
			encodeLastBlock(l, output);
			auto r = CompressionHelpers::getPart(source, 1, blockOffset, remaining);
			encodeLastBlock(r, output);
		}
		else
		{
			auto b = CompressionHelpers::getPart(source, blockOffset, remaining);
			encodeLastBlock(b, output);
		}

	}

	
}

void HlacEncoder::reset()
{
	indexInBlock = 0;
	currentCycle = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	blockOffset = 0;
	blockIndex = 0;
	numBytesWritten = 0;
	numBytesUncompressed = 0;
	numTemplates = 0;
	numDeltas = 0;
	blockOffset = 0;
	bitRateForCurrentCycle = 0;
	firstCycleLength = -1;
	ratio = 0.0f;
}


HlacEncoder::~HlacEncoder()
{

}




float HlacEncoder::getCompressionRatio() const
{
	return (float)(numBytesWritten) / (float)(numBytesUncompressed);
}

bool HlacEncoder::encodeBlock(AudioSampleBuffer& block, OutputStream& output)
{
	auto block16 = CompressionHelpers::AudioBufferInt16(block, 0, options.normalisationMode, options.normalisationThreshold);

	if (!normaliseBlockAndAddHeader(block16, output))
		return false;

	return encodeBlock(block16, output);
}

bool HlacEncoder::encodeBlock(CompressionHelpers::AudioBufferInt16& block16, OutputStream& output)
{
	auto compressedBlock = createCompressedBlock(block16);
	auto thisBlockSize = compressedBlock.getSize();

	writeChecksumBytesForBlock(output);
	
	if (thisBlockSize > 2 * COMPRESSION_BLOCK_SIZE)
	{
		writeCycleHeader(true, 16, COMPRESSION_BLOCK_SIZE, output);

		numBytesWritten += sizeof(int16)*COMPRESSION_BLOCK_SIZE + 3;

		return output.write(block16.getReadPointer(), sizeof(int16) * COMPRESSION_BLOCK_SIZE);
	}
	else
	{
		numBytesWritten += (uint32)compressedBlock.getSize();
		return output.write(compressedBlock.getData(), compressedBlock.getSize());
	}
}


bool HlacEncoder::normaliseBlockAndAddHeader(CompressionHelpers::AudioBufferInt16& block16, OutputStream& output)
{
#if HLAC_VERSION > 2
	numBytesWritten += 4;
	return block16.getMap().writeNormalisationHeader(output);
#endif
}

MemoryBlock HlacEncoder::createCompressedBlock(CompressionHelpers::AudioBufferInt16& block16)
{
	jassert(block16.size == COMPRESSION_BLOCK_SIZE);

	MemoryOutputStream blockMos;
	blockMos.preallocate(COMPRESSION_BLOCK_SIZE * 2);

	firstCycleLength = -1;
	
	auto maxBitDepth = CompressionHelpers::getPossibleBitReductionAmount(block16);
	
	numBytesUncompressed += COMPRESSION_BLOCK_SIZE * 2;

	if (maxBitDepth <= options.bitRateForWholeBlock)
	{
		indexInBlock = 0;

		if (options.fixedBlockWidth > 0)
		{
			while (!isBlockExhausted())
			{
				int numRemaining = jmin<int>(options.fixedBlockWidth, COMPRESSION_BLOCK_SIZE - indexInBlock);
				auto part = CompressionHelpers::getPart(block16, indexInBlock, numRemaining);
				encodeCycle(part, blockMos);
				indexInBlock += numRemaining;
			}
		}
		else
		{
			encodeCycle(block16, blockMos);
		}
		
		blockMos.flush();
		return blockMos.getMemoryBlock();
	}


	indexInBlock = 0;

	while (!isBlockExhausted())
	{
		int numRemaining = COMPRESSION_BLOCK_SIZE - indexInBlock;
		auto rest = CompressionHelpers::getPart(block16, indexInBlock, numRemaining);


		if (numRemaining <= 4)
		{
			

			LOG("ENC " + String(numBytesUncompressed / 2) + "\t\tNew Block with bit depth: " + String(maxBitDepth));


			if (!encodeCycleDelta(rest, blockMos))
				return MemoryBlock();

			indexInBlock += numRemaining;

			continue;
		}

		int idealCycleLength;

		if (firstCycleLength < 0)
		{
			if (options.fixedBlockWidth > 0)
				idealCycleLength = options.fixedBlockWidth;
			else
				idealCycleLength = getCycleLength(rest) + 1;
		}
		else
			idealCycleLength = firstCycleLength;

		int cycleLength = idealCycleLength == 0 ? numRemaining : jmin<int>(numRemaining, idealCycleLength);

		currentCycle = CompressionHelpers::getPart(rest, 0, cycleLength);

		bitRateForCurrentCycle = CompressionHelpers::getPossibleBitReductionAmount(currentCycle);

		if (options.useDiffEncodingWithFixedBlocks)
		{
			auto byteAmount = CompressionHelpers::getByteAmountForDifferential(currentCycle);
			auto normalByteAmount = collection.getNumBytesForBitRate(bitRateForCurrentCycle, cycleLength);

			indexInBlock += cycleLength;

			if (byteAmount == normalByteAmount)
				encodeCycle(currentCycle, blockMos);
			else
				encodeDiff(currentCycle, blockMos);

			continue;
		}

		indexInBlock += cycleLength;

		if (!encodeCycle(currentCycle, blockMos))
			return MemoryBlock();

	}

	blockMos.flush();
	return blockMos.getMemoryBlock();
}

uint8 HlacEncoder::getBitReductionAmountForMSEncoding(AudioSampleBuffer& block)
{
	ignoreUnused(block);

	return 0;

#if 0
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
#endif
}


bool HlacEncoder::writeChecksumBytesForBlock(OutputStream& output)
{
	
	auto checkSum = CompressionHelpers::Misc::createChecksum();

	if (!output.writeInt((int)checkSum))
		return false;

	numBytesWritten += 4;
	return true;
}

bool HlacEncoder::writeNormalisationAmount(OutputStream& output)
{
#if HLAC_VERSION > 2
	if (!output.writeByte((uint8)currentNormaliseBitShiftAmount))
		return false;

	numBytesWritten += 1;
	return true;
#else
	return true;
#endif
}

bool HlacEncoder::writeUncompressed(CompressionHelpers::AudioBufferInt16& block, OutputStream& output)
{
	jassert(block.size == COMPRESSION_BLOCK_SIZE);

	writeCycleHeader(true, 16, block.size, output);

	
	

	return output.write(block.getReadPointer(), block.size * 2);
}

bool HlacEncoder::encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	if (cycle.size == 0)
		return true;

	++numTemplates;

	auto compressor = collection.getSuitableCompressorForData(cycle.getReadPointer(), cycle.size);

	auto numBytesToWrite = compressor->getByteAmount(cycle.size);
    
	LOG("ENC  " + String(blockOffset + indexInBlock) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(cycle.size));

	if (!writeCycleHeader(true, compressor->getAllowedBitRange(), cycle.size, output))
		return false;

	if (numBytesToWrite > 0)
	{
		MemoryBlock mb;
		mb.setSize(numBytesToWrite, true);
		compressor->compress((uint8*)mb.getData(), cycle.getReadPointer(), cycle.size);

		bool checkDecompression = false;

		if (checkDecompression)
		{
			CompressionHelpers::AudioBufferInt16 shouldBeZero(cycle.size);

			memcpy(shouldBeZero.getWritePointer(), cycle.getReadPointer(), sizeof(int16)*cycle.size);


			compressor->decompress(shouldBeZero.getWritePointer(), (uint8*)mb.getData(), cycle.size);

			CompressionHelpers::IntVectorOperations::sub(shouldBeZero.getWritePointer(), cycle.getReadPointer(), cycle.size);
			
			jassert(CompressionHelpers::getPossibleBitReductionAmount(shouldBeZero) == 0);

		}
		

		return output.write(mb.getData(), numBytesToWrite);
	}

	return true;
}


bool HlacEncoder::encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output)
{
	jassert(cycle.size % 4 == 0);

	int numFullValues = CompressionHelpers::Diff::getNumFullValues(cycle.size);

	auto packedBuffer = CompressionHelpers::Diff::createBufferWithFullValues(cycle);

	jassert(packedBuffer.size == numFullValues);

	auto compressorFull = collection.getSuitableCompressorForData(packedBuffer.getReadPointer(), packedBuffer.size);
	auto numBytesForFull = compressorFull->getByteAmount(numFullValues);
	auto bitRateFull = compressorFull->getAllowedBitRange();

	auto numErrorValues = CompressionHelpers::Diff::getNumErrorValues(cycle.size);
	auto packedErrorBuffer = CompressionHelpers::Diff::createBufferWithErrorValues(cycle, packedBuffer);

	auto compressorError = collection.getSuitableCompressorForData(packedErrorBuffer.getReadPointer(), packedErrorBuffer.size);
	auto numBytesForError = compressorError->getByteAmount(numErrorValues);
	auto bitRateError = compressorError->getAllowedBitRange();



	if (!writeDiffHeader(bitRateFull, bitRateError, cycle.size, output))
		return false;

	if (numBytesForFull > 0)
	{
		MemoryBlock mbFull;
		mbFull.setSize(numBytesForFull);
		compressorFull->compress((uint8*)mbFull.getData(), packedBuffer.getReadPointer(), numFullValues);

		if (!output.write(mbFull.getData(), numBytesForFull))
			return false;
	}

	LOG("ENC  " + String(blockOffset + indexInBlock) + "\t\t\tNew Diff block bit depth " + String(compressorFull->getAllowedBitRange()) + " -> " + String(compressorError->getAllowedBitRange()) + ": " + String(cycle.size));

	if (numBytesForError > 0)
	{
		MemoryBlock mbError;
		mbError.setSize(numBytesForError);
		compressorError->compress((uint8*)mbError.getData(), packedErrorBuffer.getReadPointer(), numErrorValues);

		

		return output.write(mbError.getData(), numBytesForError);
	}

	return true;
	
}

bool HlacEncoder::encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output)
{
	jassertfalse;

    if(nextCycle.size < 8)
    {
        return encodeCycle(nextCycle, output);
    }
    
	++numDeltas;

	//CompressionHelpers::AudioBufferInt16 workBuffer(nextCycle.size);
	CompressionHelpers::IntVectorOperations::sub(workBuffer.getWritePointer(), nextCycle.getReadPointer(), currentCycle.getReadPointer(), nextCycle.size);
	auto compressor = collection.getSuitableCompressorForData(workBuffer.getReadPointer(), nextCycle.size);
	auto numBytesToWrite = compressor->getByteAmount(nextCycle.size);

	LOG("ENC   " + String(blockOffset + indexInBlock) + "\t\t\t\tSave delta with bit rate " + String(compressor->getAllowedBitRange()) + ": " + String(nextCycle.size));

	if (!writeCycleHeader(false, compressor->getAllowedBitRange(), nextCycle.size, output))
		return false;

	if (numBytesToWrite > 0)
	{
		MemoryBlock mb;
		mb.setSize(numBytesToWrite, true);
		compressor->compress((uint8*)mb.getData(), workBuffer.getReadPointer(), nextCycle.size);
		return output.write(mb.getData(), numBytesToWrite);
	}
    
    return true;
}

bool HlacEncoder::writeCycleHeader(bool isTemplate, int bitDepth, int numSamples, OutputStream& output)
{
	jassert(bitDepth != 15);

	uint8 headerByte = (uint8)bitDepth;
	
	if (isTemplate)
		headerByte |= 0x20;

	if (!output.writeByte(headerByte))
		return false;

	return output.writeShort((int16)numSamples);
}


bool HlacEncoder::writeDiffHeader(int fullBitRate, int errorBitRate, int blockSize, OutputStream& output)
{
	uint8 firstbyte = 0xC0;
	firstbyte |= (uint8)fullBitRate;
	
	if (!output.writeByte(firstbyte))
		return false;

	uint8 secondByte = (uint8)errorBitRate;
	uint8 blockSizeLog = (uint8)log2(blockSize) & 0x0F;
	uint16 shortPacked = ((uint16)secondByte << 8) | (uint16)blockSizeLog;

	return output.writeShort(shortPacked);
}


void HlacEncoder::encodeLastBlock(AudioSampleBuffer& block, OutputStream& output)
{
	CompressionHelpers::AudioBufferInt16 a(block, 0, options.normalisationMode, options.normalisationThreshold);

	normaliseBlockAndAddHeader(a, output);
	writeChecksumBytesForBlock(output);
	
	MemoryOutputStream lastTemp;

	if (options.fixedBlockWidth > 0)
	{
		indexInBlock = 0;

		while (indexInBlock < a.size)
		{
			int numRemaining = jmin<int>(a.size - indexInBlock, options.fixedBlockWidth);

			auto part = CompressionHelpers::getPart(a, indexInBlock, numRemaining);

			encodeCycle(part, lastTemp);
			indexInBlock += numRemaining;
		}
	}

	

	int numZerosToPad = COMPRESSION_BLOCK_SIZE - a.size;

	jassert(numZerosToPad > 0);

	LOG("ENC  PADDING " + String(numZerosToPad));

	writeCycleHeader(true, 0, numZerosToPad, lastTemp);

	lastTemp.flush();

	output.write(lastTemp.getData(), lastTemp.getDataSize());

	numBytesWritten += (uint32)lastTemp.getDataSize();
}


/** Returns the best length for the cycle template.
*
* It calculates the perfect length as fractional number and then returns the upper ceiling value.
* This is so that subsequent cycles can use the additional sample or not depending on the bit reduction amount.
*
* If it can't reduce the bit range it will return the block size
*/
int HlacEncoder::getCycleLength(CompressionHelpers::AudioBufferInt16& block)
{
	int unused;
	return CompressionHelpers::getCycleLengthWithLowestBitRate(block, unused, workBuffer);
}

int HlacEncoder::getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest)
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

} // namespace hlac
