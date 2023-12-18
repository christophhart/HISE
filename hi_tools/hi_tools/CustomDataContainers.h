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

#ifndef CUSTOMDATACONTAINERS_H_INCLUDED
#define CUSTOMDATACONTAINERS_H_INCLUDED

namespace hise { using namespace juce;

#define UNORDERED_STACK_SIZE NUM_POLYPHONIC_VOICES


#if JUCE_32BIT
#pragma warning(push)
#pragma warning(disable: 4018)
#endif

template <class HeapElementType, class ConstructionDataType> class PreallocatedHeapArray
{
public:

	PreallocatedHeapArray() {};
	~PreallocatedHeapArray()
	{
		clear();
	}

	void operator +=(const ConstructionDataType& d)
	{
		constructionData.add(d);
	}

	void reserve(int numElements)
	{
		constructionData.ensureStorageAllocated(numElements);
	}

	void finalise()
	{
		if (constructionData.isEmpty())
		{
			start = nullptr;
			ende = nullptr;
			length = 0;
		}
		else
		{

			allocate(constructionData.size());
			start = data.get();
			ende = start + length;

			auto ptr = start;

			for (const auto& cd : constructionData)
			{
				new (ptr++) HeapElementType(cd);
			}

			constructionData.clear();
		}

		finalised = true;
	}

	HeapElementType& operator[](int index) const
	{
		jassert(finalised);
		jassert(isPositiveAndBelow(index, length));
		auto ptr = const_cast<HeapElementType*>(start + index);
		return *ptr;
	}

	void clear()
	{
		for (int i = 0; i < length; i++)
		{
			data[i].~HeapElementType();
		}

		data.free();
		length = 0;
		finalised = false;
	}

	inline HeapElementType* begin() const noexcept
	{
		jassert(finalised);

		return const_cast<HeapElementType*>(start);
	}

	inline HeapElementType* end() const noexcept
	{
		jassert(finalised);

		return const_cast<HeapElementType*>(ende);
	}

	int size() const noexcept
	{
		return length;
	}

private:

	void allocate(int numElements)
	{
		length = numElements;
		data.allocate(numElements, true);
	}

	Array<ConstructionDataType> constructionData;

	HeapBlock<HeapElementType> data;
	int length = -1;

	bool finalised = false;

	HeapElementType* start = nullptr;
	HeapElementType* ende = nullptr;

	JUCE_DECLARE_NON_COPYABLE(PreallocatedHeapArray);
};



template <typename ElementType, class Sorter = DefaultElementComparator<ElementType>> class OrderedStack
{
public:

	OrderedStack()
	{
		clear();
	}

	void push(const ElementType& p)
	{
		if (pointer < 128)
		{
			data[pointer] = p;
			pointer++;
		}
		else
			jassertfalse; // full


	}

	bool isEmpty() const { return pointer == 0; }

	ElementType pop()
	{
		if (isEmpty())
		{
			jassertfalse;
			return ElementType();
		}

		pointer--;

		return data[pointer];
	}

	int indexOf(const ElementType& type) const
	{
		for (int i = 0; i < pointer; i++)
		{
			if (data[i] == type)
				return i;
		}

		return -1;
	}

	bool contains(const ElementType& element) const
	{
		return indexOf(element) != -1;
	}

	bool removeElement(const ElementType& type)
	{
		auto indexToRemove = indexOf(type);

		if (indexToRemove == -1)
			return false;

		return remove(indexToRemove);
	}

	void clear()
	{
		for (int i = 0; i < 128; i++)
		{
			data[i] = ElementType();
		}

		pointer = 0;
	}

	ElementType getLast() const noexcept
	{
		if (isEmpty())
			return ElementType();

		return data[pointer - 1];
	}

	void addSorted(const ElementType& t)
	{
		for (int i = 0; i < pointer; i++)
		{
			if (data[i] > t)
				insert(t, i);
		}
	}

	void insert(const ElementType& t, int index)
	{
		for (int i = pointer - 1; i > index; i--)
		{
			data[i - 1] = data[i];
		}

		data[index] = t;
		pointer++;
	}

	bool remove(int index)
	{
		if (isEmpty())
			return false;

		if (index < pointer)
		{
			for (int i = index; i < pointer; i++)
			{
				data[i] = data[i + 1];
			}

			pointer--;

			data[pointer] = ElementType();

			return true;
		}

		return false;
	}

	int size() const noexcept { return pointer; }

private:

	int pointer = 0;
	ElementType data[128];
};


/** A container that has a unsorted but packed list of elements.
*
*	Features:
*
*	- no allocation (uses a fixed size)
*	- O(1) insertion (just adds it at the end)
*	- O(n) deletion (it will put the last element in the "hole")
*	- O(1) size
*	- fast iteration (unlike unlinked lists)
*	- cache line friendly (the data is aligned)
*
*/
template <typename ElementType, int SIZE=UNORDERED_STACK_SIZE, typename LockType=DummyCriticalSection> class UnorderedStack
{
public:

	typedef typename LockType::ScopedLockType Lock;

	/** Creates an unordered stack. */

	UnorderedStack() noexcept
	{
		position = 0;

		Lock sl(lock);

		for (int i = 0; i < SIZE; i++)
		{
			data[i] = ElementType();
		}
	}

	~UnorderedStack()
	{
		Lock sl(lock);

		for (int i = 0; i < position; i++)
		{
			data[i] = ElementType();
		}
	}

	/** Inserts an element at the end of the unordered stack. */
	bool insert(const ElementType& elementTypeToInsert)
	{
		Lock sl(lock);

		if (contains(elementTypeToInsert))
		{
			return false;
		}

		data[position] = elementTypeToInsert;

		position = jmin<int>(position + 1, SIZE - 1);
		return true;
	}

	bool insertWithoutSearch(ElementType&& elementTypeToInsert)
	{
		Lock sl(lock);

		jassert(!contains(elementTypeToInsert));

		data[position] = std::move(elementTypeToInsert);

		auto np = position + 1;
		position = jmin<int>(np, SIZE - 1);
		return np < SIZE;
	}

	bool insertWithoutSearch(const ElementType& elementTypeToInsert)
	{
		Lock sl(lock);

		jassert(!contains(elementTypeToInsert));

		data[position] = elementTypeToInsert;

		auto np = position + 1;
		position = jmin<int>(np, SIZE - 1);
		return np < SIZE;
	}

	/** Removes the given element and puts the last element into its slot. */

	bool remove(const ElementType& elementTypeToRemove)
	{
		Lock sl(lock);

		if (!contains(elementTypeToRemove))
		{
			return false;
		}

		for (int i = 0; i < position; i++)
		{
			if (data[i] == elementTypeToRemove)
			{
				jassert(position > 0);
				removeElement(i);
			}
		}

		return true;
	}

	bool removeWithLambda(const std::function<bool(const ElementType&)>& f)
	{
		for (int i = 0; i < position; i++)
		{
			if (f(data[i]))
			{
				jassert(position > 0);
				removeElement(i);
				return true;
			}
		}

		return false;
	}

	bool removeElement(int index)
	{
		Lock sl(lock);

		if (isPositiveAndBelow(index, position))
		{
			position = jmax<int>(0, position-1);
			data[index] = std::move(data[position]);
			data[position] = ElementType();
			return true;
		}

		return false;
	}

	bool contains(const ElementType& elementToLookFor) const noexcept
	{
		return indexOf(elementToLookFor) != -1;
	}

	int indexOf(const ElementType& elementToLookFor) const noexcept
	{
		Lock sl(lock);

		for (int i = 0; i < position; i++)
		{
			if (data[i] == elementToLookFor)
				return i;
		}

		return -1;
	}

	void shrink(int newSize) noexcept
	{
		Lock sl(lock);

		jassert(newSize > 0);
		position = jlimit<int>(0, position, newSize);
	}

    void clear()
    {
		Lock sl(lock);

        memset(data, 0, sizeof(ElementType) * position);
		clearQuick();
    }
    
	void clearQuick()
	{
		Lock sl(lock);

		position = 0;
	}

	ElementType operator[](int index) const
	{
		Lock sl(lock);

		if (index < position)
		{
			return data[index];
		}
		else
		{
			return ElementType();
		}
	};

	bool isEmpty() const
	{
		Lock sl(lock);

		return position == 0;
	}

	int size() const
	{
		Lock sl(lock);

		return (int)position;
	}

	inline ElementType* begin() const noexcept
	{
		ElementType* d = const_cast<ElementType*>(data);
		return d;
	}

	inline ElementType* end() const noexcept
	{
		Lock sl(lock);

		ElementType* d = const_cast<ElementType*>(data);
		return d + position;
	}

	inline const LockType& getLock() const noexcept { return data; }

	
private:

	LockType lock;

	ElementType data[SIZE];

	int position;

	JUCE_DECLARE_NON_COPYABLE(UnorderedStack)
};

#if JUCE_32BIT
#pragma warning(pop)
#endif

/** A bitmap with a compile-time size.
 
    This is more or less a drop in replacement of the BigInteger class without the dynamic reallocation.
*/
template <int NV, typename DataType=uint32, bool ThrowOnOutOfBounds=false> class VoiceBitMap
{
    constexpr static int getElementSize() { return sizeof(DataType) * 8; }
    constexpr static int getNumElements() { return NV / getElementSize(); };
    constexpr static DataType getMaxValue()
    {
        if constexpr (std::is_same<DataType, uint8>())           return 0xFF;
        else if constexpr (std::is_same<DataType, uint16>())     return 0xFFFF;
        else if constexpr (std::is_same<DataType, uint32>())     return 0xFFFFFFFF;
        else /*if constexpr (std::is_same<DataType, uint64>())*/ return 0xFFFFFFFFFFFFFFFF;
    }

public:

    VoiceBitMap()
    {
        clear();
    }

	explicit VoiceBitMap(const void* externalByteData, size_t numBytes=getNumBytes())
    {
	    jassert(numBytes == getNumBytes());
		memcpy(data.data(), externalByteData, numBytes);
		checkEmpty();
    }
	
    void clear()
    {
        memset(data.data(), 0, getNumBytes());
		empty = true;
    }

    void setBit(int voiceIndex, bool value)
    {
		if(isPositiveAndBelow(voiceIndex, getNumBits()))
		{
			auto dIndex = voiceIndex / getElementSize();
	        auto bIndex = voiceIndex % getElementSize();

			if (value)
	        {
	            auto mask = 1 << bIndex;
	            data[dIndex] |= mask;
				empty = false;
	        }
	        else
	        {
	            auto mask = 1 << bIndex;
	            data[dIndex] &= ~mask;
				checkEmpty();
	        }

			return;
		}

		if constexpr (ThrowOnOutOfBounds)
		{
			throw std::out_of_range("out of bounds");
		}
    }

    String toBase64() const
    {
        MemoryBlock mb(data, sizeof(data));
        return mb.toBase64Encoding();
    }
    
    bool fromBase64(const String& b64)
    {
        MemoryBlock mb;
        
        if(mb.fromBase64Encoding(b64) && mb.getSize() == sizeof(data))
        {
			memcpy(data, mb.getData(), mb.getSize());
			checkEmpty();
			
            return true;
        }
        
        return false;
    }
    
    bool operator[](int index) const
    {
		if(empty)
			return false;

        if(isPositiveAndBelow(index, getNumBits()))
        {
            auto bIndex = index % getElementSize();
            auto dIndex = index / getElementSize();
            DataType mask = 1 << bIndex;
            return (data[dIndex] & mask);
        }

		if constexpr (ThrowOnOutOfBounds)
		{
			throw std::out_of_range("out of bounds");
		}
		else
		{
			return false;
		}
    }

	static constexpr size_t getNumBytes() { return sizeof(data); };

	const void* getData() const
    {
	    return data.data();
    }

	static constexpr size_t getNumBits() { return getNumBytes() * 8; }

	size_t getHighestSetBit() const
	{
		size_t rv = 0;

		for(int i = getNumElements() - 1; i >= 0; --i)
		{
			auto v = data[i];
			if(v != 0)
			{
				for (int j = getElementSize() - 1; j >= 0; --j)
	            {
	                DataType mask = 1 << j;
					auto match = ((v & mask) != 0) && (rv == 0);
					rv += static_cast<size_t>(match) * (i * getElementSize() + j);
	            }
			}
		}

		return rv;
	}

	void setAll(bool shouldBeSet)
	{
		if(!shouldBeSet)
			clear();
		else
		{
			memset(data.data(), std::numeric_limits<DataType>::max(), getNumBytes());
			empty = false;
		}
	}

    int getFirstFreeBit() const
    {
		if(empty)
			return -1;

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

    VoiceBitMap& operator|=(const VoiceBitMap& other)
    {
		if(other.empty)
			return *this;

		for (int i = 0; i < getNumElements(); i++)
            data[i] |= other.data[i];

        return *this;
    }

	bool hasSomeBitsAs(const VoiceBitMap& other) const
    {
		if(empty)
			return false;

	    for(int i = 0; i < getNumElements(); i++)
	    {
		    if(data[i] & other.data[i])
				return true;
	    }

		return false;
    }

	bool isEmpty() const noexcept { return empty; }

private:

	void checkEmpty()
	{
        auto thisEmpty = true;

		for(int i = 0; i < getNumElements(); i++)
            thisEmpty &= (data[i] == DataType(0));
        
        empty = thisEmpty;
	}

    static constexpr int NumElements = getNumElements();

    std::array<DataType, NumElements> data;
    bool empty = {true};
};


/** A simple data block that either uses a preallocated memory to avoid relocating or a heap block. */
template <int BSize, int Alignment> struct ObjectStorage
{
	static constexpr int SmallBufferSize = BSize;

	ObjectStorage()
	{
		free();
	}

	~ObjectStorage()
	{
		free();
		bigBuffer.free();
	}

    ObjectStorage& operator=(ObjectStorage&& other)
    {
        objPtr = other.objPtr;
        other.objPtr = nullptr;
        
        allocatedSize = other.allocatedSize;
        other.allocatedSize = 0;
        
        memcpy(smallBuffer, other.smallBuffer, BSize + Alignment);
        bigBuffer = std::move(other.bigBuffer);
        
        return *this;
    }
    
	ObjectStorage(const ObjectStorage& other)
	{
		setSize(other.allocatedSize);
		memcpy(getObjectPtr(), other.getObjectPtr(), allocatedSize);
	}

	void free()
	{
		if (allocatedSize > SmallBufferSize)
		{
			bigBuffer.free();
		}
		
		memset(smallBuffer, 0, BSize + Alignment);
		objPtr = nullptr;
		allocatedSize = 0;
	}
	
	void setExternalPtr(void* ptr)
	{
		free();
		objPtr = ptr;
	}

	void* getObjectPtr() const
	{
		return objPtr;
	}

    void ensureAllocated(size_t numToAllocate, bool copyOldContent=false)
    {
        if(numToAllocate > allocatedSize)
            setSize(numToAllocate, copyOldContent);
    }
    
	void setSize(size_t newSize, bool copyOldContent=false)
	{
		if (newSize != allocatedSize)
		{
            if (newSize >= (SmallBufferSize))
			{
                HeapBlock<uint8> newBuffer;
                
				newBuffer.allocate(newSize + Alignment, !copyOldContent);
                
                if(copyOldContent && allocatedSize > 0)
                    memcpy(newBuffer.get() + Alignment, getObjectPtr(), allocatedSize);
                
                std::swap(newBuffer, bigBuffer);
				objPtr = bigBuffer.get();

				newBuffer.free();
                allocatedSize = newSize;
			}
			else
			{
                if(copyOldContent && allocatedSize > SmallBufferSize)
                {
					jassert(isPositiveAndBelow(newSize, SmallBufferSize));
                    memcpy(&smallBuffer + Alignment, bigBuffer.get() + Alignment, newSize);
                }

				if(allocatedSize > SmallBufferSize)
					bigBuffer.free();
				
				objPtr = &smallBuffer;
                allocatedSize = newSize;
			}

			if constexpr (Alignment != 0)
			{
				if (auto o = reinterpret_cast<uint64_t>(objPtr) % Alignment)
					objPtr = (static_cast<uint8*>(objPtr) + (Alignment - o));
			}
		}
	}

private:

	void* objPtr = nullptr;
	size_t allocatedSize = 0;
	uint8 smallBuffer[BSize + Alignment];
	HeapBlock<uint8> bigBuffer;
};


/** A simple container holding NUM_POLYPHONIC_VOICES elements of the given ObjectType
*	@ingroup data_containers
*
*	In order to make this work, the ObjectType must have a standard constructor.
*/
template <class ObjectType> class FixedVoiceAmountArray
{
public:
	explicit FixedVoiceAmountArray(int numInArray):
		numUsed(jlimit<int>(0, NUM_POLYPHONIC_VOICES, numInArray))
	{}

	inline ObjectType& operator[](int index) const
	{
		if (isPositiveAndBelow(index, numUsed))
		{
			auto r = const_cast<ObjectType*>(data + index);
			return *r;
		}
		else
		{
			jassertfalse;
			auto r = const_cast<ObjectType*>(&fallback);
			return *r;
		}
	}

	FixedVoiceAmountArray(FixedVoiceAmountArray&& other) :
		numUsed(other.numUsed)
	{
		
	}

	int size() const noexcept { return (int)numUsed; };

	inline ObjectType* begin() const noexcept
	{
		ObjectType* d = const_cast<ObjectType*>(data);
		return d;
	}

	inline ObjectType* end() const noexcept
	{
		ObjectType* d = const_cast<ObjectType*>(data);
		return d + numUsed;
	}

private:

	const size_t numUsed;

	ObjectType data[NUM_POLYPHONIC_VOICES];
	ObjectType fallback;
};

namespace MultithreadedQueueHelpers
{
	/** The return type of a function that is called on each element of a queue. */
	enum ReturnStatus
	{
		OK = 0,
		SkipFurtherExecutions,
		AbortClearing,
		numReturnStatuses
	};

	/** This enum specifies the operating mode of a given lockfree queue. You can pass this as template parameter. */
	enum class Configuration
	{
		NoAllocationsNoTokenlessUsage = 0, ///< The default setting. You have to provide a token list and a fixed size
		NoAllocationsTokenlessUsageAllowed, ///< fixed size queue, but is allowed to operate without tokens
		AllocationsAllowedNoTokenlessUsage, ///< dynamically resizable queue, but must be initialised with tokens
		AllocationsAllowedAndTokenlessUsageAllowed, // dynamically resizable queue that can be used without tokens.
		numConfiguration
	};

	constexpr Configuration DefaultConfiguration = Configuration::NoAllocationsNoTokenlessUsage;

	constexpr bool allocationsAllowed(Configuration c)
	{
		return c == Configuration::AllocationsAllowedNoTokenlessUsage ||
			   c == Configuration::AllocationsAllowedAndTokenlessUsageAllowed;
	}

	constexpr bool tokenlessUsageAllowed(Configuration c)
	{
		return c == Configuration::NoAllocationsTokenlessUsageAllowed ||
			   c == Configuration::AllocationsAllowedAndTokenlessUsageAllowed;
	}

	/** This object will be used to build the internal tokens. */
	struct PublicToken
	{
		/** The name of the thread. Just useful for debugging. */
		String threadName;

		/** The list of thread Ids. Get them with Thread::getCurrentThreadIds(). */
		Array<void*> threadIds;

		/** If you know this thread can push elements, set this to true. */
		bool canBeProducer;
	};
}

/** A wrapper around moodycamels ConcurrentQueue with more JUCE like interface and some assertions. */
template <typename ElementType, 
	      MultithreadedQueueHelpers::Configuration ConfigurationType=MultithreadedQueueHelpers::DefaultConfiguration>
	class MultithreadedLockfreeQueue
{
public:

	/** A function prototype for functions that can be called on a ElementType. */
	using ElementFunction = std::function<MultithreadedQueueHelpers::ReturnStatus(ElementType& t)>;

	MultithreadedLockfreeQueue(int numMaxElements) :
		numElements(numMaxElements),
		queue(numElements),
		dummyCToken(queue),
		dummyPToken(queue),
		tokensHaveBeenSet(false),
		somethingHasBeenPushed(false)
	{}

	/** This initialised the queue with the thread Ids. Best use KillstateHandler.createTokens() for this. */
	void setThreadTokens(const Array<MultithreadedQueueHelpers::PublicToken>& threadTokens, const ElementFunction& clearFunction=ElementFunction())
	{
		if(!isEmpty())
			clear(clearFunction);

		if (!allocationsAllowed())
		{
			const int blockSize = moodycamel::ConcurrentQueue<ElementType>::BLOCK_SIZE;
			int numProducers = 0;

			for (const auto& tk : threadTokens)
			{
				if (tk.canBeProducer)
					numProducers++;
			}

			int numRequired = roundToInt((ceil((double)numElements / (double)blockSize) + (double)1) * (double)numProducers * (double)blockSize);
			
			queue = std::move(moodycamel::ConcurrentQueue<ElementType>(size_t(numRequired)));
		}

		tokens.ensureStorageAllocated(threadTokens.size());

		dummyCToken = moodycamel::ConsumerToken(queue);
		dummyPToken = moodycamel::ProducerToken(queue);

		for (const auto& tk : threadTokens)
			tokens.add(PrivateThreadToken(queue, tk));

		tokensHaveBeenSet = true;
	}

	bool push(ElementType&& e)
	{
		somethingHasBeenPushed.store(true);

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet);

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
		{
			if (allocationsAllowed())
				return queue.enqueue(e);
			else
				return queue.try_enqueue(e);
		}
		else
		{
			if (allocationsAllowed())
				return queue.enqueue(getProducerToken(), e);
			else
				return queue.try_enqueue(getProducerToken(), e);
		}
		
	}

	bool pop(ElementType& e)
	{
		if (!somethingHasBeenPushed)
			return false;

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet);

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
			return queue.try_dequeue(e);
		else
			return queue.try_dequeue(getConsumerToken(), e);
	}

