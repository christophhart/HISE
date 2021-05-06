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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once


namespace scriptnode {
namespace envelope {
namespace pimpl {
using namespace juce;
using namespace hise;

float ahdsr_base::getSampleRateForCurrentMode() const
{
	return sampleRate;
}

void ahdsr_base::refreshUIPath(Path& p, Point<float>& position)
{
}

void ahdsr_base::setExternalData(const snex::ExternalData& d, int index)
{
	if (recursiveSetter)
		return;

	base::setExternalData(d, index);

	rb = dynamic_cast<SimpleRingBuffer*>(d.obj);

	ScopedValueSetter<bool> svs(recursiveSetter, true);
	setRingBufferPropertyObject<AhdsrRingBufferProperties>();
}

void ahdsr_base::setAttackRate(float rate) {
	attack = rate;
}

void ahdsr_base::setHoldTime(float holdTimeMs) {
	hold = holdTimeMs;

	holdTimeSamples = holdTimeMs * ((float)getSampleRateForCurrentMode() / 1000.0f);
}

void ahdsr_base::setDecayRate(float rate)
{
	decay = rate;

	decayCoef = calcCoef(decay, targetRatioDR);
	decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);
}

void ahdsr_base::setReleaseRate(float rate)
{
	release = jmax<float>(1.0f, rate);

	releaseCoef = calcCoef(release, targetRatioDR);
	releaseBase = -targetRatioDR * (1.0f - releaseCoef);
}

void ahdsr_base::setSustainLevel(float level)
{
	sustain = level;
	decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);

}

void ahdsr_base::setTargetRatioDR(float targetRatio) {

	if (targetRatio < 0.0000001f)
		targetRatio = 0.0000001f;
	targetRatioDR = targetRatio;

	decayBase = (sustain - targetRatioDR) * (1.0f - decayCoef);
	releaseBase = -targetRatioDR * (1.0f - releaseCoef);
}


void ahdsr_base::setAttackCurve(float newValue)
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

void ahdsr_base::setDecayCurve(float newValue)
{
	decayCurve = newValue;

	const float newRatio = decayCurve * 0.0001f;

	setTargetRatioDR(newRatio);
	setDecayRate(decay);
	setReleaseRate(release);
}

float ahdsr_base::calcCoef(float rate, float targetRatio) const
{
	const float factor = (float)getSampleRateForCurrentMode() * 0.001f;

	rate *= factor;
	return expf(-logf((1.0f + targetRatio) / targetRatio) / rate);
}

ahdsr_base::ahdsr_base() :
	attack(20.0f),
	attackLevel(1.0f),
	attackCurve(0.0f),
	decayCurve(0.0f),
	hold(10.0f),
	decay(300.0f),
	sustain(1.0f),
	release(20.0f)
{
	setAttackCurve(0.0f);
	setDecayCurve(0.0f);
}

void ahdsr_base::calculateCoefficients(float timeInMilliSeconds, float base, float maximum, float &stateBase, float &stateCoeff) const
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


void ahdsr_base::state_base::refreshAttackTime()
{
	envelope->calculateCoefficients(attackTime, envelope->attackBase, attackLevel, attackBase, attackCoef);
}

void ahdsr_base::state_base::refreshDecayTime()
{
	const float susModValue = modValues[ahdsr_base::SustainLevelChain];
	const float thisSustain = envelope->sustain * susModValue;

	decayCoef = envelope->calcCoef(decayTime, envelope->targetRatioDR);
	decayBase = (thisSustain - envelope->targetRatioDR) * (1.0f - decayCoef);
}

void ahdsr_base::state_base::refreshReleaseTime()
{
	releaseCoef = envelope->calcCoef(releaseTime, envelope->targetRatioDR);
	releaseBase = -envelope->targetRatioDR * (1.0f - releaseCoef);
}

ahdsr_base::state_base::state_base() :
	current_state(IDLE),
	holdCounter(0),
	current_value(0.0f),
	lastSustainValue(1.0f),
	attackLevel(1.0f)
{
	for (int i = 0; i < 5; i++)
		modValues[i] = 1.0f;
}

void ahdsr_base::state_base::setAttackRate(float rate)
{
	const float modValue = modValues[ahdsr_base::AttackTimeChain];

	if (modValue == 0.0f)
	{
		attackBase = 1.0f;
		attackCoef = 0.0f;
	}
	else if (modValue != 1.0f)
	{
		attackTime = modValue * rate;
		refreshAttackTime();
	}
	else
	{
		attackTime = rate;
		refreshAttackTime();
	}
}

void ahdsr_base::state_base::setDecayRate(float rate)
{
	const float modValue = modValues[ahdsr_base::DecayTimeChain];

	const float susModValue = modValues[ahdsr_base::SustainLevelChain];
	const float thisSustain = envelope->sustain * susModValue;

	if (modValue == 0.0f)
	{
		decayTime = rate;
		decayBase = thisSustain;
		decayCoef = 0.0f;
	}
	else if (modValue != 1.0f)
	{
		decayTime = modValue * rate;
		refreshDecayTime();
	}
	else if (susModValue != 1.0f) // the decay rates need to be recalculated when the sustain modulation is active...
	{
		decayTime = envelope->decay;
		refreshDecayTime();
	}
	else
	{
		decayTime = rate;
		decayCoef = envelope->decayCoef;
		decayBase = envelope->decayBase;
	}
}

void ahdsr_base::state_base::setReleaseRate(float rate)
{
	const float modValue = modValues[ahdsr_base::ReleaseTimeChain];

	if (modValue != 1.0f)
	{
		releaseTime = modValue * rate;
		refreshReleaseTime();
	}
	else
	{
		releaseTime = rate;
		releaseCoef = envelope->releaseCoef;
		releaseBase = envelope->releaseBase;
	}
}



float ahdsr_base::state_base::tick()
{
	auto state = this;

	const float thisSustain = envelope->sustain * state->modValues[3];

	active = current_state != state_base::IDLE;

	switch (state->current_state)
	{
	case state_base::IDLE:	    

		break;
	case state_base::ATTACK:
	{
		if (envelope->attack != 0.0f)
		{
			state->current_value = (state->attackBase + state->current_value * state->attackCoef);
			if (state->attackLevel > thisSustain)
			{
				if (state->current_value >= state->attackLevel)
				{
					state->current_value = state->attackLevel;
					state->holdCounter = 0;
					state->current_state = state_base::HOLD;
				}
			}
			else if (state->attackLevel <= thisSustain)
			{
				if (state->current_value >= thisSustain)
				{
					state->current_value = thisSustain;
					state->current_state = state_base::SUSTAIN;
				}
			}

			break;
		}
		else
		{
			state->current_value = state->attackLevel;
			state->holdCounter = 0;
			state->current_state = state_base::HOLD;
		}
	}
	case state_base::HOLD:
	{
		state->holdCounter++;

		if (state->holdCounter >= envelope->holdTimeSamples)
		{
			state->current_state = state_base::DECAY;
		}
		else
		{
			state->current_value = state->attackLevel;
			break;
		}
	}

	case state_base::DECAY:
	{
		if (envelope->decay != 0.0f)
		{
			state->current_value = state->decayBase + state->current_value * state->decayCoef;
			if ((state->current_value - thisSustain) < 0.001f)
			{
				state->lastSustainValue = state->current_value;
				state->current_state = state_base::SUSTAIN;

				if (thisSustain == 0.0f)  state->current_state = state_base::IDLE;
			}
		}
		else
		{
			state->current_state = state_base::SUSTAIN;
			state->current_value = thisSustain;

			if (thisSustain == 0.0f)  state->current_state = state_base::IDLE;
		}
		break;
	}
	case state_base::SUSTAIN: state->current_value = thisSustain; break;
	case state_base::RELEASE:
	{
		if (envelope->release != 0.0f)
		{
			state->current_value = state->releaseBase + state->current_value * state->releaseCoef;
			if (state->current_value <= 0.001f)
			{
				state->current_value = 0.0f;
				state->current_state = state_base::IDLE;
			}
		}
		else
		{
			state->current_value = 0.0f;
			state->current_state = state_base::IDLE;
		}

		break;
	}
	case state_base::RETRIGGER:
	{

#if HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO
		const bool down = attack > 0.0f;

		if (down)
		{
			state->current_value -= 0.005f;
			if (state->current_value <= 0.0f)
			{
				state->current_value = 0.0f;
				state->current_state = state_base::ATTACK;
			}
		}
		else
		{
			state->current_value += 0.005f;

			if (state->current_value >= state->attackLevel)
			{
				state->current_value = state->attackLevel;
				state->holdCounter = 0;
				state->current_state = state_base::HOLD;
			}
		}
		break;
#else
		state->current_state = state_base::ATTACK;
		return tick();
#endif
	}
	}

	return state->current_value;
}

static float ratioOrZero(double nom, double denom) { return denom != 0.0 ? nom / denom : 0.0; }

float ahdsr_base::state_base::getUIPosition(double deltaMs)
{
	switch (current_state)
	{
	case EnvelopeState::RETRIGGER:
	case EnvelopeState::ATTACK:  return (double)current_state + ratioOrZero(deltaMs, attackTime);
	case EnvelopeState::HOLD:	 return (double)current_state + ratioOrZero(deltaMs, envelope->hold);
	case EnvelopeState::DECAY:   return (double)current_state + ratioOrZero(deltaMs, decayTime);
	case EnvelopeState::SUSTAIN: return (double)current_state;
	case EnvelopeState::RELEASE: return (double)current_state + ratioOrZero(deltaMs, releaseTime);
	}

	return -1.0f;
}

bool ahdsr_base::AhdsrRingBufferProperties::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
		return SimpleRingBuffer::toFixSize<9>(v);

