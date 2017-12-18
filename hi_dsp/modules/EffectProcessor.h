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

namespace hise { using namespace juce;

#define EFFECT_PROCESSOR_COLOUR 0xff3a6666

/** Base class for all Processors that applies a audio effect on the audio data. 
*	@ingroup effect
*
*/
class EffectProcessor: public Processor
{
public:

	EffectProcessor(MainController *mc, const String &uid): 
		Processor(mc, uid),	
		isTailing(false),
		useStepSize(true),
		tailCheck(2, 0),
		emptyBuffer(1, 0) 
	{};

	virtual ~EffectProcessor() {};

	/** You have to override this method, since almost every effect needs the samplerate anyway. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		Processor::prepareToPlay(sampleRate, samplesPerBlock);

		if (samplesPerBlock > 0)
		{
			ProcessorHelpers::increaseBufferIfNeeded(tailCheck, samplesPerBlock);	
		}

		for(int i = 0; i < getNumChildProcessors(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			if(mc != nullptr)
            {
                mc->prepareToPlay(sampleRate, samplesPerBlock);

				ProcessorHelpers::increaseBufferIfNeeded(getBufferForChain(i), samplesPerBlock);
            }
            else
            {
                jassertfalse;
            }
		}

	};

	Colour getColour() const override 
	{
		return Colour(EFFECT_PROCESSOR_COLOUR);
	}

	/** Overwrite this method if the effect has a tail (produces sound if no input is active */
	virtual bool hasTail() const = 0;
	
	/** Checks if the effect is tailing off. This simply returns the calculated value, but the EffectChain overwrites this. */
	virtual bool isTailingOff() const {	return isTailing; };

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) = 0;

	virtual void handleHiseEvent(const HiseEvent &m)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			static_cast<ModulatorChain*>(getChildProcessor(i))->handleHiseEvent(m);
		}
	};

protected:

	/** Overwrite this method and return a reference to an internal buffer.
	*
	*	This is used to store the precalculated modulation values from preRenderCallback()
	*/
	virtual AudioSampleBuffer &getBufferForChain(int /*chainIndex*/) { return emptyBuffer; };

	/** Takes a copy of the buffer before it is processed to check if a tail was added after processing. */
	void saveBufferForTailCheck(AudioSampleBuffer &b, int startSample, int numSamples)
	{
		tailCheck.copyFrom(0, startSample, b.getReadPointer(0, startSample), numSamples);
		tailCheck.copyFrom(1, startSample, b.getReadPointer(1, startSample), numSamples);
	}

	/** If your effect produces a tail, you have to call this method after your processing. */
	void checkTailing(AudioSampleBuffer &b, int startSample, int numSamples);

	virtual const float *getModulationValuesForStepsizeCalculation(int /*chainIndex*/, int /*voiceIndex*/) { jassertfalse; return nullptr; };

	/** Searches the modulation buffer for the minima and maxima and returns a power of two number according to the dynamic.
	*
	*	This can be used to check if there is some action in the modulation that needs splitting of processing to avoid parameter jumps without having to
	*	calculate each sample.
	*/
	int calculateStepSize(int /*voiceIndex*/, int numSamples)
	{
		if(!useStepSize) return numSamples;

		return 64;

#if 0
		int stepSize = numSamples;

		for(int i = 0; i < getNumChildProcessors(); ++i)
		{
			int thisStepSize = numSamples;

			const float *modulationData = getModulationValuesForStepsizeCalculation(i, voiceIndex);

            float delta = FloatVectorOperations::findMinAndMax(modulationData, numSamples).getLength();
            

            const int factor = numSamples / 256;
            
            if(factor != 0) delta /= (float)factor;
		
			if      (delta <= 0.002f) thisStepSize = numSamples;
			else if (delta <= 0.005f) thisStepSize = 64;
			else if (delta <= 0.01f)  thisStepSize = 32;
			else                      thisStepSize = 16;
			
			stepSize = jmin(stepSize, thisStepSize);

		}

		return stepSize;
#endif
	}

	void useStepSizeCalculation(bool shouldBeUsed) {useStepSize = shouldBeUsed; };

	float getCurrentModulationValue(int chainIndex, int voiceIndex, int samplePosition)
	{
		return static_cast<ModulatorChain*>(getChildProcessor(chainIndex))->getVoiceValues(voiceIndex)[samplePosition];
	}

	float* getCurrentModulationValues(int chainIndex, int voiceIndex, int startSample)
	{
		return static_cast<ModulatorChain*>(getChildProcessor(chainIndex))->getVoiceValues(voiceIndex) + startSample;
	}

	void calculateChain(int chainIndex, int voiceIndex, int startSample, int numSamples)
	{
		ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(chainIndex));

		jassert(mc != nullptr);

		mc->renderVoice(voiceIndex, startSample, numSamples);

		float *modValues = mc->getVoiceValues(voiceIndex);

		const float *timeVariantModValues = getBufferForChain(chainIndex).getReadPointer(0, startSample);

		// Offset of startSamples, since getVoiceValues() returns the whole buffer
		FloatVectorOperations::multiply(modValues + startSample, timeVariantModValues, numSamples);

	};


