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

#ifndef UPDATEMERGER_H_INCLUDED
#define UPDATEMERGER_H_INCLUDED

namespace hise { using namespace juce;

/** A collection of little helper functions to clean float arrays.
*    @ingroup utility
*
*    Source: http://musicdsp.org/showArchiveComment.php?ArchiveID=191
*/
struct FloatSanitizers
{
    template <typename ContainerType> static void sanitizeArray(ContainerType& d)
    {
        for (auto& s : d)
            sanitizeFloatNumber(s);
    }

    /** Returns the silence threshold as gain factor. Uses the HISE_SILENCE_THRESHOLD_DB preprocessor. */
    static bool isSilence(const float value)
    {
        static const float Silence = std::pow(10.0f, (float)HISE_SILENCE_THRESHOLD_DB * -0.05f);
        static const float MinusSilence = -1.0f * Silence;
        return value < Silence && value > MinusSilence;
    }
    
    static bool isNotSilence(const float value)
    {
        return !isSilence(value);
    }
    
    static void sanitizeArray(float* data, int size);;

    static float sanitizeFloatNumber(float& input);;

    struct Test : public UnitTest
    {
        Test() :
            UnitTest("Testing float sanitizer")
        {

        };

        void runTest() override;
    };
};

static FloatSanitizers::Test floatSanitizerTest;

class FallbackRamper
{
public:

	FallbackRamper(float* data, int numValues_):
		d(data),
		numValues(numValues_)
	{};

	float ramp(float startValue, float delta1)
	{
		float value = startValue;

		while (--numValues >= 0)
		{
			*d++ = value;
			value += delta1;
		}

		return value;
	}

private:

	float* d;
	int numValues;

};

template <int RampLength> class AlignedSSERamper
{
public:

	AlignedSSERamper(float* data_) :
		data(data_)
	{
		jassert(dsp::SIMDRegister<float>::isSIMDAligned(data));
	}
	

	void ramp(float startValue, float delta1)
	{
#if JUCE_LINUX

		for (int i = 0; i < RampLength; i+= 4)
		{
			data[i] = startValue;
			data[i+1] = startValue + delta1;
			data[i + 2] = startValue + delta1*2.0f;
			data[i + 3] = startValue + delta1*3.0f;
			startValue += delta1 * 4.0f;
		}

#else
		using SSEType = dsp::SIMDRegister<float>;

		constexpr int numSSE = SSEType::SIMDRegisterSize / sizeof(float);
		constexpr int numLoop = RampLength / numSSE;

		SSEType deltaConstant(delta1);
		SSEType step = deltaConstant * 4.0f;
        
#if JUCE_WINDOWS
		SSEType r = SSEType::fromNative({ 0.0f, 1.0f, 2.0f, 3.0f });
#else
        SSEType r = SSEType::fromNative({0.0f, 1.0f, 2.0f, 3.0f});
#endif
		SSEType deltaRamp = deltaConstant * r;
		deltaRamp += startValue;
		
		for (int i = 0; i < numLoop; i++)
		{
			deltaRamp.copyToRawArray(data);
			deltaRamp += step;
			data += numSSE;
		}
#endif
	}

private:

	float* data;
};

#define USE_BLOCK_DIVIDER_STATISTICS 1

struct BlockDividerStatistics
{
public:

	static void resetStatistics()
	{
		numAlignedCalls = 0;
		numOddCalls = 0;
	}

	static void incCounter(bool aligned)
	{
#if USE_BLOCK_DIVIDER_STATISTICS
		aligned ? numAlignedCalls++ : numOddCalls++;
#endif
	}

	static int getAlignedCallPercentage()
	{
		const int total = numAlignedCalls + numOddCalls;
		auto p = total != 0 ? ((double)numAlignedCalls / (double)total) : 0.0;
		return roundToInt(p * 100.0);
	}



private:

