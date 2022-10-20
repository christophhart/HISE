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

namespace hise { using namespace juce;
namespace fixobj
{

struct Allocator : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Allocator>;

	uint8* allocate(int numBytesToAllocate)
	{
		jassert(numBytesToAllocate % 4 == 0);

		auto b = new Block(numBytesToAllocate);

		allocatedBlocks.add(b);

		return b->getData();
	}

	bool validMemoryAccess(uint8* ptr)
	{
		bool found = false;

		for (auto b : allocatedBlocks)
			found |= b->contains(ptr);
		
		return found;
	}

private:

	struct Block
	{
		Block(size_t numBytes_) :
			numBytes(numBytes_)
		{
			data.allocate(numBytes + 16, true);
			offset = 16 - reinterpret_cast<uint64>(data.get()) % 16;
		};

		uint8* getData() const
		{
			return data.get() + offset;
		}

		bool contains(uint8* ptr)
		{
			auto s = reinterpret_cast<uint64>(getData());
			auto e = s + numBytes;
			auto p = reinterpret_cast<uint64>(ptr);
			return Range<uint64>(s, e).contains(p);
		}

		HeapBlock<uint8> data;
		size_t numBytes;
		size_t offset;
	};

	OwnedArray<Block> allocatedBlocks;
};



struct LayoutBase
{
	enum class DataType
	{
		Integer,
		Boolean,
		Float,
		numTypes
	};

	struct MemoryLayoutItem : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<MemoryLayoutItem>;
		using List = ReferenceCountedArray<MemoryLayoutItem>;

		MemoryLayoutItem(Allocator::Ptr allocator, uint32 offset_, const Identifier& id_, var defaultValue_, Result* r);

		void resetToDefaultValue(uint8* dataStart);
		int getByteSize() const;
		void writeArrayElement(uint8* dataStart, int index, const var& newValue, Result* r);
		var getData(uint8* dataStart, Result* r) const;
		void write(uint8* dataStart, const var& newValue, Result* r);

		Identifier id;
		DataType type;
		uint32 offset;
		int elementSize;
		var defaultValue;

		Allocator::Ptr allocator;
	};

	struct Helpers
	{
		static void writeElement(DataType type, uint8* dataWithOffset, const var& newValue);
		static var getElement(DataType type, const uint8* dataWithOffset);
		static DataType getTypeFromVar(const var& value, Result* r);
		static int getElementSizeFromVar(const var& value, Result* r);
		static uint32 getTypeSize(DataType type);
	};

	virtual ~LayoutBase() {};

	size_t getElementSizeInBytes() const
	{
		size_t bytes = 0;

		for (auto l : layout)
			bytes += l->getByteSize();

		return bytes;
	}

	Allocator::Ptr allocator;
	MemoryLayoutItem::List layout;

protected:

	LayoutBase();

	

	
	static MemoryLayoutItem::List createLayout(Allocator::Ptr allocator, var layoutDescription, Result* r = nullptr);

	friend class ObjectReference;

	Result initResult;

	JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutBase);
};

