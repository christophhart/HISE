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

void SimpleEnvelope::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(Attack, "Attack");
	loadAttribute(Release, "Release");
	linearMode = v.getProperty("LinearMode", true); // default is on
}

ValueTree SimpleEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(Attack, "Attack");
	saveAttribute(Release, "Release");
	saveAttribute(LinearMode, "LinearMode");

	return v;
}

/*
  ==============================================================================

    SimpleEnvelope.cpp
    Created: 15 Jun 2014 2:08:15pm
    Author:  Chrisboy

  ==============================================================================
*/



SimpleEnvelope::SimpleEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m):
		EnvelopeModulator(mc, id, voiceAmount, m),
		Modulation(m),
		attack(getDefaultValue(Attack)),
		release(getDefaultValue(Release)),
		release_delta(-1.0f),
		linearMode(getDefaultValue(LinearMode) == 1.0f ? true : false)
{
	parameterNames.add("Attack");
	parameterNames.add("Release");
	parameterNames.add("LinearMode");

	editorStateIdentifiers.add("AttackChainShown");
	//editorStateIdentifiers.add("ReleaseChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	monophonicState = createSubclassedState(-1);

	attackChain = new ModulatorChain(mc, "Attack Time Modulation", voiceAmount, ModulatorChain::GainMode, this);

	attackChain->setIsVoiceStartChain(true);
	attackChain->setColour(Colours::red);
};

SimpleEnvelope::~SimpleEnvelope()
{
	attackChain = nullptr;
};


void SimpleEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case Attack:
		setAttackRate(jmax<float>(0.0f, newValue), nullptr);
		break;
	case Release:
		release = jmax<float>(0.0f, newValue);
		setReleaseRate(newValue);
		break;
	case LinearMode:
		linearMode = newValue > 0.5;
		setAttackRate(attack, nullptr);
		setReleaseRate(release);
		break;
	default:
		jassertfalse;
	}
}

float SimpleEnvelope::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getDefaultValue(parameterIndex);
	}

	switch (parameterIndex)
	{
	case Attack:
		return 5.0f;
	case Release:
		return 10.0f;
	case LinearMode:
		return 1.0f;
	default:
		jassertfalse;
		return -1;
	}
}

float SimpleEnvelope::getAttribute(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getAttribute(parameterIndex);
	}

	switch (parameterIndex)
	{
	case Attack:
		return attack;
	case Release:
		return release;
	case LinearMode:
		return linearMode ? 1.0f : 0.0f;
	default:
		jassertfalse;
		return -1;
	}
}

float SimpleEnvelope::startVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::startVoice(voiceIndex);

		const bool restartEnvelope = shouldRetrigger || getNumPressedKeys() == 1;

		if (restartEnvelope)
		{
			state = static_cast<SimpleEnvelopeState*>(monophonicState.get());

			float attackModValue = 1.0f;

			if (attackChain->hasVoiceModulators())
			{
				attackChain->startVoice(voiceIndex);
				attackModValue = attackChain->getConstantVoiceValue(voiceIndex);
			}

			const float thisAttackTime = attack * attackModValue;

			if (linearMode)
				state->attackDelta = this->calcCoefficient(thisAttackTime);
			else
				setAttackRate(thisAttackTime, state);

			// Don't reset the envelope for tailing releases
			if (shouldRetrigger)
			{
				state->current_state = SimpleEnvelopeState::RETRIGGER;
			}
			else
			{
				state->current_state = SimpleEnvelopeState::ATTACK;
			}

			return thisAttackTime > 0.0f ? 0.0f : 1.0f;
		}

		return state->current_value;
	}
	else
	{
		state = static_cast<SimpleEnvelopeState*>(states[voiceIndex]);

		if (state->current_state != SimpleEnvelopeState::IDLE)
			reset(voiceIndex);

		float attackModValue = 1.0f;

		if (attackChain->hasVoiceModulators())
		{
			attackChain->startVoice(voiceIndex);
			attackModValue = attackChain->getConstantVoiceValue(voiceIndex);
		}
		
		const float thisAttackTime = attack * attackModValue;

		if (linearMode)
			state->attackDelta = this->calcCoefficient(thisAttackTime);
		else
			setAttackRate(thisAttackTime, state);

		state->current_state = SimpleEnvelopeState::ATTACK;

		return thisAttackTime > 0.0f ? 0.0f : 1.0f;
	}
}

