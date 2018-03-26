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


namespace hise { using namespace juce;

#if !USE_FFT_CONVOLVER
struct ConvolutionEffect::WdlPimpl
{
	wdl::WDL_ImpulseBuffer impulseBuffer;
	wdl::WDL_ConvolutionEngine_Div convolutionEngine;
};
#endif

ConvolutionEffect::ConvolutionEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
AudioSampleProcessor(this),
dryGain(0.0f),
wetGain(1.0f),
wetBuffer(2, 0),
latency(0),
isReloading(false),
rampFlag(false),
rampIndex(0),
processFlag(true),
loadAfterProcessFlag(false),
isCurrentlyProcessing(false),
#if USE_FFT_CONVOLVER
convolverL(new MultithreadedConvolver()),
convolverR(new MultithreadedConvolver())
#else
wdlPimpl(new WdlPimpl())
#endif
{
	parameterNames.add("DryGain");
	parameterNames.add("WetGain");
	parameterNames.add("Latency");
	parameterNames.add("ImpulseLength");
	parameterNames.add("ProcessInput");
	parameterNames.add("UseBackgroundThread");
	parameterNames.add("Predelay");
	parameterNames.add("Damping");

	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);

	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 0.0f);

#if USE_FFT_CONVOLVER
	convolverL->reset();
	convolverR->reset();
#endif
}

ConvolutionEffect::~ConvolutionEffect()
{
#if USE_FFT_CONVOLVER
	convolverL = nullptr;
	convolverR = nullptr;
#else
	wdlPimpl = nullptr;
#endif


}

void ConvolutionEffect::setImpulse()
{
	if (getSampleBuffer() == nullptr) return;

	if (getSampleBuffer()->getNumChannels() == 0) return;

	ScopedValueSetter<bool> s(isReloading, true);

	ScopedLock sl(getImpulseLock());

#if USE_FFT_CONVOLVER

	const auto offset = getRange().getStart();
	const auto irLength = getRange().getLength();

	auto l = getSampleBuffer()->getReadPointer(0, offset);
	auto r = getSampleBuffer()->getReadPointer(1, offset);

	auto resampleRatio = getResampleFactor();

	int resampledLength = roundDoubleToInt((double)irLength * resampleRatio);
	AudioSampleBuffer scratchBuffer(2, resampledLength);

	if (resampleRatio != 1.0)
	{
		
		LagrangeInterpolator resampler;

		resampler.process(resampleRatio, l, scratchBuffer.getWritePointer(0), resampledLength);
		resampler.process(resampleRatio, r, scratchBuffer.getWritePointer(1), resampledLength);
	}
	else
	{
		FloatVectorOperations::copy(scratchBuffer.getWritePointer(0), l, irLength);
		FloatVectorOperations::copy(scratchBuffer.getWritePointer(1), r, irLength);
	}

	if (damping != 1.0f)
		scratchBuffer.applyGainRamp(0, resampledLength, 1.0f, damping);

	
	convolverL->reset();
	convolverR->reset();

    const auto headSize = nextPowerOfTwo(getBlockSize());
	const auto fullTailLength = nextPowerOfTwo(resampledLength - headSize);

	convolverL->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(0), resampledLength);
	convolverR->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(1), resampledLength);

#else

	wdlPimpl->convolutionEngine.Reset();

	wdlPimpl->impulseBuffer.SetNumChannels(getSampleBuffer()->getNumChannels());
	const int numSamples = wdlPimpl->impulseBuffer.SetLength(length);

	float *bufferL = wdlPimpl->impulseBuffer.impulses[0].Get();
	float *bufferR = getSampleBuffer()->getNumChannels() > 1 ? wdlPimpl->impulseBuffer.impulses[1].Get() : bufferL;

	FloatVectorOperations::copy(bufferL, getSampleBuffer()->getReadPointer(0, sampleRange.getStart()), numSamples);
	if (getSampleBuffer()->getNumChannels() > 1)
	{
		FloatVectorOperations::copy(bufferR, getSampleBuffer()->getReadPointer(1, sampleRange.getStart()), numSamples);

	}

	wdlPimpl->convolutionEngine.SetImpulse(&(wdlPimpl->impulseBuffer), 0, getBlockSize(), 0, 0, getBlockSize());
