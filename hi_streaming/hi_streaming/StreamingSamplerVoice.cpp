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

// =============================================================================================================================================== SampleLoader methods


    
SampleLoader::SampleLoader(SampleThreadPool *pool_) :
	SampleThreadPoolJob("SampleLoader"),
	backgroundPool(pool_),
	writeBufferIsBeingFilled(false),
	sound(0),
	readIndex(0),
	readIndexDouble(0.0),
	idealBufferSize(0),
	minimumBufferSizeForSamplesPerBlock(0),
	positionInSampleFile(0),
	isReadingFromPreloadBuffer(true),
	sampleStartModValue(0),
	readBuffer(nullptr),
	writeBuffer(nullptr),
	diskUsage(0.0),
	lastCallToRequestData(0.0),
	b1(DEFAULT_BUFFER_TYPE_IS_FLOAT, 2, 0),
	b2(DEFAULT_BUFFER_TYPE_IS_FLOAT, 2, 0)
{
	unmapper.setLoader(this);

	setBufferSize(BUFFER_SIZE_FOR_STREAM_BUFFERS);
}

SampleLoader::~SampleLoader()
{
	b1.setSize(2, 0);
	b2.setSize(2, 0);
}

/** Sets the buffer size in samples. */
void SampleLoader::setBufferSize(int newBufferSize)
{
	ScopedLock sl(getLock());

#if HISE_IOS 

	// because of memory
	idealBufferSize = 4096;
#else
	idealBufferSize = newBufferSize;
#endif

	refreshBufferSizes();
}

bool SampleLoader::assertBufferSize(int minimumBufferSize)
{
	minimumBufferSizeForSamplesPerBlock = minimumBufferSize;

	refreshBufferSizes();

	return true;
}

void SampleLoader::startNote(StreamingSamplerSound const *s, int startTime)
{
	diskUsage = 0.0;

	sound = s;

#if HISE_SAMPLER_ALLOW_RELEASE_START
	releasePlayState = StreamingSamplerSound::ReleasePlayState::Inactive;
	s->resetReleaseData();
#endif

	sampleStartModValue = (int)startTime;

	auto localReadBuffer = &s->getPreloadBuffer();
	auto localWriteBuffer = &b1;

	// the read pointer will be pointing directly to the preload buffer of the sample sound
	readBuffer = localReadBuffer;
	writeBuffer = localWriteBuffer;

	lastSwapPosition = 0.0;

	readIndex = startTime;
	readIndexDouble = (double)startTime;

	isReadingFromPreloadBuffer = true;

	// Set the sampleposition to (1 * bufferSize) because the first buffer is the preload buffer
	positionInSampleFile = (int)localReadBuffer->getNumSamples();

	voiceCounterWasIncreased = false;

	entireSampleIsLoaded = s->isEntireSampleLoaded();

	if (!entireSampleIsLoaded)
	{
		// The other buffer will be filled on the next free thread pool slot
		requestNewData();
	}
};

void SampleLoader::reset()
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	releasePlayState = StreamingSamplerSound::ReleasePlayState::Inactive;
#endif

	const StreamingSamplerSound *currentSound = sound.get();

	if (currentSound != nullptr)
	{
		const bool isMonolith = currentSound->isMonolithic();

		if (isMonolith)
		{
			currentSound->decreaseVoiceCount();
			clearLoader();

		}
		else
		{
			// If the samples are not monolithic, we'll need to close the
			// file handles on the background thread.

			unmapper.setSoundToUnmap(currentSound);

			if (nonRealtime)
			{
				unmapper.runJob();
			}
			else
				backgroundPool->addJob(&unmapper, false);

			clearLoader();
		}
	}
	else
		clearLoader();
}

NotificationType SampleLoader::waitForTimestretchSeek(StreamingSamplerVoice* v)
{
	voiceToSeekTimestretchStart = v;

	if(v != nullptr)
	{
		requestNewData();
		auto isSynchronous = isNonRealtime();
		return isSynchronous ? dontSendNotification : sendNotificationAsync;
	}

	return dontSendNotification;
}

void SampleLoader::clearLoader()
{
	sound = nullptr;
	diskUsage = 0.0f;
	cancelled = true;
	resetJob();
}

double SampleLoader::getDiskUsage() noexcept
{
	const double returnValue = (double)diskUsage.get();
	diskUsage = 0.0f;
	return returnValue;
}

void SampleLoader::setStreamingBufferDataType(bool shouldBeFloat)
{
	if (b1.isFloatingPoint() != shouldBeFloat)
	{
		ScopedLock sl(getLock());

		b1 = hlac::HiseSampleBuffer(shouldBeFloat, 2, 0);
		b2 = hlac::HiseSampleBuffer(shouldBeFloat, 2, 0);

		refreshBufferSizes();
	}
}