	static int numAlignedCalls;
	static int numOddCalls;
};

/** This class divides a block into fixed chunks of data.
*
*	It can be used to divide a block of audio data into smaller chunks
*	and takes care of the edge cases when using SSE processing.
*	
*/
template <int SkipAmount, typename FloatType=float> class BlockDivider
{
public:

	/** checks the loop counter and returns the number of samples that have to be calculated manually. 
	*
	*	If it returns zero, you can use the entire blocksize specified by the SkipAmount.
	*	It also guarantees SSE alignment so you can write a SSE loop without edge case handling.
	*	
	*/
	int cutBlock(int& loopCounter, bool& newBlock, FloatType* pointerToCheck)
	{
		if (counter != 0)
		{
			newBlock = false;
			const int numToCutThisTime = jmin<int>(loopCounter, SkipAmount - counter);

			counter = (counter + numToCutThisTime) % SkipAmount;
			loopCounter -= numToCutThisTime;

			BlockDividerStatistics::incCounter(false);

			return numToCutThisTime;
		}

		if (loopCounter < SkipAmount)
		{
			jassert(counter == 0);

			newBlock = true;
			counter += loopCounter;

			const int returnValue = loopCounter;
			loopCounter = 0;

			BlockDividerStatistics::incCounter(false);

			return returnValue;
		}
		else
		{
			jassert(counter == 0);
			
			newBlock = true;

			const bool aligned = dsp::SIMDRegister<FloatType>::isSIMDAligned(pointerToCheck);

			BlockDividerStatistics::incCounter(aligned);

			loopCounter -= SkipAmount;

			return (1-(int)aligned) * SkipAmount;
		}
	}

	void reset()
	{
		counter = 0;
	}

private:

	int counter = 0;
};

/** A counter which can be used to limit the frequency for eg. GUI updates
*	@ingroup utility
*
*	If set up correctly using either limitFromSampleRateToFrameRate() or limitFromBlockSizeToFrameRate(), it has an internal counter
*	that is incremented each time update() is called and returns true, if a new change message is due.
*	
*/
template <class Locktype=SpinLock> class ExecutionLimiter
{
public:
	/** creates a new UpdateMerger and registeres the given listener.*/
	ExecutionLimiter():
        countLimit(1),
        updateCounter(0)
	{};

	/** A handy method to limit updates from buffer block to frame rate level.
	*	
	*	Use this if you intend to call update() every buffer block.
	*/
	void limitFromBlockSizeToFrameRate(double sampleRate, int blockSize) noexcept
	{
		if(blockSize > 0)
			limitFromBlockRateToFrameRate(sampleRate / (double)blockSize);
	}

	/** sets a manual skip number. Use this if you don't need the fancy block -> frame conversion. */
	void setManualCountLimit(int skipAmount)
	{
		typename Locktype::ScopedLockType sl(processLock);

		countLimit = jmax(1, skipAmount);
		updateCounter = 0;
	};

	/** Call this method whenever something changes and the UpdateMerger class will check if a update is necessary.
	*
	*	@return \c true if a update should be made or \c false if not.
	*/
	inline bool shouldUpdate() noexcept
	{
		// the limit rate has not been set!
		jassert(countLimit > 0);
		if(++updateCounter >= countLimit)
		{
			typename Locktype::ScopedLockType sl(processLock);
			updateCounter = 0;
			return true;
		};
		return false;		
	};

	inline void advance(int numSteps)
	{
		updateCounter += numSteps;
	}

	/** Call this method whenever something changes and the UpdateMerger class will check if a update is necessary.
	*
	*	You can pass a step amount if you want to merge some steps. If the count limit is reached, the overshoot will be
	*	retained.
	*/
	inline bool shouldUpdate(int stepsToSkip)
	{
		jassert(countLimit > 0);
		
		updateCounter += stepsToSkip;

		if(updateCounter >= countLimit)
		{
			typename Locktype::ScopedLockType sl(processLock);

			updateCounter = updateCounter % countLimit;
			return true;
		}

		return false;

	}

private:

	Locktype processLock;

	void limitFromBlockRateToFrameRate(double blocksPerSeconds) noexcept
	{
		countLimit = jmax(1, roundToInt(blocksPerSeconds / frameRate));
		updateCounter = 0;
	};

	double frameRate = 30.0;
	
	int countLimit;
	volatile int updateCounter;
};


using UpdateMerger = ExecutionLimiter<SpinLock>;


/** A Ramper applies linear ramping to a value.
*	@ingroup utility
*
*/
class Ramper
{
public:

	Ramper() :
		targetValue(0.0f),
		stepDelta(0.0f),
		stepAmount(-1)
	{};

	/** Sets the step amount that the ramper will use. You can overwrite this value by supplying a step number in setTarget. */
	void setStepAmount(int newStepAmount) { stepAmount = newStepAmount; };

	/** sets the new target and recalculates the step size using either the supplied step number or the step amount previously set by setStepAmount(). */
	void setTarget(float currentValue, float newTarget, int numberOfSteps = -1)
	{
		if (numberOfSteps != -1) stepDelta = (newTarget - currentValue) / numberOfSteps;
		else if (stepAmount != -1) stepDelta = (newTarget - currentValue) / stepAmount;
		else jassertfalse; // Either the step amount should be set, or a new step amount should be supplied

		targetValue = newTarget;
		busy = true;
	};

