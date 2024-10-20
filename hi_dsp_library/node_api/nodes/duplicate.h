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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce; 
using namespace hise;
using namespace snex;
using namespace snex::Types;

enum class CloneProcessType
{
    Serial,   // Processes the clones serially with the original signal
    Parallel, // Processes the clones with an empty input,
              // then adds the output to the original signal
    Copy,     // Copies the input signal, then processes each clone
              // and adds the output to the original signal
    Dynamic   // Allows a dynamic change for this module
};

namespace wrap
{

struct clone_manager
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void numClonesChanged(int newNumClones) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

    virtual ~clone_manager()
    {
        SimpleReadWriteLock::ScopedWriteLock sl(getCloneResizeLock());
        listeners.clear();
    };
    
	clone_manager(int initialCloneAmount) :
		numClones(initialCloneAmount)
	{};

	virtual void addNumClonesListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
		l->numClonesChanged(numClones);
	}

	void removeNumClonesListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	int getNumClones() const { return numClones; }

    virtual int getTotalNumClones() const = 0;

    void setNumClones(int newSize)
    {
        auto maxClones = getTotalNumClones();
        
        if(maxClones == 0)
            return;
        
        newSize = jlimit(1, maxClones, newSize);
        
        if(getNumClones() != newSize)
        {
            numClones = newSize;
            sendMessageToListeners();
        }
    }
    
	SimpleReadWriteLock& getCloneResizeLock() { return cloneResizeLock; }

protected:

	void sendMessageToListeners()
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
                l->numClonesChanged(numClones);
		}
	}

private:

    hise::SimpleReadWriteLock cloneResizeLock;
	int numClones = 1;
	Array<WeakReference<Listener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(clone_manager);
};



template <typename T> struct cloned_node_reference
{
	using ObjectType = T;

	cloned_node_reference() = default;

	template <int P> auto get()
	{
		auto& o = firstObj->template get<P>();
        using TType = typename std::remove_reference<decltype(o)>::type;
		cloned_node_reference<TType> hn;

		hn.firstObj = &o;
		hn.objectDelta = objectDelta;
        hn.cloneManager = cloneManager;
		
		return hn;
	}

    /** Allows the connection of internal node modulation connections. */
    auto& getParameter()
	{
        return *this;
	}

    auto& getWrappedObject() { return *this; }

    /** Connects each child to the object from the Target instance.
     *  The target instance is always a cloned_node_reference. */
	template <int P, typename Target> void connect(Target& obj)
	{
        // must point to the same clone manager...
        jassert(obj.cloneManager == cloneManager);

        auto numVoices = cloneManager->getTotalNumClones();
        
        for(int i = 0; i < numVoices; i++)
        {
            auto src = getChild(i);
            auto& p = src->getParameter();
            auto& dst = obj[i];

            p.connect<P>(dst);
        }
	}

    template <int P> void setParameter(double v)
    {
        auto numVoices = cloneManager->getNumClones();
        
        for(int i = 0; i < numVoices; i++)
        {
            auto ptr = getChild(i);
            T::template setParameterStatic<P>(ptr, v);
        }
    }
    
	void setExternalData(const ExternalData& cd, int index)
	{
		auto numVoices = cloneManager->getTotalNumClones();

		for (int i = 0; i < numVoices; i++)
		{
			auto ptr = getChild(i);
			ptr->setExternalData(cd, index);
		}
	}

    T* getChild(const int index)
	{
		auto ptr = reinterpret_cast<uint8*>(firstObj);
        ptr += objectDelta * index;
        return reinterpret_cast<T*>(ptr);
	}

    T& operator[](const int index)
    {
	    return *getChild(index);
    }

	T* firstObj = nullptr;
	scriptnode::wrap::clone_manager* cloneManager;
	size_t objectDelta = 0;
};

/** A compile time clone data for exported nodes */
template <typename T, int AllowResizing, int NumClones> struct clone_data
{
    template <bool AllClones> struct Iterator
    {
        Iterator(clone_data& p):
          parent(p)
        {};
        
        T* begin() const
        {
            return const_cast<T*>(parent.data);
        }
        
        T* end() const
        {
            if constexpr (!AllowResizing || AllClones)
                return begin() + NumClones;
            else
                return begin() + parent.getNumClones();
        }
        
        clone_data& parent;
    };
    
    clone_data(clone_manager& m):
      manager(m)
    {
        
    }
    
    using ObjectType = T;
    
    static constexpr int getInitialCloneAmount() { return NumClones; };
    
    int getTotalNumClones() const { return getInitialCloneAmount(); }
    
    int getNumClones() const
    {
        if constexpr (!AllowResizing)
            return NumClones;
        
        return jlimit(1, NumClones, manager.getNumClones());
    }
    
private:
    
    clone_manager& manager;
    T data[NumClones];
};

