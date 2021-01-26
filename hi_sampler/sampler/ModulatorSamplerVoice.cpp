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
	const bool samePitch = !static_cast<ModulatorSampler*>(getOwnerSynth())->isPitchTrackingEnabled();

	auto sound = currentlyPlayingSamplerSound->getReferenceToSound();

	int sampleStartModulationDelta;

	// if value is >= 0, then it comes from the modulator chain value
	const bool sampleStartModIsFromChain = sampleStartModValue >= 0.0f;

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

	wrappedVoice.setPitchFactor(midiNoteNumber, samePitch ? midiNoteNumber : currentlyPlayingSamplerSound->getRootNote(), sound, getOwnerSynth()->getMainController()->getGlobalPitchFactor());
	wrappedVoice.setSampleStartModValue(sampleStartModulationDelta);
	wrappedVoice.startNote(midiNoteNumber, velocity, sound, -1);

	voiceUptime = wrappedVoice.voiceUptime;
	uptimeDelta = wrappedVoice.uptimeDelta;
    isActive = true;

	jassert(uptimeDelta > 0.0);
	jassert(sound->isEntireSampleLoaded() || uptimeDelta <= MAX_SAMPLER_PITCH);
}

void ModulatorSamplerVoice::stopNote(float velocity, bool allowTailoff)
{
	ModulatorSynthVoice::stopNote(velocity, allowTailoff);
}

void ModulatorSamplerVoice::calculateBlock(int startSample, int numSamples)
{
    const StreamingSamplerSound *sound = wrappedVoice.getLoadedSound();
    
	// In a synthgroup it might be possible that the wrapped sound is null
	jassert(sound != nullptr || getOwnerSynth()->isInGroup());
 
	CHECK_AND_LOG_ASSERTION(getOwnerSynth(), DebugLogger::Location::SampleRendering, sound != nullptr, 1);

	ADD_GLITCH_DETECTOR(getOwnerSynth(), DebugLogger::Location::SampleRendering);
 
	ignoreUnused(sound);

	const int startIndex = startSample;
	const int samplesInBlock = numSamples;

	auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	const double propertyPitch = currentlyPlayingSamplerSound->getPropertyPitch();
	
	applyConstantPitchFactor(propertyPitch);

	const double pitchCounter = limitPitchDataToMaxSamplerPitch(voicePitchValues, uptimeDelta, startSample, numSamples);

	wrappedVoice.setPitchCounterForThisBlock(pitchCounter);
	wrappedVoice.setPitchValues(voicePitchValues);

	

	wrappedVoice.uptimeDelta = uptimeDelta;

	voiceBuffer.clear();

	wrappedVoice.renderNextBlock(voiceBuffer, startSample, numSamples);

	CHECK_AND_LOG_BUFFER_DATA(getOwnerSynth(), DebugLogger::Location::SampleRendering, voiceBuffer.getReadPointer(0, startSample), true, samplesInBlock);
	CHECK_AND_LOG_BUFFER_DATA(getOwnerSynth(), DebugLogger::Location::SampleRendering, voiceBuffer.getReadPointer(1, startSample), false, samplesInBlock);

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
	
	float totalGain = getOwnerSynth()->getConstantGainModValue();
	
	float thisCrossfadeGain = getConstantCrossfadeModulationValue();

	totalGain *= thisCrossfadeGain;

	totalGain *= currentlyPlayingSamplerSound->getPropertyVolume();
	totalGain *= currentlyPlayingSamplerSound->getNormalizedPeak();
	totalGain *= velocityXFadeValue;
	
	const float lGain = totalGain * currentlyPlayingSamplerSound->getBalance(false);
	const float rGain = totalGain * currentlyPlayingSamplerSound->getBalance(true);
	
	if (lGain != 1.0f) FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), lGain, samplesInBlock);
	if (rGain != 1.0f) FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), rGain, samplesInBlock);

	

#if USE_BACKEND
	if (sampler->isLastStartedVoice(this))
	{
		handlePlaybackPosition(sound);
	}
#endif
}

