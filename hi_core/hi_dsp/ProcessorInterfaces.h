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

	ProcessorWithExternalData(MainController* mc);;

	virtual ~ProcessorWithExternalData();;

	bool removeDataObject(ExternalData::DataType t, int index) override;

	void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex) override;

	FilterDataObject* getFilterData(int index) override;

	Colour getSharedReferenceColour(ExternalData::DataType t, int index) const;

protected:

	ComplexDataUIBase* createAndInit(ExternalData::DataType t);

	virtual void referenceShared(ExternalData::DataType type, int index);

private:

	struct SharedReference
	{
		SharedReference(ExternalData::DataType type_, int index_, Colour c_);;

		bool operator==(const SharedReference& other) const;

		ExternalData::DataType type;
		int index;
		Colour c;
	};

	Array<SharedReference> sharedReferences;

    PooledUIUpdater* getUpdater();

	UndoManager* getUndoManager();

	MainController* mc_internal = nullptr;
};

/** A baseclass interface for processors with a single data type. */
class ProcessorWithSingleStaticExternalData: public ProcessorWithExternalData
{
public:

	ProcessorWithSingleStaticExternalData(MainController* mc, ExternalData::DataType t, int numObjects=1);

	virtual ~ProcessorWithSingleStaticExternalData();;

	Table* getTable(int index) final override;

	SliderPackData* getSliderPack(int index) final  override;

	MultiChannelAudioBuffer* getAudioFile(int index) final override;

	SimpleRingBuffer* getDisplayBuffer(int index) final override;

	FilterDataObject* getFilterData(int index) final override
	{
		return static_cast<FilterDataObject*>(getWithoutCreating(ExternalData::DataType::FilterCoefficients, index));
	}

	int getNumDataObjects(ExternalData::DataType t) const  final override;

	void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex) override;

private:

	friend class LookupTableProcessor;
	friend class SliderPackProcessor;
	friend class AudioSampleProcessor;

	ComplexDataUIBase* getWithoutCreating(ExternalData::DataType requiredType, int index) const;

	const ExternalData::DataType dataType;
	ReferenceCountedArray<ComplexDataUIBase> ownedObjects;
};

/** A baseclass interface for a constant amount of multiple data types. */
class ProcessorWithStaticExternalData : public ProcessorWithExternalData
{
public:

	ProcessorWithStaticExternalData(MainController* mc, int numTables, int numSliderPacks, int numAudioFiles, int numDisplayBuffers);

	int getNumDataObjects(ExternalData::DataType t) const final override;

	Table* getTable(int index) final override;

	SliderPackData* getSliderPack(int index) final override;

	MultiChannelAudioBuffer* getAudioFile(int index) final override;

	SimpleRingBuffer* getDisplayBuffer(int index) final override;

	SampleLookupTable* getTableUnchecked(int index = 0);

	const SampleLookupTable* getTableUnchecked(int index = 0) const;

	SliderPackData* getSliderPackDataUnchecked(int index = 0);

	const SliderPackData* getSliderPackDataUnchecked(int index = 0) const;

	MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0);

	const MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0) const;

	SimpleRingBuffer* getDisplayBufferUnchecked(int index = 0);

	const SimpleRingBuffer* getDisplayBufferUnchecked(int index = 0) const;

	void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex) override;

private:

	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
	ReferenceCountedArray<SimpleRingBuffer> displayBuffers;
};

/** A baseclass for processors with multiple data types that can be resized
*/
class ProcessorWithDynamicExternalData : public ProcessorWithExternalData
{
public:

	ProcessorWithDynamicExternalData(MainController* mc);;

	virtual ~ProcessorWithDynamicExternalData();;

	SliderPackData* getSliderPack(int index)  final override;

	Table* getTable(int index) final override;

	MultiChannelAudioBuffer* getAudioFile(int index) final  override;

	SimpleRingBuffer* getDisplayBuffer(int index) final override;

	FilterDataObject* getFilterData(int index) final override;

	int getNumDataObjects(ExternalData::DataType t) const;

	void saveComplexDataTypeAmounts(ValueTree& v) const;

