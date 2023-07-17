/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once

namespace scriptnode
{
namespace data
{

namespace embedded
{
struct multichannel_data
{
	multichannel_data& data = *this;

	multichannel_data() :
		buffer(new MultiChannelAudioBuffer())
	{};

	virtual ~multichannel_data() {};

	virtual double getSamplerate() const = 0;
	virtual int getNumChannels() const = 0;
	virtual int getNumSamples() const = 0;
	virtual const float* getChannelData(int index) = 0;

	MultiChannelAudioBuffer::Ptr buffer;
	float* channelData[NUM_MAX_CHANNELS];
};
}
}
}

namespace snex {

using namespace juce;
using namespace hise;
using namespace Types;


    struct InitialiserList : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<InitialiserList>;
        
        struct ChildBase : public ReferenceCountedObject
        {
            using List = ReferenceCountedArray<ChildBase>;
            
            virtual bool getValue(VariableStorage& v) const = 0;
            
            virtual InitialiserList::Ptr createChildList() const = 0;
            
            virtual ~ChildBase() {};
            
            virtual bool forEach(const std::function<bool(ChildBase*)>& func)
            {
                return func(this);
            }
            
            virtual juce::String toString() const = 0;
        };
        
        juce::String toString() const;
        
        static Ptr makeSingleList(const VariableStorage& v)
        {
            InitialiserList::Ptr singleList = new InitialiserList();
            singleList->addImmediateValue(v);
            return singleList;
        }
        
        void addChildList(InitialiserList::Ptr other)
        {
            addChild(new ListChild(other->root));
        }
        
        void addImmediateValue(const VariableStorage& v)
        {
            addChild(new ImmediateChild(v));
        }
        
        void addChild(ChildBase* b)
        {
            root.add(b);
        }
        
        ReferenceCountedObject* getExpression(int index);
        
        Result getValue(int index, VariableStorage& v);
        
        Array<VariableStorage> toFlatConstructorList() const
        {
            Array<VariableStorage> list;
            
            for (auto c : root)
            {
                VariableStorage v;
                
                if (c->getValue(v))
                    list.add(v);
            }
            
            return list;
        }
        
        Ptr createChildList(int index)
        {
            if (auto child = root[index])
            {
                return child->createChildList();
            }
            
            return nullptr;
        }
        
        Ptr getChild(int index)
        {
            if (auto cb = dynamic_cast<ListChild*>(root[index].get()))
            {
                Ptr p = new InitialiserList();
                p->root.addArray(cb->list);
                return p;
            }
            
            return nullptr;
        }
        
        int size() const
        {
            return root.size();
        }
        
        bool forEach(const std::function<bool(ChildBase*)>& func) const
        {
            for (auto l : root)
            {
                if (l->forEach(func))
                    return true;
            }
            
            return false;
        }
        
        struct ExpressionChild;
        
        
        
        struct MemberPointer;
        
        /** @internal This is used when a struct is being initialised by a externally defined C++ struct
         (via placement new) and has the sole purpose of avoiding compile warnings...
         */
        struct ExternalInitialiser : public ChildBase
        {
            ExternalInitialiser():
            ChildBase()
            {}
            
            juce::String toString() const override
            {
                return "external_class";
            }
            
            bool getValue(VariableStorage& ) const override
            {
                return true;
            }
            
            InitialiserList::Ptr createChildList() const override
            {
                InitialiserList::Ptr n = new InitialiserList();
                n->addChild(new ExternalInitialiser());
                return n;
            }
        };
        
    private:
        
        struct ImmediateChild : public ChildBase
        {
            ImmediateChild(const VariableStorage& v_) :
            v(v_)
            {};
            
            ~ImmediateChild()
            {
                
            }
            
            bool getValue(VariableStorage& target) const override
            {
                target = v;
                return true;
            }
            
