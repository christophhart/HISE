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

EnvelopeFollower::MagnitudeRamp::MagnitudeRamp() :
indexInBufferedArray(0),
currentPeak(0.0f),
rampedValue(0.0f)
{

}

void EnvelopeFollower::MagnitudeRamp::setRampLength(int newRampLength)
{
	rampBuffer.setSize(1, newRampLength, true, false, true);
	size = newRampLength;
	bufferRamper.setValue(0.0f);
	rampedValue = 0.0;
	indexInBufferedArray = 0;
}

float EnvelopeFollower::MagnitudeRamp::getEnvelopeValue(float inputValue)
{
	if (indexInBufferedArray < size)
	{
		rampBuffer.setSample(0, indexInBufferedArray++, inputValue);
	}
	else if (indexInBufferedArray == size)
	{
		indexInBufferedArray = 0;
		bufferRamper.setTarget(rampedValue, rampBuffer.getMagnitude(0, size), size);
	}
	else
	{
		jassertfalse;
	}

	bufferRamper.ramp(rampedValue);

	return rampedValue;
}

AudioFileEnvelope::AudioFileEnvelope(MainController *mc, const String &id, Modulation::Mode m) :
	TimeVariantModulator(mc, id, m),
	Modulation(m),
	AudioSampleProcessor(this),
	gain(1.0),
	currentValue(1.0f),
	uptime(0.0),
	offset(0.0f),
	keysPressed(0),
	intensityModulationValue(1.0f),
	frequencyModulationValue(1.0f),
	frequencyChain(new ModulatorChain(mc, "Frequency Modulation", 1, Modulation::PitchMode, this)),
	intensityChain(new ModulatorChain(mc, "Intensity Modulation", 1, m, this)),
    intensityBuffer(1, 0),
    frequencyBuffer(1, 0),
	legato(false),
	mode(SimpleLP),
	smoothingTime(0.0f),
	syncMode(SyncToHostMode::FreeRunning),
	syncFactor(1.0),
	resampleFactor(1.0),
	attackReleaseEnvelopeFollower(EnvelopeFollower::AttackRelease(50.0, 50.0))
{
	

	editorStateIdentifiers.add("IntensityChainShown");
	editorStateIdentifiers.add("FrequencyChainShown");

	parameterNames.add(Identifier("Legato"));
	parameterNames.add(Identifier("SmoothTime"));
	parameterNames.add(Identifier("Mode"));
	parameterNames.add(Identifier("AttackTime"));
	parameterNames.add(Identifier("ReleaseTime"));
	parameterNames.add(Identifier("Gain"));
	parameterNames.add(Identifier("Offset"));

	setAttribute(RampLength, 1024, dontSendNotification);

	frequencyUpdater.setManualCountLimit(4096);

	CHECK_COPY_AND_RETURN_8(this);

};

AudioFileEnvelope::~AudioFileEnvelope()
{
	
};


void AudioFileEnvelope::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);
	AudioSampleProcessor::restoreFromValueTree(v);

	loadAttribute(Legato, "Legato");
	loadAttribute(SmoothTime, "SmoothTime");
	loadAttribute(Mode, "Mode");
	loadAttribute(AttackTime, "AttackTime");
	loadAttribute(ReleaseTime, "ReleaseTime");
	loadAttribute(Gain, "Gain");
	loadAttribute(Offset, "Offset");
	loadAttribute(RampLength, "RampLength");
	loadAttribute(SyncMode, "SyncMode");
}

ValueTree AudioFileEnvelope::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveAttribute(Legato, "Legato");
	saveAttribute(SmoothTime, "SmoothTime");
	saveAttribute(Mode, "Mode");
	saveAttribute(AttackTime, "AttackTime");
	saveAttribute(ReleaseTime, "ReleaseTime");
	saveAttribute(Gain, "Gain");
	saveAttribute(Offset, "Offset");
	saveAttribute(RampLength, "RampLength");
	saveAttribute(SyncMode, "SyncMode");

	AudioSampleProcessor::saveToValueTree(v);

	return v;
}

Processor * AudioFileEnvelope::getChildProcessor(int i)
{
	switch (i)
	{
	case IntensityChain:	return intensityChain;
	case FrequencyChain:	return frequencyChain;
	default:				jassertfalse; return nullptr;
	}
}

const Processor * AudioFileEnvelope::getChildProcessor(int i) const
{
	switch (i)
	{
	case IntensityChain:	return intensityChain;
	case FrequencyChain:	return frequencyChain;
	default:				jassertfalse; return nullptr;
	}
}


ProcessorEditorBody *AudioFileEnvelope::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new AudioFileEnvelopeEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};