StereoChannelData SampleLoader::fillVoiceBuffer(hlac::HiseSampleBuffer &voiceBuffer, double numSamples) const
{
	auto localReadBuffer = readBuffer.get();
	auto localWriteBuffer = writeBuffer.get();

	const int numSamplesInBuffer = localReadBuffer->getNumSamples();
	const int maxSampleIndexForFillOperation = (int)(readIndexDouble + numSamples) + 1; // Round up the samples

	if (maxSampleIndexForFillOperation >= numSamplesInBuffer) // Check because of preloadbuffer style
	{
		if (entireSampleIsLoaded)
		{
			const int index = (int)readIndexDouble;
			StereoChannelData returnData;
            
            if(isPositiveAndBelow(maxSampleIndexForFillOperation, localReadBuffer->getNumSamples()))
            {
                returnData.b = localReadBuffer;
                returnData.offsetInBuffer = index;
                return returnData;
            }
		}

		const int indexBeforeWrap = jmax<int>(0, (int)(readIndexDouble));
		const int numSamplesInFirstBuffer = localReadBuffer->getNumSamples() - indexBeforeWrap;

		voiceBuffer.setUseOneMap(localReadBuffer->useOneMap);

		jassert(numSamplesInFirstBuffer >= 0);

		// Reset the offset so that the first one will go through
		auto existingOffset = localReadBuffer->getNormaliseMap(0).getOffset();
		auto offsetInBuffer = indexBeforeWrap % COMPRESSION_BLOCK_SIZE;

		voiceBuffer.clearNormalisation({});

		voiceBuffer.getNormaliseMap(0).setOffset(existingOffset + offsetInBuffer);

		if(!localReadBuffer->useOneMap)
			voiceBuffer.getNormaliseMap(1).setOffset(localReadBuffer->getNormaliseMap(1).getOffset());

		if (numSamplesInFirstBuffer > 0)
		{
            hlac::HiseSampleBuffer::copy(voiceBuffer, *localReadBuffer, 0, indexBeforeWrap, numSamplesInFirstBuffer);
		}

		const int offset = numSamplesInFirstBuffer;
        int numSamplesToCopyFromSecondBuffer = (int)(ceil(numSamples - (double)numSamplesInFirstBuffer)) + 1;
        
        if(entireSampleIsLoaded)
        {
            if(sound.get()->isLoopEnabled())
            {
                auto offsetInLoop = localReadBuffer->getNumSamples() - sound.get()->getLoopEnd();
                auto startInBuffer = sound.get()->getLoopStart() + offsetInLoop;
                
                hlac::HiseSampleBuffer::copy(voiceBuffer, *localReadBuffer, offset, startInBuffer, numSamplesToCopyFromSecondBuffer);
            }
            else
            {
                voiceBuffer.clear(offset, numSamplesToCopyFromSecondBuffer);
            }
        }
        else
        {
            const int numSamplesAvailableInSecondBuffer = localWriteBuffer->getNumSamples() - offset;
            
            if ((numSamplesAvailableInSecondBuffer > 0) && (numSamplesAvailableInSecondBuffer <= localWriteBuffer->getNumSamples()))
            {
                //const int numSamplesToCopyFromSecondBuffer = jmin<int>(numSamplesAvailableInSecondBuffer, voiceBuffer.getNumSamples() - offset);
                
                numSamplesToCopyFromSecondBuffer = jmin<int>(numSamplesToCopyFromSecondBuffer, numSamplesAvailableInSecondBuffer);
                
                if (writeBufferIsBeingFilled || entireSampleIsLoaded)
                    voiceBuffer.clear(offset, numSamplesToCopyFromSecondBuffer);
                else
                    hlac::HiseSampleBuffer::copy(voiceBuffer, *localWriteBuffer, offset, 0, numSamplesToCopyFromSecondBuffer);
            }
            else
            {
                // The streaming buffers must be greater than the block size!
                jassertfalse;
                voiceBuffer.clear();
            }
        }
        
		StereoChannelData returnData;

		returnData.b = &voiceBuffer;
		returnData.offsetInBuffer = 0;


#if USE_SAMPLE_DEBUG_COUNTER

		const float *l = voiceBuffer.getReadPointer(0, 0);
		const float *r = voiceBuffer.getReadPointer(1, 0);

		float ll = l[0];
		float lr = r[0];

		for (int i = 1; i < voiceBuffer.getNumSamples(); i++)
		{
			const float tl = l[i];
			const float tr = r[i];

			jassert(tl == tr);
			jassert(tl - ll == 1.0f);
			ll = tl;
			lr = tr;
		}
#endif

		return returnData;
	}
	else
	{
		const int index = (int)readIndexDouble;

		StereoChannelData returnData;

		if(entireSampleIsLoaded && getLoadedSound()->isReleaseStartEnabled() && getLoadedSound()->isLoopEnabled())
		{
			getLoadedSound()->fillSampleBuffer(voiceBuffer, (int)(numSamples) + 2, index, getReleasePlayState());
			returnData.b = &voiceBuffer;
			returnData.offsetInBuffer = 0;
		}
		else
		{
			returnData.b = localReadBuffer;
			returnData.offsetInBuffer = index;
		}
		
		return returnData;
	}
}

bool SampleLoader::advanceReadIndex(double uptime)
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	if(seekToReleaseStart)
	{
		seekToReleaseStart = false;

		if(entireSampleIsLoaded)
		{
			readIndexDouble = uptime;
			return true;
		}
		else
		{
			readBuffer = sound.get()->getReleaseStartBuffer();
			writeBuffer = &b1;

			lastSwapPosition = sound.get()->getReleaseStart() - sound.get()->getSampleStart();
			readIndexDouble = uptime - lastSwapPosition;
			positionInSampleFile = lastSwapPosition + readBuffer.get()->getNumSamples();
			return requestNewData();
		}
	}
