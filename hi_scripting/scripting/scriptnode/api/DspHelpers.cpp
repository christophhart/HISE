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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;







void DspHelpers::increaseBuffer(AudioSampleBuffer& b, const PrepareSpecs& specs)
{
	auto numChannels = specs.numChannels;
	auto numSamples = specs.blockSize;

	if (numChannels != b.getNumChannels() ||
		b.getNumSamples() < numSamples)
	{
		b.setSize(numChannels, numSamples);
	}
}

void DspHelpers::increaseBuffer(snex::Types::heap<float>& b, const PrepareSpecs& specs, bool clearForFrame)
{
    if(specs.blockSize == 1 && clearForFrame)
    {
        b.setSize(0);
        return;
    }
    
	auto numChannels = specs.numChannels;
	auto numSamples = specs.blockSize;
	auto numElements = numChannels * numSamples;

	if (numElements > b.size())
		b.setSize(numElements);
}

void DspHelpers::setErrorIfFrameProcessing(const PrepareSpecs& ps)
{
	if (ps.blockSize == 1)
		Error::throwError(Error::IllegalFrameCall);
}

void DspHelpers::setErrorIfNotOriginalSamplerate(const PrepareSpecs& ps, NodeBase* n)
{
	auto original = n->getRootNetwork()->getOriginalSampleRate();

	if (ps.sampleRate != original)
	{
		Error::throwError(Error::SampleRateMismatch, (int)original, (int)ps.sampleRate);
	}
}


#if 0
scriptnode::DspHelpers::ParameterCallback DspHelpers::getFunctionFrom0To1ForRange(InvertableParameterRange range, const ParameterCallback& originalFunction)
{
	if (RangeHelpers::isIdentity(range))
	{
		if (!range.inv)
			return originalFunction;
		else
		{
			return [originalFunction](double normalisedValue)
			{
				originalFunction(1.0 - normalisedValue);
			};
		}
	}

	return [range, originalFunction](double normalisedValue)
	{
		auto v = range.convertFrom0to1(normalisedValue);
		originalFunction(v);
	};
}
#endif

void DspHelpers::validate(PrepareSpecs sp, PrepareSpecs rp)
{
	auto isIdle = [](const PrepareSpecs& p)
	{
		return p.numChannels == 0 && p.sampleRate == 0.0 && p.blockSize == 0;
	};

	if (isIdle(sp) || isIdle(rp))
		return;

	if (sp.numChannels != rp.numChannels)
		Error::throwError(Error::ChannelMismatch, sp.numChannels, rp.numChannels);
	if (sp.sampleRate != rp.sampleRate)
		Error::throwError(Error::SampleRateMismatch, sp.sampleRate, rp.sampleRate);
	if (sp.blockSize != rp.blockSize)
		Error::throwError(Error::BlockSizeMismatch, sp.blockSize, rp.blockSize);
}

void DspHelpers::throwIfFrame(PrepareSpecs ps)
{
	if (ps.blockSize == 1)
		Error::throwError(Error::IllegalFrameCall);
}

}