void ModulatorSamplerVoice::handlePlaybackPosition(const StreamingSamplerSound * sound)
{
    if(sound == nullptr) return;
    
	if (sound->isLoopEnabled() && sound->getLoopLength() != 0)
	{
		int samplePosition = (int)voiceUptime;

		if (samplePosition + sound->getSampleStart() > sound->getLoopEnd())
		{
			samplePosition = (int)(((int64)samplePosition % sound->getLoopLength()) + sound->getLoopStart() - sound->getSampleStart());
		}

		sampler->setCurrentPlayingPosition((double)samplePosition / (double)sound->getSampleLength());
	}
	else
	{
		const double normalizedPosition = voiceUptime / (double)sound->getSampleLength();
		sampler->setCurrentPlayingPosition(normalizedPosition);
	}
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
	wrappedVoice.setTemporaryVoiceBuffer(static_cast<ModulatorSampler*>(ownerSynth)->getTemporaryVoiceBuffer());
	
	wrappedVoice.setDebugLogger(&ownerSynth->getMainController()->getDebugLogger());
};



MultiMicModulatorSamplerVoice::MultiMicModulatorSamplerVoice(ModulatorSynth *ownerSynth, int numMultiMics):
ModulatorSamplerVoice(ownerSynth)
{
	wrappedVoices.clear();

	for (int i = 0; i < numMultiMics; i++)
	{
		wrappedVoices.add(new StreamingSamplerVoice(getOwnerSynth()->getMainController()->getSampleManager().getGlobalSampleThreadPool()));
		wrappedVoices.getLast()->prepareToPlay(getOwnerSynth()->getSampleRate(), getOwnerSynth()->getLargestBlockSize());
		wrappedVoices.getLast()->setLoaderBufferSize((int)getOwnerSynth()->getAttribute(ModulatorSampler::BufferSize));
		wrappedVoices.getLast()->setTemporaryVoiceBuffer(static_cast<ModulatorSampler*>(ownerSynth)->getTemporaryVoiceBuffer());
		wrappedVoices.getLast()->setDebugLogger(&ownerSynth->getMainController()->getDebugLogger());
	}
}

void MultiMicModulatorSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	jassert(s != nullptr);

	midiNoteNumber += transposeAmount;

	currentlyPlayingSamplerSound = static_cast<ModulatorSamplerSound*>(s);

	velocityXFadeValue = currentlyPlayingSamplerSound->getGainValueForVelocityXFade((int)(velocity * 127.0f));
	const bool samePitch = !sampler->isPitchTrackingEnabled();

	const int rootNote = samePitch ? midiNoteNumber : currentlyPlayingSamplerSound->getRootNote();
	const double globalPitchFactor = getOwnerSynth()->getMainController()->getGlobalPitchFactor();
    

	int sampleStartModulationDelta;

	auto sound = currentlyPlayingSamplerSound->getReferenceToSound();

	// if value is >= 0, then it comes from the modulator chain value
	const bool sampleStartModIsFromChain = sampleStartModValue >= 0.0f;

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

		voiceToUse->setPitchFactor(midiNoteNumber, rootNote, micSound, globalPitchFactor);
		voiceToUse->setSampleStartModValue(sampleStartModulationDelta);
		voiceToUse->startNote(midiNoteNumber, velocity, micSound, -1);

		voiceUptime = wrappedVoices[i]->voiceUptime;
		uptimeDelta = wrappedVoices[i]->uptimeDelta;
        isActive = true;
	}
}

void MultiMicModulatorSamplerVoice::calculateBlock(int startSample, int numSamples)
{
	ADD_GLITCH_DETECTOR(getOwnerSynth(), DebugLogger::Location::MultiMicSampleRendering);

	const int startIndex = startSample;
	const int samplesInBlock = numSamples;

	auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	const double propertyPitch = (float)currentlyPlayingSamplerSound->getPropertyPitch();
	const double pitchCounter = limitPitchDataToMaxSamplerPitch(voicePitchValues, uptimeDelta * propertyPitch, startSample, numSamples);

	

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

} // namespace hise