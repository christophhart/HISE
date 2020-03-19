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


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;

WrapType::WrapType(int size_) :
	ComplexType(),
	size(size_)
{
	finaliseAlignment();
}

void WrapType::dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
{
	auto v = juce::String(*reinterpret_cast<int*>(complexTypeStartPointer));

	s << v << "\n";
}



snex::jit::FunctionClass* WrapType::getFunctionClass()
{
	auto wId = NamespacedIdentifier("WrapOperators");
	auto wrapOperators = new FunctionClass(wId);

	auto wrapSize = size;

	auto assignOperator = new FunctionData();
	assignOperator->returnType = TypeInfo(this);
	assignOperator->addArgs("current", TypeInfo(this));
	assignOperator->addArgs("newValue", TypeInfo(Types::ID::Integer));
	assignOperator->id = wId.getChildId(wrapOperators->getSpecialSymbol(FunctionClass::AssignOverload));

	auto assign_asm = [wrapSize](InlineData* d_)
	{
		auto d = dynamic_cast<AsmInlineData*>(d_);

		auto& cc = d->gen.cc;

		auto thisObj = d->target;
		auto value = d->args[1];

		jassert(thisObj->getTypeInfo().getTypedComplexType<WrapType>() != nullptr);
		jassert(value->getType() == Types::ID::Integer);

		X86Mem ptr;

		if (thisObj->isMemoryLocation())
			ptr = thisObj->getAsMemoryLocation().cloneAdjustedAndResized(0, 4);
		else
			ptr = x86::ptr(PTR_REG_R(thisObj)).cloneAdjustedAndResized(0, 4);

		auto t = cc.newGpd();

		if (value->isMemoryLocation())
			cc.mov(t, INT_MEM(value));
		else
			cc.mov(t, INT_REG_R(value));

		if (isPowerOfTwo(wrapSize))
		{
			cc.and_(t, wrapSize - 1);
		}
		else
		{
			auto d = cc.newGpd();
			auto s = cc.newInt32Const(ConstPool::kScopeLocal, wrapSize);
			cc.cdq(d, t);
			cc.idiv(d, t, s);
			cc.mov(t, d);
		}

		cc.mov(ptr, t);

		return Result::ok();
	};

	assignOperator->inliner = new Inliner(assignOperator->id, assign_asm, {});

	wrapOperators->addFunction(assignOperator);

	auto castOperator = new FunctionData();
	castOperator->id = wId.getChildId(wrapOperators->getSpecialSymbol(FunctionClass::NativeTypeCast));
	castOperator->returnType = TypeInfo(Types::ID::Integer);
	castOperator->inliner = new Inliner(castOperator->id, [](InlineData* b)
	{
		auto d = b->toAsmInlineData();
		auto source = d->object;
		auto target = d->target;
		auto& cc = d->gen.cc;

		jassert(source->getTypeInfo().getTypedComplexType<WrapType>() != nullptr);
		jassert(target->getType() == Types::ID::Integer);

		//target->createRegister(cc);

		if (source->isMemoryLocation())
			target->setCustomMemoryLocation(source->getAsMemoryLocation(), source->isGlobalMemory());
		else
			target->setCustomMemoryLocation(x86::ptr(PTR_REG_R(source)), source->isGlobalMemory());

		return Result::ok();
	}, {});

	wrapOperators->addFunction(castOperator);

	return wrapOperators;
}

snex::InitialiserList::Ptr WrapType::makeDefaultInitialiserList() const
{
	return InitialiserList::makeSingleList(VariableStorage(0));
}

juce::Result WrapType::initialise(void* dataPointer, InitialiserList::Ptr initValues)
{
	if (initValues->size() != 1)
		return Result::fail("Can't initialise with more than one value");

	VariableStorage v;
	initValues->getValue(0, v);

	*reinterpret_cast<int*>(dataPointer) = v.toInt() % size;

	return Result::ok();
}

juce::String WrapType::toStringInternal() const
{
	juce::String w;
	w << "wrap<" << juce::String(size) << ">";
	return w;
}

bool WrapType::isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const
{
	if (nativeTargetType == Types::ID::Integer)
		return true;

	return ComplexType::isValidCastTarget(nativeTargetType, complexTargetType);
}

SpanType::SpanType(const TypeInfo& t, int size_) :
	elementType(t),
	size(size_)
{
	if(!t.isComplexType())
		finaliseAlignment();
}


