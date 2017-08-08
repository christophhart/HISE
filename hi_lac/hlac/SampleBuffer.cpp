
void HiseSampleBuffer::reverse(int startSample, int numSamples)
{
	if (isFloatingPoint())
		floatBuffer.reverse(startSample, numSamples);
	else
		throw std::logic_error("not implemented");
}

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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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
	if (isFloatingPoint())
		floatBuffer.clear(startSample, numSamples);
	else
	{
		hlac::CompressionHelpers::IntVectorOperations::clear(leftIntBuffer.getWritePointer(startSample), numSamples);

		if (hasSecondChannel())
			hlac::CompressionHelpers::IntVectorOperations::clear(rightIntBuffer.getWritePointer(startSample), numSamples);
	}
}

void HiseSampleBuffer::copy(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples)
{
	if (source.isFloatingPoint() == dst.isFloatingPoint())
	{
		auto byteToCopy = (source.isFloatingPoint() ? sizeof(float) : sizeof(int16)) * numSamples;

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
		// Data type mismatch!
		jassertfalse;
	}
}

void HiseSampleBuffer::add(HiseSampleBuffer& dst, const HiseSampleBuffer& source, int startSampleDst, int startSampleSource, int numSamples)
{
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
		throw std::logic_error("not implemented");
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
		return floatBuffer.getReadPointer(channel, startSample);
	}
	else
	{
		if (channel == 0)
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
		throw std::logic_error("not implemented");
	}
}

AudioSampleBuffer* HiseSampleBuffer::getFloatBufferForFileReader()
{
	jassert(isFloatingPoint());

	return &floatBuffer;
}