	/** Copies multiple elements into the queue. If you don't need the data afterwards, use moveMultiple instead. */
	bool copyMultiple(ElementType* source, int numElements)
	{
		somethingHasBeenPushed.store(true);

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet);

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
		{
			if (allocationsAllowed())
				return queue.enqueue_bulk(*source, (size_t)numElements);
			else
				return queue.try_enqueue_bulk(*source, (size_t)numElements);
		}
		else
		{
			if (allocationsAllowed())
				return queue.enqueue_bulk(getProducerToken(), *source, (size_t)numElements);
			else
				return queue.try_enqueue_bulk(getProducerToken(), *source, (size_t)numElements);
		}
		
	}

	/** Moves multiple elements into the queue. The original data is invalid after this operation. */
	bool moveMultiple(ElementType* source, int numElements)
	{
		somethingHasBeenPushed.store(true);

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet);

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
		{
			if (allocationsAllowed())
				return queue.enqueue_bulk(std::make_move_iterator(*source), (size_t)numElements);
			else
				return queue.try_enqueue_bulk(std::make_move_iterator(*source), (size_t)numElements);
		}
		else
		{
			if (allocationsAllowed())
				return queue.enqueue_bulk(getProducerToken(), std::make_move_iterator(*source), (size_t)numElements);
			else
				return queue.try_enqueue_bulk(getProducerToken(), std::make_move_iterator(*source), (size_t)numElements);
		}
		
	}

	int popMultiple(ElementType* destination, int numDesired)
	{
		if (somethingHasBeenPushed)
			return 0;

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet);

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
			return queue.try_dequeue_bulk(*destination, numDesired);
		else
			return queue.try_dequeue_bulk(getConsumerToken(), *destination, numDesired);
	}

	void addElementToSkip(ElementType&& element)
	{
		cancelledElements.add(element);
	}

	void clear(const ElementFunction& f=ElementFunction())
	{
		if (!somethingHasBeenPushed)
			return;

		jassert(tokenlessUsageAllowed() || tokensHaveBeenSet || isEmpty());

		ElementType item;
		bool skipFurtherExecution = false;
		

		if (tokenlessUsageAllowed() && !tokensHaveBeenSet)
		{
			while (queue.try_dequeue(item))
			{
				if (f && !skipFurtherExecution)
				{
					auto result = f(item);

					if (result == MultithreadedQueueHelpers::SkipFurtherExecutions)
						skipFurtherExecution = true;

					if (result == MultithreadedQueueHelpers::AbortClearing)
						break;
				}
			}
		}
		else
		{
			auto& cToken = getConsumerToken();

			while (queue.try_dequeue(cToken, item))
			{
				if (f && !skipFurtherExecution)
				{
					auto result = f(item);

					if (result == MultithreadedQueueHelpers::SkipFurtherExecutions)
						skipFurtherExecution = true;

					if (result == MultithreadedQueueHelpers::AbortClearing)
						break;
				}
			}
		}
	}

	int size() const noexcept
	{
		return somethingHasBeenPushed ? queue.size_approx() : 0;
	}

	bool isEmpty() const noexcept
	{
		return !somethingHasBeenPushed || queue.size_approx() == 0;
	}