            InitialiserList::Ptr createChildList() const override
            {
                InitialiserList::Ptr n = new InitialiserList();
                n->addImmediateValue(v);
                return n;
            }
            
            juce::String toString() const override
            {
                return Types::Helpers::getCppValueString(v);
            }
            
            VariableStorage v;
        };
        
        struct ListChild : public ChildBase
        {
            ListChild(const ChildBase::List& l) :
            list(l)
            {};
            
            virtual InitialiserList::Ptr createChildList() const override
            {
                InitialiserList::Ptr n = new InitialiserList();
                n->root.addArray(list);
                return n;
            }
            
            bool getValue(VariableStorage& target) const override
            {
                if (list.size() == 1)
                    return list[0]->getValue(target);
                
                return false;
            }
            
            bool forEach(const std::function<bool(ChildBase *)>& func) override
            {
                for (auto l : list)
                {
                    if (l->forEach(func))
                        return true;
                }
                
                return false;
            }
            
            juce::String toString() const override
            {
                juce::String s;
                s << "{ ";
                
                int index = 0;
                
                for (auto l : list)
                {
                    s << l->toString();
                    
                    if(++index < list.size())
                        s << ", ";
                }
                
                s << " }";
                return s;
            }
            
            ChildBase::List list;
        };
        
        ChildBase::List root;
    };

template <int NumChannels> struct SampleData
{
	bool isEmpty() const
	{
		return data[0].size() == 0;
	}

	double getPitchFactor() const
	{
		return hmath::pow(2.0, (noteNumber - rootNote) / 12.0);
	}

	void clear()
	{
		for (auto& bl : data)
			bl.referToNothing();
		rootNote = -1;
	}

	template <typename IndexType> span<float, NumChannels> operator[](const IndexType& d) const
	{
		span<float, NumChannels> fd;

		if (!isEmpty())
		{
			for (int i = 0; i < NumChannels; i++)
				fd[i] = data[i][d];
		}

		return fd;
	}

	void fromHiseEvent(const HiseEvent& e)
	{
		clear();
		noteNumber = e.getNoteNumberIncludingTransposeAmount();
		velocity = e.getVelocity();
	}

	void setLoopRange(Range<int> lr)
	{
		loopRange[0] = lr.getStart();
		loopRange[1] = lr.getEnd();
	}

	double rootNote = -1.0;
	double noteNumber = 0.0;
	span<int, 2, 8> loopRange;
	int velocity = 0;
	int roundRobin = 1;
	span<block, NumChannels> data;
};

using StereoSample = SampleData<2>;
using MonoSample = SampleData<1>;



/** HISE has a few complex inbuilt data structures that are connected to various UI elements (Tables, SliderPacks) etc.
    @ingroup snex_data_structures
 
    The access to those classes and their underlying data is handled by this class.
 
    This class is used as parameter in the `setExternalData` callback, which will be called whenever something changes.
 
    It's usually good practice to keep a copy of the last ExternalData around in your class to refer blocks to it or synchronize
    access using a DataReadLock or - more unusual - DataWriteLock.
 */
struct ExternalData
{
    /** There are different data types that are supported. */
	enum class DataType: int
	{
		Table, ///< the data of a Table object (a look up table with 512 float numbers)
		SliderPack, ///< the data of a SliderPack (a resizable array of float numbers)
		AudioFile, ///< a multichannel audio file with some metadata (loop points, samplerate).
		FilterCoefficients, ///< filter coefficients that can be used for displaying purposes
		DisplayBuffer, ///< a FIFO buffer that can be used for analysing / visualisations
		numDataTypes,
		ConstantLookUp
	};

	ExternalData():
		dataType(DataType::numDataTypes),
		data(nullptr),
		numSamples(0),
		numChannels(0),
		obj(nullptr),
		sampleRate(0.0)
	{}


	ExternalData(const ExternalData& d) = default;

	struct Factory : public hise::PathFactory
	{
		String getId() const override { return {}; };