#endif

	int numSamplesInBuffer = readBuffer.get()->getNumSamples();
	readIndexDouble = uptime - lastSwapPosition;

	if (readIndexDouble >= numSamplesInBuffer)
	{
		if (entireSampleIsLoaded)
		{
            if(sound.get()->isLoopEnabled())
            {
                auto loopLengthDouble = (double)sound.get()->getLoopLength();
                
                lastSwapPosition += loopLengthDouble;
                readIndexDouble = uptime - lastSwapPosition;
            }
            
			return true;
		}
		else
		{
			lastSwapPosition = (double)positionInSampleFile;
			positionInSampleFile += getNumSamplesForStreamingBuffers();
			readIndexDouble = uptime - lastSwapPosition;

			swapBuffers();
			const bool queueIsFree = requestNewData();

			return queueIsFree;
		}
	}

	return true;
}

int SampleLoader::getNumSamplesForStreamingBuffers() const
{
	jassert(b1.getNumSamples() == b2.getNumSamples());
	return b1.getNumSamples();
}

bool SampleLoader::requestNewData()
{
	cancelled = false;

	if (nonRealtime)
	{
		runJob();
		return true;
	}

#if KILL_VOICES_WHEN_STREAMING_IS_BLOCKED
	if (this->isQueued() && !isWaitingForTimestretchSeek())
	{
		writeBuffer.get()->clear();

		cancelled = true;
		backgroundPool->notify();
		return false;
	}
	else
	{
		backgroundPool->addJob(this, false);
		return true;
	}
#else
	backgroundPool->addJob(this, false);
	return true;
#endif
};


SampleThreadPoolJob::JobStatus SampleLoader::runJob()
{
	if(isWaitingForTimestretchSeek())
	{
		voiceToSeekTimestretchStart->skipTimestretchSilenceAtStart();

		const auto& f = voiceToSeekTimestretchStart->delayedStartFunction;

		if(f)
			f(false, voiceToSeekTimestretchStart->voiceIndexForDelayedStart);

		voiceToSeekTimestretchStart = nullptr;

		

		return SampleThreadPoolJob::jobHasFinished;
	}

	if (cancelled)
	{
		return SampleThreadPoolJob::jobHasFinished;
	}

	const double readStart = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());

	if (writeBufferIsBeingFilled)
	{
		return SampleThreadPoolJob::jobNeedsRunningAgain;
	}

	writeBufferIsBeingFilled = true; // A poor man's mutex but gets the job done.

	const StreamingSamplerSound *localSound = sound.get();

	if (!voiceCounterWasIncreased && localSound != nullptr)
	{
		localSound->increaseVoiceCount();
		voiceCounterWasIncreased = true;
	}

	fillInactiveBuffer();

	writeBufferIsBeingFilled = false;

	const double readStop = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
	const double readTime = (readStop - readStart);
	const double timeSinceLastCall = readStop - lastCallToRequestData;
	const float diskUsageThisTime = jmax<float>(diskUsage.get(), (float)(readTime / timeSinceLastCall));
	diskUsage = diskUsageThisTime;
	lastCallToRequestData = readStart;

	return SampleThreadPoolJob::JobStatus::jobHasFinished;
}

size_t SampleLoader::getActualStreamingBufferSize() const
{
	return b1.getNumSamples() * 2 * 2;
}

void SampleLoader::fillInactiveBuffer()
{
	const StreamingSamplerSound *localSound = sound.get();

	if (localSound == nullptr) return;

	if (localSound != nullptr)
	{
		if (localSound->hasEnoughSamplesForBlock(positionInSampleFile + getNumSamplesForStreamingBuffers()))
		{
			localSound->fillSampleBuffer(*writeBuffer.get(), getNumSamplesForStreamingBuffers(), (int)positionInSampleFile, getReleasePlayState());
		}
		else if (localSound->hasEnoughSamplesForBlock(positionInSampleFile))
		{
			const int numSamplesToFill = (int)localSound->getSampleLength() - positionInSampleFile;
			const int numSamplesToClear = getNumSamplesForStreamingBuffers() - numSamplesToFill;

			localSound->fillSampleBuffer(*writeBuffer.get(), numSamplesToFill, (int)positionInSampleFile, getReleasePlayState());

			writeBuffer.get()->clear(numSamplesToFill, numSamplesToClear);
		}
		else
		{
			writeBuffer.get()->clear();
		}

#if LOG_SAMPLE_RENDERING
		logger->checkAssertion(nullptr, DebugLogger::Location::SampleLoaderReadOperation, localSound != nullptr, 1174);
#endif

#if USE_SAMPLE_DEBUG_COUNTER

		const float *l = writeBuffer.get()->getReadPointer(0);
		const float *r = writeBuffer.get()->getReadPointer(1);

		int co = (int)positionInSampleFile;

		for (int i = 0; i < writeBuffer.get()->getNumSamples(); i++)
		{

			const float tl = l[i];
			const float tr = r[i];
			const float expected = (float)co;

			jassert(tl == tr);
			jassert(tl == 0.0f || (abs(expected - tl) < 0.00001f));

			co++;

		}
#endif
	}
};

