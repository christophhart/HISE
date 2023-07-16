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
USE_ASMJIT_NAMESPACE;

SpanType::SpanType(const TypeInfo& t, int size_) :
	elementType(t),
	size(size_)
{
	if(!t.isComplexType() || t.getComplexType()->isFinalised())
		finaliseAlignment();
}


SpanType::~SpanType()
{
}

bool SpanType::isSimdType(const TypeInfo& t)
{
#if SNEX_ENABLE_SIMD
	if (auto st = t.getTypedIfComplexType<SpanType>())
	{
		return st->isSimd();
	}
#endif

	return false;
}

bool SpanType::isSimd() const
{
#if SNEX_ENABLE_SIMD
	if (getElementType() == Types::ID::Float && getNumElements() == 4)
	{
		jassert(hasAlias());
		jassert(toString() == "float4");
		return true;
	}
#endif

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

juce::Result SpanType::callDestructor(InitData& d)
{
	for (int i = 0; i < getNumElements(); i++)
	{
		if (auto typePtr = getElementType().getComplexType())
		{
			if (typePtr->hasDestructor())
			{
				InitData sd;
				ScopedPointer<SyntaxTreeInlineData> inner;
				sd.t = ComplexType::InitData::Type::Desctructor;

				auto offset = getElementSize() * i;

				if (d.dataPointer != nullptr)
				{
					sd.dataPointer = ComplexType::getPointerWithOffset(d.dataPointer, offset);
				}
				else
				{
					auto outer = d.functionTree->toSyntaxTreeData();
					jassert(outer != nullptr);
					inner = new SyntaxTreeInlineData(*outer);
					inner->object = new Operations::MemoryReference(outer->location, outer->object, elementType, (int)offset);
				}

				auto r = typePtr->callDestructor(sd);

				if (!r.wasOk())
					return r;
			}
		}
	}

	return Result::ok();
}

snex::jit::FunctionClass* SpanType::getFunctionClass()
{
	NamespacedIdentifier sId("span");
	auto st = new FunctionClass(sId);

	auto byteSize = getElementSize();
	int numElements = getNumElements();

	auto& subscript = st->createSpecialFunction(FunctionClass::Subscript);
	subscript.returnType = getElementType();
	subscript.addArgs("index", TypeInfo(Types::ID::Dynamic));
	subscript.inliner = new Inliner(subscript.id, [byteSize](InlineData* d_)
	{
		auto d = d_->toAsmInlineData();
		ignoreUnused(d);
		ASMJIT_ONLY(d->gen.emitSpanReference(d->target, d->object, d->args[0], byteSize));
		return Result::ok();
	}, {});

	

	{


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
	}

	{
		auto numElements = getNumElements();

		auto isSimdableFunc = new FunctionData();
		isSimdableFunc->id = st->getClassName().getChildId("isSimdable");
		isSimdableFunc->returnType = TypeInfo(Types::ID::Integer);
		isSimdableFunc->inliner = Inliner::createHighLevelInliner(isSimdableFunc->id, [numElements](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto isSimd = (int)(numElements % 4 == 0);

			auto value = VariableStorage(Types::ID::Integer, isSimd);
			d->target = new Operations::Immediate(d->expression->location, value);
			return Result::ok();
		});

		st->addFunction(isSimdableFunc);
	}

	{
		auto& toSimdFunction = st->createSpecialFunction(FunctionClass::ToSimdOp);

		toSimdFunction.returnType = TypeInfo(Types::ID::Dynamic, false, true);

#if SNEX_ASMJIT_BACKEND
		toSimdFunction.inliner = Inliner::createAsmInliner(toSimdFunction.id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			if (!d->gen.canVectorize())
				return Result::fail("Vectorization is deactivated");

			if (d->object->isMemoryLocation())
				d->target->setCustomMemoryLocation(d->object->getAsMemoryLocation(), d->object->isGlobalMemory());
			else
				d->target->setCustomMemoryLocation(x86::qword_ptr(PTR_REG_R(d->object)), d->object->isGlobalMemory());

			return Result::ok();
		});


		toSimdFunction.inliner->returnTypeFunction = [this](InlineData* d)
		{
			auto rt = dynamic_cast<ReturnTypeInlineData*>(d);

			auto& handler = rt->object->currentCompiler->namespaceHandler;

			auto float4Type = handler.getAliasType(NamespacedIdentifier("float4"));

			if (getNumElements() % 4 != 0)
				return Result::fail("Can't convert to SIMD");

			int numSimdElements = getNumElements() / 4;

			ComplexType::Ptr simdArrayType = new SpanType(float4Type, numSimdElements);
			simdArrayType = handler.registerComplexTypeOrReturnExisting(simdArrayType);
			rt->f.returnType = TypeInfo(simdArrayType, false, true);

			return Result::ok();
		};
#endif
	}

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
	auto elementSize = getElementSize();

	if (elementSize == 0)
		return 1;

	if (isSimd())
	{
		return 16;
	}

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

    int sizeToDump = jmin(128, size);
    
	for (int i = 0; i < sizeToDump; i++)
	{
		juce::String symbol;

		auto address = ComplexType::getPointerWithOffset(complexTypeStartPointer, i * getElementSize());

		s << "\n";

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

juce::Result SpanType::initialise(InitData d)
{
	juce::String e;

	if (d.initValues == nullptr)
	{
		return Result::fail("no init values");
	}

	if (d.initValues->size() != size && d.initValues->size() != 1)
	{

		e << "initialiser list size mismatch. Expected: " << juce::String(size);
		e << ", Actual: " << juce::String(d.initValues->size());

		return Result::fail(e);
	}

	for (int i = 0; i < size; i++)
	{
		auto indexToUse = d.initValues->size() == 1 ? 0 : i;

		if (elementType.isComplexType())
		{
			InitData c;

			auto offset = getElementSize() * i;

			c.dataPointer = getPointerWithOffset(d.dataPointer, offset);


			AssemblyMemory cm;

			if (d.asmPtr != nullptr)
			{
#if SNEX_ASMJIT_BACKEND
				cm = d.asmPtr->cloneWithOffset((int)offset);
				c.asmPtr = &cm;
#else
				jassertfalse;
				return Result::fail("MIR!!!");
#endif
			}
			


			c.initValues = d.initValues->createChildList(indexToUse);

			c.callConstructor = d.callConstructor;

			if (c.callConstructor)
			{
				if (!elementType.getComplexType()->hasDefaultConstructor())
				{
					String s;
					s << elementType.toString() << ": no default constructor";
					return Result::fail(s);
				}
			}

			auto ok = elementType.getComplexType()->initialise(c);

			if (!ok.wasOk())
				return ok;
		}
		else
		{
			if (d.asmPtr == nullptr)
			{
				VariableStorage valueToUse;
				auto ok = d.initValues->getValue(indexToUse, valueToUse);

				if (!ok.wasOk())
					return ok;

				if (valueToUse.getType() != elementType.getType())
				{
					e << "type mismatch at index " + juce::String(i) << ": " << Types::Helpers::getTypeName(valueToUse.getType());
					return Result::fail(e);
				}

				ComplexType::writeNativeMemberType(d.dataPointer, (int)getElementSize() * i, valueToUse);
			}
			else
			{
				ComplexType::writeNativeMemberTypeToAsmStack(d, indexToUse, (int)getElementSize() * i, (int)getElementSize());
			}
			
		}
	}

	return Result::ok();
}

size_t SpanType::getElementSize() const
{
	if (elementType.isComplexType())
	{
		jassert(elementType.getComplexType()->isFinalised());

		int childSize = elementType.getRequiredByteSizeNonZero();

		auto alignment = elementType.getRequiredAlignment();
		
		if (alignment != 0)
		{
			jassert(alignment != 0);

			size_t elementPadding = 0;

			if (childSize % alignment != 0)
			{
				elementPadding = alignment - (size_t)childSize % alignment;
			}

			childSize += (int)elementPadding;
		}

		return childSize;
	}
	else
		return elementType.getRequiredByteSize();
}

DynType::DynType(const TypeInfo& elementType_) :
	elementType(elementType_)
{
	ComplexType::finaliseAlignment();
}

size_t DynType::getRequiredByteSize() const
{
	return sizeof(VariableStorage);
}

size_t DynType::getRequiredAlignment() const
{
	return 16;
}

void DynType::dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
{
	auto bytePtr = (uint8*)complexTypeStartPointer;

	int numElements = *reinterpret_cast<int*>(bytePtr + 4);
	uint8* actualDataPointer = *reinterpret_cast<uint8**>(bytePtr + 8);

	

	s << "\t{ size: " << juce::String(numElements);

#if SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP
	s << ", data: 0x" << String::toHexString(reinterpret_cast<uint64_t>(actualDataPointer)).toUpperCase();
#endif
	//s << "\t Absolute: " << String::toHexString(reinterpret_cast<uint64_t>(bytePtr) + 8) << "\n";
	
	s << " }\n";

	intentLevel++;

	auto numElementsToDump = jmin(64, numElements);

	for (int i = 0; i < numElementsToDump; i++)
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
			if (!s.endsWithChar('\n'))
				s << '\n';
		}

		actualDataPointer += elementType.getRequiredByteSize();
	}

	if (numElementsToDump != numElements)
	{
		s << "[...]\n";
	}

	intentLevel--;
}




