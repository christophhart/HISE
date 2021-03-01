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

#ifndef PROCESSORINTERFACES_H_INCLUDED
#define PROCESSORINTERFACES_H_INCLUDED

namespace hise { using namespace juce;

using namespace snex;

class ProcessorWithExternalData : public ExternalDataHolder
{
public:

	ProcessorWithExternalData(MainController* mc) :
		mc_internal(mc)
	{};

	virtual ~ProcessorWithExternalData() {};

	bool removeDataObject(ExternalData::DataType t, int index) override
	{
		return true;
	}

protected:

	ComplexDataUIBase* createAndInit(ExternalData::DataType t, bool useSampleLookup)
	{
		ComplexDataUIBase* d;

		if (useSampleLookup)
			d = ExternalData::create(t);
		else
			d = ExternalData::create<hise::MidiTable>(t);

		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(d))
			af->setProvider(new hise::PooledAudioFileDataProvider(mc_internal));

		d->setGlobalUIUpdater(getUpdater());
		d->setUndoManager(getUndoManager());

		return d;
	}

private:

	PooledUIUpdater* getUpdater()
	{
		return mc_internal->getGlobalUIUpdater();
	}

	UndoManager* getUndoManager()
	{
		return mc_internal->getControlUndoManager();
	}

	MainController* mc_internal = nullptr;
};

/** A baseclass interface for processors with a single data type. */
class ProcessorWithSingleStaticExternalData : public ProcessorWithExternalData
{
public:

	ProcessorWithSingleStaticExternalData(MainController* mc, ExternalData::DataType t, int numObjects=1, bool useSampleLookupTable=true):
		ProcessorWithExternalData(mc),
		dataType(t)
	{
		for (int i = 0; i < numObjects; i++)
		{
			ownedObjects.add(createAndInit(t, useSampleLookupTable));
		}
	}

	virtual ~ProcessorWithSingleStaticExternalData() {};

	Table* getTable(int index) final override
	{ 
		return static_cast<Table*>(getWithoutCreating(ExternalData::DataType::Table, index));
	}
	
	SliderPackData* getSliderPack(int index) final  override
	{
		return static_cast<SliderPackData*>(getWithoutCreating(ExternalData::DataType::SliderPack, index));
	}

	MultiChannelAudioBuffer* getAudioFile(int index) final override
	{
		return static_cast<MultiChannelAudioBuffer*>(getWithoutCreating(ExternalData::DataType::AudioFile, index));
	}

	int getNumDataObjects(ExternalData::DataType t) const  final override
	{
		return t == dataType ? ownedObjects.size() : 0;
	}
	
private:

	friend class LookupTableProcessor;
	friend class SliderPackProcessor;
	friend class AudioSampleProcessor;

	ComplexDataUIBase* getWithoutCreating(ExternalData::DataType requiredType, int index) const
	{
		if (dataType == requiredType && isPositiveAndBelow(index, ownedObjects.size()))
		{
			return ownedObjects[index];
		}

		return nullptr;
	}

	const ExternalData::DataType dataType;
	ReferenceCountedArray<ComplexDataUIBase> ownedObjects;
};

/** A baseclass interface for a constant amount of multiple data types. */
class ProcessorWithStaticExternalData : public ProcessorWithExternalData
{
public:

	ProcessorWithStaticExternalData(MainController* mc, int numTables, int numSliderPacks, int numAudioFiles, bool useSampleLookup=true):
		ProcessorWithExternalData(mc)
	{
		for (int i = 0; i < numTables; i++)
			tables.add(static_cast<Table*>(createAndInit(ExternalData::DataType::Table, useSampleLookup)));

		for (int i = 0; i < numSliderPacks; i++)
			sliderPacks.add(static_cast<SliderPackData*>(createAndInit(ExternalData::DataType::SliderPack, useSampleLookup)));

		for (int i = 0; i < numAudioFiles; i++)
			audioFiles.add(static_cast<MultiChannelAudioBuffer*>(createAndInit(ExternalData::DataType::AudioFile, useSampleLookup)));
	}