	void restoreComplexDataTypes(const ValueTree& v);

	void registerExternalObject(ExternalData::DataType t, int index, ComplexDataUIBase* obj);

	void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex) override;

private:

	template <typename T> bool ensurePosition(ReferenceCountedArray<T>& list, int newSize)
	{
		auto numToInsert = newSize - list.size();

		if(numToInsert <= 1)
			return false;

		list.ensureStorageAllocated(newSize);

		for(int i = 0; i < numToInsert; i++)
			list.add(nullptr);

		return true;
	}

	ReferenceCountedArray<SliderPackData> sliderPacks;
	ReferenceCountedArray<Table> tables;
	ReferenceCountedArray<MultiChannelAudioBuffer> audioFiles;
	ReferenceCountedArray<SimpleRingBuffer> ringBuffers;
	ReferenceCountedArray<FilterDataObject> filterData;

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
		ProcessorValueConverter();;

		ProcessorValueConverter(const Table::ValueTextConverter& c, Processor* p);;

		ProcessorValueConverter(Processor* p);;

		
		bool operator==(const ProcessorValueConverter& other) const;

		explicit operator bool() const;

		String getDefaultTextValue(float input);

		Table::ValueTextConverter converter;
		WeakReference<Processor> processor;

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorValueConverter);
	};

	// ================================================================================================================

	LookupTableProcessor(MainController* mc, int numTables);
	virtual ~LookupTableProcessor();

	//SET_PROCESSOR_CONNECTOR_TYPE_ID("LookupTableProcessor");

	// ================================================================================================================

	void addYValueConverter(const Table::ValueTextConverter& converter, Processor* p);

	void refreshYConvertersAfterRemoval();

	SampleLookupTable* getTableUnchecked(int index = 0);

	const SampleLookupTable* getTableUnchecked(int index = 0) const;

protected:

	Table::ValueTextConverter defaultYConverter = Table::getDefaultTextValue;

private:

	void updateYConverters();

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

	SliderPackProcessor(MainController* mc, int numSliderPacks);;

	SliderPackData* getSliderPackUnchecked(int index = 0);

	const SliderPackData* getSliderPackUnchecked(int index = 0) const;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliderPackProcessor);
	JUCE_DECLARE_WEAK_REFERENCEABLE(SliderPackProcessor);

	// ================================================================================================================
};

using namespace snex;

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
		FourBars,
		EightBars,
		TwelveBars,
		SixteenBars
	};

	AudioSampleProcessor(MainController* mc);;

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
	String getFileName() const;;

	double getSampleRateForLoadedFile() const;

	AudioSampleBuffer& getAudioSampleBuffer();
	const AudioSampleBuffer& getAudioSampleBuffer() const;

	MultiChannelAudioBuffer& getBuffer();
	const MultiChannelAudioBuffer& getBuffer() const;

	MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0);

	const MultiChannelAudioBuffer* getAudioFileUnchecked(int index = 0) const;

protected:

	WeakReference<AudioSampleBufferPool> currentPool;

private:

	int getConstrainedLoopValue(String metadata);
	
	// ================================================================================================================
	
};

/** Create this object and pass it to the constructor and it will query the filter stats.
 *
 *  This allows you to use custom build nodes to use the draggable filter path.
 */
struct ProcessorFilterStatistics: public ReferenceCountedObject
{
	/** The parameters for each band. */
	enum BandParameter
	{
		Gain = 0, ///< the gain (not available on HP/LP)
		Freq, ///< the center frequency
		Q, ///< the q factor (not available on HP/LP)
		Enabled, ///< enables / disables the band
		Type, ///< defines the type of the band @see FilterType
		numBandParameters
	};

	enum class DragAction
	{
		DragX,
		DragY,
		ShiftDrag,
		DoubleClick,
		RightClick
	};

	using Ptr = ReferenceCountedObjectPtr<ProcessorFilterStatistics>;

	struct Holder
	{
		virtual ~Holder() {};

		bool isFilterStatisticsInitialised() const { return filterStatistics != nullptr; }