void SampleLoader::refreshBufferSizes()
{
	const int numSamplesToUse = jmax<int>(idealBufferSize, minimumBufferSizeForSamplesPerBlock);

	if (getNumSamplesForStreamingBuffers() < numSamplesToUse)
	{
		StreamingHelpers::increaseBufferIfNeeded(b1, numSamplesToUse);
		StreamingHelpers::increaseBufferIfNeeded(b2, numSamplesToUse);

		readBuffer = &b1;
		writeBuffer = &b2;

		reset();
	}
}

bool SampleLoader::swapBuffers()
{
	auto localReadBuffer = readBuffer.get();

#if HISE_SAMPLER_ALLOW_RELEASE_START
	if(localReadBuffer == sound.get()->getReleaseStartBuffer())
	{
		readBuffer = writeBuffer.get();

		if(readBuffer.get() == &b1)
		{
			writeBuffer = &b2;
			DBG("READ IS B1");
		}
			
		else
		{
			writeBuffer = &b1;
			DBG("READ IS B2");
		}
	}
#endif

	if (localReadBuffer == &b1)
	{
		readBuffer = &b2;
		writeBuffer = &b1;
	}
	else // This condition will also be true if the read pointer points at the preload buffer
	{
		readBuffer = &b1;
		writeBuffer = &b2;
	}

	isReadingFromPreloadBuffer = false;
	sampleStartModValue = 0;

	return writeBufferIsBeingFilled == false;
};

// ==================================================================================================== StreamingSamplerVoice methods

StreamingSamplerVoice::StreamingSamplerVoice(SampleThreadPool *pool) :
	loader(pool),
	sampleStartModValue(0),
	stretcher(false),
	stretchRatio(1.0)
{
	pitchData = nullptr;
};


void StreamingSamplerVoice::startNote(int /*midiNoteNumber*/,
	float /*velocity*/,
	SynthesiserSound* s,
	int /*currentPitchWheelPosition*/)
{
	StreamingSamplerSound *sound = dynamic_cast<StreamingSamplerSound*>(s);

	stretcher.configure(sound->isStereo() ? 2 : 1, sound->getSampleRate());

	if (sound != nullptr && sound->getSampleLength() > 0)
	{
		loader.startNote(sound, sampleStartModValue);

		jassert(sound != nullptr);
		
		voiceUptime = (double)sampleStartModValue;

		// You have to call setPitchFactor() before startNote().
		jassert(uptimeDelta != 0.0);

		// Resample if sound has different samplerate than the audio sample rate
		uptimeDelta *= (sound->getSampleRate() / getSampleRate());

		if(!sound->isEntireSampleLoaded())
			uptimeDelta = jmin<double>((double)MAX_SAMPLER_PITCH, uptimeDelta);

		constUptimeDelta = uptimeDelta;

#if HISE_SAMPLER_ALLOW_RELEASE_START
		jumpToReleaseOnNextRender = false;
		releaseFadeDuration = 0;
		releaseFadeCounter = 0;
		releaseGain = 1.0f;
#endif

		isActive = true;

		if(stretcher.isEnabled())
			stretcherNeedsInitialisation = true;
	}
	else
	{
		resetVoice();
	}
}

void StreamingSamplerVoice::skipTimestretchSilenceAtStart()
{
	auto numBeforeOutput = stretcher.getLatency(stretchRatio);

	StereoChannelData data = loader.fillVoiceBuffer(*getTemporaryVoiceBuffer(), numBeforeOutput);

	auto outL = (float*)alloca(sizeof(float*) * numBeforeOutput);
	auto outR = (float*)alloca(sizeof(float*) * numBeforeOutput);

	interpolateFromStereoData(0, outL, outR, numBeforeOutput, nullptr, 1.0, 0.0, data, numBeforeOutput);

	float* inp[2] = { outL, outR };

	voiceUptime += stretcher.skipLatency(inp, stretchRatio);

	if (!loader.advanceReadIndex(voiceUptime))
	{
		jassertfalse;
		resetVoice();
	}
}

const StreamingSamplerSound * StreamingSamplerVoice::getLoadedSound()
{
	return loader.getLoadedSound();
}

void StreamingSamplerVoice::setLoaderBufferSize(int newBufferSize)
{
	loader.setBufferSize(newBufferSize);
}

void StreamingSamplerVoice::stopNote(float, bool /*allowTailOff*/)
{
	clearCurrentNote();
	loader.reset();
}

void StreamingSamplerVoice::setDebugLogger(DebugLogger* newLogger)
{
	logger = newLogger;
	loader.setLogger(logger);
}

static int alignedCalls = 0;
static int unalignedCalls = 0;

#define USE_CUBIC_INTERPOLATION 0 // not there yet, need to fetch more samples to get 4 values...