	int getNumDataObjects(ExternalData::DataType t) const final override
	{
		switch (t)
		{
		case ExternalData::DataType::Table: return tables.size();
		case ExternalData::DataType::SliderPack: return sliderPacks.size();
		case ExternalData::DataType::AudioFile: return audioFiles.size();
		}
	}

	Table* getTable(int index) final override
	{
		if(isPositiveAndBelow(index, tables.size()))
			return tables[index];

		return nullptr;
	}

	SliderPackData* getSliderPack(int index) final override
	{
		if (isPositiveAndBelow(index, sliderPacks.size()))
			return sliderPacks[index];

		return nullptr;
	}

	MultiChannelAudioBuffer* getAudioFile(int index) final override
	{
		if (isPositiveAndBelow(index, audioFiles.size()))
			return audioFiles[index];

		return nullptr;
	}

	Table* getTableUnchecked(int index = 0)
	{
		return *(tables.getRawDataPointer() + index);
	}

	const Table* getTableUnchecked(int index = 0) const
	{
		return *(tables.getRawDataPointer() + index);
	}

	SliderPackData* getSliderPackDataUnchecked(int index = 0)
	{
		return *(sliderPacks.getRawDataPointer() + index);
	}

	const SliderPackData* getSliderPackDataUnchecked(int index = 0) const
	{
		return *(sliderPacks.getRawDataPointer() + index);
	}

	MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0)
	{
		return *(audioFiles.getRawDataPointer() + index);
	}

	const MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0) const
	{
		return *(audioFiles.getRawDataPointer() + index);
	}

private:

	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
};

/** A baseclass for processors with multiple data types that can be resized
*/
class ProcessorWithDynamicExternalData : public ProcessorWithExternalData
{
public:

	ProcessorWithDynamicExternalData(MainController* mc, bool useSampleLookup_ = true) :
		ProcessorWithExternalData(mc),
		useSampleLookup(useSampleLookup_)
	{
	};

	virtual ~ProcessorWithDynamicExternalData() {};

	SliderPackData* getSliderPack(int index)  final override
	{
		if (auto d = sliderPacks[index])
			return d;

		auto s = createAndInit(snex::ExternalData::DataType::SliderPack, useSampleLookup);

		sliderPacks.set(index, static_cast<SliderPackData*>(s));
		return sliderPacks[index];
	}

	Table* getTable(int index) final override
	{
		if (auto d = tables[index])
			return d;

		auto s = createAndInit(snex::ExternalData::DataType::Table, useSampleLookup);

		tables.set(index, static_cast<Table*>(s));
		return tables[index];
	}

	MultiChannelAudioBuffer* getAudioFile(int index) final  override
	{
		if (auto d = audioFiles[index])
			return d;

		auto s = createAndInit(snex::ExternalData::DataType::AudioFile, useSampleLookup);

		audioFiles.set(index, static_cast<MultiChannelAudioBuffer*>(s));
		return audioFiles[index];
	}

	int getNumDataObjects(ExternalData::DataType t) const
	{
		switch (t)
		{
		case ExternalData::DataType::SliderPack: return sliderPacks.size();
		case ExternalData::DataType::Table: return tables.size();
		case ExternalData::DataType::AudioFile: return audioFiles.size();
		}

		return 0;
	}

	void saveComplexDataTypeAmounts(ValueTree& v) const
	{
		using namespace scriptnode;

		ExternalData::forEachType([&v, this](ExternalData::DataType t)
		{
			auto numObjects = getNumDataObjects(t);

			if (numObjects > 0)
				v.setProperty(ExternalData::getNumIdentifier(t), numObjects, nullptr);
		});
	}

	void restoreComplexDataTypes(const ValueTree& v)
	{
		using namespace scriptnode;

		ExternalData::forEachType([this, &v](ExternalData::DataType t)
		{
			auto numObjects = (int)v.getProperty(ExternalData::getNumIdentifier(t), 0);

			for (int i = 0; i < numObjects; i++)
				getComplexBaseType(t, i);
		});
	}