snex::jit::FunctionClass* DynType::getFunctionClass()
{
	auto dynOperators = new FunctionClass(NamespacedIdentifier("dyn"));

	auto assignFunction = new FunctionData();
	assignFunction->id = dynOperators->getClassName().getChildId("referTo");
	//assignFunction->addArgs("this", TypeInfo(this));
	assignFunction->addArgs("other", TypeInfo(Types::ID::Pointer, true));
	assignFunction->addArgs("size", TypeInfo(Types::ID::Integer));
	assignFunction->addArgs("offset", TypeInfo(Types::ID::Integer));

	//assignFunction->setDefaultParameter("size", VariableStorage(-1));
	assignFunction->setDefaultParameter("offset", VariableStorage(0));

	assignFunction->returnType = TypeInfo(this);

#if SNEX_ASMJIT_BACKEND
	assignFunction->inliner = new Inliner(assignFunction->id, [](InlineData* d_)
	{
		auto setToZeroIfImmediate = [](int& vToChange, AssemblyRegister::Ptr& p, int limit)
		{
			if (p != nullptr)
			{
				if (p->isImmediate())
				{
					auto newSize = p->getImmediateIntValue();

					if (isPositiveAndBelow(newSize, limit + 1))
					{
						vToChange = newSize;
						p = nullptr;
						return true;
					}
					else if (newSize == -1)
					{
						vToChange = limit;
						p = nullptr;
						return true;
					}
					else
					{
						vToChange = newSize;
						p = nullptr;
						return false;
					}
				}
			}

			return true;
		};

		auto d = d_->toAsmInlineData();
		auto& cc = d->gen.cc;

		auto value = d->args[0];
		auto valueType = value->getTypeInfo();
		auto isDyn = valueType.getTypedIfComplexType<DynType>() != nullptr;
		auto thisObj = d->object;
		
		auto offset = 0;
		auto fixSize = -1;
		auto totalSize = -1;

		auto sizeReg = d->args[1];
		auto offsetReg = d->args[2];


		if (auto st = valueType.getTypedIfComplexType<SpanType>())
		{
			fixSize = (int)st->getNumElements();
			totalSize = fixSize;
		}

		if (sizeReg != nullptr && sizeReg->getType() != Types::ID::Integer)
			return Result::fail("Can't use non-integer values as size argument to referTo()");
		if (offsetReg != nullptr && offsetReg->getType() != Types::ID::Integer)
			return Result::fail("Can't use non-integer values as offset argument to referTo()");


		if (!isDyn)
		{
			if (!setToZeroIfImmediate(fixSize, sizeReg, totalSize))
				return Result::fail("illegal size: " + String(fixSize));
			if (!setToZeroIfImmediate(offset, offsetReg, totalSize - fixSize))
				return Result::fail("illegal offset: " + String(offset));
		}
		else
		{
			setToZeroIfImmediate(fixSize, sizeReg, INT_MAX-1);
			setToZeroIfImmediate(offset, offsetReg, INT_MAX-1);
		}
		
		jassert(thisObj->getTypeInfo().getTypedComplexType<DynType>() != nullptr);
		jassert(d->args.size() >= 1);
		
		value->loadMemoryIntoRegister(cc);
		auto dynSourcePtr = x86::ptr(PTR_REG_R(value));
		X86Mem ptr;

		if (thisObj->isMemoryLocation())
			ptr = thisObj->getAsMemoryLocation();
		else
			ptr = x86::ptr(PTR_REG_R(thisObj));

		cc.mov(ptr.cloneAdjustedAndResized(0, 4), 0);

		

		if (sizeReg != nullptr)
			cc.mov(ptr.cloneAdjustedAndResized(4, 4), INT_REG_R(sizeReg));
		else if (fixSize >= 0)
			cc.mov(ptr.cloneAdjustedAndResized(4, 4), fixSize);
		else
		{
			jassert(isDyn);
			auto tmp = cc.newGpd();
			cc.mov(tmp, dynSourcePtr.cloneAdjustedAndResized(4, 4));
			cc.mov(ptr.cloneAdjustedAndResized(4, 4), tmp);
		}

		if (isDyn)
		{
			auto tmp = cc.newGpq();
			cc.mov(tmp, dynSourcePtr.cloneAdjustedAndResized(8, 8));
			cc.mov(ptr.cloneAdjustedAndResized(8, 8), tmp);
		}
		else
		{
			cc.mov(ptr.cloneAdjustedAndResized(8, 8), PTR_REG_R(value));
		}

		if (offsetReg != nullptr)
			cc.add( ptr.cloneAdjustedAndResized(8, 8), PTR_REG_R(offsetReg));
		else if (offset != 0)
			cc.add(ptr.cloneAdjustedAndResized(8, 8), offset * 4);

		if (isDyn && d->gen.isRuntimeErrorCheckEnabled(d->object->getScope()))
		{
			auto testSize = sizeReg != nullptr || (fixSize >= 0);
			auto testOffset = offsetReg != nullptr || (offset > 0);

			auto checkReg = cc.newGpd();
			auto limitReg = cc.newGpd();
			auto error = cc.newLabel();
			auto ok = cc.newLabel();
			auto sourceSizePtr = dynSourcePtr.cloneAdjustedAndResized(4, 4);
			auto destSizePtr = ptr.cloneAdjustedAndResized(4, 4);

			if (testSize)
			{
				cc.mov(checkReg, destSizePtr);
				cc.mov(limitReg, sourceSizePtr);
				cc.cmp(checkReg, limitReg);
				cc.mov(checkReg, (int)RuntimeError::ErrorType::DynReferIllegalSize);
				cc.ja(error);
			}

			if (testOffset)
			{
				cc.mov(checkReg, sourceSizePtr);
				cc.sub(checkReg, destSizePtr);

				if (offset > 0)
					cc.sub(checkReg, offset);
				else
					cc.sub(checkReg, INT_REG_R(offsetReg));

				cc.cmp(checkReg, 0);
				cc.mov(checkReg, (int)RuntimeError::ErrorType::DynReferIllegalOffset);
				cc.js(error);
			}

			cc.jmp(ok);

			cc.bind(error);
			auto flag = d->object->getScope()->getGlobalScope()->getRuntimeErrorFlag();

			cc.mov(limitReg.r64(), flag);

			auto ptr = x86::ptr(limitReg.r64()).cloneResized(4);

			auto loc = d->gen.location.getXYPosition();

			cc.mov(ptr, checkReg);
			cc.mov(ptr.cloneAdjusted(4), loc.x);
			cc.mov(ptr.cloneAdjusted(8), loc.y);

			if(sizeReg != nullptr)
				cc.mov(ptr.cloneAdjusted(12), INT_REG_R(sizeReg));
			else
				cc.mov(ptr.cloneAdjusted(12), fixSize);

			if(offsetReg != nullptr)
				cc.mov(ptr.cloneAdjusted(16), INT_REG_R(offsetReg));
			else
				cc.mov(ptr.cloneAdjusted(16), offset);
			

			cc.bind(ok);
		}

		d->target = d->object;

		return Result::ok();
		
	}, {});
#endif

	dynOperators->addFunction(assignFunction);


	{
		auto sizeFunction = new FunctionData();
		sizeFunction->id = dynOperators->getClassName().getChildId("size");
		sizeFunction->returnType = TypeInfo(Types::ID::Integer);

#if SNEX_ASMJIT_BACKEND
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
#endif

		dynOperators->addFunction(sizeFunction);
	}


	{
#if SNEX_ASMJIT_BACKEND
		auto& toSimdFunction = dynOperators->createSpecialFunction(FunctionClass::ToSimdOp);

		toSimdFunction.inliner = Inliner::createAsmInliner(toSimdFunction.id, [this](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			if (!d->gen.canVectorize())
				return Result::fail("Vectorization is deactivated");

			auto& cc = d->gen.cc;

			auto mem = cc.newStack((uint32_t)getRequiredByteSize(), (uint32_t)getRequiredAlignment());

			auto dataReg = cc.newGpq();
			auto sizeReg = cc.newGpd();

			if (d->object->isMemoryLocation())
				cc.lea(dataReg, d->object->getAsMemoryLocation());
			else
				cc.mov(dataReg, PTR_REG_R(d->object));

			cc.mov(sizeReg, x86::ptr(dataReg).cloneAdjustedAndResized(4, 4));
			cc.sar(sizeReg, 2);
			cc.mov(mem.cloneAdjustedAndResized(4, 4), sizeReg);
			cc.mov(dataReg, x86::ptr(dataReg).cloneAdjustedAndResized(8, 8));
			cc.mov(mem.cloneAdjustedAndResized(8, 8), dataReg);

			d->target->setCustomMemoryLocation(mem, false);

			return Result::ok();
		});


		toSimdFunction.inliner->returnTypeFunction = [](InlineData* d)
		{
			auto rt = dynamic_cast<ReturnTypeInlineData*>(d);

			auto& handler = rt->object->currentCompiler->namespaceHandler;
			auto float4Type = handler.getAliasType(NamespacedIdentifier("float4"));
			ComplexType::Ptr dynType = new DynType(float4Type.withModifiers(false, false, false));
			dynType = handler.registerComplexTypeOrReturnExisting(dynType);
			rt->f.returnType = TypeInfo(dynType, false, true);
			
			return Result::ok();
		};
#endif
	}

	{
		

		auto isSimdableFunc = new FunctionData();
		isSimdableFunc->id = dynOperators->getClassName().getChildId("isSimdable");
		isSimdableFunc->returnType = TypeInfo(Types::ID::Integer);

#if SNEX_ASMJIT_BACKEND
		isSimdableFunc->inliner = Inliner::createAsmInliner(isSimdableFunc->id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto& cc = d->gen.cc;
			
			d->target->createRegister(cc);

			auto sizeReg = cc.newGpd();
			auto dataReg = cc.newGpq();
			
			auto target = INT_REG_W(d->target);

			if (d->object->isMemoryLocation())
			{
				cc.mov(sizeReg, d->object->getAsMemoryLocation().cloneAdjustedAndResized(4, 4));
				cc.mov(dataReg, d->object->getAsMemoryLocation().cloneAdjustedAndResized(8, 8));
			}
			else
			{
				cc.mov(sizeReg, x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(4, 4));
				cc.mov(dataReg, x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(8, 8));
			}

			auto dTarget = cc.newGpd();
			cc.xor_(dTarget, dTarget);
			cc.test(dataReg.r8(), 15);
			cc.sete(dTarget.r8());

			cc.xor_(target, target);
			cc.test(sizeReg.r8(), 3);
			cc.sete(target.r8());
			cc.and_(target, dTarget);

			return Result::ok();
		});
#endif

		dynOperators->addFunction(isSimdableFunc);
	}

	{
		auto& subscriptFunction = dynOperators->createSpecialFunction(FunctionClass::Subscript);
		subscriptFunction.returnType = elementType.withModifiers(false, true, false);
		subscriptFunction.addArgs("this", TypeInfo(this));
		subscriptFunction.addArgs("index", TypeInfo(Types::ID::Integer));

		auto t = elementType;

#if SNEX_ASMJIT_BACKEND
		subscriptFunction.inliner = Inliner::createAsmInliner(subscriptFunction.id, [t](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto& cc = d->gen.cc;

			auto target = d->target;
			auto dynReg = d->args[0];
			auto index = d->args[1];
			auto limit = cc.newGpd();

			

			jassert(dynReg->getTypeInfo().getTypedComplexType<DynType>() != nullptr);
			jassert(target->getTypeInfo() == t);

			auto gs = d->target->getScope()->getGlobalScope();
			auto safeCheck = gs->isRuntimeErrorCheckEnabled();

			auto okBranch = cc.newLabel();
			auto errorBranch = cc.newLabel();

			// Skip safe checks for index objects...
			//safeCheck &= (index->getTypeInfo().getTypedIfComplexType<IndexBase>() == nullptr);

			if (safeCheck)
			{
				X86Mem p;

				dynReg->loadMemoryIntoRegister(cc);

				p = x86::ptr(PTR_REG_R(dynReg)).cloneAdjustedAndResized(4, 4);

				cc.setInlineComment("Bound check for dyn");
				cc.mov(limit, p);
				
				if (index->isMemoryLocation())
				{
					if (IS_IMM(index))
						cc.cmp(limit, (int64_t)INT_IMM(index));
					else
						cc.cmp(limit, INT_MEM(index));
				}
				else
					cc.cmp(limit, INT_REG_R(index));

				cc.jg(okBranch);


				auto flagReg = cc.newGpd();

				auto errorMemReg = cc.newGpq();
				cc.mov(errorMemReg, gs->getRuntimeErrorFlag());

				auto errorFlag = x86::ptr(errorMemReg).cloneResized(4);

				cc.mov(flagReg, (int)RuntimeError::ErrorType::DynAccessOutOfBounds);

				cc.mov(errorFlag, flagReg);

				auto& l = d->gen.location;

				cc.mov(flagReg, (int)l.getLine());
				cc.mov(errorFlag.cloneAdjustedAndResized(4, 4), flagReg);
				cc.mov(flagReg, (int)l.getColNumber());
				cc.mov(errorFlag.cloneAdjustedAndResized(8, 4), flagReg);
				
				if(index->isImmediate())
					cc.mov(flagReg, (int64_t)INT_IMM(index));
				else if (index->isMemoryLocation())
					cc.mov(flagReg, INT_MEM(index));
				else
					cc.mov(flagReg, INT_REG_R(index));
				
				cc.mov(errorFlag.cloneAdjustedAndResized(12, 4), flagReg);
				cc.mov(errorFlag.cloneAdjustedAndResized(16, 4), limit);

				cc.jmp(errorBranch);
				cc.bind(okBranch);
			}

			auto endLabel = cc.newLabel();

			cc.setInlineComment("dyn_ref");
			d->gen.emitSpanReference(target, dynReg, index, t.getRequiredByteSize());
			

			if (safeCheck)
			{
				auto memReg = cc.newGpq();
				
				cc.lea(memReg, target->getMemoryLocationForReference());

				target->setCustomMemoryLocation(x86::ptr(memReg).cloneResized((uint32_t)t.getRequiredByteSize()), target->isGlobalMemory());
				
				cc.jmp(endLabel);
				cc.bind(errorBranch);
				auto nirvana = cc.newStack((uint32_t)t.getRequiredByteSize(), (uint32_t)t.getRequiredAlignment());
				
				cc.lea(memReg, nirvana);

				cc.bind(endLabel);
				cc.nop();
			}
				
			return Result::ok();
		});
#endif
	}


	return dynOperators;
}