private:
	AudioSampleBuffer tailCheck;

	AudioSampleBuffer emptyBuffer;

	bool isTailing;

	bool useStepSize;
};

/** A MasterEffectProcessor renders a effect on a block of audio samples. 
*	@ingroup effect
*
*	Derive all effects that are processed on the whole buffer from this class. For polyphonic effects, use VoiceEffectProcessor as baseclass.
*/
class MasterEffectProcessor: public EffectProcessor,
							 public RoutableProcessor
{
public:
	MasterEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid)
	{
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

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->renderVoice(0, startSample, numSamples);

			mc->renderNextBlock(getBufferForChain(i), startSample, numSamples);

			FloatVectorOperations::multiply(getBufferForChain(i).getWritePointer(0, startSample), mc->getVoiceValues(0), numSamples);
		}
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


	virtual void numDestinationChannelsChanged() override 
	{

	};
	virtual void numSourceChannelsChanged() override
	{
		
	};

	virtual void startMonophonicVoice()
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->startVoice(0);
		}
	}
	
	virtual void stopMonophonicVoice()
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->stopVoice(0);
		}
	}

	virtual void resetMonophonicVoice()
	{
		
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
	virtual void renderNextBlock(AudioSampleBuffer &/*buffer*/, int startSample, int numSamples) final override
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
		if (getLeftSourceChannel() != -1 && getRightSourceChannel() != -1 &&
            getLeftSourceChannel() < getMatrix().getNumDestinationChannels() &&
            getRightSourceChannel() < getMatrix().getNumDestinationChannels())
		{
			float *leftChannel = buffer.getWritePointer(getLeftSourceChannel());
			float *rightChannel = buffer.getWritePointer(getRightSourceChannel());

			float *samples[2] = { leftChannel, rightChannel };

			const int samplesToUse = getBlockSize();

			AudioSampleBuffer stereoBuffer(samples, 2, buffer.getNumSamples());

			applyEffect(stereoBuffer, 0, samplesToUse);

#if ENABLE_ALL_PEAK_METERS
			currentValues.outL = stereoBuffer.getMagnitude(0, 0, samplesToUse);
			currentValues.outR = stereoBuffer.getMagnitude(1, 0, samplesToUse);
#endif

			if (getMatrix().isEditorShown())
			{
				float gainValues[NUM_MAX_CHANNELS];

				jassert(getMatrix().getNumSourceChannels() == buffer.getNumChannels());

				for (int i = 0; i < buffer.getNumChannels(); i++)
				{
					gainValues[i] = buffer.getMagnitude(i, 0, samplesToUse);
				}

				getMatrix().setGainValues(gainValues, true);
				getMatrix().setGainValues(gainValues, false);

			}
		}
	};
};

