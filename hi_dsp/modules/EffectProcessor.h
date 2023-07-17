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

#ifndef HI_EFFECT_PROCESSOR_H_INCLUDED
#define HI_EFFECT_PROCESSOR_H_INCLUDED

#include <array>

namespace hise { using namespace juce;

#define EFFECT_PROCESSOR_COLOUR 0xff3a6666

/** Base class for all Processors that applies a audio effect on the audio data. 
*	
*
*	You won't ever subclass from this class directly, but use either MasterEffectProcessor or VoiceEffectProcessor,
	depending on the type of your effect.
*
*/
class EffectProcessor: public Processor
{
public:

	struct SuspensionState
	{
		void reset()
		{
			numSilentBuffers = 0;
			currentlySuspended = false;
		}

		int numSilentBuffers = 0;
		bool currentlySuspended = false;
		bool playing = false;
	};

	static bool isSilent(AudioSampleBuffer& b, int startSample, int numSamples);

	

	EffectProcessor(MainController *mc, const String &uid, int numVoices): 
		Processor(mc, uid, numVoices),	
		isTailing(false)
	{
		
	};

	virtual ~EffectProcessor()
	{

		modChains.clear();
	};

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples)
	{
		for (auto& mb : modChains)
		{
			if (!mb.getChain()->shouldBeProcessedAtAll())
			{
				mb.clear();
				continue;
			}

			mb.calculateMonophonicModulationValues(startSample, numSamples);
			mb.calculateModulationValuesForCurrentVoice(0, startSample, numSamples);

			if (mb.isAudioRateModulation())
				mb.expandVoiceValuesToAudioRate(0, startSample, numSamples);
		}
	}

	/** You have to override this method, since almost every effect needs the samplerate anyway. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	Colour getColour() const override 
	{
		return Colour(EFFECT_PROCESSOR_COLOUR);
	}

	virtual void voicesKilled()
	{
		jassert(!hasTail());
	}

	/** Overwrite this method if the effect has a tail (produces sound if no input is active */
	virtual bool hasTail() const = 0;

	/** Overwrite this method and return true if the effect should be suspended when there is no audio input. */
	virtual bool isSuspendedOnSilence() const { return false; };

	/** Overwrite this method and return true if the effect is currently suspended. */
	virtual bool isCurrentlySuspended() const { return false; };

	/** Checks if the effect is tailing off. This simply returns the calculated value, but the EffectChain overwrites this. */
	bool isTailingOff() const {	return isTailing; };

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) = 0;

	virtual void handleHiseEvent(const HiseEvent &m)
	{
		for (auto& mc : modChains)
			mc.handleHiseEvent(m);
	};

protected:

	bool isTailing = false;

	int numSilentCallbacksToWait = 86;
	

	bool isInSendContainer() const noexcept { return isInSend; };

	void finaliseModChains();

#if 0
	/** Takes a copy of the buffer before it is processed to check if a tail was added after processing. */
	void saveBufferForTailCheck(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		if (hasTail())
		{
			tailCheck.copyFrom(0, startSample, b.getReadPointer(0, startSample), numSamples);
			tailCheck.copyFrom(1, startSample, b.getReadPointer(1, startSample), numSamples);
		}
	}

	/** If your effect produces a tail, you have to call this method after your processing. */

	void checkTailing(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		// Call this only on effects that produce a tail!
		jassert(hasTail());

		const float maxInL = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(0, startSample), numSamples);
		const float maxInR = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(1, startSample), numSamples);

		const float maxL = FloatVectorOperations::findMaximum(b.getReadPointer(0, startSample), numSamples);
		const float maxR = FloatVectorOperations::findMaximum(b.getReadPointer(1, startSample), numSamples);

		const float in = maxInL + maxInR;
		const float out = maxL + maxR;

		isTailing = (in == 0.0f && out >= 0.01f);
	}
#endif

	ModulatorChain::Collection modChains;

private:

	bool isInSend = false;

	bool finalised = false;
};

