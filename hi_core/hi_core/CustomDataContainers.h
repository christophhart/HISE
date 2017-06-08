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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef CUSTOMDATACONTAINERS_H_INCLUDED
#define CUSTOMDATACONTAINERS_H_INCLUDED

#define UNORDERED_STACK_SIZE NUM_POLYPHONIC_VOICES


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
	UnorderedStack() noexcept;

	~UnorderedStack();

	/** Inserts an element at the end of the unordered stack. */
	void insert(const ElementType& elementTypeToInsert);;

	/** Removes the given element and puts the last element into its slot. */
	void remove(const ElementType& elementTypeToRemove);;

	void removeElement(int index);

	bool contains(const ElementType& elementToLookFor) const;

    void clear()
    {
        memset(data, 0, sizeof(ElementType) * position);
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

	bool isEmpty() const;

	int size() const;



private:

	ElementType data[UNORDERED_STACK_SIZE];

	size_t position;
};




#endif  // CUSTOMDATACONTAINERS_H_INCLUDED