snex::InitialiserList::Ptr DynType::makeDefaultInitialiserList() const
{
	InitialiserList::Ptr n = new InitialiserList();

	n->addImmediateValue(VariableStorage(nullptr, 0));
	n->addImmediateValue(VariableStorage(0));

	return n;
}

juce::Result DynType::initialise(InitData d)
{
	if (d.asmPtr == nullptr)
	{
		VariableStorage ptr;
		d.initValues->getValue(0, ptr);

		jassert(ptr.getType() == Types::ID::Pointer);
		
		VariableStorage s;
		d.initValues->getValue(1, s);

		memset(d.dataPointer, 0, 4);

		ComplexType::writeNativeMemberType(d.dataPointer, 4, s);
		ComplexType::writeNativeMemberType(d.dataPointer, 8, ptr);
	}
	else
	{
		auto source = dynamic_cast<Operations::Expression*>(d.initValues->getExpression(0));

		jassert(source != nullptr);
		jassert(source->getTypeInfo().getTypedIfComplexType<SpanType>() != nullptr);
		jassert(d.initValues->size() == 1);

		auto numElements = source->getTypeInfo().getTypedComplexType<SpanType>()->getNumElements();

		d.initValues->addImmediateValue(numElements);

		ComplexType::writeNativeMemberTypeToAsmStack(d, 1, 4, 4);
		ComplexType::writeNativeMemberTypeToAsmStack(d, 0, 8, 8);
	}

	return Result::ok();
}