		Ptr getOrCreateProcessorFilterStatistics()
		{
			if(filterStatistics == nullptr)
				filterStatistics = createFilterStatistics();

			return filterStatistics;
		}

	protected:

		virtual Ptr createFilterStatistics() = 0;

	private:

		Ptr filterStatistics;
	};

	ProcessorFilterStatistics(Processor* p_):
	  p(p_)
	{};

	operator bool() const { return p.get() != nullptr; }

	bool isValid() const { return *this; }

	virtual ~ProcessorFilterStatistics() = default;

	virtual int getNumFilterBands() const = 0;
	virtual void removeFilterBand(int index) = 0;
	virtual void addFilterBand(int index, double freq, double gain) = 0;
	virtual FilterDataObject::CoefficientData getCoefficients(int index) = 0;
	virtual hise::SimpleReadWriteLock& getEqLock() const = 0;
	virtual String getTypeString(int filterIndex) const = 0;
	void sendBroadcasterMessage(const String& type, const var& value, NotificationType n=sendNotificationSync)
	{
		eventBroadcaster.sendMessage(n, type, value);
	}

	virtual SimpleRingBuffer::Ptr getFFTBuffer() = 0;
	virtual int getAttributeIndex(int filterIndex, BandParameter bp) const = 0;
	virtual StringArray getFilterModes() const = 0;

	bool isValidParameterIndex(int filterIndex, BandParameter bp) const
	{
		if(isPositiveAndBelow(filterIndex, getNumFilterBands()))
		{
			auto idx = getAttributeIndex(filterIndex, bp);
			return idx != - 1;
		};

		return false;
	}

	float getBandAttribute(int filterIndex, BandParameter bp) const
	{
		auto index = getAttributeIndex(filterIndex, bp);

		if(index != -1)
		{
			return p->getAttribute(index);
		}
		else
		{
			switch(bp)
			{
			case BandParameter::Gain: return 0.0f;
			case BandParameter::Q:	   return 1.0f;
			case BandParameter::Enabled: return 1.0f;
			default:
				// Type and Frequency don't have a sensible default
				jassertfalse;
			}
			return 0.0f;
		}
	}

	void setBandAttribute(int filterIndex, BandParameter bp, float newValue, NotificationType n)
	{
		auto idx = getAttributeIndex(filterIndex, bp);

		if(idx != -1)
			p->setAttribute(idx, newValue, n);
	}

	virtual BandParameter getParameterForDragAction(DragAction d) const
	{
		switch(d)
		{
		case DragAction::DragX: return Freq;
		case DragAction::DragY: return Gain;
		case DragAction::ShiftDrag: return Q;
		case DragAction::DoubleClick: return Enabled;
		case DragAction::RightClick: return numBandParameters;
		default: jassertfalse; return numBandParameters;
		}
	}

	bool isEnabled(int filterIndex) const
	{
		if(isPositiveAndBelow(filterIndex, getNumFilterBands()))
		{
			if(isValidParameterIndex(filterIndex, BandParameter::Enabled))
				return getBandAttribute(filterIndex, BandParameter::Enabled) > 0.5f;

			// filterIndex > numBands but no enabled parameter => enabled
			return true;
		}

		return false;
	}

	void toggleSpectrumAnalyser()
	{
		if(auto fftBuffer = getFFTBuffer())
		{
			fftBuffer->setActive(!fftBuffer->isActive());
			sendBroadcasterMessage("FFTEnabled", fftBuffer->isActive());
		}
	}

	LambdaBroadcaster<String, var> eventBroadcaster;

	WeakReference<Processor> p;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorFilterStatistics);
};

/** Subclass from this interface if your module provides a custom filter statistics defined by a JSON object. */
struct ProcessorWithCustomFilterStatistics: public ProcessorFilterStatistics::Holder
{
	struct CustomFilterStats: public ProcessorFilterStatistics
	{
		CustomFilterStats(Processor* p, const var& filterStats):
		  ProcessorFilterStatistics(p)
		{
			setFilterStats(filterStats);
		}