		Path createPath(const String& id) const override;
	};

	ExternalData(ComplexDataUIBase* b, int absoluteIndex);

	ExternalData(scriptnode::data::embedded::multichannel_data& d, DataType type);

	/** Creates an external data object from a constant value class. */
	template <typename T> ExternalData(T& other, DataType type):
		dataType(type),
		obj(nullptr)
	{
		data = other.begin();
		numSamples = other.size();
		numChannels = 1;
	}

	static String getDataTypeName(DataType t, bool plural=false);
	static Identifier getNumIdentifier(DataType t);

    static void forEachType(const std::function<void(DataType)>& f);

    /** assigns a block container to the data type. If the external data is pointing to an audio file you can specify the channelIndex. */
	void referBlockTo(block& b, int channelIndex) const;

    /** This will send a update message to the UI so you can implement eg. a playback position. */
	void setDisplayedValue(double valueToDisplay);
	
	static void setDisplayValueStatic(void* externalObj, double valueToDisplay)
	{
		static_cast<ExternalData*>(externalObj)->setDisplayedValue(valueToDisplay);
	}

	template <class B, class D> static constexpr bool isSameOrBase()
	{
		return std::is_same<B, D>() || std::is_base_of<B, D>();
	}

	static DataType getDataTypeForId(const Identifier& id, bool plural=false);

	template <class DataClass> static constexpr DataType getDataTypeForClass()
	{
		if (isSameOrBase<hise::Table, DataClass>())
			return DataType::Table;

		if (isSameOrBase<SliderPackData, DataClass>())
			return DataType::SliderPack;

		if (isSameOrBase<MultiChannelAudioBuffer, DataClass>())
			return DataType::AudioFile;

		if (isSameOrBase<FilterDataObject, DataClass>())
			return DataType::FilterCoefficients;

		if (isSameOrBase<SimpleRingBuffer, DataClass>())
			return DataType::DisplayBuffer;

		return DataType::numDataTypes;
	}

    /** Returns true if there is no data associated with this object. */
	bool isEmpty() const
	{
		return dataType == DataType::numDataTypes || numSamples == 0 || obj == nullptr || numChannels == 0 || data == nullptr;
	}

	bool isNotEmpty() const
	{
		return !isEmpty();
	}

	AudioSampleBuffer toAudioSampleBuffer() const;

	static DataType getDataTypeForClass(ComplexDataUIBase* d);

	static ComplexDataUIBase* create(DataType t)
	{
		if (t == DataType::Table)
			return new SampleLookupTable();
		if (t == DataType::SliderPack)
			return new SliderPackData();
		if (t == DataType::AudioFile)
			return new MultiChannelAudioBuffer();
		if (t == DataType::FilterCoefficients)
			return new FilterDataObject();
		if (t == DataType::DisplayBuffer)
			return new SimpleRingBuffer();

		return nullptr;
	}

    /** If the ExternalData object points to audio data this specifies whether it's a multisample set or a single sample. */
	bool isXYZ() const { return isXYZAudioData; };

