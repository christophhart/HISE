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

#ifndef GLOBALMODULATORCONTAINER_H_INCLUDED
#define GLOBALMODULATORCONTAINER_H_INCLUDED
namespace hise { using namespace juce;

class GlobalModulatorContainerSound : public ModulatorSynthSound
{
public:
	GlobalModulatorContainerSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class GlobalModulatorContainerVoice : public ModulatorSynthVoice
{
public:

	GlobalModulatorContainerVoice(ModulatorSynth *ownerSynth) :
		ModulatorSynthVoice(ownerSynth)
	{};

	bool canPlaySound(SynthesiserSound *) override { return true; };

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;;

	void checkRelease() override;

};

template <class ModulatorType> class GlobalModulatorDataBase: public scriptnode::modulation::Host
{
public:

	using RuntimeData = scriptnode::modulation::EventData;
	
	GlobalModulatorDataBase(Modulator* mod_):
		mod(mod_)
	{}

	bool operator==(const GlobalModulatorDataBase& other) const
	{
		return mod == other.mod;
	}

	ModulatorType* getModulator() const
	{
		if (mod.get() == nullptr)
			return nullptr;

		return static_cast<ModulatorType*>(mod.get());
	}

	int getBlockSizeForCurrentBlock() const
	{
		auto bf = mod->getMainController()->getBufferSizeForCurrentBlock();
		jassert(bf % HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR == 0);
		return bf / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	}

private:

	WeakReference<Modulator> mod;

};

class VoiceStartData : public GlobalModulatorDataBase<VoiceStartModulator>
{
public:

	VoiceStartData(Modulator* mod):
		GlobalModulatorDataBase(mod)
	{
		FloatVectorOperations::clear(voiceValues, 128);
	}

	void saveValue(int noteNumber, int voiceIndex)
	{
		if (auto m = getModulator())
		{
			if (isPositiveAndBelow(noteNumber, 128))
			{
				voiceValues[noteNumber] = m->getVoiceStartValue(voiceIndex);
			}
		}
	}

	static RuntimeData getModulationSignalStatic(Host* obj, const HiseEvent& e, bool)
	{
		if(e.isEmpty())
			return {};

		RuntimeData d;
		d.type = scriptnode::modulation::SourceType::VoiceStart;
		auto typed = static_cast<VoiceStartData*>(obj);
		d.constantValue = typed->voiceValues[e.getNoteNumberIncludingTransposeAmount()];

		return d;
	}

	float getConstantVoiceValue(int noteNumber) const
	{
        if(auto m = const_cast<VoiceStartData*>(this)->getModulator())
        {
            auto unsavedValue = m->getUnsavedValue();
            
            if(unsavedValue != -1.0f)
                return unsavedValue;
        }
        
		if (isPositiveAndBelow(noteNumber, 128))
		{
			return voiceValues[noteNumber];
		}
		
		return 1.0f;
	}

	float voiceValues[128];
};

class TimeVariantData : public GlobalModulatorDataBase<TimeVariantModulator>
{
public:

	TimeVariantData(Modulator* mod, int samplesPerBlock) :
		GlobalModulatorDataBase(mod),
		savedValuesForBlock(1, 0)
	{
		prepareToPlay(samplesPerBlock);
	}

	void prepareToPlay(int samplesPerBlock)
	{
		ProcessorHelpers::increaseBufferIfNeeded(savedValuesForBlock, samplesPerBlock);
	}

	static RuntimeData getModulationSignalStatic(Host* obj, const HiseEvent&, bool)
	{
		auto typed = static_cast<TimeVariantData*>(obj);

		RuntimeData d;
		d.type = scriptnode::modulation::SourceType::TimeVariant;
		d.thisBlockSize = &typed->thisBlockSize;
		d.resetFlag = nullptr;
		d.signal = (float*)typed->getReadPointer(0);
		d.constantValue = 0.0f;
		return d;
	}

	void saveValues(const float* data, int startSample, int numSamples)
	{
		jassertfalse;
		auto dest = savedValuesForBlock.getWritePointer(0, startSample);
		FloatVectorOperations::copy(dest, data + startSample, numSamples);
		isClear = false;
		thisBlockSize = startSample + numSamples;
	}

	const float* getReadPointer(int startSample) const
	{
        if(savedValuesForBlock.getNumSamples() == 0)
            return nullptr;
        
		return savedValuesForBlock.getReadPointer(0, startSample);
	}

