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

/** A simple gain effect that allows time variant modulation. */
class SlotFX : public MasterEffectProcessor
{
public:

	SET_PROCESSOR_NAME("SlotFX", "Effect Slot")

	SlotFX(MainController *mc, const String &uid);

	~SlotFX() {};

	

	bool hasTail() const override { return false; };

	Processor *getChildProcessor(int /*processorIndex*/) override
	{
		return wrappedEffect;
	};


	const Processor *getChildProcessor(int /*processorIndex*/) const override
	{
		return wrappedEffect;
	};

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
		MasterEffectProcessor::restoreFromValueTree(v);

        auto d = v.getChildWithName("ChildProcessors").getChild(0);
        
		
        
        setEffect(d.getProperty("Type"), true);
        
		ScopedLock sl(getMainController()->getLock());
        wrappedEffect->restoreFromValueTree(d);
    }
    
	float getAttribute(int /*index*/) const override { return -1; }

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) 
	{ 
		ScopedLock sl(getMainController()->getLock());

		Processor::prepareToPlay(sampleRate, samplesPerBlock);
		wrappedEffect->prepareToPlay(sampleRate, samplesPerBlock); 
	}
	
	void renderWholeBuffer(AudioSampleBuffer &buffer) override;

	void applyEffect(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSamples*/) override 
	{ 
		//
	}

	void reset()
	{
		isClear = true;
		
		setEffect(EmptyFX::getClassType().toString(), true);
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

			wrappedEffect = oe;
			otherSlot->wrappedEffect = te;
		}
		
		wrappedEffect.get()->sendRebuildMessage(true);
		otherSlot->wrappedEffect.get()->sendRebuildMessage(true);

		sendChangeMessage();
		otherSlot->sendChangeMessage();
	}

	int getCurrentEffectID() const { return currentIndex; }

	MasterEffectProcessor* getCurrentEffect() { return wrappedEffect.get(); }

	const StringArray& getEffectList() const { return effectList; }

	bool setEffect(const String& typeName, bool synchronously=false);

private:

	

	class Constrainer : public FactoryType::Constrainer
	{
#define DEACTIVATE(x) if (typeName == x::getClassType()) return false;

		bool allowType(const Identifier &typeName) override
		{
			DEACTIVATE(PolyFilterEffect);
			DEACTIVATE(MonoFilterEffect);
			DEACTIVATE(HarmonicFilter);
			DEACTIVATE(HarmonicMonophonicFilter);
			DEACTIVATE(StereoEffect);
			DEACTIVATE(RouteEffect);
			DEACTIVATE(AudioProcessorWrapper);
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
};


} // namespace hise

#endif  // SLOTFX_H_INCLUDED