bool SpanType::isSimdType(const TypeInfo& t)
{
	if (auto st = t.getTypedIfComplexType<SpanType>())
	{
		if (st->getElementType() == Types::ID::Float && st->getNumElements() == 4)
		{
			jassert(st->hasAlias());
			jassert(st->toString() == "float4");
			return true;
		}
	}

	return false;
}

void SpanType::finaliseAlignment()
{
	if (elementType.isComplexType())
		elementType.getComplexType()->finaliseAlignment();

	ComplexType::finaliseAlignment();
}

bool SpanType::forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer)
{
	if (elementType.isComplexType())
	{
		// must be aligned already...
		jassert((uint64_t)dataPointer % getRequiredAlignment() == 0);

		for (size_t i = 0; i < size; i++)
		{
			auto mPtr = ComplexType::getPointerWithOffset(dataPointer, getElementSize() * i);

			if (elementType.getComplexType()->forEach(t, typePtr, mPtr))
				return true;
		}
	}

	return false;
}

snex::jit::FunctionClass* SpanType::getFunctionClass()
{
	NamespacedIdentifier sId("SpanFunctions");
	auto st = new FunctionClass(sId);

	auto byteSize = getElementSize();
	int numElements = getNumElements();

	auto subscript = st->createSpecialFunction(FunctionClass::Subscript);
	subscript.returnType = getElementType();
	subscript.addArgs("index", TypeInfo(Types::ID::Dynamic));
	subscript.inliner = new Inliner(subscript.id, [byteSize](InlineData* d_)
	{
		auto d = d_->toAsmInlineData();
		d->gen.emitSpanReference(d->target, d->object, d->args[0], byteSize);
		return Result::ok();
	}, {});

	auto sizeFunction = new FunctionData();
	sizeFunction->id = NamespacedIdentifier("size");
	sizeFunction->returnType = TypeInfo(Types::ID::Integer);
	sizeFunction->inliner = new Inliner(sizeFunction->id, {}, [numElements](InlineData* d)
	{
		auto s = d->toSyntaxTreeData();
		auto value = VariableStorage(Types::ID::Integer, numElements);
		s->target = new Operations::Immediate(s->expression->location, value);
		return Result::ok();
	});

	st->addFunction(sizeFunction);

	return st;
}

snex::jit::TypeInfo SpanType::getElementType() const
{
	return elementType;
}

int SpanType::getNumElements() const
{
	return size;
}

size_t SpanType::getRequiredByteSize() const
{
	return getElementSize() * size;
}

size_t SpanType::getRequiredAlignment() const
{
	return elementType.getRequiredAlignment();
}

snex::InitialiserList::Ptr SpanType::makeDefaultInitialiserList() const
{
	if (elementType.isComplexType())
	{
		auto c = elementType.getComplexType()->makeDefaultInitialiserList();

		InitialiserList::Ptr n = new InitialiserList();
		n->addChildList(c);
		return n;
	}
	else
		return InitialiserList::makeSingleList(VariableStorage(getElementType().getType(), var(0.0)));
}

void SpanType::dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const
{
	intendLevel++;

	for (int i = 0; i < size; i++)
	{
		juce::String symbol;

		auto address = ComplexType::getPointerWithOffset(complexTypeStartPointer, i * getElementSize());

		if (elementType.isComplexType())
		{
			symbol << Types::Helpers::getIntendation(intendLevel);

			if (hasAlias())
				symbol << toString();
			else
				symbol << "Span";

			symbol << "[" << juce::String(i) << "]: \n";

			s << symbol;
			elementType.getComplexType()->dumpTable(s, intendLevel, dataStart, address);
		}
		else
		{
			symbol << "[" << juce::String(i) << "]";

			Types::Helpers::dumpNativeData(s, intendLevel, symbol, dataStart, address, getElementSize(), elementType.getType());
		}
	}

	intendLevel--;
}

juce::String SpanType::toStringInternal() const
{
	juce::String s = "span<";

	s << elementType.toString();
	s << ", " << size << ">";

	return s;
}

