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

ConvolutionEffect::ConvolutionEffect(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
AudioSampleProcessor(mc)
{
	getBuffer().addListener(this);

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
	parameterNames.add("FFTType");

	
}

ConvolutionEffect::~ConvolutionEffect()
{
	getBuffer().removeListener(this);

	
}

MultithreadedConvolver* ConvolutionEffectBase::createNewEngine(audiofft::ImplementationType fftType)
{
	auto newConvolver = new MultithreadedConvolver(fftType);
	newConvolver->reset();
	newConvolver->setUseBackgroundThread(useBackgroundThread, true);

	return newConvolver;
}

ConvolutionEffectBase::ConvolutionEffectBase():
	dryGain(0.0f),
	wetGain(1.0f),
	wetBuffer(2, 0),
	latency(0),
	isReloading(false),
	rampFlag(false),
	rampIndex(0),
	processFlag(true),
	loadAfterProcessFlag(false),
	isCurrentlyProcessing(false)
{
	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::FastMode, 1.0f);

	smoothedGainerWet.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 1.0f);
	smoothedGainerDry.setParameter((int)ScriptingDsp::SmoothedGainer::Parameters::Gain, 0.0f);

	convolverL = createNewEngine(audiofft::ImplementationType::BestAvailable);
	convolverR = createNewEngine(audiofft::ImplementationType::BestAvailable);
}

ConvolutionEffectBase::~ConvolutionEffectBase()
{
	SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
	
	convolverL = nullptr;
	convolverR = nullptr;
}

void ConvolutionEffectBase::setImpulse(bool sync)
{
	if (sync)
	{
		jassert(!getImpulseBufferBase().getDataLock().writeAccessIsLocked());
		handleAsyncUpdate();
	}
	else
		triggerAsyncUpdate();
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
	const double base = cutoffFrequency / 20000.0;
	const double invBase = 1.0 - base;
	const double factor = -1.0 * (double)numSamples / 8.0;

	SimpleOnePole lp1;
	lp1.setType(SimpleOnePoleSubType::FilterType::LP);
	lp1.setFrequency(20000.0);
	lp1.setSampleRate(sampleRate >= 0.0 ? sampleRate : 44100.0);
	lp1.setNumChannels(2);

	SimpleOnePole lp2;
	lp2.setType(SimpleOnePoleSubType::FilterType::LP);
	lp2.setFrequency(20000.0);
	lp2.setSampleRate(sampleRate >= 0.0 ? sampleRate : 44100.0);
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
	setImpulse(false);
}


void ConvolutionEffectBase::prepareBase(double sampleRate, int samplesPerBlock)
{
	ProcessorHelpers::increaseBufferIfNeeded(wetBuffer, samplesPerBlock);

	lastBlockSize = samplesPerBlock;

	if (sampleRate != lastSampleRate)
	{
		lastSampleRate = sampleRate;

		smoothedGainerWet.prepareToPlay(sampleRate, samplesPerBlock);
		smoothedGainerDry.prepareToPlay(sampleRate, samplesPerBlock);

		leftPredelay.prepareToPlay(sampleRate);
		rightPredelay.prepareToPlay(sampleRate);

		setImpulse(false);
	}
}

