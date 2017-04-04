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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef MODULATORSYNTHCHAIN_H_INCLUDED
#define MODULATORSYNTHCHAIN_H_INCLUDED

/** This constrainer removes all Modulators that depend on midi input.
*
*	Modulators on ModulatorSynthChains don't get the Midi data, so it's no use for them to reside there.
*/
class NoMidiInputConstrainer: public FactoryType::Constrainer
{
public:

	NoMidiInputConstrainer();

	

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

class ModulatorSynthChain;



/** A ModulatorSynthChain processes multiple independent ModulatorSynth instances.
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
#if USE_BACKEND
						   public ViewManager,
#endif
						   public Chain
{
public:

	SET_PROCESSOR_NAME("SynthChain", "Container");

	enum EditorStates
	{
		InterfaceShown = ModulatorSynth::EditorState::numEditorStates

	};

	ModulatorSynthChain(MainController *mc, const String &id, int numVoices_, UndoManager *viewUndoManager = nullptr) :
		MacroControlBroadcaster(this),
		ModulatorSynth(mc, id, numVoices_),
#if USE_BACKEND
		ViewManager(this, viewUndoManager),
#endif
		numVoices(numVoices_),
		handler(this),
		vuValue(0.0f)
	{
#if USE_BACKEND == 0
		ignoreUnused(viewUndoManager);
#endif

		FactoryType *t = new ModulatorSynthChainFactoryType(numVoices, this);

		getMatrix().setAllowResizing(true);

		setGain(1.0);

		editorStateIdentifiers.add("InterfaceShown");

		setFactoryType(t);

        setEditorState(Processor::EditorState::BodyShown, false);
        
		// Skip the pitch chain
		pitchChain->setBypassed(true);

		//gainChain->getFactoryType()->setConstrainer(new NoMidiInputConstrainer());

		constrainer = new NoMidiInputConstrainer();

		gainChain->getFactoryType()->setConstrainer(constrainer, false);

		effectChain->getFactoryType()->setConstrainer(constrainer, false);

		disableChain(PitchModulation, true);
	};

	virtual ~ModulatorSynthChain()
	{
		getHandler()->clear();

		effectChain = nullptr;
		midiProcessorChain = nullptr;
		gainChain = nullptr;
		pitchChain = nullptr;

		constrainer = nullptr;

	}

	

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	Chain::Handler *getHandler() override { return &handler; };

	const Chain::Handler *getHandler() const override {return &handler;};

	FactoryType *getFactoryType() const override {return modulatorSynthFactory;};

	void setFactoryType(FactoryType *newFactoryType) override {modulatorSynthFactory = newFactoryType;};

	/** returns the total amount of child groups (internal chains + all child synths) */
	int getNumChildProcessors() const override { return ModulatorSynth::getNumChildProcessors() + handler.getNumProcessors(); };

	/** returns the total amount of child groups (internal chains + all child synths) */
	const Processor *getChildProcessor(int processorIndex) const override
	{ 
		if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
		else													return handler.getProcessor(processorIndex - numInternalChains);	
	};

	Processor *getChildProcessor(int processorIndex) override
	{ 
		if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
		else													return handler.getProcessor(processorIndex - numInternalChains);	
	};

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay (double newSampleRate, int samplesPerBlock) override
	{
		ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

		for(int i = 0; i < synths.size(); i++) synths[i]->prepareToPlay(newSampleRate, samplesPerBlock);
	};


	void numSourceChannelsChanged() override;

	void numDestinationChannelsChanged() override;

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