void SimpleEnvelope::stopVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::stopVoice(voiceIndex);

		if (getNumPressedKeys() == 0)
		{
			auto monoState = static_cast<SimpleEnvelopeState*>(monophonicState.get());
			monoState->current_state = SimpleEnvelopeState::RELEASE;
		}
	}
	else
	{
		if (isPositiveAndBelow(voiceIndex, states.size()))
		{
			auto thisState = static_cast<SimpleEnvelopeState*>(states[voiceIndex]);
			thisState->current_state = SimpleEnvelopeState::RELEASE;
		}
	}
}

void SimpleEnvelope::reset(int voiceIndex)
{
	if (isMonophonic)
	{
		return;
	}
	else
	{
		EnvelopeModulator::reset(voiceIndex);

		if (isPositiveAndBelow(voiceIndex, states.size()))
		{
			SimpleEnvelopeState *thisState = static_cast<SimpleEnvelopeState*>(states[voiceIndex]);

			thisState->current_state = SimpleEnvelopeState::IDLE;
			thisState->current_value = 0.0f;
		}
	}
}

bool SimpleEnvelope::isPlaying(int voiceIndex) const
{
	if (isMonophonic)
	{
		// monophonic envelopes can't kill voices
		return true;
	}
	else
	{
		auto thisState = static_cast<SimpleEnvelopeState*>(states[voiceIndex]);
		return thisState->current_state != SimpleEnvelopeState::IDLE;
	}
}

void SimpleEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	jassert(voiceIndex < states.size());

	if(isMonophonic)
		state = static_cast<SimpleEnvelopeState*>(monophonicState.get());
	else
		state = static_cast<SimpleEnvelopeState*>(states[voiceIndex]);

	if (state->current_state == SimpleEnvelopeState::SUSTAIN)
	{
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples);
	}
	else if (state->current_state == SimpleEnvelopeState::IDLE)
	{
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 0.0f, numSamples);
	}
	else
	{
		
		float *out = internalBuffer.getWritePointer(0, startSample);
		
		if (linearMode)
		{
			while (--numSamples >= 0)
				*out++ = calculateNewValue(voiceIndex);
		}
		else
		{
			while (--numSamples >= 0)
				*out++ = calculateNewExpValue();
		}
	}
}

void SimpleEnvelope::handleHiseEvent(const HiseEvent &m)
{
	EnvelopeModulator::handleHiseEvent(m);

	if(attackChain->shouldBeProcessedAtAll())
		attackChain->handleHiseEvent(m);
};

void SimpleEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	
	setInternalAttribute(Attack, attack);
	setInternalAttribute(Release, release);
	if(attackChain != nullptr) attackChain->prepareToPlay(sampleRate, samplesPerBlock);
}

ProcessorEditorBody *SimpleEnvelope::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new SimpleEnvelopeEditorBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif

};

float SimpleEnvelope::calcCoefficient(float time, float targetRatio/*=1.0f*/) const
{
	auto srToUse = (float)getControlRate();

	if (linearMode)
	{
		return 1.0f / ((time / 1000.0f) * (srToUse));
	}
	else
	{
		if (time == 0.0f) return 0.0f;

		const float factor = srToUse * 0.001f;

		time *= factor;
		return expf(-logf((1.0f + targetRatio) / targetRatio) / time);
	}
}