template <typename SignalType, bool isFloat> void interpolateMonoSamples(const SignalType* inL, const SignalType* unusedIn, const float* pitchData, float* outL, float* unusedOut, int startSample, double indexInBuffer, double uptimeDelta, int numSamples)
{
	ignoreUnused(unusedIn, unusedOut);

	constexpr float gainFactor = isFloat ? 1.0f : (1.0f / (float)INT16_MAX);

	if (pitchData != nullptr)
	{
		pitchData += startSample;

		float indexInBufferFloat = (float)indexInBuffer;

		for (int i = 0; i < numSamples; i++)
		{
			const int pos = int(indexInBufferFloat);
			const float alpha = indexInBufferFloat - (float)pos;

			auto l1 = (float)inL[pos];
			auto l2 = (float)inL[pos + 1];
			
#if USE_CUBIC_INTERPOLATION
			auto l0 = (float)(pos > 0 ? inL[pos - 1] : 0);
			auto l3 = (float)inL[pos + 2];

			float l = Interpolator::interpolateCubic(l0, l1, l2, l3, alpha);
#else
			float l = Interpolator::interpolateLinear(l1, l2, alpha);
#endif

			outL[i] = l * gainFactor;

			jassert(*pitchData <= (float)MAX_SAMPLER_PITCH);

			indexInBufferFloat += pitchData[i];
		}
	}
	else
	{
		float indexInBufferFloat = (float)indexInBuffer;
		const float uptimeDeltaFloat = (float)uptimeDelta;

		while (numSamples > 0)
		{
			const int pos = int(indexInBufferFloat);
			const float alpha = indexInBufferFloat - (float)pos;
			
			auto l1 = (float)inL[pos];
			auto l2 = (float)inL[pos + 1];
			
#if USE_CUBIC_INTERPOLATION
			auto l0 = (float)(pos > 0 ? inL[pos - 1] : 0);
			auto l3 = (float)inL[pos + 2];

			float l = Interpolator::interpolateCubic(l0, l1, l2, l3, alpha);
#else
			float l = Interpolator::interpolateLinear(l1, l2, alpha);
#endif

			*outL++ = l * gainFactor;

			indexInBufferFloat += uptimeDeltaFloat;

			numSamples--;
		}
	}
}

template <typename SignalType, bool isFloat> void interpolateStereoSamples(const SignalType* inL, const SignalType* inR, const float* pitchData, float* outL, float* outR, int startSample, double indexInBuffer, double uptimeDelta, int numSamples, int maxIndexInBuffer)
{
	constexpr float gainFactor = isFloat ? 1.0f : (1.0f / (float)INT16_MAX);

	if (pitchData != nullptr)
	{
		pitchData += startSample;

		float indexInBufferFloat = (float)indexInBuffer;

		for (int i = 0; i < numSamples; i++)
		{
			const int pos = int(indexInBufferFloat);

			if (pos >= maxIndexInBuffer)
				return;

			const float alpha = indexInBufferFloat - (float)pos;

			auto l1 = (float)inL[pos];
			auto l2 = (float)inL[pos+1];
			auto r1 = (float)inR[pos];
			auto r2 = (float)inR[pos + 1];

#if USE_CUBIC_INTERPOLATION
			auto l0 = (float)(pos > 0 ? inL[pos - 1] : 0);
			auto l3 = (float)inL[pos + 2];
			auto r0 = (float)(pos > 0 ? inR[pos - 1] : 0);
			auto r3 = (float)inR[pos + 2];

			float l = Interpolator::interpolateCubic(l0, l1, l2, l3, alpha);
			float r = Interpolator::interpolateCubic(r0, r1, r2, r3, alpha);
#else
			float l = Interpolator::interpolateLinear(l1, l2, alpha);
			float r = Interpolator::interpolateLinear(r1, r2, alpha);
#endif

			outL[i] = l * gainFactor;
			outR[i] = r * gainFactor;

			//jassert(*pitchData <= (float)MAX_SAMPLER_PITCH);

			indexInBufferFloat += pitchData[i];
		}
	}
	else
	{
		float indexInBufferFloat = (float)indexInBuffer;
		const float uptimeDeltaFloat = (float)uptimeDelta;

		auto numTargetSamples = (double)(maxIndexInBuffer - indexInBuffer);

		jassert(numTargetSamples > 0.0);

		numSamples = jmin(numSamples, (int)(numTargetSamples / uptimeDelta));

		while (numSamples > 0)
		{
			const int pos = int(indexInBufferFloat);
			const float alpha = indexInBufferFloat - (float)pos;
			
			auto l1 = (float)inL[pos];
			auto l2 = (float)inL[pos + 1];
			auto r1 = (float)inR[pos];
			auto r2 = (float)inR[pos + 1];
			
#if USE_CUBIC_INTERPOLATION
			auto l0 = (float)(pos > 0 ? inL[pos - 1] : 0);
			auto l3 = (float)inL[pos + 2];
			auto r0 = (float)(pos > 0 ? inR[pos - 1] : 0);
			auto r3 = (float)inR[pos + 2];

			float l = Interpolator::interpolateCubic(l0, l1, l2, l3, alpha);
			float r = Interpolator::interpolateCubic(r0, r1, r2, r3, alpha);
#else
			float l = Interpolator::interpolateLinear(l1, l2, alpha);
			float r = Interpolator::interpolateLinear(r1, r2, alpha);
#endif

			*outL++ = l * gainFactor;
			*outR++ = r * gainFactor;

			indexInBufferFloat += uptimeDeltaFloat;

			numSamples--;
		}
	}
}