	if (id == RingBufferIds::NumChannels)
		return SimpleRingBuffer::toFixSize<1>(v);

	return false;
}

bool simple_ar_base::PropertyObject::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
		return SimpleRingBuffer::toFixSize<1024>(v);

	if (id == RingBufferIds::NumChannels)
		return SimpleRingBuffer::toFixSize<1>(v);

	return false;
}

void simple_ar_base::PropertyObject::transformReadBuffer(AudioSampleBuffer& b)
{
	State s;

	auto attack = parent->uiValues[0];
	auto release = parent->uiValues[1];

	s.env.setAttackDouble(attack);
	s.env.setReleaseDouble(release);

	auto totalSeconds = (attack + release) * 0.001 + 0.1;
	auto sr = 512.0 / totalSeconds;
	auto isOn = s.targetValue > 0.0f;
	
	s.env.setSampleRate(sr);
	s.env.reset();
	int counter = 0;
	s.setGate(true);

	jassert(b.getNumSamples() == 1024);

	auto ptr = b.getWritePointer(0);
	int phase = 0;

	for (int i = 0; i < 1024; i++)
	{
		switch (phase)
		{
		case 0:
			ptr[i] = s.tick();
			if (!s.smoothing)
				phase = 1;
			break;
		case 1:
			ptr[i] = s.tick();
			s.setGate(false);
			phase = 2;
			break;
		case 2:
			ptr[i] = s.tick();
			break;
		}
	}


#if 0

	path.startNewSubPath(0, 1.0f);

	int currentIndex = -1;

	auto deltaY = 1.0f / getHeight();
	float lastV = -10.0;

	while (counter < 90000 && stateCopy.lastValue < 0.9999)
	{
		auto v = 1.0f - stateCopy.tick();

		if (isOn && std::abs(lv - v) < 0.0001)
			currentIndex = counter;

		if (std::abs(lastV - v) > deltaY)
		{
			path.lineTo(counter, v);
			lastV = v;
		}

		counter++;
	}

	auto tCounter = counter;

	while (counter < (tCounter + 4))
	{
		path.lineTo(counter, 0.0f);
		counter++;
	}

	path.lineTo(counter, 0.0f);

	stateCopy.setGate(false);

	lastV = -12.0f;

	while (counter < 90000 && stateCopy.lastValue > 0.001)
	{
		auto v = 1.0f - stateCopy.tick();

		if (!isOn && std::abs(lv - v) < 0.0001)
			currentIndex = counter;

		if (std::abs(lastV - v) > deltaY)
		{
			path.lineTo(counter, v);
			lastV = v;
		}

		counter++;
	}

	path.lineTo(counter - 1, 1.0f);

	if (currentIndex != -1 && currentIndex < (counter - 1))
	{
		original.startNewSubPath(0, 0.0f);

		original.startNewSubPath(currentIndex, 0.0f);
		original.lineTo(currentIndex, 1.0f);

		original.startNewSubPath(counter - 1, 1.0f);
	}
#endif

	// Make the path...
}

void simple_ar_base::setExternalData(const snex::ExternalData& d, int index)
{
	if (recursiveSetter)
		return;

	base::setExternalData(d, index);

	ScopedValueSetter<bool> svs(recursiveSetter, true);
	setRingBufferPropertyObject<PropertyObject>();
}

void simple_ar_base::setDisplayValue(int index, double value)
{
	if (index == 2)
	{
		if (rb != nullptr)
			rb->getUpdater().sendDisplayChangeMessage((float)value, sendNotificationAsync, true);
	}
	else
	{
		uiValues[index] = value;

		if (rb != nullptr)
			rb->getUpdater().sendContentChangeMessage(sendNotificationAsync, index);
	}
}

} // pimpl
} // envelope
} // scriptnode