#endif
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
	case UseBackgroundThread:	return convolverL->isUsingBackgroundThread() ? 1.0f : 0.0f;
	case Predelay:		return predelayMs;
	case Damping:		return Decibels::gainToDecibels(damping);
	default:			jassertfalse; return 1.0f;
	}
}

void ConvolutionEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case DryGain:		dryGain = Decibels::decibelsToGain(newValue); 
						smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, dryGain); 
						break;
	case WetGain:		wetGain = Decibels::decibelsToGain(newValue);
						smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, wetGain);
						break;
	case Latency:		latency = (int)newValue;
		jassert(isPowerOfTwo(latency));
		setImpulse();
		break;
	case ImpulseLength:	setImpulse();
		break;
	case ProcessInput:	enableProcessing(newValue >= 0.5f); break;
	case UseBackgroundThread:	convolverL->setUseBackgroundThread(newValue > 0.5f);
								convolverR->setUseBackgroundThread(newValue > 0.5f);
								break;
	case Predelay:		predelayMs = newValue;
						calcPredelay();
						break;
	case Damping:		damping = Decibels::decibelsToGain(newValue); break;
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
	loadAttribute(UseBackgroundThread, "UseBackgroundThread");
	loadAttribute(Predelay, "Predelay");
	loadAttribute(Damping, "Damping");

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
	saveAttribute(UseBackgroundThread, "UseBackgroundThread");
	saveAttribute(Predelay, "Predelay");
	saveAttribute(Damping, "Damping");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

void ConvolutionEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	ProcessorHelpers::increaseBufferIfNeeded(wetBuffer, samplesPerBlock);

	if (sampleRate != lastSampleRate)
	{
		ScopedLock sl(getImpulseLock());

		lastSampleRate = sampleRate;

		smoothedGainerWet.prepareToPlay(sampleRate, samplesPerBlock);
		smoothedGainerDry.prepareToPlay(sampleRate, samplesPerBlock);

		leftPredelay.prepareToPlay(sampleRate);
		rightPredelay.prepareToPlay(sampleRate);

#if USE_FFT_CONVOLVER
		// TODO: Resample IR
#else
		wdlPimpl->convolutionEngine.Reset();
#endif
	}

	
}

void ConvolutionEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	ADD_GLITCH_DETECTOR(this, DebugLogger::Location::ConvolutionRendering);
    
	if (startSample != 0)
	{
		debugError(this, "Buffer start not 0!");
	}

	float *l = buffer.getWritePointer(0, 0);
	float *r = buffer.getWritePointer(1, 0);

	float *channels[2] = { l, r };

	isCurrentlyProcessing.store(true);

	ScopedLock sl(getImpulseLock());

	if (isReloading || (!processFlag && !rampFlag))
	{
		smoothedGainerDry.processBlock(channels, 2, numSamples);

#if ENABLE_ALL_PEAK_METERS
		currentValues.inL = FloatVectorOperations::findMaximum(l, numSamples);
		currentValues.inR = FloatVectorOperations::findMaximum(r, numSamples);
#endif

		isCurrentlyProcessing.store(false);
		return;
	}

#if !USE_FFT_CONVOLVER
	wdlPimpl->convolutionEngine.Add(channels, numSamples, 2);
	smoothedGainerDry.processBlock(channels, 2, numSamples);
#endif

	

#if !USE_FFT_CONVOLVER
	const int availableSamples = jmin(wdlPimpl->convolutionEngine.Avail(numSamples), numSamples);
#else
	const int availableSamples = numSamples;
