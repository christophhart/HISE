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

#pragma once

namespace hise
{
using namespace juce;

class HardcodedSwappableEffect : public HotswappableProcessor,
							     public ProcessorWithExternalData,
								 public ProcessorWithCustomFilterStatistics,
								 public RuntimeTargetHolder
{
public:

	static Identifier getSanitizedParameterId(const String& id);

	~HardcodedSwappableEffect() override;

	// ===================================================================================== Complex Data API calls

	int getNumDataObjects(ExternalData::DataType t) const override;
	Table* getTable(int index) override { return getOrCreate<Table>(tables, index); }
	SliderPackData* getSliderPack(int index) override { return getOrCreate<SliderPackData>(sliderPacks, index); }
	MultiChannelAudioBuffer* getAudioFile(int index) override { return getOrCreate<MultiChannelAudioBuffer>(audioFiles, index); }
	FilterDataObject* getFilterData(int index) override { return getOrCreate<FilterDataObject>(filterData, index);; }
	SimpleRingBuffer* getDisplayBuffer(int index) override { return getOrCreate<SimpleRingBuffer>(displayBuffers, index); }

	LambdaBroadcaster<String> errorBroadcaster;

#if USE_BACKEND
	static void onDllReload(HardcodedSwappableEffect& fx, const std::pair<scriptnode::dll::ProjectDll*, scriptnode::dll::ProjectDll*>& update);
#endif

	// ===================================================================================== Custom hardcoded API calls

	virtual const ModulatorChain::Collection* getModulationChains() const { return nullptr; }
	virtual ModulatorChain::Collection* getModulationChains() { return nullptr; }

	Processor* getCurrentEffect() { return dynamic_cast<Processor*>(this); }
	const Processor* getCurrentEffect() const override { return dynamic_cast<const Processor*>(this); }

	StringArray getModuleList() const override;
	bool setEffect(const String& factoryId, bool /*unused*/) override;
	bool swap(HotswappableProcessor* other) override;
	bool isPolyphonic() const { return polyHandler.isEnabled(); }

	virtual StringArray getIllegalParameterIds() const;

	virtual int getParameterOffset() const { return 0; }

    String getCurrentEffectId() const override { return currentEffect; }
    
	Processor& asProcessor() { return *dynamic_cast<Processor*>(this); }
	const Processor& asProcessor() const { return *dynamic_cast<const Processor*>(this); }

	// ===================================================================================== Processor API tool functions

	/** Call this in every derived destructor. */
	void shutdown()
	{
		mc_->removeTempoListener(&tempoSyncer);
		shutdownCalled = true;
		effectUpdater.shutdown();

		listeners.clear();
		disconnectRuntimeTargets(&asProcessor());

		if(opaqueNode != nullptr)
		{
			factory->deinitOpaqueNode(opaqueNode);
			opaqueNode = nullptr;
		}

		tables.clear();
		audioFiles.clear();
		filterData.clear();
		sliderPacks.clear();
		modProperties.reset();
	}

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

	virtual int getExtraModulationIndex(int modulationSlotIndexWithoutOffset) const
	{
		return modulationSlotIndexWithoutOffset;
	}

	void setupChannelData(float** data, AudioSampleBuffer& b, int startSample)
	{
		for (int i = 0; i < numChannelsToRender; i++)
		{
			data[i] = b.getWritePointer(channelIndexes[i], startSample);
		}
	}

    void disconnectRuntimeTargets(Processor* p) override;
    void connectRuntimeTargets(Processor* p) override;

#if USE_BACKEND
	void preallocateUnloadedParameters(Array<Identifier> unloadedParameters_);
#endif

	ModulatorChain::ExtraModulatorRuntimeTargetSource::ParameterInitData getParameterInitData(int pIndex);

protected:

	ModulatorChain* getPitchChain()
	{
		if(auto synth = dynamic_cast<ModulatorSynth*>(&asProcessor()))
		{
			auto mc = synth->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation);
			return dynamic_cast<ModulatorChain*>(mc);
		}

		auto pp = asProcessor().getParentProcessor(true);

		if(pp != nullptr)
		{
			auto mc = pp->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation);
			return dynamic_cast<ModulatorChain*>(mc);
		}

		return nullptr;
	}

	bool hasLoadedButUncompiledEffect() const;

	HardcodedSwappableEffect(MainController* mc, bool isPolyphonic);

	ModulatorChain::ModChainWithBuffer* getModulationChainForParameter(int parameterIndex) const;

	struct DataWithListener : public ComplexDataUIUpdaterBase::EventListener
	{
		DataWithListener(HardcodedSwappableEffect& parent, ComplexDataUIBase* p, int index_, OpaqueNode* nodeToInitialise);;
		~DataWithListener() override;

		void updateData();
		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var newValue) override;

		OpaqueNode* node;
		const int index = 0;
		ComplexDataUIBase::Ptr data;
	};

	float* getParameterPtr(int index) const;
	virtual Result prepareOpaqueNode(OpaqueNode* n);

	template <typename T> T* getOrCreate(ReferenceCountedArray<T>& list, int index)
	{
		if (isPositiveAndBelow(index, list.size()))
			return list[index].get();

		auto t = createAndInit(ExternalData::getDataTypeForClass<T>());
		list.add(dynamic_cast<T*>(t));
		return list.getLast().get();
	}


	OwnedArray<DataWithListener> listeners;
	LambdaBroadcaster<String, int, bool> effectUpdater;
	
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
	ReferenceCountedArray<SimpleRingBuffer> displayBuffers;
	ReferenceCountedArray<FilterDataObject> filterData;

#if USE_BACKEND
	ValueTree previouslySavedTree;
#endif

	String currentEffect = "No network";

	bool shutdownCalled = false;

	int numParameters = 0;
	ObjectStorage<sizeof(float) * OpaqueNode::NumMaxParameters, 8> lastParameters;
	
	snex::Types::ModValue modValue;
	snex::Types::DllBoundaryTempoSyncer tempoSyncer;
	scriptnode::PolyHandler polyHandler;
	mutable SimpleReadWriteLock lock;
	ScopedPointer<scriptnode::OpaqueNode> opaqueNode;
	ScopedPointer<scriptnode::dll::FactoryBase> factory;

	bool channelCountMatches = false;
	int channelIndexes[NUM_MAX_CHANNELS];
	int numChannelsToRender = 0;
	int hash = -1;

    Array<scriptnode::InvertableParameterRange> parameterRanges;
	Array<Identifier> unloadedParameters;

	OpaqueNode::ModulationProperties modProperties;

private:

	bool disconnected = false;
	MainController* mc_;
	friend class HardcodedMasterEditor;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HardcodedSwappableEffect);
};

}