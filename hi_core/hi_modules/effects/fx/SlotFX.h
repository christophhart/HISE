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



class HardcodedSwappableEffect : public HotswappableProcessor,
							     public ProcessorWithExternalData,
								 public RuntimeTargetHolder
{
public:

	virtual ~HardcodedSwappableEffect();

	// ===================================================================================== Complex Data API calls

	int getNumDataObjects(ExternalData::DataType t) const override;
	Table* getTable(int index) override { return getOrCreate<Table>(tables, index); }
	SliderPackData* getSliderPack(int index) override { return getOrCreate<SliderPackData>(sliderPacks, index); }
	MultiChannelAudioBuffer* getAudioFile(int index) override { return getOrCreate<MultiChannelAudioBuffer>(audioFiles, index); }
	FilterDataObject* getFilterData(int index) override { return getOrCreate<FilterDataObject>(filterData, index);; }
	SimpleRingBuffer* getDisplayBuffer(int index) override { return getOrCreate<SimpleRingBuffer>(displayBuffers, index); }

	LambdaBroadcaster<String> errorBroadcaster;

	// ===================================================================================== Custom hardcoded API calls

	Processor* getCurrentEffect() { return dynamic_cast<Processor*>(this); }
	const Processor* getCurrentEffect() const override { return dynamic_cast<const Processor*>(this); }

	StringArray getModuleList() const override;
	bool setEffect(const String& factoryId, bool /*unused*/) override;
	bool swap(HotswappableProcessor* other) override;
	bool isPolyphonic() const { return polyHandler.isEnabled(); }

    String getCurrentEffectId() const override { return currentEffect; }
    
	Processor& asProcessor() { return *dynamic_cast<Processor*>(this); }
	const Processor& asProcessor() const { return *dynamic_cast<const Processor*>(this); }

	// ===================================================================================== Processor API tool functions

	Result sanityCheck();

	void setHardcodedAttribute(int index, float newValue);
	float getHardcodedAttribute(int index) const;
	Path getHardcodedSymbol() const;
	ProcessorEditorBody* createHardcodedEditor(ProcessorEditor* parent);
	void restoreHardcodedData(const ValueTree& v);
	ValueTree writeHardcodedData(ValueTree& v) const;

	virtual bool checkHardcodedChannelCount();

	bool processHardcoded(AudioSampleBuffer& b, HiseEventBuffer* e, int startSample, int numSamples);

	virtual void renderData(ProcessDataDyn& data);

	bool hasHardcodedTail() const;

    var getParameterProperties() const override;
    
    void disconnectRuntimeTargets(MainController* mc) override;
    void connectRuntimeTargets(MainController* mc) override;
    
protected:
	
	HardcodedSwappableEffect(MainController* mc, bool isPolyphonic);

	struct DataWithListener : public ComplexDataUIUpdaterBase::EventListener
	{
		DataWithListener(HardcodedSwappableEffect& parent, ComplexDataUIBase* p, int index_, OpaqueNode* nodeToInitialise);;

		~DataWithListener()
		{
			if (data != nullptr)
				data->getUpdater().removeEventListener(this);
		}

		void updateData()
		{
			if (node != nullptr)
			{
				SimpleReadWriteLock::ScopedWriteLock sl(data->getDataLock());

				ExternalData ed(data.get(), index);

				SimpleRingBuffer::ScopedPropertyCreator sps(data.get());

				node->setExternalData(ed, index);
			}

		}

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var newValue) override
		{
			if (t == ComplexDataUIUpdaterBase::EventType::ContentRedirected ||
				t == ComplexDataUIUpdaterBase::EventType::ContentChange)
			{
				updateData();
			}
		}

		OpaqueNode* node;
		const int index = 0;
		ComplexDataUIBase::Ptr data;
	};

	OwnedArray<DataWithListener> listeners;

	LambdaBroadcaster<String, int, bool> effectUpdater;

	template <typename T> T* getOrCreate(ReferenceCountedArray<T>& list, int index)
	{
		if (isPositiveAndBelow(index, list.size()))
			return list[index].get();

		auto t = createAndInit(ExternalData::getDataTypeForClass<T>());
		list.add(dynamic_cast<T*>(t));
		return list.getLast().get();
	}

	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
	ReferenceCountedArray<SimpleRingBuffer> displayBuffers;
	ReferenceCountedArray<FilterDataObject> filterData;

	ValueTree previouslySavedTree;
	bool properlyLoaded = true;

	String currentEffect = "No network";

	float* getParameterPtr(int index) const
	{
		if(isPositiveAndBelow(index, numParameters))
			return static_cast<float*>(lastParameters.getObjectPtr()) + index;
		return
			nullptr;
	}

	int numParameters = 0;
	ObjectStorage<sizeof(float) * OpaqueNode::NumMaxParameters, 8> lastParameters;
	
	snex::Types::ModValue modValue;
	snex::Types::DllBoundaryTempoSyncer tempoSyncer;
	scriptnode::PolyHandler polyHandler;
	mutable SimpleReadWriteLock lock;
	ScopedPointer<scriptnode::OpaqueNode> opaqueNode;
	ScopedPointer<scriptnode::dll::FactoryBase> factory;

	virtual Result prepareOpaqueNode(OpaqueNode* n);

	bool channelCountMatches = false;

	int channelIndexes[NUM_MAX_CHANNELS];
	int numChannelsToRender = 0;

	int hash = -1;

    Array<scriptnode::InvertableParameterRange> parameterRanges;
    