juce::Result SpanType::initialise(void* dataPointer, InitialiserList::Ptr initValues)
{
	juce::String e;

	if (initValues->size() != size && initValues->size() != 1)
	{

		e << "initialiser list size mismatch. Expected: " << juce::String(size);
		e << ", Actual: " << juce::String(initValues->size());

		return Result::fail(e);
	}

	for (int i = 0; i < size; i++)
	{
		auto indexToUse = initValues->size() == 1 ? 0 : i;

		if (elementType.isComplexType())
		{
			auto childList = initValues->createChildList(indexToUse);

			auto ok = elementType.getComplexType()->initialise(getPointerWithOffset(dataPointer, getElementSize() * i), childList);

			if (!ok.wasOk())
				return ok;
		}
		else
		{
			VariableStorage valueToUse;
			auto ok = initValues->getValue(indexToUse, valueToUse);

			if (!ok.wasOk())
				return ok;

			if (valueToUse.getType() != elementType.getType())
			{
				e << "type mismatch at index " + juce::String(i) << ": " << Types::Helpers::getTypeName(valueToUse.getType());
				return Result::fail(e);
			}

			ComplexType::writeNativeMemberType(dataPointer, getElementSize() * i, valueToUse);
		}
	}

	return Result::ok();
}

size_t SpanType::getElementSize() const
{
	if (elementType.isComplexType())
	{
		jassert(elementType.getComplexType()->isFinalised());

		auto alignment = elementType.getRequiredAlignment();
		int childSize = elementType.getRequiredByteSize();
		size_t elementPadding = 0;

		if (childSize % alignment != 0)
		{
			elementPadding = alignment - childSize % alignment;
		}

		return childSize + elementPadding;
	}
	else
		return elementType.getRequiredByteSize();
}


DynType::DynType(const TypeInfo& elementType_) :
	elementType(elementType_)
{

}

size_t DynType::getRequiredByteSize() const
{
	return sizeof(VariableStorage);
}

size_t DynType::getRequiredAlignment() const
{
	return 8;
}

void DynType::dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
{
	auto bytePtr = (uint8*)complexTypeStartPointer;

	int numElements = *reinterpret_cast<int*>(bytePtr + 4);
	uint8* actualDataPointer = *reinterpret_cast<uint8**>(bytePtr + 8);

	s << "size: " << juce::String(numElements);
	s << "\t Absolute: " << juce::String(reinterpret_cast<uint64_t>(bytePtr + 4)) << "\n";
	s << "data: " << juce::String(reinterpret_cast<uint64_t>(actualDataPointer));
	s << "\t Absolute: " << juce::String(reinterpret_cast<uint64_t>(bytePtr + 8)) << "\n";
	
	intentLevel++;

	for (int i = 0; i < numElements; i++)
	{
		if (elementType.isComplexType())
		{
			elementType.getComplexType()->dumpTable(s, intentLevel, dataStart, actualDataPointer);
		}
		else
		{
			juce::String idx = "[";
			idx << juce::String(i) << "]";

			Types::Helpers::dumpNativeData(s, intentLevel, idx, dataStart, actualDataPointer, elementType.getRequiredByteSize(), elementType.getType());
		}
		
		actualDataPointer += elementType.getRequiredByteSize();
	}

	intentLevel--;
}




snex::jit::FunctionClass* DynType::getFunctionClass()
{
	auto dynOperators = new FunctionClass(NamespacedIdentifier("DynFunctions"));

	auto& assignFunction = dynOperators->createSpecialFunction(FunctionClass::AssignOverload);
	assignFunction.addArgs("this", TypeInfo(this));
	assignFunction.addArgs("other", TypeInfo(Types::ID::Pointer, true));
	assignFunction.returnType = TypeInfo(this);

	assignFunction.inliner = new Inliner(assignFunction.id, [](InlineData* d_)
	{
		auto d = d_->toAsmInlineData();

		auto value = d->args[1];
		auto valueType = value->getTypeInfo();
		auto thisObj = d->target;

		auto& cc = d->gen.cc;

		jassert(thisObj->getTypeInfo().getTypedComplexType<DynType>() != nullptr);
		jassert(thisObj == d->args[0]);
		jassert(d->args.size() == 2);

		if (auto st = valueType.getTypedIfComplexType<SpanType>())
		{
			if(thisObj->isGlobalVariableRegister())
				thisObj->createMemoryLocation(cc);

			value->loadMemoryIntoRegister(cc);

			auto size = (int)st->getNumElements();

			X86Mem ptr;

			if (thisObj->isMemoryLocation())
				ptr = thisObj->getAsMemoryLocation();
			else
				ptr = x86::ptr(PTR_REG_R(thisObj));

			cc.mov(ptr.cloneAdjustedAndResized(0, 4), 0);
			cc.mov(ptr.cloneAdjustedAndResized(4, 4), size);
			cc.mov(ptr.cloneAdjustedAndResized(8, 8), PTR_REG_R(value));

			return Result::ok();
		}
		else
		{
			juce::String s;
			s << "Can't assign" << valueType.toString() << " to " << thisObj->getTypeInfo().toString();
			return Result::fail(s);
		}
	}, {});

	auto sizeFunction = new FunctionData();
	sizeFunction->id = dynOperators->getClassName().getChildId("size");
	sizeFunction->returnType = TypeInfo(Types::ID::Integer);
	
	sizeFunction->inliner = new Inliner(sizeFunction->id, [](InlineData* d_)
	{
		auto d = d_->toAsmInlineData();
		auto& cc = d->gen.cc;
		jassert(d->target->getType() == Types::ID::Integer);
		jassert(d->object->getTypeInfo().getTypedComplexType<DynType>() != nullptr);

		auto thisObj = d->object;

		d->target->createRegister(cc);
		
		X86Mem ptr;

		if (d->object->isMemoryLocation())
			ptr = d->object->getAsMemoryLocation().cloneAdjustedAndResized(4, 4);
		else
			ptr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(4, 4);

		cc.mov(INT_REG_W(d->target), ptr);

		return Result::ok();
	}, {});

	dynOperators->addFunction(sizeFunction);

	
	return dynOperators;
}

