namespace hise
{
using namespace juce;

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::processBlock(float* data, int numValues)
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

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
float hise::DelayLine<MaxLength, LockType, AllowFade>::getDelayedValue(float inputValue)
{
	ScopedLockType sl(processLock);

	if (fadeTimeSamples == 0 || fadeCounter < 0)
		processSampleWithoutFade(inputValue);
	else
		processSampleWithFade(inputValue);

	return inputValue;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::clear()
{
	memset(delayBuffer, 0, sizeof(float) * currentDelayTime);

	writeIndex = currentDelayTime;
	readIndex = 0;
	fadeCounter = -1;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
String hise::DelayLine<MaxLength, LockType, AllowFade>::toDbgString()
{
	String s;

	s << "currentDelayTime: " << currentDelayTime << ", ";
	s << "readIndex: " << readIndex << ",";
	s << "writeIndex: " << writeIndex << ", ";
	return s;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::processSampleWithFade(float& f)
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

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::processSampleWithoutFade(float& f)
{
	jassert(isPositiveAndBelow(readIndex, DELAY_BUFFER_SIZE));
	jassert(isPositiveAndBelow(writeIndex, DELAY_BUFFER_SIZE));

	delayBuffer[writeIndex++] = f;

	f = delayBuffer[readIndex++];

	readIndex &= DELAY_BUFFER_MASK;
	writeIndex &= DELAY_BUFFER_MASK;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::setInternalDelayTime(int delayInSamples)
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

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
float hise::DelayLine<MaxLength, LockType, AllowFade>::getLastValue()
{
	return delayBuffer[readIndex];
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::setFadeTimeSamples(int newFadeTimeInSamples)
{
	ScopedLockType sl(processLock);

	fadeTimeSamples = newFadeTimeInSamples;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::setDelayTimeSamples(int delayInSamples)
{
	ScopedLockType sl(processLock);

	setInternalDelayTime(delayInSamples);
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::setDelayTimeSeconds(double delayInSeconds)
{
	setDelayTimeSamples((int)(delayInSeconds * sampleRate));
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
void hise::DelayLine<MaxLength, LockType, AllowFade>::prepareToPlay(double sampleRate_)
{
	ScopedLockType sl(processLock);

	sampleRate = sampleRate_;
}

template <int MaxLength/*=65536*/, typename LockType/*=SpinLock*/, bool AllowFade/*=true*/>
hise::DelayLine<MaxLength, LockType, AllowFade>::DelayLine() :
	readIndex(0),
	oldReadIndex(0),
	writeIndex(0),
	lastIgnoredDelayTime(-1),
	sampleRate(44100.0), // better safe than sorry...
	currentDelayTime(0),
	fadeCounter(-1),
	fadeTimeSamples(1024)
{
	static_assert(isPowerOfTwo(MaxLength), "delay line length must be power of two");

	FloatVectorOperations::clear(delayBuffer, DELAY_BUFFER_SIZE);
}
}