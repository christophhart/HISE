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

#ifndef HLACAUDIOFORMATREADER_H_INCLUDED
#define HLACAUDIOFORMATREADER_H_INCLUDED



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
	}

	HlacReaderCommon(const File& f) :
		input(nullptr),
		header(f)
	{
		decoder.setupForDecompression();
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

	bool internalHlacRead(int** destSamples, int numDestChannels, int startOffsetInDestBuffer, int64 startSampleInFile, int numSamples);

	

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

	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

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

private:
	
	static void copySampleData(int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels, const void* sourceData, int numChannels, int numSamples) noexcept;

	ScopedPointer<MemoryInputStream> mis;
	HlacReaderCommon internalReader;

	bool isMonolith = false;
};


#endif  // HLACAUDIOFORMATREADER_H_INCLUDED
