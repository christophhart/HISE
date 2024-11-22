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

HiseLosslessAudioFormatReader::HiseLosslessAudioFormatReader(InputStream* input_) :
	AudioFormatReader(input_, "HLAC"),
	internalReader(input_)
{
	numChannels = internalReader.header.getNumChannels();
	sampleRate = internalReader.header.getSampleRate();
	bitsPerSample = internalReader.header.getBitsPerSample();
	lengthInSamples = internalReader.header.getBlockAmount() * COMPRESSION_BLOCK_SIZE;
	usesFloatingPointData = true;
	isMonolith = internalReader.header.getVersion() < 2;

	if (isMonolith)
	{
		lengthInSamples = (input_->getTotalLength() - 1) / numChannels / sizeof(int16);
	}
}

bool HiseLosslessAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	if (isMonolith)
	{
		clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (numSamples <= 0)
			return true;

		const int bytesPerFrame = sizeof(int16) * numChannels;

		input->setPosition(1 + startSampleInFile * bytesPerFrame);

		while (numSamples > 0)
		{
			const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
			char tempBuffer[tempBufSize];

			const int numThisTime = jmin(tempBufSize / bytesPerFrame, numSamples);
			const int bytesRead = input->read(tempBuffer, numThisTime * bytesPerFrame);

			if (bytesRead < numThisTime * bytesPerFrame)
			{
				jassert(bytesRead >= 0);
				zeromem(tempBuffer + bytesRead, (size_t)(numThisTime * bytesPerFrame - bytesRead));
			}

			copySampleData(destSamples, startOffsetInDestBuffer, numDestChannels,
				tempBuffer, (int)numChannels, numThisTime);

			startOffsetInDestBuffer += numThisTime;
			numSamples -= numThisTime;
		}

		return true;
	}
	else
	{
		return internalReader.internalHlacRead(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile, numSamples);
	}
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
	}
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
	}
}

HiseLosslessHeader HiseLosslessHeader::createMonolithHeader(int numChannels, double sampleRate)
{
	HiseLosslessHeader monoHeader(false, 0, sampleRate, numChannels, 16, false, 0);

	monoHeader.blockAmount = 0;
	monoHeader.headerByte1 = numChannels == 2 ? 0 : 1;
	monoHeader.headerByte2 = 0;
	monoHeader.headerSize = 1;

	return monoHeader;
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

	decoder.setHlacVersion(header.getVersion());

	bool isStereo = destSamples[1] != nullptr;

	if (startSampleInFile != decoder.getCurrentReadPosition())
	{
		auto byteOffset = header.getOffsetForReadPosition(startSampleInFile, useHeaderOffsetWhenSeeking);

		decoder.seekToPosition(*input, (uint32)startSampleInFile, byteOffset);
	}

	if (isStereo)
	{
		if (usesFloatingPointData)
		{
			float** destinationFloat = reinterpret_cast<float**>(destSamples);

			destinationFloat[0] += startOffsetInDestBuffer;
			destinationFloat[1] += startOffsetInDestBuffer;

			AudioSampleBuffer b(destinationFloat, 2, numSamples);
			HiseSampleBuffer hsb(b);

			decoder.decode(hsb, true, *input, (int)startSampleInFile, numSamples);
		}
		else
		{
			int16** destinationFixed = reinterpret_cast<int16**>(destSamples);

			destinationFixed[0] += startOffsetInDestBuffer;
			destinationFixed[1] += startOffsetInDestBuffer;

			HiseSampleBuffer hsb(destinationFixed, 2, numSamples);
			
			decoder.decode(hsb, true, *input, (int)startSampleInFile, numSamples);
		}
	}
	else
	{
		if (usesFloatingPointData)
		{
			float* destinationFloat = reinterpret_cast<float*>(destSamples[0]);

			AudioSampleBuffer b(&destinationFloat, 1, numSamples);
			HiseSampleBuffer hsb(b);
			hsb.allocateNormalisationTables((int)startSampleInFile);

			decoder.decode(hsb, false, *input, (int)startSampleInFile, numSamples);
		}
		else
		{
			int16** destinationFixed = reinterpret_cast<int16**>(destSamples);

			HiseSampleBuffer hsb(destinationFixed, 1, numSamples);
			hsb.allocateNormalisationTables((int)startSampleInFile);

			decoder.decode(hsb, false, *input, (int)startSampleInFile, numSamples);
		}
	}

	return true;
}

