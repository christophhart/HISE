/*
  ==============================================================================

    SlotFX.h
    Created: 28 Jun 2017 2:51:50pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef SLOTFX_H_INCLUDED
#define SLOTFX_H_INCLUDED

namespace hise { using namespace juce;
using namespace scriptnode;

class HardcodedMasterFX: public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("Hardcoded Master FX", "HardcodedMasterFX", "A master effect wrapper around a compiled DSP network");

	HardcodedMasterFX(MainController* mc, const String& uid);

	~HardcodedMasterFX();;

	bool hasTail() const override { return true; };

	Processor *getChildProcessor(int /*processorIndex*/) override { return nullptr; };

	const Processor *getChildProcessor(int /*processorIndex*/) const override { return nullptr; };

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 0; };

	void voicesKilled() override;

	void setInternalAttribute(int index, float newValue) override
	{
		lastParameters[index] = newValue;

		SimpleReadWriteLock::ScopedReadLock sl(lock);

		if (opaqueNode != nullptr && isPositiveAndBelow(index, opaqueNode->numParameters))
			opaqueNode->parameterFunctions[index](opaqueNode->parameterObjects[index], (double)newValue);
	}

	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree& v) override;

	float getAttribute(int index) const override 
	{ 
		if (isPositiveAndBelow(index, OpaqueNode::NumMaxParameters))
			return lastParameters[index];

		return 0.0f;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setEffect(const String& factoryId);

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	Path getSpecialSymbol() const override
	{
		Path p;
		p.loadPathFromData(HnodeIcons::freezeIcon, sizeof(HnodeIcons::freezeIcon));
		return p;
	}

	void handleHiseEvent(const HiseEvent &m) override
	{
	}

	StringArray getListOfAvailableNetworks() const;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

private:

	String currentEffect = "No network";

	void prepareOpaqueNode(OpaqueNode* n);

	friend class HardcodedMasterEditor;

	float lastParameters[OpaqueNode::NumMaxParameters];
	
	scriptnode::PolyHandler polyHandler;
	mutable SimpleReadWriteLock lock;
	ScopedPointer<scriptnode::OpaqueNode> opaqueNode;
	ScopedPointer<scriptnode::dll::FactoryBase> factory;
};


