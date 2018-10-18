
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

namespace hlac { using namespace juce; 

HiseSampleBuffer::HiseSampleBuffer(HiseSampleBuffer& otherBuffer, int offset)
{
	if (otherBuffer.isFloatingPoint())
	{
		jassertfalse;
	}
	else
	{
		size = otherBuffer.size - offset;

		leftIntBuffer = hlac::CompressionHelpers::getPart(otherBuffer.leftIntBuffer, offset, size);

		numChannels = otherBuffer.numChannels;

		if (numChannels > 1)
			rightIntBuffer = hlac::CompressionHelpers::getPart(otherBuffer.rightIntBuffer, offset, size);
	}
}


void HiseSampleBuffer::reverse(int startSample, int numSamples)
{
	if (isFloatingPoint())
	{
		floatBuffer.reverse(startSample, numSamples);

		int fadeLength = jmin<int>(500, numSamples);

		floatBuffer.applyGainRamp(numSamples - fadeLength, fadeLength, 1.0f, 0.0f);

	}
		
	else
	{

		leftIntBuffer.reverse(startSample, numSamples);

		if (numChannels > 1)
			rightIntBuffer.reverse(startSample, numSamples);

	}
		
}


void HiseSampleBuffer::setSize(int numChannels_, int numSamples)
{
	jassert(isPositiveAndBelow(numChannels, 3));



	numChannels = numChannels_;
	size = numSamples;

	if (isFloatingPoint())
		floatBuffer.setSize(numChannels, numSamples);
	else
	{
		leftIntBuffer = FixedSampleBuffer(numSamples);

		if (numChannels > 1)
			rightIntBuffer = FixedSampleBuffer(numSamples);
		else
			rightIntBuffer = FixedSampleBuffer(0);
	}
}

void HiseSampleBuffer::clear()
{
	if (isFloatingPoint())
		floatBuffer.clear();
	else
	{
		hlac::CompressionHelpers::IntVectorOperations::clear(leftIntBuffer.getWritePointer(), leftIntBuffer.size);

		if (hasSecondChannel())
			hlac::CompressionHelpers::IntVectorOperations::clear(rightIntBuffer.getWritePointer(), rightIntBuffer.size);
	}
}

void HiseSampleBuffer::clear(int startSample, int numSamples)
{
	if (numSamples <= 0)
		return;

	if (isFloatingPoint())
		floatBuffer.clear(startSample, numSamples);
	else
	{
		hlac::CompressionHelpers::IntVectorOperations::clear(leftIntBuffer.getWritePointer(startSample), numSamples);

		if (hasSecondChannel())
			hlac::CompressionHelpers::IntVectorOperations::clear(rightIntBuffer.getWritePointer(startSample), numSamples);
	}
}


void HiseSampleBuffer::allocateNormalisationTables(int offsetToUse)
{
	leftIntBuffer.getMap().setOffset(offsetToUse);
	leftIntBuffer.getMap().allocateTableIndexes(size + leftIntBuffer.getMap().getOffset());

	if (hasSecondChannel())
	{
		rightIntBuffer.getMap().setOffset(offsetToUse);
		rightIntBuffer.getMap().allocateTableIndexes(size + rightIntBuffer.getMap().getOffset());
	}
}


void HiseSampleBuffer::flushNormalisationInfo()
{
	auto& lMap = getNormaliseMap(0);
	auto& rMap = getNormaliseMap(1);

	auto s = lMap.size();

	uint8 currentRangeL = lMap.getTableData()[0];
	uint8 currentRangeR = rMap.getTableData()[0];

	int currentStart = 0;
	int currentEnd = 1024;

	for (int i = 1; i < s; i++)
	{
		uint8 nextL = lMap.getTableData()[i];
		uint8 nextR = lMap.getTableData()[i];

		if (nextL != currentRangeL || nextR != currentRangeR)
		{
			Normaliser::NormalisationInfo newInfo;
			newInfo.leftNormalisation = currentRangeL;
			newInfo.rightNormalisation = currentRangeR;
			newInfo.range = { currentStart, currentEnd };

			currentStart = currentEnd;
			currentEnd += 1024;
		}
		else
		{
			currentEnd += 1024;
		}
	}


}

hlac::FixedSampleBuffer& HiseSampleBuffer::getFixedBuffer(int channelIndex)
{
	jassert(!isFloatingPoint());

	if (channelIndex == 0)
	{
		return leftIntBuffer;
	}
	else
	{
		jassert(hasSecondChannel());
		return rightIntBuffer;
	}
}

static int dummy = 0;

void HiseSampleBuffer::copy(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples)
{
	if (numSamples <= 0)
		return;

	if (source.isFloatingPoint() == dst.isFloatingPoint())
	{
		if (source.isFloatingPoint())
		{
			auto byteToCopy = sizeof(float) * numSamples;

			memcpy(dst.getWritePointer(0, startSampleDst), source.getReadPointer(0, startSampleSource), byteToCopy);

			if (dst.hasSecondChannel())
			{
				if (source.hasSecondChannel())
					memcpy(dst.getWritePointer(1, startSampleDst), source.getReadPointer(1, startSampleSource), byteToCopy);
				else
					memcpy(dst.getWritePointer(1, startSampleDst), source.getReadPointer(0, startSampleSource), byteToCopy);
			}
		}
		else
		{
			auto byteToCopy = sizeof(int16) * numSamples;

			memcpy(dst.getWritePointer(0, startSampleDst), source.getReadPointer(0, startSampleSource), byteToCopy);

			if (dst.hasSecondChannel())
			{
				if (source.hasSecondChannel())
					memcpy(dst.getWritePointer(1, startSampleDst), source.getReadPointer(1, startSampleSource), byteToCopy);
				else
					memcpy(dst.getWritePointer(1, startSampleDst), source.getReadPointer(0, startSampleSource), byteToCopy);
			}

			Range<int> srcRange({ startSampleSource, startSampleSource + numSamples });
			Range<int> dstRange({ startSampleDst, startSampleDst + numSamples });

			dst.normaliser.copyFrom(source.normaliser, srcRange, dstRange);

			

#if 0

			auto o1 = (dst.getNormaliseMap(0).getOffset() + startSampleDst) % COMPRESSION_BLOCK_SIZE;
			auto o2 = (source.getNormaliseMap(0).getOffset() + startSampleSource) % COMPRESSION_BLOCK_SIZE;

			

			dst.setUseOneMap(source.useOneMap);
				
			if ((o1 == o2))
			{
				dst.getNormaliseMap(0).copyIntBufferWithNormalisation(source.getNormaliseMap(0), (const int16*)source.getReadPointer(0), (int16*)dst.getWritePointer(0, 0), startSampleSource, startSampleDst, numSamples, true);

				if (dst.hasSecondChannel() && !dst.useOneMap)
				{
					dst.getNormaliseMap(1).copyIntBufferWithNormalisation(source.getNormaliseMap(1), (const int16*)source.getReadPointer(1), (int16*)dst.getWritePointer(1, 0), startSampleSource, startSampleDst, numSamples, true);
				}

				dummy++;

				//AudioThreadGuard::Suspender ss;
				//CompressionHelpers::dump(dst.getFixedBuffer(0), "Merge" + String(dummy) + ".wav");

			}
			else
			{
				equaliseNormalisationAndCopy(dst, source, startSampleDst, startSampleSource, numSamples, 0);

				if (dst.hasSecondChannel())
				{
					equaliseNormalisationAndCopy(dst, source, startSampleDst, startSampleSource, numSamples, 1);
					

				}
			}
#endif
			
		}
	}
	else
	{
		// Data type mismatch!
		jassertfalse;
	}
}

void HiseSampleBuffer::equaliseNormalisationAndCopy(HiseSampleBuffer &dst, const HiseSampleBuffer &source, int startSampleDst, int startSampleSource, int numSamples, int channelIndex)
{
	if (channelIndex == 1 && (dst.useOneMap || source.useOneMap))
		return;

	Range<int> dst_range({ 0, startSampleDst });
	Range<int> src_range({ startSampleSource, startSampleSource + numSamples });
	Range<int> src_dst_range({ startSampleDst, startSampleDst + numSamples });

	auto min_dst = dst.getNormaliseMap(channelIndex).getLowestNormalisationAmount(dst_range);
	auto min_src = source.getNormaliseMap(channelIndex).getLowestNormalisationAmount(src_range);

	auto normalisationLevel = jmin<uint8>(min_dst, min_src);

	auto& map_dst = dst.getNormaliseMap(channelIndex);

	auto dst_ptr = static_cast<int16*>(dst.getWritePointer(channelIndex, 0));

	for (int i = dst_range.getStart(); i < dst_range.getEnd(); i++)
	{
		auto index = map_dst.getIndexForSamplePosition(i + map_dst.getOffset());
		auto diff = map_dst.getTableData()[index] - normalisationLevel;

		if (diff > 0)
		{
			int divider = 1 << diff;

			dst_ptr[i] /= divider;
		}
	}

	auto& map_src = source.getNormaliseMap(channelIndex);
	auto src_ptr = static_cast<const int16*>(source.getReadPointer(channelIndex, 0));

	for (int i = src_range.getStart(); i < src_range.getEnd(); i++)
	{
		auto index = map_src.getIndexForSamplePosition(i + map_src.getOffset());
		auto diff = map_src.getTableData()[index] - normalisationLevel;

		auto dst_index = i - src_range.getStart() + startSampleDst;

		if (diff == 0)
		{
			dst_ptr[dst_index] = src_ptr[i];
		}
		else
		{
			int divider = 1 << diff;
			dst_ptr[dst_index] = src_ptr[i] / divider;
		}
	}

	memset(dst.getNormaliseMap(channelIndex).preallocated, normalisationLevel, 16);
	dst.getNormaliseMap(channelIndex).setOffset(0);
}

void HiseSampleBuffer::add(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples)
{
	if (numSamples <= 0)
		return;

	if (source.isFloatingPoint() && dst.isFloatingPoint())
	{
		dst.floatBuffer.addFrom(0, startSampleDst, source.floatBuffer, 0, startSampleSource, numSamples);

		if (dst.hasSecondChannel())
		{
			if (source.hasSecondChannel())
				dst.floatBuffer.addFrom(1, startSampleDst, source.floatBuffer, 1, startSampleSource, numSamples);
			else
				dst.floatBuffer.addFrom(1, startSampleDst, source.floatBuffer, 0, startSampleSource, numSamples);
		}
	}
	else if (!source.isFloatingPoint() && !dst.isFloatingPoint())
	{
		auto ld = dst.leftIntBuffer.getWritePointer(startSampleDst);
		auto ls = source.leftIntBuffer.getReadPointer(startSampleSource);

		CompressionHelpers::IntVectorOperations::add(ld, ls, numSamples);

		if (dst.hasSecondChannel())
		{
			if (source.hasSecondChannel())
			{
				auto ld2 = dst.rightIntBuffer.getWritePointer(startSampleDst);
				auto ls2 = source.rightIntBuffer.getReadPointer(startSampleSource);

				CompressionHelpers::IntVectorOperations::add(ld2, ls2, numSamples);
			}
			else
			{
				auto ld2 = dst.rightIntBuffer.getWritePointer(startSampleDst);
				auto ls2 = source.leftIntBuffer.getReadPointer(startSampleSource);

				CompressionHelpers::IntVectorOperations::add(ld2, ls2, numSamples);
			}
		}
	}
	else
	{
		// Data type mismatch!
		jassertfalse;
	}
}

void* HiseSampleBuffer::getWritePointer(int channel, int startSample)
{
	if (isFloatingPoint())
	{
		return floatBuffer.getWritePointer(channel, startSample);
	}
	else
	{
		if (channel == 0)
			return leftIntBuffer.getWritePointer(startSample);
		else if (channel == 1 && hasSecondChannel())
		{
			return rightIntBuffer.getWritePointer(startSample);
		}
		else
		{
			jassertfalse;
			return nullptr;
		}
	}
}

const void* HiseSampleBuffer::getReadPointer(int channel, int startSample/*=0*/) const
{
	if (isFloatingPoint())
	{
		return floatBuffer.getReadPointer(channel % numChannels, startSample);
	}
	else
	{
		if (channel == 0 || numChannels == 1)
			return leftIntBuffer.getReadPointer(startSample);
		else if (channel == 1 && hasSecondChannel())
		{
			return rightIntBuffer.getReadPointer(startSample);
		}
		else
		{
			jassertfalse;
			return nullptr;
		}
	}
}

void HiseSampleBuffer::applyGainRamp(int channelIndex, int startOffset, int rampLength, float startGain, float endGain)
{
	if (isFloatingPoint())
	{
		floatBuffer.applyGainRamp(channelIndex, startOffset, rampLength, startGain, endGain);
	}
	else
	{
		if(channelIndex == 0)
			leftIntBuffer.applyGainRamp(startOffset, rampLength, startGain, endGain);

		if(channelIndex == 1 && hasSecondChannel())
			rightIntBuffer.applyGainRamp(startOffset, rampLength, startGain, endGain);
	}
}

AudioSampleBuffer* HiseSampleBuffer::getFloatBufferForFileReader()
{
	jassert(isFloatingPoint());

	return &floatBuffer;
}

const hlac::CompressionHelpers::NormaliseMap& HiseSampleBuffer::getNormaliseMap(int channelIndex) const
{
	return (channelIndex == 0) ? leftIntBuffer.getMap() : rightIntBuffer.getMap();
}

hlac::CompressionHelpers::NormaliseMap& HiseSampleBuffer::getNormaliseMap(int channelIndex)
{
	return (channelIndex == 0) ? leftIntBuffer.getMap() : rightIntBuffer.getMap();
}

} // namespace hlac
