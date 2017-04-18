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


#ifndef HLACENCODER_H_INCLUDED
#define HLACENCODER_H_INCLUDED

class HlacEncoder
{
public:

	HlacEncoder():
		currentCycle(0),
		workBuffer(0)
	{
		reset();
	};

	struct CompressorOptions
	{
		bool useDeltaEncoding = true;
		int16 fixedBlockWidth = -1;
		bool reuseFirstCycleLengthForBlock = true;
		bool removeDcOffset = true;
		float deltaCycleThreshhold = 0.2f;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;
	};


	void compress(AudioSampleBuffer& source, OutputStream& output, uint32* blockOffsetData);
	
	void reset();

	void setOptions(CompressorOptions& newOptions)
	{
		options = newOptions;
	}

	float getCompressionRatio() const;

	uint32 getNumBlocksWritten() const { return blockIndex; }

private:

	bool encodeBlock(AudioSampleBuffer& block, OutputStream& output);

	bool encodeBlock(CompressionHelpers::AudioBufferInt16& block, OutputStream& output);

	MemoryBlock createCompressedBlock(CompressionHelpers::AudioBufferInt16& block);

	uint8 getBitReductionAmountForMSEncoding(AudioSampleBuffer& block);

	bool isBlockExhausted() const
	{
		return indexInBlock >= COMPRESSION_BLOCK_SIZE;
	}


	bool writeUncompressed(CompressionHelpers::AudioBufferInt16& block, OutputStream& output);

	bool encodeCycle(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeDiff(CompressionHelpers::AudioBufferInt16& cycle, OutputStream& output);
	bool encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, OutputStream& output);
	void  encodeLastBlock(AudioSampleBuffer& block, OutputStream& output);

	bool writeCycleHeader(bool isTemplate, int bitDepth, int numSamples, OutputStream& output);
	bool writeDiffHeader(int fullBitRate, int errorBitRate, int blockSize, OutputStream& output);

	int getCycleLength(CompressionHelpers::AudioBufferInt16& block);
	int getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	int indexInBlock = 0;

	uint32 numBytesWritten = 0;
	uint32 numBytesUncompressed = 0;

	uint32 numTemplates = 0;
	uint32 numDeltas = 0;

	uint32 blockOffset = 0;
	uint32 blockIndex = 0;


	uint8 bitRateForCurrentCycle = 0;

	int firstCycleLength = -1;

	MemoryBlock readBuffer;

	CompressorOptions options;

	float ratio = 0.0f;

	uint64 readIndex = 0;

	double decompressionSpeed = 0.0;
};


#endif  // HLACENCODER_H_INCLUDED
