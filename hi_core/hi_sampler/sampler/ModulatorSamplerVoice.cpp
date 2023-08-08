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

void ModulatorSamplerVoice::startVoiceInternal(int midiNoteNumber, float velocity)
{
	auto sampler = static_cast<ModulatorSampler*>(getOwnerSynth());
	auto startMod = calculateSampleStartMod();
	auto sound = currentlyPlayingSamplerSound->getReferenceToSound();

	wrappedVoice.setPitchFactor(midiNoteNumber, !sampler->isPitchTrackingEnabled() ? midiNoteNumber : currentlyPlayingSamplerSound->getRootNote(), sound.get(), getOwnerSynth()->getMainController()->getGlobalPitchFactor());
	wrappedVoice.setSampleStartModValue(startMod);
	wrappedVoice.startNote(midiNoteNumber, velocity, sound.get(), -1);

	voiceUptime = wrappedVoice.voiceUptime;
	uptimeDelta = wrappedVoice.uptimeDelta;
	isActive = true;

	jassert(uptimeDelta > 0.0);
	jassert(sound->isEntireSampleLoaded() || uptimeDelta <= MAX_SAMPLER_PITCH);
}



int ModulatorSamplerVoice::calculateSampleStartMod()
{
	int sampleStartModulationDelta;

	// if value is >= 0, then it comes from the modulator chain value
	const bool sampleStartModIsFromChain = sampleStartModValue >= 0.0f;

	auto sound = currentlyPlayingSamplerSound->getReferenceToSound();

	if (sampleStartModIsFromChain)
	{
		jassert(sampleStartModValue <= 1.0f);
		sampleStartModulationDelta = (int)(jlimit<float>(0.0f, 1.0f, sampleStartModValue) * sound->getSampleStartModulation());
	}
	else
	{
		auto maxOffset = sound->getSampleStartModulation();

		// just flip the sign and use it directly...
		sampleStartModulationDelta = jlimit<int>(0, maxOffset, (int)(-1.0f * sampleStartModValue));

		// Now set the sample start mod value...
		BACKEND_ONLY(sampler->getSamplerDisplayValues().currentSampleStartPos = jlimit<float>(0.0f, 1.0f, sampleStartModulationDelta / (float)maxOffset));
	}

	return sampleStartModulationDelta;
}

void ModulatorSamplerVoice::startNote(int midiNoteNumber,
	float velocity,
	SynthesiserSound* s,
	int /*currentPitchWheelPosition*/)
{
    ADD_GLITCH_DETECTOR(getOwnerSynth(), DebugLogger::Location::SampleStart);
    
    ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();

	jassert(s != nullptr);

	currentlyPlayingSamplerSound = static_cast<ModulatorSamplerSound*>(s);
    
	velocityXFadeValue = currentlyPlayingSamplerSound->getGainValueForVelocityXFade((int)(velocity * 127.0f));
	
	if (playFromPurger != nullptr && 
		currentlyPlayingSamplerSound->hasUnpurgedButUnloadedSounds())
	{
		// We need to notify the sample thread to preload this sample now...
		playFromPurger->notifyStart(midiNoteNumber, velocity);
	}
	else
	{
		startVoiceInternal(midiNoteNumber, velocity);
	}
	
	
	
	if (auto fEnve = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::PanMode))
	{
		if (auto fe = sampler->getEnvelopeFilter())
		{
			snex::Types::PolyHandler::ScopedVoiceSetter svs(fe->polyManager, getVoiceIndex());
			fe->reset();
		}
		else
			jassertfalse;
	}
}

void ModulatorSamplerVoice::stopNote(float velocity, bool allowTailoff)
{
	ModulatorSynthVoice::stopNote(velocity, allowTailoff);
}

