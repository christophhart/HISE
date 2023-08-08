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
		void reset();

		int numSilentBuffers = 0;
		bool currentlySuspended = false;
		bool playing = false;
	};

	static bool isSilent(AudioSampleBuffer& b, int startSample, int numSamples);

	

	EffectProcessor(MainController *mc, const String &uid, int numVoices);;

	virtual ~EffectProcessor();;

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples);

	/** You have to override this method, since almost every effect needs the samplerate anyway. */
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	Colour getColour() const override;

	virtual void voicesKilled();

	/** Overwrite this method if the effect has a tail (produces sound if no input is active */
	virtual bool hasTail() const = 0;

	/** Overwrite this method and return true if the effect should be suspended when there is no audio input. */
	virtual bool isSuspendedOnSilence() const;;

	/** Overwrite this method and return true if the effect is currently suspended. */
	virtual bool isCurrentlySuspended() const;;

	/** Checks if the effect is tailing off. This simply returns the calculated value, but the EffectChain overwrites this. */
	bool isTailingOff() const;;

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) = 0;

	virtual void handleHiseEvent(const HiseEvent &m);;

protected:

	bool isTailing = false;

	int numSilentCallbacksToWait = 86;
	

	bool isInSendContainer() const noexcept;;

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

	MasterEffectProcessor(MainController *mc, const String &uid);;

	virtual ~MasterEffectProcessor();;

	Path getSpecialSymbol() const override;

	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree &v) override;

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler/* =dontSendNotification */) noexcept override;

	virtual bool isFadeOutPending() const noexcept;

	virtual void updateSoftBypass();

	bool isSoftBypassed() const noexcept;

	virtual void setSoftBypass(bool shouldBeSoftBypassed, bool useRamp=true);

	virtual void numDestinationChannelsChanged() override;;
	virtual void numSourceChannelsChanged() override;;

	virtual void startMonophonicVoice();

	virtual void stopMonophonicVoice();

	void setKillBuffer(AudioSampleBuffer& b);

	virtual void resetMonophonicVoice();

	bool isCurrentlySuspended() const final override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void setEventBuffer(HiseEventBuffer* eventBufferFromSynth);

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
	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int startSample, int numSamples) final override;

	/** This renders the whole buffer. 
	*
	*	You can still modulate the wet signal amount or pan effects using multiplications
	**/
	virtual void renderWholeBuffer(AudioSampleBuffer &buffer);;
    
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

	MonophonicEffectProcessor(MainController *mc, const String &uid);;
	
	virtual ~MonophonicEffectProcessor();;

	Path getSpecialSymbol() const override;

	virtual void startMonophonicVoice(const HiseEvent& e);

	virtual void stopMonophonicVoice();

	virtual void resetMonophonicVoice();

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
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
};



/** A VoiceEffectProcessor will process each voice before it is summed up and allows polyphonic effects.
*	@ingroup dsp_base_classes
*/
class VoiceEffectProcessor: public EffectProcessor
{
public:

	VoiceEffectProcessor(MainController *mc, const String &uid, int numVoices_);;

	virtual ~VoiceEffectProcessor();;

	Path getSpecialSymbol() const override;

	/** This is called before every voice is processed. Use this to calculate all non polyphonic modulators in your subclasses chains! */
	virtual void preRenderCallback(int startSample, int numSamples);

	/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*/
	virtual void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSample) = 0;

	void preVoiceRendering(int voiceIndex, int startSample, int numSamples);

	/** renders a voice and applies the effect on the voice. */
	virtual void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples);

	bool checkPreSuspension(int voiceIndex, ProcessDataDyn& d);


	void checkPostSuspension(int voiceIndex, ProcessDataDyn& data);

	bool isCurrentlySuspended() const final override;

	virtual void startVoice(int voiceIndex, const HiseEvent& e);

	virtual void stopVoice(int voiceIndex);

	virtual void reset(int voiceIndex);

	void handleHiseEvent(const HiseEvent &m) override;;


	void setForceMonoMode(bool shouldUseMonoMode);

protected:

	bool forceMono = false;

	Array<SuspensionState> polyState;
};

} // namespace hise

#endif