private:

	struct PrivateThreadToken
	{
		explicit PrivateThreadToken(moodycamel::ConcurrentQueue<ElementType>& q, 
									const MultithreadedQueueHelpers::PublicToken& publicToken):
			threadIds(std::move(publicToken.threadIds)),
			name(publicToken.threadName),
			cToken(q),
			pToken(q),
			canBeProducer(publicToken.canBeProducer)
		{}

		Array<void*> threadIds;
		String name;
		moodycamel::ProducerToken pToken;
		moodycamel::ConsumerToken cToken;
		bool canBeProducer;
	};

	moodycamel::ProducerToken& getProducerToken()
	{
		jassert(tokensHaveBeenSet);

		auto id = Thread::getCurrentThreadId();

		for (auto& t : tokens)
		{
			if (t.threadIds.contains(id))
			{
				// You somehow tried to push something from an unexcpected thread...
				jassert(t.canBeProducer);
				return t.pToken;
			}
		}

		jassertfalse;
		return dummyPToken;
	}

	moodycamel::ConsumerToken& getConsumerToken()
	{
		jassert(tokensHaveBeenSet);

		auto id = Thread::getCurrentThreadId();

		for (auto& t : tokens)
			if (t.threadIds.contains(id))
				return t.cToken;

		jassertfalse;
		return dummyCToken;
	}
	
	constexpr bool allocationsAllowed()
	{
		return MultithreadedQueueHelpers::allocationsAllowed(ConfigurationType);
	}

	constexpr bool tokenlessUsageAllowed()
	{
		return MultithreadedQueueHelpers::tokenlessUsageAllowed(ConfigurationType);
	}

	Array<ElementType, CriticalSection> cancelledElements;

	int numElements;
	moodycamel::ConcurrentQueue<ElementType> queue;
	moodycamel::ConsumerToken dummyCToken;
	moodycamel::ProducerToken dummyPToken;
	Array<PrivateThreadToken> tokens;
	bool tokensHaveBeenSet = false;
	std::atomic<bool> somethingHasBeenPushed;
};