bool HlacReaderCommon::fixedBufferRead(HiseSampleBuffer& buffer, int numDestChannels, int startOffsetInBuffer, int64 startSampleInFile, int numSamples)
{
	bool isStereo = numDestChannels == 2;

	if (startSampleInFile < 0)
	{
		auto silence = (int)jmin(-startSampleInFile, (int64)numSamples);

		auto numToClear = jmin(silence, buffer.getNumSamples() - startOffsetInBuffer);

		buffer.clear(startOffsetInBuffer, numToClear);

		startOffsetInBuffer += silence;
		numSamples -= silence;
		startSampleInFile = 0;
	}

	if (numSamples == 0)
		return true;

	if (startSampleInFile != decoder.getCurrentReadPosition())
	{
		auto byteOffset = header.getOffsetForReadPosition(startSampleInFile, useHeaderOffsetWhenSeeking);

		decoder.seekToPosition(*input, (uint32)startSampleInFile, byteOffset);
	}

	decoder.setHlacVersion(header.getVersion());

	if(startOffsetInBuffer == 0)
		decoder.decode(buffer, isStereo, *input, (int)startSampleInFile, numSamples);
	else
	{
		HiseSampleBuffer offset(buffer, startOffsetInBuffer);
		decoder.decode(offset, isStereo, *input, (int)startSampleInFile, numSamples);
		buffer.copyNormalisationRanges(offset, startOffsetInBuffer);
	}

	return true;
}

void HiseLosslessAudioFormatReader::copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept
{
	jassert(numDestChannels == numDestChannels);

	if (numChannels == 1)
	{
		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
	}
	else
	{
		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
	}
}

bool HiseLosslessAudioFormatReader::copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64 offsetInFile, int numChannelsToCopy, int numSamples)
{
	if (numSamples <= 0)
		return true;

	const int bytesPerFrame = sizeof(int16) * numChannelsToCopy;

	input->setPosition(1 + offsetInFile * bytesPerFrame);

	while (numSamples > 0)
	{
		const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
		char tempBuffer[tempBufSize];

		const int numThisTime = jmin(tempBufSize / bytesPerFrame, numSamples);
		const int bytesRead = input->read(tempBuffer, numThisTime * bytesPerFrame);

		if (bytesRead < numThisTime * bytesPerFrame)
		{
			jassert(bytesRead >= 0);
			zeromem(tempBuffer + bytesRead, (size_t)(numThisTime * bytesPerFrame - bytesRead));
		}



		//copySampleData(destSamples, startOffsetInDestBuffer, numDestChannels,
		//	tempBuffer, (int)numChannels, numThisTime);

		if (numChannelsToCopy == 1)
		{
			memcpy(destination.getWritePointer(0, startOffsetInBuffer), tempBuffer, numThisTime * sizeof(int16));

			if (numDestChannels == 2)
			{
				memcpy(destination.getWritePointer(1, startOffsetInBuffer), tempBuffer, numThisTime * sizeof(int16));
			}
		}
		else
		{
			jassert(destination.getNumChannels() == 2);

			int16* channels[2] = { static_cast<int16*>(destination.getWritePointer(0, 0)), static_cast<int16*>(destination.getWritePointer(1, 0)) };

			ReadHelper<AudioData::Int16, AudioData::Int16, AudioData::LittleEndian>::read(channels, startOffsetInBuffer, numDestChannels, tempBuffer, 2, numThisTime);
		}

		startOffsetInBuffer += numThisTime;
		numSamples -= numThisTime;
	}

	return true;
}

bool HlacMemoryMappedAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	if (isMonolith)
	{
		clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (map == nullptr || !mappedSection.contains(Range<int64>(startSampleInFile, startSampleInFile + numSamples)))
		{
			jassertfalse; // you must make sure that the window contains all the samples you're going to attempt to read.
			return false;
		}

		copySampleData(destSamples, startOffsetInDestBuffer, numDestChannels, sampleToPointer(startSampleInFile), numChannels, numSamples);

		return true;
	}
	else
	{
		if (internalReader.input != nullptr)
		{
			return internalReader.internalHlacRead(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile, numSamples);
		}

		// You have to call mapEverythingAndCreateMemoryStream() before using this method
		jassertfalse;
		return false;
	}
}


bool HlacMemoryMappedAudioFormatReader::mapSectionOfFile(Range<int64> samplesToMap)
{
	if (isMonolith)
	{
		dataChunkStart = 1;
		dataLength = getFile().getSize() - 1;

		return MemoryMappedAudioFormatReader::mapSectionOfFile(samplesToMap);
	}
	else
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

		map.reset(new MemoryMappedFile(getFile(), fileRange, MemoryMappedFile::readOnly, false));

		if (map != nullptr && !map->getRange().isEmpty())
		{
			int64 mappedStart = samplesToMap.getStart() / COMPRESSION_BLOCK_SIZE;

			int64 mappedEnd = jmin<int64>(lengthInSamples, samplesToMap.getEnd() - (samplesToMap.getEnd() % COMPRESSION_BLOCK_SIZE) + 1);
			mappedSection = Range<int64>(mappedStart, mappedEnd);

			auto actualMappedRange = map->getRange();

			int offset = (int)(fileRange.getStart() - actualMappedRange.getStart());
			int length = (int)(actualMappedRange.getLength() - offset);

			mis = new MemoryInputStream((uint8*)map->getData() + offset, length, false);

			internalReader.input = mis;

			internalReader.setUseHeaderOffsetWhenSeeking(false);

			return true;

		}

		return false;
	}
}