class ObjectReference : public LayoutBase,
	public ReferenceCountedObject,
	public DebugableObjectBase
{
public:

	using Ptr = ReferenceCountedObjectPtr<ObjectReference>;
	using CompareFunction = std::function<int(Ptr, Ptr)>;

	ObjectReference();
	ObjectReference& operator=(const ObjectReference& other);
	ObjectReference(const ObjectReference& other);

	bool operator==(const ObjectReference& other) const;

	void reset();

	void clear();

	bool isValid() const;

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FixObject"); }


	
	int getNumChildElements() const override
	{ 
		return memberReferences.size();
	};

	DebugInformationBase* getChildElement(int index);

	struct MemberReference : public ReferenceCountedObject,
		public AssignableObject,
		public DebugableObjectBase
	{
		using Ptr = ReferenceCountedObjectPtr<MemberReference>;

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FixObjectMember"); }

		/** This will be shown as value of the object. */
		String getDebugValue() const override;;

		/** This will be shown as name of the object. */
		String getDebugName() const 
		{   
			String s;
			s << "%PARENT%.";
			s << memberProperties->id.toString(); 
			return s;
		};

		String getDebugDataType() const override
		{ 
			String s;

			switch (memberProperties->type)
			{
			case LayoutBase::DataType::Integer: s << "int"; break;
			case LayoutBase::DataType::Float:   s << "float"; break;
			case LayoutBase::DataType::Boolean: s << "bool"; break;
            default:                            break;
			}

			if (!arrayMembers.isEmpty())
				s << "[" << String(arrayMembers.size()) << "]";

			return s;
		}

		int getNumChildElements() const override
		{
			return arrayMembers.size();
		}

		DebugInformationBase* getChildElement(int index) override;


		MemberReference(MemoryLayoutItem::Ptr p, uint8* data_, int indexInArray_);
		MemberReference& operator=(var newValue);
		MemberReference::Ptr operator[](int index) const
		{
			return arrayMembers[index];
		}

		bool isValid() const;

		void assign(const int index, var newValue) override;

		/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
		var getAssignedValue(int index) const override;

		/** Overwrite this and return an index that can be used to look up the value when the script is executed. */
		int getCachedIndex(const var &indexExpression) const override;

		var getNativeValue() const;

		explicit operator var();
		explicit operator int() const;
		explicit operator float() const;
		explicit operator bool() const;

		MemoryLayoutItem::Ptr memberProperties;
		ReferenceCountedArray<MemberReference> arrayMembers;
		uint8* data;
		int indexInArray = -1;

		JUCE_DECLARE_WEAK_REFERENCEABLE(MemberReference);
	};

	MemberReference::Ptr operator[](const Identifier& id) const
	{
		return dynamic_cast<MemberReference*>(memberReferences[id].getObject());
	}

	void init(LayoutBase* referencedLayout, uint8* preallocatedData, bool resetToDefault);

	size_t elementSize = 0;
	uint8* data = nullptr;

	WeakReference<LayoutBase> layoutReference;
	NamedValueSet memberReferences;
};

#if 0
struct Stack : public LayoutBase,
	public hise::UnorderedStack<ObjectReference, 128>,
	public ReferenceCountedObject
{
	Stack(const var& description)
	{
		layout = createLayout(allocator, description, &initResult);

		if (initResult.wasOk())
		{
			auto ptr = begin();

			for (auto l : layout)
				elementSize += l->getByteSize();

			numAllocated = elementSize * 128;

			if (numAllocated > 0)
			{
				allocatedData.allocate(numAllocated, true);
				auto dataPtr = allocatedData.get();

				for (int i = 0; i < 128; i++)
				{
					ptr[i].init(this, dataPtr, true);
					dataPtr += elementSize;
				}
			}
		}
	}

private:

	int numAllocated = 0;
	int elementSize = 0;
	HeapBlock<uint8> allocatedData;
};
#endif

struct Array : public LayoutBase,
	public AssignableObject,
	public ConstScriptingObject
{
	ObjectReference::CompareFunction compareFunction;

	struct Wrapper
	{
		API_METHOD_WRAPPER_1(Array, indexOf);
		API_VOID_METHOD_WRAPPER_1(Array, fill);
		API_VOID_METHOD_WRAPPER_0(Array, clear);
		API_METHOD_WRAPPER_2(Array, copy);
	};

	Identifier getObjectName() const override
	{
		RETURN_STATIC_IDENTIFIER("FixObjectArray");
	}

	int getNumChildElements() const override { return items.size(); }

	DebugInformationBase* getChildElement(int index) override;

	Array(ProcessorWithScriptingContent* s, int numElements):
		ConstScriptingObject(s, 1)
	{
		addConstant("length", numElements);

		ADD_API_METHOD_1(indexOf);
		ADD_API_METHOD_1(fill);
		ADD_API_METHOD_0(clear);
		ADD_API_METHOD_2(copy);
	};

	void init(LayoutBase* parentLayout);

	void assign(const int index, var newValue) override
	{
		if (auto fo = dynamic_cast<ObjectReference*>(newValue.getObject()))
		{
			if (auto i = items[index])
			{
				*i = *fo;
			}
		}
	}

	var getAssignedValue(int index) const override
	{
		if (isPositiveAndBelow(index, items.size()))
		{
			return var(items[index].get());
		}

		return var();
	}

	int getCachedIndex(const var &indexExpression) const override { return (int)indexExpression; }

	// =======================================================================================================

	/** Fills the array with the given object. */
	void fill(var obj);

	/** Clears the array (resets all objects to their default. */
	virtual void clear();

	/** Returns the index of the first element that matches the given object. */
	int indexOf(var obj) const;

	/** Copies the property from each element into a buffer (or array). */
	bool copy(String propertyName, var target);

	/** Returns the size of the array. */
	virtual int size() const;

	/** checks if the array contains the object. */
	bool contains(var obj) const;

	// =======================================================================================================

protected:

	size_t elementSize = 0;
	size_t numElements = 0;
	size_t numAllocated = 0;
	ReferenceCountedArray<ObjectReference> items;
	uint8* data;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Array);
};



