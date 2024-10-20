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

template <class ModulatorType> class GlobalModulatorDataBase
{
public:

	GlobalModulatorDataBase(Modulator* mod_):
		mod(mod_)
	{}

	bool operator==(const GlobalModulatorDataBase& other) const
	{
		return mod == other.mod;
	}

	ModulatorType* getModulator()
	{
		if (mod.get() == nullptr)
			return nullptr;

		return static_cast<ModulatorType*>(mod.get());
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

	void saveValues(const float* data, int startSample, int numSamples)
	{
		jassertfalse;
		auto dest = savedValuesForBlock.getWritePointer(0, startSample);
		FloatVectorOperations::copy(dest, data + startSample, numSamples);
		isClear = false;
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
		return wp;
	}

	void clear()
	{
		if (!isClear)
		{
			FloatVectorOperations::fill(savedValuesForBlock.getWritePointer(0, 0), 1.0f, savedValuesForBlock.getNumSamples());
			isClear = true;
		}
	}

private:

	AudioSampleBuffer savedValuesForBlock;
	bool isClear = false;
};

class EnvelopeData : public GlobalModulatorDataBase<EnvelopeModulator>
{
public:

	EnvelopeData(Modulator* mod, int samplesPerBlock) :
		GlobalModulatorDataBase(mod),
		savedValuesForBlock(NUM_POLYPHONIC_VOICES, 0)
	{
		prepareToPlay(samplesPerBlock);
	}

	void prepareToPlay(int samplesPerBlock)
	{
		ProcessorHelpers::increaseBufferIfNeeded(savedValuesForBlock, samplesPerBlock);
	}

	const float* getReadPointer(int voiceIndex, int startSample) const
	{
		return savedValuesForBlock.getReadPointer(voiceIndex, startSample);
	}

	void saveValues(int voiceIndex, const float* data, int startSample, int numSamples)
	{
		auto dest = savedValuesForBlock.getWritePointer(voiceIndex, startSample);
		FloatVectorOperations::copy(dest, data + startSample, numSamples);
		isClear = false;
	}

	void clear(int voiceIndex)
	{
		if (!isClear)
		{
			FloatVectorOperations::fill(savedValuesForBlock.getWritePointer(voiceIndex, 0), 1.0f, savedValuesForBlock.getNumSamples());
			isClear = true;
		}
	}

private:

	AudioSampleBuffer savedValuesForBlock;
	bool isClear = false;
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

	void restoreParameterConnections(const ValueTree& v)
	{
		connectedParameters.clear();

		for (const auto& c : v)
		{
			auto p = new ParameterConnection(nullptr, -1, {});
			p->restoreFromValueTree(c);
			connectedParameters.add(p);
		}
	}

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

	float getVoiceStartValueFor(const Processor *voiceStartModulator);

    int getNumActiveVoices() const override { return 0; };
    
	GlobalModulatorContainer(MainController *mc, const String &id, int numVoices);;

	~GlobalModulatorContainer();

	void processorChanged(EventType /*t*/, Processor* /*p*/) override { refreshList(); }

	void restoreFromValueTree(const ValueTree &v) override;

	const float* getEnvelopeValuesForModulator(Processor* p, int startIndex, int voiceIndex);

	const float *getModulationValuesForModulator(Processor *p, int startIndex);
	float getConstantVoiceValue(Processor *p, int noteNumber);

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	

	//void changeListenerCallback(SafeChangeBroadcaster *) { refreshList(); }

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
    
    
private:

    struct GlobalModulatorCable;

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
