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

class MultithreadedConvolver : public fftconvolver::TwoStageFFTConvolver,
                               public ReferenceCountedObject
{
public:
    
    using Ptr = ReferenceCountedObjectPtr<MultithreadedConvolver>;
    
	class BackgroundThread : public Thread
	{
	public:

		BackgroundThread() :
          Thread("Convolution Background Thread"),
          queue(512)
		{}

        ~BackgroundThread()
        {
            soonToBeDeleted.clear();
            jassert(numRegisteredConvolvers == 0);
            
            stopThread(1000);
            queue.callForEveryElementInQueue({});
            
        }
        
		void run() override
		{
			while (!threadShouldExit())
			{
                {
                    ScopedValueSetter<bool> svs(currentlyRendering, true);
                    
                    queue.callForEveryElementInQueue([this](MultithreadedConvolver::Ptr c)
                    {
                        if (threadShouldExit())
                            return false;
                        
                        c->doBackgroundProcessing();
                        c->pending.store(false);
                        
                        return true;
                    });
                }
                
                ReferenceCountedArray<MultithreadedConvolver> copy;
                
                if(!soonToBeDeleted.isEmpty())
                {
                    SpinLock::ScopedLockType sl(deleteLock);
                    copy.swapWith(soonToBeDeleted);
                }
                
                copy.clear();
                
				wait(500);
			}
		};

        void addConvolverJob(MultithreadedConvolver::Ptr c)
        {
            queue.push(c);
            notify();
        }
        
        void addConvolverToBeDeleted(MultithreadedConvolver::Ptr c)
        {
            SpinLock::ScopedLockType sl(deleteLock);
            soonToBeDeleted.add(c);
        }
        
		bool isBusy() const { return currentlyRendering; }
		bool currentlyRendering = false;
		
        int numRegisteredConvolvers = 0;
        
        hise::LockfreeQueue<MultithreadedConvolver::Ptr> queue;
        
        SpinLock deleteLock;
        
        ReferenceCountedArray<MultithreadedConvolver> soonToBeDeleted;
	};


public:

	MultithreadedConvolver(audiofft::ImplementationType fftType) :
		TwoStageFFTConvolver(fftType),
		backgroundThread(nullptr)
	{};

	virtual ~MultithreadedConvolver()
	{
        jassert(!pending);
        
        if(backgroundThread != nullptr)
            backgroundThread->numRegisteredConvolvers--;
	};

	void startBackgroundProcessing() override
	{
        pending.store(true);
        
		if (backgroundThread != nullptr)
		{
            backgroundThread->addConvolverJob(this);
		}
		else
		{
			doBackgroundProcessing();
            pending.store(false);
		}
	}

	void waitForBackgroundProcessing() override
	{
        while (pending.load())
            jassertfalse;
	}

	static bool prepareImpulseResponse(const AudioSampleBuffer& originalBuffer, AudioSampleBuffer& buffer, bool* abortFlag, Range<int> range, double resampleRatio);

	static double getResampleFactor(double sampleRate, double impulseSampleRate);

	void setUseBackgroundThread(BackgroundThread* newThreadToUse, bool forceUpdate = false)
	{
		if (backgroundThread != newThreadToUse || forceUpdate)
        {
            if(backgroundThread != nullptr)
                backgroundThread->numRegisteredConvolvers--;
            
            backgroundThread = newThreadToUse;
            
            if(backgroundThread != nullptr)
                backgroundThread->numRegisteredConvolvers++;
            
            if (backgroundThread != nullptr && !backgroundThread->isThreadRunning())
                backgroundThread->startThread(10);
        }
	}

	bool isUsingBackgroundThread() const
	{
		return backgroundThread != nullptr;
	}

private:

    std::atomic<bool> pending = { false };
    
    BackgroundThread* backgroundThread = nullptr;
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
        
        auto tToUse = !nonRealtime && useBackgroundThread ? &backgroundThread : nullptr;
        
        convolverL->setUseBackgroundThread(tToUse);
		convolverR->setUseBackgroundThread(tToUse);
	}

	virtual MultiChannelAudioBuffer& getImpulseBufferBase() = 0;
	virtual const MultiChannelAudioBuffer& getImpulseBufferBase() const = 0;

protected:

    MultithreadedConvolver::BackgroundThread backgroundThread;
    
	void resetBase();

	void prepareBase(double sampleRate, int blockSize);

	void processBase(ProcessDataDyn& d);

	SimpleReadWriteLock swapLock;

	bool reloadInternal();

	bool useBackgroundThread = false;
	bool nonRealtime = false;
	bool processingEnabled = true;

	audiofft::ImplementationType currentType = audiofft::ImplementationType::numImplementationTypes;

	MultithreadedConvolver::Ptr createNewEngine(audiofft::ImplementationType fftType);

	double getResampleFactor() const
	{
		return MultithreadedConvolver::getResampleFactor(lastSampleRate, getImpulseBufferBase().sampleRate);
	}

	GainSmoother smoothedGainerWet;
	GainSmoother smoothedGainerDry;

	AudioSampleBuffer wetBuffer;
    AudioSampleBuffer fadeBuffer;
    
    float fadeValue = 0.0f;
    float fadeDelta = 0.001f;

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

	MultithreadedConvolver::Ptr convolverL;
	MultithreadedConvolver::Ptr convolverR;

    MultithreadedConvolver::Ptr fadeOutConvolverL;
    MultithreadedConvolver::Ptr fadeOutConvolverR;
    
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