snex::InitialiserList::Ptr DynType::makeDefaultInitialiserList() const
{
	InitialiserList::Ptr n = new InitialiserList();

	n->addImmediateValue(VariableStorage(nullptr, 0));
	n->addImmediateValue(VariableStorage(0));

	return n;
}

juce::Result DynType::initialise(void* dataPointer, InitialiserList::Ptr initValues)
{
	VariableStorage ptr;
	initValues->getValue(0, ptr);

	jassert(ptr.getType() == Types::ID::Pointer);
	auto data = ptr.getDataPointer();

	VariableStorage s;
	initValues->getValue(1, s);

	memset(dataPointer, 0, 4);

	ComplexType::writeNativeMemberType(dataPointer, 4, s);
	ComplexType::writeNativeMemberType(dataPointer, 8, ptr);

	return Result::ok();
}

juce::String DynType::toStringInternal() const
{
	juce::String w;
	w << "dyn<" << elementType.toString() << ">";
	return w;
}

VariadicTypeBase::VariadicTypeBase(VariadicSubType::Ptr subType) : type(subType)
{
	type->variadicType = this;
}

size_t VariadicTypeBase::getRequiredByteSize() const
{
	size_t s = 0;

	for (auto t : types)
		s += t->getRequiredByteSize();

	return s;
}

snex::jit::VariadicTypeBase* VariadicTypeBase::getVariadicObjectFromInlineData(InlineData* d)
{
	if (auto rt = dynamic_cast<ReturnTypeInlineData*>(d))
	{
		if (rt->object == nullptr)
		{
			jassertfalse;
			return nullptr;
		}

		auto t = rt->object->getTypeInfo().getTypedIfComplexType<VariadicTypeBase>();
		jassert(t != nullptr);
		return t;
	}

	if (d->isHighlevel())
	{
		auto treeData = dynamic_cast<SyntaxTreeInlineData*>(d);

		if (treeData->object == nullptr)
		{
			jassertfalse;
			return nullptr;
		}

		auto t = treeData->object->getTypeInfo().getTypedIfComplexType<VariadicTypeBase>();
		jassert(t != nullptr);
		return t;
	}
	else
	{
		auto asmData = dynamic_cast<AsmInlineData*>(d);

		if (asmData->object == nullptr)
		{
			jassertfalse;
			return nullptr;
		}

		auto t = asmData->object.get()->getTypeInfo().getTypedIfComplexType<VariadicTypeBase>();
		jassert(t != nullptr);

		return t;
	}
}


int VariadicTypeBase::getNumSubTypes() const
{
	return types.size();
}

void VariadicTypeBase::addType(ComplexType::Ptr newType)
{
	types.add(newType);
}

size_t VariadicTypeBase::getOffsetForSubType(int index) const
{
	size_t bytes = 0;

	for (int i = 0; i < jmin(types.size(), index); i++)
	{
		bytes += types[i]->getRequiredByteSize();
	}

	return bytes;
}

snex::jit::FunctionClass* VariadicTypeBase::getFunctionClass()
{
	auto fc = new FunctionClass(type->variadicId);

	for (const auto& f : type->functions)
	{
		fc->addFunction(new FunctionData(f));
	}

	return fc;
}

