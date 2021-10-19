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

namespace hlac {

HiseLosslessAudioFormatReader::HiseLosslessAudioFormatReader(juce::InputStream* input_) :
    juce::AudioFormatReader(input_, "HLAC"),
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
		lengthInSamples = (input_->getTotalLength() - 1) / numChannels / sizeof(int16_t);
	}
}

bool HiseLosslessAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples)
{
	if (isMonolith)
	{
		clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (numSamples <= 0)
			return true;

		const int bytesPerFrame = sizeof(int16_t) * numChannels;

		input->setPosition(1 + startSampleInFile * bytesPerFrame);

		while (numSamples > 0)
		{
			const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
			char tempBuffer[tempBufSize];

			const int numThisTime = juce::jmin(tempBufSize / bytesPerFrame, numSamples);
			const int bytesRead = input->read(tempBuffer, numThisTime * bytesPerFrame);

			if (bytesRead < numThisTime * bytesPerFrame)
			{
				jassert(bytesRead >= 0);
                juce::zeromem(tempBuffer + bytesRead, (size_t)(numThisTime * bytesPerFrame - bytesRead));
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


void HiseLosslessAudioFormatReader::setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == juce::AudioDataConverters::DataFormat::float32BE) ||
		(dataType == juce::AudioDataConverters::DataFormat::float32LE);

	internalReader.setTargetAudioDataType(dataType);
}


uint32_t HiseLosslessHeader::getOffsetForReadPosition(int64_t samplePosition, bool addHeaderOffset)
{
	if (samplePosition % COMPRESSION_BLOCK_SIZE == 0)
	{
		auto blockIndex = (uint32_t)samplePosition / COMPRESSION_BLOCK_SIZE;

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
		auto blockIndex = (uint32_t)samplePosition / COMPRESSION_BLOCK_SIZE;

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

uint32_t HiseLosslessHeader::getOffsetForNextBlock(int64_t samplePosition, bool addHeaderOffset)
{
	if (samplePosition % COMPRESSION_BLOCK_SIZE == 0)
	{
		auto blockIndex = (uint32_t)samplePosition / COMPRESSION_BLOCK_SIZE;

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
		auto blockIndex = (uint32_t)samplePosition / COMPRESSION_BLOCK_SIZE;

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

void HlacReaderCommon::setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == juce::AudioDataConverters::DataFormat::float32BE) ||
		(dataType == juce::AudioDataConverters::DataFormat::float32LE);
}

bool HlacReaderCommon::internalHlacRead(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples)
{
    juce::ignoreUnused(startSampleInFile);
    juce::ignoreUnused(numDestChannels);

	decoder.setHlacVersion(header.getVersion());

	bool isStereo = destSamples[1] != nullptr;

	if (startSampleInFile != decoder.getCurrentReadPosition())
	{
		auto byteOffset = header.getOffsetForReadPosition(startSampleInFile, useHeaderOffsetWhenSeeking);

		decoder.seekToPosition(*input, (uint32_t)startSampleInFile, byteOffset);
	}

	if (isStereo)
	{
		if (usesFloatingPointData)
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

            juce::AudioSampleBuffer b(destinationFloat, 2, numSamples);
			HiseSampleBuffer hsb(b);

			decoder.decode(hsb, true, *input, (int)startSampleInFile, numSamples);
		}
		else
		{
			auto** destinationFixed = reinterpret_cast<int16_t**>(destSamples);

			if (isStereo)
			{
				destinationFixed[0] = destinationFixed[0] + startOffsetInDestBuffer;
			}
			else
			{
				destinationFixed[0] = destinationFixed[0] + startOffsetInDestBuffer;
				destinationFixed[1] = destinationFixed[1] + startOffsetInDestBuffer;
			}

			HiseSampleBuffer hsb(destinationFixed, 2, numSamples);
			
			decoder.decode(hsb, true, *input, (int)startSampleInFile, numSamples);
		}
	}
	else
	{
		if (usesFloatingPointData)
		{
			float* destinationFloat = reinterpret_cast<float*>(destSamples[0]);

            juce::AudioSampleBuffer b(&destinationFloat, 1, numSamples);
			HiseSampleBuffer hsb(b);
			hsb.allocateNormalisationTables((int)startSampleInFile);

			decoder.decode(hsb, false, *input, (int)startSampleInFile, numSamples);
		}
		else
		{
			auto** destinationFixed = reinterpret_cast<int16_t**>(destSamples);

			HiseSampleBuffer hsb(destinationFixed, 1, numSamples);
			hsb.allocateNormalisationTables((int)startSampleInFile);

			decoder.decode(hsb, false, *input, (int)startSampleInFile, numSamples);
		}
	}

	return true;
}

bool HlacReaderCommon::fixedBufferRead(HiseSampleBuffer& buffer, int numDestChannels, int startOffsetInBuffer, int64_t startSampleInFile, int numSamples)
{
	bool isStereo = numDestChannels == 2;

	if (startSampleInFile < 0)
	{
		auto silence = (int)juce::jmin(-startSampleInFile, (int64_t)numSamples);

		auto numToClear = juce::jmin(silence, buffer.getNumSamples() - startOffsetInBuffer);

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

		decoder.seekToPosition(*input, (uint32_t)startSampleInFile, byteOffset);
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
		ReadHelper<juce::AudioData::Float32, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
	}
	else
	{
		ReadHelper<juce::AudioData::Float32, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
	}
}

bool HiseLosslessAudioFormatReader::copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64_t offsetInFile, int numChannelsToCopy, int numSamples)
{
	if (numSamples <= 0)
		return true;

	const int bytesPerFrame = sizeof(int16_t) * numChannelsToCopy;

	input->setPosition(1 + offsetInFile * bytesPerFrame);

	while (numSamples > 0)
	{
		const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
		char tempBuffer[tempBufSize];

		const int numThisTime = juce::jmin(tempBufSize / bytesPerFrame, numSamples);
		const int bytesRead = input->read(tempBuffer, numThisTime * bytesPerFrame);

		if (bytesRead < numThisTime * bytesPerFrame)
		{
			jassert(bytesRead >= 0);
            juce::zeromem(tempBuffer + bytesRead, (size_t)(numThisTime * bytesPerFrame - bytesRead));
		}



		//copySampleData(destSamples, startOffsetInDestBuffer, numDestChannels,
		//	tempBuffer, (int)numChannels, numThisTime);

		if (numChannelsToCopy == 1)
		{
			memcpy(destination.getWritePointer(0, startOffsetInBuffer), tempBuffer, numThisTime * sizeof(int16_t));

			if (numDestChannels == 2)
			{
				memcpy(destination.getWritePointer(1, startOffsetInBuffer), tempBuffer, numThisTime * sizeof(int16_t));
			}
		}
		else
		{
			jassert(destination.getNumChannels() == 2);

			int16_t* channels[2] = { static_cast<int16_t*>(destination.getWritePointer(0, 0)), static_cast<int16_t*>(destination.getWritePointer(1, 0)) };

			ReadHelper<juce::AudioData::Int16, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(channels, startOffsetInBuffer, numDestChannels, tempBuffer, 2, numThisTime);
		}

		startOffsetInBuffer += numThisTime;
		numSamples -= numThisTime;
	}

	return true;
}

bool HlacMemoryMappedAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples)
{
	if (isMonolith)
	{
		clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
			startSampleInFile, numSamples, lengthInSamples);

		if (map == nullptr || !mappedSection.contains(juce::Range<int64_t>(startSampleInFile, startSampleInFile + numSamples)))
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


bool HlacMemoryMappedAudioFormatReader::mapSectionOfFile(juce::Range<int64_t> samplesToMap)
{
	if (isMonolith)
	{
		dataChunkStart = 1;
		dataLength = getFile().getSize() - 1;

		return MemoryMappedAudioFormatReader::mapSectionOfFile(samplesToMap);
	}
	else
	{
		dataChunkStart = (int64_t)internalReader.header.getOffsetForReadPosition(0, true);
		dataLength = getFile().getSize() - dataChunkStart;

		int64_t start = (int64_t)internalReader.header.getOffsetForReadPosition(samplesToMap.getStart(), true);
		int64_t end = 0;

		if (samplesToMap.getEnd() >= lengthInSamples)
		{
			end = getFile().getSize();
		}
		else
		{
			end = internalReader.header.getOffsetForNextBlock(samplesToMap.getEnd(), true);
		}

		auto fileRange = juce::Range<int64_t>(start, end);

		map.reset(new juce::MemoryMappedFile(getFile(), fileRange, juce::MemoryMappedFile::readOnly, false));

		if (map != nullptr && !map->getRange().isEmpty())
		{
			int64_t mappedStart = samplesToMap.getStart() / COMPRESSION_BLOCK_SIZE;

			int64_t mappedEnd = juce::jmin<int64_t>(lengthInSamples, samplesToMap.getEnd() - (samplesToMap.getEnd() % COMPRESSION_BLOCK_SIZE) + 1);
			mappedSection = juce::Range<int64_t>(mappedStart, mappedEnd);

			auto actualMappedRange = map->getRange();

			int offset = (int)(fileRange.getStart() - actualMappedRange.getStart());
			int length = (int)(actualMappedRange.getLength() - offset);

			mis = std::make_unique<juce::MemoryInputStream>((uint8_t*)map->getData() + offset, length, false);

			internalReader.input = mis.get();

			internalReader.setUseHeaderOffsetWhenSeeking(false);

			return true;

		}

		return false;
	}
}

void HlacMemoryMappedAudioFormatReader::setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType)
{
	usesFloatingPointData = (dataType == juce::AudioDataConverters::DataFormat::float32BE) ||
		(dataType == juce::AudioDataConverters::DataFormat::float32LE);

	internalReader.setTargetAudioDataType(dataType);
}

void HlacMemoryMappedAudioFormatReader::copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept
{
	jassert(numDestChannels == numDestChannels);

	if (numChannels == 1)
	{
		ReadHelper<juce::AudioData::Float32, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, 1, sourceData, 1, numSamples);
	}
	else
	{
		ReadHelper<juce::AudioData::Float32, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, 2, numSamples);
	}
}