template <typename DataType, CloneProcessType ProcessType>
    struct clone_base : public clone_manager
{
    using ActiveIterator = typename DataType::template Iterator<false>;
    using AllIterator = typename DataType::template Iterator<true>;
    
    constexpr auto& getObject() { return *this; }
	constexpr const auto& getObject() const { return *this; }

	constexpr auto& getWrappedObject() { return *DataType:: template Iterator<false>(cloneData).begin(); }
	constexpr const auto& getWrappedObject() const { return *DataType:: template Iterator<false>(cloneData).begin(); }

    static constexpr int NumChannels = DataType::ObjectType::NumChannels;
    
	using ObjectType = clone_base;
	using WrappedObjectType = typename DataType::ObjectType::WrappedObjectType;

	clone_base() :
		clone_manager(DataType::getInitialCloneAmount()),
        cloneData(*this)
	{};

    ~clone_base()
    {
        
    }
    
    CloneProcessType getProcessType() const
    {
        if constexpr (ProcessType != CloneProcessType::Dynamic)
            return ProcessType;
        else
            return processType;
    }
    
	void setCloneProcessType(double newType)
	{
		if constexpr (ProcessType == CloneProcessType::Dynamic)
        {
            auto newProcessType = (CloneProcessType)(int)newType;
            
            if (newProcessType != processType)
            {
                processType = newProcessType;
                resetCopyBuffer();
            }
        }
	}

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto typed = static_cast<clone_base*>(obj);

		if constexpr (P == 0)
			typed->setNumClones(v);
		if constexpr (P == 1)
			typed->setCloneProcessType(v);
	}

	SN_PARAMETER_MEMBER_FUNCTION;

	void initialise(NodeBase* n)
	{
        if constexpr (prototypes::check::initialise<typename DataType::ObjectType>::value)
        {
            for(auto& o: AllIterator(cloneData))
                o.initialise(n);
        }
	}

	int getTotalNumClones() const override { return cloneData.getTotalNumClones(); }

	void resetCopyBuffer()
	{
        SimpleReadWriteLock::ScopedWriteLock sl(getCloneResizeLock());

        auto pt = getProcessType();
        
        workBuffer.setSize(0);
        originalBuffer.setSize(0);
        
        if (pt > CloneProcessType::Serial)
            FrameConverters::increaseBuffer(workBuffer, lastSpecs);
        
        if(pt == CloneProcessType::Copy)
            FrameConverters::increaseBuffer(originalBuffer, lastSpecs);
	}

	void prepare(PrepareSpecs ps)
	{
		lastSpecs = ps;
		resetCopyBuffer();

        SimpleReadWriteLock::ScopedReadLock sl(getCloneResizeLock());
        
        for (auto& obj : AllIterator(cloneData))
        {
            obj.prepare(ps);
            obj.reset();
        }
	}

	void reset()
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getCloneResizeLock()))
		{
			for (auto& obj : ActiveIterator(cloneData))
				obj.reset();
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& frameData)
	{
		static_assert(FrameDataType::hasCompileTimeSize(), "Can't use this with dynamic data");

		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getCloneResizeLock()))
		{
            auto pt = getProcessType();
            
            switch(pt)
            {
                case CloneProcessType::Serial:
                {
                    for (auto& obj : ActiveIterator(cloneData))
                        obj.processFrame(frameData);
                    break;
                }
                case CloneProcessType::Parallel:
                {
                    for(auto& obj: ActiveIterator(cloneData))
                    {
                        FrameDataType work;
                        obj.processFrame(work);
                        frameData += work;
                    }
                    break;
                }
                case CloneProcessType::Copy:
                {
                    FrameDataType original = frameData;
                    frameData = {};
                    
                    for(auto& obj: ActiveIterator(cloneData))
                    {
                        FrameDataType work = original;
                        obj.processFrame(work);
                        frameData += work;
                    }
                    break;
                }
                default:
                    jassertfalse;
                    break;
            }
		}
	}

	template <int P> void processSplitFix(ProcessData<P>& d)
	{
		constexpr int NumChannels = P;

        bool shouldCopy = getProcessType() == CloneProcessType::Copy;
        
        if(shouldCopy)
        {
            ProcessDataHelpers<NumChannels>::copyTo(d, originalBuffer);
            
            for(int i = 0; i < d.getNumChannels(); i++)
                FloatVectorOperations::clear(d.getRawDataPointers()[i], d.getNumSamples());
        }
        
		auto wcd = snex::Types::ProcessDataHelpers<NumChannels>::makeChannelData(workBuffer, d.getNumSamples());
        ProcessData<NumChannels> wd(wcd.begin(), d.getNumSamples());
        wd.copyNonAudioDataFrom(d);
        
        auto dPtr = d.getRawDataPointers();
        const auto wPtr = wd.getRawDataPointers();
        
        for(auto& obj: ActiveIterator(cloneData))
        {
            if(shouldCopy)
                FloatVectorOperations::copy(workBuffer.begin(), originalBuffer.begin(), workBuffer.size());
            else
                FloatVectorOperations::clear(workBuffer.begin(), workBuffer.size());
            
            obj.process(wd);

            for (int i = 0; i < NumChannels; i++)
                FloatVectorOperations::add(dPtr[i], wPtr[i], d.getNumSamples());
        }
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getCloneResizeLock()))
		{
            auto pt = getProcessType();
            
            switch(pt)
            {
                case CloneProcessType::Serial:
                {
                    for (auto& obj : ActiveIterator(cloneData))
                        obj.process(d);
                    break;
                }
                case CloneProcessType::Parallel:
                case CloneProcessType::Copy:
                {
                    if constexpr (ProcessDataType::hasCompileTimeSize())
                    {
                        processSplitFix(d);
                    }
                    else
                    {
                        switch (d.getNumChannels())
                        {
                        case 1: processSplitFix(d.template as<ProcessData<1>>()); break;
                        case 2: processSplitFix(d.template as<ProcessData<2>>()); break;
                        case 3: processSplitFix(d.template as<ProcessData<3>>()); break;
                        case 4: processSplitFix(d.template as<ProcessData<4>>()); break;
                        case 6: processSplitFix(d.template as<ProcessData<6>>()); break;
                        case 8: processSplitFix(d.template as<ProcessData<8>>()); break;
                        case 16: processSplitFix(d.template as<ProcessData<16>>()); break;
                        default: jassertfalse;
                        }
                    }
                    
                    break;
                }
                default: jassertfalse;
            }
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getCloneResizeLock()))
		{
			for (auto& obj : ActiveIterator(cloneData))
				obj.handleHiseEvent(e);
		}
	}

	template <int P> auto get()
	{
        AllIterator it(cloneData);
		auto d = it.begin();
		auto& o = d->template get<P>();
		using TType = typename std::remove_reference<decltype(o)>::type;

		cloned_node_reference<TType> hn;

        hn.objectDelta = sizeof(typename DataType::ObjectType);
		hn.cloneManager = dynamic_cast<clone_manager*>(this);
		hn.firstObj = &o;

		return hn;
	}

    DataType cloneData;
	PrepareSpecs lastSpecs;
	
	heap<float> workBuffer;
    heap<float> originalBuffer;
	CloneProcessType processType;
};

}

