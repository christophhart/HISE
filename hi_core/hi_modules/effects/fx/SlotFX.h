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

class HotswappableProcessor
{
public:

	virtual ~HotswappableProcessor() {};

	virtual bool setEffect(const String& name, bool synchronously) = 0;
	virtual bool swap(HotswappableProcessor* other) = 0;

	virtual void clearEffect() { setEffect("", false); }

	virtual StringArray getModuleList() const = 0;

	virtual Processor* getCurrentEffect() = 0;
	virtual const Processor* getCurrentEffect() const = 0;
    
    virtual String getCurrentEffectId() const = 0;
    
    virtual var getParameterProperties() const = 0;
};





/** Aplaceholder for another effect that can be swapped pretty conveniently.
	@ingroup effectTypes.
	
	Use this as building block for dynamic signal chains.
*/
class SlotFX : public MasterEffectProcessor,
			   public HotswappableProcessor
{
public:

	SET_PROCESSOR_NAME("SlotFX", "Effect Slot", "")

	static ProcessorMetadata createMetadata()
	{
		return ProcessorMetadata(getClassType())
			.withDescription("A placeholder for another effect that can be swapped dynamically.")
			.withStandardMetadata<SlotFX>()
			.withChildFXConstrainer<SlotFX::Constrainer>();
	}

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
		LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);

		MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
		wrappedEffect->prepareToPlay(sampleRate, samplesPerBlock); 
		wrappedEffect->setKillBuffer(*killBuffer);
	}
	
    var getParameterProperties() const override { return var(); };
    
    String getCurrentEffectId() const override { return isPositiveAndBelow(currentIndex, effectList.size()) ? effectList[currentIndex] : "No Effect"; }
    
	void handleHiseEvent(const HiseEvent &m) override;

	void startMonophonicVoice() override;

	void stopMonophonicVoice() override;

	void resetMonophonicVoice();

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override 
	{ 
		//
	}

	void clearEffect() override
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

	bool swap(HotswappableProcessor* otherSlot) override;

	int getCurrentEffectID() const { return currentIndex; }

	Processor* getCurrentEffect() override;

	const Processor* getCurrentEffect() const override { return const_cast<SlotFX*>(this)->getCurrentEffect(); }

	StringArray getModuleList() const override { return effectList; }

	/** This creates a new processor and sets it as processed FX.
	*
	*	Note that if synchronously is false, it will dispatch the initialisation
	*
	*/
	bool setEffect(const String& typeName, bool synchronously=false);

	class Constrainer : public EffectProcessorChainFactoryType::NoVoiceEffectConstrainer
	{
	public:

		static ProcessorMetadata::WildcardFilterList getWildcard()
		{
			return {
				{
					ProcessorMetadataIds::Effect,
					ProcessorMetadataIds::MasterEffect.toString() + "|" + 
					ProcessorMetadataIds::MonophonicEffect.toString() + "|" +
				    makeNegativeFilterWildcard<RouteEffect, SlotFX>()
				}
			};
		};

		String getDescription() const override { return "slot fx constrainer"; }

		bool allowType(const Identifier& typeName) override
		{
			if (!NoVoiceEffectConstrainer::allowType(typeName))
				return false;

			if (typeName == RouteEffect::getClassType()) return false;
			if (typeName == SlotFX::getClassType()) return false;

			return true;
		}

	};

private:

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
