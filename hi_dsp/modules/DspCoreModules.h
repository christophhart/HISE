/*
  ==============================================================================

    DspCoreModules.h
    Created: 10 Jul 2016 1:00:04pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef DSPCOREMODULES_H_INCLUDED
#define DSPCOREMODULES_H_INCLUDED

namespace hise { using namespace juce;

//#define DELAY_BUFFER_SIZE 65536
//#define DELAY_BUFFER_MASK 65536-1

template <int MaxLength=65536, typename LockType=SpinLock, bool AllowFade=true> class DelayLine
{
    static constexpr int DELAY_BUFFER_SIZE = MaxLength;
    static constexpr int DELAY_BUFFER_MASK = MaxLength - 1;
    
public:
    
    using ScopedLockType = typename LockType::ScopedLockType;
    
	DelayLine() :
		readIndex(0),
		oldReadIndex(0),
		writeIndex(0),
		lastIgnoredDelayTime(-1),
		sampleRate(44100.0), // better safe than sorry...
		currentDelayTime(0),
		fadeCounter(-1),
		fadeTimeSamples(1024)
	{
		FloatVectorOperations::clear(delayBuffer, DELAY_BUFFER_SIZE);
	}

	void prepareToPlay(double sampleRate_)
	{
        ScopedLockType sl(processLock);

		sampleRate = sampleRate_;
	}

	void setDelayTimeSeconds(double delayInSeconds)
	{
		setDelayTimeSamples((int)(delayInSeconds * sampleRate));
	}

	void setDelayTimeSamples(int delayInSamples)
	{
		ScopedLockType sl(processLock);

		setInternalDelayTime(delayInSamples);
	}

	void setFadeTimeSamples(int newFadeTimeInSamples)
	{
		ScopedLockType sl(processLock);

		fadeTimeSamples = newFadeTimeInSamples;
	}

	template <typename T> void processBlock(T& data)
	{
		processBlock(data.begin(), data.size());
	}

	void processBlock(float* data, int numValues)
	{
		ScopedLockType sl(processLock);

		if (fadeCounter < 0)
		{
			for (int i = 0; i < numValues; i++)
			{
				processSampleWithoutFade(data[i]);
			}
		}
		else
		{
			for (int i = 0; i < numValues; i++)
			{
				if (fadeTimeSamples == 0 || fadeCounter < 0)	
					processSampleWithoutFade(data[i]);
				else					
					processSampleWithFade(data[i]);
			}
		}
	}

	float getLastValue()
	{
		return delayBuffer[readIndex];
	}

	float getDelayedValue(float inputValue)
	{
		ScopedLockType sl(processLock);

		if (fadeTimeSamples == 0 || fadeCounter < 0)	
			processSampleWithoutFade(inputValue);
		else					
			processSampleWithFade(inputValue);

		return inputValue;
	}


	void clear()
	{
		memset(delayBuffer, 0, sizeof(float) * currentDelayTime);

		writeIndex = currentDelayTime;
		readIndex = 0;
		fadeCounter = -1;

	}

	String toDbgString()
	{
		String s;

		s << "currentDelayTime: " << currentDelayTime << ", ";
		s << "readIndex: " << readIndex << ",";
		s << "writeIndex: " << writeIndex << ", ";
		return s;
	}

private:

	void processSampleWithFade(float& f)
	{
		jassert(fadeTimeSamples != 0);

		delayBuffer[writeIndex++] = f;

		const float oldValue = delayBuffer[oldReadIndex++];
		const float newValue = delayBuffer[readIndex++];

		const float mix = (float)fadeCounter / (float)fadeTimeSamples;

		f = newValue * mix + oldValue * (1.0f - mix);

		oldReadIndex &= DELAY_BUFFER_MASK;
		readIndex &= DELAY_BUFFER_MASK;
		writeIndex &= DELAY_BUFFER_MASK;

		fadeCounter++;

		if (fadeCounter >= fadeTimeSamples)
		{
			fadeCounter = -1;
			if (lastIgnoredDelayTime != 0)
			{
				setInternalDelayTime(lastIgnoredDelayTime);
			}
		}
	}

	void processSampleWithoutFade(float& f)
	{
		jassert(isPositiveAndBelow(readIndex, DELAY_BUFFER_SIZE));
		jassert(isPositiveAndBelow(writeIndex, DELAY_BUFFER_SIZE));

		delayBuffer[writeIndex++] = f;

		f = delayBuffer[readIndex++];

		readIndex &= DELAY_BUFFER_MASK;
		writeIndex &= DELAY_BUFFER_MASK;
	}

	void setInternalDelayTime(int delayInSamples)
	{
		delayInSamples = jmin<int>(delayInSamples, DELAY_BUFFER_SIZE - 1);

		if (fadeTimeSamples > 0 && fadeCounter > 0)
		{
			lastIgnoredDelayTime = delayInSamples;
			return;
		}
		else
		{
			lastIgnoredDelayTime = 0;
		}

		currentDelayTime = delayInSamples;

		oldReadIndex = readIndex;

		fadeCounter = 0;

		readIndex = (writeIndex - delayInSamples) & DELAY_BUFFER_MASK;
	}

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


} // namespace hise

#endif  // DSPCOREMODULES_H_INCLUDED
