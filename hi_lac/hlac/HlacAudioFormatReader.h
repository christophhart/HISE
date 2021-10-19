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

namespace hlac {

struct HiseLosslessHeader
{
	HiseLosslessHeader(juce::InputStream* input);

	HiseLosslessHeader(const juce::File& f);

	HiseLosslessHeader(bool useEncryption, uint8_t globalBitShiftAmount, double sampleRate, int numChannels, int bitsPerSample, bool useCompression, uint32_t numBlocks);

	int getVersion() const;
	bool isEncrypted() const;
	int getBitShiftAmount() const;
	uint32_t getNumChannels() const;
	uint32_t getBitsPerSample() const;
	bool usesCompression() const;
	double getSampleRate() const;
	uint32_t getBlockAmount() const;

	uint32_t getOffsetForReadPosition(int64_t samplePosition, bool addHeaderOffset);

	uint32_t getOffsetForNextBlock(int64_t samplePosition, bool addHeaderOffset);

	bool write(juce::OutputStream* output);

	void storeOffsets(uint32_t* offsets, int numOffsets);

	void readMetadataFromStream(juce::InputStream* stream);

	static HiseLosslessHeader createMonolithHeader(int numChannels, double sampleRate);

private:

	uint8_t headerByte1 = 0;
	uint8_t headerByte2 = 0;
	uint8_t sampleDataByte = 0;
	uint32_t blockAmount = 0;
    juce::HeapBlock<uint32_t> blockOffsets;
	bool headerValid = false;
	bool isOldMonolith = false;
	uint32_t headerSize;
};

class HlacReaderCommon
{
public:

	HlacReaderCommon(juce::InputStream* input_):
		input(input_),
		header(input)
	{
		decoder.setupForDecompression();
		decoder.setHlacVersion(header.getVersion());
	}

	HlacReaderCommon(const juce::File& f) :
		input(nullptr),
		header(f)
	{
		decoder.setupForDecompression();
		decoder.setHlacVersion(header.getVersion());
	}

	/** You can choose what the target data type should be. If you read into integer AudioSampleBuffers, you might want to call this method
	*	in order to save unnecessary conversions between float and integer numbers. */
	void setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType);

	/** When seeking, add the length of the header as offset. */
	void setUseHeaderOffsetWhenSeeking(bool shouldUseHeaderOffset)
	{
		useHeaderOffsetWhenSeeking = shouldUseHeaderOffset;
	};

private:

	friend class HlacSubSectionReader;

	bool internalHlacRead(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples);

	bool fixedBufferRead(HiseSampleBuffer& buffer, int numDestChannels, int startOffsetInBuffer, int64_t startSampleInFile, int numSamples);

	

	friend class HiseLosslessAudioFormatReader;
	friend class HlacMemoryMappedAudioFormatReader;

    juce::InputStream* input;

	HlacDecoder decoder;
	HiseLosslessHeader header;

	bool usesFloatingPointData;

	bool useHeaderOffsetWhenSeeking = true;

};

class HiseLosslessAudioFormatReader : public juce::AudioFormatReader
{
public:
	HiseLosslessAudioFormatReader(juce::InputStream* input_);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples) override;

	double getDecompressionPerformanceForLastFile() { return internalReader.decoder.getDecompressionPerformance(); }

	void setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType);

private:

	friend class HlacSubSectionReader;


	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

	bool copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64_t offsetInFile, int numChannels, int numSamples);

	HlacReaderCommon internalReader;

	bool isMonolith = false;

};


class HlacMemoryMappedAudioFormatReader : public juce::MemoryMappedAudioFormatReader
{
public:

	HlacMemoryMappedAudioFormatReader(const juce::File& f, const AudioFormatReader& details, int64_t start, int64_t length, int frameSize) :
		MemoryMappedAudioFormatReader(f, details, start, length, frameSize),
		internalReader(f)
	{
		isMonolith = internalReader.header.getVersion() < 2;

		if (isMonolith)
		{
			bytesPerFrame = internalReader.header.getNumChannels() * sizeof(int16_t);
			dataChunkStart = 1;
			dataLength = f.getSize() - 1;
		}
	}

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64_t startSampleInFile, int numSamples) override;

	bool mapSectionOfFile(juce::Range<int64_t> samplesToMap) override;

	void getSample(int64_t /*sampleIndex*/, float* result) const noexcept override
	{
		// this should never be used
		jassertfalse;
		*result = 0.0f;
	}

	void setTargetAudioDataType(juce::AudioDataConverters::DataFormat dataType);

private:
	
	friend class HlacSubSectionReader;

	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

	bool copyFromMonolith(HiseSampleBuffer& destination, int startOffsetInBuffer, int numDestChannels, int64_t offsetInFile, int numChannels, int numSamples);

	std::unique_ptr<juce::MemoryInputStream> mis;
	HlacReaderCommon internalReader;

	bool isMonolith = false;
};

class HlacSubSectionReader: public juce::AudioFormatReader
{
public:

	HlacSubSectionReader(AudioFormatReader* sourceReader, int64_t subsectionStartSample, int64_t subsectionLength);

	bool readSamples(int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
		int64_t startSampleInFile, int numSamples);

	void readMaxLevels(int64_t startSampleInFile, int64_t numSamples, juce::Range<float>* results, int numChannelsToRead);

	void readIntoFixedBuffer(HiseSampleBuffer& buffer, int startSample, int numSamples, int64_t readerStartSample);

private:

	bool isMonolith = false;

	HlacMemoryMappedAudioFormatReader* memoryReader = nullptr;
	HiseLosslessAudioFormatReader* normalReader     = nullptr;

	HlacReaderCommon* internalReader = nullptr;

	int64_t start  = 0;
	int64_t length = 0;
};

} // namespace hlac

#endif  // HLACAUDIOFORMATREADER_H_INCLUDED