private:

	MainController* mc_;
	friend class HardcodedMasterEditor;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HardcodedSwappableEffect);
};

class HardcodedMasterFX: public MasterEffectProcessor,
						 public HardcodedSwappableEffect
{
public:

	SET_PROCESSOR_NAME("Hardcoded Master FX", "HardcodedMasterFX", "A master effect wrapper around a compiled DSP network");

	HardcodedMasterFX(MainController* mc, const String& uid);
	~HardcodedMasterFX();;

	bool hasTail() const override;

	bool isSuspendedOnSilence() const override;

    bool isFadeOutPending() const noexcept override
    {
        if(numChannelsToRender == 2)
            return MasterEffectProcessor::isFadeOutPending();
        
        return false;
    }
    
#if NUM_HARDCODED_FX_MODS
	Processor *getChildProcessor(int processorIndex) override { return isPositiveAndBelow(processorIndex, NUM_HARDCODED_FX_MODS) ? paramModulation[processorIndex] : nullptr; };
    const Processor *getChildProcessor(int processorIndex) const override { return isPositiveAndBelow(processorIndex, NUM_HARDCODED_FX_MODS) ? paramModulation[processorIndex] : nullptr; };
#else
	Processor *getChildProcessor(int ) override { return nullptr; };
	const Processor *getChildProcessor(int ) const override { return nullptr; };
#endif

	int getNumInternalChains() const override { return NUM_HARDCODED_FX_MODS; };
	int getNumChildProcessors() const override { return NUM_HARDCODED_FX_MODS; };

	void connectionChanged()
	{
		MasterEffectProcessor::connectionChanged();
        channelCountMatches = checkHardcodedChannelCount();
	}

    

	void voicesKilled() override;
	void setInternalAttribute(int index, float newValue) override;
	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree& v) override;
	float getAttribute(int index) const override;

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	Path getSpecialSymbol() const override;
	void handleHiseEvent(const HiseEvent &m) override;
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) final override;

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;

#if NUM_HARDCODED_FX_MODS
	ModulatorChain* paramModulation[NUM_HARDCODED_FX_MODS];
#endif

};