	template <int NumChannels> static int getXYZData(const void* obj_, SampleData<NumChannels>* dataToFill, const HiseEvent& e)
	{
		dataToFill->fromHiseEvent(e);

		auto ed = static_cast<const ExternalData*>(obj_);

		if (ed->obj != nullptr)
		{
			if (auto st = SimpleReadWriteLock::ScopedTryReadLock(ed->obj->getDataLock()))
			{
				if (ed->isXYZ())
				{
					auto xyzData = static_cast<MultiChannelAudioBuffer::XYZItem*>(ed->data);

					int n = (int)dataToFill->noteNumber;
					int v = dataToFill->velocity;
					int r = dataToFill->roundRobin;

					for (int i = 0; i < ed->numSamples; i++)
					{
						if (xyzData[i].matches(n, v, r))
						{
							dataToFill->rootNote = (double)xyzData[i].root;

							auto ptrs = xyzData[i].data->buffer.getArrayOfWritePointers();
							auto nc = xyzData[i].data->buffer.getNumChannels();
							auto ns = xyzData[i].data->buffer.getNumSamples();

							dataToFill->setLoopRange(xyzData[i].data->loopRange);

							for (int c = 0; c < NumChannels; c++)
							{
								auto srcIndex = c * (nc > 1);
								dataToFill->data[c].referToRawData(ptrs[srcIndex], ns);
							}

							return 1;
						}
					}
				}
				else
				{
					dataToFill->rootNote = dataToFill->noteNumber;

					for (int c = 0; c < NumChannels; c++)
						ed->referBlockTo(dataToFill->data[c], c);

					auto lr = static_cast<MultiChannelAudioBuffer*>(ed->obj)->getLoopRange(true);
					dataToFill->setLoopRange(lr);

					return 1;
				}
			}
		}

		return 0;
	}

	int getStereoSample(StereoSample& d, const HiseEvent& e) const
	{
		return getXYZData<2>(this, &d, e);
	}

	int getMonoSample(MonoSample& d, const HiseEvent& e) const
	{
		return getXYZData<1>(this, &d, e);
	}

	static ComplexDataUIBase::EditorBase* createEditor(ComplexDataUIBase* dataObject);
	
    /** the data type of the target data. */
	DataType dataType = DataType::numDataTypes;
    
    /** The number of samples (= the length of the float array) */
	int numSamples = 0;
    
    /** The number of audio channels (usually 1 if not an audio file). */
	int numChannels = 0;
    
    /** Specifies whether the data points to a multichannel sample set. */
	int isXYZAudioData = 0;
    
    /** An untyped pointer to the actual data. */
	void* data = nullptr;
    
    /** A pointer to the complex data object that is used for eg. UI updates. */
	hise::ComplexDataUIBase* obj = nullptr;
    
    /** the samplerate (if the data is an audio file). */
	double sampleRate = 0.0;
	
};

/** @internal A interface class that handles the communication between a SNEX node and externally defined complex data types of HISE.

	This is the lowest common denominator for all the different data management situations in HISE and is used
	by the ExternalDataProviderBase to fetch the data required by the internal processing.

*/
struct ExternalDataHolder
{
	virtual ~ExternalDataHolder() {};

	/** Converts any data type to a float array for the given index.
	
		Be aware that the index is the index of each slot, not the total index.
	*/
	ExternalData getData(ExternalData::DataType t, int index);

	/** Converts the given index of each data type to an absolute index. */
	int getAbsoluteIndex(ExternalData::DataType t, int dataIndex) const;

	virtual int getNumDataObjects(ExternalData::DataType t) const = 0;

	virtual Table* getTable(int index) = 0;
	virtual SliderPackData* getSliderPack(int index) = 0;
	virtual MultiChannelAudioBuffer* getAudioFile(int index) = 0;
	virtual FilterDataObject* getFilterData(int index) = 0;
	virtual SimpleRingBuffer* getDisplayBuffer(int index) = 0;

    virtual void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex)
    {
        // this data holder doesn't support linking
        jassertfalse;
    }
    
	ComplexDataUIBase* getComplexBaseType(ExternalData::DataType t, int index);

	/** Override this method and remove the object in question. Return true if successful. */
	virtual bool removeDataObject(ExternalData::DataType t, int index) = 0;

	/** Call this to clear all data objects. */
	void clearAllDataObjects()
	{
		ExternalData::forEachType([this](ExternalData::DataType t)
		{
			int numObjects = getNumDataObjects(t);

			for (int i = 0; i < numObjects; i++)
				removeDataObject(t, i);
		});
	}

	JUCE_DECLARE_WEAK_REFERENCEABLE(ExternalDataHolder);
};