	void registerExternalObject(ExternalData::DataType t, int index, ComplexDataUIBase* obj)
	{
		switch (t)
		{
		case ExternalData::DataType::Table:
			tables.set(index, dynamic_cast<Table*>(obj)); break;
		case ExternalData::DataType::SliderPack:
			sliderPacks.set(index, dynamic_cast<SliderPackData*>(obj)); break;
		case ExternalData::DataType::AudioFile:
			audioFiles.set(index, dynamic_cast<MultiChannelAudioBuffer*>(obj)); break;
		}
	}


private:

	const bool useSampleLookup;

	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ProcessorWithDynamicExternalData);
};

/** A Processor that uses a Table.
*	@ingroup processor_interfaces
*
*	If your Processor uses a Table object for anything, you can subclass it from this interface class and use its Table. 
*	*/
class LookupTableProcessor: public ProcessorWithSingleStaticExternalData
{
public:

	struct ProcessorValueConverter
	{
		ProcessorValueConverter() :
			converter(Table::getDefaultTextValue),
			processor(nullptr)
		{};

		ProcessorValueConverter(const Table::ValueTextConverter& c, Processor* p) :
			converter(c),
			processor(p)
		{};

		ProcessorValueConverter(Processor* p) :
			converter(Table::getDefaultTextValue),
			processor(p)
		{};

		
		bool operator==(const ProcessorValueConverter& other) const
		{
			return other.processor == processor;
		}

		explicit operator bool() const
		{
			return processor != nullptr;
		}

		String getDefaultTextValue(float input)
		{
			if (processor.get())
				return converter(input);
			else
				return Table::getDefaultTextValue(input);
		}

		Table::ValueTextConverter converter;
		WeakReference<Processor> processor;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorValueConverter);
	};

	// ================================================================================================================

	LookupTableProcessor(MainController* mc, int numTables, bool useSampleLookUp);
	virtual ~LookupTableProcessor();

	//SET_PROCESSOR_CONNECTOR_TYPE_ID("LookupTableProcessor");

	// ================================================================================================================

	void addYValueConverter(const Table::ValueTextConverter& converter, Processor* p)
	{
		if (p == dynamic_cast<Processor*>(this))
			defaultYConverter = converter;
		else
		{
			for (int i = 0; i < yConverters.size(); i++)
			{
				auto thisP = yConverters[i]->processor.get();

				if (thisP == nullptr || thisP == p)
					yConverters.remove(i--);
			}

			yConverters.add(new ProcessorValueConverter(converter, p ));
		}

		updateYConverters();
	}

	void refreshYConvertersAfterRemoval()
	{
		for (int i = 0; i < yConverters.size(); i++)
		{
			auto thisP = yConverters[i]->processor.get();

			if (thisP == nullptr)
				yConverters.remove(i--);
		}

		updateYConverters();
	}

	Table* getTableUnchecked(int index = 0)
	{
		return static_cast<Table*>(ownedObjects.getUnchecked(index).get());
	}

	const Table* getTableUnchecked(int index = 0) const
	{
		return static_cast<const Table*>(ownedObjects.getUnchecked(index).get());
	}

	MidiTable* getMidiTable() { return static_cast<MidiTable*>(getTableUnchecked(0)); };

protected:

	Table::ValueTextConverter defaultYConverter = Table::getDefaultTextValue;

private:

	void updateYConverters()
	{
		const auto cToUse = yConverters.size() == 1 ? yConverters.getFirst()->converter : defaultYConverter;

		for (int i = 0; i < getNumDataObjects(snex::ExternalData::DataType::Table); i++)
			getTable(i)->setYTextConverterRaw(cToUse);
	}

	OwnedArray<ProcessorValueConverter> yConverters;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LookupTableProcessor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(LookupTableProcessor);

	// ================================================================================================================
};

class SliderPackData;

/** A Processor that uses a SliderPack. 
*	@ingroup processor_interfaces
*
*	It is a pure virtual class interface without member data to prevent the Diamond of Death.
*/
class SliderPackProcessor: public ProcessorWithSingleStaticExternalData
{
public:

	// ================================================================================================================

	SliderPackProcessor(MainController* mc, int numSliderPacks):
		ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::SliderPack, numSliderPacks)
	{};

	SliderPackData* getSliderPackUnchecked(int index = 0)
	{
		return static_cast<SliderPackData*>(ownedObjects.getUnchecked(index).get());
		
	}

	const SliderPackData* getSliderPackUnchecked(int index = 0) const
	{
		return static_cast<const SliderPackData*>(ownedObjects.getUnchecked(index).get());
	}

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPackProcessor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(SliderPackProcessor);

	// ================================================================================================================
};

using namespace snex;

#if 0
/** This is the base class for all processors that use complex data. 
	
	If your processor just holds a single data type, subclass it from the specialised
	subclasses LookupTableProcessor, SliderPackProcessor or AudioSampleProcessor.
	
	You will have to call setNumDataObjects() in your subclass constructor and then
	fetch the data objects using the calls getTable(), etc...

	STEPS TODO:  1. replace AudioSampleProcessor with this
				 2. replace SliderPackProcessor with this
				 3. replace TableProcessor with this
*/
struct ProcessorAsComplexDataHolder : public snex::ExternalDataHolder
{
	virtual ~ProcessorAsComplexDataHolder() {};

	int getNumDataObjects(ExternalData::DataType t) const override
	{
		switch (t)
		{
		case ExternalData::DataType::AudioFile: return audioFiles.size();
		case ExternalData::DataType::Table: return tables.size();
		case ExternalData::DataType::SliderPack: return sliderPacks.size();
		}
	}

	Table* getTable(int index) override
	{
		jassert(isPositiveAndBelow(index, tables.size()));

		return tables[index];
	}

	SliderPackData* getSliderPack(int index) override
	{
		jassert(isPositiveAndBelow(index, sliderPacks.size()));
		return sliderPacks[index];
	}

	MultiChannelAudioBuffer* getAudioFile(int index) override
	{ 
		jassert(isPositiveAndBelow(index, audioFiles.size()));
		return audioFiles[index];
	}

	/** Override this method and remove the object in question. Return true if successful. */
	bool removeDataObject(ExternalData::DataType t, int index) override
	{
		// not sure if we ever need it...
		return false;
	}

	void setNumDataObjects(ExternalData::DataType dt, int numObjects, Processor* p)
	{
		switch (dt)
		{
		case ExternalData::DataType::Table:
			ensureSize<Table>(tables, numObjects, p, useSampleLookUpTable ? createSampleLookUpTable : createMidiTable);
			break;
		case ExternalData::DataType::SliderPack:
			ensureSize<SliderPackData>(sliderPacks, numObjects, p, createSliderPackData);
			break;
		case ExternalData::DataType::AudioFile:
			ensureSize<MultiChannelAudioBuffer>(audioFiles, numObjects, p, createAudioFile);
			break;
		default:
			break;
		}
	}

	void setUseMidiTables()
	{
		useSampleLookUpTable = false;
	}

private:

	bool useSampleLookUpTable = true;

	static SliderPackData* createSliderPackData(Processor* p)
	{
		auto t = new SliderPackData();
		t->setGlobalUIUpdater(p->getMainController()->getGlobalUIUpdater());
		t->setUndoManager(p->getMainController()->getControlUndoManager());
		return t;
	}

	static MultiChannelAudioBuffer* createAudioFile(Processor* p)
	{
		auto t = new MultiChannelAudioBuffer();
		t->setGlobalUIUpdater(p->getMainController()->getGlobalUIUpdater());
		t->setUndoManager(p->getMainController()->getControlUndoManager());
		t->setProvider(new PooledAudioFileDataProvider(p->getMainController()));

		return t;
	}

	static Table* createSampleLookUpTable(Processor* p)
	{
		auto t = new SampleLookupTable();
		t->setGlobalUIUpdater(p->getMainController()->getGlobalUIUpdater());
		t->setUndoManager(p->getMainController()->getControlUndoManager());
		return t;
	}