void ModulatorSamplerVoice::calculateBlock(int startSample, int numSamples)
{
	if (waitForPlayFromPurge.load())
	{
		voiceBuffer.clear(startSample, numSamples);
		return;
	}

    const StreamingSamplerSound *sound = wrappedVoice.getLoadedSound();
    
	// In a synthgroup it might be possible that the wrapped sound is null
	jassert(sound != nullptr || getOwnerSynth()->isInGroup());
 
	CHECK_AND_LOG_ASSERTION(getOwnerSynth(), DebugLogger::Location::SampleRendering, sound != nullptr, 1);

	ADD_GLITCH_DETECTOR(getOwnerSynth(), DebugLogger::Location::SampleRendering);

	auto owner = static_cast<ModulatorSampler*>(getOwnerSynth());

	if(owner->getTimestretchOptions().mode == ModulatorSampler::TimestretchOptions::TimestretchMode::TempoSynced)
	{
		PolyHandler::ScopedVoiceSetter svs(owner->getSyncVoiceHandler(), getVoiceIndex());

		wrappedVoice.setTimestretchRatio(owner->getCurrentTimestretchRatio());
	}

	ignoreUnused(sound);

	const int startIndex = startSample;
	const int samplesInBlock = numSamples;

	auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	double propertyPitch = currentlyPlayingSamplerSound->getPropertyPitch();
	
	auto oldUptime = voiceUptime;

	if (auto env = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::PitchMode))
	{
		propertyPitch *= env->getUptimeValue(voiceUptime);
	}

	applyConstantPitchFactor(propertyPitch);

	const double pitchCounter = limitPitchDataToMaxSamplerPitch(voicePitchValues, uptimeDelta, startSample, numSamples);

	wrappedVoice.setPitchCounterForThisBlock(pitchCounter);
	wrappedVoice.setPitchValues(voicePitchValues);

	

	wrappedVoice.uptimeDelta = uptimeDelta;

	voiceBuffer.clear();

	

	wrappedVoice.renderNextBlock(voiceBuffer, startSample, numSamples);

	CHECK_AND_LOG_BUFFER_DATA(getOwnerSynth(), DebugLogger::Location::SampleRendering, voiceBuffer.getReadPointer(0, startSample), true, samplesInBlock);
	CHECK_AND_LOG_BUFFER_DATA(getOwnerSynth(), DebugLogger::Location::SampleRendering, voiceBuffer.getReadPointer(1, startSample), false, samplesInBlock);

	float envGain = 1.0f;

	if (auto gEnv = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::GainMode))
	{
		auto v0 = gEnv->getUptimeValue(voiceUptime);
		auto v1 = gEnv->getUptimeValue(wrappedVoice.voiceUptime);

		if (std::abs(v0 - v1) < 0.001f)
		{
			envGain = v0;
		}
		else
		{
			voiceBuffer.applyGainRamp(startSample, samplesInBlock, v0, v1);
		}
	}

	voiceUptime = wrappedVoice.voiceUptime;
	
	if (!wrappedVoice.isActive)
	{
		resetVoice();
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesInBlock);

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesInBlock);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesInBlock);
	}

	if (auto crossFadeValues = getCrossfadeModulationValues(startSample, numSamples))
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), crossFadeValues + startIndex, samplesInBlock);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), crossFadeValues + startIndex, samplesInBlock);

		jassert(getConstantCrossfadeModulationValue() == 1.0f);
	}
	
	float totalGain = getOwnerSynth()->getConstantGainModValue() * envGain;
	
	float thisCrossfadeGain = getConstantCrossfadeModulationValue();

	totalGain *= thisCrossfadeGain;

	totalGain *= currentlyPlayingSamplerSound->getPropertyVolume();
	totalGain *= currentlyPlayingSamplerSound->getNormalizedPeak();
	totalGain *= velocityXFadeValue;
	
	const float lGain = totalGain * currentlyPlayingSamplerSound->getBalance(false);
	const float rGain = totalGain * currentlyPlayingSamplerSound->getBalance(true);
	
	if (lGain != 1.0f) FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), lGain, samplesInBlock);
	if (rGain != 1.0f) FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), rGain, samplesInBlock);

	if (auto fEnv = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::PanMode))
	{
		if (auto env = static_cast<ModulatorSampler*>(getOwnerSynth())->getEnvelopeFilter())
		{
			auto fValue = fEnv->getUptimeValue(oldUptime);
			snex::Types::PolyHandler::ScopedVoiceSetter svs(env->polyManager, getVoiceIndex());
			env->process(fValue, voiceBuffer, startIndex, samplesInBlock);
		}
		else
			jassertfalse;
	}

	if (sampler->isLastStartedVoice(this))
	{
		handlePlaybackPosition(sound);
	}
}