/** @internal This interface class adds the ability to send a "force update" message
    to all its registered listeners. 
*/
struct ExternalDataHolderWithForcedUpdate : public ExternalDataHolder
{
	virtual ~ExternalDataHolderWithForcedUpdate() {};

	struct ForcedUpdateListener
	{
		virtual ~ForcedUpdateListener() 
		{
		};

		virtual void forceRebuild(ExternalData::DataType dt, int index) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(ForcedUpdateListener);
	};

	void addForcedUpdateListener(ForcedUpdateListener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeForcedUpdateListener(ForcedUpdateListener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void sendForceUpdateMessage(ForcedUpdateListener* listenerToExclude, ExternalData::DataType dt, int index)
	{
		for (auto l : listeners)
		{
			if (l != nullptr && l != listenerToExclude)
				l->forceRebuild(dt, index);
		}
	}

private:

	Array<WeakReference<ForcedUpdateListener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ExternalDataHolderWithForcedUpdate);
};



/** @internal A base class that fetches the data from the ExternalDataHolder and forwards it to
    its inner structure (either JIT compiled, hardcoded C++ or interpreted nodes)
*/
struct ExternalDataProviderBase
{
	/** Override this method and return the amount of data types that this provider requires.

		It will use these values at initialisation.
	*/
	virtual int getNumRequiredDataObjects(ExternalData::DataType t) const = 0;

	/** Override this method and forward the block with the given index to the internal data. */
	virtual void setExternalData(const ExternalData& data, int index) = 0;

	virtual ~ExternalDataProviderBase() {};

	void setExternalDataHolder(ExternalDataHolder* newHolder)
	{
		externalDataHolder = newHolder;
		initExternalData();
	}
	
	void initExternalData();

protected:

	WeakReference<ExternalDataHolder> externalDataHolder;
};


}


namespace scriptnode
{




namespace data
{


/** @internal Subclass this when you want to show a UI for the given data. */
struct base
{
	

	virtual ~base() {};

	/** This can be used to connect the UI to the data. */
	hise::ComplexDataUIBase* getUIPointer() { return externalData.obj; }

	virtual void setExternalData(const snex::ExternalData& d, int index)
	{
		// This function must always be called while the writer lock is active
		jassert(d.isEmpty() || d.obj->getDataLock().writeAccessIsLocked() || d.obj->getDataLock().writeAccessIsSkipped());

		

		externalData = d;

		
	}

	snex::ExternalData externalData;

	//JUCE_DECLARE_WEAK_REFERENCEABLE(base);
};

}

/** Use this in order to lock the access to the external data.
    @ingroup snex_helpers
 
    */
struct DataReadLock
{
	DataReadLock(data::base* d, bool tryRead=false) :
		lockToUse(d->externalData.obj != nullptr ? &d->externalData.obj->getDataLock() : nullptr)
	{
		if (lockToUse != nullptr)
		{
			if (tryRead)
				holdsLock = lockToUse->enterReadLock();
			else
				holdsLock = lockToUse->enterTryReadLock();
		}
	}

    /** Create a data lock from the external data object. */
	DataReadLock(snex::ExternalData& d, bool tryRead=false) :
		lockToUse(d.obj != nullptr ? &d.obj->getDataLock() : nullptr)
	{
		if (lockToUse != nullptr)
		{
			if (tryRead)
				holdsLock = lockToUse->enterReadLock();
			else
				holdsLock = lockToUse->enterTryReadLock();
		}
	}

    /** Returns true if the lock could be acquired (if `tryRead` is false, then this will always return true).
     
        You can use this for a scoped based approach to data synchronisation:
     
            
            if(auto lock = DataReadLock(ed, true))
            {
                doSomethingWhileLocked();
            }
            
     */
	operator bool() const { return isLocked(); }

	bool isLocked() const { return lockToUse != nullptr && holdsLock; };

	~DataReadLock()
	{
		if (lockToUse != nullptr)
			lockToUse->exitReadLock(holdsLock);
	}

