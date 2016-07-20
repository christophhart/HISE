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

#ifndef HI_EFFECT_PROCESSORCHAIN_H_INCLUDED
#define HI_EFFECT_PROCESSORCHAIN_H_INCLUDED


#define FOR_EACH_VOICE_EFFECT(x) {for(int i = 0; i < voiceEffects.size(); ++i) {if(!voiceEffects[i]->isBypassed()) voiceEffects[i]->x;}}
#define FOR_EACH_MONO_EFFECT(x) {for(int i = 0; i < monoEffects.size(); ++i) {if(!monoEffects[i]->isBypassed())monoEffects[i]->x;}}
#define FOR_EACH_MASTER_EFFECT(x) {for(int i = 0; i < masterEffects.size(); ++i) {if(!masterEffects[i]->isBypassed())masterEffects[i]->x;}}

#define FOR_ALL_EFFECTS(x) {for(int i = 0; i < allEffects.size(); ++i) {if(!allEffects[i]->isBypassed())allEffects[i]->x;}}


/** A EffectProcessorChain renders multiple EffectProcessors.
*	@ingroup effect
*
*	It renders MasterEffectProcessor objects and VoiceEffectProcessorObjects seperately.
*/
class EffectProcessorChain: public VoiceEffectProcessor,
							public Chain
{
public:

	SET_PROCESSOR_NAME("EffectChain", "FX Chain")

	EffectProcessorChain(Processor *parentProcessor, const String &id, int numVoices);

	ChainHandler *getHandler() override {return &handler;};

	const ChainHandler *getHandler() const override {return &handler;};

	Processor *getParentProcessor() override { return parentProcessor; };

	const Processor *getParentProcessor() const override { return parentProcessor; };

	

	FactoryType *getFactoryType() const override {return effectChainFactory;};

	Colour getColour() const { return Colour(0xff3a6666).withMultipliedBrightness(1.1f);};

	void setFactoryType(FactoryType *newFactoryType) override {effectChainFactory = newFactoryType;};

	float getAttribute(int ) const override {return 1.0f;	};

	void setInternalAttribute(int , float ) override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		// Skip the effectProcessor's prepareToPlay since it assumes all child processors are ModulatorChains
		Processor::prepareToPlay(sampleRate, samplesPerBlock);


		for(int i = 0; i < allEffects.size(); i++) allEffects[i]->prepareToPlay(sampleRate, samplesPerBlock);
	};

	void handleMidiEvent(const MidiMessage &m) override
	{	
		if(isBypassed()) return;
		FOR_ALL_EFFECTS(handleMidiEvent(m)); 
	};

	void preRenderCallback(int startSample, int numSamples) override
	{
		if(isBypassed()) return;
		FOR_EACH_VOICE_EFFECT(preRenderCallback(startSample, numSamples));
	}

	void applyEffect(int /*voiceIndex*/, AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override {};

	void preVoiceRendering(int /*voiceIndex*/, int /*startSample*/, int /*numSamples*/) override {};

	void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override 
	{ 
		if(isBypassed()) return;

        ADD_GLITCH_DETECTOR("Rendering voice effects for" + parentProcessor->getId());
        
		FOR_EACH_VOICE_EFFECT(renderVoice(voiceIndex, b, startSample, numSamples)); 
	};

	void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples) override
	{
		if(isBypassed()) return;

		FOR_ALL_EFFECTS(renderNextBlock(buffer, startSample, numSamples));
	};

	bool hasTail() const override
	{
		for(int i = 0; i < allEffects.size(); i++)
		{
			if (allEffects[i]->hasTail()) return true;
		}

		return false;
	};

	bool isTailingOff() const override
	{
		for(int i = 0; i < allEffects.size(); i++)
		{
			if (allEffects[i]->isTailingOff()) return true;
		}

		return false;
	};

	void renderMasterEffects(AudioSampleBuffer &b)
	{
		if(isBypassed()) return;

        ADD_GLITCH_DETECTOR("Rendering master effects for" + parentProcessor->getId());
        
		FOR_EACH_MASTER_EFFECT(renderWholeBuffer(b));

		currentValues.outL = (b.getMagnitude(0, 0, b.getNumSamples()));
		currentValues.outR = (b.getMagnitude(1, 0, b.getNumSamples()));

	}

	AudioSampleBuffer & getBufferForChain(int /*index*/)
	{
		jassertfalse;
		return emptyBuffer;
	};

	void startVoice(int voiceIndex, int noteNumber) override 
	{
		if(isBypassed()) return;
		FOR_EACH_VOICE_EFFECT(startVoice(voiceIndex, noteNumber)); 
		FOR_EACH_MONO_EFFECT(startMonophonicVoice(noteNumber));
		FOR_EACH_MASTER_EFFECT(startMonophonicVoice());
	};

	void stopVoice(int voiceIndex) override 
	{
		if(isBypassed()) return;
		FOR_EACH_VOICE_EFFECT(stopVoice(voiceIndex));	
		FOR_EACH_MONO_EFFECT(stopMonophonicVoice());
		FOR_EACH_MASTER_EFFECT(stopMonophonicVoice());
	};

	void reset(int voiceIndex)
	{ 
		if(isBypassed()) return;
		FOR_EACH_VOICE_EFFECT(reset(voiceIndex));	
		FOR_EACH_MONO_EFFECT(resetMonophonicVoice());
		FOR_EACH_MASTER_EFFECT(resetMonophonicVoice());
	};

	int getNumChildProcessors() const override { return getHandler()->getNumProcessors(); };

	Processor *getChildProcessor(int processorIndex) override { return getHandler()->getProcessor(processorIndex); };

	const Processor *getChildProcessor(int processorIndex) const override { return getHandler()->getProcessor(processorIndex); };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	class EffectChainHandler: public ChainHandler
	{
	public:

		/** Creates a ChainHandler. */
		EffectChainHandler(EffectProcessorChain *handledChain): chain(handledChain) {};

		~EffectChainHandler() {};

		/** adds a Effect to the chain and calls its prepareToPlay method. 
		*
		*	You simply pass the reference to the newly created EffectProcessor, and the function detects
		*	the correct type and adds it to the specific chain (constant, variant or envelope).
		*
		*	If you call this method after the EffectProcessorChain is initialized, the EffectProcessor's prepareToPlay will be called.	
		*/
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;

		void remove(Processor *processorToBeRemoved) override
		{
			ScopedLock sl(chain->getMainController()->getLock());

			jassert(dynamic_cast<EffectProcessor*>(processorToBeRemoved) != nullptr);
			
			chain->allEffects.removeAllInstancesOf(dynamic_cast<EffectProcessor*>(processorToBeRemoved));

			if(VoiceEffectProcessor* vep = dynamic_cast<VoiceEffectProcessor*>(processorToBeRemoved)) chain->voiceEffects.removeObject(vep);
			else if (MasterEffectProcessor* mep = dynamic_cast<MasterEffectProcessor*>(processorToBeRemoved)) chain->masterEffects.removeObject(mep);
			else if (MonophonicEffectProcessor* mep = dynamic_cast<MonophonicEffectProcessor*>(processorToBeRemoved)) chain->monoEffects.removeObject(mep);
			else jassertfalse;

			jassert(chain->allEffects.size() == (chain->masterEffects.size() + chain->voiceEffects.size() + chain->monoEffects.size()));

			sendChangeMessage();
		}

		void moveProcessor(Processor *processorInChain, int delta)
		{
			if (MasterEffectProcessor *mep = dynamic_cast<MasterEffectProcessor*>(processorInChain))
			{
				const int indexOfProcessor = chain->masterEffects.indexOf(mep);
				jassert(indexOfProcessor != -1);

				const int indexOfSwapProcessor = jlimit<int>(0, chain->masterEffects.size(), indexOfProcessor + delta);

				const int indexOfProcessorInAllEffects = chain->allEffects.indexOf(mep);
				const int indexOfSwapProcessorInAllEfects = jlimit<int>(0, chain->allEffects.size(), indexOfProcessorInAllEffects + delta);

				if (indexOfProcessor != indexOfSwapProcessor)
				{
					ScopedLock sl(chain->getMainController()->getLock());

					chain->masterEffects.swap(indexOfProcessor, indexOfSwapProcessor);
					chain->allEffects.swap(indexOfProcessorInAllEffects, indexOfSwapProcessorInAllEfects);
				}
			}
		}

		Processor *getProcessor(int processorIndex)
		{
			if (processorIndex < chain->voiceEffects.size())
			{
				return chain->voiceEffects.getUnchecked(processorIndex);
			}

			processorIndex -= chain->voiceEffects.size();

			if (processorIndex < chain->monoEffects.size())
			{
				return chain->monoEffects.getUnchecked(processorIndex);
			}

			processorIndex -= chain->monoEffects.size();

			if (processorIndex < chain->masterEffects.size())
			{
				return chain->masterEffects.getUnchecked(processorIndex);
			}

			jassertfalse;

			return chain->allEffects.getUnchecked(processorIndex);
		};

		const Processor *getProcessor(int processorIndex) const
		{
			

			if (processorIndex < chain->voiceEffects.size())
			{
				return chain->voiceEffects.getUnchecked(processorIndex);
			}

			processorIndex -= chain->voiceEffects.size();

			if (processorIndex < chain->monoEffects.size())
			{
				return chain->monoEffects.getUnchecked(processorIndex);
			}

			processorIndex -= chain->monoEffects.size();

			if (processorIndex < chain->masterEffects.size())
			{
				return chain->masterEffects.getUnchecked(processorIndex);
			}

			jassertfalse;

			return chain->allEffects.getUnchecked(processorIndex);
			
		};

		virtual int getNumProcessors() const
		{
			return chain->allEffects.size(); 
		};

		void clear() override
		{
			chain->voiceEffects.clear();
			chain->masterEffects.clear();
			chain->monoEffects.clear();
			chain->allEffects.clear();

			sendChangeMessage();
		}

	private:

		EffectProcessorChain *chain;

		


	};

