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

	std::pair<bool, int> getProcessingOrderIndex(MasterEffectProcessor* fx) const
	{
		if(useReorderedMasterFX)
			return { true, reorderedMasterEffects.indexOf(fx) };
		else
			return { false, masterEffects.indexOf(fx) };
	}

	std::pair<bool, int> getProcessingOrderIndex(VoiceEffectProcessor* fx) const
	{
		if(useReorderedPolyFX)
			return { true, reorderedPolyEffects.indexOf(fx) };
		else
			return { false, voiceEffects.indexOf(fx) };
	}

	template <typename T> void setFXOrderInternal(Range<int> dynamicRange, const var& newOrder)
	{
		auto& use = std::is_same<T, MasterEffectProcessor>() ? useReorderedMasterFX : useReorderedPolyFX;

		Array<T*>* target;
		OwnedArray<T>* source;

		if constexpr(std::is_same<T, MasterEffectProcessor>())
		{
			target = &reorderedMasterEffects;
			source = &masterEffects;
		}
		else
		{
			target = &reorderedPolyEffects;
			source = &voiceEffects;
		}

		Array<T*> newReorderedFX;

		bool changed = false;

		if(newOrder.isArray())
		{
			for(int i = 0; i < dynamicRange.getStart(); i++)
			{
				if(isPositiveAndBelow(i, source->size()))
					newReorderedFX.add(source->getUnchecked(i));
			}

			for(const auto& v: *newOrder.getArray())
			{
				auto idx = (int)v;

				jassert(dynamicRange.isEmpty() || dynamicRange.contains(idx));
				
				if(isPositiveAndBelow(idx, source->size()))
					newReorderedFX.add(source->getUnchecked(idx));
			}

			if(!dynamicRange.isEmpty())
			{
				for(int i = dynamicRange.getEnd(); i < source->size(); i++)
					newReorderedFX.add(source->getUnchecked(i));
			}

			{
				LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);
				newReorderedFX.swapWith(*target);
				use = true;
				changed = true;
			}

			for(auto fx: *source)
				fx->setBypassed(!target->contains(fx), sendNotificationAsync);
		}
		else
		{
			use = false;
		}

#if USE_BACKEND
		if(changed)
			getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, MainController::ProcessorChangeHandler::EventType::RebuildModuleList, false);
#endif

	}

	void setFXOrder(bool changePolyOrder, Range<int> dynamicRange, var newOrder)
	{
		if(changePolyOrder)
		{
			this->setFXOrderInternal<VoiceEffectProcessor>(dynamicRange, newOrder);
		}
		else
		{
			this->setFXOrderInternal<MasterEffectProcessor>(dynamicRange, newOrder);
		}
#if 0
		bool changed = false;

		if(newOrder.isArray())
		{
			Array<MasterEffectProcessor*> newReorderedFX;

			for(int i = 0; i < dynamicRange.getStart(); i++)
			{
				if(isPositiveAndBelow(i, masterEffects.size()))
					newReorderedFX.add(masterEffects[i]);
			}

			for(const auto& v: *newOrder.getArray())
			{
				auto idx = (int)v;

				jassert(dynamicRange.isEmpty() || dynamicRange.contains(idx));
				
				if(isPositiveAndBelow(idx, masterEffects.size()))
					newReorderedFX.add(masterEffects[idx]);
			}

			if(!dynamicRange.isEmpty())
			{
				for(int i = dynamicRange.getEnd(); i < masterEffects.size(); i++)
					newReorderedFX.add(masterEffects[i]);
			}

			{
				LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);
				newReorderedFX.swapWith(reorderedEffects);
				useReorderedFX = true;
				changed = true;
			}

			for(auto mfx: masterEffects)
					mfx->setBypassed(!reorderedEffects.contains(mfx), sendNotificationAsync);
		}
		else
		{
			useReorderedFX = false;
		}

#if USE_BACKEND
		if(changed)
			getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, MainController::ProcessorChangeHandler::EventType::RebuildModuleList, false);
#endif
#endif
	}

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

	template <typename T> struct FXIterator
	{
		FXIterator(EffectProcessorChain& parent):
		  p(parent)
		{};

		T** begin() const
		{
			if constexpr (std::is_same<T, MasterEffectProcessor>())
				return p.useReorderedMasterFX ? p.reorderedMasterEffects.begin() : p.masterEffects.begin();
			else
				return p.useReorderedPolyFX ? p.reorderedPolyEffects.begin() : p.voiceEffects.begin();
				
		};

		T** end() const
		{
			if constexpr (std::is_same<T, MasterEffectProcessor>())
				return p.useReorderedMasterFX ? p.reorderedMasterEffects.end() : p.masterEffects.end();
			else
				return p.useReorderedPolyFX ? p.reorderedPolyEffects.end() : p.voiceEffects.end();
		};

		EffectProcessorChain& p;
	};

	bool renderPolyFxAsMono = false;

	// Gives it a limit of 6 million years...
	int64 resetCounter = -1;
	int resetCounterStartValue = 22050;

	bool tailActive = false;

	AudioSampleBuffer killBuffer;

	EffectChainHandler handler;

	OwnedArray<VoiceEffectProcessor> voiceEffects;
	OwnedArray<MasterEffectProcessor> masterEffects;
	Array<MasterEffectProcessor*> reorderedMasterEffects;
	Array<VoiceEffectProcessor*> reorderedPolyEffects;
	bool useReorderedMasterFX = false;
	bool useReorderedPolyFX = false;

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
		midiMetronome,
		noiseGrainPlayer
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