juce::String DynType::toStringInternal() const
{
	juce::String w;
	w << "dyn<" << elementType.toString() << ">";
	return w;
}

StructType::StructType(const NamespacedIdentifier& s, const Array<TemplateParameter>& tp) :
	id(s),
	templateParameters(tp)
{
#if JUCE_DEBUG
	for (auto& p : templateParameters)
	{
		jassert(!p.isTemplateArgument());
		jassert(p.argumentId.isValid());
	}
#endif
}

StructType::~StructType()
{
}

size_t StructType::getRequiredByteSize() const
{
	if (externalyDefinedSize != 0)
		return externalyDefinedSize;

	size_t s = 0;

	for (auto m : memberData)
	{
		jassert(m->typeInfo.getTypedIfComplexType<TemplatedComplexType>() == nullptr);
			

		s += (m->typeInfo.getRequiredByteSize() + m->padding);
	}

	auto alignment = getRequiredAlignment();

	if (alignment > 0)
	{
		auto missingBytesForAlignment = s % alignment;

		if (missingBytesForAlignment != 0)
		{
			s += (alignment - missingBytesForAlignment);
		}
	}

	

	return s;
}

juce::String StructType::toStringInternal() const
{
	juce::String s;


	auto parts = id.getIdList();

	NamespacedIdentifier current;

	for (int i = 0; i < parts.size(); i++)
	{
		current = current.getChildId(parts[i]);

		s << parts[i];

		auto filtered = TemplateParameter::ListOps::filter(templateParameters, current);

		if (!filtered.isEmpty())
		{
			s << TemplateParameter::ListOps::toString(filtered, false);
		}

		if (i != (parts.size() - 1))
			s << "::";
	}

	return s;
}