void ModulatorSamplerVoice::handlePlaybackPosition(const StreamingSamplerSound * sound)
{
    if(sound == nullptr) return;
    
	double normPos = 0.0;

	if (sound->isLoopEnabled() && sound->getLoopLength() != 0)
	{
		int samplePosition = (int)voiceUptime;

		if (sound->isReversed())
		{
			if (samplePosition > sound->getSampleEnd() - sound->getLoopStart())
			{
				auto offset = sound->getSampleEnd() - sound->getLoopEnd();
				samplePosition = hmath::wrap(samplePosition - offset, sound->getLoopLength()) + offset;
			}
		}
		else
		{
			auto sampleStart = sound->getSampleStart();

			if (samplePosition + sampleStart > sound->getLoopEnd())
			{
				auto ls = sound->getLoopStart() - sampleStart;
				samplePosition = hmath::wrap(samplePosition - ls, sound->getLoopLength()) + ls;
			}
		}

		normPos = (double)samplePosition / (double)sound->getSampleLength();
	}
	else
	{
		normPos = voiceUptime / (double)sound->getSampleLength();
	
	}

	if (sound->isReversed())
		normPos = 1.0 - normPos;
	

	sampler->setCurrentPlayingPosition(normPos);
}

double ModulatorSamplerVoice::limitPitchDataToMaxSamplerPitch(float * pitchData, double uptimeDelta, int startSample, int numSamples)
{
	double pitchCounter = 0.0;

	if (pitchData == nullptr) pitchCounter = uptimeDelta * (double)numSamples;
	else
	{
		const float uptimeDeltaFloat = (float)uptimeDelta;

		pitchCounter = 0.0;
		pitchData += startSample;

		FloatVectorOperations::multiply(pitchData, uptimeDeltaFloat, numSamples);

#if USE_IPP
		float pitchSum = 0.0f;
		ippsThreshold_32f_I(pitchData, numSamples, (float)MAX_SAMPLER_PITCH, ippCmpGreater);
		ippsSum_32f(pitchData, numSamples, &pitchSum, ippAlgHintAccurate);
		
		pitchCounter = (double)pitchSum;

#else
		for (int i = 0; i < numSamples; i++)
		{
			pitchCounter += jmin<double>((double)MAX_SAMPLER_PITCH, (double)*pitchData++);
		}
		
#endif			
}

	return pitchCounter;
}

void ModulatorSamplerVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynthVoice::prepareToPlay(sampleRate, samplesPerBlock);

	wrappedVoice.prepareToPlay(sampleRate, samplesPerBlock);
}

void ModulatorSamplerVoice::setLoaderBufferSize(int newBufferSize)
{
	wrappedVoice.setLoaderBufferSize(newBufferSize);	
}

double ModulatorSamplerVoice::getDiskUsage()
{
	return wrappedVoice.getDiskUsage();
}

size_t ModulatorSamplerVoice::getStreamingBufferSize() const
{
	return wrappedVoice.loader.getActualStreamingBufferSize();
}



void ModulatorSamplerVoice::setStreamingBufferDataType(bool shouldBeFloat)
{
	wrappedVoice.loader.setStreamingBufferDataType(shouldBeFloat);
}

float ModulatorSamplerVoice::getConstantCrossfadeModulationValue() const noexcept
{
	return sampler->getConstantCrossFadeModulationValue();
}

