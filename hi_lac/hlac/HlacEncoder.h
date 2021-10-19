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

namespace hlac {

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
		int16_t fixedBlockWidth = -1;
		bool removeDcOffset = true;
		bool applyDithering = false;
		uint8_t normalisationMode = 0;
		uint8_t normalisationThreshold = 4;
		int bitRateForWholeBlock = 6;
		bool useDiffEncodingWithFixedBlocks = false;

		static juce::String getBoolString(bool b)
		{
			return b ? "true" : "false";
		}

        juce::String toString() const
		{
            juce::String s;
            juce::NewLine nl;

			s << "useCompression: " << getBoolString(useCompression) << nl;
			s << "fixedBlockWidth: " << juce::String(fixedBlockWidth) << nl;
			s << "removeDCOffset: " << getBoolString(removeDcOffset) << nl;
			s << "bitRateForWholeBlock: " << juce::String(bitRateForWholeBlock) << nl;
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


	void compress(juce::AudioSampleBuffer& source, juce::OutputStream& output, uint32_t* blockOffsetData);
	
	void reset();

	void setOptions(CompressorOptions& newOptions)
	{
		options = newOptions;
	}

	float getCompressionRatio() const;

	uint32_t getNumBlocksWritten() const { return blockIndex; }

private:

	bool encodeBlock (juce::AudioSampleBuffer& block, juce::OutputStream& output);

	bool encodeBlock(CompressionHelpers::AudioBufferInt16& block, juce::OutputStream& output);

	bool normaliseBlockAndAddHeader(CompressionHelpers::AudioBufferInt16& block16, juce::OutputStream& output);

    juce::MemoryBlock createCompressedBlock(CompressionHelpers::AudioBufferInt16& block);

	uint8_t getBitReductionAmountForMSEncoding (juce::AudioSampleBuffer& block);

	bool isBlockExhausted() const
	{
		return indexInBlock >= COMPRESSION_BLOCK_SIZE;
	}

	bool writeChecksumBytesForBlock (juce::OutputStream& output);

	bool writeNormalisationAmount (juce::OutputStream& output);

	bool writeUncompressed (CompressionHelpers::AudioBufferInt16& block, juce::OutputStream& output);

	bool encodeCycle (CompressionHelpers::AudioBufferInt16& cycle, juce::OutputStream& output);
	bool encodeDiff (CompressionHelpers::AudioBufferInt16& cycle, juce::OutputStream& output);
	bool encodeCycleDelta(CompressionHelpers::AudioBufferInt16& nextCycle, juce::OutputStream& output);
	void  encodeLastBlock (juce::AudioSampleBuffer& block, juce::OutputStream& output);

	bool writeCycleHeader(bool isTemplate, int bitDepth, int numSamples, juce::OutputStream& output);
	bool writeDiffHeader(int fullBitRate, int errorBitRate, int blockSize, juce::OutputStream& output);

	int getCycleLength(CompressionHelpers::AudioBufferInt16& block);
	int getCycleLengthFromTemplate(CompressionHelpers::AudioBufferInt16& newCycle, CompressionHelpers::AudioBufferInt16& rest);

	BitCompressors::Collection collection;

	CompressionHelpers::AudioBufferInt16 currentCycle;

	CompressionHelpers::AudioBufferInt16 workBuffer;

	int indexInBlock = 0;

	uint32_t numBytesWritten = 0;
	uint32_t numBytesUncompressed = 0;

	uint32_t numTemplates = 0;
	uint32_t numDeltas = 0;

	uint32_t blockOffset = 0;
	uint32_t blockIndex = 0;

	uint8_t bitRateForCurrentCycle = 0;

	int firstCycleLength = -1;

    juce::MemoryBlock readBuffer;

	CompressorOptions options;

	int currentNormaliseBitShiftAmount = 0;

	float ratio = 0.0f;

	uint64_t readIndex = 0;

	double decompressionSpeed = 0.0;
};

} // namespace hlac

#endif  // HLACENCODER_H_INCLUDED
