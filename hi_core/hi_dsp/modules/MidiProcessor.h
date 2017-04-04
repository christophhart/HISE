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

#ifndef MIDIPROCESSOR_H_INCLUDED
#define MIDIPROCESSOR_H_INCLUDED

class MidiProcessorEditor;
class ModulatorSynth;

#define MIDI_PROCESSOR_COLOUR 0xFFC65638

/**	A MidiProcessor processes a MidiBuffer.
*	@ingroup midiProcessor
*
*	It can be used to change the incoming MIDI data before it is sent to a ModulatorSynth.
*/
class MidiProcessor: public Processor
{
public:

	/** Creates a new MidiProcessor. You can supply a ModulatorSynth which owns the MidiProcessor to allow the processor to change its properties. */
	MidiProcessor(MainController *m, const String &id);
	virtual ~MidiProcessor();

	void setIndexInChain(int chainIndex) noexcept { indexInChain = chainIndex; }
	int getIndexInChain() const noexcept { return indexInChain; }

	void addHiseEventToBuffer(const HiseEvent &m);

	Colour getColour() const
    {
        return Colour(MIDI_PROCESSOR_COLOUR);
    };

	static Path getSymbolPath()
	{
		Path path;

		static const unsigned char pathData[] = { 110, 109, 0, 0, 226, 66, 92, 46, 226, 67, 98, 0, 0, 226, 66, 112, 2, 227, 67, 79, 80, 223, 66, 92, 174, 227, 67, 0, 0, 220, 66, 92, 174, 227, 67, 98, 177, 175, 216, 66, 92, 174, 227, 67, 0, 0, 214, 66, 112, 2, 227, 67, 0, 0, 214, 66, 92, 46, 226, 67, 98, 0, 0, 214, 66, 72, 90, 225, 67, 177, 175, 216, 66, 92, 174, 224, 67, 0, 0,
			220, 66, 92, 174, 224, 67, 98, 79, 80, 223, 66, 92, 174, 224, 67, 0, 0, 226, 66, 72, 90, 225, 67, 0, 0, 226, 66, 92, 46, 226, 67, 99, 109, 0, 128, 218, 66, 92, 110, 222, 67, 98, 0, 128, 218, 66, 112, 66, 223, 67, 79, 208, 215, 66, 92, 238, 223, 67, 0, 128, 212, 66, 92, 238, 223, 67, 98, 177, 47, 209, 66, 92, 238, 223, 67, 0,
			128, 206, 66, 112, 66, 223, 67, 0, 128, 206, 66, 92, 110, 222, 67, 98, 0, 128, 206, 66, 72, 154, 221, 67, 177, 47, 209, 66, 92, 238, 220, 67, 0, 128, 212, 66, 92, 238, 220, 67, 98, 79, 208, 215, 66, 92, 238, 220, 67, 0, 128, 218, 66, 72, 154, 221, 67, 0, 128, 218, 66, 92, 110, 222, 67, 99, 109, 0, 128, 203, 66, 92, 142, 220,
			67, 98, 0, 128, 203, 66, 112, 98, 221, 67, 79, 208, 200, 66, 92, 14, 222, 67, 0, 128, 197, 66, 92, 14, 222, 67, 98, 177, 47, 194, 66, 92, 14, 222, 67, 0, 128, 191, 66, 112, 98, 221, 67, 0, 128, 191, 66, 92, 142, 220, 67, 98, 0, 128, 191, 66, 72, 186, 219, 67, 177, 47, 194, 66, 92, 14, 219, 67, 0, 128, 197, 66, 92, 14, 219,
			67, 98, 79, 208, 200, 66, 92, 14, 219, 67, 0, 128, 203, 66, 72, 186, 219, 67, 0, 128, 203, 66, 92, 142, 220, 67, 99, 109, 0, 128, 188, 66, 92, 110, 222, 67, 98, 0, 128, 188, 66, 112, 66, 223, 67, 79, 208, 185, 66, 92, 238, 223, 67, 0, 128, 182, 66, 92, 238, 223, 67, 98, 177, 47, 179, 66, 92, 238, 223, 67, 0, 128, 176, 66,
			112, 66, 223, 67, 0, 128, 176, 66, 92, 110, 222, 67, 98, 0, 128, 176, 66, 72, 154, 221, 67, 177, 47, 179, 66, 92, 238, 220, 67, 0, 128, 182, 66, 92, 238, 220, 67, 98, 79, 208, 185, 66, 92, 238, 220, 67, 0, 128, 188, 66, 72, 154, 221, 67, 0, 128, 188, 66, 92, 110, 222, 67, 99, 109, 0, 0, 181, 66, 92, 46, 226, 67, 98, 0, 0, 181,
			66, 112, 2, 227, 67, 79, 80, 178, 66, 92, 174, 227, 67, 0, 0, 175, 66, 92, 174, 227, 67, 98, 177, 175, 171, 66, 92, 174, 227, 67, 0, 0, 169, 66, 112, 2, 227, 67, 0, 0, 169, 66, 92, 46, 226, 67, 98, 0, 0, 169, 66, 72, 90, 225, 67, 177, 175, 171, 66, 92, 174, 224, 67, 0, 0, 175, 66, 92, 174, 224, 67, 98, 79, 80, 178, 66, 92, 174,
			224, 67, 0, 0, 181, 66, 72, 90, 225, 67, 0, 0, 181, 66, 92, 46, 226, 67, 99, 109, 0, 128, 197, 66, 151, 79, 215, 67, 98, 243, 139, 173, 66, 151, 79, 215, 67, 0, 0, 154, 66, 148, 50, 220, 67, 0, 0, 154, 66, 151, 47, 226, 67, 98, 0, 0, 154, 66, 154, 44, 232, 67, 243, 139, 173, 66, 151, 15, 237, 67, 0, 128, 197, 66, 151, 15, 237,
			67, 98, 12, 116, 221, 66, 151, 15, 237, 67, 0, 0, 241, 66, 154, 44, 232, 67, 0, 0, 241, 66, 151, 47, 226, 67, 98, 0, 0, 241, 66, 148, 50, 220, 67, 13, 116, 221, 66, 151, 79, 215, 67, 0, 128, 197, 66, 151, 79, 215, 67, 99, 109, 0, 128, 197, 66, 151, 79, 218, 67, 98, 209, 247, 214, 66, 151, 79, 218, 67, 0, 0, 229, 66, 163, 209,
			221, 67, 0, 0, 229, 66, 151, 47, 226, 67, 98, 0, 0, 229, 66, 139, 141, 230, 67, 210, 247, 214, 66, 151, 15, 234, 67, 0, 128, 197, 66, 151, 15, 234, 67, 98, 47, 8, 180, 66, 151, 15, 234, 67, 0, 0, 166, 66, 139, 141, 230, 67, 0, 0, 166, 66, 151, 47, 226, 67, 98, 0, 0, 166, 66, 163, 209, 221, 67, 47, 8, 180, 66, 151, 79, 218, 67,
			0, 128, 197, 66, 151, 79, 218, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		return path;
	}

	Path getSpecialSymbol() const override
	{
		return getSymbolPath();
	};

	/** Normally a MidiProcessor has no child processors, but it is virtual for the MidiProcessorChain. */
	virtual Processor *getChildProcessor(int /*processorIndex*/) override {return nullptr;};

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override {return nullptr;};

	/** Normally a MidiProcessor has no child processors, but it is virtual for the MidiProcessorChain. */
	virtual int getNumChildProcessors() const override {return 0;};

	/** If you want an editor that is more than the header, overwrite this method and return a subclass of ProcessorEditorBody. */
	virtual ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor) override;

	
	/** Process the incoming event. */
	virtual void processHiseEvent(HiseEvent &e) = 0;

