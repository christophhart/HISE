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


#ifndef HLACENCODER_H_INCLUDED
#define HLACENCODER_H_INCLUDED

namespace hlac { using namespace juce; 

class HlacEncoder
{
public:

	HlacEncoder():
		currentCycle(0),
		workBuffer(0)
	{
		reset();
	};

	~HlacEncoder();

	struct CompressorOptions
	{
		enum class Presets
		{
			Uncompressed = 0,
			WholeBlock = 1,
			Diff,
			numPresets
		};

		bool useCompression = true;
		int16 fixedBlockWidth = -1;
		bool removeDcOffset = true;
		bool applyDithering = false;
		uint8 normalisationMode = 0;
		uint8 normalisationThreshold = 4;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;

		static String getBoolString(bool b)
		{
			return b ? "true" : "false";
		}

		String toString() const
		{
			String s;
			NewLine nl;

			s << "useCompression: " << getBoolString(useCompression) << nl;
			s << "fixedBlockWidth: " << String(fixedBlockWidth) << nl;
			s << "removeDCOffset: " << getBoolString(removeDcOffset) << nl;
			s << "bitRateForWholeBlock: " << String(bitRateForWholeBlock) << nl;
			s << "useDiffEncodingWithFixedBlocks: " << getBoolString(useDiffEncodingWithFixedBlocks) << nl;

			return s;
		}

		static CompressorOptions getPreset(Presets p)
		{
			if (p == Presets::Uncompressed)
			{
				HlacEncoder::CompressorOptions uncompressed;

				uncompressed.fixedBlockWidth = 1024;
				uncompressed.useCompression = false;
				uncompressed.removeDcOffset = false;
				uncompressed.useDiffEncodingWithFixedBlocks = false;

				return uncompressed;
			}
			if (p == Presets::WholeBlock)
			{
				HlacEncoder::CompressorOptions wholeBlock;

				wholeBlock.fixedBlockWidth = 1024;
				wholeBlock.removeDcOffset = false;
				wholeBlock.useDiffEncodingWithFixedBlocks = false;

				return wholeBlock;
			}
			if (p == Presets::Diff)
			{
				HlacEncoder::CompressorOptions diff;

				diff.fixedBlockWidth = 1024;
				diff.removeDcOffset = false;
				diff.bitRateForWholeBlock = 4;
				diff.useDiffEncodingWithFixedBlocks = true;

				return diff;
			}

			return CompressorOptions();
		}

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

	bool normaliseBlockAndAddHeader(CompressionHelpers::AudioBufferInt16& block16, OutputStream& output);

	MemoryBlock createCompressedBlock(CompressionHelpers::AudioBufferInt16& block);

	uint8 getBitReductionAmountForMSEncoding(AudioSampleBuffer& block);

	bool isBlockExhausted() const
	{
		return indexInBlock >= COMPRESSION_BLOCK_SIZE;
	}

	bool writeChecksumBytesForBlock(OutputStream& output);

	bool writeNormalisationAmount(OutputStream& output);

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

	int currentNormaliseBitShiftAmount = 0;

	float ratio = 0.0f;

	uint64 readIndex = 0;

	double decompressionSpeed = 0.0;
};

} // namespace hlac

#endif  // HLACENCODER_H_INCLUDED