#endif

	

	if (availableSamples > 0)
	{
#if USE_FFT_CONVOLVER

        float* convolutedL = wetBuffer.getWritePointer(0);//reinterpret_cast<float*>(alloca(sizeof(float) * numSamples));
        float* convolutedR = wetBuffer.getWritePointer(1);//reinterpret_cast<float*>(alloca(sizeof(float) * numSamples));

		//memset(convolutedL, 0, sizeof(float)*numSamples);
		//memset(convolutedR, 0, sizeof(float)*numSamples);

		convolverL->process(l, convolutedL, numSamples);
		convolverR->process(r, convolutedR, numSamples);

		smoothedGainerDry.processBlock(channels, 2, numSamples);

		

#else
		const float *convolutedL = wdlPimpl->convolutionEngine.Get()[0];
		const float *convolutedR = wdlPimpl->convolutionEngine.Get()[1];
#endif

#if ENABLE_ALL_PEAK_METERS

		currentValues.inL = FloatVectorOperations::findMaximum(l, numSamples);
		currentValues.inR = FloatVectorOperations::findMaximum(r, numSamples);
		
#endif

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

#if !USE_FFT_CONVOLVER
			wdlPimpl->convolutionEngine.Advance(availableSamples);
#endif

			if (rampIndex >= rampingTime)
			{
				if (!processFlag)
				{
#if !USE_FFT_CONVOLVER
					wdlPimpl->convolutionEngine.Reset();
#endif
				}

				rampFlag = false;
			}
		}
		else
		{
			if (predelayMs != 0.0f)
			{
				float* outL = wetBuffer.getWritePointer(0);
				float* outR = wetBuffer.getWritePointer(1);

				float* inL = convolutedL;
				float* inR = convolutedR;

				for (int i = 0; i < availableSamples; i++)
				{
					*outL++ = leftPredelay.getDelayedValue(*inL);
					*outR++ = rightPredelay.getDelayedValue(*inR);
				}
			}
			else
			{
				FloatVectorOperations::copy(wetBuffer.getWritePointer(0), convolutedL, availableSamples);
				FloatVectorOperations::copy(wetBuffer.getWritePointer(1), convolutedR, availableSamples);
			}

			

			smoothedGainerWet.processBlock(wetBuffer.getArrayOfWritePointers(), 2, availableSamples);

#if ENABLE_ALL_PEAK_METERS
			currentValues.outL = wetGain * FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(0), availableSamples);
			currentValues.outR = wetGain * FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(1), availableSamples);
#endif

			FloatVectorOperations::add(l, wetBuffer.getReadPointer(0), availableSamples);
			FloatVectorOperations::add(r, wetBuffer.getReadPointer(1), availableSamples);



#if !USE_FFT_CONVOLVER
			wdlPimpl->convolutionEngine.Advance(availableSamples);
#endif
		}
	}

	isCurrentlyProcessing.store(false);

	CHECK_AND_LOG_BUFFER_DATA(this, DebugLogger::Location::ConvolutionRendering, l, true, numSamples);
	CHECK_AND_LOG_BUFFER_DATA(this, DebugLogger::Location::ConvolutionRendering, r, false, numSamples);
}

ProcessorEditorBody *ConvolutionEffect::createEditor(ProcessorEditor *parentEditor)
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
		ScopedLock sl(getImpulseLock());

		processFlag = shouldBeProcessed;

		rampFlag = true;
		rampUp = shouldBeProcessed;
		rampIndex = 0;
	}
}

void ConvolutionEffect::calcPredelay()
{
	leftPredelay.setDelayTimeSeconds(predelayMs / 1000.0);
	rightPredelay.setDelayTimeSeconds(predelayMs / 1000.0);
}

void GainSmoother::processBlock(float** data, int numChannels, int numSamples)
{
	if (numChannels == 1)
	{
		float *l = data[0];

		if (fastMode)
		{
			const float a = 0.99f;
			const float invA = 1.0f - a;

			while (--numSamples >= 0)
			{
				const float smoothedGain = lastValue * a + gain * invA;
				lastValue = smoothedGain;

				*l++ *= smoothedGain;
			}
		}
		else
		{
			while (--numSamples >= 0)
			{
				const float smoothedGain = smoother.smooth(gain);

				*l++ *= smoothedGain;
			}
		}


	}

	else if (numChannels == 2)
	{
		if (fastMode)
		{
			const float a = 0.99f;
			const float invA = 1.0f - a;

			float *l = data[0];
			float *r = data[1];

			while (--numSamples >= 0)
			{
				const float smoothedGain = lastValue * a + gain * invA;
				lastValue = smoothedGain;

				*l++ *= smoothedGain;
				*r++ *= smoothedGain;
			}
		}
		else
		{
			float *l = data[0];
			float *r = data[1];

			while (--numSamples >= 0)
			{
				const float smoothedGain = smoother.smooth(gain);

				*l++ *= smoothedGain;
				*r++ *= smoothedGain;
			}
		}

	}
}

} // namespace hise
