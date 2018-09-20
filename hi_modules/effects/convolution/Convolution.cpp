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
loadingThread(*this),
#if USE_FFT_CONVOLVER
convolverL(new MultithreadedConvolver()),
convolverR(new MultithreadedConvolver())
#else
wdlPimpl(new WdlPimpl())
#endif
{
	finaliseModChains();

	parameterNames.add("DryGain");
	parameterNames.add("WetGain");
	parameterNames.add("Latency");
	parameterNames.add("ImpulseLength");
	parameterNames.add("ProcessInput");
	parameterNames.add("UseBackgroundThread");
	parameterNames.add("Predelay");
	parameterNames.add("HiCut");
	parameterNames.add("Damping");

	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);

	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 0.0f);

#if USE_FFT_CONVOLVER
	convolverL->reset();
	convolverR->reset();
	convolverL->setUseBackgroundThread(false);
	convolverR->setUseBackgroundThread(false);
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
	loadingThread.reloadImpulse();
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
#if USE_FFT_CONVOLVER
	case UseBackgroundThread:	return convolverL->isUsingBackgroundThread() ? 1.0f : 0.0f;
#endif
	case Predelay:		return predelayMs;
	case HiCut:			return (float)cutoffFrequency;
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
#if USE_FFT_CONVOLVER
	case UseBackgroundThread:	convolverL->setUseBackgroundThread(newValue > 0.5f);
								convolverR->setUseBackgroundThread(newValue > 0.5f);
								break;
#endif
	case Predelay:		predelayMs = newValue;
						calcPredelay();
						break;
	case HiCut:			cutoffFrequency = (double)newValue; 
						calcCutoff();
						
						break;
	case Damping:		damping = Decibels::decibelsToGain(newValue); 
						setImpulse();
						break;
	default:			jassertfalse; return;
	}
}

float ConvolutionEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DryGain:		return -100.0f;
	case WetGain:		return 0.0f;
	case Latency:		return 0.0f;
	case ImpulseLength:	return 1.0f;
	case ProcessInput:	return true;
	case UseBackgroundThread:	return false;
	case Predelay:		return 0.0f;
	case HiCut:			return 20000.0f;
	case Damping:		return 0.0f;
	default:			jassertfalse; return 1.0f;
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
	loadAttributeWithDefault(Predelay);
	loadAttributeWithDefault(HiCut);
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
	saveAttribute(HiCut, "HiCut");
	saveAttribute(Damping, "Damping");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

void ConvolutionEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

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
		

		setImpulse();

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

		if (convolverL != nullptr)
			convolverL->process(l, convolutedL, numSamples);

		if(convolverR != nullptr)
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
#if USE_FFT_CONVOLVER

					convolverL->cleanPipeline();
					convolverR->cleanPipeline();

					

#else

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

				const float* inL = convolutedL;
				const float* inR = convolutedR;

				for (int i = 0; i < availableSamples; i++)
				{
					*outL++ = leftPredelay.getDelayedValue(*inL++);
					*outR++ = rightPredelay.getDelayedValue(*inR++);
				}
			}
			else
			{
				FloatVectorOperations::copy(wetBuffer.getWritePointer(0), convolutedL, availableSamples);
				FloatVectorOperations::copy(wetBuffer.getWritePointer(1), convolutedR, availableSamples);
			}

			

			smoothedGainerWet.processBlock(wetBuffer.getArrayOfWritePointers(), 2, availableSamples);

#if ENABLE_ALL_PEAK_METERS
			currentValues.outL = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(0), availableSamples);
			currentValues.outR = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(1), availableSamples);
#endif

			FloatVectorOperations::addWithMultiply(l, wetBuffer.getReadPointer(0), 0.5f, availableSamples);
			FloatVectorOperations::addWithMultiply(r, wetBuffer.getReadPointer(1), 0.5f, availableSamples);



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

void ConvolutionEffect::applyExponentialFadeout(AudioSampleBuffer& buffer, int numSamples, float targetValue)
{
	float* l = buffer.getWritePointer(0);
	float* r = buffer.getWritePointer(1);

	const float base = targetValue;
	const float invBase = 1.0f - targetValue;
	const float factor = -1.0f * (float)numSamples / 4.0f;

	for (int i = 0; i < numSamples; i++)
	{
		const float multiplier = base + invBase * expf((float)i / factor);

		*l++ *= multiplier;
		*r++ *= multiplier;
	}
}