/** A MasterEffectProcessor renders a effect on a stereo signal.
*	@ingroup dsp_base_classes
*
*	Derive all effects that are processed on the whole buffer from this class. For polyphonic effects that need to process single voices, use VoiceEffectProcessor as base class.
*/
class MasterEffectProcessor: public EffectProcessor,
							 public RoutableProcessor
{
public:
	
	enum SoftBypassState
	{
		Inactive,
		Pending,
		Bypassed,
		numSoftBypassStates
	};

	MasterEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid, 1)
	{
		softBypassRamper.setValueWithoutSmoothing(1.0f);

		getMatrix().init();
		getMatrix().setOnlyEnablingAllowed(true);
		getMatrix().setNumAllowedConnections(2);
	};

	virtual ~MasterEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		path.loadPathFromData (HiBinaryData::SpecialSymbols::masterEffect, sizeof (HiBinaryData::SpecialSymbols::masterEffect));

		return path;
	}

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = Processor::exportAsValueTree();
		v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		Processor::restoreFromValueTree(v);

		ValueTree r = v.getChildWithName("RoutingMatrix");

		if (r.isValid())
		{
			getMatrix().restoreFromValueTree(r);
		}
	}

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler/* =dontSendNotification */) noexcept override
	{
		Processor::setBypassed(shouldBeBypassed, notifyChangeHandler);
		setSoftBypass(shouldBeBypassed, getMainController()->shouldUseSoftBypassRamps());
	}

	virtual bool isFadeOutPending() const noexcept;

	virtual void updateSoftBypass()
	{
		const bool shouldBeBypassed = isBypassed();

		setSoftBypass(isBypassed(), !shouldBeBypassed);
	}

	bool isSoftBypassed() const noexcept { return softBypassState == Bypassed; }

	virtual void setSoftBypass(bool shouldBeSoftBypassed, bool useRamp=true);

	virtual void numDestinationChannelsChanged() override 
	{

	};
	virtual void numSourceChannelsChanged() override
	{
		
	};

	virtual void startMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.startVoice(0);
	}
	
	virtual void stopMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.stopVoice(0);
	}

	void setKillBuffer(AudioSampleBuffer& b)
	{
		killBuffer = &b;
	}

	virtual void resetMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.resetVoice(0);
	}

	bool isCurrentlySuspended() const final override
	{
		return masterState.currentlySuspended;
	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		if (sampleRate > 0.0 && samplesPerBlock > 0)
			softBypassRamper.reset(sampleRate / (double)samplesPerBlock, 0.1);

		masterState.reset();

		
	}

	void setEventBuffer(HiseEventBuffer* eventBufferFromSynth)
	{
		eventBuffer = eventBufferFromSynth;
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*
	*	Also this effect grabs the whole buffer (it can be divided by incoming midi messages for VoiceEffectProcessors).
	*/
	virtual void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) = 0;

	/** This only renders the modulatorChains. */
	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int startSample, int numSamples) final override
	{
		jassert(isOnAir());

		renderAllChains(startSample, numSamples);
	}

	/** This renders the whole buffer. 
	*
	*	You can still modulate the wet signal amount or pan effects using multiplications
	**/
	virtual void renderWholeBuffer(AudioSampleBuffer &buffer)
	{
		if (softBypassState == Bypassed)
			return;

		auto leftChannel = getLeftSourceChannel();
		auto rightChannel = getRightSourceChannel();
		auto numAllowed = getMatrix().getNumAllowedConnections();
		auto numMax = getMatrix().getNumDestinationChannels();

		auto ok = (leftChannel != -1 && rightChannel != -1) ||
                  (numAllowed != 2 && (leftChannel != -1 || rightChannel != -1));

		ok &= leftChannel < numMax &&
			  rightChannel < numMax;

		if (ok)
		{
			auto isStereo = rightChannel != -1;

			float *samples[2] = { buffer.getWritePointer(leftChannel), isStereo ? buffer.getWritePointer(rightChannel) : nullptr };

			const int samplesToUse = buffer.getNumSamples();

			AudioSampleBuffer stereoBuffer(samples, isStereo ? 2 : 1, samplesToUse);

			if (softBypassState == Pending)
			{
				masterState.reset();

				jassert(stereoBuffer.getNumChannels() <= killBuffer->getNumChannels());
				jassert(stereoBuffer.getNumSamples() <= killBuffer->getNumSamples());

				int numSamples = stereoBuffer.getNumSamples();
				int numChannels = isStereo ? 2 : 1;

				float start = jmin<float>(1.0f, softBypassRamper.getCurrentValue());
				float end = jmax<float>(0.0f, softBypassRamper.getNextValue());

				float start_inv = 1.0f - start;
				float end_inv = 1.0f - end;

				// We don't want to fade to the input signal in a AUX send context
				// so in this case we'll skip this loop
				int numChannelsToFadeIn = numChannels * (int)!isInSendContainer();

				for (int i = 0; i < numChannelsToFadeIn; i++)
					killBuffer->copyFromWithRamp(i, 0, stereoBuffer.getReadPointer(i), numSamples, start_inv, end_inv);

				applyEffect(stereoBuffer, 0, samplesToUse);
				isTailing = !isSilent(stereoBuffer, 0, samplesToUse);

				stereoBuffer.applyGainRamp(0, numSamples, start, end);

				for (int i = 0; i < numChannelsToFadeIn; i++)
					stereoBuffer.addFrom(i, 0, killBuffer->getReadPointer(i), numSamples);

				if (!softBypassRamper.isSmoothing())
				{
					if (end < 0.5f)
					{
						voicesKilled();
						softBypassState = Bypassed;
					}
					else
					{
						softBypassState = Inactive;
					}
				}

				currentValues.outL = softBypassState == Bypassed ? 0.0f : stereoBuffer.getMagnitude(0, 0, samplesToUse);

				if(isStereo)
					currentValues.outR = softBypassState == Bypassed ? 0.0f : stereoBuffer.getMagnitude(1, 0, samplesToUse);
			}
			else
			{
				auto suspendAtSilence = isSuspendedOnSilence();

				if (suspendAtSilence && masterState.numSilentBuffers > numSilentCallbacksToWait)
				{
					if (isSilent(stereoBuffer, 0, samplesToUse))
					{
						if (getMatrix().anyChannelActive())
						{
							float gainValues[NUM_MAX_CHANNELS];

							memset(gainValues, 0, getMatrix().getNumSourceChannels() * sizeof(float));

							getMatrix().setGainValues(gainValues, true);
							getMatrix().setGainValues(gainValues, false);
						}

#if ENABLE_ALL_PEAK_METERS
						currentValues.outL = 0.0f;
						currentValues.outR = 0.0f;
#endif

						masterState.currentlySuspended = true;
						return;
					}
						
				}

				masterState.currentlySuspended = false;
				applyEffect(stereoBuffer, 0, samplesToUse);

				if (suspendAtSilence)
				{
					isTailing = !isSilent(stereoBuffer, 0, samplesToUse);

					if (!isTailing)
						masterState.numSilentBuffers++;
					else
						masterState.numSilentBuffers = 0;
				}
				else
				{
					isTailing = hasTail() && !isSilent(stereoBuffer, 0, samplesToUse);
					masterState.numSilentBuffers = 0;
				}

				

#if ENABLE_ALL_PEAK_METERS
				currentValues.outL = stereoBuffer.getMagnitude(0, 0, samplesToUse);

				if(isStereo)
					currentValues.outR = stereoBuffer.getMagnitude(1, 0, samplesToUse);
#endif
			}

			if (getMatrix().anyChannelActive())
			{
				float gainValues[NUM_MAX_CHANNELS];

				jassert(getMatrix().getNumSourceChannels() == buffer.getNumChannels());

				for (int i = 0; i < buffer.getNumChannels(); i++)
				{
					if (getMatrix().isEditorShown(i))
						gainValues[i] = buffer.getMagnitude(i, 0, samplesToUse);
					else
						gainValues[i] = 0.0f;
				}

				getMatrix().setGainValues(gainValues, true);
				getMatrix().setGainValues(gainValues, false);
			}
		}

		
	};
    
	AudioSampleBuffer* killBuffer = nullptr;