/** A wrapper around moodycamels ReaderWriterQueue with more JUCE like interface and some assertions. */
template <class ElementType> class LockfreeQueue
{
public:

	using ElementFunction = std::function<bool(ElementType&)>;

	LockfreeQueue(int numElements) :
		queue((size_t)numElements)
	{
		
	}

	virtual ~LockfreeQueue()
	{

	}

	LockfreeQueue() :
		queue(0)
	{};

	/** Removes an element and returns false if the queue is empty. */
	bool pop(ElementType& newElement)
	{
		return queue.try_dequeue(newElement);
	}

	/** Adds an element to the queue. If it fails because the queue is full, it throws an assertion and return false. */
	bool push(const ElementType& newElement)
	{
		const bool ok = queue.try_enqueue(std::move(newElement));
		jassert_skip_unit_test(ok);
		return ok;
	}

	/** Iterates over the queue, calls the given function for every element and removes it. */
	bool callForEveryElementInQueue(const ElementFunction& f)
	{
		ElementType t;
		
		while (pop(t))
		{
			if (!f(t))
				return false;
		}

		jassert(queue.size_approx() == 0);

		return true;
	}

	/** If the type of the queue is callable, this will call all functions in the queue. */
	bool callEveryElementInQueue()
	{
		ElementType t;
		
		while (pop(t))
		{
			if (!t())
				return false;
		}

		jassert(queue.size_approx() == 0);

		return true;
	}

	bool isEmpty() const
	{
		return queue.size_approx() == 0;
	}

	int size() const
	{
		return (int)queue.size_approx();
	}

private:

	moodycamel::ReaderWriterQueue<ElementType> queue;
};

} // namespace hise

#endif  // CUSTOMDATACONTAINERS_H_INCLUDED