/** Aplaceholder for another effect that can be swapped pretty conveniently.
	@ingroup effectTypes.
	
	Use this as building block for dynamic signal chains.
*/
class SlotFX : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SlotFX", "Effect Slot", "A placeholder for another effect that can be swapped dynamically.")

	SlotFX(MainController *mc, const String &uid);

	~SlotFX() {};

	

	bool hasTail() const override 
	{ 
		return wrappedEffect != nullptr ? wrappedEffect->hasTail() : false;
	};

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return getCurrentEffect();
	};

	void voicesKilled() override
	{
		if (wrappedEffect != nullptr)
			wrappedEffect->voicesKilled();
	}

	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return getCurrentEffect();
	};

	void updateSoftBypass() override
	{
		if (wrappedEffect != nullptr)
			wrappedEffect->updateSoftBypass();
	}

	void setSoftBypass(bool shouldBeSoftBypassed, bool useRamp) override
	{
		if (wrappedEffect != nullptr && !ProcessorHelpers::is<EmptyFX>(getCurrentEffect()))
			wrappedEffect->setSoftBypass(shouldBeSoftBypassed, useRamp);
	}

	bool isFadeOutPending() const noexcept override
	{
		if (wrappedEffect != nullptr)
			return wrappedEffect->isFadeOutPending();

		return false;
	}

	int getNumInternalChains() const override { return 0; };
	int getNumChildProcessors() const override { return 1; };

	void setInternalAttribute(int /*index*/, float /*newValue*/) override
	{

	}

    ValueTree exportAsValueTree() const override
    {
        ValueTree v = MasterEffectProcessor::exportAsValueTree();
        
        return v;
    }
    
    void restoreFromValueTree(const ValueTree& v) override
    {
		LockHelpers::noMessageThreadBeyondInitialisation(getMainController());

		MasterEffectProcessor::restoreFromValueTree(v);

        auto d = v.getChildWithName("ChildProcessors").getChild(0);
        
        setEffect(d.getProperty("Type"), true);
        
        wrappedEffect->restoreFromValueTree(d);
    }
    
	float getAttribute(int /*index*/) const override { return -1; }

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) 
	{ 
		LockHelpers::noMessageThreadBeyondInitialisation(getMainController());

		AudioThreadGuard guard(&getMainController()->getKillStateHandler());
		AudioThreadGuard::Suspender sp(isOnAir());
		LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock);

		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
		wrappedEffect->prepareToPlay(sampleRate, samplesPerBlock); 
		wrappedEffect->setKillBuffer(*killBuffer);
	}
	
	void handleHiseEvent(const HiseEvent &m) override;

	void startMonophonicVoice() override;

	void stopMonophonicVoice() override;

	void resetMonophonicVoice();

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override 
	{ 
		//
	}

	void reset()
	{
		ScopedPointer<MasterEffectProcessor> newEmptyFX;
		
		if (wrappedEffect != nullptr)
		{
			LOCK_PROCESSING_CHAIN(this);

			newEmptyFX.swapWith(wrappedEffect);
		}

		if (newEmptyFX != nullptr)
		{
			getMainController()->getGlobalAsyncModuleHandler().removeAsync(newEmptyFX.release(), ProcessorFunction());
		}

		newEmptyFX = new EmptyFX(getMainController(), "Empty");

		if (getSampleRate() > 0)
			newEmptyFX->prepareToPlay(getSampleRate(), getLargestBlockSize());

		newEmptyFX->setParentProcessor(this);
		auto newId = getId() + "_" + newEmptyFX->getId();
		newEmptyFX->setId(newId);

		{
			LOCK_PROCESSING_CHAIN(this);
			newEmptyFX.swapWith(wrappedEffect);
		}
	}

	void swap(SlotFX* otherSlot)
	{
		auto te = wrappedEffect.release();
		auto oe = otherSlot->wrappedEffect.release();

		int tempIndex = currentIndex;

		

		currentIndex = otherSlot->currentIndex;
		otherSlot->currentIndex = tempIndex;

		{
			ScopedLock sl(getMainController()->getLock());

			bool tempClear = isClear;
			isClear = otherSlot->isClear;
			otherSlot->isClear = tempClear;

			wrappedEffect = oe;
			otherSlot->wrappedEffect = te;
		}
		
		wrappedEffect.get()->sendRebuildMessage(true);
		otherSlot->wrappedEffect.get()->sendRebuildMessage(true);

		sendChangeMessage();
		otherSlot->sendChangeMessage();
	}

	int getCurrentEffectID() const { return currentIndex; }

	MasterEffectProcessor* getCurrentEffect();

	const MasterEffectProcessor* getCurrentEffect() const { return const_cast<SlotFX*>(this)->getCurrentEffect(); }

	const StringArray& getEffectList() const { return effectList; }

	/** This creates a new processor and sets it as processed FX.
	*
	*	Note that if synchronously is false, it will dispatch the initialisation
	*
	*/
	bool setEffect(const String& typeName, bool synchronously=false);

private:

	class Constrainer : public FactoryType::Constrainer
	{
		String getDescription() const override { return "No poly FX"; }

#define DEACTIVATE(x) if (typeName == x::getClassType()) return false;

		bool allowType(const Identifier &typeName) override
		{
			DEACTIVATE(PolyFilterEffect);

#if HISE_INCLUDE_OLD_MONO_FILTER
			DEACTIVATE(MonoFilterEffect);
#endif
            DEACTIVATE(PolyshapeFX);
			DEACTIVATE(HarmonicFilter);
			DEACTIVATE(HarmonicMonophonicFilter);
			DEACTIVATE(StereoEffect);
			DEACTIVATE(RouteEffect);
			DEACTIVATE(SlotFX);

			return true;
		}

#undef DEACTIVATE
	};

	void createList();

	int currentIndex = -1;

	StringArray effectList;

	bool isClear = true;
    
    bool hasScriptFX = false;

	ScopedPointer<MasterEffectProcessor> wrappedEffect;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SlotFX)
};


} // namespace hise

#endif  // SLOTFX_H_INCLUDED