void ConvolutionEffectBase::processBase(ProcessDataDyn& d)
{
	if (auto sp = SimpleReadWriteLock::ScopedTryReadLock(swapLock))
	{
		auto channels = d.getRawChannelPointers();
		auto numSamples = d.getNumSamples();
		auto l = channels[0];
		auto r = channels[1];

		isCurrentlyProcessing.store(true);

		if (isReloading || (!processFlag && !rampFlag))
		{
			smoothedGainerDry.processBlock(channels, 2, numSamples);

			isCurrentlyProcessing.store(false);
			return;
		}

		const int availableSamples = numSamples;

		if (availableSamples > 0)
		{
			float* convolutedL = wetBuffer.getWritePointer(0);
			float* convolutedR = wetBuffer.getWritePointer(1);

			if (smoothInputBuffer)
			{
				auto smoothed_input_l = (float*)alloca(sizeof(float)*numSamples);
				auto smoothed_input_r = (float*)alloca(sizeof(float)*numSamples);

				float s_gain = 0.0f;
				float s_step = 1.0f / (float)numSamples;

				for (int i = 0; i < numSamples; i++)
				{
					smoothed_input_l[i] = s_gain * l[i];
					smoothed_input_r[i] = s_gain * r[i];

					s_gain += s_step;
				}

				wetBuffer.clear();
				convolverL->cleanPipeline();
				convolverR->cleanPipeline();

				if (convolverL != nullptr)
					convolverL->process(smoothed_input_l, convolutedL, numSamples);

				if (convolverR != nullptr)
					convolverR->process(smoothed_input_r, convolutedR, numSamples);

				smoothInputBuffer = false;
			}
			else
			{
				if (convolverL != nullptr)
					convolverL->process(l, convolutedL, numSamples);

				if (convolverR != nullptr)
					convolverR->process(r, convolutedR, numSamples);
			}

			smoothedGainerDry.processBlock(channels, 2, numSamples);

			if (rampFlag)
			{
				const int rampingTime = (CONVOLUTION_RAMPING_TIME_MS * (int)lastSampleRate) / 1000;

				for (int i = 0; i < availableSamples; i++)
				{
					float rampValue = jlimit<float>(0.0f, 1.0f, (float)rampIndex / (float)rampingTime);

					rampValue *= rampValue; // Cheap mans logarithm

					const float gainValue = 0.5f * wetGain * (float)(rampUp ? rampValue : (1.0f - rampValue));
					l[i] += gainValue * convolutedL[i];
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

				FloatVectorOperations::addWithMultiply(l, wetBuffer.getReadPointer(0), 0.5f, availableSamples);
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

	if (getImpulseBufferBase().isEmpty())
	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
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
	const auto fullTailLength = nextPowerOfTwo(resampledLength - headSize);

	ScopedPointer<MultithreadedConvolver> s1, s2;

	s1 = createNewEngine(currentType);
	s2 = createNewEngine(currentType);
	s1->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(0), resampledLength);
	s2->init(headSize, jmin<int>(8192, fullTailLength), scratchBuffer.getReadPointer(1), resampledLength);

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		std::swap(s1, convolverL);
		std::swap(s2, convolverR);
	}

	return true;
}


float ConvolutionEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DryGain:		return Decibels::gainToDecibels(dryGain);
	case WetGain:		return Decibels::gainToDecibels(wetGain);
	case Latency:		return (float)latency;
	case ImpulseLength:	return 1.0f;
	case ProcessInput:	return processingEnabled ? 1.0f : 0.0f;
	case UseBackgroundThread:	return convolverL->isUsingBackgroundThread() ? 1.0f : 0.0f;
	case Predelay:		return predelayMs;
	case HiCut:			return (float)cutoffFrequency;
	case Damping:		return Decibels::gainToDecibels(damping);
	case FFTType:		return (float)(int)currentType;
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
		setImpulse(false);
		break;
	case ImpulseLength:	setImpulse(false);
		break;
	case ProcessInput:	processingEnabled = newValue >= 0.5f;
						enableProcessing(processingEnabled); 
						break;
	case UseBackgroundThread:	
	{
		useBackgroundThread = newValue > 0.5f;

		{
			SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
			convolverL->setUseBackgroundThread(useBackgroundThread && !nonRealtime);
			convolverR->setUseBackgroundThread(useBackgroundThread && !nonRealtime);
		}
		
		break;
	}
	case Predelay:		predelayMs = newValue;
						calcPredelay();
						break;
	case HiCut:			cutoffFrequency = (double)newValue; 
						calcCutoff();
						
						break;
	case Damping:		damping = Decibels::decibelsToGain(newValue); 
						setImpulse(false);
						break;
	case FFTType:		
	{
		auto newType = (audiofft::ImplementationType)(int)newValue;

		if (newType != audiofft::ImplementationType::numImplementationTypes)
		{
			currentType = newType;
			setImpulse(false);
		}
		
		break;
	}
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
	case FFTType:		return (float)(int)audiofft::ImplementationType::BestAvailable;
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
	loadAttributeWithDefault(FFTType);

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
	saveAttribute(FFTType, "FFTType");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

void ConvolutionEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	prepareBase(sampleRate, samplesPerBlock);
}

void ConvolutionEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	ADD_GLITCH_DETECTOR(this, DebugLogger::Location::ConvolutionRendering);

	if (startSample != 0)
	{
		debugError(this, "Buffer start not 0!");
	}


	auto numChannels = buffer.getNumChannels();
	auto channels = (float**)alloca(numChannels * sizeof(float*));
	
	for (int i = 0; i < numChannels; i++)
		channels[i] = buffer.getWritePointer(i, startSample);

	ProcessDataDyn d(channels, numSamples, numChannels);

#if ENABLE_ALL_PEAK_METERS
	currentValues.inL = FloatVectorOperations::findMaximum(channels[0], numSamples);
	currentValues.inR = FloatVectorOperations::findMaximum(channels[1], numSamples);
#endif

	processBase(d);

#if ENABLE_ALL_PEAK_METERS
	currentValues.outL = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(0, startSample), numSamples);
	currentValues.outR = FloatVectorOperations::findMaximum(wetBuffer.getReadPointer(1, startSample), numSamples);
#endif
}

void ConvolutionEffect::voicesKilled()
{
	{
		SimpleReadWriteLock::ScopedReadLock sl(swapLock);
		convolverL->cleanPipeline();
		convolverR->cleanPipeline();
	}

	leftPredelay.clear();
	rightPredelay.clear();
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

#if 0
void ConvolutionEffect::LoadingThread::run()
{
	while (!threadShouldExit())
	{
		bool doSomething = (pending && (parent.rampFlag == false)) ||
						   shouldRestart;

		if (doSomething)
		{
			ScopedValueSetter<bool> svs(parent.isReloading, true);

			shouldRestart = false;
			
			if (reloadInternal())
			{
				pending = false;
			}
		}

		wait(100);
	}
}
#endif

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

} // namespace hise
