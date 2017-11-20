/*  HISE Lossless Audio Codec
*	�2017 Christoph Hart
*
*	Redistribution and use in source and binary forms, with or without modification,
*	are permitted provided that the following conditions are met:
*
*	1. Redistributions of source code must retain the above copyright notice,
*	   this list of conditions and the following disclaimer.
*
*	2. Redistributions in binary form must reproduce the above copyright notice,
*	   this list of conditions and the following disclaimer in the documentation
*	   and/or other materials provided with the distribution.
*
*	3. All advertising materials mentioning features or use of this software must
*	   display the following acknowledgement:
*	   This product includes software developed by Hart Instruments
*
*	4. Neither the name of the copyright holder nor the names of its contributors may be used
*	   to endorse or promote products derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY CHRISTOPH HART "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
*	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*	DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

namespace hlac { using namespace juce; 

void HlacDecoder::setupForDecompression()
{
	reset();

	workBuffer = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	currentCycle = CompressionHelpers::AudioBufferInt16(COMPRESSION_BLOCK_SIZE);
	readBuffer.setSize(COMPRESSION_BLOCK_SIZE * 2);
	readIndex = 0;

	decompressionSpeed = 0.0;

}


double HlacDecoder::getDecompressionPerformance() const
{
	if(decompressionSpeeds.size() <= 1)
		return decompressionSpeed;

	double sum = 0.0;

	for (int i = 0; i < decompressionSpeeds.size(); i++)
	{
		sum += decompressionSpeeds[i];
	}

	sum = sum / (double)decompressionSpeeds.size();

	return sum;
}

void HlacDecoder::reset()
{
	indexInBlock = 0;
	currentCycle = CompressionHelpers::AudioBufferInt16(0);
	workBuffer = CompressionHelpers::AudioBufferInt16(0);
	blockOffset = 0;
	blockOffset = 0;
	bitRateForCurrentCycle = 0;
	firstCycleLength = -1;
	ratio = 0.0f;
	readOffset = 0;
}


bool HlacDecoder::decodeBlock(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int channelIndex)
{
	auto checksum = input.readInt();

	if (!CompressionHelpers::Misc::validateChecksum((uint32)checksum))
	{
		// Something is wrong here...
		jassertfalse;
	}

	indexInBlock = 0;
	
	const int floatIndexToUse = channelIndex == 0 ? leftFloatIndex : rightFloatIndex;

	int numTodo = jmin<int>(destination.getNumSamples() - floatIndexToUse, COMPRESSION_BLOCK_SIZE);

    LOG("DEC " + String(readOffset + readIndex) + "\t\tNew Block");
    
	while (indexInBlock < COMPRESSION_BLOCK_SIZE)
	{
		auto header = readCycleHeader(input);

		jassert(header.getNumSamples() != 0);
		jassert(header.getNumSamples() <= COMPRESSION_BLOCK_SIZE);

		if (header.isDiff())
			decodeDiff(header, decodeStereo, destination, input, channelIndex);
		else
			decodeCycle(header, decodeStereo, destination, input, channelIndex);
        
        jassert(indexInBlock <= 4096);
	}

	if(!decodeStereo || channelIndex == 1)
		readIndex += indexInBlock;

	return numTodo != 0;
}

void HlacDecoder::decode(HiseSampleBuffer& destination, bool decodeStereo, InputStream& input, int offsetInSource/*=0*/, int numSamples/*=-1*/)
{
#if HLAC_MEASURE_DECODING_PERFORMANCE
	double start = Time::getMillisecondCounterHiRes();
#endif

	if (numSamples < 0)
		numSamples = destination.getNumSamples();
	
	

	const int endThisTime = numSamples + offsetInSource;

	// You must seek on a higher level!
	//jassert(offsetInSource == readOffset);

	readIndex = 0;
	leftNumToSkip = offsetInSource - readOffset;
	rightNumToSkip = leftNumToSkip;

	leftFloatIndex = 0;
	rightFloatIndex = 0;

	int channelIndex = 0;
	

	while (!input.isExhausted() && readOffset + readIndex < endThisTime)
	{
		if (!decodeBlock(destination, decodeStereo, input, channelIndex))
			break;

		if (decodeStereo)
			channelIndex = 1 - channelIndex;
	}

	readOffset += readIndex;

#if HLAC_MEASURE_DECODING_PERFORMANCE
	double stop = Time::getMillisecondCounterHiRes();
	double sampleLength = (double)numSamples / 44100.0;
	double delta = (stop - start) / 1000.0;

	decompressionSpeed = sampleLength / delta;

	decompressionSpeeds.add(decompressionSpeed);

	//Logger::writeToLog("HLAC Decoding Performance: " + String(decompressionSpeed, 1) + "x realtime");
#endif
}

void HlacDecoder::decodeDiff(const CycleHeader& header, bool /*decodeStereo*/, HiseSampleBuffer& destination, InputStream& input, int channelIndex)
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

	LOG("DEC  " + String(readOffset + readIndex + indexInBlock) + "\t\t\tNew diff with bit depth " + String(fullBitRate) + "/" + String(errorBitRate) + ": " + String(blockSize));

	if (errorBitRate > 0)
	{
		auto compressorError = collection.getSuitableCompressorForBitRate(errorBitRate);
		auto numErrorValues = CompressionHelpers::Diff::getNumErrorValues(blockSize);
		auto numErrorBytes = compressorError->getByteAmount(numErrorValues);

		input.read(readBuffer.getData(), numErrorBytes);

		compressorError->decompress(workBuffer.getWritePointer(), (uint8*)readBuffer.getData(), numErrorValues);

		CompressionHelpers::Diff::addErrorSignal(currentCycle, (const uint16*)workBuffer.getReadPointer(), numErrorValues);
	}

	writeToFloatArray(true, false, destination, channelIndex, blockSize);

	indexInBlock += blockSize;
}