bool HlacMemoryMappedAudioFormatReader::copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64_t offsetInFile, int numSrcChannels, int numSamples)
{
	auto sourceData = sampleToPointer(offsetInFile);

	if (numSrcChannels == 1)
	{
		memcpy(destination.getWritePointer(0, startOffsetInBuffer), sourceData, numSamples * sizeof(int16_t));

		if (numDestChannels == 2)
		{
			memcpy(destination.getWritePointer(1, startOffsetInBuffer), sourceData, numSamples * sizeof(int16_t));
		}
	}
	else
	{
		jassert(destination.getNumChannels() == 2);

		int16_t* channels[2] = { static_cast<int16_t*>(destination.getWritePointer(0, 0)), static_cast<int16_t*>(destination.getWritePointer(1, 0)) };

		ReadHelper<juce::AudioData::Int16, juce::AudioData::Int16, juce::AudioData::LittleEndian>::read(channels, startOffsetInBuffer, numDestChannels, sourceData, 2, numSamples);
	}

	return true;
}

HlacSubSectionReader::HlacSubSectionReader(AudioFormatReader* sourceReader, int64_t subsectionStartSample, int64_t subsectionLength) :
	AudioFormatReader(0, sourceReader->getFormatName()),
	start(subsectionStartSample)
{
	length = juce::jmin (juce::jmax ((int64_t)0, sourceReader->lengthInSamples - subsectionStartSample), subsectionLength);

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

bool HlacSubSectionReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples)
{
	clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
		startSampleInFile, numSamples, length);

	if(memoryReader != nullptr)
		return memoryReader->readSamples(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile + start, numSamples);
	else
		return normalReader->readSamples(destSamples, numDestChannels, startOffsetInDestBuffer, startSampleInFile + start, numSamples);
}

void HlacSubSectionReader::readMaxLevels(int64_t startSampleInFile, int64_t numSamples, juce::Range<float>* results, int numChannelsToRead)
{
	startSampleInFile = juce::jmax((int64_t)0, startSampleInFile);
	numSamples = juce::jmax((int64_t)0, juce::jmin(numSamples, length - startSampleInFile));

	if(memoryReader != nullptr)
		memoryReader->readMaxLevels(startSampleInFile + start, numSamples, results, numChannelsToRead);
	else
		normalReader->readMaxLevels(startSampleInFile + start, numSamples, results, numChannelsToRead);
}

void HlacSubSectionReader::readIntoFixedBuffer(HiseSampleBuffer& buffer, int startSample, int numSamples, int64_t readerStartSample)
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