/** A EffectProcessor which allows monophonic modulation of its parameters.
*	@ingroup effects
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

	MonophonicEffectProcessor(MainController *mc, const String &uid): EffectProcessor(mc, uid) {};
	
	virtual ~MonophonicEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		path.loadPathFromData (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath, sizeof (HiBinaryData::ProcessorEditorHeaderIcons::monophonicPath));

		return path;
	}

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			mc->renderVoice(0, startSample, numSamples);
			mc->renderNextBlock(getBufferForChain(i), startSample, numSamples);

			FloatVectorOperations::multiply(getBufferForChain(i).getWritePointer(0, startSample), mc->getVoiceValues(0), numSamples);
		}
	}

	virtual void startMonophonicVoice(int noteNumber=-1)
	{
		ignoreUnused(noteNumber);

		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->startVoice(0);
		}
	}
	
	const float *getModulationValuesForStepsizeCalculation(int chainIndex, int /*voiceIndex*/) override
	{
		return getBufferForChain(chainIndex).getReadPointer(0);
	}

	virtual void stopMonophonicVoice()
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->stopVoice(0);
		}
	}

	virtual void resetMonophonicVoice() {}

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

		const int stepSize = calculateStepSize(0, numSamples);

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



/** A VoiceEffectProcessor has multiple states that allows polyphonic rendering of the audio effect. 
*	@ingroup effect
*/
class VoiceEffectProcessor: public EffectProcessor
{
public:

	VoiceEffectProcessor(MainController *mc, const String &uid, int numVoices_): 
		EffectProcessor(mc, uid),
		numVoices(numVoices_)
	{};

	

	virtual ~VoiceEffectProcessor() {};

	Path getSpecialSymbol() const override
	{
		Path path;

		static const unsigned char pathData[] = { 110,109,0,0,2,67,92,174,213,67,98,0,0,2,67,211,15,215,67,217,133,255,66,92,46,216,67,0,0,250,66,92,46,216,67,98,39,122,244,66,92,46,216,67,0,0,240,66,211,15,215,67,0,0,240,66,92,174,213,67,98,0,0,240,66,230,76,212,67,39,122,244,66,92,46,211,67,0,0,250,
		66,92,46,211,67,98,217,133,255,66,92,46,211,67,0,0,2,67,230,76,212,67,0,0,2,67,92,174,213,67,99,109,0,0,230,66,92,174,213,67,98,0,0,230,66,211,15,215,67,217,133,225,66,92,46,216,67,0,0,220,66,92,46,216,67,98,39,122,214,66,92,46,216,67,0,0,210,66,211,
		15,215,67,0,0,210,66,92,174,213,67,98,0,0,210,66,230,76,212,67,39,122,214,66,92,46,211,67,0,0,220,66,92,46,211,67,98,217,133,225,66,92,46,211,67,0,0,230,66,230,76,212,67,0,0,230,66,92,174,213,67,99,109,0,0,200,66,92,174,213,67,98,0,0,200,66,211,15,215,
		67,217,133,195,66,92,46,216,67,0,0,190,66,92,46,216,67,98,39,122,184,66,92,46,216,67,0,0,180,66,211,15,215,67,0,0,180,66,92,174,213,67,98,0,0,180,66,230,76,212,67,39,122,184,66,92,46,211,67,0,0,190,66,92,46,211,67,98,217,133,195,66,92,46,211,67,0,0,200,
		66,230,76,212,67,0,0,200,66,92,174,213,67,99,109,0,0,2,67,92,46,206,67,98,0,0,2,67,211,143,207,67,217,133,255,66,92,174,208,67,0,0,250,66,92,174,208,67,98,39,122,244,66,92,174,208,67,0,0,240,66,211,143,207,67,0,0,240,66,92,46,206,67,98,0,0,240,66,230,
		204,204,67,39,122,244,66,92,174,203,67,0,0,250,66,92,174,203,67,98,217,133,255,66,92,174,203,67,0,0,2,67,230,204,204,67,0,0,2,67,92,46,206,67,99,109,0,0,230,66,92,46,206,67,98,0,0,230,66,211,143,207,67,217,133,225,66,92,174,208,67,0,0,220,66,92,174,208,
		67,98,39,122,214,66,92,174,208,67,0,0,210,66,211,143,207,67,0,0,210,66,92,46,206,67,98,0,0,210,66,230,204,204,67,39,122,214,66,92,174,203,67,0,0,220,66,92,174,203,67,98,217,133,225,66,92,174,203,67,0,0,230,66,230,204,204,67,0,0,230,66,92,46,206,67,99,
		109,0,0,200,66,92,46,206,67,98,0,0,200,66,211,143,207,67,217,133,195,66,92,174,208,67,0,0,190,66,92,174,208,67,98,39,122,184,66,92,174,208,67,0,0,180,66,211,143,207,67,0,0,180,66,92,46,206,67,98,0,0,180,66,230,204,204,67,39,122,184,66,92,174,203,67,0,
		0,190,66,92,174,203,67,98,217,133,195,66,92,174,203,67,0,0,200,66,230,204,204,67,0,0,200,66,92,46,206,67,99,109,0,0,2,67,92,174,198,67,98,0,0,2,67,211,15,200,67,217,133,255,66,92,46,201,67,0,0,250,66,92,46,201,67,98,39,122,244,66,92,46,201,67,0,0,240,
		66,211,15,200,67,0,0,240,66,92,174,198,67,98,0,0,240,66,230,76,197,67,39,122,244,66,92,46,196,67,0,0,250,66,92,46,196,67,98,217,133,255,66,92,46,196,67,0,0,2,67,230,76,197,67,0,0,2,67,92,174,198,67,99,109,0,0,230,66,92,174,198,67,98,0,0,230,66,211,15,
		200,67,217,133,225,66,92,46,201,67,0,0,220,66,92,46,201,67,98,39,122,214,66,92,46,201,67,0,0,210,66,211,15,200,67,0,0,210,66,92,174,198,67,98,0,0,210,66,230,76,197,67,39,122,214,66,92,46,196,67,0,0,220,66,92,46,196,67,98,217,133,225,66,92,46,196,67,0,
		0,230,66,230,76,197,67,0,0,230,66,92,174,198,67,99,109,0,0,200,66,92,174,198,67,98,0,0,200,66,211,15,200,67,217,133,195,66,92,46,201,67,0,0,190,66,92,46,201,67,98,39,122,184,66,92,46,201,67,0,0,180,66,211,15,200,67,0,0,180,66,92,174,198,67,98,0,0,180,
		66,230,76,197,67,39,122,184,66,92,46,196,67,0,0,190,66,92,46,196,67,98,217,133,195,66,92,46,196,67,0,0,200,66,230,76,197,67,0,0,200,66,92,174,198,67,99,101,0,0 };

		path.loadPathFromData (pathData, sizeof (pathData));

		return path;

	}