void HlacDecoder::decodeCycle(const CycleHeader& header, bool /*decodeStereo*/, HiseSampleBuffer& destination, InputStream& input, int channelIndex)
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

	

	if (header.isTemplate())
	{
        LOG("DEC  " + String(readOffset + readIndex + indexInBlock) + "\t\t\tNew Template with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(numSamples));

        if (compressor->getAllowedBitRange() != 0)
		{
			compressor->decompress(currentCycle.getWritePointer(), (const uint8*)readBuffer.getData(), numSamples);

			writeToFloatArray(true, false, destination, channelIndex, numSamples);
		}
		else
		{
			writeToFloatArray(false, false, destination, channelIndex, numSamples);
		}
	}
	else
	{
        LOG("DEC  " + String(readOffset + indexInBlock) + "\t\t\t\tNew Delta with bit depth " + String(compressor->getAllowedBitRange()) + ": " + String(numSamples) + " Index in Block:" + String(indexInBlock));
        
		if (compressor->getAllowedBitRange() > 0)
		{
			compressor->decompress(workBuffer.getWritePointer(), (const uint8*)readBuffer.getData(), numSamples);

			CompressionHelpers::IntVectorOperations::add(workBuffer.getWritePointer(), currentCycle.getReadPointer(), numSamples);
			
			writeToFloatArray(true, true, destination, channelIndex, numSamples);
		}
		else
		{
			writeToFloatArray(true, false, destination, channelIndex, numSamples);
		}
	}

	indexInBlock += numSamples;
}


void HlacDecoder::writeToFloatArray(bool shouldCopy, bool useTempBuffer, HiseSampleBuffer& destination, int channelIndex, int numSamples)
{
	auto src = useTempBuffer ? workBuffer.getReadPointer() : currentCycle.getReadPointer();

	int& skipToUse = channelIndex == 0 ? leftNumToSkip : rightNumToSkip;

	if (skipToUse == 0)
	{
		auto bufferOffset = channelIndex == 0 ? leftFloatIndex : rightFloatIndex;

		auto numThisTime = jmin<int>(numSamples, destination.getNumSamples() - bufferOffset);

		if (numThisTime > 0)
		{
			if (shouldCopy)
			{
				if (destination.isFloatingPoint())
				{
					auto dst = static_cast<float*>(destination.getWritePointer(channelIndex, bufferOffset));
					CompressionHelpers::fastInt16ToFloat(src, dst, numThisTime);
				}
				else
				{
					auto dst = static_cast<int16*>(destination.getWritePointer(channelIndex, bufferOffset));
					memcpy(dst, src, sizeof(int16) * numThisTime);
				}
			}
				
			else
			{
				if (destination.isFloatingPoint())
					FloatVectorOperations::clear(static_cast<float*>(destination.getWritePointer(channelIndex, bufferOffset)), numThisTime);
				else
					CompressionHelpers::IntVectorOperations::clear(static_cast<int16*>(destination.getWritePointer(channelIndex, bufferOffset)), numThisTime);
			}
				


			if (channelIndex == 0)
				leftFloatIndex += numThisTime;
			else
				rightFloatIndex += numThisTime;
		}
	}
	else
	{
		if (skipToUse > numSamples)
		{
			skipToUse -= numSamples;
		}
		else
		{
			auto bufferOffset = readIndex;
            int numThisTime = jmin<int>(numSamples - skipToUse, destination.getNumSamples() - bufferOffset);
			//leftSeekOffset = numToSkipLeft;

			if (shouldCopy)
			{
				if (destination.isFloatingPoint())
				{
					auto dst = static_cast<float*>(destination.getWritePointer(channelIndex, bufferOffset));
					CompressionHelpers::fastInt16ToFloat(src + skipToUse, dst, numThisTime);
				}
				else
				{
					auto dst = static_cast<int16*>(destination.getWritePointer(channelIndex, bufferOffset));
					memcpy(dst, src + skipToUse, sizeof(int16) * numThisTime);
				}
			}
				
			else
			{
				if (destination.isFloatingPoint())
					FloatVectorOperations::clear(static_cast<float*>(destination.getWritePointer(channelIndex, bufferOffset)), numThisTime);
				else
					CompressionHelpers::IntVectorOperations::clear(static_cast<int16*>(destination.getWritePointer(channelIndex, bufferOffset)), numThisTime);
			}
				


			if (channelIndex == 0)
				leftFloatIndex += numThisTime;
			else
				rightFloatIndex += numThisTime;

			skipToUse = 0;
		}
	}
}

void HlacDecoder::seekToPosition(InputStream& input, uint32 position, uint32 byteOffset)
{
	if (position % COMPRESSION_BLOCK_SIZE == 0)
	{
		input.setPosition(byteOffset);
		readOffset = position;
	}
	else
	{
		const int blockStart = (position / COMPRESSION_BLOCK_SIZE) * COMPRESSION_BLOCK_SIZE;
		input.setPosition(byteOffset);
		readOffset = blockStart;
	}
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
	return (headerInfo & 0x20) > 0;
}

uint8 HlacDecoder::CycleHeader::getBitRate(bool getFullBitRate) const
{
	if (isDiff())
	{
		if (getFullBitRate)
			return headerInfo & 0x1F;

		else
		{
			uint8 b = (uint8)((numSamples & 0xFF00) >> 8);
			return b & 0x1f;
		}
	}
	else
	{
		return headerInfo & 0x1F;
	}
}

bool HlacDecoder::CycleHeader::isDiff() const
{
	return (headerInfo & 0xC0) > 0;
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

} // namespace hlac