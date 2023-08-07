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

	String getDescription() const override;

	bool allowType(const Identifier &typeName) override;

private:

	Array<FactoryType::ProcessorEntry> forbiddenModulators;
};

class SynthGroupFXConstrainer : public FactoryType::Constrainer
{
public:

    SynthGroupFXConstrainer();

    String getDescription() const override;

    bool allowType(const Identifier &typeName) override;
};

class SynthGroupConstrainer : public FactoryType::Constrainer
{
public:

	SynthGroupConstrainer();

	String getDescription() const override;

	bool allowType(const Identifier &typeName) override;

private:

	Array<FactoryType::ProcessorEntry> forbiddenModulators;
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

	Chain::Handler *getHandler() override;;
	const Chain::Handler *getHandler() const override;;

	FactoryType *getFactoryType() const override;;
	void setFactoryType(FactoryType *newFactoryType) override;;

	/** returns the total amount of child groups (internal chains + all child synths) */
	int getNumChildProcessors() const override;;

	/** returns the total amount of child groups (internal chains + all child synths) */
	const Processor *getChildProcessor(int processorIndex) const override;;

	Processor *getChildProcessor(int processorIndex) override;;

	/** Prepares all ModulatorSynths for playback. */
	void prepareToPlay (double newSampleRate, int samplesPerBlock) override;;


	void numSourceChannelsChanged() override;

	void numDestinationChannelsChanged() override;

	ValueTree exportAsValueTree() const override;

	void addProcessorsWhenEmpty() override;;

	Processor *getParentProcessor();;

	const Processor *getParentProcessor() const;;

	/** Restores the Processor. Don't forget to call compileAllScripts() !. */
	void restoreFromValueTree(const ValueTree &v) override;
    
    void reset();

	void setPackageName(const String &newPackageName);

	String getPackageName() const;;
	
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

	int getVoiceAmount() const;;

	int getNumActiveVoices() const override;

	void killAllVoices() override;
	
	void resetAllVoices() override;

	bool areVoicesActive() const override;

	/** Handles the ModulatorSynthChain. */
	class ModulatorSynthChainHandler: public Chain::Handler
	{
	public:
		ModulatorSynthChainHandler(ModulatorSynthChain *synthToHandle);;

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

	void setActiveChannels(const HiseEvent::ChannelFilterData& newActiveChannels);

	HiseEvent::ChannelFilterData* getActiveChannelData();

	void setUseUniformVoiceHandler(bool shouldUseVoiceHandler, UniformVoiceHandler* externalVoiceHandler) override;

    bool isUniformVoiceHandlerRoot() const;;
	
private:

	ScopedPointer<UniformVoiceHandler> ownedUniformVoiceHandler;

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