void AudioFileEnvelope::rangeUpdated()
{
	uptime = 0.0;
	resampleFactor = getSampleRateForLoadedFile() / getSampleRate();
	peakInRange = getSampleBuffer()->getMagnitude(sampleRange.getStart(), length);
}

float AudioFileEnvelope::getAttribute(int parameter_index) const
{
	switch(parameter_index)
	{
	case Parameters::Legato: return legato ? 1.0f : 0.0f;
		
	case Parameters::Mode:			return (float)mode;
	case Parameters::AttackTime:	return attackReleaseEnvelopeFollower.getAttack();
	case Parameters::ReleaseTime:	return attackReleaseEnvelopeFollower.getRelease();
	case Parameters::Gain:			return gain;
	case Parameters::RampLength:	return (float)magnitudeRampEnvelopeFollower.size;
	case Parameters::Offset:		return offset;
	case Parameters::SmoothTime:	return smoother.getSmoothingTime();
	case Parameters::SyncMode:		return (float)(int)syncMode;
	default:						jassertfalse;
									return -1.0f;
	}
};


void AudioFileEnvelope::applyTimeModulation(AudioSampleBuffer &buffer, int startIndex, int samplesToCopy)
{
	float *dest = buffer.getWritePointer(0, startIndex);
	float *mod = internalBuffer.getWritePointer(0, startIndex);

	intensityChain->renderAllModulatorsAsMonophonic(intensityBuffer, startIndex, samplesToCopy);
	
	frequencyChain->renderAllModulatorsAsMonophonic(frequencyBuffer, startIndex, samplesToCopy);

	if(frequencyUpdater.shouldUpdate(samplesToCopy))
	{
		frequencyModulationValue = frequencyBuffer.getReadPointer(0, startIndex)[0];
		
	}

	float *intens = intensityBuffer.getWritePointer(0, startIndex);
	FloatVectorOperations::multiply(intens, getIntensity(), samplesToCopy);

	if(getMode() == GainMode)		TimeModulation::applyGainModulation( mod, dest, getIntensity(), intens, samplesToCopy);
	else if(getMode() == PitchMode) TimeModulation::applyPitchModulation(mod, dest, getIntensity(), intens, samplesToCopy);

};

float AudioFileEnvelope::getIntensity() const noexcept
{
	switch (getMode())
	{
	case Modulation::GainMode:		return intensityModulationValue * Modulation::getIntensity();
	case Modulation::PitchMode:		return (Modulation::getIntensity() - 1.0f)*intensityModulationValue + 1.0f;
	default:						jassertfalse; return -1.0f;
	}
}

void AudioFileEnvelope::setInternalAttribute(int parameter_index, float newValue)
{
	switch(parameter_index)
	{
	case Parameters::Legato:
		legato = newValue >= 0.5f;
		break;
	case Parameters::AttackTime:
		attackReleaseEnvelopeFollower.setAttack(newValue);
		break;
	case Parameters::ReleaseTime:
		attackReleaseEnvelopeFollower.setRelease(newValue);
		break;
	case Parameters::Gain:
		gain = newValue;
		break;
	case Parameters::Offset:
		offset = newValue;
		break;
	case Parameters::RampLength:
		magnitudeRampEnvelopeFollower.setRampLength((int)newValue);
		break;
	case Parameters::Mode:
		mode = (EnvelopeFollowerMode)(int)newValue;
		CHECK_COPY_AND_RETURN_20(this);
		attackReleaseEnvelopeFollower.setSampleRate(getSampleRate()); // update the coefficients
		break;
	case Parameters::SmoothTime:
		smoothingTime = newValue;
		smoother.setSmoothingTime(smoothingTime);
		break;
	case Parameters::SyncMode:
		setSyncMode((int)newValue);
		break;
	default: 
		jassertfalse;
	}
};


float AudioFileEnvelope::calculateNewValue ()
{
	if(length == 0 || getBuffer() == nullptr) return 1.0f;

	uptime += frequencyModulationValue * syncFactor * resampleFactor;
	const int samplePos = (int)uptime % length + sampleRange.getStart();
	jassert(samplePos < getSampleBuffer()->getNumSamples());


	float sample = getSampleBuffer()->getSample(0, samplePos);
	sample = EnvelopeFollower::prepareAudioInput(sample, peakInRange);
		
	float envelopeValue = 1.0f;

	switch(mode)
	{
	case SimpleLP:	envelopeValue = sample;
					break;
	case RampedAverage:	envelopeValue = magnitudeRampEnvelopeFollower.getEnvelopeValue(sample);
					break;
	case AttackRelease:	
					envelopeValue = attackReleaseEnvelopeFollower.calculateValue(sample);
					break;
	};
	
	currentValue = smoother.smooth(envelopeValue);
	currentValue = EnvelopeFollower::constrainTo0To1((currentValue + offset)*gain);

	return currentValue;
}

