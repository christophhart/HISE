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
*/



/** ============================================================================================================================== UNIT TEST */

class UnorderedStackTest : public UnitTest
{
public:

	struct DummyStruct
	{
		DummyStruct(int index_) :
			index(index_)
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

