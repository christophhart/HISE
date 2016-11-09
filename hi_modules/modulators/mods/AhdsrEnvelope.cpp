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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


Processor * AhdsrEnvelope::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex];
}

const Processor * AhdsrEnvelope::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex];
}

AhdsrEnvelope::AhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
	EnvelopeModulator(mc, id, voiceAmount, m),
	Modulation(m),
	attack(getDefaultValue(Attack)),
	attackLevel(1.0f),
	attackCurve(0.0),
	hold(getDefaultValue(Hold)),
	decay(getDefaultValue(Decay)),
	sustain(1.0f),
	release(getDefaultValue(Release))
{
	parameterNames.add("Attack");
	parameterNames.add("AttackLevel");
	parameterNames.add("Hold");
	parameterNames.add("Decay");
	parameterNames.add("Sustain");
	parameterNames.add("Release");
	parameterNames.add("AttackCurve");
	parameterNames.add("DecayCurve");

	editorStateIdentifiers.add("AttackTimeChainShown");
	editorStateIdentifiers.add("AttackLevelChainShown");
	editorStateIdentifiers.add("DecayTimeChainShown");
	editorStateIdentifiers.add("SustainLevelChainShown");
	editorStateIdentifiers.add("ReleaseTimeChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	internalChains.add(new ModulatorChain(mc, "Attack Time", voiceAmount, ModulatorChain::GainMode, this));
	internalChains.add(new ModulatorChain(mc, "Attack Level", voiceAmount, ModulatorChain::GainMode, this));
	internalChains.add(new ModulatorChain(mc, "Decay Time", voiceAmount, ModulatorChain::GainMode, this));
	internalChains.add(new ModulatorChain(mc, "Sustain Level", voiceAmount, ModulatorChain::GainMode, this));
	internalChains.add(new ModulatorChain(mc, "Release Time", voiceAmount, ModulatorChain::GainMode, this));

	for(int i = 0; i < internalChains.size(); i++)
	{
		internalChains[i]->setIsVoiceStartChain(true);
	};

    setTargetRatioDR(0.0001f);

	setAttackCurve(0.0f);
	setDecayCurve(0.0f);

}


void AhdsrEnvelope::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttributeWithDefault(AttackCurve);
	loadAttributeWithDefault(DecayCurve);
	loadAttribute(Attack, "Attack");
	loadAttribute(AttackLevel, "AttackLevel");
	loadAttribute(Hold, "Hold");
	loadAttribute(Decay, "Decay");
	loadAttribute(Sustain, "Sustain");
	loadAttribute(Release, "Release");
}

ValueTree AhdsrEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(AttackCurve, "AttackCurve");
	saveAttribute(DecayCurve, "DecayCurve");
	saveAttribute(Attack, "Attack");
	saveAttribute(AttackLevel, "AttackLevel");
	saveAttribute(Hold, "Hold");
	saveAttribute(Decay, "Decay");
	saveAttribute(Sustain, "Sustain");
	saveAttribute(Release, "Release");
	
	return v;
}

void AhdsrEnvelope::setAttackRate(float rate) {
	attack = rate;
}

void AhdsrEnvelope::setHoldTime(float holdTimeMs) {
	hold = holdTimeMs;

	holdTimeSamples = holdTimeMs * ((float)getSampleRate() / 1000.0f);
}

void AhdsrEnvelope::setDecayRate(float rate)
{
    decay = rate;
	
    decayCoef = calcCoef(decay, targetRatioDR);
    decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);
}

void AhdsrEnvelope::setReleaseRate(float rate)
{
    
	release = jmax<float>(1.0f, rate);

    releaseCoef = calcCoef(release, targetRatioDR);
    releaseBase = -targetRatioDR * (1.0f - releaseCoef);
}

void AhdsrEnvelope::setSustainLevel(float level)
{
    sustain = level;
    decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);
	
}

