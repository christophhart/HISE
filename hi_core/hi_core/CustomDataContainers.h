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


template <class ElementType> class FixedAlignedHeapArray
{
public:

	FixedAlignedHeapArray():
		length(0)
	{}

	void operator+=(const ElementType& newElement)
	{
		if (length + 1 > allocated)
		{
			// Try to avoid reallocating when adding multiple elements...
			//jassert(length == 0);

			HeapBlock<ElementType> newData;

			int newLength = length + 1;

			newData.malloc(newLength);

			auto newPtr = newData.get();

			for (int i = 0; i < length; i++)
				new (newPtr++) ElementType(data[i]);

			new (newPtr++) ElementType(newElement);

			clear();

			length = newLength;
			data.swapWith(newData);
		}
		else
		{
			auto ptr = data.get() + length;

			new (ptr) ElementType(newElement);
			length++;
		}
	}

	~FixedAlignedHeapArray()
	{
		clear();
	}

	inline ElementType* begin() const noexcept
	{
		return const_cast<ElementType*>(data.get());
	}

	inline ElementType* end() const noexcept
	{
		return const_cast<ElementType*>(data.get()) + length;
	}

	inline ElementType& operator[](int index) const noexcept
	{
		// If it won't be catched during development, you're out of luck...
		jassert(index < length);
		return data[index];
	}

	int size() const noexcept { return length; };

	void reserve(int elements)
	{
		// You must call this before anything else...
		jassert(length == 0);

		allocated = elements;
		data.allocate(elements, true);
	}

	void clear()
	{
		for (int i = 0; i < length; i++)
		{
			data[i].~ElementType();
		}

		data.free();
		length = 0;
	}

private:

	

	int length;
	int allocated = 0;

	HeapBlock<ElementType> data;
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
template <typename ElementType> class UnorderedStack
{
public:

	/** Creates an unordered stack. */

	UnorderedStack() noexcept
	{
		position = 0;

		for (int i = 0; i < UNORDERED_STACK_SIZE; i++)
		{
			data[i] = ElementType();
		}
	}

	
	~UnorderedStack()
	{
		for (int i = 0; i < position; i++)
		{
			data[i] = ElementType();
		}
	}

	/** Inserts an element at the end of the unordered stack. */
	void insert(const ElementType& elementTypeToInsert)
	{
		if (contains(elementTypeToInsert))
		{
			return;
		}

		data[position] = elementTypeToInsert;

		if (position < UNORDERED_STACK_SIZE)
			position++;
	}

	void insertWithoutSearch(const ElementType& elementTypeToInsert)
	{
		jassert(!contains(elementTypeToInsert));

		data[position] = elementTypeToInsert;

		if (position < UNORDERED_STACK_SIZE)
			position++;
	}

	/** Removes the given element and puts the last element into its slot. */

	void remove(const ElementType& elementTypeToRemove)
	{
		if (!contains(elementTypeToRemove))
		{
			return;
		}

		for (int i = 0; i < position; i++)
		{
			if (data[i] == elementTypeToRemove)
			{
				jassert(position > 0);

				removeElement(i);
			}
		}
	}

	void removeElement(int index)
	{
		if (index < position)
		{
			--position;
			data[index] = data[position];
			data[position] = ElementType();
		}
	}

	bool contains(const ElementType& elementToLookFor) const
	{
		for (size_t i = 0; i < position; i++)
		{
			if (data[i] == elementToLookFor)
				return true;
		}

		return false;
	}

	void shrink(int newSize)
	{
		jassert(newSize > 0);
		position = jmin<size_t>(position, (size_t)newSize);
	}

    void clear()
    {
        memset(data, 0, sizeof(ElementType) * position);
		clearQuick();
    }
    
	void clearQuick()
	{
		position = 0;
	}

	ElementType operator[](int index) const
	{
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
		return position == 0;
	}

	int size() const
	{
		return (int)position;
	}

	inline ElementType* begin() const noexcept
	{
		ElementType* d = const_cast<ElementType*>(data);

		return d;
	}

	inline ElementType* end() const noexcept
	{
		ElementType* d = const_cast<ElementType*>(data);

		return d + position;
	}

private:

	ElementType data[UNORDERED_STACK_SIZE];

	size_t position;

	JUCE_DECLARE_NON_COPYABLE(UnorderedStack)
};

#if JUCE_32BIT
#pragma warning(pop)
#endif







/** A simple container holding NUM_POLYPHONIC_VOICES elements of the given ObjectType
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

	int size() const noexcept { return numUsed; };

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
	bool push(const ElementType&& newElement)
	{
		const bool ok = queue.try_enqueue(newElement);
#if HI_RUN_UNIT_TESTS == 0
		jassert(ok);
#endif
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