		void setFilterStats(const var& filterStats)
		{
			if(auto obj = filterStats.getDynamicObject())
			{
				properties = obj->getProperties();

				auto l = properties["TypeList"];

				types.clear();

				if(l.isArray())
				{
					for(auto& v: *l.getArray())
						types.add(v.toString());
				}

				auto po = properties["ParameterOrder"];

				parameters.clear();

				if(po.isArray())
				{
					for(auto& v: *po.getArray())
						parameters.add(getBandParameterFromName(v.toString()));
				}

				if(auto obj = properties["DragActions"].getDynamicObject())
				{
					const auto& dobj = obj->getProperties();

					auto dx = dobj.getWithDefault("DragX", "Freq").toString();
					auto dy = dobj.getWithDefault("DragY", "Gain").toString(); 
					auto sd = dobj.getWithDefault("ShiftDrag", "Q").toString();
					auto dc = dobj.getWithDefault("DoubleClick", "Enabled").toString();
					auto rc = dobj.getWithDefault("RightClick", "").toString();

					dragActions.clear();

					dragActions.push_back({ DragAction::DragX, getBandParameterFromName(dx) });
					dragActions.push_back({ DragAction::DragY, getBandParameterFromName(dy) });
					dragActions.push_back({ DragAction::ShiftDrag, getBandParameterFromName(sd) });
					dragActions.push_back({ DragAction::DoubleClick, getBandParameterFromName(dc) });
					dragActions.push_back({ DragAction::RightClick, getBandParameterFromName(rc) });
				}
			}
		}

		static BandParameter getBandParameterFromName(const String& n)
		{
			static const StringArray pnames({
				"Gain",
				"Freq",
				"Q",
				"Enabled",
				"Type"
			});

			auto idx = pnames.indexOf(n);

			if(idx != -1)
				return (BandParameter)idx;

			return BandParameter::numBandParameters;
		}

		int getNumFilterBands() const override { return properties.getWithDefault("NumFilterBands", 0); }
		void removeFilterBand(int ) override { jassertfalse; }
		void addFilterBand(int , double , double ) override { jassertfalse; }
		FilterDataObject::CoefficientData getCoefficients(int index) override
		{
			auto fc = ExternalData::DataType::FilterCoefficients;

			auto slot = (int)properties.getWithDefault("FilterDataSlot", 0);

			if(auto ed = dynamic_cast<ExternalDataHolder*>(p.get()))
			{
				if(isPositiveAndBelow(slot, ed->getNumDataObjects(fc)))
				{
					if(isPositiveAndBelow(index, ed->getFilterData(slot)->getNumCoefficients()))
						return ed->getFilterData(slot)->getCoefficients(index);
				}
			}

			return { {}, 0, nullptr }; 
		}

		mutable SimpleReadWriteLock unusedLock;

		hise::SimpleReadWriteLock& getEqLock() const override { return unusedLock; }

		String getTypeString(int filterIndex) const override
		{
			return types[filterIndex];
		}

		StringArray getFilterModes() const override { return types; }

		SimpleRingBuffer::Ptr getFFTBuffer() override
		{
			auto idx = properties.getWithDefault("FFTDisplayBufferIndex", -1);

			if(auto ed = dynamic_cast<ExternalDataHolder*>(p.get()))
			{
				auto numDisplayBuffers = ed->getNumDataObjects(ExternalData::DataType::DisplayBuffer);

				if(isPositiveAndBelow(idx, numDisplayBuffers))
				{
					auto rb = ed->getDisplayBuffer(idx);

					auto classIndex = rb->getPropertyObject()->getClassIndex();

					if(classIndex == scriptnode::analyse::Helpers::FFT::PropertyIndex)
					{
						return rb;
					}
				}
			}

			return nullptr;
		}

		int getAttributeIndex(int filterIndex, BandParameter bp) const
		{
			auto offset = (int)properties.getWithDefault("FirstBandOffset", 0);
			auto pidx = parameters.indexOf(bp);

			if(pidx == -1)
				return -1;

			return offset + filterIndex * parameters.size() + pidx;
		}

