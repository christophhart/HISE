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

#define AHDSR_DOWNSAMPLE_FACTOR 4

Processor * AhdsrEnvelope::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
}

const Processor * AhdsrEnvelope::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
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
	parameterNames.add("EcoMode");

	editorStateIdentifiers.add("AttackTimeChainShown");
	editorStateIdentifiers.add("AttackLevelChainShown");
	editorStateIdentifiers.add("DecayTimeChainShown");
	editorStateIdentifiers.add("SustainLevelChainShown");
	editorStateIdentifiers.add("ReleaseTimeChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	monophonicState = createSubclassedState(-1);

	internalChains.reserve(numInternalChains);

	internalChains += {this, "Attack Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Attack Level", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Decay Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Sustain Level", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Release Time", ModulatorChain::ModulationType::VoiceStartOnly };

	internalChains.finalise();

	for (auto& mb : internalChains)
		mb.getChain()->setParentProcessor(this);

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
	loadAttribute(EcoMode, "EcoMode");
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
	saveAttribute(EcoMode, "EcoMode");

	return v;
}

float AhdsrEnvelope::getSampleRateForCurrentMode() const
{
	auto sr = getControlRate();
	
	return (float)sr;
}

void AhdsrEnvelope::setAttackRate(float rate) {
	attack = rate;
}

