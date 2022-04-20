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

struct ConvolutionEffectBase: public AsyncUpdater,
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


/** @brief A convolution reverb using zero-latency convolution
*	@ingroup effectTypes
*
*/
class ConvolutionEffect: public MasterEffectProcessor,
						 public AudioSampleProcessor,
						 public MultiChannelAudioBuffer::Listener,
						 public ConvolutionEffectBase
{

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

	void bufferWasLoaded() override
	{
		setImpulse(sendNotificationSync);
	}

	void bufferWasModified() override
	{
		setImpulse(sendNotificationSync);
	}

	MultiChannelAudioBuffer& getImpulseBufferBase() override { return getBuffer(); }
	const MultiChannelAudioBuffer& getImpulseBufferBase() const override { return getBuffer(); }

	// ============================================================================================= MasterEffect methods

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override {return true; };

	void voicesKilled() override;

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

};


} // namespace hise

namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace filters
{

struct convolution : public data::base,
					 public ConvolutionEffectBase
{
	enum Parameters
	{
		Gate,
		Predelay,
		Damping,
		HiCut,
		Multithread,
		numParameters
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gate, convolution);
		DEF_PARAMETER(Damping, convolution);
		DEF_PARAMETER(HiCut, convolution);
		DEF_PARAMETER(Predelay, convolution);
	    DEF_PARAMETER(Multithread, convolution);
	};
	PARAMETER_MEMBER_FUNCTION;

	SET_HISE_NODE_ID("convolution");
	SN_GET_SELF_AS_OBJECT(convolution);
	SN_DESCRIPTION("A convolution reverb node");

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;

	static constexpr bool isPolyphonic() { return false; }
	static constexpr bool isNormalisedModulation() { return false; }

	bool handleModulation(double& d) { return false; }

	MultiChannelAudioBuffer& getImpulseBufferBase() override
	{
		auto b = dynamic_cast<MultiChannelAudioBuffer*>(externalData.obj);
		jassert(b != nullptr);
		return *b;
	}

	const MultiChannelAudioBuffer& getImpulseBufferBase() const override
	{
		auto b = dynamic_cast<const MultiChannelAudioBuffer*>(externalData.obj);
		jassert(b != nullptr);
		return *b;
	}

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		base::setExternalData(d, index);
		getImpulseBufferBase().setDisabledXYZProviders({ Identifier("SampleMap"), Identifier("SFZ") });
		setImpulse(sendNotificationSync);
	}

	void prepare(PrepareSpecs specs)
	{
		DspHelpers::setErrorIfFrameProcessing(specs);
		prepareBase(specs.sampleRate, specs.blockSize);
	}

#if 0
	void rebuildImpulse()
	{
		OwnedArray<MultithreadedConvolver> newConvolvers;

		for (int i = 0; i < lastSpecs.numChannels; i++)
			newConvolvers.add(new MultithreadedConvolver(audiofft::ImplementationType::BestAvailable));

		AudioSampleBuffer impulseBuffer;

		if(externalData.isNotEmpty())
		{
			DataReadLock l(this);

			auto ratio = MultithreadedConvolver::getResampleFactor(lastSpecs.sampleRate, externalData.sampleRate);
			MultithreadedConvolver::prepareImpulseResponse(externalData.toAudioSampleBuffer(), impulseBuffer, nullptr, {0, externalData.numSamples}, ratio);
		}

		if (impulseBuffer.getNumSamples() != 0)
		{
			const auto headSize = nextPowerOfTwo(lastSpecs.blockSize);
			const auto fullTailLength = nextPowerOfTwo(impulseBuffer.getNumSamples() - headSize);

			{
				for (int i = 0; i < newConvolvers.size(); i++)
				{
					int channelIndex = i % impulseBuffer.getNumChannels();
					auto c = newConvolvers[i];

					c->init(headSize, fullTailLength, impulseBuffer.getReadPointer(channelIndex), impulseBuffer.getNumSamples());
				}
			}
		}

		for (auto c : newConvolvers)
			c->setUseBackgroundThread(multithread);

		{
			SimpleReadWriteLock::ScopedWriteLock sl(impulseLock);
			convolvers.swapWith(newConvolvers);
		}
	}
#endif

	void reset()
	{
		resetBase();
	}

	

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& dynData = data.template as<ProcessDataDyn>();
		processBase(dynData);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		jassertfalse;
	}

	void setMultithread(double shouldBeMultithreaded)
	{
		useBackgroundThread = shouldBeMultithreaded > 0.5;

		{
			SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
			convolverL->setUseBackgroundThread(useBackgroundThread && !nonRealtime);
			convolverR->setUseBackgroundThread(useBackgroundThread && !nonRealtime);
		}
	}

	void setDamping(double targetSustainDb)
	{
		if (damping != targetSustainDb)
		{
			damping = Decibels::decibelsToGain(targetSustainDb);
			setImpulse(sendNotificationAsync);
		}
	}

	void setHiCut(double targetFreq)
	{
		if (cutoffFrequency != targetFreq)
		{
			cutoffFrequency = (double)targetFreq;
			calcCutoff();
		}
	}

	void setPredelay(double newDelay)
	{
		if (newDelay != predelayMs)
		{
			predelayMs = newDelay;
			calcPredelay();
		}
	}

	void setGate(double shouldBeEnabled)
	{
		processingEnabled = shouldBeEnabled >= 0.5;
		enableProcessing(processingEnabled);
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(convolution, Gate);
			p.setParameterValueNames({ "Off", "On" });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(convolution, Predelay);
			p.setRange({ 0.0, 1000.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(convolution, Damping);
			p.setRange({ -100.0, 0.0, 0.1 });
			p.setDefaultValue(0.0);
			p.setSkewForCentre(-12.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(convolution, HiCut);
			p.setRange({ 20.0, 20000.0, 1.0});
			p.setDefaultValue(20000.0);
			p.setSkewForCentre(1000.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(convolution, Multithread);
			p.setParameterValueNames({ "Off", "On" });
			data.add(std::move(p));
		}
	}
};

}
}

#endif  // CONVOLUTION_H_INCLUDED