	/** If this method is called within processMidiMessage(), the message will be ignored. */
	void ignoreEvent() { processThisMessage = false; };



	bool isProcessed() const {return processThisMessage;};

	bool processThisMessage;

	virtual void setOwnerSynth(ModulatorSynth *ownerSynth_)
	{
		jassert(ownerSynth == nullptr);
		ownerSynth = ownerSynth_;
	};

	ModulatorSynth *getOwnerSynth() {return ownerSynth;};

protected:

	/** the sample position within the processBlock. */
	int samplePos;

private:

	int numThisTime;
	int indexInChain = -1;

	WeakReference<MidiProcessor>::Master masterReference;
    friend class WeakReference<MidiProcessor>;

	ModulatorSynth *ownerSynth;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiProcessor)
};

class ChainEditor;

/** A MidiProcessorChain is a container for multiple MidiProcessors. 
*	@ingroup midiTypes
*
*/
class MidiProcessorChain: public MidiProcessor,
						  public Chain
{
public:

	SET_PROCESSOR_NAME("MidiProcessorChain", "Midi Processor Chain");

	MidiProcessorChain(MainController *m, const String &id, Processor *ownerProcessor);

	~MidiProcessorChain()
	{
		processors.clear();
	};

	/** Wraps the handlers method. */
	int getNumChildProcessors() const override { return handler.getNumProcessors();	};

	/** Wraps the handlers method. */
	Processor *getChildProcessor(int processorIndex) override { return handler.getProcessor(processorIndex);	};

	Processor *getParentProcessor() override { return parentProcessor; };

	const Processor *getParentProcessor() const override { return parentProcessor; };

	const Processor *getChildProcessor(int processorIndex) const override { return handler.getProcessor(processorIndex);	};

	float getAttribute(int ) const override {return 1.0f;};
	void setInternalAttribute(int, float) override {};

	Chain::Handler *getHandler() override {return &handler;};

	const Chain::Handler *getHandler() const override {return &handler;};

	FactoryType *getFactoryType() const override {return midiProcessorFactory;};

	void setFactoryType(FactoryType *newFactoryType) override {midiProcessorFactory = newFactoryType;};

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void addArtificialEvent(const HiseEvent& m);

	void sendAllNoteOffEvent()
	{
		allNotesOffAtNextBuffer = true;
	};

	void renderNextHiseEventBuffer(HiseEventBuffer &buffer, int numSamples);

	/** Sequentially processes all processors. */
	void processHiseEvent(HiseEvent &m) override
	{
		if (isBypassed())
		{
			if (m.isTimerEvent()) m.ignoreEvent(true);
			return;
		}
		for(int i = 0; (i < processors.size()); i++)
		{
			if (processors[i]->isBypassed())
			{
				if (m.isTimerEvent() && processors[i]->getIndexInChain() == m.getTimerIndex())
				{
					m.ignoreEvent(true);
				}

				continue;
			}

            if(m.isIgnored())
                continue;
            
			processors[i]->processHiseEvent(m);
		}
	};

	/** Handles the creation of MidiProcessors within a MidiProcessorChain. */
	class MidiProcessorChainHandler: public Chain::Handler
	{
	public:

		MidiProcessorChainHandler(MidiProcessorChain *c):
			chain(c)
		{};

		void add(Processor *newProcessor, Processor *siblingToInsertBefore);

		void remove(Processor *processorToBeRemoved)
		{
			ScopedLock sl(chain->getMainController()->getLock());

			jassert(dynamic_cast<MidiProcessor*>(processorToBeRemoved) != nullptr);
			for(int i = 0; i < chain->processors.size(); i++)
			{
				if (chain->processors[i] == processorToBeRemoved)
				{
					chain->processors.remove(i);
					break;
				}
			}

			sendChangeMessage();
		};

		const Processor *getProcessor(int processorIndex) const override
		{
			return chain->processors[processorIndex];
		}

		Processor *getProcessor(int processorIndex) override
		{
			return chain->processors[processorIndex];
		}

		int getNumProcessors() const
		{
			return chain->processors.size();
		};

		void clear()
		{
			chain->processors.clear();

			sendChangeMessage();
		}

	private:

		MidiProcessorChain *chain;
	};

private:


	bool allNotesOffAtNextBuffer;
	ScopedPointer<FactoryType> midiProcessorFactory;

	Processor* parentProcessor;

	MidiProcessorChainHandler handler;

	friend class MidiProcessorChainHandler;

	OwnedArray<MidiProcessor> processors;

	HiseEventBuffer futureEventBuffer;
	HiseEventBuffer artificialEvents;

};



class HardcodedScriptFactoryType;

/**	Allows creation of MidiProcessors.
*
*	@ingroup midiProcessor
*/
class MidiProcessorFactoryType: public FactoryType
{
public:

	// private enum for handling
	enum
	{
		scriptProcessor = 0,
		transposer,
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

class ModulatorSynth;




#endif  // MIDIPROCESSOR_H_INCLUDED
