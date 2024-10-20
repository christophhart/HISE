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

namespace hise { using namespace juce;

namespace fixobj
{

void LayoutBase::Helpers::writeElement(DataType type, uint8* dataWithOffset, const var& newValue)
{
	switch (type)
	{
	case DataType::Integer: *reinterpret_cast<int*>(dataWithOffset) = (int)newValue; break;
	case DataType::Float:    *reinterpret_cast<float*>(dataWithOffset) = (float)newValue; break;
	case DataType::Boolean: *reinterpret_cast<int*>(dataWithOffset) = (int)(bool)newValue;
        break;
    default:
        break;
	}
}

var LayoutBase::Helpers::getElement(DataType type, const uint8* dataWithOffset)
{
	switch (type)
	{
	case DataType::Integer: return var(*reinterpret_cast<const int*>(dataWithOffset));
	case DataType::Float:    return var(*reinterpret_cast<const float*>(dataWithOffset));
	case DataType::Boolean: return var(*reinterpret_cast<const int*>(dataWithOffset) != 0);
	default:                jassertfalse; return var();
	}
}

hise::fixobj::LayoutBase::DataType LayoutBase::Helpers::getTypeFromVar(const var& value, Result* r)
{
	if (value.isArray())
		return getTypeFromVar(value[0], r);

	if (value.isInt() || value.isInt64())
		return DataType::Integer;

	if (value.isDouble())
		return DataType::Float;

	if (value.isBool())
		return DataType::Boolean;

	if (r != nullptr)
		*r = Result::fail("illegal data type: \"" + value.toString() + "\"");

	return DataType::numTypes;
}

int LayoutBase::Helpers::getElementSizeFromVar(const var& value, Result* r)
{
	if (value.isArray())
		return value.size();

	if (value.isObject() || value.isString())
	{
		if (r != nullptr)
			*r = Result::fail("illegal type");
	}

	return 1;
}

juce::uint32 LayoutBase::Helpers::getTypeSize(DataType type)
{
	switch (type)
	{
	case DataType::Integer: return sizeof(int);
	case DataType::Boolean: return sizeof(int);
	case DataType::Float:   return sizeof(float);
    default:                return 0;
	}
}

int LayoutBase::Helpers::createHash(MemoryLayoutItem::List list)
{
	String s;

	for(auto l: list)
	{
		s << l->id;
		s << (uint8)l->type;
	}

	return s.hashCode();
}

LayoutBase::~LayoutBase()
{}

size_t LayoutBase::getElementSizeInBytes() const
{
	size_t bytes = 0;

	for (auto l : layout)
		bytes += l->getByteSize();

	return bytes;
}


uint8* Allocator::allocate(int numBytesToAllocate)
{
	jassert(numBytesToAllocate % 4 == 0);

	auto b = new Block(numBytesToAllocate);

	allocatedBlocks.add(b);

	return b->getData();
}

bool Allocator::validMemoryAccess(uint8* ptr)
{
	bool found = false;

	for (auto b : allocatedBlocks)
		found |= b->contains(ptr);
		
	return found;
}

Allocator::Block::Block(size_t numBytes_):
	numBytes(numBytes_)
{
	data.allocate(numBytes + 16, true);
	offset = 16 - reinterpret_cast<uint64>(data.get()) % 16;
}

uint8* Allocator::Block::getData() const
{
	return data.get() + offset;
}

bool Allocator::Block::contains(uint8* ptr)
{
	auto s = reinterpret_cast<uint64>(getData());
	auto e = s + numBytes;
	auto p = reinterpret_cast<uint64>(ptr);
	return Range<uint64>(s, e).contains(p);
}

LayoutBase::MemoryLayoutItem::MemoryLayoutItem(Allocator::Ptr allocator_, uint32 offset_, const Identifier& id_, var defaultValue_, Result* r) :
	id(id_),
	allocator(allocator_),
	type(Helpers::getTypeFromVar(defaultValue_, r)),
	elementSize(Helpers::getElementSizeFromVar(defaultValue_, r)),
	offset(offset_),
	defaultValue(defaultValue_)
{

}

void LayoutBase::MemoryLayoutItem::resetToDefaultValue(uint8* dataStart)
{
	write(dataStart, defaultValue, nullptr);
}

int LayoutBase::MemoryLayoutItem::getByteSize() const
{
	return Helpers::getTypeSize(type) * elementSize;
}

void LayoutBase::MemoryLayoutItem::writeArrayElement(uint8* dataStart, int index, const var& newValue, Result* r)
{
	if (isPositiveAndBelow(index, elementSize - 1))
	{
		auto ptr = dataStart + Helpers::getTypeSize(type) * index;
		jassert(allocator->validMemoryAccess(ptr));
		Helpers::writeElement(type, ptr, newValue);
	}
	else
	{
		if (r != nullptr)
			*r = Result::fail("out of bounds");
	}
}

var LayoutBase::MemoryLayoutItem::getData(uint8* dataStart, Result* r) const
{
	if (elementSize == 1)
	{
		auto ptr = dataStart + offset;
		jassert(allocator->validMemoryAccess(ptr));
		return Helpers::getElement(type, ptr);
	}

	if (r != nullptr)
		*r = Result::fail("Can't get reference to fix array");

	return var();
}

void LayoutBase::MemoryLayoutItem::write(uint8* dataStart, const var& newValue, Result* r)
{
	if (elementSize == 1)
	{
		if (newValue.isArray())
		{
			if (r != nullptr)
				*r = Result::fail("Can't write array to single element");

			return;
		}

		auto ptr = dataStart + offset;
		jassert(allocator->validMemoryAccess(ptr));
		Helpers::writeElement(type, ptr, newValue);
	}
	else
	{
		if (auto ar = newValue.getArray())
		{
			auto numElementsToRead = ar->size();

			if (elementSize != numElementsToRead)
			{
				if (r != nullptr)
					*r = Result::fail("array size mismatch. Expected " + String(elementSize));

				return;
			}

			auto ts = Helpers::getTypeSize(type);

			for (int i = 0; i < numElementsToRead; i++)
			{
				auto d = dataStart + offset + i * ts;
				jassert(allocator->validMemoryAccess(d));
				Helpers::writeElement(type, d, ar->getUnchecked(i));
			}
		}
		else
		{
			if (r != nullptr)
				*r = Result::fail("This data type requires an array.");
		}
	}
}

LayoutBase::LayoutBase() :
	initResult(Result::ok())
{

}

hise::fixobj::LayoutBase::MemoryLayoutItem::List LayoutBase::createLayout(Allocator::Ptr allocator, var layoutDescription, Result* r /*= nullptr*/)
{
	MemoryLayoutItem::List items;

	if (auto obj = layoutDescription.getDynamicObject())
	{
		uint32 offset = 0;

		for (const auto& prop : obj->getProperties())
		{
			auto id = prop.name;
			auto v = prop.value;

			auto newItem = new MemoryLayoutItem(allocator, offset, prop.name, prop.value, r);
			items.add(newItem);
			offset += newItem->getByteSize();
		}
	}

	if (items.isEmpty())
		*r = Result::fail("No data");

	return items;
}

struct Factory::Wrapper
{
	API_METHOD_WRAPPER_0(Factory, create);
	API_METHOD_WRAPPER_1(Factory, createArray);
	API_METHOD_WRAPPER_1(Factory, createStack);
	API_VOID_METHOD_WRAPPER_1(Factory, setCompareFunction);
};

Factory::Factory(ProcessorWithScriptingContent* s, const var& d) :
	ConstScriptingObject(s, 0),
	customCompareFunction(getScriptProcessor(), this, var(), 2)
{
	allocator = new Allocator();

	ADD_API_METHOD_0(create);
	ADD_API_METHOD_1(createArray);
	ADD_API_METHOD_1(createStack);
	ADD_API_METHOD_1(setCompareFunction);

	addConstant("prototype", d);
	layout = createLayout(allocator, d, &initResult);

	typeHash = Helpers::createHash(layout);

	compareFunction = BIND_MEMBER_FUNCTION_2(Factory::compare);
}

var Factory::create()
{
	if (initResult.wasOk())
	{
		auto b = allocator->allocate((int)getElementSizeInBytes());

		auto r = new ObjectReference();
		r->init(this, b, true);
		singleObjects.add(r);

		return var(r);
	}

	return var();
}

var Factory::createArray(int numElements)
{
	if (initResult.wasOk())
	{
		auto newElement = new Array(getScriptProcessor(), numElements);
		newElement->compareFunction = compareFunction;
		newElement->init(this);
		arrays.add(newElement);
		return var(newElement);
	}

	return {};
}

var Factory::createStack(int numElements)
{
	if (initResult.wasOk())
	{
		auto newElement = new Stack(getScriptProcessor(), numElements);
		newElement->compareFunction = compareFunction;
		newElement->init(this);
		arrays.add(newElement);
		return var(newElement);
	}
    
    return var();
}



void Factory::setCompareFunction(var newCompareFunction)
{
	if(newCompareFunction.isString())
	{
		auto text = newCompareFunction.toString();

		if(text.contains(","))
		{
			auto tokens = StringArray::fromTokens(text, ",", "");

			juce::Array<Identifier> ids;

			for(auto& t: tokens)
				ids.add(Identifier(t));
			
			juce::Array<ObjectReference::MultiComparator<1>::Item> items;

			for(auto& id: ids)
			{
				for(auto l: layout)
				{
					if(l->id == id)
					{
						items.add(l);
						break;
					}
				}
			}
			
			if(items.size() != ids.size())
				reportScriptError("unknown properties: " + text);

			if(items.size() < 2)
				reportScriptError("Redundant comma");

			auto dataPtr = (void*)items.getRawDataPointer();

			switch(items.size())
			{
			case 2: compareFunction = ObjectReference::MultiComparator<2>(dataPtr); break;
			case 3: compareFunction = ObjectReference::MultiComparator<3>(dataPtr); break;
			case 4: compareFunction = ObjectReference::MultiComparator<4>(dataPtr); break;
			default: reportScriptError("At this point you might want to use a custom function");
			}
		}
		else
		{
			auto id = Identifier(text);

			bool found = false;

			for(auto l: layout)
			{
				if(l->id == id)
				{
					bool isArray = l->elementSize > 1;

					switch(l->type)
					{
					case DataType::Boolean:
						if(isArray) compareFunction = ObjectReference::NumberComparator<bool, true>(l->offset, l->elementSize);
						else        compareFunction = ObjectReference::NumberComparator<bool, false>(l->offset);
						break;
					case DataType::Integer:
						if(isArray) compareFunction = ObjectReference::NumberComparator<int, true>(l->offset, l->elementSize);
						else        compareFunction = ObjectReference::NumberComparator<int, false>(l->offset);
						break;
					case DataType::Float:
						if(isArray) compareFunction = ObjectReference::NumberComparator<float, true>(l->offset, l->elementSize);
						else        compareFunction = ObjectReference::NumberComparator<float, false>(l->offset);
						break;
					case DataType::numTypes:
						break;
					}					

					found = true;
					break;
				}
			}

			if(!found)
				reportScriptError("Can't find property " + newCompareFunction.toString());
		}
	}
	else if (HiseJavascriptEngine::isJavascriptFunction(newCompareFunction))
	{
		customCompareFunction = WeakCallbackHolder(getScriptProcessor(), this, newCompareFunction, 2);
		customCompareFunction.incRefCount();
	}
	else
	{
		compareFunction = BIND_MEMBER_FUNCTION_2(Factory::compare);
	}

	// update the compare function for all created arrays or stacks
	for(auto& a: arrays)
	{
		a->compareFunction = compareFunction;
	}
}

int Factory::compare(ObjectReference::Ptr v1, ObjectReference::Ptr v2)
{
	if (customCompareFunction)
	{
		var args[2] = { var(v1.get()), var(v2.get()) };
		var r(0);
		auto ok = customCompareFunction.callSync(args, 2, &r);

		return (int)r;
	}
	else
	{
		if (*v1 == *v2)
		{
			return 0;
		}
		else
		{
			auto p1 = reinterpret_cast<uint64>(v1.get());
			auto p2 = reinterpret_cast<uint64>(v2.get());

			if (p1 > p2)
				return 1;
			else
				return -1;
		}
	}

	return 0;
}

bool ObjectReference::operator==(const ObjectReference& other) const
{
	if (data == other.data)
		return true;

	if (layout[0] == other.layout[0])
	{
		bool same = true;

		auto i1 = reinterpret_cast<const int*>(data);
		auto i2 = reinterpret_cast<const int*>(other.data);

		auto numIntsToCheck = elementSize / sizeof(int);

		for (int i = 0; i < numIntsToCheck; i++)
			same &= (i1[i] == i2[i]);

		return same;
	}

	return false;
}

ObjectReference::ObjectReference()
{
	reset();
}

ObjectReference::ObjectReference(const ObjectReference& other)
{
	*this = other;
}

bool ObjectReference::isValid() const
{
	return layoutReference != nullptr && data != nullptr;
}



void ObjectReference::writeAsJSON (OutputStream& out, const int indentLevel, const bool allOnOneLine, int maximumDecimalPlaces)
{
	JSON_DECLARE_NL_IND;
		
	out << '{'; nl();

	for(auto l: layout)
	{
		auto t = l->type == DataType::Float ? Types::ID::Double : Types::ID::Integer;
		auto v = l->getData(data, nullptr);
		ind(); out << l->id.toString().quoted() <<  ": " << snex::Types::Helpers::getCppValueString(v, t);

		if(layout.getLast().get() != l)
			out.writeByte(',');

		nl();
	}
	
	out << '}'; nl();
}

Identifier ObjectReference::getObjectName() const
{ RETURN_STATIC_IDENTIFIER("FixObject"); }

int ObjectReference::getNumChildElements() const
{ 
	return memberReferences.size();
}

Identifier ObjectReference::MemberReference::getObjectName() const
{ RETURN_STATIC_IDENTIFIER("FixObjectMember"); }

String ObjectReference::MemberReference::getDebugName() const
{   
	String s;
	s << "%PARENT%.";
	s << memberProperties->id.toString(); 
	return s;
}

String ObjectReference::MemberReference::getDebugDataType() const
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

int ObjectReference::MemberReference::getNumChildElements() const
{
	return arrayMembers.size();
}

ObjectReference::MemberReference::Ptr ObjectReference::MemberReference::operator[](int index) const
{
	return arrayMembers[index];
}

ObjectReference::MemberReference::Ptr ObjectReference::operator[](const Identifier& id) const
{
	return dynamic_cast<MemberReference*>(memberReferences[id].getObject());
}

hise::DebugInformationBase* ObjectReference::getChildElement(int index)
{

	if (isPositiveAndBelow(index, memberReferences.size()))
	{
		auto id = "%PARENT%" + memberReferences.getName(index).toString();
		auto v = memberReferences.getValueAt(index);

		auto obj = dynamic_cast<DebugableObjectBase*>(v.getObject());

		return new DebugableObjectInformation(obj, id, DebugableObjectInformation::Type::Globals);
	}

	return nullptr;
}

void ObjectReference::init(LayoutBase* referencedLayout, uint8* preallocatedData, bool resetToDefault)
{
	allocator = referencedLayout->allocator;
	data = preallocatedData;

	layoutReference = referencedLayout;
	layout = referencedLayout->layout;
	typeHash = Helpers::createHash(layout);

	jassert(allocator->validMemoryAccess(data));
	jassert(allocator->validMemoryAccess(data + getElementSizeInBytes() - 1));

	initResult = Result::ok();

	if (!isValid())
		return;

	for (auto l : layoutReference->layout)
	{
		if (data != nullptr && resetToDefault)
			l->resetToDefaultValue(data);

		auto m = new MemberReference(l, data, -1);
		memberReferences.set(l->id, var(m));
	}

	elementSize = getElementSizeInBytes();
}

void ObjectReference::reset()
{
	data = nullptr;
	layoutReference = nullptr;
	elementSize = 0;
	initResult = Result::fail("uninitialised");
}

void ObjectReference::clear()
{
	if (isValid())
	{
		for (auto l : layout)
			l->resetToDefaultValue(data);
	}
}

hise::fixobj::ObjectReference& ObjectReference::operator=(const ObjectReference& other)
{
	if (!isValid())
	{
		data = other.data;
		elementSize = other.elementSize;
		layoutReference = other.layoutReference;
		initResult = other.initResult;
	}
	else
	{
		if (!other.isValid())
		{
			reset();
		}
		else
		{
			jassert(other.elementSize == elementSize);
			jassert(layout.size() == other.layout.size());
			memcpy(data, other.data, elementSize);
		}
	}

	return *this;
}

String ObjectReference::MemberReference::getDebugValue() const
{
	if (arrayMembers.isEmpty() && isValid())
		return getNativeValue().toString();

	return "";
}

hise::DebugInformationBase* ObjectReference::MemberReference::getChildElement(int index)
{
	if (isPositiveAndBelow(index, arrayMembers.size()))
	{
		WeakReference<MemberReference> safeThis(this);

		String id;
		id << "%PARENT%[" << index << "]";

		auto vf = [safeThis, index]()
		{
			if (safeThis != nullptr)
				return var(safeThis->arrayMembers[index].get());

			return var();
		};

		return new LambdaValueInformation(vf, Identifier(id), {}, DebugInformation::Type::Globals, getLocation());
	}

	return nullptr;
}

ObjectReference::MemberReference::MemberReference(MemoryLayoutItem::Ptr p, uint8* data_, int indexInArray_) :
	memberProperties(p),
	data(data_),
	indexInArray(indexInArray_)
{
	if (p->elementSize > 1 && indexInArray == -1)
	{
		for (int i = 0; i < p->elementSize; i++)
		{
			arrayMembers.add(new MemberReference(p, data, i));
		}
	}
}

bool ObjectReference::MemberReference::isValid() const
{
	return data != nullptr;
}

void ObjectReference::MemberReference::assign(const int index, var newValue)
{
	auto c = arrayMembers[index];

	*c = newValue;
}

var ObjectReference::MemberReference::getAssignedValue(int index) const
{
	if (isPositiveAndBelow(index, arrayMembers.size()))
	{
		return var((var)*arrayMembers[index]);
	}

	return var();
}

int ObjectReference::MemberReference::getCachedIndex(const var &indexExpression) const
{
	return (int)indexExpression;
}

var ObjectReference::MemberReference::getNativeValue() const
{
	jassert(isValid());

	auto ptr = data + memberProperties->offset;

	if (indexInArray != -1)
		ptr += Helpers::getTypeSize(memberProperties->type) * indexInArray;

	return Helpers::getElement(memberProperties->type, ptr);
}

ObjectReference::MemberReference::operator var()
{
	if (!arrayMembers.isEmpty())
		return var(this);

	return isValid() ? getNativeValue() : var();
}

ObjectReference::MemberReference::operator int() const
{
	return isValid() ? (int)Helpers::getElement(memberProperties->type, data) : 0;
}

ObjectReference::MemberReference::operator float() const
{
	return isValid() ? (float)Helpers::getElement(memberProperties->type, data) : 0.0f;
}

ObjectReference::MemberReference::operator bool() const
{
	return isValid() ? (bool)Helpers::getElement(memberProperties->type, data) : false;
}

hise::fixobj::ObjectReference::MemberReference& ObjectReference::MemberReference::operator=(var newValue)
{
	if (memberProperties->elementSize == 1 || indexInArray != -1)
	{
		auto ptr = data + memberProperties->offset;

		if (indexInArray != -1)
			ptr += Helpers::getTypeSize(memberProperties->type) * indexInArray;

		Helpers::writeElement(memberProperties->type, ptr, newValue);
	}
		
	return *this;
}


Identifier Array::getObjectName() const
{
	RETURN_STATIC_IDENTIFIER("FixObjectArray");
}

int Array::getNumChildElements() const
{ return items.size(); }

struct Array::Wrapper
{
	API_METHOD_WRAPPER_1(Array, indexOf);
	API_VOID_METHOD_WRAPPER_1(Array, fill);
	API_VOID_METHOD_WRAPPER_0(Array, clear);
	API_METHOD_WRAPPER_1(Array, contains);
	API_METHOD_WRAPPER_2(Array, copy);
	API_VOID_METHOD_WRAPPER_0(Array, sort);
	API_METHOD_WRAPPER_0(Array, size);
	API_METHOD_WRAPPER_0(Array, toBase64);
	API_METHOD_WRAPPER_1(Array, fromBase64);
};

Array::Array(ProcessorWithScriptingContent* s, int numElements):
	ConstScriptingObject(s, 1)
{
	addConstant("length", numElements);

	ADD_API_METHOD_1(indexOf);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_1(fill);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_2(copy);
	ADD_API_METHOD_0(size);
	ADD_API_METHOD_0(sort);
	ADD_API_METHOD_0(toBase64);
	ADD_API_METHOD_1(fromBase64);
}

void Array::assign(const int index, var newValue)
{
	if (auto fo = dynamic_cast<ObjectReference*>(newValue.getObject()))
	{
		if (auto i = items[index])
		{
			*i = *fo;
		}
	}
}

var Array::getAssignedValue(int index) const
{
	if (isPositiveAndBelow(index, items.size()))
	{
		return var(items[index].get());
	}

	return var();
}

int Array::getCachedIndex(const var& indexExpression) const
{ return (int)indexExpression; }

hise::DebugInformationBase* Array::getChildElement(int index)
{
	if (isPositiveAndBelow(index, items.size()))
	{
		String id;
		id << "%PARENT%[" << index << "]";

		WeakReference<Array> safeThis(this);

		auto vf = [safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				return safeThis.get()->getAssignedValue(index);
			}

			return var();
		};

		return new LambdaValueInformation(vf, Identifier(id), {}, DebugInformation::Type::Globals, getLocation());
	}