snex::jit::FunctionClass* StructType::getFunctionClass()
{
	auto mf = new FunctionClass(id);

	for (auto b : baseClasses)
	{
		jassert(b->baseClass != nullptr);

		for (const auto& f : b->baseClass->memberFunctions)
		{
			auto copy = new FunctionData(f);
			//copy->id = id.getChildId(f.id.getIdentifier());
			mf->addFunction(copy);
		}
	}

	for (const auto& f : memberFunctions)
	{
		mf->addFunction(new FunctionData(f));
	}

	return mf;
}



juce::Result StructType::initialise(InitData d)
{
	int index = 0;

	if (!d.callConstructor && d.initValues->size() != memberData.size())
	{
		String s;
		s << "initialiser mismatch: ";
		s << d.initValues->toString();
		s << " (expected " << memberData.size() << ")";

		return Result::fail(s);
	}

	for (auto m : memberData)
	{
		if (isPositiveAndBelow(index, d.initValues->size()))
		{
			if (m->typeInfo.isComplexType())
			{
				auto memberHasConstructor = m->typeInfo.getComplexType()->hasConstructor();

				

				InitData c;
				c.initValues = d.initValues->createChildList(index);
				c.callConstructor = d.callConstructor && memberHasConstructor;

				AssemblyMemory cm;

				if (d.asmPtr == nullptr)
					c.dataPointer = getMemberPointer(m, d.dataPointer);
				else
				{
#if SNEX_ASMJIT_BACKEND
					cm = d.asmPtr->cloneWithOffset((int)(m->offset + m->padding));
					c.asmPtr = &cm;
#else
					jassertfalse;
#endif
				}

				auto ok = m->typeInfo.getComplexType()->initialise(c);

				if (!ok.wasOk())
				{
					if (memberHasConstructor && !d.callConstructor)
						continue;

					return ok;
				}
			}
			else if(!d.callConstructor) // don't write native members when there's a constructor...
			{
				if (d.asmPtr == nullptr)
				{
					auto mPtr = getMemberPointer(m, d.dataPointer);

					VariableStorage childValue;
					d.initValues->getValue(index, childValue);

					if (m->typeInfo.getType() != childValue.getType())
						return Result::fail("type mismatch at index " + juce::String(index));

					ComplexType::writeNativeMemberType(mPtr, 0, childValue);
				}
				else
				{
					auto offset = m->offset + m->padding;
					auto size = m->typeInfo.getRequiredByteSize();

					ComplexType::writeNativeMemberTypeToAsmStack(d, index, (int)offset, (int)size);
				}
			}
		}

		index++;
	}

	if (d.callConstructor)
	{
		return callConstructor(d);
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
	intendLevel++;

	if (customDumpFunction)
	{
		for (int i = 0; i < intendLevel; i++)
			s << " ";

		s << customDumpFunction(complexTypeStartPointer);
		return;
	}

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType())
		{
			s << "\n|" << Types::Helpers::getIntendation(intendLevel) << m->typeInfo.toString() << " " << id.toString() << "::" << m->id;
			m->typeInfo.getComplexType()->dumpTable(s, intendLevel, dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id));
		}
		else
		{
			s << "\n";
			Types::Helpers::dumpNativeData(s, intendLevel, id.getChildId(m->id).toString(), dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id), m->typeInfo.getRequiredByteSize(), m->typeInfo.getType());
		}
	}


	intendLevel--;
}

snex::InitialiserList::Ptr StructType::makeDefaultInitialiserList() const
{
	jassert(isFinalised());

	InitialiserList::Ptr n = new InitialiserList();

	if (externalyDefinedSize != 0)
	{
		//n->addChild(new InitialiserList::ExternalInitialiser());
		return n;
	}

	

	for (auto m : memberData)
	{
		if (m->typeInfo.isComplexType() && m->defaultList == nullptr)
		{
			if(auto childList = m->typeInfo.getComplexType()->makeDefaultInitialiserList())
				n->addChildList(childList);
		}
			
		else if (m->defaultList != nullptr)
			n->addChildList(m->defaultList);
		else
		{
			// Look in the base class for a default initialiser...
			for (auto b : baseClasses)
			{
				for (auto bm : *b)
				{
					if (bm->id == m->id)
					{
						jassert(bm->defaultList != nullptr);
						n->addChildList(bm->defaultList);
					}
				}
			}
		}
	}

	return n;
}

bool StructType::createDefaultConstructor()
{
	if (hasConstructor())
		return false;

	return true;
}

void StructType::registerExternalAtNamespaceHandler(NamespaceHandler* handler, const String& description)
{
	if (handler->getComplexType(id))
		return;

	if (isExternalDefinition && templateParameters.isEmpty())
	{
		NamespaceHandler::ScopedNamespaceSetter sns(*handler, id.getParent());

		handler->addSymbol(id, TypeInfo(this), NamespaceHandler::Struct, {});
		NamespaceHandler::ScopedNamespaceSetter sns2(*handler, id);

		Array<TypeInfo> subList;

		for (auto m : memberData)
		{
			if (auto subClass = m->typeInfo.getTypedIfComplexType<StructType>())
			{
				subClass->registerExternalAtNamespaceHandler(handler, description);
			}
		}

		for (auto m : memberData)
		{
			auto mId = id.getChildId(m->id);
			auto mType = m->typeInfo;

			handler->addSymbol(mId, mType, NamespaceHandler::Variable, NamespaceHandler::SymbolDebugInfo::fromString(m->comment, m->visibility));
		}

		FunctionClass::Ptr fc = getFunctionClass();

		for (auto fId : fc->getFunctionIds())
		{
			Array<FunctionData> data;
			fc->addMatchingFunctions(data, fId);

			for (auto d : data)
			{
				handler->addSymbol(d.id, d.returnType, NamespaceHandler::Function, 
					NamespaceHandler::SymbolDebugInfo::fromString(d.description));
				handler->setSymbolCode(d.id, d.getCodeToInsert());
			}
		}
	}

	ComplexType::registerExternalAtNamespaceHandler(handler, {});
}