protected:

	HiseEventBuffer* eventBuffer = nullptr;
	SuspensionState masterState;

private:

	

	SoftBypassState softBypassState = Inactive;
	LinearSmoothedValue<float> softBypassRamper;

	
};

/** @internal A EffectProcessor which allows monophonic modulation of its parameters.
*
*	If your effect wants to do more than modulate the wet amount or anything else that can be achieved
*	with a trivial multiplication, you can subclass it from this class and it takes care of the following
*	things:
*
*	- calculation of all internal chains (polyphonic modulators only have one voice, so there can be some
*	  value jumps.
*	- analysis of the dynamic range within the modulation buffers to determine the update rate.
*	- sequentielly calling the virtual function applyEffect with the divided sections to allow the subclass
*	  to change the parameters before rendering.
*
*	Beware that this leads to multiple calls if the modulation buffers contain large dynamics / the incoming
*	midi messages are frequent, so if you need heavy processing, consider using MasterEffectProcessor class, which
*	also allows modulation of the basic parameters.
*/
class MonophonicEffectProcessor: public EffectProcessor
{
public:

	MonophonicEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid, 1) {};
	
	virtual ~MonophonicEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		path.loadPathFromData (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, sizeof (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));

		return path;
	}

	virtual void startMonophonicVoice(const HiseEvent& e)
	{
		ignoreUnused(e);

		for (auto& mb : modChains)
			mb.startVoice(0);
	}
	
	virtual void stopMonophonicVoice()
	{
		for (auto& mb : modChains)
			mb.stopVoice(0);
	}

	virtual void resetMonophonicVoice() 
	{
		for (auto& mb : modChains)
			mb.resetVoice(0);
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*
	*	Also this effect grabs the whole buffer (it can be divided by incoming midi messages for VoiceEffectProcessors).
	*/
	virtual void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) = 0;

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		jassert(isOnAir());

		renderAllChains(startSample, numSamples);

		constexpr int stepSize = 64;

		while(numSamples >= stepSize)
		{
			applyEffect(buffer, startSample, stepSize);

			startSample += stepSize;
			numSamples  -= stepSize;
		}

		if(numSamples != 0)
		{
			applyEffect(buffer, startSample, numSamples);
		}