	SimpleReadWriteLock* lockToUse;
	bool holdsLock = false;
};

struct DataTryReadLock : hise::SimpleReadWriteLock::ScopedTryReadLock
{
	DataTryReadLock(data::base* d) :
		SimpleReadWriteLock::ScopedTryReadLock(d->externalData.obj != nullptr ? d->externalData.obj->getDataLock() : dummy)
	{}

	DataTryReadLock(snex::ExternalData& d) :
		SimpleReadWriteLock::ScopedTryReadLock(d.obj != nullptr ? d.obj->getDataLock() : dummy)
	{}

	operator bool() const { return this->ok(); }

	SimpleReadWriteLock dummy;
};

/** Use this in order to lock the access to the external data. */
struct DataWriteLock : hise::SimpleReadWriteLock::ScopedWriteLock
{
	DataWriteLock(data::base* d) :
		SimpleReadWriteLock::ScopedWriteLock(d->externalData.obj != nullptr ? d->externalData.obj->getDataLock() : dummy)
	{}

	SimpleReadWriteLock dummy;
};

namespace data
{

struct filter_base : public data::base,
	public FilterDataObject::Broadcaster
{
	virtual ~filter_base() {};

	virtual IIRCoefficients getApproximateCoefficients() const = 0;

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		deregisterAtObject(this->externalData.obj);
		base::setExternalData(d, index);
		registerAtObject(this->externalData.obj);

		if (auto o = dynamic_cast<FilterDataObject*>(d.obj))
			o->setCoefficients(this, getApproximateCoefficients());
	}
};

#define SNEX_THROW_IF_MULTIPLE_WRITERS 0

template <bool EnableBuffer> struct display_buffer_base : public base,
														  public SimpleRingBuffer::WriterBase
{
	virtual ~display_buffer_base() 
	{
		setExternalData({}, -1);
	};

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		if constexpr (EnableBuffer)
		{
			if (rb != nullptr && rb->getCurrentWriter() == this)
				rb->setCurrentWriter(nullptr);
		}

		base::setExternalData(d, index);

		if constexpr (EnableBuffer)
		{
            rb = dynamic_cast<SimpleRingBuffer*>(d.obj);
            
			if (rb != nullptr)
			{
#if SNEX_THROW_IF_MULTIPLE_WRITERS
				if (rb->getCurrentWriter() != nullptr)
				{
					scriptnode::Error e;
					e.error = Error::RingBufferMultipleWriters;
					throw e;
				}
#endif

				rb->setCurrentWriter(this);
				registerPropertyObject(rb);
					
				if (lastSpecs.sampleRate > 0.0)
					prepare(lastSpecs);
			}
		}
	}

	virtual void prepare(snex::Types::PrepareSpecs ps)
	{
		lastSpecs = ps;

		if (rb != nullptr)
		{
			auto numSamples = rb->getReadBuffer().getNumSamples();

			ignoreUnused(numSamples);
			
#if !HI_EXPORT_AS_PROJECT_DLL
			// We can only reallocate here if we're not in the dll memory space
			rb->setRingBufferSize(ps.numChannels, numSamples);
#endif
			rb->setSamplerate(ps.sampleRate);
		}
	}

	void updateBuffer(double v, int numSamples)
	{
		if constexpr (EnableBuffer)
		{
			DataReadLock sl(this);

			auto shouldWrite = rb != nullptr && rb->isActive();

#if !SNEX_THROW_IF_MULTIPLE_WRITERS
			shouldWrite |= rb != nullptr &&  rb->getCurrentWriter() == this;
#endif

			if(shouldWrite)
				rb->write(v, numSamples);
		}
	}

	static constexpr bool isRingBufferEnabled() { return EnableBuffer; }

	virtual void registerPropertyObject(SimpleRingBuffer::Ptr rb) 
	{
		rb->registerPropertyObject<ModPlotter::ModPlotterPropertyObject>();
	};

