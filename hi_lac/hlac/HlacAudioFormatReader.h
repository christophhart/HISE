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

#ifndef HLACAUDIOFORMATREADER_H_INCLUDED
#define HLACAUDIOFORMATREADER_H_INCLUDED

namespace hlac { using namespace juce; 

struct HiseLosslessHeader
{
	HiseLosslessHeader(InputStream* input);

	HiseLosslessHeader(const File& f);

	HiseLosslessHeader(bool useEncryption, uint8 globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32 numBlocks);

	int getVersion() const;
	bool isEncrypted() const;
	int getBitShiftAmount() const;
	uint32 getNumChannels() const;
	uint32 getBitsPerSample() const;
	bool usesCompression() const;
	double getSampleRate() const;
	uint32 getBlockAmount() const;

	uint32 getOffsetForReadPosition(int64 samplePosition, bool addHeaderOffset);

	uint32 getOffsetForNextBlock(int64 samplePosition, bool addHeaderOffset);

	bool write(OutputStream* output);

	void storeOffsets(uint32* offsets, int numOffsets);

	void readMetadataFromStream(InputStream* stream);

	static HiseLosslessHeader createMonolithHeader(int numChannels, double sampleRate);

private:

	uint8 headerByte1 = 0;
	uint8 headerByte2 = 0;
	uint8 sampleDataByte = 0;
	uint32 blockAmount = 0;
	HeapBlock<uint32> blockOffsets;
	bool headerValid = false;
	bool isOldMonolith = false;
	uint32 headerSize;
};

class HlacReaderCommon
{
public:

	HlacReaderCommon(InputStream* input_):
		input(input_),
		header(input)
	{
		decoder.setupForDecompression();
		decoder.setHlacVersion(header.getVersion());
	}

	HlacReaderCommon(const File& f) :
		input(nullptr),
		header(f)
	{
		decoder.setupForDecompression();
		decoder.setHlacVersion(header.getVersion());
	}

	/** You can choose what the target data type should be. If you read into integer AudioSampleBuffers, you might want to call this method
	*	in order to save unnecessary conversions between float and integer numbers. */
	void setTargetAudioDataType(AudioDataConverters::DataFormat dataType);

	/** When seeking, add the length of the header as offset. */
	void setUseHeaderOffsetWhenSeeking(bool shouldUseHeaderOffset)
	{
		useHeaderOffsetWhenSeeking = shouldUseHeaderOffset;
	};

private:

	friend class HlacSubSectionReader;

	bool internalHlacRead(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples);

	bool fixedBufferRead(HiseSampleBuffer& buffer, int numDestChannels, int startOffsetInBuffer, int64 startSampleInFile, int numSamples);

	

	friend class HiseLosslessAudioFormatReader;
	friend class HlacMemoryMappedAudioFormatReader;

	InputStream* input;

	HlacDecoder decoder;
	HiseLosslessHeader header;

	bool usesFloatingPointData;

	bool useHeaderOffsetWhenSeeking = true;

};

class HiseLosslessAudioFormatReader : public AudioFormatReader
{
public:
	HiseLosslessAudioFormatReader(InputStream* input_);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override;

	double getDecompressionPerformanceForLastFile() { return internalReader.decoder.getDecompressionPerformance(); }

	void setTargetAudioDataType(AudioDataConverters::DataFormat dataType);

private:

	friend class HlacSubSectionReader;


	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

	bool copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64 offsetInFile, int numChannels, int numSamples);

	HlacReaderCommon internalReader;

	bool isMonolith = false;

};


class HlacMemoryMappedAudioFormatReader : public MemoryMappedAudioFormatReader
{
public:

	HlacMemoryMappedAudioFormatReader(const File& f, const AudioFormatReader& details, int64 start, int64 length, int frameSize) :
		MemoryMappedAudioFormatReader(f, details, start, length, frameSize),
		internalReader(f)
	{
		isMonolith = internalReader.header.getVersion() < 2;

		if (isMonolith)
		{
			bytesPerFrame = internalReader.header.getNumChannels() * sizeof(int16);
			dataChunkStart = 1;
			dataLength = f.getSize() - 1;
		}
	}

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples) override;

	bool mapSectionOfFile(Range<int64> samplesToMap) override;

	void getSample(int64 /*sampleIndex*/, float* result) const noexcept override
	{
		// this should never be used
		jassertfalse;
		*result = 0.0f;
	}

	void setTargetAudioDataType(AudioDataConverters::DataFormat dataType);

private:
	
	friend class HlacSubSectionReader;

	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

	bool copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64 offsetInFile, int numChannels, int numSamples);

	ScopedPointer<MemoryInputStream> mis;
	HlacReaderCommon internalReader;

	bool isMonolith = false;
};

class HlacSubSectionReader: public AudioFormatReader
{
public:

	HlacSubSectionReader(AudioFormatReader* sourceReader, int64 subsectionStartSample, int64 subsectionLength);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
		int64 startSampleInFile, int numSamples);

	void readMaxLevels(int64 startSampleInFile, int64 numSamples, Range<float>* results, int numChannelsToRead);

	void readIntoFixedBuffer(HiseSampleBuffer& buffer, int startSample, int numSamples, int64 readerStartSample);

private:

	bool isMonolith = false;

	HlacMemoryMappedAudioFormatReader* memoryReader;
	HiseLosslessAudioFormatReader* normalReader;

	HlacReaderCommon* internalReader;

	int64 start;
	int64 length;
};

} // namespace hlac

#endif  // HLACAUDIOFORMATREADER_H_INCLUDED
