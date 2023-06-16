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

#ifndef MODULATORSYNTHCHAIN_H_INCLUDED
#define MODULATORSYNTHCHAIN_H_INCLUDED

namespace hise { using namespace juce;

/** This constrainer removes all Modulators that depend on midi input.
*
*	Modulators on ModulatorSynthChains don't get the Midi data, so it's no use for them to reside there.
*/
class NoMidiInputConstrainer: public FactoryType::Constrainer
{
public:

	NoMidiInputConstrainer();

	String getDescription() const override { return "No voice modulators"; }

	bool allowType(const Identifier &typeName) override
	{
		for(int i = 0; i < forbiddenModulators.size(); i++)
		{
			if(forbiddenModulators[i].type == typeName) return false;
		}

		return true;
	}

private:

	Array<FactoryType::ProcessorEntry> forbiddenModulators;
};

class SynthGroupConstrainer : public FactoryType::Constrainer
{
public:

	SynthGroupConstrainer();

	String getDescription() const override { return "No container modules"; }

	bool allowType(const Identifier &typeName) override
	{
		for (int i = 0; i < forbiddenModulators.size(); i++)
		{
			if (forbiddenModulators[i].type == typeName) return false;
		}

		return true;
	}

private:

	Array<FactoryType::ProcessorEntry> forbiddenModulators;
};

template <int NV> class VoiceBitMap
{
	using DataType = uint32;

	constexpr static int getElementSize() { return sizeof(DataType) * 8; }
	constexpr static int getNumElements() { return NV / getElementSize(); };
	constexpr static int getMaxValue() 
	{
		if constexpr (std::is_same<DataType, uint8>())			 return 0xFF;
		else if constexpr (std::is_same<DataType, uint16>())	 return 0xFFFF;
		else if constexpr (std::is_same<DataType, uint32>())	 return 0xFFFFFFFF;
		else /*if constexpr (std::is_same<DataType, uint64>())*/ return 0xFFFFFFFFFFFFFFFF;
	}

public:

	VoiceBitMap()
	{
		clear();
	}

	void clear()
	{
		memset(data.begin(), 0, sizeof(data));
	}

	void setBit(int voiceIndex, bool value)
	{
		auto dIndex = voiceIndex / getElementSize();
		auto bIndex = voiceIndex % getElementSize();

		if (value)
		{
			auto mask = 1 << bIndex;
			data[dIndex] |= mask;
		}
		else
		{
			auto mask = 1 << bIndex;
			data[dIndex] &= ~mask;
		}
	}

	int getFirstFreeBit() const
	{
		for (int i = 0; i < getNumElements(); i++)
		{
			if (data[i] != getMaxValue())
			{
				for (int j = 0; j < getElementSize(); j++)
				{
					DataType mask = 1 << j;

					if ((data[i] & mask) == 0)
						return i * getElementSize() + j;
				}
			}
		}
		
		return -1;
	}

	VoiceBitMap<NV>& operator|=(const VoiceBitMap<NV>& other)
	{
		for (int i = 0; i < getNumElements(); i++)
			data[i] |= other.data[i];

		return *this;
	}

private:

	static constexpr int NumElements = getNumElements();

	snex::span<DataType, NumElements> data;
};

/** The uniform voice handler will unify the voice indexes of a container so that all sound generators will use the
	same voice index (derived by the event ID of the HiseEvent that started the voice).
	
	By default you can't make assumptions about voice indexes outside of the sound generator because the note might be killed
	earlier for shorter sounds and a restarted voice might have a different voice index. This class manages the voice index
	in a way that guarantees that all voices of child sound generators that are started by the same HiseEvent have the same voice index.

	The obvious advantage of this is that allows you to reuse polyphonic modulation signals, which was a no-go before, but that comes
	with some performance impact at the voice start (because of the logic that has to determine which voice index can be used by all child synths),
	so only enable this when you need to.

	Also you must not change the MIDI events within the container you're using this (so all sound generators inside the synth are supposed to start their sound),
	so MIDI processing (eg. Arpeggiators etc) should not be used inside this (you can of course use an arpeggiator outside the container you're calling this on).
*/
struct UniformVoiceHandler
{
	

	UniformVoiceHandler(ModulatorSynth* parent_) : parent(parent_) { rebuildChildSynthList(); }

	~UniformVoiceHandler()
	{
		childSynths.clear();
		parent = nullptr;
	}

	static UniformVoiceHandler* findFromParent(Processor* p);

	/** This is called in the prepareToPlay function and makes sure that all. */
	void rebuildChildSynthList();

	/** This will ask all child synths which voice index it should use for that event, then store it. */
	void processEventBuffer(const HiseEventBuffer& eventBuffer);

	/** This will return the uniform voice index that is used for the given event. */
	int getVoiceIndex(const HiseEvent& e);

	void incVoiceCounter(ModulatorSynth* s, int voiceIndex);
	void decVoiceCounter(ModulatorSynth* s, int voiceIndex);

	void cleanupAfterProcessing();

private:

	hise::SimpleReadWriteLock arrayLock;

	snex::span<std::tuple<HiseEvent, uint8>, NUM_POLYPHONIC_VOICES> currentEvents;

	WeakReference<ModulatorSynth> parent;
	Array<std::tuple<WeakReference<ModulatorSynth>, VoiceBitMap<NUM_POLYPHONIC_VOICES>>> childSynths;