	SimpleRingBuffer::Ptr rb = nullptr;
	snex::Types::PrepareSpecs lastSpecs;
};


using namespace snex;

namespace pimpl
{

struct base
{
	virtual ~base() {};

	virtual void initialise(NodeBase* n) {};
};


/** @internal This is used as interface class for the wrap::data template in order to access the base. */
struct provider_base
{
	virtual ~provider_base() {};

	virtual base* getDataObject() = 0;
};

template <int Index, ExternalData::DataType DType> struct index_type_base: public base
{
private:
	static constexpr int getNum(ExternalData::DataType t) { return t == DType ? 1 : 0; }

public:

	static constexpr int NumTables = getNum(ExternalData::DataType::Table);
	static constexpr int NumSliderPacks = getNum(ExternalData::DataType::SliderPack);
	static constexpr int NumAudioFiles = getNum(ExternalData::DataType::AudioFile);
	static constexpr int NumFilters = getNum(ExternalData::DataType::FilterCoefficients);
	static constexpr int NumDisplayBuffers = getNum(ExternalData::DataType::DisplayBuffer);
	
private:
};

template <int Index, ExternalData::DataType DType> struct plain : index_type_base<Index, DType>
{
	// Needs to be there for compatibility with the wrap::init class
	template <typename NodeType> plain(NodeType& n) {};

	template <typename NodeType> void setExternalData(NodeType& n, const ExternalData& b, int index)
	{
		if (index == Index && b.dataType == DType)
			n.setExternalData(b, 0);
	}
};

template <typename T> class has_initialise
{
    typedef char one; struct two { char x[2]; };
    template <typename C> static one test(decltype(&C::initialise));
    template <typename C> static two test(...);
public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

template <typename DataClass, ExternalData::DataType DType> struct embedded : public index_type_base<-1, DType>
{
private:
	static constexpr int getNum(ExternalData::DataType t) { return t == DType ? 1 : 0; }

public:

	static constexpr int NumTables = getNum(ExternalData::DataType::Table);
	static constexpr int NumSliderPacks = getNum(ExternalData::DataType::SliderPack);
	static constexpr int NumAudioFiles = getNum(ExternalData::DataType::AudioFile);
	static constexpr int NumFilters = getNum(ExternalData::DataType::FilterCoefficients);
	static constexpr int NumDisplayBuffers = getNum(ExternalData::DataType::DisplayBuffer);

	template <typename NodeType> embedded(NodeType& n) 
	{
		ExternalData d(obj.data, DType);
		
		if (d.obj != nullptr)
		{
			SimpleReadWriteLock::ScopedWriteLock sl(d.obj->getDataLock());
			n.setExternalData(d, 0);
		}
		else
			n.setExternalData(d, 0);
	};

	void initialise(NodeBase* n) override
	{
		if constexpr (has_initialise<DataClass>::value)
			obj.initialise(n);
	}

	template <typename NodeType> void setExternalData(NodeType& , const ExternalData& , int )
	{
		
	}

	DataClass obj;
};
struct example_matrix
{
	static constexpr int NumSliderPacks = 1;
	static constexpr int NumTables = 2;
	static constexpr int NumAudioFiles = 3;
	static constexpr int NumFilters = 0;
	static constexpr int NumDisplayBuffers = 0;

	static constexpr int matrix[5][3] =
	{
		{ 1000, 0, -1 },  // 0->e[0] || 1->0
		{ 1001, -1, -1 }, // 0->e[1] ||
		{ 2, 2, 1002 },	  // 0->2 || 1->2 || 2->e[2]
	    { -1, -1, -1 },
		{ -1, -1, -1 }
	};

private:

	// Embedded Data ==========================================

	const span<float, 5> d0 = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	const span<float, 5> d1 = { 1.0f, 1.0f, 1.0f, 1.0f, 5.0f };
	const span<float, 5> d2 = { 1.0f, 1.0f, 1.0f, 1.0f, 5.0f };

	// End of embedded Data ===================================

public:

	const span<dyn<float>, 3> embeddedData = { d0, d1, d2 };
};

}

/** @internal This data type will be used when there are multiple data slots used in the node.

	The template argument must be a class with the following properties:

	- static int constants for `NumSliderPacks`, `NumTables` and `NumAudioFiles`
	- a (constexpr) int[3][max(NumSliderPacks, NumTables, NumAudioFiles)] member that contains the index mapping
	- a const span<dyn<float>, NumEmbeddedDataSlots> member called embeddedData that points to
	  static lookup tables

	Take a look at the pimpl::example_matrix above how it works.
*/
template <typename MatrixType> struct matrix: public pimpl::base
{
	static constexpr int NumTables = MatrixType::NumTables;
	static constexpr int NumSliderPacks = MatrixType::NumSliderPacks;
	static constexpr int NumAudioFiles = MatrixType::NumAudioFiles;
	static constexpr int NumFilters = MatrixType::NumFilters;
	static constexpr int NumDisplayBuffers = MatrixType::NumDisplayBuffers;

	static bool getEmbeddedIndex(int& idx)
	{
		static constexpr int EmbeddedOffset = 1000;

		if (idx >= EmbeddedOffset)
		{
			idx -= EmbeddedOffset;
			return true;
		}

		return false;
	}

	static constexpr int getNumSlots(ExternalData::DataType dt)
	{
		if (dt == ExternalData::DataType::Table)
			return MatrixType::NumTables;
		if (dt == ExternalData::DataType::SliderPack)
			return MatrixType::NumSliderPacks;
		if (dt == ExternalData::DataType::AudioFile)
			return MatrixType::NumAudioFiles;
		if (dt == ExternalData::DataType::FilterCoefficients)
			return MatrixType::NumFilters;
		if (dt == ExternalData::DataType::DisplayBuffer)
			return MatrixType::NumDisplayBuffers;

		return 0;
	}

	template <typename NodeType> matrix(NodeType& n)
	{
		ExternalData::forEachType([&](ExternalData::DataType dt)
			{
				for (int i = 0; i < getNumSlots(dt); i++)
				{
					int idx = m.matrix[(int)dt][i];

					if (getEmbeddedIndex(idx))
					{
						ExternalData d(m.embeddedData[idx], dt);
						n.setExternalData(d, i);
					}
				}
			});
	};

	template <typename NodeType> void setExternalData(NodeType& n, const ExternalData& d, int index)
	{
		auto dt = d.dataType;

		for (int i = 0; i < getNumSlots(dt); i++)
		{
			auto idx = m.matrix[(int)dt][i];

			if (getEmbeddedIndex(idx))
				continue;

			if (idx == index)
				n.setExternalData(d, i);
		}
	}

private:

	const MatrixType m;
};

namespace external
{
	template <int Index> using table =			pimpl::plain<Index, ExternalData::DataType::Table>;
	template <int Index> using sliderpack =		pimpl::plain<Index, ExternalData::DataType::SliderPack>;
	template <int Index> using audiofile =		pimpl::plain<Index, ExternalData::DataType::AudioFile>;
	template <int Index> using filter =			pimpl::plain<Index, ExternalData::DataType::FilterCoefficients>;
	template <int Index> using displaybuffer =	pimpl::plain<Index, ExternalData::DataType::DisplayBuffer>;
}

namespace embedded
{
	template <typename DataClass> using table =		  pimpl::embedded<DataClass, ExternalData::DataType::Table>;
	template <typename DataClass> using sliderpack =  pimpl::embedded<DataClass, ExternalData::DataType::SliderPack>;
	template <typename DataClass> using audiofile =   pimpl::embedded<DataClass, ExternalData::DataType::AudioFile>;
}

}
}