	static Table* createMidiTable(Processor* p)
	{
		auto t = new MidiTable();
		t->setGlobalUIUpdater(p->getMainController()->getGlobalUIUpdater());
		t->setUndoManager(p->getMainController()->getControlUndoManager());
		return t;
	}

	template <typename SubType> static void ensureSize(ReferenceCountedArray<SubType>& list, int numRequired, Processor* p, const std::function<SubType*(Processor*)>& f)
	{
		if (list.size() != numRequired)
		{
			int numToAdd = numRequired - list.size();
			int numToDelete = -1 * numToAdd;

			for (int i = 0; i < numToAdd; i++)
				list.add(f(p));

			for (int i = 0; i < numToDelete; i++)
				list.removeLast();
		}
	}

	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
};
#endif


/** A Processor that uses a single audio sample.
*	@ingroup processor_interfaces
*
*	Be 
*
*	In order to use this class with a AudioSampleBufferComponent, just follow these steps:
*
*	1. Create a AudioSampleBufferComponent and use the method getCache() in the constructor.
*	2. Set the reference to the AudioSampleBuffer with AudioSampleBufferComponent::setAudioSampleBuffer();
*	3. Add the AudioSampleBuffer as ChangeListener (and remove it in the destructor!)
*	4. Add an AreaListener to the AudioSampleBufferComponent and call setRange() and setLoadedFile in the rangeChanged() callback

	Additional functions:

	- load / save to valuetree
	- reload when pool reference has changed
	- loop stuff

*/
class AudioSampleProcessor: public PoolBase::Listener,
							public ProcessorWithSingleStaticExternalData
{
public:

	// ================================================================================================================

	enum SyncToHostMode
	{
		FreeRunning = 1,
		OneBeat,
		TwoBeats,
		OneBar,
		TwoBars,
		FourBars
	};

	AudioSampleProcessor(MainController* mc) :
		ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::AudioFile, 1)
	{
		currentPool = &mc->getActiveFileHandler()->pool->getAudioSampleBufferPool();
	};

	/** Automatically releases the sample in the pool. */
	virtual ~AudioSampleProcessor();

	// ================================================================================================================

	/** Call this method within your exportAsValueTree method to store the sample settings. */
	void saveToValueTree(ValueTree &v) const;;

	/** Call this method within your restoreFromValueTree() method to load the sample settings. */
	void restoreFromValueTree(const ValueTree &v);

	/** This loads the file from disk (or from the pool, if existing and loadThisFile is false. */
	void setLoadedFile(const String &fileName, bool loadThisFile = false, bool forceReload = false);

	/** Sets the sample range that should be used in the plugin.
	*
	*	This is called automatically if a AudioSampleBufferComponent is set up correctly.
	*/


	void poolEntryReloaded(PoolReference referenceThatWasChanged) override;

	/** Returns the filename that was loaded.
	*
	*	It is possible that the file does not exist on your system:
	*	If you restore a pool completely from a ValueTree, it still uses the absolute filename as identification.
	*/
	String getFileName() const { return getBuffer().toBase64String(); };

	double getSampleRateForLoadedFile() const { return getBuffer().sampleRate; }

	AudioSampleBuffer& getAudioSampleBuffer() { return getBuffer().getBuffer(); }
	const AudioSampleBuffer& getAudioSampleBuffer() const { return getBuffer().getBuffer(); }

	MultiChannelAudioBuffer& getBuffer() { return *getAudioFileUnchecked(0); }
	const MultiChannelAudioBuffer& getBuffer() const { return *getAudioFileUnchecked(0); }

	MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0)
	{
		return static_cast<MultiChannelAudioBuffer*>(ownedObjects.getUnchecked(index).get());
	}

	const MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0) const
	{
		return static_cast<const MultiChannelAudioBuffer*>(ownedObjects.getUnchecked(index).get());
	}

protected:

	WeakReference<AudioSampleBufferPool> currentPool;

private:

	int getConstrainedLoopValue(String metadata);
	
	// ================================================================================================================
	
};