private:

	// This is used for getBufferForChain
	AudioSampleBuffer emptyBuffer;

	EffectChainHandler handler;

	OwnedArray<VoiceEffectProcessor> voiceEffects;
	OwnedArray<MasterEffectProcessor> masterEffects;
	OwnedArray<MonophonicEffectProcessor> monoEffects;

	Array<EffectProcessor*> allEffects;

	Processor *parentProcessor;

	ScopedPointer<FactoryType> effectChainFactory;
};


class EffectProcessorChainFactoryType: public FactoryType
{
public:

	enum
	{
		monophonicFilter = 0,
		polyphonicFilter,
		harmonicFilter,
		harmonicFilterMono,
		curveEq,
		stereoEffect,
		simpleReverb,
		simpleGain,
		convolution,
		delay,
		limiter,
		degrade,
		chorus,
        phaser,
		gainCollector,
		routeFX,
		saturation,
		audioProcessorWrapper,
		scriptFxProcessor,
		protoPlugEffect
	};

	EffectProcessorChainFactoryType(int numVoices_, Processor *ownerProcessor):
		FactoryType(ownerProcessor),
		numVoices(numVoices_)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	Processor* createProcessor	(int typeIndex, const String &id) override;
	
protected:

	const Array<ProcessorEntry>& getTypeNames() const override
	{
		return typeNames;
	};

private:

	Array<ProcessorEntry> typeNames;

	int numVoices;

};




#endif