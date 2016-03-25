/*
  ==============================================================================

    UpdateMerger.h
    Created: 19 Jun 2014 8:12:26pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef UPDATEMERGER_H_INCLUDED
#define UPDATEMERGER_H_INCLUDED

/** A counter which can be used to limit the frequency for eg. GUI updates
*	@ingroup utility
*
*	If set up correctly using either limitFromSampleRateToFrameRate() or limitFromBlockSizeToFrameRate(), it has an internal counter
*	that is incremented each time update() is called and returns true, if a new change message is due.
*	
*/
class UpdateMerger
{
public:
	/** creates a new UpdateMerger and registeres the given listener.*/
	UpdateMerger():
		frameRate(20),
        countLimit(0),
        updateCounter(0)
        
	{};

	/** A handy method to limit updates from buffer block to frame rate level.
	*	
	*	Use this if you intend to call update() every buffer block.
	*/
	void limitFromBlockSizeToFrameRate(double sampleRate, int blockSize) noexcept
	{
		limitFromSampleRateToFrameRate(sampleRate / blockSize);
	}

	/** sets a manual skip number. Use this if you don't need the fancy block -> frame conversion. */
	void setManualCountLimit(int skipAmount)
	{
		countLimit = skipAmount;

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
		if(++updateCounter == countLimit)
		{
			ScopedLock sl(lock);
			updateCounter = 0;
			return true;
		};
		return false;		
	};

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
			ScopedLock sl(lock);

			updateCounter = updateCounter % countLimit;
			return true;
		}

		return false;

	}

private:

	/** handy method to limit updates from sample rate level to frame rate level.
	*	
	*	Use this if you intend to call update() every sample.
	*/
	void limitFromSampleRateToFrameRate(double sampleRate) noexcept
	{
		countLimit = (int)(sampleRate) / frameRate;
		updateCounter = 0;
	};

	int frameRate;
	CriticalSection lock;
	int countLimit;
	volatile int updateCounter;
};

#if JUCE_DEBUG
#define GUI_UPDATER_FRAME_RATE 150
#else
#define GUI_UPDATER_FRAME_RATE 30
#endif


/** Utility class that reduces the update rate to a common framerate (~30 fps)
*
*	Unlike the UpdateMerger class, this class checks the time between calls to shouldUpdate() and returns true, if 30ms have passed since the last succesfull call to shouldUpdate().
*
*/
class GUIUpdater
{
public:

	GUIUpdater():
		timeOfLastCall(Time::currentTimeMillis()),
		timeOfDebugCall(Time::currentTimeMillis())
	{}

	/** Call this to check if the last update was longer than 30 ms ago. 
	*
	*	If debugInterval is true, then the interval between calls will be printed in debug mode.
	*/
	bool shouldUpdate(bool debugInterval=false)
	{
		const int64 currentTime = Time::currentTimeMillis();

#ifdef JUCE_DEBUG

		if (debugInterval)
		{
			const int64 delta = (currentTime - timeOfDebugCall);

			timeOfDebugCall = currentTime;

			DBG(delta);
		}

#endif

		if ((currentTime - timeOfLastCall) > GUI_UPDATER_FRAME_RATE)
		{
			timeOfLastCall = currentTime;
			return true;
		}

		return false;
	}

private:

	int64 timeOfLastCall;

	int64 timeOfDebugCall;
};



/** A lowpass filter that can be used to smooth parameter changes.
*	@ingroup utility
*/
class Smoother
{
public:

	/** Creates a new smoother. If you use this manually, you have to call prepareToPlay() and setSmoothingTime() before using it. */
	Smoother():
		active(false),
		sampleRate(-1.0f),
        smoothTime(0.0f)
	{ 
		a0 = b0 = x = currentValue = prevValue = 0.0f;
		
	};

	/** smooth the next sample. */
	float smooth(float newValue)
	{
		if(! active) return newValue;
		jassert(sampleRate > 0.0f);

		currentValue = a0 * newValue - b0 * prevValue;

		jassert(currentValue > -1000.0f);
		jassert(currentValue < 1000.0f);

		prevValue = currentValue;



		return currentValue;

	};

	/** Returns the smoothing time in seconds. */
	float getSmoothingTime() const
	{
		return smoothTime;
	};

	/** Sets the smoothing time in seconds. 
	*
	*	If you pass 0.0 as parameter, the smoother gets deactivated and saves precious CPU cycles.
	*/
	void setSmoothingTime(float newSmoothTime)
	{
		ScopedLock sl(lock);

		active = (newSmoothTime != 0.0f);

		smoothTime = newSmoothTime;

		const float freq = 1000.0f / newSmoothTime;

		x = expf(-2.0f * float_Pi * freq / sampleRate);
		a0 = 1.0f - x;
		b0 = -x;

	};

	/** Sets the internal sample rate. Call this method before setting the smooth time. */
	void prepareToPlay(double sampleRate_)
	{
		sampleRate = (float)sampleRate_;
	};

	/** Sets the internal value to the given number. Use this to prevent clicks for the first smoothing operation (default is 0.0) */
	void setDefaultValue(float value)
	{
		prevValue = value;
	}

private:

	CriticalSection lock;

	JUCE_LEAK_DETECTOR(Smoother)

	bool active;

	float sampleRate;

	float smoothTime;

	float a0, b0, x;

	float currentValue;
	float prevValue;
};


/** A Ramper applies linear ramping to a value.
*	@ingroup utility
*
*/
class Ramper
{
public:

	Ramper():
		targetValue(0.0f),
		stepDelta(0.0f),
		stepAmount(-1)
	{};


	/** Sets the step amount that the ramper will use. You can overwrite this value by supplying a step number in setTarget. */
	void setStepAmount(int newStepAmount) { stepAmount = newStepAmount; };

	/** sets the new target and recalculates the step size using either the supplied step number or the step amount previously set by setStepAmount(). */
	void setTarget(float currentValue, float newTarget, int numberOfSteps=-1)
	{
		if(numberOfSteps != -1) stepDelta = (newTarget - currentValue) / numberOfSteps;
		else if (stepAmount != -1) stepDelta = (newTarget - currentValue) / stepAmount;
		else jassertfalse; // Either the step amount should be set, or a new step amount should be supplied

		targetValue = newTarget;
	};

	/** Sets the ramper value and the target to the new value and stops ramping. */
	void setValue(float newValue)
	{
		targetValue = newValue;
		stepDelta = 0.0f;
	};

	/** ramps the supplied value and returns true if the targetValue is reached. */
	inline bool ramp(float &valueToChange) 
	{
		valueToChange += stepDelta;

		return fabs(targetValue - valueToChange) > 0.001;

		
	};

private:

	float targetValue, stepDelta;
	int stepAmount;

};


#endif  // UPDATEMERGER_H_INCLUDED