	return nullptr;
}

void Array::init(LayoutBase* parent)
{
	layout = parent->layout;
	allocator = parent->allocator;

	numElements = (int)getConstantValue(0);

	if (!initResult.wasOk())
		layout.clear();


	elementSize = getElementSizeInBytes();
	numAllocated = getElementSizeInBytes() * (numElements);

	typeHash = Helpers::createHash(layout);

	if (numAllocated > 0)
	{
		data = allocator->allocate((int)numAllocated);
		
		for (int i = 0; i < numElements; i++)
		{
			auto ptr = data + i * elementSize;

			jassert(allocator->validMemoryAccess(ptr));

			auto obj = new ObjectReference();
			obj->init(this, ptr, true);
			items.add(obj);
		}
	}
}

void Array::fill(var obj)
{
	if (auto o = dynamic_cast<ObjectReference*>(obj.getObject()))
	{
		for (auto i : items)
			*i = *o;
	}
	else
	{
		for (auto i : items)
			i->clear();
	}
}

void Array::clear()
{
	fill(var());
}

int Array::indexOf(var obj) const
{
	if (auto o = dynamic_cast<ObjectReference*>(obj.getObject()))
	{
		int numToSearch = size();

		if(numToSearch == 0)
			return -1;

		for (int i = 0; i < numToSearch; i++)
		{
			auto item = items[i];

			if (compareFunction(item, o) == 0)
				return i;
		}
	}

	return -1;
}

bool Array::copy(String propertyName, var target)
{
	size_t offset = 0;

	Identifier p(propertyName);
	DataType originalType = LayoutBase::DataType::numTypes;

	for (auto l : layout)
	{
		if (l->id == p)
		{
			offset = l->offset;
			originalType = l->type;
			break;
		}
	}

	if (originalType == LayoutBase::DataType::numTypes)
		reportScriptError("Can't find property " + p.toString());

	auto ptr = data + offset;

	if (auto b = target.getBuffer())
	{
		if (numElements != b->size)
			reportScriptError("buffer size mismatch");

		for (int i = 0; i < numElements; i++)
		{
			auto v = (float)Helpers::getElement(originalType, ptr);
			ptr += elementSize;
			b->setSample(i, v);
		}

		return true;
	}
	else if (auto a = target.getArray())
	{
		a->ensureStorageAllocated((int)numElements);
		
		for (int i = 0; i < numElements; i++)
		{
			auto v = Helpers::getElement(originalType, ptr);
			ptr += elementSize;
			a->set(i, v);
		}

		return true;
	}

	return false;
}

String Array::toBase64() const
{
	MemoryBlock mb(data, numAllocated);
	return mb.toBase64Encoding();
}

bool Array::fromBase64(const String& b64)
{
	MemoryBlock mb;
	mb.fromBase64Encoding(b64);

	if(mb.getSize() == numAllocated)
	{
		memcpy(data, mb.getData(), numAllocated);
		return true;
	}

	return false;
}

int Array::size() const
{
	return (int)numElements;
}

void Array::sort()
{
	struct Sorter
	{
		Sorter(Array& a):
			parent(a)
		{};

		int compareElements(ObjectReference::Ptr p1, ObjectReference::Ptr p2) const
		{
			//ObjectReference::Ptr p1_(&p1);
			//ObjectReference::Ptr p2_(&p2);

			return parent.compareFunction(p1, p2);
		}

		Array& parent;
	};

	Sorter s(*this);
	SortFunctionConverter<Sorter> converter (s);
	
	std::sort(items.begin(), items.begin() + size(), converter);
}

void Array::writeAsJSON(OutputStream& out, const int indentLevel, const bool allOnOneLine, int maximumDecimalPlaces)
{
	JSON_DECLARE_NL_IND;

	ind(); out.writeByte('['); nl();

	auto numToPrint = size();

	for(int i = 0; i < numToPrint; i++)
	{
		auto item = items[i];

		item->writeAsJSON(out, indentLevel+1, allOnOneLine, maximumDecimalPlaces);

		if(i != (numToPrint-1))
			out.writeByte(',');
	}

	ind(); out.writeByte(']'); nl();
}

bool Array::contains(var obj) const
{
	return indexOf(obj) != -1;
}

struct Stack::Viewer: public Component,
					  public ComponentForDebugInformation,
					  public PooledUIUpdater::SimpleTimer
{
	struct Row
	{
		static constexpr int IndexWidth = 32;
		static constexpr int Height = 24;
		static constexpr int ColumnWidth = 100;

		Row(Stack* s, int index_):
		  index(index_)
		{
			auto numColumns = s->layout.size();

			for(int i = 0; i < numColumns; i++)
			{
				changeAlpha.add(0.0f);
				values.add(s->layout[i]->defaultValue);
				types.add(s->layout[i]->type);
			}
		}

		void paint(Graphics& g, Rectangle<float> area)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.setFont(GLOBAL_MONOSPACE_FONT());

			g.drawText(String(index), area.removeFromLeft(IndexWidth).toFloat(), Justification::centred);

			for(int i = 0; i < values.size(); i++)
			{
				auto cell = area.removeFromLeft(ColumnWidth);

				g.setColour(Colours::white.withAlpha(used ? 0.05f : 0.02f));
				g.fillRect(cell.reduced(1.0f));

				if(used)
				{
					g.setColour(Colours::white.withAlpha(jlimit(0.0f, 1.0f, changeAlpha[i] * 0.4f)));
					g.fillRect(cell);
				}

				float alpha = 0.1f;

				if(used)
					alpha += 0.7f;

				g.setColour(Colours::white.withAlpha(alpha));
				g.setFont(GLOBAL_MONOSPACE_FONT());

				auto s = snex::Types::Helpers::getCppValueString(values[i], types[i] == DataType::Float ? Types::ID::Float : Types::ID::Integer);
				g.drawText(s, cell, Justification::centred);
			}
		}

		juce::Array<DataType> types;
		bool used = false;
		int index = 0;
		juce::Array<var> values;
		juce::Array<float> changeAlpha;
	};