	/** Sets the ramper value and the target to the new value and stops ramping. */
	void setValue(float newValue)
	{
		targetValue = newValue;
		stepDelta = 0.0f;
		busy = false;
	};

	/** ramps the supplied value and returns true if the targetValue is reached. */
	inline bool ramp(float &valueToChange)
	{
		valueToChange += stepDelta;
		busy = FloatSanitizers::isNotSilence(targetValue - valueToChange);
		return busy;
	};

	bool isBusy() const { return busy; }

private:

	bool busy = false;
	float targetValue, stepDelta;
	int stepAmount;

};


/** A lowpass filter that can be used to smooth parameter changes.
*	@ingroup utility
*/
template <int DownsamplingFactor> class DownsampledSmoother
{
public:

	/** Creates a new smoother. If you use this manually, you have to call prepareToPlay() and setSmoothingTime() before using it. */
	DownsampledSmoother():
		active(false),
		sampleRate(-1.0f),
        smoothTime(0.0f)
	{ 
		a0 = b0 = currentValue = prevValue = 0.0f;
		
	};

    forcedinline float smoothRaw(float a0newValue) noexcept
    {
        prevValue *= minusb0;
        prevValue += a0newValue;
        return prevValue;
    }
    
    float getA0() const noexcept { return a0; }
    
    
	/** smooth the next sample. */
	float smooth(float newValue)
	{
		SpinLock::ScopedLockType sl(spinLock);

		if(! active) return newValue;
		jassert(sampleRate > 0.0f);

		currentValue = a0 * newValue - b0 * prevValue;

		jassert(currentValue >= -1100.0f);
		jassert(currentValue <= 1100.0f);

		prevValue = currentValue;

		

		return currentValue;
	};

	bool isSmoothingActive() const
	{
		return smoothingActive;
	}


	void smoothBuffer(float* data, int numSamples)
	{
		if (!active) return;

		jassert(sampleRate > 0.0);

		for (int i = 0; i < numSamples; i++)
		{
			currentValue = a0 * data[i] - b0 * prevValue;
			prevValue = currentValue;
			data[i] = currentValue;
		}
	}

	/** Returns the smoothing time in seconds. */
	float getSmoothingTime() const
	{
		return smoothTime;
	};

	void resetToValue(float targetValue, float ramptimeMilliseconds=0.0f)
	{
		if (ramptimeMilliseconds > 0.0f)
		{
			auto rampLengthSamples = roundToInt(ramptimeMilliseconds / 1000.0f * sampleRate);

			resetRamper.setTarget(currentValue, targetValue, rampLengthSamples);
		}
		else
		{
			currentValue = targetValue;
			downsampledRampValue = targetValue;
			downsampledTargetValue = targetValue;
			resetRamper.setValue(targetValue);
			prevValue = currentValue;
		}
	}

	/** Sets the smoothing time in milliseconds. 
	*
	*	If you pass 0.0 as parameter, the smoother gets deactivated and saves precious CPU cycles.
	*/
	void setSmoothingTime(float newSmoothTime)
	{
		SpinLock::ScopedLockType sl(spinLock);

		active = (newSmoothTime != 0.0f);

		smoothTime = newSmoothTime;

		if (sampleRate > 0.0)
		{
			const float freq = 1000.0f / newSmoothTime;

			const float x = expf(-2.0f * float_Pi * freq / sampleRate);
			a0 = 1.0f - x;
			b0 = -x;
            minusb0 = x;
		}
	};

	/** Sets the internal sample rate. Call this method before setting the smooth time. */
	void prepareToPlay(double sampleRate_)
	{
		sampleRate = (float)sampleRate_ / (float)DownsamplingFactor;
		setSmoothingTime(smoothTime);
	};

	/** Sets the internal value to the given number. Use this to prevent clicks for the first smoothing operation (default is 0.0) */
	void setDefaultValue(float value)
	{
		prevValue = value;
	}

    float getDefaultValue() const
    {
        return prevValue;
    }
    
private:

	float downsampledRampValue = 1.0f;
	float downsampledTargetValue = 1.0f;
	float currentRampDelta = 0.0f;

	Ramper resetRamper;

	BlockDivider<DownsamplingFactor> blockDivider;

	SpinLock spinLock;

	JUCE_LEAK_DETECTOR(DownsampledSmoother)

	bool active;

	bool smoothingActive = false;

	float sampleRate;

	float smoothTime;

    float a0;
    float b0;
    
    float currentValue;
    float prevValue;
    float minusb0;
};

using Smoother = DownsampledSmoother<1>;

} // namespace hise

#endif  // UPDATEMERGER_H_INCLUDED