void AhdsrEnvelope::setTargetRatioDR(float targetRatio) {
    
	if (targetRatio < 0.0000001f)
        targetRatio = 0.0000001f;
    targetRatioDR = targetRatio;

    decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);
    releaseBase = -targetRatioDR * (1.0f - releaseCoef);
}

void AhdsrEnvelope::startVoice(int voiceIndex)
{
	state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);
	
	if(state->current_state != AhdsrEnvelopeState::IDLE)
	{
		reset(voiceIndex);
	}

	for(int i = 0; i < numInternalChains; i++)
	{
		internalChains[i]->startVoice(voiceIndex);
		state->modValues[i] = jmax(0.0f, internalChains[i]->getConstantVoiceValue(voiceIndex));
	}

	state->attackLevel = attackLevel * state->modValues[AttackLevelChain];
	state->setAttackRate(attack);
	state->setDecayRate(decay);
	state->setReleaseRate(release);
	
	state->current_state = AhdsrEnvelopeState::ATTACK;

	state->current_value = 0.0f;

	state->lastSustainValue = sustain * state->modValues[SustainLevelChain];
}

void AhdsrEnvelope::stopVoice(int voiceIndex)
{
	static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state = AhdsrEnvelopeState::RELEASE;
}

void AhdsrEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = polyManager.getCurrentVoice();

	state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

	const bool isSustain = static_cast<AhdsrEnvelopeState*>(states[polyManager.getCurrentVoice()])->current_state == AhdsrEnvelopeState::SUSTAIN;

	if (isSustain)
	{
		const float thisSustainValue = sustain * state->modValues[SustainLevelChain];
		const float lastSustainValue = state->lastSustainValue;
		
		if (abs(thisSustainValue - lastSustainValue) > 0.001f)
		{
			const float stepSize = (thisSustainValue - lastSustainValue) / (float)numSamples;
			float* bufferPointer = internalBuffer.getWritePointer(0, startSample);
			float rampedGain = lastSustainValue;

			for (int i = 0; i < numSamples; i++)
			{
				bufferPointer[i] = rampedGain;
				rampedGain += stepSize;
				startSample++;
			}
		}
		else
		{
			FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), thisSustainValue, numSamples);
			startSample += numSamples;
		}

		state->lastSustainValue = thisSustainValue;
		state->current_value = thisSustainValue;
	}
	else
	{
		while (numSamples > 0)
		{
			for (int i = 0; i < 4; i++)
			{
				internalBuffer.setSample(0, startSample, calculateNewValue());
				++startSample;
			}

			numSamples -= 4;
		}
	}

#if ENABLE_ALL_PEAK_METERS
	if (polyManager.getCurrentVoice() == polyManager.getLastStartedVoice()) setOutputValue(internalBuffer.getSample(0, startSample-1));
#endif
}

void AhdsrEnvelope::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);

	state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

	state->current_state = AhdsrEnvelopeState::IDLE;
	state->current_value = 0.0f;
}

void AhdsrEnvelope::handleHiseEvent(const HiseEvent &e)
{
	for(int i = 0; i < numInternalChains; i++)
	{
		internalChains[i]->handleHiseEvent(e);
	}
};

float AhdsrEnvelope::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Attack:		return 20.0f;
	case AttackLevel:	return Decibels::gainToDecibels(1.0f);
	case Hold:			return 10.0f;
	case Decay:			return 300.0f;
	case Sustain:		return Decibels::gainToDecibels(1.0f);
	case Release:		return 20.0f;
	case AttackCurve:	return 1.0f;
	case DecayCurve:	return 1.0f;
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::setInternalAttribute(int parameter_index, float newValue)
{
	switch (parameter_index)
	{
	case Attack:		setAttackRate(newValue); break;
	case AttackLevel:	attackLevel = Decibels::decibelsToGain(newValue); break;
	case Hold:			setHoldTime(newValue); break;
	case Decay:			setDecayRate(newValue); break;
	case Sustain:		setSustainLevel(Decibels::decibelsToGain(newValue)); break;
	case Release:		setReleaseRate(newValue); break;
	case AttackCurve:	setAttackCurve(newValue); break;
	case DecayCurve:	setDecayCurve(newValue); break;
	default:			jassertfalse;
	}
}