#if ENABLE_ALL_PEAK_METERS
		currentValues.outL = buffer.getMagnitude(0, startSample, numSamples);
		currentValues.outR = buffer.getMagnitude(1, startSample, numSamples);
#endif
	}
};



/** A VoiceEffectProcessor will process each voice before it is summed up and allows polyphonic effects.
*	@ingroup dsp_base_classes
*/
class VoiceEffectProcessor: public EffectProcessor
{
public:

	VoiceEffectProcessor(MainController *mc, const String &uid, int numVoices_): 
		EffectProcessor(mc, uid, numVoices_)
	{
		for (int i = 0; i < numVoices_; i++)
			polyState.add({});
	};

	virtual ~VoiceEffectProcessor() {};

	Path getSpecialSymbol() const override;

	/** This is called before every voice is processed. Use this to calculate all non polyphonic modulators in your subclasses chains! */
	virtual void preRenderCallback(int startSample, int numSamples)
	{
		for (auto& mb : modChains)
			mb.calculateMonophonicModulationValues(startSample, numSamples);

		if (forceMono)
		{
			for (auto& mb : modChains)
			{
				mb.calculateModulationValuesForCurrentVoice(0, startSample, numSamples);
			}
		}
	}

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*/
	virtual void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSample) = 0;

	void preVoiceRendering(int voiceIndex, int startSample, int numSamples)
	{
		for (auto& mb : modChains)
		{
			mb.calculateModulationValuesForCurrentVoice(voiceIndex, startSample, numSamples);
			if (mb.isAudioRateModulation())
				mb.expandVoiceValuesToAudioRate(voiceIndex, startSample, numSamples);
		}
	}

	/** renders a voice and applies the effect on the voice. */
	virtual void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
	{
		jassert(isOnAir());

		preVoiceRendering(voiceIndex, startSample, numSamples);

		constexpr int stepSize = 64;

		while(numSamples >= stepSize)
		{
			applyEffect(voiceIndex, b, startSample, stepSize);

			startSample += stepSize;
			numSamples  -= stepSize;
		}

		if(numSamples != 0)
		{
			applyEffect(voiceIndex, b, startSample, numSamples);
		}

	}

	bool checkPreSuspension(int voiceIndex, ProcessDataDyn& d)
	{
		if (isSuspendedOnSilence())
		{
			jassert(isPositiveAndBelow(voiceIndex, polyState.size()));

			auto& s = polyState.getReference(voiceIndex);

			if (s.numSilentBuffers > numSilentCallbacksToWait)
			{
				if (d.isSilent())
				{
					s.currentlySuspended = true;
					return true;
				}
					
			}
			else
			{
				s.currentlySuspended = false;
			}
		}

		return false;
	}

	

	void checkPostSuspension(int voiceIndex, ProcessDataDyn& data)
	{
		if (hasTail() || isSuspendedOnSilence())
		{
			isTailing = !data.isSilent();

			if (isTailing)
				polyState.getReference(voiceIndex).numSilentBuffers = 0;
			else
				polyState.getReference(voiceIndex).numSilentBuffers++;
		}
	}

	bool isCurrentlySuspended() const final override
	{
		if (!isSuspendedOnSilence())
			return false;

		

		for (const auto& s : polyState)
		{
			if (s.playing && !s.currentlySuspended)
				return false;
		}

		return true;
	}

	virtual void startVoice(int voiceIndex, const HiseEvent& e)
	{
		ignoreUnused(e);

		for (auto& mb : modChains)
			mb.startVoice(voiceIndex);

		if (isSuspendedOnSilence())
		{
			auto& s = polyState.getReference(voiceIndex);
			s.playing = true;
			s.reset();
		}
	}

	virtual void stopVoice(int voiceIndex)
	{
		for (auto& mb : modChains)
			mb.stopVoice(voiceIndex);
	}

	virtual void reset(int voiceIndex)
	{
		for (auto& mb : modChains)
			mb.resetVoice(voiceIndex);

		if (isSuspendedOnSilence())
		{
			polyState.getReference(voiceIndex).playing = false;
		}
	}

	void handleHiseEvent(const HiseEvent &m) override
	{
		for (auto& mb : modChains)
			mb.handleHiseEvent(m);
	};


	void setForceMonoMode(bool shouldUseMonoMode)
	{
		forceMono = shouldUseMonoMode;
	}

protected:

	bool forceMono = false;

	Array<SuspensionState> polyState;
};

} // namespace hise

#endif