        if(this == getMainController()->getMainSynthChain())
        {
            v.setProperty("packageName", packageName, nullptr);
            
			

#if USE_BACKEND
            ViewManager::saveViewsToValueTree(v);
#endif
            MacroControlBroadcaster::saveMacrosToValueTree(v);

			v.addChild(getMainController()->getMacroManager().getMidiControlAutomationHandler()->exportAsValueTree(), -1, nullptr);

        }
		return v;
	}

	void addProcessorsWhenEmpty() override;;

	Processor *getParentProcessor() {return nullptr;};

	const Processor *getParentProcessor() const {return nullptr;};

	/** Restores the Processor. Don't forget to call compileAllScripts() !. */
	void restoreFromValueTree(const ValueTree &v) override
	{
		packageName = v.getProperty("packageName", "");

		ModulatorSynth::restoreFromValueTree(v);

		
#if USE_BACKEND
		ViewManager::restoreViewsFromValueTree(v);
#endif

		ValueTree autoData = v.getChildWithName("MidiAutomation");

		if (autoData.isValid())
		{
			getMainController()->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);
		}
	}
    
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

	/** Handles the ModulatorSynthChain. */
	class ModulatorSynthChainHandler: public Chain::Handler
	{
	public:
		ModulatorSynthChainHandler(ModulatorSynthChain *synthToHandle):
			synth(synthToHandle)
		{

		};

		/** Adds a new processor to the chain. It must be owned by the chain. */
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override
		{
			ScopedLock sl(synth->getMainController()->getLock());

			ModulatorSynth *ms = dynamic_cast<ModulatorSynth*>(newProcessor);

			jassert(ms != nullptr);

			const int index = siblingToInsertBefore == nullptr ? -1 : synth->synths.indexOf(dynamic_cast<ModulatorSynth*>(siblingToInsertBefore));

			synth->synths.insert(index, ms);
			synth->prepareToPlay(synth->getSampleRate(), synth->getBlockSize());

			ms->getMatrix().setNumDestinationChannels(synth->getMatrix().getNumSourceChannels());
			ms->getMatrix().setTargetProcessor(synth);

			sendChangeMessage();
		}

		/** Deletes a processor from the chain. */
		virtual void remove(Processor *processorToBeRemoved) override
		{
			ScopedLock sl(synth->getMainController()->getLock());

			synth->synths.removeObject(dynamic_cast<ModulatorSynth*>(processorToBeRemoved));

			sendChangeMessage();
		};

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex) override
		{
			return synth->synths[processorIndex];
		};

		/** Returns the processor at the index. */
		virtual const Processor *getProcessor(int processorIndex) const override
		{
			return synth->synths[processorIndex];
		};

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const override
		{
			return synth->synths.size();
		};

		void clear()
		{
			ScopedLock sl(synth->getMainController()->getLock());

			synth->synths.clear();

			sendChangeMessage();
		}

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

private:

	HiseEvent::ChannelFilterData activeChannels;

	ModulatorSynthChainHandler handler;

	int numVoices;

	float vuValue;

	OwnedArray<ModulatorSynth> synths;

	ScopedPointer<FactoryType> modulatorSynthFactory;

	ScopedPointer<FactoryType::Constrainer> constrainer;

	String packageName;
};






class ModulatorSynthGroupSound : public ModulatorSynthSound
{
public:
    ModulatorSynthGroupSound() {}

    bool appliesToNote (int /*midiNoteNumber*/)           { return true; }
    bool appliesToChannel (int /*midiChannel*/)           { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }
};



/** This class acts as wrapper in a ModulatorSynthGroup for all child synth voices. */
class ModulatorSynthGroupVoice: public ModulatorSynthVoice
{
public:

	ModulatorSynthGroupVoice(ModulatorSynth *ownerSynth):
		ModulatorSynthVoice(ownerSynth)
	{
	};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};


	/** This stores a reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void addChildSynth(ModulatorSynth *childSynth)
	{
		ScopedLock sl(ownerSynth->getSynthLock());

		jassert(childSynth->getNumVoices() == ownerSynth->getNumVoices());

		childSynths.add(childSynth);
		childVoices.add(static_cast<ModulatorSynthVoice*>(childSynth->getVoice(voiceIndex)));

		jassert(childSynths.size() == childVoices.size());
	};

	/** This removes reference of the child synths and a reference of the voice of the child processor with the same voice index. */
	void removeChildSynth(ModulatorSynth *childSynth)
	{
		ScopedLock sl(ownerSynth->getSynthLock());

		jassert(childSynth != nullptr);
		jassert(childSynths.indexOf(childSynth) != -1);

		if (childSynth != nullptr)
		{
			childSynths.removeAllInstancesOf(childSynth);
			childVoices.removeAllInstancesOf(static_cast<ModulatorSynthVoice*>(childSynth->getVoice(voiceIndex)));

			jassert(childSynths.size() == childVoices.size());
		}

		
	};

	/** Calls the base class startNote() for the group itself and all child synths.  */
	void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int ) override;

	/** Calls the base class stopNote() for the group itself and all child synths. */
	void stopNote (float , bool) override;

	void checkRelease();

	void calculateBlock(int startSample, int numSamples) override;

    
private:

	Array<ModulatorSynth*> childSynths;
	Array<ModulatorSynthVoice*> childVoices;

	float fmModBuffer[2048];

};