	Viewer(Stack* s):
	  ComponentForDebugInformation(s, dynamic_cast<JavascriptProcessor*>(s->getScriptProcessor())),
	  SimpleTimer(s->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
	{
		setName("FixObjectStack Viewer");

		const auto numColumns = s->layout.size();
		const auto numRows = jmin<int>(16, static_cast<int>(s->numElements));

		setSize(numColumns * Row::ColumnWidth + Row::IndexWidth, numRows * Row::Height);

		for(int i = 0; i < numColumns; i++)
		{
			titles.add(s->layout[i]->id.toString());
		}

		for(int i = 0; i < s->numElements; i++)
		{
			rows.add(new Row(s, i));
		}
	};

	StringArray titles;
	OwnedArray<Row> rows;

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();

		auto header = b.removeFromTop(Row::Height);
		header.removeFromLeft(Row::IndexWidth);

		for(auto t: titles)
		{
			auto cell = header.removeFromLeft(Row::ColumnWidth).toFloat();
			g.setColour(Colours::white.withAlpha(0.1f));
			g.fillRect(cell.reduced(1.0f));
			g.setColour(Colours::white.withAlpha(0.7f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(t, cell, Justification::centred);
		}

		for(auto r: rows)
		{
			r->paint(g, b.removeFromTop(Row::Height).toFloat());
		}
	}

	void timerCallback() override
	{
		if(auto obj = getObject<Stack>())
		{
			auto numColumns = obj->layout.size();

			for(int i = 0; i < obj->numElements; i++)
			{
				auto r = rows[i];
				r->used = i < obj->position;

				if(r->used)
				{
					auto data = obj->items[i]->data;
				
					for(int j = 0; j < numColumns; j++)
					{
						auto nv = obj->layout[j]->getData(data, nullptr);
						auto ov = r->values[j];
						r->values.set(j, nv);
						auto alpha = r->changeAlpha[j];

						if(nv != ov)
							alpha = 1.0f;
						else
							alpha = jmax(0.0f, alpha - 0.05f);

						r->changeAlpha.set(j, alpha);
					}
				}
			}
			
			repaint();
		}
	}
};

struct Stack::Wrapper
{
	API_METHOD_WRAPPER_1(Stack, insert);
	API_METHOD_WRAPPER_1(Stack, remove);
	API_METHOD_WRAPPER_1(Stack, removeElement);
	API_METHOD_WRAPPER_0(Stack, size);
	API_METHOD_WRAPPER_1(Stack, indexOf);
	API_METHOD_WRAPPER_1(Stack, contains);
	API_METHOD_WRAPPER_0(Stack, isEmpty);
	API_VOID_METHOD_WRAPPER_1(Stack, set);
	API_VOID_METHOD_WRAPPER_0(Stack, clear);
	API_VOID_METHOD_WRAPPER_0(Stack, clearQuick);
};

Stack::Stack(ProcessorWithScriptingContent* s, int numElements):
	Array(s, numElements)
{
	ADD_API_METHOD_1(insert);
	ADD_API_METHOD_1(remove);
	ADD_API_METHOD_1(removeElement);
	ADD_API_METHOD_0(size);
	ADD_API_METHOD_1(indexOf);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_1(set);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_0(clearQuick);
}

Identifier Stack::getObjectName() const
{ RETURN_STATIC_IDENTIFIER("FixObjectStack"); }

Component* Stack::createPopupComponent(const MouseEvent& e, Component* parent)
{ return new Viewer(this); }

ObjectReference* Stack::getRef(const var& obj)
{
	return dynamic_cast<ObjectReference*>(obj.getObject());
}

bool Stack::insert(var obj)
{
	auto idx = indexOf(obj);

	if (idx != -1)
		return false;

	if (auto ref = getRef(obj))
	{
		*items[position] = *ref;

		position = jmin<int>(position + 1, (int)numElements - 1);
		return true;
	}

	return false;
}

int Stack::size() const
{
	return position;
}

bool Stack::remove(var obj)
{
	auto idx = indexOf(obj);

	if (idx != -1)
		return removeElement(idx);

	return false;
}

bool Stack::removeElement(int index)
{
	if (isPositiveAndBelow(index, position))
	{
		position = jmax<int>(0, position - 1);

		*items[index] = *items[position];
		items[position]->clear();
		return true;
	}

	return false;
}

void Stack::clear()
{
	for (auto i : items)
		i->clear();
	
	clearQuick();
}

void Stack::clearQuick()
{
	position = 0;
}

bool Stack::isEmpty() const
{
	return position == 0;
}

bool Stack::set(var obj)
{
	if(isEmpty())
		assign(position++, obj);
	else
	{
		auto idx = indexOf(obj);

		if(idx == -1)
		{
			if(isPositiveAndBelow(position, (int)numElements - 1))
				assign(position++, obj);
			else
				return false;
		}
			
		else
			assign(idx, obj);
	}

	return true;
}
}

} // namespace hise