void StreamingSamplerVoice::interpolateFromStereoData(int startSample, float* outL, float* outR, int numSamplesToCalculate, const float* pitchDataToUse, double thisUptimeDelta, const double startAlpha, StereoChannelData data, int samplesAvailable)
{
	double indexInBuffer = startAlpha;

	if (data.b->isFloatingPoint())
	{
		const float* const inL = static_cast<const float*>(data.b->getReadPointer(0, data.offsetInBuffer));
		const float* const inR = static_cast<const float*>(data.b->getReadPointer(1, data.offsetInBuffer));

		interpolateStereoSamples<float, true>(inL, inR, pitchDataToUse, outL, outR, startSample, indexInBuffer, thisUptimeDelta, numSamplesToCalculate, indexInBuffer + samplesAvailable);
	}
	else
	{
		const int16* const inL = static_cast<const int16*>(data.b->getReadPointer(0, data.offsetInBuffer));
		const int16* const inR = static_cast<const int16*>(data.b->getReadPointer(1, data.offsetInBuffer));

		bool useNormalisation = data.b->usesNormalisation();

		if (useNormalisation)
		{
			const int numSamplesThisTime = (int)(ceil)((pitchCounter + startAlpha)) + 1;

			float* inL_f = (float*)alloca(sizeof(float) * numSamplesThisTime);
			float* d[2] = { inL_f, nullptr };

			if (data.b->getNumChannels() == 2 && !data.b->useOneMap)
			{
				float* inR_f = (float*)alloca(sizeof(float) * numSamplesThisTime);

				d[1] = inR_f;

				data.b->convertToFloatWithNormalisation(d, data.b->getNumChannels(), data.offsetInBuffer, numSamplesThisTime);

				interpolateStereoSamples<float, true>(inL_f, inR_f, pitchDataToUse, outL, outR, startSample, indexInBuffer, thisUptimeDelta, numSamplesToCalculate, indexInBuffer + samplesAvailable);
			}
			else
			{
				data.b->convertToFloatWithNormalisation(d, 1, data.offsetInBuffer, numSamplesThisTime);

				interpolateMonoSamples<float, true>(inL_f, nullptr, pitchDataToUse, outL, nullptr, startSample, indexInBuffer, thisUptimeDelta, numSamplesToCalculate);

				memcpy(outR, outL, sizeof(float) * numSamplesToCalculate);
			}
		}
		else
		{
			interpolateStereoSamples<int16, false>(inL, inR, pitchData, outL, outR, startSample, indexInBuffer, thisUptimeDelta, numSamplesToCalculate, indexInBuffer + samplesAvailable);
		}
	}
}