bool StructType::setDefaultValue(const Identifier& id, InitialiserList::Ptr defaultList)
{
	jassert(hasMember(id));

	for (auto& m : memberData)
	{
		if (m->id == id)
		{
			if (!m->typeInfo.isComplexType())
			{
				VariableStorage dv;
				defaultList->getValue(0, dv);


				auto memberType = m->typeInfo.getType();
				auto defaultType = dv.getType();

				if (memberType != defaultType)
				{
					dv = VariableStorage(memberType, dv.toDouble());

					defaultList = new InitialiserList();
					defaultList->addImmediateValue(dv);
				}
			}

			m->defaultList = defaultList;
			return true;
		}
	}

	return false;
}

bool StructType::setTypeForDynamicReturnFunction(FunctionData& functionDataWithReturnType)
{
	for (auto& m : memberFunctions)
	{
		if (!m.returnType.isDynamic())
			continue;

		if(m.id == functionDataWithReturnType.id &&
		   m.matchesTemplateArguments(functionDataWithReturnType.templateParameters) &&
		   m.matchesArgumentTypes(functionDataWithReturnType, false))
		{
			m = functionDataWithReturnType;
			return true;
		}
	}

	return false;
}

bool validMemberType(const TypeInfo& member, const TypeInfo& t)
{
	if (member == t)
		return true;

	if (auto st = member.getTypedIfComplexType<StructType>())
	{
		WrapBuilder::InnerData mId(st, WrapBuilder::OpaqueType::GetSelfAsObject);

		if (mId.resolve())
		{
			WrapBuilder::InnerData tId(t.getTypedIfComplexType<StructType>(), WrapBuilder::OpaqueType::GetSelfAsObject);

			if (tId.resolve())
			{
				return tId.st == mId.st;
			}
		}
	}

	if (auto sp = member.getTypedIfComplexType<SpanType>())
	{
		return sp->getElementType() == t;
	}

	return false;
}

bool StructType::hasMemberAtOffset(int offset, const TypeInfo& type) const
{
	bool ok = false;

	for (auto m : memberData)
	{
		if (m->offset + m->padding == offset)
		{
			ok |= validMemberType(m->typeInfo, type);
		}
	}

	return ok;
}

snex::jit::ComplexType::Ptr StructType::createSubType(SubTypeConstructData* sd)
{
	auto cId = sd->id;

	if (cId.isExplicit())
		cId = id.getChildId(cId.getIdentifier());

	auto st = sd->handler->getSymbolType(cId);

	if (st == NamespaceHandler::Struct || st == NamespaceHandler::TemplatedClass)
	{
		if (st == NamespaceHandler::Struct)
		{
			jassertfalse;
			ComplexType::Ptr p = new StructType(cId, {});
			p = sd->handler->registerComplexTypeOrReturnExisting(p);
			return p;
		}
		else
		{
			TemplateParameter::List l = getTemplateInstanceParameters();
			l.addArray(sd->l);
			return sd->handler->createTemplateInstantiation({ cId, {} }, l, sd->r);
		}
	}

	return {};
}

bool StructType::hasMember(const Identifier& id) const
{
	for (auto m : memberData)
		if (m->id == id)
			return true;

	return false;
}

bool StructType::hasMember(int index) const
{
	return isPositiveAndBelow(index, memberData.size());
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
	if (externalyDefinedSize != 0)
		return 16; // magic!

	if (auto f = memberData.getFirst())
	{
		return f->typeInfo.getRequiredAlignment();
	}

	return 1;
}

size_t StructType::getRequiredAlignment(Member* m)
{
	return m->typeInfo.getRequiredAlignmentNonZero();
}

bool StructType::hasDestructor()
{
	if (ComplexType::hasDestructor())
		return true;

	for (auto m : memberData)
	{
		if (auto p = m->typeInfo.getTypedIfComplexType<ComplexType>())
		{
			if (p->hasDestructor())
				return true;
		}
	}

	return false;
}

juce::Result StructType::callDestructor(InitData& d)
{
	if (ComplexType::hasDestructor())
	{
		auto r = ComplexType::callDestructor(d);

		if (!r.wasOk())
			return r;
	}

	// Deinitialize in the reverse order...
	for (int i = memberData.size() - 1; i >= 0; i--)
	{
		if (auto typePtr = memberData[i]->typeInfo.getTypedIfComplexType<ComplexType>())
		{
			if (typePtr->hasDestructor())
			{
				ScopedPointer<SyntaxTreeInlineData> inner;
				
				InitData sd;
				sd.t = ComplexType::InitData::Type::Desctructor;

				auto offset = memberData[i]->offset + memberData[i]->padding;

				if (d.dataPointer != nullptr)
				{
					sd.dataPointer = ComplexType::getPointerWithOffset(d.dataPointer, offset);
				}
				else
				{
					jassert(d.functionTree != nullptr);
					auto outer = d.functionTree->toSyntaxTreeData();
					inner = new SyntaxTreeInlineData(*d.functionTree->toSyntaxTreeData());
					inner->expression = new Operations::MemoryReference(outer->location, outer->expression, memberData[i]->typeInfo, (int)offset);
				}

				sd.functionTree = inner.get();
				auto r = typePtr->callDestructor(sd);

				if (!r.wasOk())
					return r;
			}
		}
	}
	
	return Result::ok();
}

bool StructType::hasConstructor()
{
	// You need to supply a constructor that fills the data pointer with the structure
	if (externalyDefinedSize != 0)
	{
		// If this isn't specified, you will not be able to intialise the opaque data set...
		jassert(ComplexType::hasConstructor());
		return true;
	}

	if (ComplexType::hasConstructor())
		return true;

	for (auto m : memberData)
	{
		if (auto t = m->typeInfo.getTypedIfComplexType<ComplexType>())
		{
			if (t->hasConstructor())
				return true;
		}
	}

	return false;
}

bool StructType::hasDefaultConstructor()
{
	if (!ComplexType::hasDefaultConstructor())
		return false;

	for (auto& m : memberData)
	{
		if (auto c = m->typeInfo.getTypedIfComplexType<ComplexType>())
		{
			if (!c->hasDefaultConstructor())
				return false;
		}
	}

	return true;
}

juce::Identifier StructType::getConstructorId()
{
	if (hasConstructor())
		return id.getIdentifier();
	else
		return {};
}