const float * ModulatorSamplerVoice::getCrossfadeModulationValues(int startSample, int numSamples)
{
	if (!sampler->isUsingCrossfadeGroups())
		return nullptr;

	return sampler->calculateCrossfadeModulationValuesForVoice(voiceIndex, startSample, numSamples, currentlyPlayingSamplerSound->getRRGroup() - 1);
}



void ModulatorSamplerVoice::resetVoice()
{
	sampler->resetNoteDisplay(this->getCurrentlyPlayingNote() + getTransposeAmount());

	wrappedVoice.resetVoice();

	ModulatorSynthVoice::resetVoice();

};



ModulatorSamplerVoice::ModulatorSamplerVoice(ModulatorSynth *ownerSynth) :
ModulatorSynthVoice(ownerSynth),
sampleStartModValue(0.0f),
velocityXFadeValue(1.0f),
sampler(dynamic_cast<ModulatorSampler*>(ownerSynth)),
wrappedVoice(sampler->getBackgroundThreadPool())
{
	auto ms = static_cast<ModulatorSampler*>(ownerSynth);

	wrappedVoice.setTemporaryVoiceBuffer(ms->getTemporaryVoiceBuffer(), ms->getTemporaryStretchBuffer());
	
	wrappedVoice.setDebugLogger(&ownerSynth->getMainController()->getDebugLogger());
	
};



MultiMicModulatorSamplerVoice::MultiMicModulatorSamplerVoice(ModulatorSynth *ownerSynth, int numMultiMics):
ModulatorSamplerVoice(ownerSynth)
{
	wrappedVoices.clear();

	auto ms = static_cast<ModulatorSampler*>(ownerSynth);

	for (int i = 0; i < numMultiMics; i++)
	{
		wrappedVoices.add(new StreamingSamplerVoice(getOwnerSynth()->getMainController()->getSampleManager().getGlobalSampleThreadPool()));
		wrappedVoices.getLast()->prepareToPlay(getOwnerSynth()->getSampleRate(), getOwnerSynth()->getLargestBlockSize());
		wrappedVoices.getLast()->setLoaderBufferSize((int)getOwnerSynth()->getAttribute(ModulatorSampler::BufferSize));
		wrappedVoices.getLast()->setTemporaryVoiceBuffer(ms->getTemporaryVoiceBuffer(), ms->getTemporaryStretchBuffer());
		wrappedVoices.getLast()->setDebugLogger(&ownerSynth->getMainController()->getDebugLogger());
	}
}

void MultiMicModulatorSamplerVoice::startVoiceInternal(int midiNoteNumber, float velocity)
{
	auto sampler = static_cast<ModulatorSampler*>(getOwnerSynth());
	auto startMod = calculateSampleStartMod();
	auto sound = currentlyPlayingSamplerSound->getReferenceToSound();

	const bool samePitch = !sampler->isPitchTrackingEnabled();
	const int rootNote = samePitch ? midiNoteNumber : currentlyPlayingSamplerSound->getRootNote();
	const double globalPitchFactor = getOwnerSynth()->getMainController()->getGlobalPitchFactor();

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		auto micSound = currentlyPlayingSamplerSound->getReferenceToSound(i);

		if (micSound == nullptr)
		{
			jassertfalse;
			continue;
		}

		if (!micSound->hasActiveState()) continue;

		StreamingSamplerVoice *voiceToUse = wrappedVoices[i];

		voiceToUse->setPitchFactor(midiNoteNumber, rootNote, micSound.get(), globalPitchFactor);
		voiceToUse->setSampleStartModValue(startMod);
		voiceToUse->startNote(midiNoteNumber, velocity, micSound.get(), -1);

		voiceUptime = wrappedVoices[i]->voiceUptime;
		uptimeDelta = wrappedVoices[i]->uptimeDelta;
		isActive = true;
	}
}

void MultiMicModulatorSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	jassert(s != nullptr);

	midiNoteNumber += transposeAmount;

	

	currentlyPlayingSamplerSound = static_cast<ModulatorSamplerSound*>(s);

	velocityXFadeValue = currentlyPlayingSamplerSound->getGainValueForVelocityXFade((int)(velocity * 127.0f));
	
	if (playFromPurger != nullptr &&
		currentlyPlayingSamplerSound->hasUnpurgedButUnloadedSounds())
	{
		// We need to notify the sample thread to preload this sample now...
		playFromPurger->notifyStart(midiNoteNumber, velocity);
	}
	else
	{
		startVoiceInternal(midiNoteNumber, velocity);
	}
}

void MultiMicModulatorSamplerVoice::calculateBlock(int startSample, int numSamples)
{
	ADD_GLITCH_DETECTOR(getOwnerSynth(), DebugLogger::Location::MultiMicSampleRendering);

	if (waitForPlayFromPurge.load())
	{
		voiceBuffer.clear(startSample, numSamples);
		return;
	}

	const int startIndex = startSample;
	const int samplesInBlock = numSamples;

	auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	double propertyPitch = (float)currentlyPlayingSamplerSound->getPropertyPitch();

	if (auto env = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::PitchMode))
	{
		propertyPitch *= env->getUptimeValue(voiceUptime);
	}

	const double pitchCounter = limitPitchDataToMaxSamplerPitch(voicePitchValues, uptimeDelta * propertyPitch, startSample, numSamples);

	auto oldUptime = voiceUptime;

	voiceBuffer.clear();

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		const StreamingSamplerSound *sound = wrappedVoices[i]->getLoadedSound();

		if (sound == nullptr) continue;

		wrappedVoices[i]->setPitchValues(voicePitchValues);
		wrappedVoices[i]->setPitchCounterForThisBlock(pitchCounter);
		wrappedVoices[i]->uptimeDelta = uptimeDelta * propertyPitch;

		float *leftChannel = voiceBuffer.getWritePointer(2*i);
		float *rightChannel = voiceBuffer.getWritePointer(2*i + 1);
		float *channels[2] = { leftChannel, rightChannel };

		AudioSampleBuffer channelBuffer(channels, 2, voiceBuffer.getNumSamples());

		wrappedVoices[i]->renderNextBlock(channelBuffer, startSample, numSamples);

		voiceUptime = wrappedVoices[i]->voiceUptime;

		if (!wrappedVoices[i]->isActive)
		{
			resetVoice();
		}
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesInBlock);
	
	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		for (int i = 0; i < wrappedVoices.size(); i++)
		{
			if (wrappedVoices[i]->getLoadedSound() == nullptr) continue;

			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i, startIndex), modValues + startIndex, samplesInBlock);
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i + 1, startIndex), modValues + startIndex, samplesInBlock);
		}
	}

	if (auto crossFadeValues = getCrossfadeModulationValues(startSample, numSamples))
	{
		for (int i = 0; i < wrappedVoices.size(); i++)
		{
			if (wrappedVoices[i]->getLoadedSound() == nullptr) continue;

			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i, startIndex), crossFadeValues + startIndex, samplesInBlock);
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i + 1, startIndex), crossFadeValues + startIndex, samplesInBlock);
		}

		jassert(getConstantCrossfadeModulationValue() == 1.0f);
	}

	float totalGain = getOwnerSynth()->getConstantGainModValue();
	float thisCrossfadeGain = getConstantCrossfadeModulationValue();

	totalGain *= thisCrossfadeGain;
	totalGain *= currentlyPlayingSamplerSound->getPropertyVolume();
	totalGain *= currentlyPlayingSamplerSound->getNormalizedPeak();
	totalGain *= velocityXFadeValue;

	

	if (auto gEnv = currentlyPlayingSamplerSound->getEnvelope(Modulation::Mode::GainMode))
	{
		auto v0 = gEnv->getUptimeValue(oldUptime);
		auto v1 = gEnv->getUptimeValue(voiceUptime);

		if (std::abs(v0 - v1) < 0.001f)
		{
			totalGain *= v0;
		}
		else
		{
			voiceBuffer.applyGainRamp(startSample, samplesInBlock, v0, v1);
		}
	}

	const float lGain = totalGain * currentlyPlayingSamplerSound->getBalance(false);
	const float rGain = totalGain * currentlyPlayingSamplerSound->getBalance(true);

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		if (wrappedVoices[i]->getLoadedSound() == nullptr) continue;

		if (lGain != 1.0f)
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i, startIndex), lGain, samplesInBlock);
		
		if (rGain != 1.0f)
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(2 * i + 1, startIndex), rGain, samplesInBlock);
	}

	if (sampler->isLastStartedVoice(this))
	{
		if (wrappedVoices.size() != 0 && wrappedVoices[0]->getLoadedSound() != nullptr)
		{
			handlePlaybackPosition(wrappedVoices[0]->getLoadedSound());
		}
	}
}

void MultiMicModulatorSamplerVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynthVoice::prepareToPlay(sampleRate, samplesPerBlock);

	voiceBuffer.setSize(wrappedVoices.size() * 2, samplesPerBlock);

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		wrappedVoices[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

void MultiMicModulatorSamplerVoice::setLoaderBufferSize(int newBufferSize)
{
	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		wrappedVoices[i]->setLoaderBufferSize(newBufferSize);
	}
}

double MultiMicModulatorSamplerVoice::getDiskUsage()
{
	double diskUsage = 0.0;

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		diskUsage += wrappedVoices[i]->getDiskUsage();
	}

	return diskUsage;
}

size_t MultiMicModulatorSamplerVoice::getStreamingBufferSize() const
{
	size_t size = 0;

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		size += wrappedVoices[i]->loader.getActualStreamingBufferSize();
	}

	return size;
}

void MultiMicModulatorSamplerVoice::setStreamingBufferDataType(bool shouldBeFloat)
{
	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		wrappedVoices[i]->loader.setStreamingBufferDataType(shouldBeFloat);
	}
}

void MultiMicModulatorSamplerVoice::resetVoice()
{
	sampler->resetNoteDisplay(this->getCurrentlyPlayingNote());

	for (int i = 0; i < wrappedVoices.size(); i++)
	{
		wrappedVoices[i]->resetVoice();
	}

	ModulatorSynthVoice::resetVoice();
}



ModulatorSamplerVoice::PlayFromPurger::PlayFromPurger(ModulatorSamplerVoice* voiceToPreload) :
	Job("Play from purge"),
	v(*voiceToPreload)
{

}

void ModulatorSamplerVoice::PlayFromPurger::notifyStart(int n, float ve)
{
	number = n;
	velocity = ve;

	v.waitForPlayFromPurge.store(true);

	// in non realtime mode we want to force the preloading at this point.
	if (v.nonRealtime)
		runJob();
	else
		v.getOwnerSynth()->getMainController()->getSampleManager().getGlobalSampleThreadPool()->addJob(this, false);
}

hise::SampleThreadPool::Job::JobStatus ModulatorSamplerVoice::PlayFromPurger::runJob()
{
	auto sampler = static_cast<ModulatorSampler*>(v.getOwnerSynth());
	auto sound = v.getCurrentlyPlayingSamplerSound();
	auto preloadSize = sampler->getAttribute(ModulatorSampler::Parameters::PreloadSize);
	
	if(preloadSize != -1)
		preloadSize *= sampler->getPreloadScaleFactor();

	for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
	{
		if (shouldExit())
			return SampleThreadPool::Job::jobNeedsRunningAgain;

		sound->getReferenceToSound(i)->setPreloadSize(preloadSize, false);
	}
	
	v.startVoiceInternal(number, velocity);

	v.saveStartUptimeDelta();

	sampler->refreshMemoryUsage(true);

	v.waitForPlayFromPurge.store(false);

	return SampleThreadPool::Job::jobHasFinished;
}

} // namespace hise