void HlacMemoryMappedAudioFormatReader::setTargetAudioDataType(AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == AudioDataConverters::DataFormat::float32BE) ||
		(dataType == AudioDataConverters::DataFormat::float32LE);

	internalReader.setTargetAudioDataType(dataType);
}

void HlacMemoryMappedAudioFormatReader::copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept
{
	jassert(numDestChannels == numDestChannels);

	if (numChannels == 1)
	{
		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
	}
	else
	{
		ReadHelper<AudioData::Float32, AudioData::Int16, AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
	}
}

bool HlacMemoryMappedAudioFormatReader::copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64 offsetInFile, int numSrcChannels, int numSamples)
{
	auto sourceData = sampleToPointer(offsetInFile);

	if (numSrcChannels == 1)
	{
		memcpy(destination.getWritePointer(0, startOffsetInBuffer), sourceData, numSamples * sizeof(int16));

		if (numDestChannels == 2)
		{
			memcpy(destination.getWritePointer(1, startOffsetInBuffer), sourceData, numSamples * sizeof(int16));
		}
	}
	else
	{
		jassert(destination.getNumChannels() == 2);

		int16* channels[2] = { static_cast<int16*>(destination.getWritePointer(0, 0)), static_cast<int16*>(destination.getWritePointer(1, 0)) };

		ReadHelper<AudioData::Int16, AudioData::Int16, AudioData::LittleEndian>::read(channels, startOffsetInBuffer, numDestChannels, sourceData, 2, numSamples);
	}

	return true;
}

HlacSubSectionReader::HlacSubSectionReader(AudioFormatReader* sourceReader, int64 subsectionStartSample, int64 subsectionLength) :
	AudioFormatReader(0, sourceReader->getFormatName()),
	start(subsectionStartSample)
{
	length = jmin(jmax((int64)0, sourceReader->lengthInSamples - subsectionStartSample), subsectionLength);

	sampleRate = sourceReader->sampleRate;
	bitsPerSample = sourceReader->bitsPerSample;
	numChannels = sourceReader->numChannels;
	usesFloatingPointData = sourceReader->usesFloatingPointData;
	lengthInSamples = length;

	

	if (auto m = dynamic_cast<HlacMemoryMappedAudioFormatReader*>(sourceReader))
	{
		memoryReader = m;
		normalReader = nullptr;

		internalReader = &memoryReader->internalReader;
		isMonolith = memoryReader->isMonolith;

	}
	else
	{
		memoryReader = nullptr;
		normalReader = dynamic_cast<HiseLosslessAudioFormatReader*>(sourceReader);

		internalReader = &normalReader->internalReader;
		isMonolith = normalReader->isMonolith;
	}
}

bool HlacSubSectionReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
		startSampleInFile, numSamples, length);

	if(memoryReader != nullptr)
		return memoryReader->readSamples(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile + start, numSamples);
	else
		return normalReader->readSamples(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile + start, numSamples);
}

void HlacSubSectionReader::readMaxLevels(int64 startSampleInFile, int64 numSamples, Range<float>* results, int numChannelsToRead)
{
	startSampleInFile = jmax((int64)0, startSampleInFile);
	numSamples = jmax((int64)0, jmin(numSamples, length - startSampleInFile));

	if(memoryReader != nullptr)
		memoryReader->readMaxLevels(startSampleInFile + start, numSamples, results, numChannelsToRead);
	else
		normalReader->readMaxLevels(startSampleInFile + start, numSamples, results, numChannelsToRead);
}

void HlacSubSectionReader::readIntoFixedBuffer(HiseSampleBuffer& buffer, int startSample, int numSamples, int64 readerStartSample)
{
	if (isMonolith)
	{
		if (memoryReader != nullptr)
		{
			memoryReader->copyFromMonolith(buffer, startSample, buffer.getNumChannels(), start + readerStartSample, numChannels, numSamples);
		}
		else
		{
			normalReader->copyFromMonolith(buffer, startSample, buffer.getNumChannels(), start + readerStartSample, numChannels, numSamples);
		}
	}
	else
	{
		internalReader->fixedBufferRead(buffer, numChannels, startSample, start + readerStartSample, numSamples);

		if (buffer.getNumChannels() == 1 || numChannels == 1)
		{
			buffer.setUseOneMap(true);
		}
	}
}

} // namespace hlac