float SimpleEnvelope::calculateNewValue(int voiceIndex)
{
	switch (state->current_state)
	{
	
	case SimpleEnvelopeState::SUSTAIN: break;
	case SimpleEnvelopeState::IDLE: break;
	case SimpleEnvelopeState::ATTACK:
		state->current_value += state->attackDelta;
		if (state->current_value >= 1.0f)
		{
			state->current_value = 1.0f;
			state->current_state = SimpleEnvelopeState::SUSTAIN;
		}
		break;
	case SimpleEnvelopeState::RETRIGGER:
	{
#if HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO
		state->current_value -= 0.005f;
		if (state->current_value <= 0.0f)
		{
			state->current_value = 0.0f;
			state->current_state = SimpleEnvelopeState::ATTACK;
		}
		break;
#else
		state->current_state = SimpleEnvelopeState::ATTACK;
		return calculateNewValue(voiceIndex);
#endif
	}
	case SimpleEnvelopeState::RELEASE:
		state->current_value -= release_delta;
		if (state->current_value <= 0.0f)
		{
			state->current_value = 0.0f;
			state->current_state = SimpleEnvelopeState::IDLE;
		}
		break;
	default:					    jassertfalse; break;
	}

	return state->current_value;
}

float SimpleEnvelope::calculateNewExpValue()
{
	switch (state->current_state)
	{
	case SimpleEnvelopeState::SUSTAIN: break;
	case SimpleEnvelopeState::IDLE: break;
	case SimpleEnvelopeState::RETRIGGER:
	{
#if HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO
		state->current_value -= 0.005f;
		if (state->current_value <= 0.0f)
		{
			state->current_value = 0.0f;
			state->current_state = SimpleEnvelopeState::ATTACK;
		}
#else
		state->current_state = SimpleEnvelopeState::ATTACK;
		return calculateNewExpValue();
#endif
		break;
	}
	case SimpleEnvelopeState::ATTACK:
		
		state->current_value = state->expAttackBase + state->current_value * state->expAttackCoef;

		

		if (state->current_value >= 1.0f)
		{
			state->current_value = 1.0f;
			state->current_state = SimpleEnvelopeState::SUSTAIN;
		}
		break;
	case SimpleEnvelopeState::RELEASE:
		state->current_value = expReleaseBase + state->current_value * expReleaseCoef;

		if (state->current_value <= 0.0001f){
			state->current_value = 0.0f;
			state->current_state = SimpleEnvelopeState::IDLE;
		}
		break;
	default:					    jassertfalse; break;
	}

	return state->current_value;
}



void SimpleEnvelope::setAttackRate(float rate, SimpleEnvelopeState *stateToChange) {
	
	if (stateToChange == nullptr)
	{
		attack = rate;

		if (linearMode)
		{
			expAttackCoef = 0.0f;
			expAttackBase = 1.0f;
		}
		else
		{
			const float targetRatioA = 0.3f;

			expAttackCoef = calcCoefficient(attack, targetRatioA);
			expAttackBase = (1.0f + targetRatioA) * (1.0f - expAttackCoef);
		}
	}
	else
	{
		SimpleEnvelopeState *thisState = static_cast<SimpleEnvelopeState*>(stateToChange);

		if (linearMode)
		{
			thisState->expAttackCoef = 0.0f;
			thisState->expAttackBase = 1.0f;
		}
		else
		{
			const float targetRatioA = 0.3f;

			thisState->expAttackCoef = calcCoefficient(rate, targetRatioA);
			thisState->expAttackBase = (1.0f + targetRatioA) * (1.0f - thisState->expAttackCoef);
		}
	}
}


void SimpleEnvelope::setReleaseRate(float rate)
{
	release = rate;

	if (linearMode)
	{
		expReleaseCoef = 0.0f;
		expReleaseBase = 1.0f;

		release_delta = calcCoefficient(release);
	}
	else
	{
		const float targetRatioR = 0.0001f;

		expReleaseCoef = calcCoefficient(release, targetRatioR);
		expReleaseBase = -targetRatioR * (1.0f - expReleaseCoef);
	}
}

} // namespace hise