void StreamingSamplerVoice::renderNextBlock(AudioSampleBuffer &outputBuffer, int startSample, int numSamples)
{
	const StreamingSamplerSound *sound = loader.getLoadedSound();

#if USE_SAMPLE_DEBUG_COUNTER
	const int startDebug = startSample;
	const int numDebug = numSamples;
#endif

	if (sound != nullptr)
	{
		

		float* outL = outputBuffer.getWritePointer(0, startSample);
		float* outR = outputBuffer.getWritePointer(1, startSample);

		float* postStretchL = outL;
		float* postStretchR = outR;

		int numSamplesToCalculate = numSamples;
		auto pitchDataToUse = pitchData;
		auto thisUptimeDelta = uptimeDelta;

		if(stretcher.isEnabled())
		{
			pitchCounter = numSamples * stretchRatio;
			numSamplesToCalculate = roundToInt(pitchCounter);

			if (pitchData != nullptr)
				thisUptimeDelta *= pitchData[0];

			auto pitchSt = std::log2(thisUptimeDelta) * 12.0;

			if(stretcherNeedsInitialisation)
			{
				auto isDelayed = initStretcher(pitchSt);

				if(isDelayed == sendNotificationAsync)
				{
					jassert(delayedStartFunction);
					delayedStartFunction(true, voiceIndexForDelayedStart);
				}

				stretcherNeedsInitialisation = false;
			}
			else
				stretcher.setTransposeSemitones(pitchSt, timestretchTonality);


			if(loader.isWaitingForTimestretchSeek())
			{
				DBG("WAIT UNTIL SEEK");
				return;
			}

			pitchDataToUse = nullptr;
			thisUptimeDelta = 1.0;
			
			outL = stretchBuffer->getWritePointer(0, 0);
			outR = stretchBuffer->getWritePointer(1, 0);
		}

		const double startAlpha = fmod(voiceUptime, 1.0);

		jassert(pitchCounter != 0);

		auto tempVoiceBuffer = getTemporaryVoiceBuffer();

		jassert(tempVoiceBuffer != nullptr);
		if (!isPositiveAndBelow(pitchCounter + startAlpha, (double)tempVoiceBuffer->getNumSamples()))
		{
			tempVoiceBuffer->setSize(tempVoiceBuffer->getNumChannels(), roundToInt((pitchCounter + startAlpha) * 1.5));
		}

		// Copy the not resampled values into the voice buffer.
		StereoChannelData data = loader.fillVoiceBuffer(*tempVoiceBuffer, pitchCounter + startAlpha);
		
		bool applyReleaseGainToFullBuffer = true;

#if HISE_SAMPLER_ALLOW_RELEASE_START
		if(jumpToReleaseOnNextRender || releaseFadeCounter > 0)
		{
			auto options = loader.getLoadedSound()->getReleaseStartOptions();

			jassert(options != nullptr);

			if(jumpToReleaseOnNextRender)
			{
				if(options->gainMatchingMode == StreamingHelpers::ReleaseStartOptions::GainMatchingMode::Volume)
				{
					releaseGain = loader.getLoadedSound()->getReleaseAttenuation();
				}

				releaseFadeDuration = options->releaseFadeTime;
				releaseFadeCounter = releaseFadeDuration;
				jumpToReleaseOnNextRender = false;
			}

			auto numToFadeIn = std::ceil(pitchCounter + startAlpha) + 2;

			if(data.b != tempVoiceBuffer)
			{
				hlac::HiseSampleBuffer::copy(*tempVoiceBuffer, *data.b, 0, data.offsetInBuffer, numToFadeIn);
				data.b = tempVoiceBuffer;
				data.offsetInBuffer = 0;
			}

			tempVoiceBuffer->burnNormalisation(true);

			auto startGain = jmax(0.0, (double)releaseFadeCounter) / (double)releaseFadeDuration;
			auto endGain = jmax(0.0, ((double)releaseFadeCounter - numToFadeIn) / (double)releaseFadeDuration);

			tempVoiceBuffer->applyGainRampWithGamma(0, 0, numToFadeIn, startGain, endGain, options->fadeGamma);
			tempVoiceBuffer->applyGainRampWithGamma(1, 0, numToFadeIn, startGain, endGain, options->fadeGamma);

			auto rb = sound->getReleaseStartBuffer();

			auto offsetInBuffer = releaseFadeDuration - releaseFadeCounter;


			auto numToCopy = jmin(rb->getNumSamples() - offsetInBuffer - numToFadeIn, numToFadeIn);

			if(releaseGain != 1.0f)
			{
				hlac::HiseSampleBuffer::addWithGain(*tempVoiceBuffer, *rb, 0, offsetInBuffer, numToCopy, releaseGain);
				applyReleaseGainToFullBuffer = false;
			}
			else
			{
				hlac::HiseSampleBuffer::add(*tempVoiceBuffer, *rb, 0, offsetInBuffer, numToCopy);
			}

			releaseFadeCounter -= (pitchCounter + startAlpha);

			if(releaseFadeCounter <= 0.0)
			{
				voiceUptime = (double)(sound->getReleaseStart() - sound->getSampleStart());

				voiceUptime += releaseFadeDuration;
				voiceUptime += (-1) * releaseFadeCounter;
				voiceUptime -= pitchCounter;
				
				releaseFadeCounter = 0.0;
				loader.setSeekToReleaseStart();
			}
		}
#endif
		
		auto samplesAvailable = data.b->getNumSamples() - data.offsetInBuffer;

		const int startFixed = startSample;
		const int numSamplesFixed = numSamples;

#if USE_SAMPLE_DEBUG_COUNTER
		jassert((int)voiceUptime == data.leftChannel[0]);
#endif

		interpolateFromStereoData(startSample, outL, outR, numSamplesToCalculate, pitchDataToUse, thisUptimeDelta, startAlpha,
		                          data, samplesAvailable);

		

#if HISE_SAMPLER_ALLOW_RELEASE_START
		if(releaseGain != 1.0f && applyReleaseGainToFullBuffer)
		{
			outputBuffer.applyGain(startSample, numSamples, releaseGain);
		}
		else
		{
			loader.getLoadedSound()->calculateReleasePeak(outL, numSamplesToCalculate);
		}
#endif

		
		
#if USE_SAMPLE_DEBUG_COUNTER 

		for (int i = startDebug; i < numDebug; i++)
		{
			const float l = outputBuffer.getSample(0, i);
			const float r = outputBuffer.getSample(1, i);

			jassert(l == r);
			jassert((abs(l - voiceUptime) < 0.000001) || l == 0.0f);

			voiceUptime += uptimeDelta;

		}

		outputBuffer.clear();
#else
		voiceUptime += pitchCounter;
#endif

		if(stretcher.isEnabled())
		{
			float* inp[2];
			float* out[2];

			inp[0] = outL;
			inp[1] = outR;

			out[0] = postStretchL;
			out[1] = postStretchR;

			int numInput = roundToInt(pitchCounter);

			int numOutput = numSamples;

			stretcher.process(inp, numInput, out, numOutput);
            
            if(!sound->isStereo())
                FloatVectorOperations::copy(out[1], out[0], numOutput);
		}

		if (!loader.advanceReadIndex(voiceUptime))
		{
#if LOG_SAMPLE_RENDERING
			logger->addStreamingFailure(voiceUptime);
#endif

			outputBuffer.clear(startFixed, numSamplesFixed);

			resetVoice();
			return;
		}

		bool enoughSamples = sound->hasEnoughSamplesForBlock((int)(voiceUptime));// +numSamples * MAX_SAMPLER_PITCH));

#if HISE_SAMPLER_ALLOW_RELEASE_START
		if(loader.getReleasePlayState() == StreamingSamplerSound::ReleasePlayState::Playing && voiceUptime > sound->getSampleLength())
			enoughSamples = false;
#endif


#if LOG_SAMPLE_RENDERING
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFillPost, true, outputBuffer.getReadPointer(0, startFixed), numSamplesFixed);
		logger->checkSampleData(nullptr, DebugLogger::Location::SampleVoiceBufferFillPost, false, outputBuffer.getReadPointer(1, startFixed), numSamplesFixed);
#endif

		if (!enoughSamples) resetVoice();
	}
	else
	{
		resetVoice();
	}
};