	float* initialiseBuffer(int startSample, int numSamples)
	{
		auto wp = savedValuesForBlock.getWritePointer(0, 0);
		FloatVectorOperations::fill(wp + startSample, 1.0f, numSamples);
		isClear = false;
		thisBlockSize = startSample + numSamples;
		return wp;
	}

	void clear()
	{
		if (!isClear)
		{
			FloatVectorOperations::fill(savedValuesForBlock.getWritePointer(0, 0), 1.0f, savedValuesForBlock.getNumSamples());
			isClear = true;
			thisBlockSize = 0;
		}
	}

private:

	AudioSampleBuffer savedValuesForBlock;
	int thisBlockSize = 0;
	bool isClear = false;
};

class EnvelopeData : public GlobalModulatorDataBase<EnvelopeModulator>
{
public:

	using ClearState = scriptnode::modulation::ClearState;

	EnvelopeData(Modulator* mod, int samplesPerBlock) :
		GlobalModulatorDataBase(mod),
		savedValuesForBlock(NUM_POLYPHONIC_VOICES, 0),
	    emptyBlock(1, 0),
		monophonicallyReducedSignal(1, 0)
	{
		for(int i = 0; i < NUM_POLYPHONIC_VOICES; i++)
		{
			isClear[i] = ClearState::Reset;
			thisBlockSize[i] = 0;
		}

		prepareToPlay(samplesPerBlock);
	}

