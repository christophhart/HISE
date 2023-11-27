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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace hise
{
using namespace juce;


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

bool MultithreadedConvolver::prepareImpulseResponse(const AudioSampleBuffer& originalBuffer, AudioSampleBuffer& buffer, bool* abortFlag, Range<int> range, double resampleRatio)
{
	AudioSampleBuffer copyBuffer(2, originalBuffer.getNumSamples());

	if (range.isEmpty())
		range = { 0, originalBuffer.getNumSamples() };

	if (originalBuffer.getNumSamples() == 0)
		return true;

	copyBuffer.copyFrom(0, 0, originalBuffer.getReadPointer(0), originalBuffer.getNumSamples(), 1.0f);
	copyBuffer.copyFrom(1, 0, originalBuffer.getReadPointer(originalBuffer.getNumChannels() >= 2 ? 1 : 0), originalBuffer.getNumSamples(), 1.0f);

	if (abortFlag != nullptr && *abortFlag)
		return false;

	const auto offset = range.getStart();
	const auto irLength = range.getLength();

	auto l = copyBuffer.getReadPointer(0, offset);
	auto r = copyBuffer.getReadPointer(1, offset);

	int resampledLength = roundToInt((double)irLength * resampleRatio);

	buffer.setSize(2, resampledLength);

	if (abortFlag != nullptr && *abortFlag)
		return false;

	if (resampleRatio != 1.0)
	{
		LagrangeInterpolator resampler;
		resampler.process(1.0 / resampleRatio, l, buffer.getWritePointer(0), resampledLength);
		resampler.reset();
		resampler.process(1.0 / resampleRatio, r, buffer.getWritePointer(1), resampledLength);
	}
	else
	{
		FloatVectorOperations::copy(buffer.getWritePointer(0), l, irLength);
		FloatVectorOperations::copy(buffer.getWritePointer(1), r, irLength);
	}

	return true;
}

double MultithreadedConvolver::getResampleFactor(double sampleRate, double impulseSampleRate)
{
	auto resampleFactor = sampleRate / impulseSampleRate;

	// not yet initialised, return a default until it will be recalled with the correct sample rate
	if (resampleFactor < 0.0)
	{
		return 1.0;
	}
	else
	{
		return resampleFactor;
	}
}

MultithreadedConvolver::Ptr ConvolutionEffectBase::createNewEngine(audiofft::ImplementationType fftType)
{
    MultithreadedConvolver::Ptr newConvolver = new MultithreadedConvolver(fftType);
	newConvolver->reset();
	newConvolver->setUseBackgroundThread(useBackgroundThread ? &backgroundThread : nullptr, true);

	return newConvolver;
}

ConvolutionEffectBase::ConvolutionEffectBase() :
	dryGain(0.0f),
	wetGain(1.0f),
	wetBuffer(2, 0),
    fadeBuffer(2, 0),
	latency(0),
	isReloading(false),
	rampFlag(false),
	rampIndex(0),
	processFlag(true),
	loadAfterProcessFlag(false),
	isCurrentlyProcessing(false)
{
	smoothedGainerWet.setParameter((int)GainSmoother::Parameters::FastMode, 1.0f);
	smoothedGainerDry.setParameter((int)GainSmoother::Parameters::FastMode, 1.0f);

	smoothedGainerWet.setParameter((int)GainSmoother::Parameters::Gain, 1.0f);
	smoothedGainerDry.setParameter((int)GainSmoother::Parameters::Gain, 0.0f);

	convolverL = createNewEngine(audiofft::ImplementationType::BestAvailable);
	convolverR = createNewEngine(audiofft::ImplementationType::BestAvailable);
}

ConvolutionEffectBase::~ConvolutionEffectBase()
{
	SimpleReadWriteLock::ScopedMultiWriteLock sl(swapLock);

	convolverL = nullptr;
	convolverR = nullptr;
    fadeOutConvolverL = nullptr;
    fadeOutConvolverR = nullptr;
}

void ConvolutionEffectBase::setImpulse(NotificationType sync)
{
	if (!prepareCalledOnce)
		sync = dontSendNotification;

	switch (sync)
	{
	case dontSendNotification:
		break;
	case sendNotification:
	case sendNotificationAsync:
		if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
		{
			triggerAsyncUpdate();
			break;
		}
	case sendNotificationSync:
		cancelPendingUpdate();
		handleAsyncUpdate();
		break;
	default:
		jassertfalse;
	}
}

void ConvolutionEffectBase::enableProcessing(bool shouldBeProcessed)
{
	if (processFlag != shouldBeProcessed)
	{
		processFlag = shouldBeProcessed;

		if (processFlag)
			smoothInputBuffer = true;

		rampFlag = true;
		rampUp = shouldBeProcessed;
		rampIndex = 0;
	}
}

void ConvolutionEffectBase::calcPredelay()
{
	leftPredelay.setDelayTimeSeconds(predelayMs / 1000.0);
	rightPredelay.setDelayTimeSeconds(predelayMs / 1000.0);
}

void ConvolutionEffectBase::applyExponentialFadeout(AudioSampleBuffer& buffer, int numSamples, float targetValue)
{
	if (targetValue == 1.0f)
		return;

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

void ConvolutionEffectBase::applyHighFrequencyDamping(AudioSampleBuffer& buffer, int numSamples, double cutoffFrequency, double sampleRate)
{
	if (cutoffFrequency >= 20000.0)
		return;

	const double base = cutoffFrequency / 20000.0;
	const double invBase = 1.0 - base;
	const double factor = -1.0 * (double)numSamples / 8.0;

	SimpleOnePole lp1;
	lp1.setType(SimpleOnePoleSubType::FilterType::LP);
	lp1.setFrequency(20000.0);
	lp1.setSampleRate(sampleRate > 0.0 ? sampleRate : 44100.0);
	lp1.setNumChannels(2);

	SimpleOnePole lp2;
	lp2.setType(SimpleOnePoleSubType::FilterType::LP);
	lp2.setFrequency(20000.0);
	lp2.setSampleRate(sampleRate > 0.0 ? sampleRate : 44100.0);
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

void ConvolutionEffectBase::calcCutoff()
{
	setImpulse(sendNotificationAsync);
}


void ConvolutionEffectBase::resetBase()
{
	smoothedGainerDry.reset();
	smoothedGainerWet.reset();
	wetBuffer.clear();
    fadeBuffer.clear();
	smoothInputBuffer = false;
	rampFlag = false;

	if (predelayMs > 0)
	{
		leftPredelay.clear();
		rightPredelay.clear();
	}

	if (convolverL != nullptr)
		convolverL->cleanPipeline();

	if (convolverR != nullptr)
		convolverR->cleanPipeline();
}

void ConvolutionEffectBase::prepareBase(double sampleRate, int samplesPerBlock)
{
	if (wetBuffer.getNumSamples() < samplesPerBlock)
	{
        fadeBuffer.setSize(fadeBuffer.getNumChannels(), samplesPerBlock);
        fadeBuffer.clear();

		wetBuffer.setSize(wetBuffer.getNumChannels(), samplesPerBlock);
		wetBuffer.clear();
	}

	lastBlockSize = samplesPerBlock;

    
    
	if (sampleRate != lastSampleRate)
	{
		lastSampleRate = sampleRate;

        fadeDelta = 1.0f / (0.02f * (float)sampleRate);
        
		smoothedGainerWet.prepareToPlay(sampleRate, samplesPerBlock);
		smoothedGainerDry.prepareToPlay(sampleRate, samplesPerBlock);

		leftPredelay.prepareToPlay(sampleRate);
		rightPredelay.prepareToPlay(sampleRate);
	}

	prepareCalledOnce = sampleRate > 0.0;
	setImpulse(sendNotificationSync);
}

void ConvolutionEffectBase::processBase(ProcessDataDyn& d)
{
    TRACE_DSP();
    
	if (auto sp = SimpleReadWriteLock::ScopedTryReadLock(swapLock))
	{
		auto channels = d.getRawChannelPointers();
		int numChannels = d.getNumChannels();
		auto numSamples = d.getNumSamples();
		auto l = channels[0];
		auto r = numChannels > 1 ? channels[1] : nullptr;

		FloatSanitizers::sanitizeArray(l, numSamples);

		if (numChannels > 1)
			FloatSanitizers::sanitizeArray(r, numSamples);

		isCurrentlyProcessing.store(true);

		if (isReloading || (!processFlag && !rampFlag))
		{
			smoothedGainerDry.processBlock(channels, numChannels, numSamples);

			isCurrentlyProcessing.store(false);
			return;
		}

		const int availableSamples = numSamples;

		if (availableSamples > 0)
		{
			float* convolutedL = wetBuffer.getWritePointer(0);
			float* convolutedR = numChannels > 1 ? wetBuffer.getWritePointer(1) : nullptr;

			if (convolutedL == nullptr)
			{
				jassertfalse;
				return;
			}

			if (smoothInputBuffer)
			{
				auto smoothed_input_l = (float*)alloca(sizeof(float)*numSamples);
				auto smoothed_input_r = numChannels > 1 ? (float*)alloca(sizeof(float)*numSamples) : nullptr;

				float s_gain = 0.0f;
				float s_step = 1.0f / (float)numSamples;

				for (int i = 0; i < numSamples; i++)
				{
					smoothed_input_l[i] = s_gain * l[i];

					if (numChannels > 1)
						smoothed_input_r[i] = s_gain * r[i];

					s_gain += s_step;
				}

				wetBuffer.clear();
				convolverL->cleanPipeline();

				if (numChannels > 1)
					convolverR->cleanPipeline();

				if (convolverL != nullptr)
					convolverL->process(smoothed_input_l, convolutedL, numSamples);

				if (convolverR != nullptr && numChannels > 1)
					convolverR->process(smoothed_input_r, convolutedR, numSamples);

				smoothInputBuffer = false;
			}
			else if (fadeOutConvolverL != nullptr)
            {
                auto fadeL = fadeBuffer.getWritePointer(0);
                auto fadeR = numChannels > 1 ? fadeBuffer.getWritePointer(1) : nullptr;
                
                //fadeBuffer.clear();
                //wetBuffer.clear();
                

                auto dryCopyL = (float*)alloca(sizeof(float)*numSamples);
                auto dryCopyR = (float*)alloca(sizeof(float)*numSamples);
                
                FloatVectorOperations::copy(dryCopyL, l, numSamples);
                FloatVectorOperations::copy(dryCopyR, r == nullptr ? l : r, numSamples);
                
                auto dryFadeValue = fadeValue;
                
                for(int i = 0; i < numSamples; i++)
                {
                    float g = jlimit(0.0f, 1.0f, dryFadeValue);
                    
                    dryCopyL[i] *= g*g;
                    dryCopyR[i] *= g*g;
                    
                    dryFadeValue += fadeDelta;
                }
                
                if (convolverL != nullptr)
                    convolverL->process(dryCopyL, convolutedL, numSamples);

                if (convolverR != nullptr && numChannels > 1)
                    convolverR->process(dryCopyR, convolutedR, numSamples);
                
                if (fadeOutConvolverL != nullptr)
                    fadeOutConvolverL->process(l, fadeL, numSamples);
                if (fadeOutConvolverR != nullptr)
                    fadeOutConvolverR->process(r, fadeR, numSamples);
                
                for(int i = 0; i < numSamples; i++)
                {
                    float g = jlimit(0.0f, 1.0f, fadeValue);
                    g = 1.0f - g;
                    g *= g;
                    convolutedL[i] += fadeL[i] * g;
                    convolutedR[i] += fadeR[i] * g;
                    
                    fadeValue += fadeDelta;
                }
                
                if(fadeValue >= 1.0f)
                {
                    backgroundThread.addConvolverToBeDeleted(fadeOutConvolverL);
                    backgroundThread.addConvolverToBeDeleted(fadeOutConvolverR);
                    
                    fadeOutConvolverL = nullptr;
                    fadeOutConvolverR = nullptr;
                }
            }
            else
			{
				if (convolverL != nullptr)
					convolverL->process(l, convolutedL, numSamples);

				if (convolverR != nullptr && numChannels > 1)
					convolverR->process(r, convolutedR, numSamples);
			}

			smoothedGainerDry.processBlock(channels, numChannels, numSamples);

			if (rampFlag)
			{
				const int rampingTime = (CONVOLUTION_RAMPING_TIME_MS * (int)lastSampleRate) / 1000;

				for (int i = 0; i < availableSamples; i++)
				{
					float rampValue = jlimit<float>(0.0f, 1.0f, (float)rampIndex / (float)rampingTime);

					rampValue *= rampValue; // Cheap mans logarithm

					const float gainValue = 0.5f * wetGain * (float)(rampUp ? rampValue : (1.0f - rampValue));
					l[i] += gainValue * convolutedL[i];

					if (numChannels > 1)
						r[i] += gainValue * convolutedR[i];

					rampIndex++;
				}

				if (rampIndex >= rampingTime)
					rampFlag = false;
			}
			else
			{
				if (predelayMs != 0.0f)
				{
					float* outL = wetBuffer.getWritePointer(0);
					float* outR = numChannels > 1 ? wetBuffer.getWritePointer(1) : nullptr;

					const float* inL = convolutedL;
					const float* inR = convolutedR;

					for (int i = 0; i < availableSamples; i++)
					{
						*outL++ = leftPredelay.getDelayedValue(*inL++);

						if (numChannels > 1)
							*outR++ = rightPredelay.getDelayedValue(*inR++);
					}
				}
				else
				{
					FloatVectorOperations::copy(wetBuffer.getWritePointer(0), convolutedL, availableSamples);

					if (numChannels > 1)
						FloatVectorOperations::copy(wetBuffer.getWritePointer(1), convolutedR, availableSamples);
				}

				smoothedGainerWet.processBlock(wetBuffer.getArrayOfWritePointers(), numChannels, availableSamples);

				FloatVectorOperations::addWithMultiply(l, wetBuffer.getReadPointer(0), 0.5f, availableSamples);

				if (numChannels > 1)
					FloatVectorOperations::addWithMultiply(r, wetBuffer.getReadPointer(1), 0.5f, availableSamples);
			}
		}

		isCurrentlyProcessing.store(false);
	}
}

bool ConvolutionEffectBase::reloadInternal()
{
	if (convolverL == nullptr)
		return true;

	if (getImpulseBufferBase().isEmpty() || 
		getImpulseBufferBase().getBuffer().getNumChannels() == 0|| 
		getImpulseBufferBase().getBuffer().getNumSamples() == 0 )
	{
		while(backgroundThread.isBusy())
            Thread::getCurrentThread()->wait(10);

		SimpleReadWriteLock::ScopedMultiWriteLock sl(swapLock);

		convolverL->reset();
		convolverR->reset();
		return true;
	}

	AudioSampleBuffer scratchBuffer;
	AudioSampleBuffer copyOfOriginal;

	{
		SimpleReadWriteLock::ScopedReadLock sl(getImpulseBufferBase().getDataLock());
		copyOfOriginal.makeCopyOf(getImpulseBufferBase().getBuffer());
	}

	auto resampleRatio = getResampleFactor();

	{
		bool unused = false;

		if (!MultithreadedConvolver::prepareImpulseResponse(copyOfOriginal, scratchBuffer, &unused, { 0, copyOfOriginal.getNumSamples() }, resampleRatio))
			return false;
	}

	int headSize = lastBlockSize;
	auto sampleRate = lastSampleRate;

	auto resampledLength = scratchBuffer.getNumSamples();

	if (damping != 1.0f)
		applyExponentialFadeout(scratchBuffer, resampledLength, damping);

	if (cutoffFrequency != 20000.0)
		applyHighFrequencyDamping(scratchBuffer, resampledLength, cutoffFrequency, sampleRate);

	headSize = nextPowerOfTwo(headSize);
	const auto fullTailLength = jmax(headSize, nextPowerOfTwo(resampledLength - headSize));

	MultithreadedConvolver::Ptr s1, s2;

    
    
	for (int c = 0; c < scratchBuffer.getNumChannels(); c++)
	{
		auto r = scratchBuffer.getWritePointer(c);
		int numSamples = scratchBuffer.getNumSamples();
		FloatSanitizers::sanitizeArray(r, numSamples);

		for (int i = 0; i < numSamples; i++)
		{
			JUCE_UNDENORMALISE(r[i]);
		}
	}

	s1 = createNewEngine(currentType);
	s2 = createNewEngine(currentType);
	s1->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(0), resampledLength);
	s2->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(1), resampledLength);

    s1->cleanPipeline();
    s2->cleanPipeline();

    scratchBuffer.clear();
    
    s1->process(scratchBuffer.getReadPointer(0), scratchBuffer.getWritePointer(1), jmin(scratchBuffer.getNumSamples(), 2048));
    
    scratchBuffer.clear();
    
    s2->process(scratchBuffer.getReadPointer(0), scratchBuffer.getWritePointer(1), jmin(scratchBuffer.getNumSamples(), 2048));
    
    
	{
		while (backgroundThread.isBusy())
		{
			auto currentThread = Thread::getCurrentThread();
			
			if(currentThread != nullptr)
				currentThread->wait(10);
		}

		SimpleReadWriteLock::ScopedMultiWriteLock sl(swapLock);
        
        std::swap(fadeOutConvolverL, convolverL);
		std::swap(fadeOutConvolverR, convolverR);
        
        fadeValue = 0.0f;
        
        if(convolverL != nullptr)
        {
            backgroundThread.addConvolverToBeDeleted(convolverL);
            backgroundThread.addConvolverToBeDeleted(convolverR);
        }
        
        convolverL = s1;
        convolverR = s2;
	}

	return true;
}


}