/** A Processor that has a dynamic size of child processors.
*	@ingroup processor_interfaces
*
*	If your Processor has more than a fixed amount of internal child processors, derive it from this class, write a Chain::Handler subclass with all
*	needed operations and you can add / delete Processors on runtime.
*
*	You might want to overwrite the Processors functions getNumChildProcessors() and getChildProcessor() with the handlers methods (handle internal chains manually)
*	This allows the restoreState function to only clear the dynamic list of processors.
*/
class Chain
{
public:

	// ====================================================================================================================

	/**
	*
	*	Subclass this, put it in your subclassed Chain and return a member object of the chain in Chain::getHandler().
	*/
	class Handler
	{
	public:

		struct Listener
		{
			enum EventType
			{
				ProcessorAdded,
				ProcessorDeleted,
				ProcessorOrderChanged,
				Cleared,
				numEventTypes
			};

            virtual ~Listener() {};
            
			virtual void processorChanged(EventType t, Processor* p) = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		// ================================================================================================================

		virtual ~Handler() {};

		/** Adds a new processor to the chain. It must be owned by the chain. */
		virtual void add(Processor *newProcessor, Processor *siblingToInsertBefore) = 0;

		/** Deletes a processor from the chain. */
		virtual void remove(Processor *processorToBeRemoved, bool deleteProcessor=true) = 0;

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex) = 0;

		virtual const Processor *getProcessor(int processorIndex) const = 0;

		/** Overwrite this method and implement a move operation. */
		virtual void moveProcessor(Processor* /*processorInChain*/, int /*delta*/) {};

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const = 0;

		/** Deletes all Processors in the Chain. */
		virtual void clear() = 0;

		void clearAsync(Processor* parent);

		void addListener(Listener* l)
		{
			listeners.addIfNotAlreadyThere(l);
		}

		void removeListener(Listener* l)
		{
			listeners.removeAllInstancesOf(l);
		}

		void addPostEventListener(Listener* l)
		{
			postEventlisteners.addIfNotAlreadyThere(l);
		}

		void removePostEventListener(Listener* l)
		{
			postEventlisteners.removeAllInstancesOf(l);
		}

		void notifyListeners(Listener::EventType t, Processor* p)
		{
			ScopedLock sl(listeners.getLock());

			for (auto l : listeners)
			{
				if (l != nullptr)
					l->processorChanged(t, p);
			}
		}

		void notifyPostEventListeners(Listener::EventType t, Processor* p)
		{
			ScopedLock sl(postEventlisteners.getLock());

			for (auto l : postEventlisteners)
			{
				if (l != nullptr)
					l->processorChanged(t, p);
			}
		}

	private:

		

		Array<WeakReference<Listener>, CriticalSection> listeners;
		Array<WeakReference<Listener>, CriticalSection> postEventlisteners;
	};

	// ===================================================================================================================

	/** Restores a Chain from a ValueTree. It creates all processors and restores their values. It returns false, if anything went wrong. */
	bool restoreChain(const ValueTree &v);

	/** Overwrite this and return the processor that owns this chain if it exists. */
	virtual Processor *getParentProcessor() = 0;

	/** Overwrite this and return the processor that owns this chain if it exists. */
	virtual const Processor *getParentProcessor() const = 0;

	/** return your subclassed Handler. */
	virtual Handler *getHandler() = 0;

	/** read only access to the Handler. */
	virtual const Handler *getHandler() const = 0;

	virtual ~Chain() {};

	/** Sets the FactoryType that will be used. */
	virtual void setFactoryType(FactoryType *newType) = 0;

	/** Returns the Factory type this processor is using. */
	virtual FactoryType *getFactoryType() const = 0;

	// ================================================================================================================


};

#define ADD_NAME_TO_TYPELIST(x) (typeNames.add(FactoryType::ProcessorEntry(x::getClassType(), x::getClassName())))