void AhdsrEnvelope::setHoldTime(float holdTimeMs) {
	hold = holdTimeMs;

	holdTimeSamples = holdTimeMs * ((float)getSampleRateForCurrentMode() / 1000.0f);
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

float AhdsrEnvelope::startVoice(int voiceIndex)
{
	stateInfo.state = AhdsrEnvelopeState::ATTACK;
	stateInfo.changeTime = getMainController()->getUptime();

	if (isMonophonic)
	{
		state = static_cast<AhdsrEnvelopeState*>(monophonicState.get());

		EnvelopeModulator::startVoice(voiceIndex);

		const bool restartEnvelope = shouldRetrigger || getNumPressedKeys() == 1;

		if (restartEnvelope)
		{
			
			for (auto& mb : internalChains)
				mb.startVoice(voiceIndex);
			
			state->modValues[AttackTimeChain] = internalChains[AttackTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[AttackLevelChain] = internalChains[AttackLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[DecayTimeChain] = internalChains[DecayTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[SustainLevelChain] = internalChains[SustainLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[ReleaseTimeChain] = internalChains[ReleaseTimeChain].getChain()->getConstantVoiceValue(voiceIndex);

			// Don't reset the envelope for tailing releases
			if (shouldRetrigger && state->current_state != AhdsrEnvelopeState::IDLE)
			{
				state->current_state = AhdsrEnvelopeState::RETRIGGER;
			}
			else
			{
				state->current_state = AhdsrEnvelopeState::ATTACK;
				state->current_value = 0.0f;
			}

			state->attackLevel = attackLevel * state->modValues[AttackLevelChain];
			state->setAttackRate(attack);
			state->setDecayRate(decay);
			state->setReleaseRate(release);

			state->lastSustainValue = sustain * state->modValues[SustainLevelChain];
		}
	}
	else
	{
		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

		if (state->current_state != AhdsrEnvelopeState::IDLE)
		{
			reset(voiceIndex);
		}

		for (auto& mb : internalChains)
			mb.startVoice(voiceIndex);

		state->modValues[AttackTimeChain] = internalChains[AttackTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[AttackLevelChain] = internalChains[AttackLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[DecayTimeChain] = internalChains[DecayTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[SustainLevelChain] = internalChains[SustainLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[ReleaseTimeChain] = internalChains[ReleaseTimeChain].getChain()->getConstantVoiceValue(voiceIndex);

		state->attackLevel = attackLevel * state->modValues[AttackLevelChain];
		state->setAttackRate(attack);
		state->setDecayRate(decay);
		state->setReleaseRate(release);

		state->current_state = AhdsrEnvelopeState::ATTACK;

		state->current_value = 0.0f;
		state->leftOverSamplesFromLastBuffer = 0;

		state->lastSustainValue = sustain * state->modValues[SustainLevelChain];
	}

	return calculateNewValue(voiceIndex);
}

void AhdsrEnvelope::stopVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::stopVoice(voiceIndex);

		
	}
	else
	{
		static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state = AhdsrEnvelopeState::RELEASE;
	}
}

void AhdsrEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	jassert(voiceIndex < states.size());

	if (isMonophonic)
		state = static_cast<AhdsrEnvelopeState*>(monophonicState.get());
	else
		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

	const bool isSustain = static_cast<AhdsrEnvelopeState*>(state)->current_state == AhdsrEnvelopeState::SUSTAIN;

	if (isSustain)
	{
		const float thisSustainValue = sustain * state->modValues[SustainLevelChain];
		const float lastSustainValue = state->lastSustainValue;
		
		if (std::abs(thisSustainValue - lastSustainValue) > 0.001f)
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
			internalBuffer.setSample(0, startSample, calculateNewValue(voiceIndex));
			++startSample;
			numSamples--;
		}

		
	}

	const bool isActiveVoice = polyManager.getCurrentVoice() == polyManager.getLastStartedVoice();

	if (isMonophonic || isActiveVoice)
	{
		if (state->current_state != stateInfo.state)
		{
			stateInfo.state = state->current_state;
			stateInfo.changeTime = getMainController()->getUptime();
		}
	}
}

void AhdsrEnvelope::reset(int voiceIndex)
{
	if (isMonophonic)
	{
		stateInfo.state = AhdsrEnvelopeState::IDLE;
		return;
	}
	else
	{
		EnvelopeModulator::reset(voiceIndex);

		if (voiceIndex == polyManager.getLastStartedVoice())
			stateInfo.state = AhdsrEnvelopeState::IDLE;

		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);
		state->current_state = AhdsrEnvelopeState::IDLE;
		state->current_value = 0.0f;
	}
}

void AhdsrEnvelope::handleHiseEvent(const HiseEvent &e)
{
	EnvelopeModulator::handleHiseEvent(e);

	if (isInMonophonicMode() && getNumPressedKeys() == 0)
	{
		auto monoState = static_cast<AhdsrEnvelopeState*>(monophonicState.get());
		monoState->current_state = AhdsrEnvelopeState::RELEASE;
	}

	for (auto& mb : internalChains)
		mb.handleHiseEvent(e);
};

float AhdsrEnvelope::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getDefaultValue(parameterIndex);
	}

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
	case EcoMode:		return 1.0f;
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case Attack:		setAttackRate(newValue); break;
	case AttackLevel:	attackLevel = Decibels::decibelsToGain(newValue); break;
	case Hold:			setHoldTime(newValue); break;
	case Decay:			setDecayRate(newValue); break;
	case Sustain:		setSustainLevel(Decibels::decibelsToGain(newValue)); break;
	case Release:		setReleaseRate(newValue); break;
	case AttackCurve:	setAttackCurve(newValue); break;
	case DecayCurve:	setDecayCurve(newValue); break;
	case EcoMode:		break; // not needed anymore...
	default:			jassertfalse;
	}
}

float AhdsrEnvelope::getAttribute(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getAttribute(parameterIndex);
	}

	switch (parameterIndex)
	{
	case Attack:		return attack;
	case AttackLevel:	return Decibels::gainToDecibels(attackLevel);
	case Hold:			return hold;
	case Decay:			return decay;
	case Sustain:		return Decibels::gainToDecibels(sustain);
	case Release:		return release;
	case AttackCurve:	return attackCurve;
	case DecayCurve:	return decayCurve;
	case EcoMode:		return 1.0f; // not needed anymore...
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto& mb : internalChains)
		mb.prepareToPlay(sampleRate, samplesPerBlock);

	setAttackRate(attack);
	setDecayRate(decay);
	setReleaseRate(release);
	setSustainLevel(sustain);
}

bool AhdsrEnvelope::isPlaying(int voiceIndex) const
{
	if (isMonophonic)
		return true;

	return static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state != AhdsrEnvelopeState::IDLE;
}

void AhdsrEnvelope::calculateCoefficients(float timeInMilliSeconds, float base, float maximum, float &stateBase, float &stateCoeff) const
{
	if (timeInMilliSeconds < 1.0f)
	{
		stateCoeff = 0.0f;
		stateBase = 1.0f;
		return;
	}

	const float t = (timeInMilliSeconds / 1000.0f) * (float)getSampleRateForCurrentMode();
	const float exp1 = (powf(base, 1.0f / t));
	const float invertedBase = 1.0f / (base - 1.0f);

	stateCoeff = exp1;
	stateBase = (exp1 *invertedBase - invertedBase) * maximum;
}

