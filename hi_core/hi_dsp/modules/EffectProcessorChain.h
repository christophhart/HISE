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

#ifndef HI_EFFECT_PROCESSORCHAIN_H_INCLUDED
#define HI_EFFECT_PROCESSORCHAIN_H_INCLUDED

namespace hise { using namespace juce;

#define FOR_EACH_VOICE_EFFECT(x) {for(int i = 0; i < voiceEffects.size(); ++i) {if(!voiceEffects[i]->isBypassed()) voiceEffects[i]->x;}}
#define FOR_EACH_MONO_EFFECT(x) {for(int i = 0; i < monoEffects.size(); ++i) {if(!monoEffects[i]->isBypassed())monoEffects[i]->x;}}
#define FOR_EACH_MASTER_EFFECT(x) {for(int i = 0; i < masterEffects.size(); ++i) {if(!masterEffects[i]->isSoftBypassed())masterEffects[i]->x;}}

#define FOR_ALL_EFFECTS(x) {for(int i = 0; i < allEffects.size(); ++i) {if(!allEffects[i]->isBypassed())allEffects[i]->x;}}


/** A EffectProcessorChain renders multiple EffectProcessors.
*	@ingroup effect
*
*	It renders MasterEffectProcessor objects and VoiceEffectProcessorObjects seperately.
*/
class EffectProcessorChain: public Processor,
							public Chain
{
public:

	SET_PROCESSOR_NAME("EffectChain", "FX Chain", "chain");

	EffectProcessorChain(Processor *parentProcessor, const String &id, int numVoices);

	~EffectProcessorChain();

	Chain::Handler *getHandler() override;;

	const Chain::Handler *getHandler() const override;;

	Processor *getParentProcessor() override;;

	const Processor *getParentProcessor() const override;;

	FactoryType *getFactoryType() const override;;

	Colour getColour() const;;

	void setFactoryType(FactoryType *newFactoryType) override;;

	float getAttribute(int ) const override;;

	void setInternalAttribute(int , float ) override;;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	void handleHiseEvent(const HiseEvent &m);;

	void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples);;

	void preRenderCallback(int startSample, int numSamples);

	void resetMasterEffects();

	void renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples);;

	bool hasTailingMasterEffects() const noexcept;

	bool hasTailingPolyEffects() const;

	void killMasterEffects();

	void updateSoftBypassState();

	void renderMasterEffects(AudioSampleBuffer &b);

	void startVoice(int voiceIndex, const HiseEvent& e);;

	void stopVoice(int voiceIndex);;

	void reset(int voiceIndex);;

	int getNumChildProcessors() const override;;

	Processor *getChildProcessor(int processorIndex) override;;

	const Processor *getChildProcessor(int processorIndex) const override;;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	class EffectChainHandler: public Chain::Handler
	{
	public:

		/** Creates a Chain::Handler. */
		EffectChainHandler(EffectProcessorChain *handledChain);;

		~EffectChainHandler();;

		/** adds a Effect to the chain and calls its prepareToPlay method. 
		*
		*	You simply pass the reference to the newly created EffectProcessor, and the function detects
		*	the correct type and adds it to the specific chain (constant, variant or envelope).
		*
		*	If you call this method after the EffectProcessorChain is initialized, the EffectProcessor's prepareToPlay will be called.	
		*/
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;

		void remove(Processor *processorToBeRemoved, bool removeEffect=true) override;

		void moveProcessor(Processor *processorInChain, int delta);

		Processor *getProcessor(int processorIndex) override;

		const Processor *getProcessor(int processorIndex) const override;

		int getNumProcessors() const override;

		void clear() override;

	private:

		EffectProcessorChain *chain;
	};

	/** Enable this to enforce the rendering of polyphonic effects (namely the filter in a container effect chain. */
	void setForceMonophonicProcessingOfPolyphonicEffects(bool shouldProcessPolyFX);

private:

	bool renderPolyFxAsMono = false;

	// Gives it a limit of 6 million years...
	int64 resetCounter = -1;
	int resetCounterStartValue = 22050;

	bool tailActive = false;

	AudioSampleBuffer killBuffer;

	EffectChainHandler handler;

	OwnedArray<VoiceEffectProcessor> voiceEffects;
	OwnedArray<MasterEffectProcessor> masterEffects;
	OwnedArray<MonophonicEffectProcessor> monoEffects;

	Array<EffectProcessor*, DummyCriticalSection, 32> allEffects;

	Processor *parentProcessor;

	ScopedPointer<FactoryType> effectChainFactory;
};


class EffectProcessorChainFactoryType: public FactoryType
{
public:

	enum
	{
		polyphonicFilter=0,
		harmonicFilter,
		harmonicFilterMono,
		curveEq,
		stereoEffect,
		simpleReverb,
		simpleGain,
		convolution,
		delay,
		chorus,
        phaser,
		routeFX,
		sendFX,
		saturation,
		scriptFxProcessor,
		polyScriptFxProcessor,
		slotFX,
		emptyFX,
		dynamics,
		analyser,
		shapeFX,
		polyshapeFx,
		hardcodedMasterFx,
		polyHardcodedFx,
		midiMetronome
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


} // namespace hise

#endif
