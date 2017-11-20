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

AudioLooperVoice::AudioLooperVoice(ModulatorSynth *ownerSynth) :
ModulatorSynthVoice(ownerSynth),
syncFactor(1.0f)
{
}

void AudioLooperVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();

	voiceUptime = (double)getCurrentHiseEvent().getStartOffset();

	AudioLooper *looper = dynamic_cast<AudioLooper*>(getOwnerSynth());

	const AudioSampleBuffer *buffer = looper->getBuffer();

	uptimeDelta = buffer != nullptr ? 1.0 : 0.0;

	const double resampleFactor = looper->getSampleRateForLoadedFile() / getSampleRate();

	uptimeDelta *= resampleFactor;

    uptimeDelta *= looper->getMainController()->getGlobalPitchFactor();
    
	if (looper->pitchTrackingEnabled)
	{
		//const double noteDelta = jlimit<int>(-24, 24, midiNoteNumber - looper->rootNote);

		const double noteDelta = midiNoteNumber - looper->rootNote;

		const double pitchDelta = pow(2, noteDelta / 12.0);

		uptimeDelta *= pitchDelta;
	}
}

void AudioLooperVoice::calculateBlock(int startSample, int numSamples)
{
	

	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getVoicePitchValues();

	const float *modValues = getVoiceGainValues(startSample, numSamples);

	AudioLooper *looper = dynamic_cast<AudioLooper*>(getOwnerSynth());

	const AudioSampleBuffer *buffer = looper->getBuffer();

	if (buffer == nullptr || buffer->getNumChannels() == 0)
		return;

	const float *leftSamples = buffer->getReadPointer(0);
	const float *rightSamples = buffer->getNumChannels() > 1 ? buffer->getReadPointer(1) : leftSamples;

	if (!looper->loopEnabled && (voiceUptime + numSamples) > looper->length)
	{
        voiceBuffer.clear();
            
        clearCurrentNote();
        voiceUptime = 0.0;
        uptimeDelta = 0.0;
        isActive = false;
        return;
	}
    

	while (--numSamples >= 0)
	{
		const int samplePos = (int)voiceUptime % looper->length + looper->sampleRange.getStart();
		const int nextSamplePos = ((int)voiceUptime + 1) % looper->length + looper->sampleRange.getStart();

        if(!looper->loopEnabled && nextSamplePos > looper->length)
        {
            voiceBuffer.clear();
            
            clearCurrentNote();
            voiceUptime = 0.0;
            uptimeDelta = 0.0;
            isActive = false;
            return;
        }
        
		const double alpha = fmod(voiceUptime, 1.0);

		const float leftPrevSample = leftSamples[samplePos];
		const float rightPrevSample = rightSamples[samplePos];

		const float leftNextSample = leftSamples[nextSamplePos];
		const float rightNextSample = rightSamples[nextSamplePos];

		const float leftSample = Interpolator::interpolateLinear(leftPrevSample, leftNextSample, (float)alpha);
		const float rightSample = Interpolator::interpolateLinear(rightPrevSample, rightNextSample, (float)alpha);

		//const float currentSample = invAlpha * v1 + alpha * v2;

		// Stereo mode assumed
		voiceBuffer.setSample(0, startSample, leftSample);
		voiceBuffer.setSample(1, startSample, rightSample);

		jassert(voicePitchValues == nullptr || voicePitchValues[startSample] > 0.0f);

		
		const double pitchDelta = (uptimeDelta * syncFactor * (voicePitchValues == nullptr ? 1.0f : voicePitchValues[startSample]));
		voiceUptime += pitchDelta;

		++startSample;
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);

	const bool isLastVoice = getOwnerSynth()->isLastStartedVoice(this);

	if (isLastVoice && looper->length != 0 && looper->inputMerger.shouldUpdate())
	{
		const float inputValue = (float)((int)voiceUptime % looper->length) / (float)looper->length;
		looper->setInputValue(inputValue, dontSendNotification);
	}
}