float AhdsrEnvelope::getAttribute(int parameter_index) const
{
	switch (parameter_index)
	{
	case Attack:		return attack;
	case AttackLevel:	return Decibels::gainToDecibels(attackLevel);
	case Hold:			return hold;
	case Decay:			return decay;
	case Sustain:		return Decibels::gainToDecibels(sustain);
	case Release:		return release;
	case AttackCurve:	return attackCurve;
	case DecayCurve:	return decayCurve;
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	setAttackRate(attack);
	setDecayRate(decay);
	setReleaseRate(release);
	setSustainLevel(sustain);
	

}

bool AhdsrEnvelope::isPlaying(int voiceIndex) const
{
	return static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state != AhdsrEnvelopeState::IDLE;
}

void AhdsrEnvelope::calculateCoefficients(float timeInMilliSeconds, float base, float maximum, float &stateBase, float &stateCoeff) const
{
	const float t = (timeInMilliSeconds / 1000.0f) * (float)getSampleRate();
	const float exp1 = (powf(base, 1.0f / t));
	const float invertedBase = 1.0f / (base - 1.0f);

	stateCoeff = exp1;
	stateBase = (exp1 *invertedBase - invertedBase) * maximum;
}

float AhdsrEnvelope::calculateNewValue()
{
    const float thisSustain = sustain * state->modValues[SustainLevelChain];
    
	switch (state->current_state) 
	{
		case AhdsrEnvelopeState::IDLE:	    break;
		case AhdsrEnvelopeState::ATTACK:
		{
			if (attack != 0.0f)
			{
				state->current_value = (state->attackBase + state->current_value * state->attackCoef);

				if (state->attackLevel > thisSustain)
				{
					if (state->current_value >= state->attackLevel)
					{
						state->current_value = state->attackLevel;
						state->holdCounter = 0;
						state->current_state = AhdsrEnvelopeState::HOLD;
					}
				}
				else if (state->attackLevel <= thisSustain)
				{
					if (state->current_value >= thisSustain)
					{
						state->current_value = thisSustain;
						state->current_state = AhdsrEnvelopeState::SUSTAIN;
					}
				}

				break;
			}
			else
			{
				state->current_value = state->attackLevel;
				state->holdCounter = 0;
				state->current_state = AhdsrEnvelopeState::HOLD;
			}
		}
		case AhdsrEnvelopeState::HOLD:
			{
				state->holdCounter++;

				if (state->holdCounter >= holdTimeSamples)
				{
					state->current_state = AhdsrEnvelopeState::DECAY;
				}
				else
				{
					state->current_value = state->attackLevel;
					break;
				}
			}
		case AhdsrEnvelopeState::DECAY:
		{
			if (decay != 0.0f)
			{
				state->current_value = state->decayBase + state->current_value * state->decayCoef;
				if ((state->current_value - thisSustain) < 0.001f)
				{
					state->lastSustainValue = state->current_value;
					state->current_state = AhdsrEnvelopeState::SUSTAIN;

					if (thisSustain == 0.0f)  state->current_state = AhdsrEnvelopeState::IDLE;
				}
			}
			else
			{
				state->current_state = AhdsrEnvelopeState::SUSTAIN;
				state->current_value = thisSustain;

				if (thisSustain == 0.0f)  state->current_state = AhdsrEnvelopeState::IDLE;
			}
			break;
		}
		case AhdsrEnvelopeState::SUSTAIN: state->current_value = thisSustain; break;
		case AhdsrEnvelopeState::RELEASE:
		{
			if (release != 0.0f)
			{
				state->current_value = state->releaseBase + state->current_value * state->releaseCoef;
				if (state->current_value <= 0.001f)
				{
					state->current_value = 0.0f;
					state->current_state = AhdsrEnvelopeState::IDLE;
				}
			}
			else
			{
				state->current_value = 0.0f;
				state->current_state = AhdsrEnvelopeState::IDLE;
			}
		}
	}

	return state->current_value;
}


void AhdsrEnvelope::setAttackCurve(float newValue)
{
	attackCurve = newValue;

	if (newValue > 0.5001f)
	{
		const float r1 = (newValue - 0.5f)*2.0f;
		attackBase = r1 * 100.0f;
	}
	else if (newValue < 0.4999f)
	{
		const float r1 = 1.0f - (newValue *2.0f);
		attackBase = 1.0f / (r1 * 100.0f);
	}
	else
	{
		attackBase = 1.2f;
	}
}

void AhdsrEnvelope::setDecayCurve(float newValue)
{
	decayCurve = newValue;

	const float newRatio = decayCurve * 0.0001f;

	setTargetRatioDR(newRatio);
	setDecayRate(decay);
	setReleaseRate(release);
}

ProcessorEditorBody * AhdsrEnvelope::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND

	return new AhdsrEnvelopeEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};

