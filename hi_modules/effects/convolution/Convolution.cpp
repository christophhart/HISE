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

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4127 4706 4100)
#endif

namespace wdl
{

#include "wdl/convoengine.cpp"
#include "wdl/fft.c"

}

#if JUCE_MSVC
 #pragma warning (pop)
#endif

ConvolutionEffect::ConvolutionEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
AudioSampleProcessor(this),
dryGain(0.0f),
wetGain(1.0f),
latency(0),
isReloading(false),
rampFlag(false),
rampIndex(0),
processFlag(true)
{
	parameterNames.add("DryGain");
	parameterNames.add("WetGain");
	parameterNames.add("Latency");
	parameterNames.add("ImpulseLength");
	parameterNames.add("ProcessInput");

	

	
	

}

void ConvolutionEffect::setImpulse()
{
	if (getSampleBuffer() == nullptr) return;

	if (getSampleBuffer()->getNumChannels() == 0) return;

	ScopedValueSetter<bool> s(isReloading, true);

	ScopedLock sl(lock);

	convolutionEngine.Reset();

	impulseBuffer.SetNumChannels(getSampleBuffer()->getNumChannels());
	const int numSamples = impulseBuffer.SetLength(length);

	float *bufferL = impulseBuffer.impulses[0].Get();
	float *bufferR = getSampleBuffer()->getNumChannels() > 1 ? impulseBuffer.impulses[1].Get() : bufferL;

	FloatVectorOperations::copy(bufferL, getSampleBuffer()->getReadPointer(0, sampleRange.getStart()), numSamples);
	if (getSampleBuffer()->getNumChannels() > 1)
	{
		FloatVectorOperations::copy(bufferR, getSampleBuffer()->getReadPointer(1, sampleRange.getStart()), numSamples);

	}

	convolutionEngine.SetImpulse(&impulseBuffer, 0, getBlockSize(), 0, 0, getBlockSize());
}

float ConvolutionEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DryGain:		return Decibels::gainToDecibels(dryGain);
	case WetGain:		return Decibels::gainToDecibels(wetGain);
	case Latency:		return (float)latency;
	case ImpulseLength:	return 1.0f;
	case ProcessInput:	return processFlag ? 1.0f : 0.0f;
	default:			jassertfalse; return 1.0f;
	}
}

void ConvolutionEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case DryGain:		dryGain = Decibels::decibelsToGain(newValue); break;
	case WetGain:		wetGain = Decibels::decibelsToGain(newValue); break;
	case Latency:		latency = (int)newValue;
		jassert(isPowerOfTwo(latency));
		setImpulse();
		break;
	case ImpulseLength:	setImpulse();
		break;
	case ProcessInput:	enableProcessing(newValue >= 0.5f); break;
	default:			jassertfalse; return;
	}
}

void ConvolutionEffect::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(DryGain, "DryGain");
	loadAttribute(WetGain, "WetGain");
	loadAttribute(Latency, "Latency");
	loadAttribute(ImpulseLength, "ImpulseLength");
	loadAttribute(ProcessInput, "ProcessInput");

	AudioSampleProcessor::restoreFromValueTree(v);
}

ValueTree ConvolutionEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(DryGain, "DryGain");
	saveAttribute(WetGain, "WetGain");
	saveAttribute(Latency, "Latency");
	saveAttribute(ImpulseLength, "ImpulseLength");
	saveAttribute(ProcessInput, "ProcessInput");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

void ConvolutionEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    ScopedLock sl(lock);
    
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);



	convolutionEngine.Reset();
}

void ConvolutionEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	if (startSample != 0)
	{
		debugError(this, "Buffer start not 0!");
	}

	

	ScopedLock sl(lock);

	float *l = buffer.getWritePointer(0, 0);
	float *r = buffer.getWritePointer(1, 0);

	float *channels[2] = { l, r };



	if (isReloading || (!processFlag && !rampFlag))
	{
		FloatVectorOperations::multiply(l, dryGain, numSamples);
		FloatVectorOperations::multiply(r, dryGain, numSamples);

		currentValues.inL = FloatVectorOperations::findMaximum(l, numSamples);
		currentValues.inR = FloatVectorOperations::findMaximum(l, numSamples);

		return;
	}

	convolutionEngine.Add(channels, numSamples, 2);

	FloatVectorOperations::multiply(l, dryGain, numSamples);
	FloatVectorOperations::multiply(r, dryGain, numSamples);

	currentValues.inL = FloatVectorOperations::findMaximum(l, numSamples);
	currentValues.inR = FloatVectorOperations::findMaximum(l, numSamples);

	const int availableSamples = jmin(convolutionEngine.Avail(numSamples), numSamples);

	if (availableSamples > 0)
	{
		const float *convolutedL = convolutionEngine.Get()[0];
		const float *convolutedR = convolutionEngine.Get()[1];

		currentValues.outL = wetGain * FloatVectorOperations::findMaximum(convolutedL, availableSamples);
		currentValues.outR = wetGain * FloatVectorOperations::findMaximum(convolutedR, availableSamples);

		if (rampFlag)
		{
			const int rampingTime = (CONVOLUTION_RAMPING_TIME_MS * (int)getSampleRate()) / 1000;

            
            
            
			for (int i = 0; i < availableSamples; i++)
			{
                float rampValue = jlimit<float>(0.0f, 1.0f, (float)rampIndex / (float)rampingTime);
                
                //rampValue *= rampValue; // Cheap mans logarithm
                
                const float gainValue = wetGain * (float)(rampUp ? rampValue : (1.0f - rampValue));
                l[startSample + i] += gainValue * convolutedL[i];
                r[startSample + i] += gainValue * convolutedR[i];
                
				rampIndex++;
			}

			convolutionEngine.Advance(availableSamples);

			if (rampIndex >= rampingTime)
			{
				if (!processFlag)
				{
					convolutionEngine.Reset();
				}

				rampFlag = false;
			}
		}
		else
		{

			FloatVectorOperations::addWithMultiply(l, convolutedL, wetGain, availableSamples);
			FloatVectorOperations::addWithMultiply(r, convolutedR, wetGain, availableSamples);

			convolutionEngine.Advance(availableSamples);
		}



	}
}

ProcessorEditorBody *ConvolutionEffect::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ConvolutionEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

void ConvolutionEffect::enableProcessing(bool shouldBeProcessed)
{
	if (processFlag != shouldBeProcessed)
	{
		ScopedLock sl(lock);

		processFlag = shouldBeProcessed;

		rampFlag = true;
		rampUp = shouldBeProcessed;
		rampIndex = 0;
	}
}