void ConvolutionEffect::applyHighFrequencyDamping(AudioSampleBuffer& buffer, int numSamples, double cutoffFrequency, double sampleRate)
{
	const double base = cutoffFrequency / 20000.0;
	const double invBase = 1.0 - base;
	const double factor = -1.0 * (double)numSamples / 8.0;

	SimpleOnePole lp1;
	lp1.setFrequency(20000.0);
	lp1.setSampleRate(sampleRate);
	lp1.setNumChannels(2);

	SimpleOnePole lp2;
	lp2.setFrequency(20000.0);
	lp2.setSampleRate(sampleRate);
	lp2.setNumChannels(2);
	

	for (int i = 0; i < numSamples; i += 64)
	{
		const double multiplier = base + invBase * exp((double)i / factor);

		auto numToProcess = jmin<int>(64, numSamples - i);

		FilterHelpers::RenderData r(buffer, i, numToProcess);

		r.freqModValue = multiplier;

		lp1.render(r);
		lp2.render(r);
	}
}

void ConvolutionEffect::calcCutoff()
{
	setImpulse();
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

void ConvolutionEffect::LoadingThread::run()
{
	while (!threadShouldExit())
	{
		while(shouldReload)
		{
			ScopedValueSetter<bool> ss(isReloading, true);
			shouldRestart = false;
			reloadInternal();
		}

		wait(500);
	}
	
}

void ConvolutionEffect::LoadingThread::reloadInternal()
{
	if (parent.getSampleBuffer() == nullptr || parent.getSampleBuffer()->getNumChannels() == 0)
	{
		ScopedLock sl(parent.getImpulseLock());

		parent.convolverL->reset();
		parent.convolverR->reset();
		shouldReload = false;
		return;
	}

	ScopedValueSetter<bool> s(parent.isReloading, true);

	auto pBuffer = *parent.getSampleBuffer();

	AudioSampleBuffer copyBuffer(2, parent.getSampleBuffer()->getNumSamples());

	copyBuffer.copyFrom(0, 0, pBuffer.getReadPointer(0), pBuffer.getNumSamples(), 1.0f);
	copyBuffer.copyFrom(1, 0, pBuffer.getReadPointer(pBuffer.getNumChannels() >= 2 ? 1 : 0), pBuffer.getNumSamples(), 1.0f);

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}
	
#if USE_FFT_CONVOLVER

	const auto offset = parent.getRange().getStart();
	const auto irLength = parent.getRange().getLength();

	if (irLength > 44100 * 20)
		jassertfalse;

	auto l = copyBuffer.getReadPointer(0, offset);
	auto r = copyBuffer.getReadPointer(1, offset);

	auto resampleRatio = parent.getResampleFactor();

	int resampledLength = roundDoubleToInt((double)irLength * resampleRatio);

	AudioSampleBuffer scratchBuffer(2, resampledLength);

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}
		

	if (resampleRatio != 1.0)
	{
		LagrangeInterpolator resampler;
		resampler.process(1.0 / resampleRatio, l, scratchBuffer.getWritePointer(0), resampledLength);
		resampler.reset();
		resampler.process(1.0 / resampleRatio, r, scratchBuffer.getWritePointer(1), resampledLength);
	}
	else
	{
		FloatVectorOperations::copy(scratchBuffer.getWritePointer(0), l, irLength);
		FloatVectorOperations::copy(scratchBuffer.getWritePointer(1), r, irLength);
	}

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}

	if (parent.damping != 1.0f)
		applyExponentialFadeout(scratchBuffer, resampledLength, parent.damping);

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}

	if (parent.cutoffFrequency != 20000.0)
		applyHighFrequencyDamping(scratchBuffer, resampledLength, parent.cutoffFrequency, parent.getSampleRate());

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}


	const auto headSize = nextPowerOfTwo(parent.getLargestBlockSize());
	const auto fullTailLength = nextPowerOfTwo(resampledLength - headSize);

	ScopedLock sl(parent.getImpulseLock());

	parent.convolverL->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(0), resampledLength);

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}

	parent.convolverR->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(1), resampledLength);

	if (shouldRestart)
	{
		shouldReload = true;
		return;
	}

	shouldReload = false;

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

} // namespace hise