void AudioFileEnvelope::setSyncMode(int newSyncMode)
{
	syncMode = (SyncToHostMode)newSyncMode;

	const double globalBpm = getMainController()->getBpm();

	const bool tempoOK = globalBpm > 0.0 && globalBpm < 1000.0;

	const bool sampleRateOK = getSampleRate() != -1.0;

	const bool lengthOK = length != 0;

	if (!tempoOK || !sampleRateOK || !lengthOK)
	{
		syncFactor = 1.0;
		return;
	}

	int multiplier = 1;

	switch (syncMode)
	{
	case SyncToHostMode::FreeRunning:		syncFactor = 1.0; return;
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
		syncFactor = 1.0;
	}
	else
	{
		syncFactor = (double)length / (double)(lengthForOneBeat * multiplier);
	}
}

void AudioFileEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedLock sl(getFileLock());

	Processor::prepareToPlay(sampleRate, samplesPerBlock);

	TimeModulation::prepareToModulate(sampleRate, samplesPerBlock);
	
	if(sampleRate != -1.0)
	{
		CHECK_COPY_AND_RETURN_24(this);

		ProcessorHelpers::increaseBufferIfNeeded(intensityBuffer, samplesPerBlock * 2);
		ProcessorHelpers::increaseBufferIfNeeded(frequencyBuffer, samplesPerBlock * 2);

		intensityChain->prepareToPlay(sampleRate, samplesPerBlock);
		frequencyChain->prepareToPlay(sampleRate, samplesPerBlock);
		
		smoother.prepareToPlay(sampleRate);
		smoother.setSmoothingTime(smoothingTime);

		inputMerger.setManualCountLimit(5);

		attackReleaseEnvelopeFollower.setSampleRate(sampleRate);

		resampleFactor = getSampleRateForLoadedFile() / sampleRate;

		setSyncMode(syncMode);
	}
	// Use the block size to ramp the blocks.
	intensityInterpolator.setStepAmount(samplesPerBlock);
};

void AudioFileEnvelope::calculateBlock(int startSample, int numSamples)
{
	ScopedLock sl(getFileLock());

#if ENABLE_ALL_PEAK_METERS
	if (--numSamples >= 0)
	{
		const float value = calculateNewValue();
		internalBuffer.setSample(0, startSample, value);
		++startSample;
		setOutputValue(value);
	}
#endif

	while (--numSamples >= 0)
	{
		internalBuffer.setSample(0, startSample, calculateNewValue());
		++startSample;
	}



	if (length != 0 && inputMerger.shouldUpdate())
	{
		const float thisInput = (float)((int)uptime % length) / (float)length;
		setInputValue(thisInput, dontSendNotification);
	}
}

void AudioFileEnvelope::handleHiseEvent(const HiseEvent &m)
{
	intensityChain->handleHiseEvent(m);
	frequencyChain->handleHiseEvent(m);
	
	if(m.isNoteOn())
	{
		if(legato == false || keysPressed == 0)
		{
			uptime = 0.0;
			intensityChain->startVoice(0);
			frequencyChain->startVoice(0);
			frequencyModulationValue = frequencyChain->getConstantVoiceValue(0);
		}

		keysPressed++;

	}

	if(m.isNoteOff())
	{
		keysPressed--;

		if(keysPressed < 0) keysPressed = 0;

		if(legato == false || keysPressed == 0)
		{
			intensityChain->stopVoice(0);
			frequencyChain->stopVoice(0);
		}

	}

}

EnvelopeFollower::AttackRelease::AttackRelease(float attackTime, float releaseTime) :
attack(attackTime),
release(releaseTime),
sampleRate(-1.0),
attackCoefficient(-1.0f),
releaseCoefficient(-1.0f),
lastValue(0.0f)
{

}

float EnvelopeFollower::AttackRelease::calculateValue(float input)
{
	jassert(sampleRate != -1);
	jassert(attackCoefficient != -1);
	jassert(releaseCoefficient != -1);

	lastValue = ((input > lastValue) ? attackCoefficient : releaseCoefficient)* (lastValue - input) + input;

	return lastValue;
}

void EnvelopeFollower::AttackRelease::setSampleRate(double sampleRate_)
{
	sampleRate = sampleRate_;
	calculateCoefficients();
}

void EnvelopeFollower::AttackRelease::setRelease(float newRelease)
{
	attack = newRelease;
	calculateCoefficients();
}

void EnvelopeFollower::AttackRelease::calculateCoefficients()
{
	if (sampleRate != -1.0f)
	{
		attackCoefficient = expf(logf(0.01f) / (attack * (float)sampleRate * 0.001f));
		releaseCoefficient = expf(logf(0.01f) / (release * (float)sampleRate * 0.001f));
	}
}
} // namespace hise
