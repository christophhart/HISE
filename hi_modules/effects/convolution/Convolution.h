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

#ifndef CONVOLUTION_H_INCLUDED
#define CONVOLUTION_H_INCLUDED

namespace hise { using namespace juce;

#define CONVOLUTION_RAMPING_TIME_MS 30


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

	void setUseBackgroundThread(bool shouldBeUsingBackgroundThread)
	{
		if (useBackgroundThread != shouldBeUsingBackgroundThread)
		{
			useBackgroundThread = shouldBeUsingBackgroundThread;
		
			if (useBackgroundThread)
				backgroundThread.startThread(9);
			else
			{
				if(backgroundThread.isThreadRunning())
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


/** @brief A convolution reverb using zero-latency convolution
*	@ingroup effectTypes
*
*	This is a wrapper for the convolution engine found in WDL (the sole MIT licenced convolution engine available)
*	It is not designed to replace real convolution reverbs (as the CPU usage for impulses > 0.6 seconds is unreasonable),
*	but your early reflection impulses or other filter impulses will be thankful for this effect.
*/
class ConvolutionEffect: public MasterEffectProcessor,
						 public AudioSampleProcessor,
						 public NonRealtimeProcessor
{

	class LoadingThread : public Thread
	{
	public:

		LoadingThread(ConvolutionEffect& parent_) :
			Thread("Convolution Impulse Calculation"),
			parent(parent_)
		{
			
		}

		~LoadingThread()
		{
			stopThread(1000);
		}

		

		void run() override;

		ConvolutionEffect& parent;

		void reloadImpulse()
		{
			if (!isThreadRunning())
			{
				startThread(5);
			}

			if (pending)
				shouldRestart = true;
			else
				pending = true;

			//startTimer(30);
		};

		
	private:

		bool pending = false;

		bool reloadInternal();

#if 0
		void timerCallback() override
		{
			if (parent.rampFlag)
				return;

			if (isReloading)
			{
				shouldRestart = true;
			}

			shouldReload = true;

			ScopedLock sl(parent.getImpulseLock());

			parent.convolverL->reset();
			parent.convolverR->reset();
		}
#endif

		bool shouldRestart = false;
		bool shouldReload = false;

	};

public:

	SET_PROCESSOR_NAME("Convolution", "Convolution Reverb", "A convolution reverb effect.");

	// ============================================================================================= Constructor / Destructor / enums

	enum Parameters
	{
		DryGain = 0, ///< the gain of the unprocessed input
		WetGain, ///< the gain of the convoluted input
		Latency, ///< you can change the latency (unused)
		ImpulseLength, ///< the Impulse length (deprecated, use the SampleArea of the AudioSampleBufferComponent to change the impulse response)
		ProcessInput, ///< if this attribute is set, the engine will fade out in a short time and reset itself.
		UseBackgroundThread, ///< if true, then the tail of the impulse response will be rendered on a background thread to save cycles on the audio thread.
		Predelay, ///< delays the reverb tail by the given amount in milliseconds
		HiCut, ///< applies a low pass filter to the impulse response
		Damping, ///< applies a fade-out to the impulse response
		FFTType, ///< the FFT implementation. It picks the best available but for some weird use cases you can force to use another one.
		numEffectParameters
	};

	ConvolutionEffect(MainController *mc, const String &id);;

	~ConvolutionEffect();
	

	// ============================================================================================= Convolution methods

	void newFileLoaded() override {	setImpulse(); }
	void rangeUpdated() override { setImpulse(); }
	void setImpulse();

	// ============================================================================================= MasterEffect methods

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override {return true; };

	void voicesKilled() override
	{
		convolverL->cleanPipeline();
		convolverR->cleanPipeline();
		leftPredelay.clear();
		rightPredelay.clear();
	}

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	const CriticalSection& getFileLock() const override { return unusedFileLock; }

	void nonRealtimeModeChanged(bool isNonRealtime) override
	{
		nonRealtime = isNonRealtime;

		convolverL->setUseBackgroundThread(!nonRealtime && useBackgroundThread);
		convolverR->setUseBackgroundThread(!nonRealtime && useBackgroundThread);
	}
	

private:

	bool useBackgroundThread = false;
	bool nonRealtime = false;

	bool processingEnabled = true;

	audiofft::ImplementationType currentType = audiofft::ImplementationType::numImplementationTypes;

	void createEngine(audiofft::ImplementationType fftType);

	SpinLock swapLock;

	LoadingThread loadingThread;

	double getResampleFactor() const
	{
		const auto renderingSampleRate = getSampleRate();
		const auto bufferSampleRate = getSampleRateForLoadedFile();

		return MultithreadedConvolver::getResampleFactor(renderingSampleRate, bufferSampleRate);
	}

	CriticalSection unusedFileLock;

	GainSmoother smoothedGainerWet;
	GainSmoother smoothedGainerDry;

	AudioSampleBuffer wetBuffer;

	const CriticalSection& getImpulseLock() const { return lock; };

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

	CriticalSection lock;

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
	void calcPredelay();

	/** Adds a exponential gain ramp to the impulse response. */
	static void applyExponentialFadeout(AudioSampleBuffer& scratchBuffer, int numSamples, float targetValue);

	/** Adds a 2-Pole Lowpass with an exponential curve to the impulse response. */
	static void applyHighFrequencyDamping(AudioSampleBuffer& buffer, int numSamples, double cutoffFrequency, double sampleRate);

	void calcCutoff();
};


} // namespace hise

namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace filters
{

struct convolution : public AudioFileNodeBase
{
	SET_HISE_NODE_ID("convolution");
	GET_SELF_AS_OBJECT(convolution);

	void prepare(PrepareSpecs specs) override
	{
		DspHelpers::setErrorIfFrameProcessing(specs);

		lastSampleRate = specs.sampleRate;
		largestBlockSize = specs.blockSize;

		OwnedArray<MultithreadedConvolver> newConvolvers;

		for (int i = 0; i < specs.numChannels; i++)
		{
			newConvolvers.add(new MultithreadedConvolver(audiofft::ImplementationType::BestAvailable));
		}

		{
			SpinLock::ScopedLockType sl(lock);
			convolvers.swapWith(newConvolvers);
		}

		rebuildImpulse();
	}

	void rebuildImpulse()
	{
		AudioSampleBuffer impulseBuffer;

		auto ratio = MultithreadedConvolver::getResampleFactor(lastSampleRate, audioFile->getSampleRate());

		{
			SpinLock::ScopedLockType sl(lock);
			MultithreadedConvolver::prepareImpulseResponse(currentBuffer->range, impulseBuffer, nullptr, {}, ratio);
		}
		
		if (impulseBuffer.getNumSamples() == 0)
		{
			SpinLock::ScopedLockType sl(impulseLock);

			for (auto c : convolvers)
				c->reset();

			return;
		}

		const auto headSize = nextPowerOfTwo(largestBlockSize);
		const auto fullTailLength = nextPowerOfTwo(impulseBuffer.getNumSamples() - headSize);

		{
			SpinLock::ScopedLockType sl(impulseLock);

			for (int i = 0; i < convolvers.size(); i++)
			{
				int channelIndex = i % impulseBuffer.getNumChannels();
				auto c = convolvers[i];

				c->init(headSize, fullTailLength, impulseBuffer.getReadPointer(channelIndex), impulseBuffer.getNumSamples());
			}
		}
		
		reset();
	}

	void contentChanged() override
	{
		AudioFileNodeBase::contentChanged();

		rebuildImpulse();
	}

	void reset()
	{
		SpinLock::ScopedLockType sl(impulseLock);

		for (auto c : convolvers)
			c->cleanPipeline();
	}

	bool handleModulation(double& )
	{
		return false;
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		SpinLock::ScopedLockType sl(impulseLock);

		auto numToProcess = jmin(data.getNumChannels(), convolvers.size());

		for (int i = 0; i < numToProcess; i++)
		{
			auto ptr = data[i].begin();
			convolvers[i]->process(ptr, ptr, data.getNumSamples());
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType&)
	{
		jassertfalse;
	}

	SpinLock impulseLock;

	OwnedArray<hise::MultithreadedConvolver> convolvers;
	
	int largestBlockSize = 0; 
	double lastSampleRate = 44100.0;
};

}
}

#endif  // CONVOLUTION_H_INCLUDED
