/*
  ==============================================================================

    HiseLosslessAudioFormat.cpp
    Created: 12 Apr 2017 10:15:39pm
    Author:  Christoph

  ==============================================================================
*/


#include "HiseLosslessAudioFormat.h"


HiseLosslessAudioFormatReader::HiseLosslessAudioFormatReader(InputStream* input_) :
	AudioFormatReader(input_, "HLAC"),
	header(input)
{
	numChannels = header.getNumChannels();
	sampleRate = header.getSampleRate();
	bitsPerSample = header.getBitsPerSample();
	lengthInSamples = header.blockAmount * COMPRESSION_BLOCK_SIZE;
	usesFloatingPointData = true;

	decoder.setupForDecompression();
}

bool HiseLosslessAudioFormatReader::readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples)
{
	ignoreUnused(startSampleInFile);
	ignoreUnused(numDestChannels);
	ignoreUnused(startOffsetInDestBuffer);

	bool isStereo = destSamples[1] != nullptr;

	if (isStereo)
	{
		float** destinationFloat = reinterpret_cast<float**>(destSamples);

		AudioSampleBuffer b(destinationFloat, 2, numSamples);

		decoder.decode(b, *input);

	}
	else
	{
		float* destinationFloat = reinterpret_cast<float*>(destSamples[0]);

		AudioSampleBuffer b(&destinationFloat, 1, numSamples);

		decoder.decode(b, *input);
	}

	return true;
}


HiseLosslessAudioFormatWriter::HiseLosslessAudioFormatWriter(EncodeMode mode_, OutputStream* output, double sampleRate, int numChannels, uint32* blockOffsetBuffer) :
	AudioFormatWriter(output, "HLAC", sampleRate, numChannels, 16),
	mode(mode_),
	tempOutputStream(new MemoryOutputStream()),
	blockOffsets(blockOffsetBuffer)
{
	usesFloatingPointData = true;
}


HiseLosslessAudioFormatWriter::~HiseLosslessAudioFormatWriter()
{
	flush();
}

bool HiseLosslessAudioFormatWriter::flush()
{
	if (tempWasFlushed)
		return true;

	if (!writeHeader())
		return false;

	if (!writeDataFromTemp())
		return false;

	tempWasFlushed = true;
	deleteTemp();
	return true;
}


void HiseLosslessAudioFormatWriter::setOptions(HlacEncoder::CompressorOptions& newOptions)
{
	encoder.setOptions(newOptions);
}

bool HiseLosslessAudioFormatWriter::write(const int** samplesToWrite, int numSamples)
{
	tempWasFlushed = false;

	bool isStereo = samplesToWrite[1] != nullptr;

	if (isStereo)
	{
		float* const* r = const_cast<float**>(reinterpret_cast<const float**>(samplesToWrite));

		AudioSampleBuffer b = AudioSampleBuffer(r, 2, numSamples);

		encoder.compress(b, *tempOutputStream, blockOffsets);
	}
	else
	{
		float* r = const_cast<float*>(reinterpret_cast<const float*>(samplesToWrite[0]));

		AudioSampleBuffer b = AudioSampleBuffer(&r, 1, numSamples);

		encoder.compress(b, *tempOutputStream, blockOffsets);
	}

	return true;
}


void HiseLosslessAudioFormatWriter::setTemporaryBufferType(bool shouldUseTemporaryFile)
{
	usesTempFile = shouldUseTemporaryFile;

	deleteTemp();

	if (shouldUseTemporaryFile)
	{
		FileOutputStream* fosOriginal = dynamic_cast<FileOutputStream*>(output);

		const bool createTempFileInTargetDirectory = fosOriginal != nullptr;

		if (createTempFileInTargetDirectory)
		{
			File originalFile = fosOriginal->getFile();
			tempFile = new TemporaryFile(originalFile, TemporaryFile::OptionFlags::putNumbersInBrackets);
			File tempTarget = tempFile->getFile();
			tempOutputStream = new FileOutputStream(tempTarget);
		}
		else
		{
			tempFile = new TemporaryFile(File::getCurrentWorkingDirectory(), TemporaryFile::OptionFlags::putNumbersInBrackets);
			File tempTarget = tempFile->getFile();
		}
	}
	else
	{
		tempOutputStream = new MemoryOutputStream();
	}
}

