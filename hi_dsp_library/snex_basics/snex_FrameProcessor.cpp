/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace Types {
using namespace juce;


template <int NumChannels>
float* snex::Types::FrameProcessor<NumChannels>::end() const
{
	return frameData.end();
}

template <int NumChannels>
float* snex::Types::FrameProcessor<NumChannels>::begin() const
{
	return frameData.begin();
}

template <int NumChannels>
snex::Types::FrameProcessor<NumChannels>::operator const FrameType&() const
{
	return frameData;
}


template <int NumChannels>
int snex::Types::FrameProcessor<NumChannels>::next()
{
	return nextFrame(this);
}


template <int NumChannels>
snex::Types::FrameProcessor<NumChannels>::operator FrameType&()
{
	return frameData;
}


template <int NumChannels>
void snex::Types::FrameProcessor<NumChannels>::writeLastFrame()
{
	jassert(frameIndex > 0);

	for (int i = 0; i < NumChannels; i++)
		channels[i][frameIndex - 1] = frameData[i];
}


template <int NumChannels>
void snex::Types::FrameProcessor<NumChannels>::loadFrame()
{
	for (int i = 0; i < NumChannels; i++)
		frameData[i] = channels[i][frameIndex];
}


template <int NumChannels>
snex::Types::FrameProcessor<NumChannels>::FrameProcessor(float** processDataPointers, int numSamples) :
	channels(*reinterpret_cast<span<float*, NumChannels>*>(processDataPointers))
{
	frameLimit = numSamples;
	loadFrame();
}


template <int NumChannels>
int snex::Types::FrameProcessor<NumChannels>::nextFrame(void* obj)
{
	auto fp = static_cast<FrameProcessor<NumChannels>*>(obj);

	if (fp->frameIndex == 0)
	{
		++fp->frameIndex;
		return fp->frameLimit;
	}

	fp->writeLastFrame();

	if (fp->frameIndex < fp->frameLimit)
	{
		fp->loadFrame();
		++fp->frameIndex;
		return 1;
	}

	return  0;
}


}
}