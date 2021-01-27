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

	s->wakeSound();

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
			returnData.b = localReadBuffer;
			returnData.offsetInBuffer = index;
			return returnData;
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
		const int numSamplesAvailableInSecondBuffer = localWriteBuffer->getNumSamples() - offset;

		if ((numSamplesAvailableInSecondBuffer > 0) && (numSamplesAvailableInSecondBuffer <= localWriteBuffer->getNumSamples()))
		{
			//const int numSamplesToCopyFromSecondBuffer = jmin<int>(numSamplesAvailableInSecondBuffer, voiceBuffer.getNumSamples() - offset);

			int numSamplesToCopyFromSecondBuffer = (int)(ceil(numSamples - (double)numSamplesInFirstBuffer)) + 1;

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
		returnData.b = localReadBuffer;
		returnData.offsetInBuffer = index;

		return returnData;
	}
}

bool SampleLoader::advanceReadIndex(double uptime)
{
	const int numSamplesInBuffer = readBuffer.get()->getNumSamples();
	readIndexDouble = uptime - lastSwapPosition;

	if (readIndexDouble >= numSamplesInBuffer)
	{
		if (entireSampleIsLoaded)
		{
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
	if (this->isQueued())
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
			localSound->fillSampleBuffer(*writeBuffer.get(), getNumSamplesForStreamingBuffers(), (int)positionInSampleFile);
		}
		else if (localSound->hasEnoughSamplesForBlock(positionInSampleFile))
		{
			const int numSamplesToFill = (int)localSound->getSampleLength() - positionInSampleFile;
			const int numSamplesToClear = getNumSamplesForStreamingBuffers() - numSamplesToFill;

			localSound->fillSampleBuffer(*writeBuffer.get(), numSamplesToFill, (int)positionInSampleFile);

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
	sampleStartModValue(0)
{
	pitchData = nullptr;
};


void StreamingSamplerVoice::startNote(int /*midiNoteNumber*/,
	float /*velocity*/,
	SynthesiserSound* s,
	int /*currentPitchWheelPosition*/)
{
	StreamingSamplerSound *sound = dynamic_cast<StreamingSamplerSound*>(s);

	if (sound != nullptr && sound->getSampleLength() > 0)
	{
		loader.startNote(sound, sampleStartModValue);

		jassert(sound != nullptr);
		sound->wakeSound();

		voiceUptime = (double)sampleStartModValue;

		// You have to call setPitchFactor() before startNote().
		jassert(uptimeDelta != 0.0);

		// Resample if sound has different samplerate than the audio sample rate
		uptimeDelta *= (sound->getSampleRate() / getSampleRate());

		if(!sound->isEntireSampleLoaded())
			uptimeDelta = jmin<double>((double)MAX_SAMPLER_PITCH, uptimeDelta);

		constUptimeDelta = uptimeDelta;

		isActive = true;

	}
	else
	{
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

template <typename SignalType, bool isFloat> void interpolateMonoSamples(const SignalType* inL, const SignalType* unusedIn, const float* pitchData, float* outL, float* unusedOut, int startSample, double indexInBuffer, double uptimeDelta, int numSamples)
{
	ignoreUnused(unusedIn, unusedOut);

	constexpr float gainFactor = isFloat ? 1.0f : (1.0f / (float)INT16_MAX);

	int osFactor = 4;

	if (pitchData != nullptr)
	{
		pitchData += startSample;

		float indexInBufferFloat = (float)indexInBuffer;

		for (int i = 0; i < numSamples; i++)
		{
            		float preL = 0;
            		if (pitchData[i] > 0.5)
                		osFactor = 8;
            		for (int z = 0; z < osFactor; z++)
            		{
				const int pos = int(indexInBufferFloat);
				float alpha = indexInBufferFloat - (float)pos;
				alpha = pow(alpha, jmax(1.0f, 0.25f / pitchData[i] * osFactor));
				const float invAlpha = 1.0f - alpha;

				float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);


				jassert(*pitchData <= (float)MAX_SAMPLER_PITCH);

				indexInBufferFloat += (pitchData[i] / osFactor);

                		preL += l;
			}

			outL[i] = preL * gainFactor / osFactor;
		}
	}
	else
	{
		float indexInBufferFloat = (float)indexInBuffer;
		const float uptimeDeltaFloat = (float)uptimeDelta;

		while (numSamples > 0)
		{
            		float preL = 0;
            		if (uptimeDeltaFloat > 0.5)
                		osFactor = 8;
            		for (int z = 0; z < osFactor; z++)
            		{
				const int pos = int(indexInBufferFloat);
				float alpha = indexInBufferFloat - (float)pos;
				alpha = pow(alpha, jmax(1.0f, 0.25f / uptimeDeltaFloat * osFactor));
				const float invAlpha = 1.0f - alpha;

				float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);


				indexInBufferFloat += (uptimeDeltaFloat / osFactor);

                		preL += l;
			}
			*outL++ = preL * gainFactor / osFactor;
			numSamples--;
		}
	}
}

template <typename SignalType, bool isFloat> void interpolateStereoSamples(const SignalType* inL, const SignalType* inR, const float* pitchData, float* outL, float* outR, int startSample, double indexInBuffer, double uptimeDelta, int numSamples, int maxIndexInBuffer)
{
	constexpr float gainFactor = isFloat ? 1.0f : (1.0f / (float)INT16_MAX);

	int osFactor = 4;

	if (pitchData != nullptr)
	{
		pitchData += startSample;

		float indexInBufferFloat = (float)indexInBuffer;

		for (int i = 0; i < numSamples; i++)
		{
            		float preL = 0;
            		float preR = 0;
            		if (pitchData[i] > 0.5)
                		osFactor = 8;
            		for (int z = 0; z < osFactor; z++)
            		{
			const int pos = int(indexInBufferFloat);

			if (pos >= maxIndexInBuffer)
				return;

			float alpha = indexInBufferFloat - (float)pos;
			alpha = pow(alpha, jmax(1.0f, 0.25f / pitchData[i] * osFactor));
			const float invAlpha = 1.0f - alpha;

			float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);
			float r = ((float)inR[pos] * invAlpha + (float)inR[pos + 1] * alpha);

			jassert(*pitchData <= (float)MAX_SAMPLER_PITCH);

			indexInBufferFloat += (pitchData[i] / osFactor);
                	preL += l;
                	preR += r;
			}
			outL[i] = preL * gainFactor / osFactor;
			outR[i] = preR * gainFactor / osFactor;
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
            		float preL = 0;
            		float preR = 0;
            		if (uptimeDeltaFloat > 0.5)
                		osFactor = 8;
            		for (int z = 0; z < osFactor; z++)
            		{
			const int pos = int(indexInBufferFloat);
			float alpha = indexInBufferFloat - (float)pos;
			alpha = pow(alpha, jmax(1.0f, 0.25f / uptimeDeltaFloat * osFactor));
			const float invAlpha = 1.0f - alpha;

			float l = ((float)inL[pos] * invAlpha + (float)inL[pos + 1] * alpha);
			float r = ((float)inR[pos] * invAlpha + (float)inR[pos + 1] * alpha);

			indexInBufferFloat += (uptimeDeltaFloat / osFactor);
                	preL += l;
                	preR += r;
			}

			*outL++ = preL * gainFactor / osFactor;
			*outR++ = preR * gainFactor / osFactor;

			numSamples--;
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
		const double startAlpha = fmod(voiceUptime, 1.0);

		jassert(pitchCounter != 0);

		auto tempVoiceBuffer = getTemporaryVoiceBuffer();

		jassert(tempVoiceBuffer != nullptr);

		// Copy the not resampled values into the voice buffer.
		StereoChannelData data = loader.fillVoiceBuffer(*tempVoiceBuffer, pitchCounter + startAlpha);

		float* outL = outputBuffer.getWritePointer(0, startSample);
		float* outR = outputBuffer.getWritePointer(1, startSample);

		auto samplesAvailable = data.b->getNumSamples() - data.offsetInBuffer;

		const int startFixed = startSample;
		const int numSamplesFixed = numSamples;

#if USE_SAMPLE_DEBUG_COUNTER
		jassert((int)voiceUptime == data.leftChannel[0]);
#endif

		double indexInBuffer = startAlpha;

		if (data.b->isFloatingPoint())
		{
			const float* const inL = static_cast<const float*>(data.b->getReadPointer(0, data.offsetInBuffer));
			const float* const inR = static_cast<const float*>(data.b->getReadPointer(1, data.offsetInBuffer));

			interpolateStereoSamples<float, true>(inL, inR, pitchData, outL, outR, startSample, indexInBuffer, uptimeDelta, numSamples, indexInBuffer + samplesAvailable);
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

					interpolateStereoSamples<float, true>(inL_f, inR_f, pitchData, outL, outR, startSample, indexInBuffer, uptimeDelta, numSamples, indexInBuffer + samplesAvailable);
				}
				else
				{
					data.b->convertToFloatWithNormalisation(d, 1, data.offsetInBuffer, numSamplesThisTime);

					interpolateMonoSamples<float, true>(inL_f, nullptr, pitchData, outL, nullptr, startSample, indexInBuffer, uptimeDelta, numSamples);

					memcpy(outR, outL, sizeof(float) * numSamples);
				}
			}
			else
			{
				interpolateStereoSamples<int16, false>(inL, inR, pitchData, outL, outR, startSample, indexInBuffer, uptimeDelta, numSamples, indexInBuffer + samplesAvailable);
			}
		}

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

		if (!loader.advanceReadIndex(voiceUptime))
		{
#if LOG_SAMPLE_RENDERING
			logger->addStreamingFailure(voiceUptime);
#endif

			outputBuffer.clear(startFixed, numSamplesFixed);

			resetVoice();
			return;
		}

		const bool enoughSamples = sound->hasEnoughSamplesForBlock((int)(voiceUptime));// +numSamples * MAX_SAMPLER_PITCH));

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

void StreamingSamplerVoice::setTemporaryVoiceBuffer(hlac::HiseSampleBuffer* buffer)
{
	tvb = buffer;
}

void StreamingSamplerVoice::initTemporaryVoiceBuffer(hlac::HiseSampleBuffer* bufferToUse, int samplesPerBlock)
{
	// The channel amount must be set correctly in the constructor
	jassert(bufferToUse->getNumChannels() > 0);

	if (bufferToUse->getNumSamples() < samplesPerBlock*MAX_SAMPLER_PITCH)
	{
		bufferToUse->setSize(bufferToUse->getNumChannels(), samplesPerBlock*MAX_SAMPLER_PITCH);
		bufferToUse->clear();
	}
}

void StreamingSamplerVoice::setStreamingBufferDataType(bool shouldBeFloat)
{
	loader.setStreamingBufferDataType(shouldBeFloat);
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
