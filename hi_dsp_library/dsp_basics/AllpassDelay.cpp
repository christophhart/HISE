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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace hise
{
using namespace juce;

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

	double inputDouble = (double)input;

	lastValue = ((inputDouble > lastValue) ? attackCoefficient : releaseCoefficient)* (lastValue - inputDouble) + inputDouble;

	return (float)lastValue;
}

void EnvelopeFollower::AttackRelease::setSampleRate(double sampleRate_)
{
	sampleRate = sampleRate_;
	calculateCoefficients();
}

void EnvelopeFollower::AttackRelease::setRelease(float newRelease)
{
	release = newRelease;
	calculateCoefficients();
}

void EnvelopeFollower::AttackRelease::calculateCoefficients()
{
	if (sampleRate != -1.0)
	{
		attackCoefficient = exp(log(0.01) / (attack * sampleRate * 0.001));
		releaseCoefficient = exp(log(0.01) / (release * sampleRate * 0.001));
	}
}

}