AudioLooper::AudioLooper(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
AudioSampleProcessor(this),
syncMode(AudioSampleProcessor::SyncToHostMode::FreeRunning),
loopEnabled(true),
pitchTrackingEnabled(false),
rootNote(64)
{
	parameterNames.add("SyncMode");
	parameterNames.add("LoopEnabled");
	parameterNames.add("PitchTracking");
	parameterNames.add("RootNote");

	inputMerger.setManualCountLimit(5);

	for (int i = 0; i < numVoices; i++)
	{
		addVoice(new AudioLooperVoice(this));

	}

	addSound(new AudioLooperSound());
}

void AudioLooper::restoreFromValueTree(const ValueTree &v)
{
	ModulatorSynth::restoreFromValueTree(v);
	AudioSampleProcessor::restoreFromValueTree(v);

	loadAttribute(SyncMode, "SyncMode");
	loadAttribute(PitchTracking, "PitchTracking");
	loadAttribute(LoopEnabled, "LoopEnabled");
	loadAttribute(RootNote, "RootNote");
}

ValueTree AudioLooper::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	saveAttribute(SyncMode, "SyncMode");
	saveAttribute(PitchTracking, "PitchTracking");
	saveAttribute(LoopEnabled, "LoopEnabled");
	saveAttribute(RootNote, "RootNote");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

float AudioLooper::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	switch (parameterIndex)
	{
	case SyncMode:		return (float)(int)syncMode;
	case LoopEnabled:	return loopEnabled ? 1.0f : 0.0f;
	case RootNote:		return (float)rootNote;
	case PitchTracking:	return pitchTrackingEnabled ? 1.0f : 0.0f;
	default:					jassertfalse; return -1.0f;
	}
}

float AudioLooper::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getDefaultValue(parameterIndex);

	switch (parameterIndex)
	{
	case SyncMode:		return (float)(int)0;
	case LoopEnabled:	return 1.0f;
	case RootNote:		return 64.0f;
	case PitchTracking:	return 0.0f;
	default: jassertfalse; return -1.0f;
	}
}

void AudioLooper::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case SyncMode:		setSyncMode((int)newValue); break;
	case LoopEnabled:	loopEnabled = newValue > 0.5f; break;
	case RootNote:		rootNote = (int)newValue; break;
	case PitchTracking:	pitchTrackingEnabled = newValue > 0.5f; break;
	default:			jassertfalse; break;
	}
}

void AudioLooper::newFileLoaded()
{
	if (!pitchTrackingEnabled)
		return;

	if (auto b = getBuffer())
	{
		auto freq = PitchDetection::detectPitch(*b, 0, b->getNumSamples(), getSampleRate());
		
		if (freq == 0.0)
			return;

		Array<Range<double>> freqRanges;

		freqRanges.add(Range<double>(0, MidiMessage::getMidiNoteInHertz(1) / 2));

		for (int i = 1; i < 126; i++)
		{
			const double thisPitch = MidiMessage::getMidiNoteInHertz(i);
			const double nextPitch = MidiMessage::getMidiNoteInHertz(i + 1);
			const double prevPitch = MidiMessage::getMidiNoteInHertz(i - 1);

			const double lowerLimit = thisPitch - (thisPitch - prevPitch) * 0.5;
			const double upperLimit = thisPitch + (nextPitch - thisPitch) * 0.5;

			freqRanges.add(Range<double>(lowerLimit, upperLimit));
		}

		for (int j = 0; j < freqRanges.size(); j++)
		{
			if (freqRanges[j].contains(freq))
			{
				setAttribute(AudioLooper::SpecialParameters::RootNote, (float)(j), sendNotification);
				return;
			}
		}
	}

	
	
}

ProcessorEditorBody* AudioLooper::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new AudioLooperEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void AudioLooper::setSyncMode(int newSyncMode)
{
	syncMode = (SyncToHostMode)newSyncMode;

	const double globalBpm = getMainController()->getBpm();

	const bool tempoOK = globalBpm > 0.0 && globalBpm < 1000.0;

	const bool sampleRateOK = getSampleRate() != -1.0;

	const bool lengthOK = length != 0;

	if (!tempoOK || !sampleRateOK || !lengthOK)
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0;
		return;
	}

	int multiplier = 1;

	switch (syncMode)
	{
	case SyncToHostMode::FreeRunning:		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0; return;
	case SyncToHostMode::OneBeat:			multiplier = 1; break;
	case SyncToHostMode::TwoBeats:			multiplier = 2; break;
	case SyncToHostMode::OneBar:			multiplier = 4; break;
	case SyncToHostMode::TwoBars:			multiplier = 8; break;
	case SyncToHostMode::FourBars:			multiplier = 16; break;
	default:								jassertfalse; multiplier = 1;
	}

	const int lengthForOneBeat = TempoSyncer::getTempoInSamples(getMainController()->getBpm(), getSampleRate(), TempoSyncer::Quarter);

	if (lengthForOneBeat == 0)
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = 1.0;
	}
	else
	{
		dynamic_cast<AudioLooperVoice*>(getVoice(0))->syncFactor = (float)length / (float)(lengthForOneBeat * multiplier);
	}

}

} // namespace hise
