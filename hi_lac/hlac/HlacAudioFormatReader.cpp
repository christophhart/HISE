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

HiseLosslessAudioFormatReader::HiseLosslessAudioFormatReader(InputStream* input_) :
	AudioFormatReader(input_, "HLAC"),
	internalReader(input)
{
	numChannels = internalReader.header.getNumChannels();
	sampleRate = internalReader.header.getSampleRate();
	bitsPerSample = internalReader.header.getBitsPerSample();
	lengthInSamples = internalReader.header.getBlockAmount() * COMPRESSION_BLOCK_SIZE;
	usesFloatingPointData = true;
}

bool HiseLosslessAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
    return internalReader.internalHlacRead(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile, numSamples);
}


void HiseLosslessAudioFormatReader::setTargetAudioDataType(AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == AudioDataConverters::DataFormat::float32BE) ||
		(dataType == AudioDataConverters::DataFormat::float32LE);

	internalReader.setTargetAudioDataType(dataType);
}



uint32 HiseLosslessHeader::getOffsetForReadPosition(int64 samplePosition, bool addHeaderOffset)
{
	if (samplePosition % COMPRESSION_BLOCK_SIZE == 0)
	{
		uint32 blockIndex = (uint32)samplePosition / COMPRESSION_BLOCK_SIZE;

		if (blockIndex < blockAmount)
		{
			return addHeaderOffset ? (headerSize + blockOffsets[blockIndex]) : blockOffsets[blockIndex];
		}
		else
		{
			jassertfalse;
			return 0;
		}
	}
	else
	{
		auto blockIndex = (uint32)samplePosition / COMPRESSION_BLOCK_SIZE;

		if (blockIndex < blockAmount)
		{
			return addHeaderOffset ? (headerSize + blockOffsets[blockIndex]) : blockOffsets[blockIndex];
		}
		else
		{
			jassertfalse;
			return 0;
		}

		jassertfalse;
	}

	return 0;
}

uint32 HiseLosslessHeader::getOffsetForNextBlock(int64 samplePosition, bool addHeaderOffset)
{
	if (samplePosition % COMPRESSION_BLOCK_SIZE == 0)
	{
		uint32 blockIndex = (uint32)samplePosition / COMPRESSION_BLOCK_SIZE;

		if (blockIndex < blockAmount-1)
		{
			return addHeaderOffset ? (headerSize + blockOffsets[blockIndex+1]) : blockOffsets[blockIndex+1];
		}
		else
		{
			jassertfalse;
			return 0;
		}
	}
	else
	{
		auto blockIndex = (uint32)samplePosition / COMPRESSION_BLOCK_SIZE;

		if (blockIndex < blockAmount-1)
		{
			return addHeaderOffset ? (headerSize + blockOffsets[blockIndex+1]) : blockOffsets[blockIndex+1];
		}
		else
		{
			jassertfalse;
			return 0;
		}

		jassertfalse;
	}

	return 0;
}

void HlacReaderCommon::setTargetAudioDataType(AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == AudioDataConverters::DataFormat::float32BE) ||
		(dataType == AudioDataConverters::DataFormat::float32LE);
}

bool HlacReaderCommon::internalHlacRead(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	ignoreUnused(startSampleInFile);
	ignoreUnused(numDestChannels);

	bool isStereo = destSamples[1] != nullptr;

	if (startSampleInFile != decoder.getCurrentReadPosition())
	{
		auto byteOffset = header.getOffsetForReadPosition(startSampleInFile, useHeaderOffsetWhenSeeking);

		decoder.seekToPosition(*input, (uint32)startSampleInFile, byteOffset);
	}

	if (isStereo)
	{
		float** destinationFloat = reinterpret_cast<float**>(destSamples);

		if (startOffsetInDestBuffer > 0)
		{
			if (isStereo)
			{
				destinationFloat[0] = destinationFloat[0] + startOffsetInDestBuffer;
			}
			else
			{
				destinationFloat[0] = destinationFloat[0] + startOffsetInDestBuffer;
				destinationFloat[1] = destinationFloat[1] + startOffsetInDestBuffer;
			}
		}

		AudioSampleBuffer b(destinationFloat, 2, numSamples);

		decoder.decode(b, *input, (int)startSampleInFile, numSamples);

	}
	else
	{
		float* destinationFloat = reinterpret_cast<float*>(destSamples[0]);

		AudioSampleBuffer b(&destinationFloat, 1, numSamples);

		decoder.decode(b, *input, (int)startSampleInFile, numSamples);
	}

	return true;
}

bool HlacMemoryMappedAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	if (internalReader.input != nullptr)
	{
		return internalReader.internalHlacRead(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile, numSamples);
	}

	// You have to call mapEverythingAndCreateMemoryStream() before using this method
	jassertfalse;
	return false;
}


bool HlacMemoryMappedAudioFormatReader::mapSectionOfFile(Range<int64> samplesToMap)
{
	dataChunkStart = (int64)internalReader.header.getOffsetForReadPosition(0, true);
	dataLength = getFile().getSize() - dataChunkStart;

	int64 start = (int64)internalReader.header.getOffsetForReadPosition(samplesToMap.getStart(), true);
	int64 end = 0;

	if (samplesToMap.getEnd() >= lengthInSamples)
	{
		end = getFile().getSize();
	}
	else
	{
		end = internalReader.header.getOffsetForNextBlock(samplesToMap.getEnd(), true);
	}

	auto fileRange = Range<int64>(start, end);
	
	map = new MemoryMappedFile(getFile(), fileRange, MemoryMappedFile::readOnly, false);

	if (map != nullptr && !map->getRange().isEmpty())
	{
		int64 mappedStart = samplesToMap.getStart() / COMPRESSION_BLOCK_SIZE;

		int64 mappedEnd = jmin<int64>(lengthInSamples, samplesToMap.getEnd() - (samplesToMap.getEnd() % COMPRESSION_BLOCK_SIZE) + 1);
		mappedSection = Range<int64>(mappedStart, mappedEnd);

		auto actualMappedRange = map->getRange();

		int offset = fileRange.getStart() - actualMappedRange.getStart();
		int length = actualMappedRange.getLength() - offset;

		mis = new MemoryInputStream((uint8*)map->getData() + offset, length, false);

		internalReader.input = mis;

		internalReader.setUseHeaderOffsetWhenSeeking(false);

		return true;

	}

	return false;
}

void HlacMemoryMappedAudioFormatReader::mapEverythingAndCreateMemoryStream()
{
	jassertfalse;

	return;

	dataChunkStart = (int64)internalReader.header.getOffsetForReadPosition(0, true);
	dataLength = getFile().getSize() - dataChunkStart;



	auto fileRange = Range<int64>(dataChunkStart, dataChunkStart + dataLength);

	map = new MemoryMappedFile(getFile(), fileRange, MemoryMappedFile::readOnly, false);

	if (map != nullptr && !map->getRange().isEmpty())
	{
		mappedSection = Range<int64>(0, internalReader.header.getBlockAmount() * COMPRESSION_BLOCK_SIZE);

		auto actualMappedRange = map->getRange();
		
		int offset = fileRange.getStart() - actualMappedRange.getStart();
		int length = actualMappedRange.getLength() - offset;

		jassert(length == dataLength);

		mis = new MemoryInputStream((uint8*)map->getData() + offset, length, false);

		internalReader.input = mis;

		internalReader.setUseHeaderOffsetWhenSeeking(false);
	}
}