float AhdsrEnvelope::calculateNewValue(int /*voiceIndex*/)
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

			break;
		}
		case AhdsrEnvelopeState::RETRIGGER:
		{

#if HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO
			const bool down = attack > 0.0f;

			if (down)
			{
				state->current_value -= 0.005f;
				if (state->current_value <= 0.0f)
				{
					state->current_value = 0.0f;
					state->current_state = AhdsrEnvelopeState::ATTACK;
				}
			}
			else
			{
				state->current_value += 0.005f;

				if (state->current_value >= state->attackLevel)
				{
					state->current_value = state->attackLevel;
					state->holdCounter = 0;
					state->current_state = AhdsrEnvelopeState::HOLD;
				}
			}
			break;
#else
			state->current_state = AhdsrEnvelopeState::ATTACK;
			return calculateNewValue(-1);
#endif
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
	const float factor = (float)getSampleRateForCurrentMode() * 0.001f;

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



AhdsrGraph::AhdsrGraph(Processor *p) :
	processor(p)
{
	setBufferedToImage(true);

	if (dynamic_cast<AhdsrEnvelope*>(p) != nullptr)
		startTimer(50);
	else
		jassertfalse;

	setColour(lineColour, Colours::lightgrey.withAlpha(0.3f));
}

AhdsrGraph::~AhdsrGraph()
{
	
}

void AhdsrGraph::paint(Graphics &g)
{
	if (flatDesign)
	{
		g.setColour(findColour(bgColour));
		g.fillAll();
		g.setColour(findColour(fillColour));
		g.fillPath(envelopePath);
		g.setColour(findColour(lineColour));
		g.strokePath(envelopePath, PathStrokeType(1.0f));
		g.setColour(findColour(outlineColour));
		g.drawRect(getLocalBounds(), 1);
	}
	else
	{
		GlobalHiseLookAndFeel::fillPathHiStyle(g, envelopePath, getWidth(), getHeight());

		g.setColour(findColour(lineColour));

		
		g.strokePath(envelopePath, PathStrokeType(1.0f));
		g.setColour(Colours::lightgrey.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
	}

	g.setColour(Colours::white.withAlpha(0.1f));

	float xPos = 0.0f;

	float tToUse = 1.0f;

	Path* pToUse = nullptr;

	switch (lastState.state)
	{
	case AhdsrEnvelope::AhdsrEnvelopeState::ATTACK: pToUse = &attackPath; tToUse = attack; break;
	case AhdsrEnvelope::AhdsrEnvelopeState::HOLD: pToUse = &holdPath; tToUse = hold; break;
	case AhdsrEnvelope::AhdsrEnvelopeState::DECAY: pToUse = &decayPath; tToUse = 0.5f * decay; break;
	case AhdsrEnvelope::AhdsrEnvelopeState::SUSTAIN: pToUse = &decayPath; 
													 tToUse = 0.001f; // nasty hack to make the bar stick at the end...
													 break;
	case AhdsrEnvelope::AhdsrEnvelopeState::RELEASE: pToUse = &releasePath; tToUse = 0.8f * release; break;
	default:
		break;
	}

	if (pToUse != nullptr)
	{
		g.fillPath(*pToUse);
		
		auto bounds = pToUse->getBounds();
		auto duration = (float)(processor->getMainController()->getUptime() - lastState.changeTime) * 1000.0f;
		
		auto normalizedDuration = 0.0f;

		if (tToUse != 0.0f)
			normalizedDuration = jlimit<float>(0.01f, 1.0f, duration / tToUse);

		xPos = bounds.getX() + normalizedDuration * bounds.getWidth();

		const float margin = 3.0f;

		auto l = Line<float>(xPos, 0.0f, xPos, (float)getHeight()-1.0f - margin);

		auto clippedLine = envelopePath.getClippedLine(l, false);

		if (clippedLine.getLength() == 0.0f)
			return;

		auto circle = Rectangle<float>(clippedLine.getStart(), clippedLine.getStart()).withSizeKeepingCentre(6.0f, 6.0f);
		
		g.setColour(findColour(lineColour).withAlpha(1.0f));

		g.fillRoundedRectangle(circle, 2.0f);
	}
}

void AhdsrGraph::setUseFlatDesign(bool shouldUseFlatDesign)
{
	flatDesign = shouldUseFlatDesign;
	repaint();
}

void AhdsrGraph::timerCallback()
{
	float this_attack = processor->getAttribute(AhdsrEnvelope::Attack);
	float this_attackLevel = processor->getAttribute(AhdsrEnvelope::AttackLevel);
	float this_hold = processor->getAttribute(AhdsrEnvelope::Hold);
	float this_decay = processor->getAttribute(AhdsrEnvelope::Decay);
	float this_sustain = processor->getAttribute(AhdsrEnvelope::Sustain);
	float this_release = processor->getAttribute(AhdsrEnvelope::Release);
	float this_attackCurve = processor->getAttribute(AhdsrEnvelope::AttackCurve);
	lastState = dynamic_cast<AhdsrEnvelope*>(processor.get())->getStateInfo();

	if (this_attack != attack ||
		this_attackCurve != attackCurve ||
		this_attackLevel != attackLevel ||
		this_decay != decay ||
		this_sustain != sustain ||
		this_hold != hold ||
		this_release != release)
	{
		attack = this_attack;
		attackLevel = this_attackLevel;
		hold = this_hold;
		decay = this_decay;
		sustain = this_sustain;
		release = this_release;
		attackCurve = this_attackCurve;

		rebuildGraph();
	}

	repaint();
}

void AhdsrGraph::rebuildGraph()
{
	float aln = pow((1.0f - (attackLevel + 100.0f) / 100.0f), 0.4f);
	const float sn = pow((1.0f - (sustain + 100.0f) / 100.0f), 0.4f);

	const float margin = 3.0f;

	aln = sn < aln ? sn : aln;

	const float width = (float)getWidth() - 2.0f*margin;
	const float height = (float)getHeight() - 2.0f*margin;

	const float an = pow((attack / 20000.0f), 0.2f) * (0.2f * width);
	const float hn = pow((hold / 20000.0f), 0.2f) * (0.2f * width);
	const float dn = pow((decay / 20000.0f), 0.2f) * (0.2f * width);
	const float rn = pow((release / 20000.0f), 0.2f) * (0.2f * width);

	float x = margin;
	float lastX = x;

	envelopePath.clear();

	attackPath.clear();
	decayPath.clear();
	holdPath.clear();
	releasePath.clear();

	envelopePath.startNewSubPath(x, margin + height);
	attackPath.startNewSubPath(x, margin + height);

	// Attack Curve

	lastX = x;
	x += an;

	const float controlY = margin + aln * height + attackCurve * (height - aln * height);

	envelopePath.quadraticTo((lastX + x) / 2, controlY, x, margin + aln * height);
	
	attackPath.quadraticTo((lastX + x) / 2, controlY, x, margin + aln * height);
	attackPath.lineTo(x, margin + height);
	attackPath.closeSubPath();

	holdPath.startNewSubPath(x, margin + height);
	holdPath.lineTo(x, margin + aln*height);

	x += hn;

	envelopePath.lineTo(x, margin + aln * height);
	holdPath.lineTo(x, margin + aln*height);
	holdPath.lineTo(x, margin + height);
	holdPath.closeSubPath();
	
	decayPath.startNewSubPath(x, margin + height);
	decayPath.lineTo(x, margin + aln*height);

	lastX = x;
	x = jmin<float>(x + (dn*4), 0.8f * width);

	envelopePath.quadraticTo(lastX, margin + sn * height, x, margin + sn * height);
	decayPath.quadraticTo(lastX, margin + sn * height, x, margin + sn * height);

	x = 0.8f * width;

	envelopePath.lineTo(x, margin + sn*height);
	decayPath.lineTo(x, margin + sn*height);

	decayPath.lineTo(x, margin + height);
	decayPath.closeSubPath();

	releasePath.startNewSubPath(x, margin + height);
	releasePath.lineTo(x, margin + sn*height);

	lastX = x;
	x += rn;

	envelopePath.quadraticTo(lastX, margin + height, x, margin + height);
	releasePath.quadraticTo(lastX, margin + height, x, margin + height);

	releasePath.closeSubPath();
	envelopePath.closeSubPath();
}

AhdsrGraph::Panel::Panel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.1f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.05f));
}

juce::Component* AhdsrGraph::Panel::createContentComponent(int /*index*/)
{
	auto g = new AhdsrGraph(getProcessor());
	g->setUseFlatDesign(true);

	g->setColour(bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	g->setColour(fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
	g->setColour(lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
	g->setColour(outlineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour3));

	if (g->findColour(bgColour).isOpaque())
		g->setOpaque(true);

	return g;
}

void AhdsrGraph::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<AhdsrEnvelope>(moduleList);
}

} // namespace hise
