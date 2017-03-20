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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef CONVOLUTION_H_INCLUDED
#define CONVOLUTION_H_INCLUDED

#define CONVOLUTION_RAMPING_TIME_MS 30

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4127 4706 4100)
#endif

namespace wdl
{
	// This include does not need to appear in the base header, so it's included in-place
	#include "wdl/convoengine.h"
};

#if JUCE_MSVC
 #pragma warning (pop)
#endif


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

	void processBlock(float** data, int numChannels, int numSamples)
	{
		if (numChannels == 1)
		{
			float *l = data[0];

			if (fastMode)
			{
				const float a = 0.99f;
				const float invA = 1.0f - a;

				while (--numSamples >= 0)
				{
					const float smoothedGain = lastValue * a + gain * invA;
					lastValue = smoothedGain;

					*l++ *= smoothedGain;
				}
			}
			else
			{
				while (--numSamples >= 0)
				{
					const float smoothedGain = smoother.smooth(gain);

					*l++ *= smoothedGain;
				}
			}


		}

		else if (numChannels == 2)
		{
			if (fastMode)
			{
				const float a = 0.99f;
				const float invA = 1.0f - a;

				float *l = data[0];
				float *r = data[1];

				while (--numSamples >= 0)
				{
					const float smoothedGain = lastValue * a + gain * invA;
					lastValue = smoothedGain;

					*l++ *= smoothedGain;
					*r++ *= smoothedGain;
				}
			}
			else
			{
				float *l = data[0];
				float *r = data[1];

				while (--numSamples >= 0)
				{
					const float smoothedGain = smoother.smooth(gain);

					*l++ *= smoothedGain;
					*r++ *= smoothedGain;
				}
			}

		}
	}

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



/** @brief A convolution reverb using zero-latency convolution
*	@ingroup effectTypes
*
*	This is a wrapper for the convolution engine found in WDL (the sole MIT licenced convolution engine available)
*	It is not designed to replace real convolution reverbs (as the CPU usage for impulses > 0.6 seconds is unreasonable),
*	but your early reflection impulses or other filter impulses will be thankful for this effect.
*/
class ConvolutionEffect: public MasterEffectProcessor,
						 public AudioSampleProcessor
{
public:

	SET_PROCESSOR_NAME("Convolution", "Convolution Reverb")

	// ============================================================================================= Constructor / Destructor / enums

	enum Parameters
	{
		DryGain = 0, ///< the gain of the unprocessed input
		WetGain, ///< the gain of the convoluted input
		Latency, ///< you can change the latency (unused)
		ImpulseLength, ///< the Impulse length (deprecated, use the SampleArea of the AudioSampleBufferComponent to change the impulse response)
		ProcessInput, ///< if this attribute is set, the engine will fade out in a short time and reset itself.
		numEffectParameters
	};

	ConvolutionEffect(MainController *mc, const String &id);;

	

	// ============================================================================================= Convolution methods

	void newFileLoaded() override {	setImpulse(); }
	void rangeUpdated() override { setImpulse(); }
	void setImpulse();

	// ============================================================================================= MasterEffect methods

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;;
	bool hasTail() const override {return false; };

	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

private:

	GainSmoother smoothedGainerWet;
	GainSmoother smoothedGainerDry;

	AudioSampleBuffer wetBuffer;

	const CriticalSection& getImpulseLock() const { return lock; };

	void enableProcessing(bool shouldBeProcessed);

	bool rampFlag;
	bool rampUp;
	bool processFlag;
	int rampIndex;

	CriticalSection lock;

	bool isUsingPoolData;

	bool isReloading;

	float dryGain;
	float wetGain;
	int latency;

	wdl::WDL_ImpulseBuffer impulseBuffer;
	wdl::WDL_ConvolutionEngine_Div convolutionEngine;

	double lastSampleRate = 0.0;
};




#endif  // CONVOLUTION_H_INCLUDED