void StreamingSamplerVoice::setPitchFactor(int midiNote, int rootNote, StreamingSamplerSound *sound, double globalPitchFactor)
{
	if (midiNote == rootNote)
	{
		uptimeDelta = globalPitchFactor;
	}
	else
	{
		uptimeDelta = sound->getPitchFactor(midiNote, rootNote) * globalPitchFactor;
	}

	if (!sound->isEntireSampleLoaded())
	{
		uptimeDelta = jmin(uptimeDelta, (double)MAX_SAMPLER_PITCH);
	}
}

void StreamingSamplerVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (sampleRate != -1.0)
	{
		loader.assertBufferSize(samplesPerBlock * MAX_SAMPLER_PITCH);

		setCurrentPlaybackSampleRate(sampleRate);
	}
}

void StreamingSamplerVoice::resetVoice()
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	releaseFadeDuration = 0;
	releaseFadeCounter = 0;
	jumpToReleaseOnNextRender = false;
#endif

	voiceUptime = 0.0;
	uptimeDelta = 0.0;
	isActive = false;
	loader.reset();
	clearCurrentNote();
}

void StreamingSamplerVoice::setSampleStartModValue(int newValue)
{
	jassert(newValue >= 0);

	sampleStartModValue = newValue;
}

hlac::HiseSampleBuffer * StreamingSamplerVoice::getTemporaryVoiceBuffer()
{
	jassert(tvb != nullptr);

	return tvb;
}

void StreamingSamplerVoice::setTemporaryVoiceBuffer(hlac::HiseSampleBuffer* buffer, AudioSampleBuffer* stretchBuffer_)
{
	tvb = buffer;
	stretchBuffer = stretchBuffer_;
}

void StreamingSamplerVoice::initTemporaryVoiceBuffer(hlac::HiseSampleBuffer* bufferToUse, int samplesPerBlock, double maxPitchRatio)
{
	// The channel amount must be set correctly in the constructor
	jassert(bufferToUse->getNumChannels() > 0);

    auto requiredSampleAmount = roundToInt((double)samplesPerBlock* maxPitchRatio);
    
	if (bufferToUse->getNumSamples() < requiredSampleAmount)
	{
		bufferToUse->setSize(bufferToUse->getNumChannels(), requiredSampleAmount);
		bufferToUse->clear();
	}
}

void StreamingSamplerVoice::setStreamingBufferDataType(bool shouldBeFloat)
{
	loader.setStreamingBufferDataType(shouldBeFloat);
}

NotificationType StreamingSamplerVoice::initStretcher(float pitchSt)
{
	jassert(stretcher.isEnabled());

	auto sound = const_cast<StreamingSamplerSound*>(loader.getLoadedSound());
	stretcher.configure(sound->isStereo() ? 2 : 1, sound->getSampleRate());
	stretcher.setResampleBuffer(1.0, nullptr, 0);
	stretcher.setTransposeSemitones(pitchSt, timestretchTonality);

	if(skipLatency == NotificationType::sendNotificationSync || loader.isNonRealtime())
	{
		loader.waitForTimestretchSeek(nullptr);
		skipTimestretchSilenceAtStart();
		return dontSendNotification;
	}
	else if (skipLatency == NotificationType::sendNotificationAsync)
	{
		return loader.waitForTimestretchSeek(this);
	}
	else
	{
		stretcher.reset();
	}
		
	stretcherNeedsInitialisation = false;
	return dontSendNotification;
}

void SampleLoader::Unmapper::setLoader(SampleLoader *loader_)
{
	loader = loader_;
}

void SampleLoader::Unmapper::setSoundToUnmap(const StreamingSamplerSound *s)
{
	jassert(sound == nullptr);
	sound = const_cast<StreamingSamplerSound *>(s);
}

SampleThreadPool::Job::JobStatus SampleLoader::Unmapper::runJob()
{
	jassert(sound != nullptr);

	if (loader->isRunning())
	{
		jassertfalse;
		return SampleThreadPoolJob::jobNeedsRunningAgain;
	}

	if (sound != nullptr)
	{
		sound->decreaseVoiceCount();
		sound->closeFileHandle();

		sound = nullptr;
	}

	return SampleThreadPoolJob::jobHasFinished;
}

} // namespace hise
