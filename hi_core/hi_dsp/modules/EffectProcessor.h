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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef HI_EFFECT_PROCESSOR_H_INCLUDED
#define HI_EFFECT_PROCESSOR_H_INCLUDED

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
		useStepSize(true) 
	{};

	virtual ~EffectProcessor() {};

	/** You have to override this method, since almost every effect needs the samplerate anyway. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		Processor::prepareToPlay(sampleRate, samplesPerBlock);
		tailCheck = AudioSampleBuffer(2, samplesPerBlock);

		for(int i = 0; i < getNumChildProcessors(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			if(mc != nullptr)
            {
                mc->prepareToPlay(sampleRate, samplesPerBlock);
                getBufferForChain(i) = AudioSampleBuffer(1, samplesPerBlock);
            }
            else
            {
                jassertfalse;
            }
		}

	};

	Colour getColour() const override 
	{
		return Colour(0xff3a6666);
	}

	/** Overwrite this method if the effect has a tail (produces sound if no input is active */
	virtual bool hasTail() const = 0;
	
	/** Checks if the effect is tailing off. This simply returns the calculated value, but the EffectChain overwrites this. */
	virtual bool isTailingOff() const {	return isTailing; };

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) = 0;

	virtual void handleMidiEvent(const MidiMessage &m)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->handleMidiEvent(m);
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
	int calculateStepSize(int voiceIndex, int numSamples)
	{
		if(!useStepSize) return numSamples;

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
	}

	void useStepSizeCalculation(bool shouldBeUsed) {useStepSize = shouldBeUsed; };

	float getCurrentModulationValue(int chainIndex, int voiceIndex, int samplePosition)
	{
		return dynamic_cast<ModulatorChain*>(getChildProcessor(chainIndex))->getVoiceValues(voiceIndex)[samplePosition];
	}

	void calculateChain(int chainIndex, int voiceIndex, int startSample, int numSamples)
	{
		ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(chainIndex));

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
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
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
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->startVoice(0);
		}
	}
	
	virtual void stopMonophonicVoice()
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
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

			AudioSampleBuffer stereoBuffer(samples, 2, buffer.getNumSamples());

			applyEffect(stereoBuffer, 0, buffer.getNumSamples());

			currentValues.outL = stereoBuffer.getMagnitude(0, 0, buffer.getNumSamples());
			currentValues.outR = stereoBuffer.getMagnitude(1, 0, buffer.getNumSamples());

			if (getMatrix().isEditorShown())
			{
				float gainValues[NUM_MAX_CHANNELS];

				jassert(getMatrix().getNumSourceChannels() == buffer.getNumChannels());

				for (int i = 0; i < buffer.getNumChannels(); i++)
				{
					gainValues[i] = buffer.getMagnitude(i, 0, buffer.getNumSamples());
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

		static const unsigned char pathData[] = { 110,109,0,0,190,66,92,174,218,67,108,0,0,190,66,92,174,218,67,108,103,246,189,66,82,14,219,67,108,162,217,189,66,10,110,219,67,108,195,169,189,66,72,205,219,67,108,233,102,189,66,206,43,220,67,108,64,17,189,66,96,137,220,67,108,252,168,188,66,194,229,
		220,67,108,98,46,188,66,185,64,221,67,108,192,161,187,66,11,154,221,67,108,111,3,187,66,126,241,221,67,108,214,83,186,66,219,70,222,67,108,100,147,185,66,235,153,222,67,108,148,194,184,66,121,234,222,67,108,237,225,183,66,81,56,223,67,108,254,241,182,
		66,66,131,223,67,108,96,243,181,66,27,203,223,67,108,182,230,180,66,175,15,224,67,108,173,204,179,66,209,80,224,67,108,248,165,178,66,89,142,224,67,108,85,115,177,66,30,200,224,67,108,135,53,176,66,252,253,224,67,108,90,237,174,66,208,47,225,67,108,160,
		155,173,66,122,93,225,67,108,49,65,172,66,222,134,225,67,108,234,222,170,66,224,171,225,67,108,175,117,169,66,105,204,225,67,108,102,6,168,66,100,232,225,67,108,251,145,166,66,191,255,225,67,108,91,25,165,66,108,18,226,67,108,120,157,163,66,94,32,226,
		67,108,69,31,162,66,141,41,226,67,108,183,159,160,66,242,45,226,67,108,194,31,159,66,138,45,226,67,108,93,160,157,66,87,40,226,67,108,124,34,156,66,91,30,226,67,108,21,167,154,66,157,15,226,67,108,25,47,153,66,38,252,225,67,108,122,187,151,66,2,228,225,
		67,108,38,77,150,66,66,199,225,67,108,6,229,148,66,247,165,225,67,108,1,132,147,66,54,128,225,67,108,249,42,146,66,25,86,225,67,108,203,218,144,66,185,39,225,67,108,78,148,143,66,53,245,224,67,108,83,88,142,66,173,190,224,67,108,163,39,141,66,67,132,
		224,67,108,3,3,140,66,30,70,224,67,108,45,235,138,66,100,4,224,67,108,212,224,137,66,64,191,223,67,108,162,228,136,66,223,118,223,67,108,58,247,135,66,110,43,223,67,108,50,25,135,66,29,221,222,67,108,26,75,134,66,32,140,222,67,108,116,141,133,66,169,
		56,222,67,108,187,224,132,66,238,226,221,67,108,92,69,132,66,38,139,221,67,108,188,187,131,66,138,49,221,67,108,50,68,131,66,81,214,220,67,108,11,223,130,66,184,121,220,67,108,135,140,130,66,249,27,220,67,108,220,76,130,66,79,189,219,67,108,49,32,130,
		66,249,93,219,67,108,164,6,130,66,49,254,218,67,108,0,0,130,66,92,174,218,67,108,0,0,130,66,92,174,218,67,108,0,0,130,66,92,174,218,67,108,153,9,130,66,102,78,218,67,108,94,38,130,66,174,238,217,67,108,61,86,130,66,112,143,217,67,108,23,153,130,66,234,
		48,217,67,108,193,238,130,66,88,211,216,67,108,5,87,131,66,245,118,216,67,108,159,209,131,66,254,27,216,67,108,66,94,132,66,172,194,215,67,108,146,252,132,66,57,107,215,67,108,44,172,133,66,220,21,215,67,108,159,108,134,66,204,194,214,67,108,111,61,135,
		66,62,114,214,67,108,22,30,136,66,102,36,214,67,108,6,14,137,66,117,217,213,67,108,165,12,138,66,156,145,213,67,108,79,25,139,66,8,77,213,67,108,89,51,140,66,230,11,213,67,108,14,90,141,66,94,206,212,67,108,178,140,142,66,153,148,212,67,108,129,202,143,
		66,187,94,212,67,108,174,18,145,66,231,44,212,67,108,105,100,146,66,61,255,211,67,108,217,190,147,66,217,213,211,67,108,32,33,149,66,215,176,211,67,108,92,138,150,66,78,144,211,67,108,165,249,151,66,83,116,211,67,108,17,110,153,66,248,92,211,67,108,177,
		230,154,66,75,74,211,67,108,149,98,156,66,89,60,211,67,108,200,224,157,66,43,51,211,67,108,87,96,159,66,198,46,211,67,108,77,224,160,66,46,47,211,67,108,178,95,162,66,97,52,211,67,108,147,221,163,66,93,62,211,67,108,251,88,165,66,28,77,211,67,108,247,
		208,166,66,147,96,211,67,108,150,68,168,66,183,120,211,67,108,235,178,169,66,120,149,211,67,108,11,27,171,66,195,182,211,67,108,16,124,172,66,132,220,211,67,108,23,213,173,66,161,6,212,67,108,69,37,175,66,1,53,212,67,108,194,107,176,66,134,103,212,67,
		108,190,167,177,66,14,158,212,67,108,109,216,178,66,120,216,212,67,108,13,253,179,66,158,22,213,67,108,227,20,181,66,88,88,213,67,108,59,31,182,66,124,157,213,67,108,108,27,183,66,221,229,213,67,108,212,8,184,66,79,49,214,67,108,218,230,184,66,159,127,
		214,67,108,242,180,185,66,157,208,214,67,108,150,114,186,66,20,36,215,67,108,79,31,187,66,207,121,215,67,108,172,186,187,66,151,209,215,67,108,76,68,188,66,52,43,216,67,108,212,187,188,66,108,134,216,67,108,251,32,189,66,5,227,216,67,108,125,115,189,
		66,197,64,217,67,108,40,179,189,66,110,159,217,67,108,209,223,189,66,197,254,217,67,108,92,249,189,66,140,94,218,67,108,0,0,190,66,92,174,218,67,108,0,0,190,66,92,174,218,67,99,101,0,0 };

		path.loadPathFromData (pathData, sizeof (pathData));

		return path;
	}

	/** Renders all chains (envelopes & voicestart are rendered monophonically. */
	void renderAllChains(int startSample, int numSamples)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->renderVoice(0, startSample, numSamples);

			mc->renderNextBlock(getBufferForChain(i), startSample, numSamples);

			FloatVectorOperations::multiply(getBufferForChain(i).getWritePointer(0, startSample), mc->getVoiceValues(0), numSamples);
		}
	}

	virtual void startMonophonicVoice()
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
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
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
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

	/** Renders the next block and applies the effect to the buffer. */
	virtual void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
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

		currentValues.outL = buffer.getMagnitude(0, startSample, numSamples);
		currentValues.outR = buffer.getMagnitude(1, startSample, numSamples);
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
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->renderNextBlock(getBufferForChain(i), startSample, numSamples);
		}
	}

	const float *getModulationValuesForStepsizeCalculation(int chainIndex, int voiceIndex) override
	{
		ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(chainIndex));
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
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->startVoice(voiceIndex);
		}
	}

	virtual void stopVoice(int voiceIndex)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->stopVoice(voiceIndex);
		}

	}

	virtual void reset(int voiceIndex)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->reset(voiceIndex);
		}
	}

	virtual void handleMidiEvent(const MidiMessage &m)
	{
		for(int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *mc = dynamic_cast<ModulatorChain*>(getChildProcessor(i));
			jassert(mc != nullptr);
			mc->handleMidiEvent(m);
		}
	};



protected:



private:

	

	int numVoices;
};

#endif