bool HiseLosslessAudioFormatWriter::writeHeader()
{
	auto numBlocks = encoder.getNumBlocksWritten();

	HiseLosslessHeader header(useEncryption, globalBitShiftAmount, sampleRate, numChannels, bitsPerSample, useCompression, numBlocks);

	jassert(header.getBitShiftAmount() == globalBitShiftAmount);
	jassert(header.getNumChannels() == numChannels);
	jassert(header.usesCompression() == useCompression);
	jassert(header.getSampleRate() == sampleRate);
	jassert(header.getBitsPerSample() == bitsPerSample);
	


	memcpy(header.blockOffsets, blockOffsets, sizeof(uint32)*numBlocks);
	
	header.write(output);
	return true;
}


bool HiseLosslessAudioFormatWriter::writeDataFromTemp()
{
	if (usesTempFile)
	{
		FileOutputStream* to = dynamic_cast<FileOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

		FileInputStream fis(to->getFile());
		return output->writeFromInputStream(fis, fis.getTotalLength()) == fis.getTotalLength();
	}
	else
	{
		MemoryOutputStream* to = dynamic_cast<MemoryOutputStream*>(tempOutputStream.get());

		jassert(to != nullptr);

		MemoryInputStream mis(to->getData(), to->getDataSize(), false);
		return output->writeFromInputStream(mis, mis.getTotalLength()) == mis.getTotalLength();
	}
}

void HiseLosslessAudioFormatWriter::deleteTemp()
{
	// If you hit this assertion, it means that you didn't call flush after writing the last data.
	// This means nothing will get written to the actual output stream...
	jassert(tempWasFlushed);

	tempFile = nullptr;
	tempOutputStream = nullptr;
}

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
	HiseLosslessAudioFormatWriter::EncodeMode mode = metadataValues.getValue("EncodeMode", "Diff") == "Diff" ?
		HiseLosslessAudioFormatWriter::EncodeMode::Diff :
		HiseLosslessAudioFormatWriter::EncodeMode::Block;

	if (blockOffsets == nullptr)
		blockOffsets.calloc(1024 * 1024 * 1024);

	return new HiseLosslessAudioFormatWriter(mode, streamToWriteTo, sampleRateToUse, numberOfChannels, blockOffsets);
}

HiseLosslessHeader::HiseLosslessHeader(bool useEncryption, uint8 globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32 numBlocks)
{
	// Header byte: | 3 bit   | 1 bit      | 4 bit        |
	//              | version | encryption | global shift |
	headerByte = HLAC_VERSION << 5;
	headerByte |= (useEncryption ? 0x10 : 0);
	headerByte |= (globalBitShiftAmount & 0x0F);

	// Sample data byte | 2 bit    | 4 bit    | 1 bit 1  | 1 bit           |
	//                  | SR index | channels | bit rate | use compression |
	sampleDataByte = CompressionHelpers::Misc::getSampleRateIndex(sampleRate) << 6;
	sampleDataByte |= (numChannels & 0x0F) << 4;
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

	const uint64 metadata = input->readInt64();
	const uint64 metadata2 = metadata & 0xFFFFFFFFFFFF0000;
	const uint16 checksum = metadata  & 0x000000000000FFFF;

	headerValid = metadata > 0 && CompressionHelpers::Misc::NumberOfSetBits(metadata2) == checksum;

	if (!headerValid)
	{
		jassertfalse;
		return;
	}
	else
	{
		headerByte     = (uint8)((metadata & 0xFF00000000000000) >> 56);
		sampleDataByte = (uint8)((metadata & 0x00FF000000000000) >> 48);
	}
	
	blockAmount = (uint32)((metadata & 0x0000FFFFFFFF0000) >> 16);
	blockOffsets.malloc(blockAmount);

	for (uint32 i = 0; i < blockAmount; i++)
	{
		blockOffsets[i] = (uint32)input->readInt();
	}
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