namespace parameter
{
using SenderType = wrap::clone_manager;

template <class ParameterClass> struct cloned
{
	PARAMETER_SPECS(ParameterType::Clone, 1);

	ParameterClass p;

    ~cloned()
    {
        if(sender != nullptr && parentListener != nullptr)
            sender->removeNumClonesListener(parentListener);
    }
    
	void callEachClone(int index, double v, bool ignoreCurrentNumClones)
	{
		if (sender != nullptr)
		{
			jassert(ignoreCurrentNumClones || isPositiveAndBelow(index, sender->getNumClones()));

			auto thisPtr = (uint8*)firstObj + index * objectDelta;

			p.setObjPtr(thisPtr);
			p.call(v);
		}
	}

	int getNumClones() const
	{
		jassert(sender != nullptr);
		return sender->getNumClones();
	}

	void setParentNumClonesListener(SenderType::Listener* l)
	{
        parentListener = l;
        
		if (sender != nullptr)
			sender->addNumClonesListener(l);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	template <int P, typename CloneRefType> void connect(CloneRefType& t)
	{
        static_assert(std::is_same<typename CloneRefType::ObjectType, typename ParameterClass::TargetType>(), "class mismatch");
        
		sender = t.cloneManager;
		objectDelta = t.objectDelta;
		firstObj = t.firstObj;

        jassert(sender != nullptr);
        
		if (parentListener != nullptr)
			sender->addNumClonesListener(parentListener);
	}

    bool isNormalised = true;
	WeakReference<SenderType> sender = nullptr;
	void* firstObj = nullptr;
	size_t objectDelta;
	WeakReference<SenderType::Listener> parentListener = nullptr;
};

template <class... CloneParameters> struct clonechain : public advanced_tuple<CloneParameters...>
{
	using Type = advanced_tuple<CloneParameters...>;

	PARAMETER_SPECS(ParameterType::CloneChain, 1);

	template <int P> auto& getParameter()
	{
		return *this;
	}

	void* getObjectPtr() { return this; }

	tuple_iterator3(callEachClone, int, index, double, v, bool, ignore);

	void callEachClone(int index, double v, bool ignore)
	{
		call_tuple_iterator3(callEachClone, index, v, ignore);
	}

	tuple_iterator1(setParentNumClonesListener, SenderType::Listener*, l);

	void setParentNumClonesListener(SenderType::Listener* l)
	{
		call_tuple_iterator1(setParentNumClonesListener, l);
	}

	template <int Index, class Target> void connect(const Target& t)
	{
		this->template get<Index>().template connect<0>(t);
	}

	int getNumClones() const
	{
		return this->template get<0>().getNumClones();
	}
};
}




} 
