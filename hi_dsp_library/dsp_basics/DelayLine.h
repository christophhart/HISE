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

#pragma once

namespace hise { using namespace juce;





template <int MaxLength=HISE_MAX_DELAY_TIME_SAMPLES, typename LockType=SpinLock, bool AllowFade=true> class DelayLine
{
    static constexpr int DELAY_BUFFER_SIZE = MaxLength;
    static constexpr int DELAY_BUFFER_MASK = MaxLength - 1;
    
public:
    
    using ScopedLockType = typename LockType::ScopedLockType;
    
	DelayLine();

	void prepareToPlay(double sampleRate_);

	void setDelayTimeSeconds(double delayInSeconds);

	void setDelayTimeSamples(int delayInSamples);

	void setFadeTimeSamples(int newFadeTimeInSamples);

	template <typename T> void processBlock(T& data)
	{
		processBlock(data.begin(), data.size());
	}

	void processBlock(float* data, int numValues);

	float getLastValue();

	float getDelayedValue(float inputValue);


	void clear();

	juce::String toDbgString();

private:

	void processSampleWithFade(float& f);

	void processSampleWithoutFade(float& f);

	void setInternalDelayTime(int delayInSamples);

	LockType processLock;

	double maxDelayTime;
	int currentDelayTime;
	double sampleRate;

	int lastIgnoredDelayTime;

	float delayBuffer[DELAY_BUFFER_SIZE];

	int readIndex;
	int oldReadIndex;
	int writeIndex;

	int fadeCounter;

	int fadeTimeSamples;
};



class AllpassDelay
{
public:
	AllpassDelay() :
		delay(0.f),
		currentValue(0.f)
	{}

	static float getDelayCoefficient(float delaySamples)
	{
		return (1.f - delaySamples) / (1.f + delaySamples);
	}

	void setDelay(float newDelay) noexcept { delay = newDelay; };

	void reset()
	{
		currentValue = 0.0f;
	}

	float getNextSample(float input) noexcept
	{
		float y = input * -delay + currentValue;
		currentValue = y * delay + input;

		return y;
	}

private:
	float delay, currentValue;
};

} 