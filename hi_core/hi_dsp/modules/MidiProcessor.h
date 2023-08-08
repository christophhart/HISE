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

#ifndef MIDIPROCESSOR_H_INCLUDED
#define MIDIPROCESSOR_H_INCLUDED

namespace hise { using namespace juce;

class MidiProcessorEditor;
class ModulatorSynth;

#define MIDI_PROCESSOR_COLOUR 0xFFC65638

/**	A MidiProcessor processes a MidiBuffer.
*	@ingroup dsp_base_classes
*
*	It can be used to change the incoming MIDI data before it is sent to a ModulatorSynth. Note that if you want to create your own MIDI processors,
	you should use the HardcodedScriptProcessor as base class since it offers a simpler integration of existing Javascript code and a cleaner API.
*/
class MidiProcessor: public Processor
{
public:

    struct EventLogger;
    
	/** Creates a new MidiProcessor. You can supply a ModulatorSynth which owns the MidiProcessor to allow the processor to change its properties. */
	MidiProcessor(MainController *m, const String &id);
	virtual ~MidiProcessor();

	void setIndexInChain(int chainIndex) noexcept;
    int getIndexInChain() const noexcept;

    /** Changes the timestamp of an artificial note. */
	bool setArtificialTimestamp(uint16 eventId, int newTimestamp);

	void addHiseEventToBuffer(const HiseEvent &m);

	Colour getColour() const;;

	static Path getSymbolPath();

    Path getSpecialSymbol() const override;;

	virtual bool isProcessingWholeBuffer() const;

    /** Normally a MidiProcessor has no child processors, but it is virtual for the MidiProcessorChain. */
	virtual Processor *getChildProcessor(int /*processorIndex*/) override;;

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override;;

	/** Normally a MidiProcessor has no child processors, but it is virtual for the MidiProcessorChain. */
	virtual int getNumChildProcessors() const override;;

	/** If you want an editor that is more than the header, overwrite this method and return a subclass of ProcessorEditorBody. */
	virtual ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	
	/** Process the incoming event. */
	virtual void processHiseEvent(HiseEvent &e) = 0;

	/** If this method is called within processMidiMessage(), the message will be ignored. */
	void ignoreEvent();;

	/** Overwrite this method if your processor wants to process the entire buffer at once in addition of single messages.
	
		By default this is deactivated, but if you override isProcessingWholeBuffer() and return true, it will use this
		method. */
	virtual void preprocessBuffer(HiseEventBuffer& buffer, int numSamples);

    bool isProcessed() const;;

	bool processThisMessage;

	virtual void setOwnerSynth(ModulatorSynth *ownerSynth_);;

	ModulatorSynth *getOwnerSynth();;

    void setEnableEventLogger(bool shouldBeEnabled);
    
    void logIfEnabled(const HiseEvent& e, bool beforeProcessing);
    
    Component* createEventLogComponent();
    
protected:

	/** the sample position within the processBlock. */
	int samplePos;

private:

#if USE_BACKEND
    SimpleReadWriteLock eventLock;
    ScopedPointer<EventLogger> eventLogger;
#endif
    
	int numThisTime;
	int indexInChain = -1;

	WeakReference<MidiProcessor>::Master masterReference;
    friend class WeakReference<MidiProcessor>;

	ModulatorSynth *ownerSynth;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiProcessor)
};

class ChainEditor;

/** @internal A MidiProcessorChain is a container for multiple MidiProcessors. 
*
*/
class MidiProcessorChain: public MidiProcessor,
						  public Chain
{
public:

	SET_PROCESSOR_NAME("MidiProcessorChain", "Midi Processor Chain", "chain");

	MidiProcessorChain(MainController *m, const String &id, Processor *ownerProcessor);

	~MidiProcessorChain();;

	/** Wraps the handlers method. */
	int getNumChildProcessors() const override;;

	/** Wraps the handlers method. */
	Processor *getChildProcessor(int processorIndex) override;;

	Processor *getParentProcessor() override;;

	const Processor *getParentProcessor() const override;;

	const Processor *getChildProcessor(int processorIndex) const override;;

	float getAttribute(int ) const override;;
	void setInternalAttribute(int, float) override;;

	Chain::Handler *getHandler() override;;

	const Chain::Handler *getHandler() const override;;

	FactoryType *getFactoryType() const override;;

	void setFactoryType(FactoryType *newFactoryType) override;;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void addArtificialEvent(const HiseEvent& m);

	bool setArtificialTimestamp(uint16 eventId, int newTimestamp);

	void sendAllNoteOffEvent();;

	void renderNextHiseEventBuffer(HiseEventBuffer &buffer, int numSamples);

	void logEvents(HiseEventBuffer& buffer, bool isBefore);

	/** Sequentially processes all processors. */
	void processHiseEvent(HiseEvent &m) override;;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void addWholeBufferProcessor(MidiProcessor* midiProcessor);

	/** @internal Handles the creation of MidiProcessors within a MidiProcessorChain. */
	class MidiProcessorChainHandler: public Chain::Handler
	{
	public:

		MidiProcessorChainHandler(MidiProcessorChain *c);;

		void add(Processor *newProcessor, Processor *siblingToInsertBefore);

		void remove(Processor *processorToBeRemoved, bool deleteMp=true);;

		const Processor *getProcessor(int processorIndex) const override;

		Processor *getProcessor(int processorIndex) override;

		int getNumProcessors() const;;

		void clear();

	private:

		MidiProcessorChain *chain;
	};

private:

	friend class MidiPlayer;

	bool allNotesOffAtNextBuffer;
	ScopedPointer<FactoryType> midiProcessorFactory;

	Processor* parentProcessor;

	MidiProcessorChainHandler handler;

	friend class MidiProcessorChainHandler;

	OwnedArray<MidiProcessor> processors;

	Array<WeakReference<MidiProcessor>> wholeBufferProcessors;

	HiseEventBuffer futureEventBuffer;
	HiseEventBuffer artificialEvents;

};

class HardcodedScriptFactoryType;


class MidiProcessorFactoryType: public FactoryType
{
public:

	// private enum for handling
	enum
	{
		scriptProcessor = 0,
		transposer,
		midiFilePlayer,
		chokeGroupProcessor,
		numMidiProcessors
	};

	MidiProcessorFactoryType(Processor *p);

	~MidiProcessorFactoryType()
	{
		typeNames.clear();
	};

	int fillPopupMenu(PopupMenu &m, int startIndex=0) override;
	
	bool allowType(const Identifier &typeName) const override;

	Processor *createProcessor(int typeIndex, const String &id) override;
	
	const Array<ProcessorEntry> & getTypeNames() const override
	{
		return typeNames;
	};

private:

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiProcessorFactoryType)

	Array<ProcessorEntry> typeNames;

	ScopedPointer<HardcodedScriptFactoryType> hardcodedScripts;
};

class HardcodedScriptFactoryType : public FactoryType
{
	// private enum for handling
	enum
	{
		legatoWithRetrigger = MidiProcessorFactoryType::numMidiProcessors,
		ccSwapper,
		releaseTrigger,
		cc2Note,
		channelFilter,
		channelSetter,
		muteAll,
		arpeggiator,
	};

public:

	HardcodedScriptFactoryType(Processor* p);;

	void fillTypeNameList();

	~HardcodedScriptFactoryType();;

	Processor* createProcessor(int typeIndex, const String& id) override;

	const Array<ProcessorEntry>& getTypeNames() const override;;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardcodedScriptFactoryType)

		Array<ProcessorEntry> typeNames;

};

class ModulatorSynth;


} // namespace hise

#endif  // MIDIPROCESSOR_H_INCLUDED