		BandParameter getParameterForDragAction(DragAction d) const override
		{
			for(const auto& da: dragActions)
			{
				if(da.first == d)
					return da.second;
			}
			
			return BandParameter::numBandParameters;
		}

		static var getDefaultProperties()
		{
			DynamicObject::Ptr p = new DynamicObject();

			Array<var> types;
			types.add("Low Pass");
			types.add("High Pass");
			types.add("Low Shelf");
			types.add("High Shelf");
			types.add("Peak");

			Array<var> parameterOrder;

			parameterOrder.add("Gain");
			parameterOrder.add("Freq");
			parameterOrder.add("Q");
			parameterOrder.add("Enabled");
			parameterOrder.add("Type");

			DynamicObject::Ptr da = new DynamicObject();

			da->setProperty("DragX", "Freq");
			da->setProperty("DragY", "Gain");
			da->setProperty("ShiftDrag", "Q");
			da->setProperty("DoubleClick", "Enabled");
			da->setProperty("RightClick", "");

			p->setProperty("NumFilterBands", 1);
			p->setProperty("FilterDataSlot", 0);
			p->setProperty("FirstBandOffset", 0);
			p->setProperty("TypeList", var(types));
			p->setProperty("ParameterOrder", parameterOrder);
			p->setProperty("FFTDisplayBufferIndex", -1);
			p->setProperty("DragActions", da.get());

			return var(p.get());
		}

		NamedValueSet properties;
		StringArray types;
		Array<BandParameter> parameters;
		std::vector<std::pair<DragAction, BandParameter>> dragActions;
	};

	ProcessorWithCustomFilterStatistics():
	  filterStatisticJson(CustomFilterStats::getDefaultProperties())
	{};

	void handleFilterStatisticUpdate()
	{
		if(isFilterStatisticsInitialised())
		{
			auto p = dynamic_cast<Processor*>(this);
			p->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
		}
	}

	void setFilterStatistics(const var& newData)
	{
		filterStatisticJson = newData;

		if(isFilterStatisticsInitialised())
			dynamic_cast<CustomFilterStats*>(getOrCreateProcessorFilterStatistics().get())->setFilterStats(newData);
	}

	ProcessorFilterStatistics::Ptr createFilterStatistics() override
	{
		// must be set before calling this method...
		jassert(filterStatisticJson.getDynamicObject() != nullptr);
		return new CustomFilterStats(dynamic_cast<Processor*>(this), filterStatisticJson.getDynamicObject());
	}

	var getFilterStatisticsJSON() const { return filterStatisticJson; }

	private:

	var filterStatisticJson;
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

            virtual ~Listener();;
            
			virtual void processorChanged(EventType t, Processor* p) = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		// ================================================================================================================

		virtual ~Handler();;

		/** Adds a new processor to the chain. It must be owned by the chain. */
		virtual void add(Processor *newProcessor, Processor *siblingToInsertBefore) = 0;

		/** Deletes a processor from the chain. */
		virtual void remove(Processor *processorToBeRemoved, bool deleteProcessor=true) = 0;

		/** Returns the processor at the index. */
		virtual Processor *getProcessor(int processorIndex) = 0;

		virtual const Processor *getProcessor(int processorIndex) const = 0;

		/** Overwrite this method and implement a move operation. */
		virtual void moveProcessor(Processor* /*processorInChain*/, int /*delta*/);;

		/** Returns the amount of processors. */
		virtual int getNumProcessors() const = 0;

		/** Deletes all Processors in the Chain. */
		virtual void clear() = 0;

		void clearAsync(Processor* parent);

		void addListener(Listener* l);

		void removeListener(Listener* l);

		void addPostEventListener(Listener* l);

		void removePostEventListener(Listener* l);

		void notifyListeners(Listener::EventType t, Processor* p);

		void notifyPostEventListeners(Listener::EventType t, Processor* p);

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

	virtual ~Chain();;

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
		ProcessorEntry(const Identifier t, const String &n):
		  type(t),
		  name(n),
		  index(-1)
		{}

		ProcessorEntry() {};

		Identifier type;
		String name;
		int index = 0;
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