float AhdsrEnvelope::calcCoef(float rate, float targetRatio) const
{
	const float factor = (float)getSampleRate() * 0.001f;

	rate *= factor;
	return expf(-logf((1.0f + targetRatio) / targetRatio) / rate);
}

void AhdsrEnvelope::AhdsrEnvelopeState::setAttackRate(float rate)
{
	const float modValue = modValues[AhdsrEnvelope::AttackTimeChain];

	if (modValue == 0.0f)
	{
		attackBase = 1.0f;
		attackCoef = 0.0f;
	}
	else if (modValue != 1.0f)
	{
		const float stateAttack = modValue * rate;

		envelope->calculateCoefficients(stateAttack, envelope->attackBase, attackLevel, attackBase, attackCoef);
	}
	else
	{
		envelope->calculateCoefficients(rate, envelope->attackBase, attackLevel, attackBase, attackCoef);
	}
}

void AhdsrEnvelope::AhdsrEnvelopeState::setDecayRate(float rate)
{
	const float modValue = modValues[AhdsrEnvelope::DecayTimeChain];

	const float susModValue = modValues[AhdsrEnvelope::SustainLevelChain];

    const float thisSustain = envelope->sustain * susModValue;
    
	if (modValue == 0.0f)
	{
        decayBase = thisSustain;
		decayCoef = 0.0f;
	}
	else if (modValue != 1.0f)
	{
		const float stateDecay = modValue * rate;

		decayCoef = envelope->calcCoef(stateDecay, envelope->targetRatioDR);
		decayBase = (thisSustain - envelope->targetRatioDR) * (1.0f - decayCoef);

	}
	else if (susModValue != 1.0f) // the decay rates need to be recalculated when the sustain modulation is active...
	{
		decayCoef = envelope->calcCoef(envelope->decay, envelope->targetRatioDR);
		decayBase = (thisSustain - envelope->targetRatioDR) * (1.0f - decayCoef);
	}
	else
	{
		decayCoef = envelope->decayCoef;
		decayBase = envelope->decayBase;
	}
}

void AhdsrEnvelope::AhdsrEnvelopeState::setReleaseRate(float rate)
{
	const float modValue = modValues[AhdsrEnvelope::ReleaseTimeChain];

	if (modValue != 1.0f)
	{
		const float stateRelease = modValue * rate;

		releaseCoef = envelope->calcCoef(stateRelease, envelope->targetRatioDR);
		releaseBase = -envelope->targetRatioDR * (1.0f - releaseCoef);

	}
	else
	{
		releaseCoef = envelope->releaseCoef;
		releaseBase = envelope->releaseBase;
	}
}