juce::String VariadicTypeBase::toStringInternal() const
{
	juce::String s;
	s << type->variadicId.toString() << "<";

	for (auto t : types)
	{
		s << t->toString();

		if (t == types.getLast().get())
			s << ">";
		else
			s << ",";
	}

	return s;
}

size_t VariadicTypeBase::getRequiredAlignment() const
{
	if (types.isEmpty())
		return 0;
	else
		return types.getFirst()->getRequiredAlignment();
}


void VariadicTypeBase::finaliseAlignment()
{
	for (auto t : types)
		t->finaliseAlignment();

	ComplexType::finaliseAlignment();
}

void VariadicTypeBase::dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
{
	s << "variadic template " << toString() << "\n";

	intentLevel++;

	auto bytePtr = (uint8*)complexTypeStartPointer;

	for (int i = 0; i < types.size(); i++)
	{
		auto thisPtr = bytePtr + getOffsetForSubType(i);
		types[i]->dumpTable(s, intentLevel, dataStart, thisPtr);
	}

	intentLevel--;
}

juce::Result VariadicTypeBase::initialise(void* dataPointer, InitialiserList::Ptr initValues)
{
	uint8* bytePtr = reinterpret_cast<uint8*>(dataPointer);

	if (initValues->size() != types.size())
		return Result::fail("initialiser amount mismatch.");

	for (int i = 0; i < types.size(); i++)
	{
		auto childType = types[i];
		auto childInitialiser = initValues->getChild(i);

		auto r = childType->initialise(bytePtr, childInitialiser);

		if (!r.wasOk())
			return r;

		bytePtr += childType->getRequiredByteSize();
	}

	return Result::ok();
}

snex::InitialiserList::Ptr VariadicTypeBase::makeDefaultInitialiserList() const
{
	InitialiserList::Ptr p = new InitialiserList();

	for (auto t : types)
		p->addChildList(t->makeDefaultInitialiserList());

	return p;
}

bool VariadicTypeBase::forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer)
{
	if (typePtr.get() == this)
		return t(typePtr, dataPointer);

	for (auto childType : types)
	{
		if (childType->forEach(t, typePtr, dataPointer))
			return true;
	}

	return false;
}

snex::jit::ComplexType::Ptr VariadicTypeBase::getSubType(int index) const
{
	return types[index];
}

StructType::StructType(const NamespacedIdentifier& s) :
	id(s)
{

}

size_t StructType::getRequiredByteSize() const
{
	size_t s = 0;

	for (auto m : memberData)
		s += (m->typeInfo.getRequiredByteSize() + m->padding);

	return s;
}

juce::String StructType::toStringInternal() const
{
	return id.toString();
}

snex::jit::FunctionClass* StructType::getFunctionClass()
{
	auto mf = new FunctionClass(id);

	for (const auto& f : memberFunctions)
	{
		mf->addFunction(new FunctionData(f));
	}

	return mf;
}

juce::Result StructType::initialise(void* dataPointer, InitialiserList::Ptr initValues)
{
	int index = 0;

	for (auto m : memberData)
	{
		if (isPositiveAndBelow(index, initValues->size()))
		{
			auto mPtr = getMemberPointer(m, dataPointer);

			if (m->typeInfo.isComplexType())
			{
				auto childList = initValues->createChildList(index);
				auto ok = m->typeInfo.getComplexType()->initialise(mPtr, childList);

				if (!ok.wasOk())
					return ok;
			}
			else
			{
				VariableStorage childValue;
				initValues->getValue(index, childValue);

				if (m->typeInfo.getType() != childValue.getType())
					return Result::fail("type mismatch at index " + juce::String(index));

				ComplexType::writeNativeMemberType(mPtr, 0, childValue);
			}
		}

		index++;
	}

	return Result::ok();
}

bool StructType::forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer)
{
	if (typePtr.get() == this)
	{
		return t(this, dataPointer);
	}

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType())
		{
			auto mPtr = getMemberPointer(m, dataPointer);

			if (m->typeInfo.getComplexType()->forEach(t, typePtr, mPtr))
				return true;
		}
	}

	return false;
}

void StructType::dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const
{
	size_t offset = 0;
	intendLevel++;

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType())
		{
			s << Types::Helpers::getIntendation(intendLevel) << id.toString() << " " << m->id << "\n";
			m->typeInfo.getComplexType()->dumpTable(s, intendLevel, dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id));
		}
		else
		{
			Types::Helpers::dumpNativeData(s, intendLevel, id.getChildId(m->id).toString(), dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id), m->typeInfo.getRequiredByteSize(), m->typeInfo.getType());
		}
	}
	intendLevel--;
}