void StructType::finaliseAlignment()
{
	if (externalyDefinedSize != 0)
	{
		ComplexType::finaliseAlignment();
		return;
	}
		
	if (isFinalised())
		return;


	int baseIndex = 0;

	for (auto b : baseClasses)
	{
		templateParameters.addArray(b->baseClass->templateParameters);

		jassert(b->baseClass->isFinalised());

		b->memberOffset = baseIndex;

		for (auto m : *b)
			memberData.insert(baseIndex++, new Member(*m));
	}

	size_t offset = 0;

	for (auto m : memberData)
	{
		if (auto tcd = m->typeInfo.getTypedIfComplexType<TemplatedComplexType>())
		{
			Operations::TemplateParameterResolver resolver(templateParameters);

			auto r = resolver.processType(m->typeInfo);
		}

		if (m->typeInfo.isComplexType())
			m->typeInfo.getComplexType()->finaliseAlignment();

		m->offset = offset;

		auto alignment = getRequiredAlignment(m);

		if (alignment != 0)
			m->padding = offset % alignment;

		offset += m->padding + m->typeInfo.getRequiredByteSize();
	}
	
	if (hasConstructor() && !ComplexType::hasConstructor())
	{
		// One of the members has a constructor, so we need to create an 
		// artificial one for this struct.

		FunctionClass::Ptr fc = getFunctionClass();

		FunctionData f;
		f.id = fc->getClassName().getChildId(FunctionClass::getSpecialSymbol(fc->getClassName(), FunctionClass::Constructor));
		f.returnType = TypeInfo(Types::ID::Void);
		
		struct SubConstructorData
		{
			WeakReference<ComplexType> childType;
			size_t offset;
		};

		Array<SubConstructorData> subConstructors;

		for (auto m : memberData)
		{
			if (auto ct = m->typeInfo.getTypedIfComplexType<ComplexType>())
			{
				if (ct->hasConstructor())
				{
					FunctionClass::Ptr sfc = ct->getFunctionClass();

					SubConstructorData sd;

					sd.childType = m->typeInfo.getComplexType().get();
					sd.offset = m->offset;

					subConstructors.add(std::move(sd));
				}
			}
		}
		
		f.inliner = Inliner::createHighLevelInliner(f.id, [subConstructors](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			using namespace Operations;

			auto ab = new AnonymousBlock(d->location);

			for (auto sc : subConstructors)
			{
				auto parent = d->object->clone(d->location);

				FunctionClass::Ptr sfc = sc.childType->getFunctionClass();

				auto f = sfc->getSpecialFunction(FunctionClass::Constructor);

				auto call = new FunctionCall(d->location, nullptr, Symbol(f.id, TypeInfo(Types::ID::Void)), f.templateParameters);
				call->setObjectExpression(new MemoryReference(d->location, parent, TypeInfo(sc.childType.get()), (int)sc.offset));

				

				if (f.canBeInlined(true))
				{
					SyntaxTreeInlineData sd(call, {}, f);
					sd.object = call->getObjectExpression();

					call->currentCompiler = d->object->currentCompiler;
					call->currentScope = d->object->currentScope;

					sd.templateParameters = f.templateParameters;
					
					auto r = f.inlineFunction(&sd);
					d->location.test(r);
					ab->addStatement(sd.target);
				}
				else
				{
					ab->addStatement(call);
				}
			}

			d->target = ab;

			return Result::ok();
		});

		addJitCompiledMemberFunction(f);
	}

	auto alignment = getRequiredAlignment();

	if (alignment != 0)
	{
		auto missingBytesForAlignment = offset % getRequiredAlignment();

		if (missingBytesForAlignment != 0)
		{
			offset += (getRequiredAlignment() - missingBytesForAlignment);
		}
	}

	jassert(offset == getRequiredByteSize());

	ComplexType::finaliseAlignment();
}

snex::Types::ID StructType::getMemberDataType(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return m->typeInfo.getType();
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
	// Save ya some trouble...
	jassert(isFinalised());

	for (auto m : memberData)
	{
		if (m->id == id)
			return m->padding + m->offset;
	}

	jassertfalse;
	return 0;
}

size_t StructType::getMemberOffset(int index) const
{
	if (isPositiveAndBelow(index, memberData.size()))
	{
		auto m = memberData[index];
		return m->offset + m->padding;
	}

	jassertfalse;
	return 0;
}

snex::jit::NamespaceHandler::Visibility StructType::getMemberVisibility(const Identifier& id) const
{
	for (auto m : memberData)
	{
		if (m->id == id)
			return m->visibility;
	}

	return NamespaceHandler::Visibility::Private;
}

juce::Identifier StructType::getMemberName(int index) const
{
	if (isPositiveAndBelow(index, memberData.size()))
	{
		return memberData[index]->id;
	}

	return {};
}

void StructType::addJitCompiledMemberFunction(const FunctionData& f)
{
	if (id != f.id.getParent())
	{
		jassertfalse;
	}

	for (auto& existing : memberFunctions)
	{
		if (existing.matchIdArgsAndTemplate(f))
		{
			existing = f;
			return;
		}
			
	}

	jassert(f.function == nullptr);

	if (TemplateParameter::ListOps::isParameter(f.templateParameters))
	{
		for (auto& existing : memberFunctions)
		{
			if (existing.matchIdArgs(f))
			{
				if (TemplateParameter::ListOps::isArgument(existing.templateParameters))
				{
					existing = f;
					return;
				}
			}
		}
	}

	memberFunctions.add(f);
}

bool StructType::injectInliner(const FunctionData& f)
{
	// Yeah, right...
	jassert(f.inliner != nullptr);
	
	for (auto& existing : memberFunctions)
	{
		if (existing.matchIdArgsAndTemplate(f))
		{
			existing.function = f.function;
			existing.inliner = f.inliner;
			return true;
		}
	}

	return false;
}

int StructType::injectInliner(const Identifier& functionId, Inliner::InlineType type, const Inliner::Func& func, const TemplateParameter::List& functionParameters)
{
	int numReplaced = 0;
	Inliner::Ptr i = Inliner::createFromType(id.getChildId(functionId), type, func);

	for (auto& f : memberFunctions)
	{
		if (f.id.getIdentifier() == functionId)
		{
			auto overrideInliner = functionParameters.isEmpty() || 
				TemplateParameter::ListOps::match(f.templateParameters, functionParameters);

			if (overrideInliner)
			{
				f.inliner = i;
				numReplaced++;
			}
		}
	}
		
	return numReplaced;
}