/** A ModulatorSynthGroup is a collection of somehow related ModulatorSynths that are processed together.
*
*	This class is designed to process related ModulatorSynths (eg. round-robin groups or multimiced samples) together. Unlike the ModulatorSynthChain,
*	this structure allows Modulators and effects to control multiple child synths.
*	In order to ensure correct functionality, the ModulatorSynthGroup assumes that all child synths start at the same time.
*
*	The ModulatorSynthGroup is rendered like a normal ModulatorSynth using a special kind of voice (the ModulatorSynthGroupVoice).
*
*	- Group start logic can be implemented here.
*	- Modulators of the ModulatorSynthGroup also control their child processors.
*
*	There are some restrictions for this group type:
*
*	- MidiProcessors are not allowed in the child processors, but you can use specialised MidiProcessors in the ModulatorSynthGroup's MidiEditor.
*	- ModulatorSynthGroups can't be nested. The child processors only collect modulation from their immediate parent.
*
*/
class ModulatorSynthGroup: public ModulatorSynth,
						   public Chain
{
public:

	enum SpecialParameters
	{
		EnableFM = ModulatorSynth::numModulatorSynthParameters,
		CarrierIndex,
		ModulatorIndex,
		numSynthGroupParameters
	};

	SET_PROCESSOR_NAME("SynthGroup", "Syntesizer Group")

	enum InternalChains
	{
		SampleStartModulation = ModulatorSynth::numInternalChains,
		numInternalChains
	};

	
	ModulatorSynthGroup(MainController *mc, const String &id, int numVoices);

	~ModulatorSynthGroup()
	{
		// This must be destroyed before the base class destructor because the MidiProcessor destructors may use some of ModulatorSynthGroup methods...
		midiProcessorChain = nullptr;
	};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	Chain::Handler *getHandler() override { return &handler; };

	const Chain::Handler *getHandler() const override {return &handler;};

	FactoryType *getFactoryType() const override {return modulatorSynthFactory;};

	void setFactoryType(FactoryType *newFactoryType) override {modulatorSynthFactory = newFactoryType;};

	/** returns the total amount of child groups (internal chains + all child synths) */
	int getNumChildProcessors() const override { return numInternalChains + handler.getNumProcessors(); };

	int getNumInternalChains() const override {return numInternalChains; };

	void setInternalAttribute(int index, float newValue) override;

	float getAttribute(int index) const override;

	/** returns the total amount of child groups (internal chains + all child synths) */
	Processor *getChildProcessor(int processorIndex) override
	{ 
		if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
		else if (processorIndex == SampleStartModulation)		return sampleStartChain;
		else													return handler.getProcessor(processorIndex - numInternalChains);	
	};

	/** returns the total amount of child groups (internal chains + all child synths) */
	const Processor *getChildProcessor(int processorIndex) const override
	{ 
		if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
        else if (processorIndex == SampleStartModulation)		return sampleStartChain;
		else													return handler.getProcessor(processorIndex - numInternalChains);	
	};


	/** Iterates over all child synths.
	*
	*	Example usage:
	*
	*		ModulatorSynth *child;
	*		ChildSynthIterator childIterator(this);
	*		
	*		while(childIterator.getNextAllowedChild(child))
	*		{
	*			// Do something here
	*		}
	*/
	class ChildSynthIterator
	{
	public:

		/** This can be set to iterate either all synths or only the allowed ones. */
		enum Mode
		{
			SkipUnallowedSynths = 0,
			IterateAllSynths
		};

		/** Creates a new ChildSynthIterator. It is okay to create one on the stack. You have to pass a pointer to the ModulatorSynthGroup that should be iterated. */
		ChildSynthIterator(ModulatorSynthGroup *groupToBeIterated, Mode iteratorMode=SkipUnallowedSynths):
			limit(groupToBeIterated->getHandler()->getNumProcessors()),
			counter(0),
			group(*groupToBeIterated),
			mode(iteratorMode)
		{

		};

		/** sets the passed pointer to the next ModulatorSynth in the chain that is allowed and returns false if the end of the chain is reached or if the child is a nullpointer */
		bool getNextAllowedChild(ModulatorSynth *&child)
		{
			if( mode == SkipUnallowedSynths)
			{
				counter = group.allowStates.findNextSetBit(counter);
				if(counter == -1) return false;
			}
			
			child = group.synths[counter++];

			if (child == nullptr) return false;

			// This should not happen
			jassert(child != nullptr);

			return child != nullptr && counter <= limit;
		};

	private:

		friend class ModulatorSynthGroup;

		ModulatorSynthGroup &group;
		int counter;
		const int limit;
		const Mode mode;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChildSynthIterator)

	};

	void allowChildSynth(int childSynthIndex, bool shouldBeAllowed)
	{
		allowStates.setBit(childSynthIndex, shouldBeAllowed);
	};

	/** set the state for all groups at once. */
	void setAllowStateForAllChildSynths(bool shouldBeEnabled)
	{
		allowStates.setRange(0, numVoices, shouldBeEnabled);
	};

	Processor *getParentProcessor() {return nullptr;};

	const Processor *getParentProcessor() const {return nullptr;};

	/** Passes the incoming MidiMessage only to the modulation chains of all child synths and NOT to the child synth's voices, as they get rendered by the ModulatorSynthGroupVoices. */
	void preHiseEventCallback(const HiseEvent &m) override;

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay (double newSampleRate, int samplesPerBlock) override
	{
		if (newSampleRate != -1.0)
		{
			modSynthGainValues = AudioSampleBuffer(1, samplesPerBlock);

			ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

			sampleStartChain->prepareToPlay(newSampleRate, samplesPerBlock);

			ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
			ModulatorSynth *childSynth;

			while (iterator.getNextAllowedChild(childSynth))
			{
				childSynth->prepareToPlay(newSampleRate, samplesPerBlock);
			}
		}
		
	};

	/** Clears the internal buffers of the childs and the group itself. */
	void initRenderCallback() override
	{
		ModulatorSynth::initRenderCallback();

		ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
		ModulatorSynth *childSynth;

		while(iterator.getNextAllowedChild(childSynth))
		{
			childSynth->initRenderCallback();
		}
	};

	void preVoiceRendering(int startSample, int numThisTime) override
	{
		ModulatorSynth::preVoiceRendering(startSample, numThisTime);

		ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
		ModulatorSynth *childSynth;

		if (fmCorrectlySetup)
		{
			ModulatorSynth *modSynth = static_cast<ModulatorSynth*>(getChildProcessor(modIndex));
			ModulatorChain *gainChainOfModSynth = static_cast<ModulatorChain*>(modSynth->getChildProcessor(ModulatorSynth::GainModulation));

			gainChainOfModSynth->renderNextBlock(modSynthGainValues, startSample, numThisTime);
		}

		while(iterator.getNextAllowedChild(childSynth))
		{
			childSynth->preVoiceRendering(startSample, numThisTime);
		}
	};

	void postVoiceRendering(int startSample, int numThisTime) override
	{
		ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
		ModulatorSynth *childSynth;

		while(iterator.getNextAllowedChild(childSynth))
		{
			childSynth->postVoiceRendering(startSample, numThisTime);
			
		}

		// Apply the gain after the rendering of the child synths...
		ModulatorSynth::postVoiceRendering(startSample, numThisTime);
	};

	void restoreFromValueTree(const ValueTree &v) override;

	ValueTree exportAsValueTree() const override;


	/** Handles the ModulatorSynthGroup. 
	*
	*	It is almost the same as Modulator, but it calls setGroup() on the ModulatorSynths that are added.
	*/
	class ModulatorSynthGroupHandler: public Chain::Handler
	{
	public:
		ModulatorSynthGroupHandler(ModulatorSynthGroup *synthGroupToHandle):
			group(synthGroupToHandle)
		{

		};

		/** Adds a new ModulatorSynth to the ModulatorSynthGroup. It also stores a reference in each ModulatorSynthGroupVoice.
		*
		*	By default, it sets the synths allow state to 'true'.
		*/
		void add(Processor *newProcessor, Processor *siblingToInsertBefore) override;

		/** Deletes a processor from the chain. It also removes the reference in the ModulatorSynthGroupVoices. */
		virtual void remove(Processor *processorToBeRemoved) override
		{	
			ScopedLock sl(group->lock);

			ModulatorSynth *m = dynamic_cast<ModulatorSynth*>(processorToBeRemoved);

			for(int i = 0; i < group->getNumVoices(); i++)
			{
				static_cast<ModulatorSynthGroupVoice*>(group->getVoice(i))->removeChildSynth(m);
			}

			group->synths.removeObject(m);

			group->checkFmState();

			sendChangeMessage();

		};

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex)
		{
			return (group->synths[processorIndex]);
		};

		const virtual Processor *getProcessor(int processorIndex) const override
		{
			return (group->synths[processorIndex]);
		};

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const override
		{
			return group->synths.size();
		};

		void clear() override
		{
			group->synths.clear();

			sendChangeMessage();
		};

	private:

		ModulatorSynthGroup *group;

	};

	String getFMState() const { return fmState; };
	void checkFmState();

	bool fmIsCorrectlySetup() const { return fmCorrectlySetup; };

private:

	// the precalculated modvalues for LFOs & stuff
	AudioSampleBuffer modSynthGainValues;

	String fmState;

	bool fmEnabled;
	bool fmCorrectlySetup;

	int modIndex;
	int carrierIndex;

	ScopedPointer<ModulatorChain> sampleStartChain;

	friend class ChildSynthIterator;

	friend class ModulatorSynthGroupVoice;

	ModulatorSynthGroupHandler handler;

	int numVoices;

	float vuValue;

	BigInteger allowStates;

	OwnedArray<ModulatorSynth> synths;

	ScopedPointer<FactoryType> modulatorSynthFactory;

	

};



#endif  // MODULATORSYNTHCHAIN_H_INCLUDED
