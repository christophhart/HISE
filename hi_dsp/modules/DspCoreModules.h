/*
  ==============================================================================

    DspCoreModules.h
    Created: 10 Jul 2016 1:00:04pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef DSPCOREMODULES_H_INCLUDED
#define DSPCOREMODULES_H_INCLUDED



#define DELAY_BUFFER_SIZE 65536
#define DELAY_BUFFER_MASK 65536-1

class DelayLine
{
public:

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
		SpinLock::ScopedLockType sl(processLock);

		sampleRate = sampleRate_;
	}

	void setDelayTimeSeconds(double delayInSeconds)
	{
		setDelayTimeSamples((int)(delayInSeconds * sampleRate));
	}

	void setDelayTimeSamples(int delayInSamples)
	{
		SpinLock::ScopedLockType sl(processLock);

		setInternalDelayTime(delayInSamples);
	}

	void setFadeTimeSamples(int newFadeTimeInSamples)
	{
		SpinLock::ScopedLockType sl(processLock);

		fadeTimeSamples = newFadeTimeInSamples;
	}

	float getDelayedValue(float inputValue)
	{
		SpinLock::ScopedLockType sl(processLock);

		delayBuffer[writeIndex++] = inputValue;

		if (fadeCounter < 0)
		{
			const float returnValue = delayBuffer[readIndex++];

			readIndex &= DELAY_BUFFER_MASK;
			writeIndex &= DELAY_BUFFER_MASK;
			return returnValue;
		}
		else
		{
			const float oldValue = delayBuffer[oldReadIndex++];
			const float newValue = delayBuffer[readIndex++];

			const float mix = (float)fadeCounter / (float)fadeTimeSamples;

			const float returnValue = newValue * mix + oldValue * (1.0f - mix);

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

			return returnValue;
		}
	}

private:

	void setInternalDelayTime(int delayInSamples)
	{
		delayInSamples = jmin<int>(delayInSamples, DELAY_BUFFER_SIZE - 1);

		if (fadeCounter != -1)
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

	SpinLock processLock;

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



#endif  // DSPCOREMODULES_H_INCLUDED