juce::MemoryBlock StructType::createByteBlock() const
{
	auto list = makeDefaultInitialiserList();

	Array<short> bytes;

	list->forEach([&bytes](InitialiserList::ChildBase* p)
		{
			VariableStorage v;

			if (p->getValue(v))
			{
				auto iData = static_cast<short>(v.toInt());
				bytes.add(iData);
			}

			return false;
		});

	MemoryOutputStream mos;

	for (auto& b : bytes)
		mos.writeShort(b);

	return mos.getMemoryBlock();
}

snex::jit::Symbol StructType::getMemberSymbol(const Identifier& mId) const
{
	if (hasMember(mId))
	{
		return Symbol(id.getChildId(mId), getMemberTypeInfo(mId));
	}

	return {};
}

TemplateInstance StructType::getTemplateInstanceId() const
{
	return TemplateInstance(id, templateParameters);
}

bool StructType::injectMemberFunctionPointer(const FunctionData& f, void* fPointer)
{
	for (auto& m : memberFunctions)
	{
		if (m.matchIdArgsAndTemplate(f))
		{
			m.function = fPointer;
			return true;
		}
	}

	return false;
}

void StructType::addWrappedMemberMethod(const Identifier& memberId, FunctionData wrapperFunction)
{
	auto functionId = wrapperFunction.id.getIdentifier();
	wrapperFunction.id = id.getChildId(functionId);

	for (auto& m : memberFunctions)
	{

		if (m.matchIdArgsAndTemplate(wrapperFunction))
		{
			// Already in there...
			return;
		}
	}

	if (wrapperFunction.inliner == nullptr)
	{
		wrapperFunction.inliner = Inliner::createHighLevelInliner(wrapperFunction.id, [this, memberId, functionId](InlineData* b)
		{
			using namespace Operations;

			auto d = b->toSyntaxTreeData();

			auto newCall = d->expression->clone(d->location);

			auto p = d->expression->getSubExpr(0)->clone(d->location);
			auto c = new VariableReference(d->location, getMemberSymbol(memberId));
			auto dot = new DotOperator(d->location, p, c);
			auto nf = dynamic_cast<FunctionCall*>(newCall.get());

			nf->setObjectExpression(dot);

			FunctionClass::Ptr fc = getMemberComplexType(memberId)->getFunctionClass();

			auto fId = fc->getClassName().getChildId(functionId);
			
			if (!fc->hasFunction(fId))
			{
				juce::String s;
				s << getMemberComplexType(memberId)->toString() << " does not have function " << functionId;
				return Result::fail(s);
			}
			

			Array<FunctionData> matches;

			fc->addMatchingFunctions(matches, fId);

			if (matches.size() == 1)
				nf->function = matches[0];
			else
			{
				Array<TypeInfo> argTypes;

				for (auto a : d->args)
					argTypes.add(a->getTypeInfo());

				for (auto& m : matches)
				{
					if (m.matchesArgumentTypes(argTypes))
					{
						nf->function = m;
						break;
					}
				}
			}

#if 0
			if (!nf->function.isResolved())
			{
				return Result::fail("Non overloaded function detected");
			}
#endif
				

			d->target = newCall;

			return Result::ok();
		});
	}

	memberFunctions.add(wrapperFunction);
}

TemplateParameter::List StructType::getTemplateInstanceParametersForFunction(NamespacedIdentifier& functionId)
{
	int unused;
	ComplexType::Ptr c;
	Array<FunctionData> u;

	findMatchesFromBaseClasses(u, functionId, unused, c);

	if(auto st = dynamic_cast<StructType*>(c.get()))
	{
		functionId.relocateSelf(id, st->id);

		return st->getTemplateInstanceParameters();
	}

	return getTemplateInstanceParameters();
}

int StructType::getBaseClassIndexForMethod(const FunctionData& f) const
{
	for (auto b : baseClasses)
	{
		auto idToUse = b->baseClass->id.getChildId(f.id.getIdentifier());
		FunctionClass::Ptr fc = b->baseClass->getFunctionClass();

		if (fc->hasFunction(idToUse))
			return b->memberOffset;
	}

	return -1;
}

juce::Array<snex::jit::FunctionData> StructType::getBaseSpecialFunctions(FunctionClass::SpecialSymbols s, TypeInfo returnType, const Array<TypeInfo>& args)
{
	Array<FunctionData> matches;

	for (auto b : baseClasses)
	{
		FunctionClass::Ptr fc = b->baseClass->getFunctionClass();

		matches.add(fc->getSpecialFunction(s, returnType, args));
	}

	return matches;
}

void StructType::findMatchesFromBaseClasses(Array<FunctionData>& possibleMatches,
	const NamespacedIdentifier& idWithDerivedParent, int& baseOffset, ComplexType::Ptr& baseClass)
{
	for(auto b: baseClasses)
	{
		FunctionClass::Ptr fc = b->baseClass->getFunctionClass();

		NamespacedIdentifier s;
		s = idWithDerivedParent.relocate(idWithDerivedParent.getParent(), b->baseClass->id);

		fc->addMatchingFunctions(possibleMatches, s);

		if(!possibleMatches.isEmpty())
		{
			baseClass = b->baseClass.get();

			if(!memberData.isEmpty())
				baseOffset = (int)getMemberOffset(b->memberOffset);

			return;
		}
	}
}

Result StructType::redirectAllOverloadedMembers(const Identifier& id, TypeInfo::List mainArgs)
{
	Inliner::Ptr iToUse = nullptr;
	void* fToUse = nullptr;
	bool found = false;

	for (auto& m : memberFunctions)
	{
		if(m.id.id == id && m.matchesArgumentTypes(mainArgs))
		{
			fToUse = m.function;
			iToUse = m.inliner;
			found = true;
			break;
		}
	}

	if (found)
	{
		for (auto& m : memberFunctions)
		{
			if (m.id.id == id)
			{
				m.function = fToUse;
				m.inliner = iToUse;
			}
		}

		return Result::ok();
	}
	else
	{
		String s;

		s << toString() << "::" << id << "(";

		for (auto& a : mainArgs)
		{
			s << a.toString() << ", ";
		}

		s = s.upToLastOccurrenceOf(", ", false, false);
		s << ") not found";

		return Result::fail(s);
	}
		
	
}

void StructType::addBaseClass(StructType* b)
{
	jassert(!isFinalised());

	baseClasses.add(new BaseClass(b));

	for (auto& p : b->internalProperties)
	{
		if (!internalProperties.contains(p.name))
			internalProperties.set(p.name, p.value);
	}
}

bool StructType::canBeMember(const NamespacedIdentifier& possibleMemberId) const
{
	auto parent = possibleMemberId.getParent();

	if (id == parent)
		return true;

	for (auto b : baseClasses)
	{
		if (b->baseClass->id == parent)
			return true;
	}

	return false;
}



}
}