struct Stack : public Array
{
	struct Wrapper
	{
		API_METHOD_WRAPPER_1(Stack, insert);
		API_METHOD_WRAPPER_1(Stack, remove);
		API_METHOD_WRAPPER_1(Stack, removeElement);
		API_METHOD_WRAPPER_0(Stack, size);
		API_METHOD_WRAPPER_1(Stack, indexOf);
		API_METHOD_WRAPPER_1(Stack, contains);
		API_METHOD_WRAPPER_0(Stack, isEmpty);
	};

	Stack(ProcessorWithScriptingContent* s, int numElements) :
		Array(s, numElements)
	{
		ADD_API_METHOD_1(insert);
		ADD_API_METHOD_1(remove);
		ADD_API_METHOD_1(removeElement);
		ADD_API_METHOD_0(size);
		ADD_API_METHOD_1(indexOf);
		ADD_API_METHOD_1(contains);
		ADD_API_METHOD_0(isEmpty);
	};

	Identifier getObjectName() { RETURN_STATIC_IDENTIFIER("FixObjectStack"); }

	// =======================================================================================================

	/** Inserts a element to the stack. */
	bool insert(var obj);

	/** Returns the number of used elements in the stack. */
	int size() const override;

	/** Removes the element from the stack and fills up the gap. */
	bool remove(var obj);

	/** Removes the element at the given index and fills the gap. */
	bool removeElement(int index);

	/** Clears the stack. */
	void clear() override;

	/** Clears the stack by moving the end pointer to the start (leaving its elements in the same state). */
	void clearQuick();

	/** Checks whether the stack is empty. */
	bool isEmpty() const;

	// =======================================================================================================

private:

	ObjectReference* getRef(const var& obj)
	{
		return dynamic_cast<ObjectReference*>(obj.getObject());
	}
	
	int position = 0;
};

struct Factory : public LayoutBase,
	public ConstScriptingObject
{
	Factory(ProcessorWithScriptingContent* s, const var& d);

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FixObjectFactory"); };

	// ============================================================================================================

	/** Creates a single object from the prototype layout. */
	var create();

	/** Creates a fixed size array with the given number of elements. */
	var createArray(int numElements);

	/** Creates an unordered stack. */
	var createStack(int numElements);

	/** Registers a function that will be used for comparison. */
	void setCompareFunction(var newCompareFunction);

	// ============================================================================================================

	int compare(ObjectReference::Ptr v1, ObjectReference::Ptr v2);;

private:

	struct Wrapper
	{
		API_METHOD_WRAPPER_0(Factory, create);
		API_METHOD_WRAPPER_1(Factory, createArray);
		API_METHOD_WRAPPER_1(Factory, createStack);
		API_VOID_METHOD_WRAPPER_1(Factory, setCompareFunction);
	};

	WeakCallbackHolder customCompareFunction;

	ObjectReference::CompareFunction compareFunction;

	ReferenceCountedArray<ObjectReference> singleObjects;
	ReferenceCountedArray<Array> arrays;
	
};
}

} // namespace hise