/** A simple stereo panner which can be modulated using all types of Modulators
*	@ingroup effectTypes
*
*/
class HardcodedPolyphonicFX : public VoiceEffectProcessor,
						      public HardcodedSwappableEffect,
							  public RoutableProcessor,
						      public VoiceResetter
{
public:

	SET_PROCESSOR_NAME("HardcodedPolyphonicFX", "Hardcoded Polyphonic FX", "A polyphonic hardcoded FX.");

	HardcodedPolyphonicFX(MainController *mc, const String &uid, int numVoices);;

	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override;;

	bool isSuspendedOnSilence() const final override;

#if NUM_HARDCODED_POLY_FX_MODS
	Processor *getChildProcessor(int processorIndex) override { return isPositiveAndBelow(processorIndex, NUM_HARDCODED_POLY_FX_MODS) ? paramModulation[processorIndex] : nullptr; };
    const Processor *getChildProcessor(int processorIndex) const override { return isPositiveAndBelow(processorIndex, NUM_HARDCODED_POLY_FX_MODS) ? paramModulation[processorIndex] : nullptr; };
#else
	Processor *getChildProcessor(int ) override { return nullptr; };
	const Processor *getChildProcessor(int ) const override { return nullptr; };
#endif

	
	int getNumChildProcessors() const override { return NUM_HARDCODED_POLY_FX_MODS; };
	int getNumInternalChains() const override { return NUM_HARDCODED_POLY_FX_MODS; };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) final override;

	void startVoice(int voiceIndex, const HiseEvent& e) final override;

	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) final override;

	void renderData(ProcessDataDyn& data) override;

	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int /*startSample*/, int /*numSamples*/) override
	{
		
	}

	void reset(int voiceIndex) override 
	{
		VoiceEffectProcessor::reset(voiceIndex);

		voiceStack.reset(voiceIndex);
	}
	
	void handleHiseEvent(const HiseEvent &m) override;


	void connectionChanged() override
	{
        channelCountMatches = checkHardcodedChannelCount();
		
	};

	void numSourceChannelsChanged() override {};
	void numDestinationChannelsChanged() override {};

	/** renders a voice and applies the effect on the voice. */
	void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	int getNumActiveVoices() const override
	{
		return voiceStack.voiceNoteOns.size();
	}

	bool isVoiceResetActive() const override
	{
		return hasHardcodedTail();
	}

	void onVoiceReset(bool allVoices, int voiceIndex) override
	{
		if (allVoices)
			voiceStack.voiceNoteOns.clear();
		else
			voiceStack.reset(voiceIndex);
	}

	VoiceDataStack voiceStack;

#if NUM_HARDCODED_POLY_FX_MODS
	ModulatorChain* paramModulation[NUM_HARDCODED_POLY_FX_MODS];
#endif
};

class HardcodedTimeVariantModulator: public TimeVariantModulator,
                                     public HardcodedSwappableEffect
{
public:
    
    SET_PROCESSOR_NAME("Hardcoded Timevariant Modulator", "HardcodedTimeVariantModulator", "Atime variant modulator wrapper around a compiled DSP network");

    HardcodedTimeVariantModulator(MainController* mc, const String& uid, Modulation::Mode m);
    ~HardcodedTimeVariantModulator();;

    Processor *getChildProcessor(int processorIndex) override { return nullptr; };
    const Processor *getChildProcessor(int processorIndex) const override { return nullptr; }

    int getNumInternalChains() const override { return 0; };
    int getNumChildProcessors() const override { return 0; };

    void setInternalAttribute(int index, float newValue) override;
    ValueTree exportAsValueTree() const override;
    void restoreFromValueTree(const ValueTree& v) override;
    float getAttribute(int index) const override;

    bool checkHardcodedChannelCount() override;
    
    ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);

    void handleHiseEvent(const HiseEvent &m) override;
    
    void calculateBlock(int startSample, int numSamples) override;
    
    Result prepareOpaqueNode(OpaqueNode* n) override;
};













/** Aplaceholder for another effect that can be swapped pretty conveniently.
	@ingroup effectTypes.
	
	Use this as building block for dynamic signal chains.
*/
class SlotFX : public MasterEffectProcessor,
			   public HotswappableProcessor
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