	/** This is called before every voice is processed. Use this to calculate all non polyphonic modulators in your subclasses chains! */
	virtual void preRenderCallback(int startSample, int numSamples)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			mc->renderNextBlock(getBufferForChain(i), startSample, numSamples);
		}
	}

	const float *getModulationValuesForStepsizeCalculation(int chainIndex, int voiceIndex) override
	{
		ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(chainIndex));
		jassert(mc != nullptr);
		return mc->getVoiceValues(voiceIndex);
	}

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

		
	}

		/** A wrapper function around the actual processing.
	*
	*	You can assume that all internal chains are processed and the numSample amount is set according to the stepsize calculated with
	*	calculateStepSize().
	*	That means you can grab the current modulation value using getCurrentModulationValue(), set the parameters and process the block
	*	with smooth parameter changes.
	*/
	virtual void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSample) = 0;

	virtual void preVoiceRendering(int voiceIndex, int startSample, int numSamples) = 0;

	/** renders a voice and applies the effect on the voice. */
	virtual void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
	{
		jassert(isOnAir());

		if(hasTail()) saveBufferForTailCheck(b, startSample, numSamples);

		const int startIndex = startSample;
		const int samplesToCheck = numSamples;

		preVoiceRendering(voiceIndex, startSample, numSamples);

		const int stepSize = calculateStepSize(voiceIndex, numSamples);

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

		if(hasTail()) checkTailing(b, startIndex, samplesToCheck);

		return;
	}

	virtual void startVoice(int voiceIndex, int /*noteNumber*/)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->startVoice(voiceIndex);
		}
	}

	virtual void stopVoice(int voiceIndex)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->stopVoice(voiceIndex);
		}

	}

	virtual void reset(int voiceIndex)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->reset(voiceIndex);
		}
	}

	virtual void handleHiseEvent(const HiseEvent &m)
	{
		for (int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = static_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->handleHiseEvent(m);
		}
	};


protected:



private:

	

	int numVoices;
};

} // namespace hise

#endif