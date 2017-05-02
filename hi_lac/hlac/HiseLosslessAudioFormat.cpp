/*  HISE Lossless Audio Codec
*	©2017 Christoph Hart
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


HiseLosslessAudioFormat::HiseLosslessAudioFormat() :
	AudioFormat("HLAC", StringArray(".hlac"))
{

}

bool HiseLosslessAudioFormat::canHandleFile(const File& fileToTest)
{
	return fileToTest.getFileExtension() == ".hlac";
}

Array<int> HiseLosslessAudioFormat::getPossibleSampleRates()
{
	Array<int> sampleRates;

	sampleRates.add(11025);
	sampleRates.add(22050);
	sampleRates.add(32000);
	sampleRates.add(44100);
	sampleRates.add(48000);
	sampleRates.add(88200);
	sampleRates.add(96000);

	return sampleRates;
}

Array<int> HiseLosslessAudioFormat::getPossibleBitDepths()
{
	Array<int> bitRates;

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

StringArray HiseLosslessAudioFormat::getQualityOptions()
{
	StringArray options;

	options.add("EncodeMode");

	return options;
}

AudioFormatReader* HiseLosslessAudioFormat::createReaderFor(InputStream* sourceStream, bool deleteStreamIfOpeningFails)
{
	ignoreUnused(deleteStreamIfOpeningFails);

	return new HiseLosslessAudioFormatReader(sourceStream);
}

AudioFormatWriter* HiseLosslessAudioFormat::createWriterFor(OutputStream* streamToWriteTo, double sampleRateToUse, unsigned int numberOfChannels, int /*bitsPerSample*/, const StringPairArray& metadataValues, int /*qualityOptionIndex*/)
{
	HiseLosslessAudioFormatWriter::EncodeMode mode = metadataValues.getValue("EncodeMode", "Diff") == "Block" ?
		HiseLosslessAudioFormatWriter::EncodeMode::Block :
		HiseLosslessAudioFormatWriter::EncodeMode::Diff;

	if (blockOffsets == nullptr)
		blockOffsets.calloc(1024 * 1024);

	return new HiseLosslessAudioFormatWriter(mode, streamToWriteTo, sampleRateToUse, numberOfChannels, blockOffsets);
}


MemoryMappedAudioFormatReader* HiseLosslessAudioFormat::createMemoryMappedReader(FileInputStream* fin)
{
#if JUCE_64BIT
	ScopedPointer<AudioFormatReader> normalReader = new HiseLosslessAudioFormatReader(fin);

	ScopedPointer<HlacMemoryMappedAudioFormatReader> reader = new HlacMemoryMappedAudioFormatReader(fin->getFile(), *normalReader, 0, normalReader->lengthInSamples, 1);

	return reader.release();
#else

	// Memory mapped file support on 32bit is pretty useless so we don't bother at all...
	jassertfalse;
	return nullptr;
#endif
}

MemoryMappedAudioFormatReader* HiseLosslessAudioFormat::createMemoryMappedReader(const File& file)
{
	FileInputStream* fis = new FileInputStream(file);

	return createMemoryMappedReader(fis);
}

HiseLosslessHeader::HiseLosslessHeader(bool useEncryption, uint8 globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32 numBlocks)
{
	// Header byte: 1 bit      | 3 bit   |  4 bit        |
	//              encryption | version | global shift  |
	headerByte = (useEncryption ? 0x80 : 0);
	headerByte |= (HLAC_VERSION << 4);
	headerByte |= (globalBitShiftAmount & 0x0F);

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

HiseLosslessHeader::HiseLosslessHeader(InputStream* input)
{
	if (input == nullptr)
	{
		jassertfalse;
		return;
	}

	readMetadataFromStream(input);
}

HiseLosslessHeader::HiseLosslessHeader(const File& f)
{
	ScopedPointer<FileInputStream> fis = new FileInputStream(f);

	readMetadataFromStream(fis);
}

void HiseLosslessHeader::readMetadataFromStream(InputStream* input)
{
	uint64 metadata = 0;
	uint8* metaDataBytePtr = reinterpret_cast<uint8*>(&metadata);

	//metadata = input->readInt64();

	metaDataBytePtr[0] = input->readByte();

	// The old format just stored the channel amount as first header byte;
	isOldMonolith = metaDataBytePtr[0] == 1 || metaDataBytePtr[0] == 2;

	if (isOldMonolith)
	{
		headerByte = metaDataBytePtr[0];
		sampleDataByte = 0;
		headerValid = true;
		blockAmount = 0;
	}
	else
	{
		metaDataBytePtr[1] = input->readByte();
		metaDataBytePtr[2] = input->readByte();
		metaDataBytePtr[3] = input->readByte();
		metaDataBytePtr[4] = input->readByte();
		metaDataBytePtr[5] = input->readByte();
		metaDataBytePtr[6] = input->readByte();
		metaDataBytePtr[7] = input->readByte();

		const uint64 metadataMasked = metadata & 0xFFFFFFFFFFFF0000;
		const uint16 checksum = metadata & 0x000000000000FFFF;

		headerValid = metadataMasked > 0 && CompressionHelpers::Misc::NumberOfSetBits(metadataMasked) == checksum;

		if (!headerValid)
		{
			jassertfalse;
			return;
		}
		else
		{
			headerByte = (uint8)((metadataMasked & 0xFF00000000000000) >> 56);
			sampleDataByte = (uint8)((metadataMasked & 0x00FF000000000000) >> 48);
		}

		blockAmount = (uint32)((metadataMasked & 0x0000FFFFFFFF0000) >> 16);
		blockOffsets.malloc(blockAmount);

		for (uint32 i = 0; i < blockAmount; i++)
		{
			blockOffsets[i] = (uint32)input->readInt();
		}
	}

	headerSize = (uint32)input->getPosition();
}



int HiseLosslessHeader::getVersion() const
{
	if (isOldMonolith)
	{
		// ancient versions didn't use a real header byte
		return headerByte;
	}
	else
		return (headerByte & 0x70) >> 4;
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
		return (headerByte & 0x0F);
	}
}

unsigned int HiseLosslessHeader::getNumChannels() const
{
	if (isOldMonolith)
	{
		// previous versions just stored the channel amount
		return headerByte;
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
		const uint8 srIndex = (sampleDataByte & 0xC0) >> 6;
		return sampleRates[srIndex];
	}
}

uint32 HiseLosslessHeader::getBlockAmount() const 
{
	return blockAmount;
}

bool HiseLosslessHeader::write(OutputStream* output)
{
	// Add the checksum of the 48 bits.
	uint64 metadata = ((uint64)headerByte << 56) | ((uint64)sampleDataByte << 48) | ((uint64)blockAmount << 16);
	uint16 checkSum = (uint16)CompressionHelpers::Misc::NumberOfSetBits(metadata);
	metadata |= checkSum;

	output->writeInt64(metadata);

	for (uint32 i = 0; i < blockAmount; i++)
	{
		if (!output->writeInt(blockOffsets[i]))
			return false;
	}

	return true;
}

void HiseLosslessHeader::storeOffsets(uint32* offsets, int numOffsets)
{
	memcpy(blockOffsets, offsets, sizeof(uint32)*numOffsets);
}