/** This interface class lets the MainController do its work.
*	@ingroup factory
*
*	You can tell a Processor (which should also be a Chain to make sense) to use a specific FactoryType 
*	with Processor::setFactoryType(), which will then use it in its popup menu to create the possible Processors. 
*	Simply overwrite these two functions in your subclass:
*
*		Processor* createProcessor	(int typeIndex, const String &id);
*		const StringArray& getTypeNames() const;
*
*	A FactoryType constrains the number of creatable Processors by
*
*	- Type (will be defined by the subclass)
*	- Constrainer (can be added to a FactoryType and uses runtime information like parent processor etc.)
*
*/
class FactoryType
{
public:

	// ================================================================================================================

	/** A Constrainer objects can impose restrictions on a particular FactoryType
	*
	*	If you want to restrict the selection of possible Processor types, you can
	*	subclass this, overwrite allowType with your custom rules and call
	*	setConstrainer() on the FactoryType you want to limit.
	*/
	class Constrainer: public BaseConstrainer
	{
	public:

		virtual ~Constrainer() {};

		/** Overwrite this and return true if the FactoryType can create this Processor and false, if not. */
		virtual bool allowType(const Identifier &typeName) = 0;

		virtual String getDescription() const = 0;

	};

	// ================================================================================================================

	/** a simple POD which contains the id and the name of a Processor type. */
	struct ProcessorEntry
	{
		ProcessorEntry(const Identifier t, const String &n);;
		ProcessorEntry() {};

		Identifier type;
		String name;
	};

	// ================================================================================================================

	/** Creates a Factory type.  */
	FactoryType(Processor *owner_);;
	virtual ~FactoryType();

	// ================================================================================================================

	/** Fills a popupmenu with all allowed processor types.
	*
	*	You can pass in a startIndex, if you overwrite this method for nested calls.
	*
	*	It returns the last index that can be used for the next menus.
	*/
	virtual int fillPopupMenu(PopupMenu &m, int startIndex = 1);;

	/** Overwrite this function and return a processor of the specific type index. */
	virtual Processor *createProcessor(int typeIndex, const String &ProcessorId) = 0;

	/** Returns the typeName using the result from the previously created popupmenu. */
	Identifier getTypeNameFromPopupMenuResult(int resultFromPopupMenu);

	/** Returns the typeName using the result from the previously created popupmenu. */
	String getNameFromPopupMenuResult(int resultFromPopupMenu);

	/** Returns the index of the type. */
	virtual int getProcessorTypeIndex(const Identifier &typeName) const;;

	/** Returns the number of Processors that this factory can create.
	*
	*	the rules defined in allowType are applied before counting the possible processors.
	*/
	virtual int getNumProcessors();

	const Processor *getOwnerProcessor() const { return owner.get(); };
	Processor *getOwnerProcessor()			   { return owner.get(); };

	/**	Checks if the type of the processor is found in the type name.
	*
	*	You can overwrite this and add more conditions (in this case, call the base class method first to keep things safe!
	*/
	virtual bool allowType(const Identifier &typeName) const;;

	/** Returns a unique ID for the new Processor.
	*
	*	It scans all Processors and returns something like "Processor12" if there are 11 other Processors with the same ID
	*/
	static String getUniqueName(Processor *id, String name = String());


	/** Returns a string array with all allowed types that this factory can produce. */
	virtual Array<ProcessorEntry> getAllowedTypes();;

	/** adds a Constrainer to a FactoryType. It will be owned by the FactoryType. You can pass nullptr. */
	virtual void setConstrainer(Constrainer *newConstrainer, bool ownConstrainer = true);

	Constrainer *getConstrainer();

	// ================================================================================================================

protected:

	/** This should only be overwritten by the subclasses. For external usage, use getAllowedTypes(). */
	virtual const Array<ProcessorEntry> &getTypeNames() const = 0;

	WeakReference<Processor> owner;

private:

	// iterates all child processors and counts the number of same IDs.
	static bool countProcessorsWithSameId(int &index, const Processor *p, 
										  Processor *processorToLookFor, const String &nameToLookFor);

	ScopedPointer<Constrainer> ownedConstrainer;
	Constrainer *constrainer;

	mutable bool baseClassCalled;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FactoryType);

	// ================================================================================================================
};

} // namespace hise

#endif  // PROCESSORINTERFACES_H_INCLUDED
