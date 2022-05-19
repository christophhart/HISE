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

#define CONVOLUTION_RAMPING_TIME_MS 60


#define USE_FFT_CONVOLVER 1

class GainSmoother
{
public:

	enum class Parameters
	{
		Gain = 0,
		SmoothingTime,
		FastMode,
		numParameters
	};

	GainSmoother() :
		gain(1.0f),
		smoothingTime(200.0f),
		fastMode(true),
		lastValue(0.0f)
	{
		smoother.setDefaultValue(1.0f);
	};


	int getNumParameters() const { return (int)Parameters::numParameters; }

	float getParameter(int index) const
	{
		if (index == 0) return gain;
		else if (index == 1) return smoothingTime;
		else return -1;
	}

	void setParameter(int index, float newValue)
	{
		if (index == 0) gain = newValue;
		else if (index == 1) smoothingTime = newValue;
	}

	void prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
	{
		smoother.prepareToPlay(sampleRate);
		smoother.setSmoothingTime(smoothingTime);
	}

	void reset()
	{
		smoother.resetToValue(gain);
		lastValue = gain;
	}

	void processBlock(float** data, int numChannels, int numSamples);

	int getNumConstants() const
	{
		return (int)Parameters::numParameters;
	}

private:

	float gain;
	float smoothingTime;
	bool fastMode;

	float lastValue;

	Smoother smoother;
};

class MultithreadedConvolver : public fftconvolver::TwoStageFFTConvolver
{
	class BackgroundThread : public Thread
	{
	public:

		BackgroundThread(MultithreadedConvolver& parent_) :
			Thread("Convolution Background Thread"),
			parent(parent_)
		{

		}

		void run() override
		{
			while (!threadShouldExit())
			{

				if (active)
				{
					ScopedValueSetter<bool> svs(currentlyRendering, true);
					parent.doBackgroundProcessing();
					active = false;

					if (threadShouldExit())
						return;
				}

				wait(500);
			}
		};

		void setSomethingTodo(bool shouldBeActive)
		{
			active = shouldBeActive;
		}

		bool isBusy() const { return currentlyRendering; }

		bool currentlyRendering = false;
		bool active = false;

		MultithreadedConvolver& parent;
	};


public:

	MultithreadedConvolver(audiofft::ImplementationType fftType) :
		TwoStageFFTConvolver(fftType),
		backgroundThread(*this)
	{};

	virtual ~MultithreadedConvolver()
	{
		backgroundThread.stopThread(1000);
	};

	void startBackgroundProcessing() override
	{
		if (useBackgroundThread)
		{
			backgroundThread.setSomethingTodo(true);
			backgroundThread.notify();
		}
		else
		{
			doBackgroundProcessing();
		}
	}



	void waitForBackgroundProcessing() override
	{
		if (useBackgroundThread)
		{
			while (backgroundThread.isBusy())
				jassertfalse;
		}
	}

	static bool prepareImpulseResponse(const AudioSampleBuffer& originalBuffer, AudioSampleBuffer& buffer, bool* abortFlag, Range<int> range, double resampleRatio);

	static double getResampleFactor(double sampleRate, double impulseSampleRate);



	void setUseBackgroundThread(bool shouldBeUsingBackgroundThread, bool forceUpdate = false)
	{
		if (useBackgroundThread != shouldBeUsingBackgroundThread || forceUpdate)
		{
			useBackgroundThread = shouldBeUsingBackgroundThread;

			if (useBackgroundThread)
				backgroundThread.startThread(9);
			else
			{
				if (backgroundThread.isThreadRunning())
					backgroundThread.stopThread(1000);
			}

		}
	}

	bool isUsingBackgroundThread() const
	{
		return useBackgroundThread;
	}

private:


	BackgroundThread backgroundThread;

	bool useBackgroundThread = true;
};

struct ConvolutionEffectBase : public AsyncUpdater,
							   public NonRealtimeProcessor
{
	ConvolutionEffectBase();;
	virtual ~ConvolutionEffectBase();;

	void setImpulse(NotificationType n);

	void handleAsyncUpdate() override
	{
		reloadInternal();
	}

	void nonRealtimeModeChanged(bool isNonRealtime) override
	{
		nonRealtime = isNonRealtime;

		SimpleReadWriteLock::ScopedReadLock sl(swapLock);
		convolverL->setUseBackgroundThread(!nonRealtime && useBackgroundThread);
		convolverR->setUseBackgroundThread(!nonRealtime && useBackgroundThread);
	}

	virtual MultiChannelAudioBuffer& getImpulseBufferBase() = 0;
	virtual const MultiChannelAudioBuffer& getImpulseBufferBase() const = 0;

protected:

	void resetBase();

	void prepareBase(double sampleRate, int blockSize);

	void processBase(ProcessDataDyn& d);

	SimpleReadWriteLock swapLock;

	bool reloadInternal();

	bool useBackgroundThread = false;
	bool nonRealtime = false;
	bool processingEnabled = true;

	audiofft::ImplementationType currentType = audiofft::ImplementationType::numImplementationTypes;

	MultithreadedConvolver* createNewEngine(audiofft::ImplementationType fftType);

	double getResampleFactor() const
	{
		return MultithreadedConvolver::getResampleFactor(lastSampleRate, getImpulseBufferBase().sampleRate);
	}

	GainSmoother smoothedGainerWet;
	GainSmoother smoothedGainerDry;

	AudioSampleBuffer wetBuffer;

	void enableProcessing(bool shouldBeProcessed);

	std::atomic<bool> isCurrentlyProcessing;
	std::atomic<bool> loadAfterProcessFlag;

	bool smoothInputBuffer = false;
	bool rampFlag;
	bool rampUp;
	bool processFlag;
	int rampIndex;

	DelayLine<4096> leftPredelay;
	DelayLine<4096> rightPredelay;

	bool isUsingPoolData;

	bool isReloading;

	float dryGain;
	float wetGain;
	int latency;

	float damping = 1.0f;

	float predelayMs = 0.0f;

	ScopedPointer<MultithreadedConvolver> convolverL;
	ScopedPointer<MultithreadedConvolver> convolverR;

	double cutoffFrequency = 20000.0;

	double lastSampleRate = 0.0;
	int lastBlockSize = 0;

	void calcPredelay();

	/** Adds a exponential gain ramp to the impulse response. */
	static void applyExponentialFadeout(AudioSampleBuffer& scratchBuffer, int numSamples, float targetValue);

	/** Adds a 2-Pole Lowpass with an exponential curve to the impulse response. */
	static void applyHighFrequencyDamping(AudioSampleBuffer& buffer, int numSamples, double cutoffFrequency, double sampleRate);

	void calcCutoff();

private:

	bool prepareCalledOnce = false;
};

}

