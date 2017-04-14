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




template <typename ElementType>
UnorderedStack<ElementType>::UnorderedStack() noexcept
{
	position = 0;

	for (int i = 0; i < UNORDERED_STACK_SIZE; i++)
	{
		data[i] = ElementType();
	}
}


template <typename ElementType>
UnorderedStack<ElementType>::~UnorderedStack()
{
	for (int i = 0; i < position; i++)
	{
		data[i] = ElementType();
	}
}


template <typename ElementType>
void UnorderedStack<ElementType>::insert(const ElementType& elementTypeToInsert)
{
	if (contains(elementTypeToInsert))
	{
		return;
	}

	data[position] = elementTypeToInsert;

	if (position < UNORDERED_STACK_SIZE)
		position++;
}


template <typename ElementType>
void UnorderedStack<ElementType>::remove(const ElementType& elementTypeToRemove)
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


template <typename ElementType>
void UnorderedStack<ElementType>::removeElement(int index)
{
	if (index < position)
	{
		--position;
		data[index] = data[position];
		data[position] = ElementType();
	}
}


template <typename ElementType>
bool UnorderedStack<ElementType>::contains(const ElementType& elementToLookFor) const
{
	for (int i = 0; i < position; i++)
	{
		if (data[i] == elementToLookFor)
			return true;
	}

	return false;
}


template <typename ElementType>
bool UnorderedStack<ElementType>::isEmpty() const
{
	return position == 0;
}


template <typename ElementType>
int UnorderedStack<ElementType>::size() const
{
	return position;
}

/** ============================================================================================================================== UNIT TEST */

class UnorderedStackTest : public UnitTest
{
public:

	struct DummyStruct
	{
		DummyStruct(int index_) :
			index(index)
		{};

		DummyStruct() :
			index(0)
		{}

		int index;
	};

	UnorderedStackTest() :
		UnitTest("Testing unordered stack")
	{

	}

	void runTest() override
	{
		beginTest("Testing basic functions with ints");

		UnorderedStack<int> intStack;

		intStack.insert(1);
		intStack.insert(2);
		intStack.insert(3);

		expectEquals<int>(intStack.size(), 3, "Size after insertion");

		intStack.remove(2);

		expectEquals<int>(intStack.size(), 2, "Size after deletion");

		intStack.insert(4);

		expectEquals<int>(intStack[0], 1, "First Element");
		expectEquals<int>(intStack[1], 3, "Second Element");
		expectEquals<int>(intStack[2], 4, "Third Element");
		expectEquals<int>(intStack[3], 0, "Default Element");

		expect(intStack.contains(4), "Contains int 1");
		expect(!intStack.contains(5), "Contains int 1");

		const float data[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

		beginTest("Testing functions with float pointer");

		UnorderedStack<const float*> fpStack;

		expect(fpStack[0] == nullptr, "Null pointer");

		fpStack.insert(data);
		fpStack.insert(data + 1);
		fpStack.insert(data + 2);
		fpStack.insert(data + 3);
		fpStack.insert(data + 4);

		expectEquals<int>(fpStack.size(), 5);

		expect(fpStack[5] == nullptr, "Null pointer 2");

		expectEquals<float>(*fpStack[0], data[0], "Float pointer elements");
		expectEquals<float>(*fpStack[1], data[1], "Float pointer elements");
		expectEquals<float>(*fpStack[2], data[2], "Float pointer elements");
		expectEquals<float>(*fpStack[3], data[3], "Float pointer elements");
		expectEquals<float>(*fpStack[4], data[4], "Float pointer elements");

		expect(fpStack.contains(data + 2), "Contains float* 1");

		float d2 = 2.0f;

		expect(!fpStack.contains(&d2), "Contains not float 2");
		expect(!fpStack.contains(nullptr), "No null");

		fpStack.remove(data + 2);

		fpStack.insert(data + 2);

		expectEquals<float>(*fpStack[0], data[0], "Float pointer elements after shuffle 1");
		expectEquals<float>(*fpStack[1], data[1], "Float pointer elements after shuffle 2");
		expectEquals<float>(*fpStack[2], data[4], "Float pointer elements after shuffle 3");
		expectEquals<float>(*fpStack[3], data[3], "Float pointer elements after shuffle 4");
		expectEquals<float>(*fpStack[4], data[2], "Float pointer elements after shuffle 5");

		beginTest("Testing with dummy struct");

		OwnedArray<DummyStruct> elements;

		UnorderedStack<DummyStruct*> elementStack;

		for (int i = 0; i < UNORDERED_STACK_SIZE; i++)
		{
			elements.add(new DummyStruct(i));
		}

		Random r;

		for (int i = 0; i < 1000; i++)
		{
			const int indexToInsert = r.nextInt(Range<int>(0, elements.size() - 1));

			auto ds = elements[indexToInsert];

			if (!elements.contains(ds))
			{
				elementStack.insert(ds);
			}
			else
			{
				elementStack.remove(ds);
			}
		}

		for (int i = 0; i < elementStack.size(); i++)
		{
			expect(elementStack[i] != nullptr);
		}

		for (int i = elementStack.size(); i < UNORDERED_STACK_SIZE; i++)
		{
			expect(elementStack[i] == nullptr);
		}

	}

};


static UnorderedStackTest unorderedStackTest;