snex::InitialiserList::Ptr StructType::makeDefaultInitialiserList() const
{
	InitialiserList::Ptr n = new InitialiserList();

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType() && m->defaultList == nullptr)
			n->addChildList(m->typeInfo.getComplexType()->makeDefaultInitialiserList());
		else if (m->defaultList != nullptr)
			n->addChildList(m->defaultList);
		else
			jassertfalse;
	}

	return n;
}

void StructType::registerExternalAtNamespaceHandler(NamespaceHandler* handler)
{
	if (handler->getComplexType(id))
		return;

	if (isExternalDefinition)
	{
		handler->addSymbol(id, TypeInfo(this), NamespaceHandler::Struct);
		NamespaceHandler::ScopedNamespaceSetter sns(*handler, id);

		Array<TypeInfo> subList;

		for (auto m : memberData)
		{
			if (auto subClass = m->typeInfo.getTypedIfComplexType<StructType>())
			{
				subClass->registerExternalAtNamespaceHandler(handler);
			}
		}

		for (auto m : memberData)
		{
			auto mId = id.getChildId(m->id);
			auto mType = m->typeInfo;
			handler->addSymbol(mId, mType, NamespaceHandler::Variable);
		}

		FunctionClass::Ptr fc = getFunctionClass();

		for (auto fId : fc->getFunctionIds())
		{
			Array<FunctionData> data;
			fc->addMatchingFunctions(data, fId);

			for (auto d : data)
			{
				handler->addSymbol(d.id, d.returnType, NamespaceHandler::Function);
			}
		}
	}

	ComplexType::registerExternalAtNamespaceHandler(handler);
}

bool StructType::setDefaultValue(const Identifier& id, InitialiserList::Ptr defaultList)
{
	jassert(hasMember(id));

	for (auto& m : memberData)
	{
		if (m->id == id)
		{
			m->defaultList = defaultList;
			return true;
		}
	}

	return false;
}

bool StructType::hasMember(const Identifier& id) const
{
	for (auto m : memberData)
		if (m->id == id)
			return true;

	return false;
}

snex::jit::TypeInfo StructType::getMemberTypeInfo(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return m->typeInfo;
	}

	jassertfalse;
	return {};
}

void* StructType::getMemberPointer(Member* m, void* dataPointer)
{
	jassert(dataPointer != nullptr);
	return ComplexType::getPointerWithOffset(dataPointer, m->offset + m->padding);
}

size_t StructType::getRequiredAlignment() const
{
	if (auto f = memberData.getFirst())
	{
		return f->typeInfo.getRequiredAlignment();
	}

	return 0;
}

size_t StructType::getRequiredAlignment(Member* m)
{
	return m->typeInfo.getRequiredAlignment();
}

void StructType::finaliseAlignment()
{
	if (isFinalised())
		return;

	size_t offset = 0;

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType())
			m->typeInfo.getComplexType()->finaliseAlignment();

		m->offset = offset;

		auto alignment = getRequiredAlignment(m);
		m->padding = offset % alignment;
		offset += m->padding + m->typeInfo.getRequiredByteSize();
	}

	jassert(offset == getRequiredByteSize());

	ComplexType::finaliseAlignment();
}

snex::Types::ID StructType::getMemberDataType(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			m->typeInfo.getType();
	}

	jassertfalse;
	return Types::ID::Void;
}

bool StructType::isNativeMember(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return !m->typeInfo.isComplexType();
	}

	return false;
}

snex::jit::ComplexType::Ptr StructType::getMemberComplexType(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return m->typeInfo.getComplexType();
	}

	jassertfalse;
	return nullptr;
}

size_t StructType::getMemberOffset(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return m->padding + m->offset;
	}

	jassertfalse;
	return 0;
}

void StructType::addJitCompiledMemberFunction(const FunctionData& f)
{
	jassert(f.function == nullptr);

	memberFunctions.add(f);
}

bool StructType::injectMemberFunctionPointer(const FunctionData& f, void* fPointer)
{
	for (auto& m : memberFunctions)
	{
		if (m.matchesArgumentTypes(f, true))
		{
			m.function = fPointer;
			return true;
		}
	}

	return false;
}

}
}