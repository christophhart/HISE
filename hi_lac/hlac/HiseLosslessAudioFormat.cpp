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

HiseLosslessAudioFormat::HiseLosslessAudioFormat() :
    juce::AudioFormat("HLAC", juce::StringArray({ ".hlac", ".ch1", ".ch2", ".ch3", ".ch4", ".ch5" }))
{

}

bool HiseLosslessAudioFormat::canHandleFile(const juce::File& fileToTest)
{
	return fileToTest.getFileExtension() == ".hlac" || fileToTest.getFileExtension().contains("ch");
}

juce::Array<int> HiseLosslessAudioFormat::getPossibleSampleRates()
{
    juce::Array<int> sampleRates;

	sampleRates.add(11025);
	sampleRates.add(22050);
	sampleRates.add(32000);
	sampleRates.add(44100);
	sampleRates.add(48000);
	sampleRates.add(88200);
	sampleRates.add(96000);

	return sampleRates;
}

juce::Array<int> HiseLosslessAudioFormat::getPossibleBitDepths()
{
    juce::Array<int> bitRates;

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

juce::StringArray HiseLosslessAudioFormat::getQualityOptions()
{
    juce::StringArray options;

	options.add("EncodeMode");

	return options;
}

juce::AudioFormatReader* HiseLosslessAudioFormat::createReaderFor(juce::InputStream* sourceStream, bool deleteStreamIfOpeningFails)
{
    juce::ignoreUnused(deleteStreamIfOpeningFails);

	return new HiseLosslessAudioFormatReader(sourceStream);
}

juce::AudioFormatWriter* HiseLosslessAudioFormat::createWriterFor(juce::OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const juce::StringPairArray& metadataValues, int /*qualityOptionIndex*/)
{
	HiseLosslessAudioFormatWriter::EncodeMode mode = metadataValues.getValue("EncodeMode", "Diff") == "Block" ?
		HiseLosslessAudioFormatWriter::EncodeMode::Block :
		HiseLosslessAudioFormatWriter::EncodeMode::Diff;

	if (blockOffsets == nullptr)
		blockOffsets.calloc(1024 * 1024);
	else
		memset(blockOffsets, 0, 1024 * 1024);

	return new HiseLosslessAudioFormatWriter(mode, streamToWriteTo, sampleRateToUse, numberOfChannels, blockOffsets);
}


juce::MemoryMappedAudioFormatReader* HiseLosslessAudioFormat::createMemoryMappedReader(juce::FileInputStream* fin)
{
#if JUCE_64BIT
    juce::ScopedPointer<juce::AudioFormatReader> normalReader = new HiseLosslessAudioFormatReader(fin);

    juce::ScopedPointer<HlacMemoryMappedAudioFormatReader> reader = new HlacMemoryMappedAudioFormatReader(fin->getFile(), *normalReader, 0, normalReader->lengthInSamples, 1);

	return reader.release();
#else

    juce::ignoreUnused(fin);

	// Memory mapped file support on 32bit is pretty useless so we don't bother at all...
	jassertfalse;
	return nullptr;
#endif
}

juce::MemoryMappedAudioFormatReader* HiseLosslessAudioFormat::createMemoryMappedReader(const juce::File& file)
{
    auto* fis = new juce::FileInputStream(file);

	return createMemoryMappedReader(fis);
}

HiseLosslessHeader::HiseLosslessHeader(bool useEncryption, uint8_t globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32_t numBlocks)
{
	headerByte1 = HLAC_VERSION;

	headerByte2 = (useEncryption ? 0x80 : 0);
	headerByte2 |= (globalBitShiftAmount & 0x0F);

	// Sample data byte | 2 bit    | 4 bit    | 1 bit 1  | 1 bit           |
	//                  | SR index | channels | bit rate | use compression |
	sampleDataByte = CompressionHelpers::Misc::getSampleRateIndex(sampleRate) << 6;
	sampleDataByte |= (numChannels & 0x0F) << 2;
	sampleDataByte |= ((bitsPerSample == 24) ? 0x02 : 0x00);
	sampleDataByte |= (useCompression ? 0x01 : 0x00);

	// block amount: number of total blocks in file (per channel)
	blockAmount = numBlocks;

	blockOffsets.calloc(blockAmount);
}

HiseLosslessHeader::HiseLosslessHeader(juce::InputStream* input)
{
	if (input == nullptr)
	{
		jassertfalse;
		return;
	}

	readMetadataFromStream(input);
}

HiseLosslessHeader::HiseLosslessHeader(const juce::File& f)
{
	auto fis = f.createInputStream();

	readMetadataFromStream(fis.get());
}

void HiseLosslessHeader::readMetadataFromStream (juce::InputStream* input)
{
	//metadata = input->readInt64();

	headerByte1 = input->readByte();
	

	// The old format just stored the channel amount as first header byte;
	isOldMonolith = headerByte1 < 2;

	if (isOldMonolith)
	{
		headerByte2 = 0;
		sampleDataByte = 0;
		headerValid = true;
		blockAmount = 0;
	}
	else
	{
		const uint32_t checkSum = (uint32_t)input->readInt();
		headerValid = CompressionHelpers::Misc::validateChecksum(checkSum);

		if (!headerValid)
		{
			headerByte2 = 0;
			sampleDataByte = 0;
			blockAmount = 0;
			jassertfalse;
			return;
		}
		else
		{
			headerByte2 = input->readByte();
			sampleDataByte = input->readByte();
			blockAmount = (uint32_t)input->readInt();

			blockOffsets.malloc(blockAmount);

			for (uint32_t i = 0; i < blockAmount; i++)
			{
				blockOffsets[i] = (uint32_t)input->readInt();
			}
		}
	}

	headerSize = (uint32_t)input->getPosition();
}



int HiseLosslessHeader::getVersion() const
{
	return headerByte1;
}

bool HiseLosslessHeader::isEncrypted() const
{
	return false;
}

int HiseLosslessHeader::getBitShiftAmount() const
{
	if (isOldMonolith)
	{
		return 0;
	}
	else
	{
		return (headerByte2 & 0x0F);
	}
}

unsigned int HiseLosslessHeader::getNumChannels() const
{
	if (isOldMonolith)
	{
		// previous versions just stored the channel amount
		return headerByte1 == 0 ? 2 : 1;
	}

	return (sampleDataByte >> 2) & 0x0F;
}

unsigned int HiseLosslessHeader::getBitsPerSample() const
{
	if (isOldMonolith)
		return 16;
	else
		return (sampleDataByte & 0x02) != 0 ? 24 : 16;
}

bool HiseLosslessHeader::usesCompression() const
{
	return !isOldMonolith && (sampleDataByte & 1) != 0;
}

double HiseLosslessHeader::getSampleRate() const
{
	if (isOldMonolith)
	{
		return 44100.0; // doesn't matter anyway (the sample rate is stored externally)
	}
	else
	{
		const static double sampleRates[4] = { 44100.0, 48000.0, 88200.0, 96000.0 };
		const uint8_t srIndex = (sampleDataByte & 0xC0) >> 6;
		return sampleRates[srIndex];
	}
}

uint32_t HiseLosslessHeader::getBlockAmount() const
{
	return blockAmount;
}

bool HiseLosslessHeader::write (juce::OutputStream* output)
{
	output->writeByte(headerByte1);

	if (headerByte1 < 2)
		return true;

	auto checkSum = CompressionHelpers::Misc::createChecksum();

	output->writeInt((int)checkSum);

	output->writeByte(headerByte2);
	output->writeByte(sampleDataByte);

	output->writeInt((int)blockAmount);
	
	for (uint32_t i = 0; i < blockAmount; i++)
	{
		if (!output->writeInt(blockOffsets[i]))
			return false;
	}

	return true;
}

void HiseLosslessHeader::storeOffsets(uint32_t* offsets, int numOffsets)
{
	memcpy(blockOffsets, offsets, sizeof(uint32_t)*numOffsets);
}

} // namespace hlac
