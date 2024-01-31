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

	uint8* allocate(int numBytesToAllocate);

	bool validMemoryAccess(uint8* ptr);

private:

	struct Block
	{
		Block(size_t numBytes_);;

		uint8* getData() const;

		bool contains(uint8* ptr);

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
		static int createHash(MemoryLayoutItem::List list);
	};

	virtual ~LayoutBase();;

	size_t getElementSizeInBytes() const;

	Allocator::Ptr allocator;
	MemoryLayoutItem::List layout;

	int typeHash = 0;

protected:

	LayoutBase();

	

	
	static MemoryLayoutItem::List createLayout(Allocator::Ptr allocator, var layoutDescription, Result* r = nullptr);

	friend class ObjectReference;

	Result initResult;

	JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutBase);
};

class ObjectReference : public LayoutBase,
						public ReferenceCountedObject,
						public DebugableObjectBase,
						public ObjectWithJSONConverter
{
public:

	

	using Ptr = ReferenceCountedObjectPtr<ObjectReference>;
	using CompareFunction = std::function<int(Ptr, Ptr)>;

	template <typename T, bool IsArray> struct NumberComparator
	{
		NumberComparator(size_t offset_, int elementSize_=1):
		  offset(offset_),
		  elementSize(elementSize_)
		{};

		int operator()(Ptr p1, Ptr p2) const
		{
			auto t1 = reinterpret_cast<T*>(p1->data + offset);
			auto t2 = reinterpret_cast<T*>(p2->data + offset);

			if(*t1 < *t2)
				return -1;
			if(*t1 > *t2)
				return 1;

			if constexpr (IsArray)
			{
				for(int i = 1; i < elementSize; i++)
				{
					if(t1[i] < t2[i])
						return -1;
					if(t1[i] > t2[i])
						return 1;
				}
			}

			return 0;
		}

		size_t offset = 0;
		int elementSize = 1;
	};

	template <int NumItems> struct MultiComparator
	{
		struct Item
		{
			Item() = default;

			Item(MemoryLayoutItem* p):
			  offset(p->offset),
			  t((uint8)p->type),
			  elementSize(p->elementSize)
			{};
			
			uint16 offset = 0;
			uint8 t = 0;
			uint8 elementSize = 0;
		};

		MultiComparator(const void* data)
		{
			memcpy(items.data(), data, sizeof(items));
		}

		int operator()(Ptr p1, Ptr p2) const
		{
			int result = 0;

			for(auto& d: items)
			{
				auto dt = (DataType)d.t;
				auto isArray = d.elementSize > 1;

				switch(dt)
				{
				case DataType::Boolean:
					result = isArray ? ObjectReference::NumberComparator<bool, true>(d.offset, d.elementSize)(p1, p2) :
									   ObjectReference::NumberComparator<bool, false>(d.offset)(p1, p2);
					break;
				case DataType::Integer:
					result = isArray ? ObjectReference::NumberComparator<int, true>(d.offset, d.elementSize)(p1, p2) :
									   ObjectReference::NumberComparator<int, false>(d.offset)(p1, p2);
					break;
				case DataType::Float:
					result = isArray ? ObjectReference::NumberComparator<float, true>(d.offset, d.elementSize)(p1, p2) :
									   ObjectReference::NumberComparator<float, false>(d.offset)(p1, p2);
					break;
				}

				if(result != 0)
					return result;
			}

			return result;
		}

		std::array<Item, NumItems> items;
	};

	ObjectReference();
	ObjectReference& operator=(const ObjectReference& other);
	ObjectReference(const ObjectReference& other);

	bool operator==(const ObjectReference& other) const;

	void reset();

	void clear();

	bool isValid() const;

	Identifier getObjectName() const override;


	int getNumChildElements() const override;;

	DebugInformationBase* getChildElement(int index);

	struct MemberReference : public ReferenceCountedObject,
		public AssignableObject,
		public DebugableObjectBase
	{
		using Ptr = ReferenceCountedObjectPtr<MemberReference>;

		Identifier getObjectName() const override;

		/** This will be shown as value of the object. */
		String getDebugValue() const override;;

		/** This will be shown as name of the object. */
		String getDebugName() const;;

		String getDebugDataType() const override;

		int getNumChildElements() const override;

		DebugInformationBase* getChildElement(int index) override;


		MemberReference(MemoryLayoutItem::Ptr p, uint8* data_, int indexInArray_);
		MemberReference& operator=(var newValue);
		MemberReference::Ptr operator[](int index) const;

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

	void writeAsJSON (OutputStream& out, const int indentLevel, const bool allOnOneLine, int maximumDecimalPlaces) override;

	MemberReference::Ptr operator[](const Identifier& id) const;

	void init(LayoutBase* referencedLayout, uint8* preallocatedData, bool resetToDefault);
	
	size_t elementSize = 0;
	uint8* data = nullptr;

	WeakReference<LayoutBase> layoutReference;
	NamedValueSet memberReferences;
};


struct Array : public LayoutBase,
	public AssignableObject,
	public ObjectWithJSONConverter,
	public ConstScriptingObject
{
	ObjectReference::CompareFunction compareFunction;

	struct Wrapper;
	

	Identifier getObjectName() const override;

	int getNumChildElements() const override;

	DebugInformationBase* getChildElement(int index) override;

	Array(ProcessorWithScriptingContent* s, int numElements);;

	void init(LayoutBase* parentLayout);

	void assign(const int index, var newValue) override;

	var getAssignedValue(int index) const override;

	int getCachedIndex(const var &indexExpression) const override;

	// =======================================================================================================

	/** Fills the array with the given object. */
	void fill(var obj);

	/** Clears the array (resets all objects to their default. */
	virtual void clear();

	/** Returns the index of the first element that matches the given object. */
	int indexOf(var obj) const;

	/** checks if the array contains the object. */
	bool contains(var obj) const;

	/** Copies the property from each element into a buffer (or array). */
	bool copy(String propertyName, var target);

	/** Exports the memory region of the entire array as Base64 encoded string. */
	String toBase64() const;

	/** Restores an array from a previously exported state. */
	bool fromBase64(const String& b64);

	/** Returns the size of the array. */
	virtual int size() const;

	/** Sorts the array with the given compare function. */
	void sort();

	// =======================================================================================================

	void writeAsJSON(OutputStream& out, const int indentLevel, const bool allOnOneLine,
	                 int maximumDecimalPlaces) override;

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
	struct Viewer;

	struct Wrapper;

	Stack(ProcessorWithScriptingContent* s, int numElements);;

	Identifier getObjectName() const override;

	Component* createPopupComponent(const MouseEvent& e, Component* parent) override;

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

	/** Replaces the object if it exists or inserts it at the end. */
	bool set(var obj);

	// =======================================================================================================

private:

	ObjectReference* getRef(const var& obj);

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

	/** Registers a function that will be used for comparison. If you pass in a string it will only compare the given property. */
	void setCompareFunction(var newCompareFunction);

	/** Returns the hash code for the memory layout which factors in member IDs, order and type. */
	int getTypeHash() const
	{
		return typeHash;
	}

	// ============================================================================================================

	int compare(ObjectReference::Ptr v1, ObjectReference::Ptr v2);;

private:

	struct Wrapper;
	

	WeakCallbackHolder customCompareFunction;

	ObjectReference::CompareFunction compareFunction;

	ReferenceCountedArray<ObjectReference> singleObjects;
	ReferenceCountedArray<Array> arrays;
	
};
}

} // namespace hise