	JUCE_DECLARE_WEAK_REFERENCEABLE(UniformVoiceHandler);
};

/** A ModulatorSynthChain processes multiple independent ModulatorSynth instances serially.
	@ingroup synthTypes
*
*	This class is supposed to be a wrapper for ModulatorSynths which are processed individually with their own MidiProcessors, Modulators and Effects.
*	You can add some MidiProcessors which will be applied to all chains as well as non polyphonic gain Modulators (like a LfoModulator) which will be applied to the
*	sum of the chain. However, midi messages are not recognized by those modulators (because they are not evaluated on the ModulatorSynthChain level), so don't expect too much.
*
*	If you want to create a group of ModulatorSynths that share common Modulators / MidiProcessors, use a ModulatorSynthGroup instead.
*
*	A ModulatorSynthChain also allows macro controls which can control any Parameter of every sub processor.
*	
*/
class ModulatorSynthChain: public ModulatorSynth,
						   public MacroControlBroadcaster,
						   public Chain
{
public:

	SET_PROCESSOR_NAME("SynthChain", "Container", "A container for other Sound generators.");

	enum EditorStates
	{
		InterfaceShown = ModulatorSynth::EditorState::numEditorStates

	};

	ModulatorSynthChain(MainController *mc, const String &id, int numVoices_);;
	~ModulatorSynthChain();

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	Chain::Handler *getHandler() override { return &handler; };
	const Chain::Handler *getHandler() const override {return &handler;};

	FactoryType *getFactoryType() const override {return modulatorSynthFactory;};
	void setFactoryType(FactoryType *newFactoryType) override {modulatorSynthFactory = newFactoryType;};

	/** returns the total amount of child groups (internal chains + all child synths) */
	int getNumChildProcessors() const override { return ModulatorSynth::getNumChildProcessors() + handler.getNumProcessors(); };

	/** returns the total amount of child groups (internal chains + all child synths) */
	const Processor *getChildProcessor(int processorIndex) const override;;

	Processor *getChildProcessor(int processorIndex) override;;

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay (double newSampleRate, int samplesPerBlock) override;;


	void numSourceChannelsChanged() override;

	void numDestinationChannelsChanged() override;

	ValueTree exportAsValueTree() const override;

	void addProcessorsWhenEmpty() override;;

	Processor *getParentProcessor() {return nullptr;};

	const Processor *getParentProcessor() const {return nullptr;};

	/** Restores the Processor. Don't forget to call compileAllScripts() !. */
	void restoreFromValueTree(const ValueTree &v) override;
    
    void reset();

	void setPackageName(const String &newPackageName) { packageName = newPackageName; }

	String getPackageName() const { return packageName; };
	
	/** Compiles all scripts in this chain. 
	*
	*	If a ScriptProcessor is restored, the script will not be compiled immediately, because it could rely on other Processors that are created afterwards.
	*	Call this function after every processor is created (normally after the restoreFromValueTree function).
	*/
	void compileAllScripts();

    bool hasDefinedFrontInterface() const;
    
    /** This renders the child synths:
	*
	*	- processes the MidiBuffer of the ModulatorSynthChain
	*	- calls the renderNextBlockWithModulators on the child synths
	*	- applies the time-variant gain modulators (no midi support!)
	*	- applies the gain of the chain.
	*/
	void renderNextBlockWithModulators(AudioSampleBuffer &buffer, const HiseEventBuffer &inputMidiBuffer) override;;

	int getVoiceAmount() const {return numVoices;};

	int getNumActiveVoices() const override;

	void killAllVoices() override;
	
	void resetAllVoices() override;

	bool areVoicesActive() const override;

	/** Handles the ModulatorSynthChain. */
	class ModulatorSynthChainHandler: public Chain::Handler
	{
	public:
		ModulatorSynthChainHandler(ModulatorSynthChain *synthToHandle):
			synth(synthToHandle)
		{

		};

		/** Adds a new processor to the chain. It must be owned by the chain. */
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;

		/** Deletes a processor from the chain. */
		virtual void remove(Processor *processorToBeRemoved, bool removeSynth=true) override;;

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex) override;;

		/** Returns the processor at the index. */
		virtual const Processor *getProcessor(int processorIndex) const override;;

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const override;;

		void clear();

	private:

		ModulatorSynthChain *synth;

	};

	void saveInterfaceValues(ValueTree &v);

	void restoreInterfaceValues(const ValueTree &v);

	void setActiveChannels(const HiseEvent::ChannelFilterData& newActiveChannels)
	{
		activeChannels = newActiveChannels;
	}

	HiseEvent::ChannelFilterData* getActiveChannelData() { return &activeChannels; }

	void setUseUniformVoiceHandler(bool shouldUseVoiceHandler);

	bool isUsingUniformVoiceHandler() const;

	UniformVoiceHandler* getUniformVoiceHandler() const { return uniformVoiceHandler.get(); }

private:

	ScopedPointer<UniformVoiceHandler> uniformVoiceHandler;

	HiseEvent::ChannelFilterData activeChannels;
	ModulatorSynthChainHandler handler;
	int numVoices;
	float vuValue;
	OwnedArray<ModulatorSynth> synths;
	ScopedPointer<FactoryType> modulatorSynthFactory;
	ScopedPointer<FactoryType::Constrainer> constrainer;
	String packageName;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorSynthChain);
};

} // namespace hise

#endif  // MODULATORSYNTHCHAIN_H_INCLUDED