	void prepareToPlay(int samplesPerBlock)
	{
		ProcessorHelpers::increaseBufferIfNeeded(savedValuesForBlock, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(emptyBlock, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(monophonicallyReducedSignal, samplesPerBlock);
		monoBlockSize = monophonicallyReducedSignal.getNumSamples() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	}

	bool isPlaying(const HiseEvent& voiceEvent) const
	{
		auto idx = getIndexForEvent(voiceEvent);

		auto& flag = isClear[idx];

		auto clear = idx == -1 || (flag == ClearState::Reset);
		return !clear;
	}

	const float* getReadPointer(const HiseEvent& voiceEvent, int startSample) const
	{
		auto voiceIndex = getIndexForEvent(voiceEvent);

		if(voiceIndex == -1 || (isClear[voiceIndex] == ClearState::Reset))
			return nullptr;

		return savedValuesForBlock.getReadPointer(voiceIndex, startSample);
	}

	const float* getMonoReadBuffer() const
	{
		if(useMonoBufferAsSource)
			return savedValuesForBlock.getReadPointer(0);
		else
			return monophonicallyReducedSignal.getReadPointer(0);
	}

	void updateThisBufferSize()
	{
		monoBlockSize = getBlockSizeForCurrentBlock();
	}

	float* initialiseMonophonicBuffer(int startSample, int numSamples)
	{
		useMonoBufferAsSource = true;
		auto wp = savedValuesForBlock.getWritePointer(0, 0);
		FloatVectorOperations::fill(wp + startSample, 1.0f, numSamples);
		isClear[0] = ClearState::Playing;

		
		auto tp = getBlockSizeForCurrentBlock();
		thisBlockSize[0] = tp;//startSample + numSamples;
		monoBlockSize = tp;
		return wp;
	}

	std::pair<const int*, const float*> getRuntimePointer(const HiseEvent& e, int startSample) const
	{
		auto voiceIndex = getIndexForEvent(e);

		if(voiceIndex == -1 || (isClear[voiceIndex] == ClearState::Reset))
			return { nullptr, nullptr };


		return { thisBlockSize + voiceIndex, savedValuesForBlock.getReadPointer(voiceIndex, startSample) };
	}

	static RuntimeData getModulationSignalStatic(Host* obj, const HiseEvent& e, bool wantsPolyphonicSignal)
	{
		auto typed = static_cast<EnvelopeData*>(obj);

		RuntimeData d;
		d.type = scriptnode::modulation::SourceType::Envelope;
		d.constantValue = 0.0f;

		if(wantsPolyphonicSignal)
		{
			auto rt = typed->getRuntimePointer(e, 0);
			d.thisBlockSize = rt.first;
			d.signal = rt.second;
			d.resetFlag = typed->isClear;
		}
		else
		{
			d.signal = typed->getMonoReadBuffer();
			d.thisBlockSize = &typed->monoBlockSize;
			d.resetFlag = nullptr;
		}
	
		return d;
	}

	void saveValues(const HiseEvent& voiceEvent, const float* data, int startSample, int numSamples)
	{
		auto voiceIndex = getIndexForEvent(voiceEvent);

		if(voiceIndex == -1)
			return;

		auto& flag = isClear[voiceIndex];

		if(flag != ClearState::Reset)
		{
			thisBlockSize[voiceIndex] = startSample + numSamples;
			auto dest = savedValuesForBlock.getWritePointer(voiceIndex, startSample);
			FloatVectorOperations::copy(dest, data + startSample, numSamples);

			useMonoBufferAsSource = false;
			
			auto mdest = monophonicallyReducedSignal.getWritePointer(0, startSample);

			FloatVectorOperations::copy(mdest, data + startSample, numSamples);
		}
	}

	void clear(const HiseEvent& voiceEvent)
	{
		auto voiceIndex = getIndexForEvent(voiceEvent);

		if(voiceIndex != -1)
		{
			if(isClear[voiceIndex] == ClearState::Playing)
				isClear[voiceIndex] = ClearState::PendingReset1;
		}
	}

	void startVoice(const HiseEvent& voiceEvent)
	{
		auto voiceIndex = getIndexForEvent(voiceEvent);

		jassert(voiceIndex != -1);

		isClear[voiceIndex] = ClearState::Playing;
	}

	/** Check if clear was called and return true if it's still playing.
	 *
	 *  This will leave two buffers through to avoid cutting off the release trail.
	 */
	bool clearIfPending(const HiseEvent& voiceEvent)
	{
		auto voiceIndex = getIndexForEvent(voiceEvent);

		if(voiceIndex != -1)
		{
			if(isClear[voiceIndex] == ClearState::PendingReset1)
			{
				isClear[voiceIndex] = ClearState::PendingReset2;
				return true;
			}
			else if (isClear[voiceIndex] == ClearState::PendingReset2)
			{
				isClear[voiceIndex] = ClearState::Reset;
				thisBlockSize[voiceIndex] = 0;
				return false;
			}
			else if (isClear[voiceIndex] == ClearState::Reset)
			{
				return false;
			}
		}

		return true;
	}

	

private:

	

	int getIndexForEvent(const HiseEvent& voiceEvent) const
	{
		if(getModulator()->isInMonophonicMode())
			return 0;

		if(voiceEvent.isEmpty())
			return -1;

		return voiceEvent.getEventId() % NUM_POLYPHONIC_VOICES;
	}

	AudioSampleBuffer monophonicallyReducedSignal;
	int monoBlockSize = -1;
	bool useMonoBufferAsSource = false;

	AudioSampleBuffer savedValuesForBlock;
	AudioSampleBuffer emptyBlock;

	int thisBlockSize[NUM_POLYPHONIC_VOICES];

	ClearState isClear[NUM_POLYPHONIC_VOICES];
};

class GlobalModulatorData
{
public:

	GlobalModulatorData(Processor *modulator);

	/** Sets up the buffers depending on the type of the modulator. */
	void prepareToPlay(double sampleRate, int blockSize);

	void saveValuesToBuffer(int startIndex, int numSamples, int voiceIndex = 0, int noteNumber=-1);
	const float *getModulationValues(int startIndex, int voiceIndex = 0) const;
	float getConstantVoiceValue(int noteNumber);

	const Processor *getProcessor() const { return modulator.get(); }

	VoiceStartModulator *getVoiceStartModulator() { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	const VoiceStartModulator *getVoiceStartModulator() const { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	TimeVariantModulator *getTimeVariantModulator() { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	const TimeVariantModulator *getTimeVariantModulator() const { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	EnvelopeModulator *getEnvelopeModulator() { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }
	const EnvelopeModulator *getEnvelopeModulator() const { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }

	GlobalModulator::ModulatorType getType() const noexcept{ return type; }

	void handleVoiceStartControlledParameters(int noteNumber);

	struct ParameterConnection: public MidiControllerAutomationHandler::AutomationData
	{
		ParameterConnection(Processor* p, int parameterIndex_, const NormalisableRange<double>& range_) :
			AutomationData()
		{
			attribute = parameterIndex_;
			processor = p;
			parameterRange = range_;
			fullRange = range_;
		};

		ValueTree exportAsValueTree() const override
		{
			ValueTree av = AutomationData::exportAsValueTree();

			av.removeProperty("Controller", nullptr);

			ValueTree v("ParameterConnection");

			v.copyPropertiesFrom(av, nullptr);

			return v;
		}

		void restoreFromValueTree(const ValueTree &v) override
		{
			AutomationData::restoreFromValueTree(v);
		}

		float lastValue = 0.0f;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(ParameterConnection);
	};

	ParameterConnection* getParameterConnection(const Processor* p, int parameterIndex)
	{
		for (auto pc : connectedParameters)
		{
			if (pc->attribute == parameterIndex && pc->processor == p)
				return pc;
		}

		return nullptr;
	}

	ParameterConnection* addConnectedParameter(Processor* p, int parameterIndex, NormalisableRange<double> normalisableRange)
	{
		auto newConnection = new ParameterConnection(p, parameterIndex, normalisableRange);
		connectedParameters.add(newConnection);
		return connectedParameters.getLast();
	}

	void removeConnectedParameter(Processor* p, int parameterIndex)
	{
		for (auto c : connectedParameters)
		{
			if (c->processor.get() == p && c->attribute == parameterIndex)
			{
				connectedParameters.removeObject(c, true);
				return;
			}
		}
	}

	ValueTree exportAllConnectedParameters() const
	{
		if (connectedParameters.size() == 0)
			return {};

		auto pId = getProcessor()->getId();

		ValueTree v("Modulator");
		v.setProperty("id", pId, nullptr);

		for (auto pc : connectedParameters)
		{
			auto child = pc->exportAsValueTree();
			v.addChild(child, -1, nullptr);
		}

		return v;
	}

	void restoreParameterConnections(const ValueTree& v);

	void handleTimeVariantControlledParameters(int startSample, int numThisTime) const;
private:

	OwnedArray<ParameterConnection> connectedParameters;
	
	WeakReference<Processor> modulator;
	GlobalModulator::ModulatorType type;

	int numVoices;
	AudioSampleBuffer valuesForCurrentBuffer;
	Array<float> constantVoiceValues;
};

/** A container that processes Modulator instances that can be used at different locations.
	@ingroup synthTypes
*/
class GlobalModulatorContainer : public ModulatorSynth,
								 public Chain::Handler::Listener
{
public:

	struct ModulatorListListener
	{
        virtual ~ModulatorListListener() {};
        
		virtual void listWasChanged() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	SET_PROCESSOR_NAME("GlobalModulatorContainer", "Global Modulator Container", "A container that processes Modulator instances that can be used at different locations.");

	static ProcessorMetadata createMetadata()
	{
		auto md = ModulatorSynth::createBaseMetadata()
			.withStandardMetadata<GlobalModulatorContainer>()
			.withDescription("A container that processes Modulator instances that can be used at different locations.")
			.withDisabledChain(ModulatorSynth::BasicChains::PitchChain)
			.withDisabledFX();

		// rewrite the gain mod chain
		md.modulation.set(ModulatorSynth::BasicChains::GainChain,
			ProcessorMetadata::ModulationMetadata(ModulatorSynth::InternalChains::GainModulation)
			.withId("Global Modulators")
			.withDescription("The modulation sources that can be picked up using GlobalModulators")
			.withConstrainer<NoGlobalsConstrainer>());

		return md;
	}

	float getVoiceStartValueFor(const Processor *voiceStartModulator);
    int getNumActiveVoices() const override { return 0; };
    
	GlobalModulatorContainer(MainController *mc, const String &id, int numVoices);;
	~GlobalModulatorContainer();

	void processorChanged(EventType /*t*/, Processor* /*p*/) override { refreshList(); }

	ValueTree exportAsValueTree() const override
	{
		auto v = ModulatorSynth::exportAsValueTree();
		v.addChild(runtimeSource.matrixData.createCopy(), -1, nullptr);
		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override;

	int getEnvelopeIndex(Processor* p) const;

	bool isEnvelopePlaying(int envelopeIndex, const HiseEvent& voiceEvent) const
	{
		jassert(isPositiveAndBelow(envelopeIndex, envelopeData.size()));

		return envelopeData.getReference(envelopeIndex).isPlaying(voiceEvent);
	}

	const float* getEnvelopeValuesForModulator(int envelopeIndex, int startIndex, const HiseEvent& voiceEvent);

	const float *getModulationValuesForModulator(Processor *p, int startIndex);
	float getConstantVoiceValue(Processor *p, int noteNumber);

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void preStartVoice(int voiceIndex, const HiseEvent& e) final override;

	void preVoiceRendering(int startSample, int numThisTime) override;

	void addProcessorsWhenEmpty() override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void addModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex, NormalisableRange<double> range, int macroIndex);
	void removeModulatorControlledParameter(const Processor* modulationSource, Processor* processor, int parameterIndex);
	bool isModulatorControlledParameter(Processor* processor, int parameterIndex) const;
	
	const Processor* getModulatorForControlledParameter(const Processor* processor, int parameterIndex) const;

	ValueTree exportModulatedParameters() const;

	void restoreModulatedParameters(const ValueTree& v);
	bool synthNeedsEnvelope() const override { return false; }
    void connectToGlobalCable(Modulator* childMod, var cable, bool addToMod);
	bool shouldReset(int voiceIndex);
	void renderEnvelopeData(int voiceIndex, int startSample, int numSamples);
    void sendVoiceStartCableValue(Modulator* m, const HiseEvent& e);

	ValueTree getMatrixModulatorData() const { return runtimeSource.matrixData; }

	MatrixIds::Helpers::Properties matrixProperties;

	var getMatrixModulationProperties() const { return matrixProperties.toJSON(getMainController()); }

	runtime_target::connection getMatrixModulatorConnection() const { return runtimeSource.createConnection(); }

	void handleRetriggeredNote(ModulatorSynthVoice *voice) override
	{
		voice->checkRelease();
		// do nothing here, it should not kill the old voice...
	}

	void connectToRuntimeTargets(scriptnode::OpaqueNode& on, bool shouldAdd);

	MacroControlledObject::ModulationPopupData::Ptr createMatrixModulationPopupData(Processor* p, int parameterIndex);

	MacroControlledObject::ModulationPopupData::Ptr createMatrixModulationPopupData(const String& targetId);

	int getCurrentNumSamples() const { return lastBlockSize;}

	StringArray customEditCallbacks;
	LambdaBroadcaster<int, String> editCallbackHandler;

	bool exclusiveSourceMode = false;
	LambdaBroadcaster<int> currentMatrixSourceBroadcaster;

	enum class DragAction
	{
		DragEnd,
		DragStart,
		Drop,
		Hover,
		DisabledHover,
		numDragActions
	};

	void sendDragMessage(int sourceIndex, const String& targetId, DragAction eventType)
	{
		dragBroadcaster.sendMessage(sendNotificationAsync, sourceIndex, targetId, eventType);
	}

	LambdaBroadcaster<int, String, DragAction> dragBroadcaster;

	void cleanUpRuntimeSources()
	{
		runtimeSource.clear();
	}

	void setExlusiveMatrixSource(int sourceIndex, NotificationType n)
	{
		if (matrixProperties.selectableSources)
		{
			currentMatrixSourceBroadcaster.sendMessage(n, sourceIndex);
		}
	}

	int getExclusiveMatrixSource() const
	{
		if(matrixProperties.selectableSources)
			return currentMatrixSourceBroadcaster.getLastValue();

		return -1;
	}

private:

	int lastBlockSize = 0;

	bool isIdleOrHasAudioLock() const
	{
		return LockHelpers::freeToGo(getMainController()) || LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::AudioLock);
	}

	struct RuntimeSource: public runtime_target::source_base
	{
		RuntimeSource(GlobalModulatorContainer& parent_):
		  parent(parent_),
		  matrixData(MatrixIds::MatrixData)
		{
			deleter.setCallback(matrixData, 
								{ MatrixIds::SourceIndex, MatrixIds::TargetId, MatrixIds::AuxIndex }, 
								valuetree::AsyncMode::Synchronously,
								VT_BIND_RECURSIVE_PROPERTY_LISTENER(onMatrixDisconnect));
		};

		~RuntimeSource()
		{
			matrixData.removeAllChildren(nullptr);
			clear();
		}

		virtual int getRuntimeHash() const
		{
			return 1;
		}

		void onMatrixDisconnect(ValueTree v, const Identifier& id)
		{
			bool shouldDelete = false;

			auto um = parent.getMainController()->getControlUndoManager();

			if(id == MatrixIds::SourceIndex)
				shouldDelete = (int)v[id] == -1;
			if(id == MatrixIds::TargetId)
			{
				auto tid = v[id].toString();
				shouldDelete = tid.isEmpty() || tid == "No connection";
			}
			if(id == MatrixIds::AuxIndex && (int)v[id] == -1)
				v.setProperty(MatrixIds::AuxIntensity, 0.0f, um);

			if(shouldDelete)
				v.getParent().removeChild(v, um);
		}

	    runtime_target::RuntimeTarget getType() const override
	    {
		    return runtime_target::RuntimeTarget::GlobalModulator;
	    }

		runtime_target::connection createConnection() const override
		{
			auto c = source_base::createConnection();
			c.connectFunction = connectStatic<true>;
	        c.disconnectFunction = connectStatic<false>;
	        c.sendBackFunction = nullptr;
			c.sendBackDataFunction = nullptr;
			return c;
		}

		void restore(const ValueTree& v, UndoManager* um);

		void clear();

		void updateTargets()
		{
			// This should only be called when the processing is suspended...
			jassert(parent.isIdleOrHasAudioLock());

			currentSignal.sampleRate_cr = parent.getSampleRate() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
			currentSignal.numSamples_cr = parent.getLargestBlockSize() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

			auto modChain = parent.getChildProcessor(ModulatorSynth::InternalChains::GainModulation);

			for(int i = 0; i < modChain->getNumChildProcessors(); i++)
			{
				auto mod = modChain->getChildProcessor(i);

				if(auto envelope = dynamic_cast<EnvelopeModulator*>(mod))
					currentSignal.modValueFunctions[i] = getModFunction(parent.envelopeData, mod);
				else if (auto tv = dynamic_cast<TimeVariantModulator*>(mod))
					currentSignal.modValueFunctions[i] = getModFunction(parent.timeVariantData, mod);
				else if (auto vs = dynamic_cast<VoiceStartModulator*>(mod))
					currentSignal.modValueFunctions[i] = getModFunction(parent.voiceStartData, mod);
			}

			for(auto t: connectedTargets)
				t->onValue(currentSignal);
		}

		template <typename T> static std::pair<scriptnode::modulation::Host*, scriptnode::modulation::EventData::QueryFunction*> getModFunction(Array<T>& list, Processor* mod)
		{
			for(auto& vsd: list)
			{
				if(vsd.getModulator() == mod)
				{
					auto h = static_cast<scriptnode::modulation::Host*>(&vsd);
					return { h, T::getModulationSignalStatic };
					break;
				}
			}

			jassertfalse;
			return { nullptr, nullptr }; 
		}

		void connectToRuntimeTargets(OpaqueNode& on, bool shouldAdd)
		{
			auto c = createConnection();
			on.connectToRuntimeTarget(shouldAdd, c);

			if(shouldAdd)
				connectedNodes.addIfNotAlreadyThere(&on);
			else
				connectedNodes.removeAllInstancesOf(&on);
		}

		template <bool Add> static bool connectStatic(runtime_target::source_base* obj, runtime_target::target_base* t)
	    {
			auto typed = static_cast<RuntimeSource*>(obj);
			auto tt = dynamic_cast<runtime_target::typed_target<modulation::SignalSource>*>(t);

			// This should only be called when the processing is suspended...
			jassert(typed->parent.isIdleOrHasAudioLock());

			if(Add)
			{
				if(!typed->connectedTargets.contains(tt))
				{
					typed->connectedTargets.insert(tt);
					tt->onValue(typed->currentSignal);
					return true;
				}
			}
			else
			{
				if(typed->connectedTargets.contains(tt))
				{
					typed->connectedTargets.remove(tt);
					return true;
				}
			}

			return false;
	    }

		ValueTree matrixData;

		valuetree::RecursivePropertyListener deleter;

		hise::UnorderedStack<runtime_target::typed_target<modulation::SignalSource>*> connectedTargets;
		Array<OpaqueNode*> connectedNodes;

		modulation::SignalSource currentSignal;
		GlobalModulatorContainer& parent;
	} runtimeSource;

	struct GlobalModulatorCable
	{
		WeakReference<Modulator> mod;
		var cable;

		void send(int voiceIndex, bool isEnvelope = false, int startSample = 0);

		bool operator==(const GlobalModulatorCable& other) const
		{
			return other.mod == mod &&
				cable == other.cable;
		}
	};

    SimpleReadWriteLock cableLock;
    
    Array<GlobalModulatorCable> timeVariantCables;
    Array<GlobalModulatorCable> voiceStartCables;
	Array<GlobalModulatorCable> envelopeCables;
    
	Array<VoiceStartData> voiceStartData;
	Array<TimeVariantData> timeVariantData;
	Array<EnvelopeData> envelopeData;

	Array<WeakReference<ModulatorListListener>> modListeners;

	friend class GlobalModulatorContainerVoice;

	Array<WeakReference<GlobalModulatorData::ParameterConnection>> allParameters;

	void refreshList();

	OwnedArray<GlobalModulatorData> data;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalModulatorContainer);
};


} // namespace hise

#endif  // GLOBALMODULATORCONTAINER_H_INCLUDED
