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
	if (auto st = t.getTypedIfComplexType<SpanType>())
	{
		return st->isSimd();
	}

	return false;
}

bool SpanType::isSimd() const
{
	if (getElementType() == Types::ID::Float && getNumElements() == 4)
	{
		jassert(hasAlias());
		jassert(toString() == "float4");
		return true;
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
		auto indexFunction = new FunctionData();

		auto iid = st->getClassName().getChildId("index");

		indexFunction->id = iid;
		indexFunction->returnType = TypeInfo(Types::ID::Dynamic);
		indexFunction->addArgs("value", TypeInfo(Types::ID::Integer));
		indexFunction->templateParameters.add(TemplateParameter(iid.getChildId("IndexType")));
		indexFunction->inliner = new Inliner(indexFunction->id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto& cc = d->gen.cc;
			auto indexType = d->target->getTypeInfo().getTypedIfComplexType<IndexBase>();

			jassert(d->target->getType() == Types::ID::Integer);
			jassert(indexType != nullptr);
			jassert(d->args[0] != nullptr);

			auto assignOp = indexType->getAsmFunction(FunctionClass::AssignOverload);

			AsmInlineData assignData(d->gen);
			assignData.target = d->target;
			assignData.object = d->target;
			assignData.args.add(d->target);
			assignData.args.add(d->args[0]);

			auto r = assignOp(&assignData);

			if (r.failed())
				return r;


			return Result::ok();
		}, {});

		indexFunction->inliner->returnTypeFunction = [](InlineData* d)
		{
			auto r = dynamic_cast<ReturnTypeInlineData*>(d);
			
			if (r->templateParameters.size() != 1)
				return Result::fail("template argument mismatch");

			r->f.returnType = r->templateParameters.getFirst().type;

			return Result::ok();
		};

		st->addFunction(indexFunction);
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
		toSimdFunction.inliner = Inliner::createAsmInliner(toSimdFunction.id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			if (!d->gen.canVectorize())
				return Result::fail("Vectorization is deactivated");

			auto& cc = d->gen.cc;

			if (d->object->isMemoryLocation())
				d->target->setCustomMemoryLocation(d->object->getAsMemoryLocation(), d->object->isGlobalMemory());
			else
			{
				d->target->setCustomMemoryLocation(x86::qword_ptr(PTR_REG_R(d->object)), d->object->isGlobalMemory());
			}
				

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

juce::Result SpanType::initialise(InitData d)
{
	juce::String e;

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
				cm = d.asmPtr->cloneWithOffset(offset);
				c.asmPtr = &cm;
			}
				
			c.initValues = d.initValues->createChildList(indexToUse);
			c.callConstructor = d.callConstructor;

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

				ComplexType::writeNativeMemberType(d.dataPointer, getElementSize() * i, valueToUse);
			}
			else
			{
				ComplexType::writeNativeMemberTypeToAsmStack(d, indexToUse, getElementSize() * i, getElementSize());
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

snex::jit::ComplexType::Ptr SpanType::createSubType(SubTypeConstructData* sd)
{
	auto id = sd->id;

	if (id.getIdentifier() == Identifier("wrapped"))
	{
		return new Wrapped(TypeInfo(this));
	}
	if (id.getIdentifier() == Identifier("unsafe"))
	{
		return new Unsafe(TypeInfo(this));
	}

	return nullptr;
}

DynType::DynType(const TypeInfo& elementType_) :
	elementType(elementType_)
{
	ComplexType::finaliseAlignment();
}

snex::jit::DynType::IndexType DynType::getIndexType(const TypeInfo& t)
{
	if (t.getTypedIfComplexType<Wrapped>()) return IndexType::W;
	if (t.getTypedIfComplexType<Unsafe>()) return IndexType::U;
	
	jassertfalse;
	return IndexType::numIndexTypes;
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
			value->loadMemoryIntoRegister(cc);
            
            if (thisObj->isGlobalVariableRegister())
                thisObj->loadMemoryIntoRegister(cc);
            
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
		else if (auto dt = valueType.getTypedIfComplexType<DynType>())
		{
			if (thisObj->isGlobalVariableRegister())
				thisObj->createMemoryLocation(cc);

			if (value->isMemoryLocation())
				thisObj->setCustomMemoryLocation(value->getAsMemoryLocation(), value->isGlobalMemory());
			else
				thisObj->setCustomMemoryLocation(x86::ptr(PTR_REG_R(value)).cloneResized(8), value->isGlobalMemory());

			return Result::ok();
		}
		else
		{
			juce::String s;
			s << "Can't assign" << valueType.toString() << " to " << thisObj->getTypeInfo().toString();
			return Result::fail(s);
		}
	}, {});

	{
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
	}

	{
		auto indexFunction = new FunctionData();

		auto iid = dynOperators->getClassName().getChildId("index");

		indexFunction->id = iid;
		indexFunction->returnType = TypeInfo(Types::ID::Dynamic);
		indexFunction->addArgs("value", TypeInfo(Types::ID::Integer));
		indexFunction->templateParameters.add(TemplateParameter(iid.getChildId("IndexType")));

		indexFunction->inliner = new Inliner(iid, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto& cc = d->gen.cc;

			auto indexType = d->target->getTypeInfo().getTypedIfComplexType<IndexBase>();

			jassert(d->target->getType() == Types::ID::Integer);
			jassert(indexType != nullptr);
			jassert(d->args[0] != nullptr);

			if (d->args[0]->isMemoryLocation())
			{
				auto immValue = d->args[0]->getImmediateIntValue();
				d->target->setImmediateValue(immValue);
				d->target->createMemoryLocation(cc);
			}
			else
			{
				d->target->createRegister(cc);
				INT_OP(cc.mov, d->target, d->args[0]);
			}

			return Result::ok();
		}, {});

		indexFunction->inliner->returnTypeFunction = [](InlineData* d)
		{
			auto r = dynamic_cast<ReturnTypeInlineData*>(d);

			if (r->templateParameters.size() != 1)
				return Result::fail("template argument mismatch");

			r->f.returnType = r->templateParameters.getFirst().type;

			return Result::ok();
		};

		dynOperators->addFunction(indexFunction);
	}

	{
		auto& toSimdFunction = dynOperators->createSpecialFunction(FunctionClass::ToSimdOp);
		
		toSimdFunction.inliner = Inliner::createAsmInliner(toSimdFunction.id, [this](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			if (!d->gen.canVectorize())
				return Result::fail("Vectorization is deactivated");

			auto& cc = d->gen.cc;

			auto mem = cc.newStack(getRequiredByteSize(), getRequiredAlignment());

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
	}

	{
		

		auto isSimdableFunc = new FunctionData();
		isSimdableFunc->id = dynOperators->getClassName().getChildId("isSimdable");
		isSimdableFunc->returnType = TypeInfo(Types::ID::Integer);
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

		dynOperators->addFunction(isSimdableFunc);
	}

	{
		auto& subscriptFunction = dynOperators->createSpecialFunction(FunctionClass::Subscript);
		subscriptFunction.returnType = elementType;
		subscriptFunction.addArgs("this", TypeInfo(this));
		subscriptFunction.addArgs("index", TypeInfo(Types::ID::Dynamic));

		auto t = elementType;

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

#if 0
			switch (getIndexType(index->getTypeInfo()))
			{
			case IndexType::W:
			{
				index->loadMemoryIntoRegister(cc);

				cc.mov(limit, p);
				auto tempReg = cc.newGpd();
				auto i = INT_REG_R(index);
				cc.cdq(tempReg, i);
				cc.idiv(tempReg, i, limit);
				cc.mov(i, tempReg);
				break;
			}
			case IndexType::U:
			{
				break;
			}
			default:
				jassertfalse;
			}
#endif

			auto gs = d->target->getScope()->getGlobalScope();
			auto safeCheck = gs->isRuntimeErrorCheckEnabled();

			auto okBranch = cc.newLabel();
			auto errorBranch = cc.newLabel();

			safeCheck &= (index->getTypeInfo().getTypedIfComplexType<IndexBase>() == nullptr);

			if (safeCheck)
			{
				X86Mem p;

				if (dynReg->isMemoryLocation())
					p = dynReg->getAsMemoryLocation().cloneAdjustedAndResized(4, 4);
				else
					p = x86::ptr(PTR_REG_R(dynReg)).cloneAdjustedAndResized(4, 4);

				cc.mov(limit, p);
				
				
				if (index->isMemoryLocation())
					cc.cmp(limit, INT_IMM(index));
				else
					cc.cmp(limit, INT_REG_R(index));
				cc.jg(okBranch);


				auto flagReg = cc.newGpd();

				auto errorFlag = x86::ptr(gs->getRuntimeErrorFlag()).cloneResized(4);


				cc.mov(flagReg, (int)RuntimeError::ErrorType::DynAccessOutOfBounds);

				cc.mov(errorFlag, flagReg);

				auto& l = d->gen.location;

				cc.mov(flagReg, (int)l.getLine());
				cc.mov(errorFlag.cloneAdjustedAndResized(4, 4), flagReg);
				cc.mov(flagReg, (int)l.getColNumber(l.program, l.location));
				cc.mov(errorFlag.cloneAdjustedAndResized(8, 4), flagReg);
				
				if (index->isMemoryLocation())
					cc.mov(flagReg, INT_IMM(index));
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
				target->loadMemoryIntoRegister(cc, true);

				cc.jmp(endLabel);
				cc.bind(errorBranch);
				auto nirvana = cc.newStack(target->getTypeInfo().getRequiredByteSize(), 0);

				auto type = d->target->getType();

				
				IF_(int) cc.mov(INT_REG_W(target), nirvana);
				IF_(double) cc.movsd(FP_REG_W(target), nirvana);
				IF_(float) cc.movss(FP_REG_W(target), nirvana);
				IF_(void*) cc.mov(PTR_REG_W(target), nirvana);

				
				cc.bind(endLabel);
				cc.nop();
			}
				




			return Result::ok();
		});
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
		auto data = ptr.getDataPointer();

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

snex::jit::ComplexType::Ptr DynType::createSubType(SubTypeConstructData* sd)
{
	auto id = sd->id;

	if (id.getIdentifier() == Identifier("wrapped"))
	{
		return new Wrapped(TypeInfo(this));
	}
	if (id.getIdentifier() == Identifier("unsafe"))
	{
		return new Unsafe(TypeInfo(this));
	}

	return nullptr;
}

StructType::StructType(const NamespacedIdentifier& s, const Array<TemplateParameter>& tp) :
	id(s),
	templateParameters(tp)
{
	for (auto& p : templateParameters)
	{
		jassert(!p.isTemplateArgument());
		jassert(p.argumentId.isValid());
	}
}

StructType::~StructType()
{
}

size_t StructType::getRequiredByteSize() const
{
	size_t s = 0;

	for (auto m : memberData)
		s += (m->typeInfo.getRequiredByteSize() + m->padding);

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

	for (const auto& f : memberFunctions)
	{
		mf->addFunction(new FunctionData(f));
	}

	return mf;
}



juce::Result StructType::initialise(InitData d)
{
	int index = 0;

	for (auto m : memberData)
	{
		if (isPositiveAndBelow(index, d.initValues->size()))
		{
			if (m->typeInfo.isComplexType())
			{
				InitData c;
				c.initValues = d.initValues->createChildList(index);
				c.callConstructor = d.callConstructor;

				AssemblyMemory cm;

				if (d.asmPtr == nullptr)
					c.dataPointer = getMemberPointer(m, d.dataPointer);
				else
				{
					cm = d.asmPtr->cloneWithOffset(m->offset + m->padding);
					c.asmPtr = &cm;
				}

				auto ok = m->typeInfo.getComplexType()->initialise(c);

				if (!ok.wasOk())
					return ok;
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

					ComplexType::writeNativeMemberTypeToAsmStack(d, index, offset, size);
				}
			}
		}

		index++;
	}

	if (d.callConstructor)
	{
		return callConstructor(d.dataPointer, d.initValues);
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

	if (isExternalDefinition && templateParameters.isEmpty())
	{
		NamespaceHandler::ScopedNamespaceSetter sns(*handler, id.getParent());

		handler->addSymbol(id, TypeInfo(this), NamespaceHandler::Struct);
		NamespaceHandler::ScopedNamespaceSetter sns2(*handler, id);

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

bool StructType::hasConstructor()
{
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
				call->setObjectExpression(new MemoryReference(d->location, parent, TypeInfo(sc.childType.get()), sc.offset));

				if (f.canBeInlined(true))
				{
					SyntaxTreeInlineData sd(call, {});
					sd.object = call->getObjectExpression();
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

void StructType::addJitCompiledMemberFunction(const FunctionData& f)
{
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

			if (!fc->hasFunction(fc->getClassName().getChildId(functionId)))
			{
				juce::String s;
				s << getMemberComplexType(memberId)->toString() << " does not have function " << functionId;
				return Result::fail(s);
			}
				

			nf->function = fc->getNonOverloadedFunction(NamespacedIdentifier(functionId));

			d->target = newCall;

			return Result::ok();
		});
	}

	memberFunctions.add(wrapperFunction);
}

IndexBase::IndexBase(const TypeInfo& parentType_) :
	ComplexType(),
	parentType(parentType_.getComplexType())
{
}

IndexBase::~IndexBase()
{
}

snex::jit::Inliner::Func IndexBase::getAsmFunction(FunctionClass::SpecialSymbols s)
{
	if (s == FunctionClass::SpecialSymbols::AssignOverload)
	{
		return [](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto& cc = d->gen.cc;
			auto thisObj = d->target;
			auto value = d->args[1];

			jassert(thisObj->getTypeInfo().getTypedComplexType<IndexBase>() != nullptr);
			jassert(value->getType() == Types::ID::Integer);

			thisObj->loadMemoryIntoRegister(cc);
			INT_OP(cc.mov, thisObj, value);

			return Result::ok();
		};
	}

	return {};
}

snex::jit::Inliner::Func IndexBase::getAsmFunctionWithFixedSize(FunctionClass::SpecialSymbols s, int wrapSize)
{
	if (s == FunctionClass::AssignOverload)
	{
		return [wrapSize](InlineData* d_)
		{
			auto d = dynamic_cast<AsmInlineData*>(d_);
			auto& cc = d->gen.cc;
			auto thisObj = d->target;
			auto value = d->args[1];

			jassert(thisObj->getType() == Types::ID::Integer || thisObj->getTypeInfo().getTypedComplexType<SpanType::Wrapped>() != nullptr);
			jassert(value->getType() == Types::ID::Integer);

			thisObj->loadMemoryIntoRegister(cc);
			INT_OP(cc.mov, thisObj, value);

			auto t = INT_REG_W(thisObj);

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

			//cc.mov(ptr, t);

			return Result::ok();
		};
	}
	if (s == FunctionClass::IncOverload)
	{
		return [wrapSize](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			jassert(d->target->getType() == Types::ID::Integer);

			auto valueReg = d->args[0];
			bool isDec, isPre;
			auto& cc = d->gen.cc;
			d->target->loadMemoryIntoRegister(cc);
			auto r = INT_REG_W(d->target);
			auto tmp = cc.newGpq();

			Operations::Increment::getOrSetIncProperties(d->templateParameters, isPre, isDec);
			jassert((valueReg == nullptr) == isPre);

			if (!isPre)
			{
				valueReg->createRegister(cc);
				cc.mov(INT_REG_W(valueReg), r);
			}

			if (isDec)
			{
				cc.mov(tmp, r);
				cc.dec(tmp);
				cc.mov(r, wrapSize - 1);
				cc.cmovge(r, tmp);
			}
			else
			{
				cc.mov(tmp, r);
				cc.inc(tmp);
				cc.xor_(r, r);
				cc.cmp(tmp, wrapSize);
				cc.cmovne(r, tmp);
			}

			return Result::ok();
		};
	}
}

bool IndexBase::forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer)
{
	if (typePtr.get() == this)
		return t(this, dataPointer);

	return false;
}

bool IndexBase::isValidCastSource(Types::ID nativeSourceType, ComplexType::Ptr complexSourceType) const
{
	if (complexSourceType != nullptr && complexSourceType->getRegisterType(false) == Types::ID::Integer)
		return true;

	if (nativeSourceType == Types::ID::Integer)
		return true;

	return false;
}

bool IndexBase::isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const
{
	if (complexTargetType != nullptr && complexTargetType->getRegisterType(false) == Types::ID::Integer)
		return true;

	if (nativeTargetType == Types::ID::Integer)
		return true;

	return false;
}

snex::InitialiserList::Ptr IndexBase::makeDefaultInitialiserList() const
{
	InitialiserList::Ptr n = new InitialiserList();
	n->addImmediateValue(0);
	return n;
}


juce::Result IndexBase::initialise(InitData data)
{
	VariableStorage v;

	auto r = data.initValues->getValue(0, v);

	if (!r.wasOk())
		return r;

	auto initValue = getInitValue(v.toInt());

	if (data.asmPtr == nullptr)
	{
		ComplexType::writeNativeMemberType(data.dataPointer, 0, initValue);
	}
	else
	{
		auto& cc = GET_COMPILER_FROM_INIT_DATA(data);
		cc.mov(data.asmPtr->memory, initValue);
	}

	return Result::ok();
}

void IndexBase::dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const
{
	Types::Helpers::dumpNativeData(s, intendLevel, "value", dataStart, complexTypeStartPointer, 4, Types::ID::Integer);
}

juce::String IndexBase::toStringInternal() const
{
	juce::String s;
	s << parentType->toString() << "::" << getIndexName().toString();
	return s;
}



snex::jit::FunctionClass* IndexBase::getFunctionClass()
{
	auto wId = NamespacedIdentifier("index_operators");
	auto indexOperators = new FunctionClass(wId);

	if (auto asOp = createOperator(indexOperators, FunctionClass::AssignOverload))
	{
		asOp->addArgs("this", TypeInfo(this));
		asOp->addArgs("newValue", TypeInfo(Types::ID::Integer));
	}

	if (auto incOp = createOperator(indexOperators, FunctionClass::IncOverload))
	{
		incOp->addArgs("value", TypeInfo(Types::ID::Integer, false, true));
	}

	{
		auto assignOp = getAsmFunction(FunctionClass::SpecialSymbols::AssignOverload);

		auto movedFunction = new FunctionData();
		movedFunction->id = wId.getChildId("moved");
		movedFunction->returnType = TypeInfo(this);
		movedFunction->addArgs("delta", TypeInfo(Types::ID::Integer));
		movedFunction->inliner = Inliner::createAsmInliner(movedFunction->id, [assignOp](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto type = d->target->getTypeInfo();

			auto isDynTarget = dynamic_cast<DynType*>(type.getTypedComplexType<IndexBase>()->parentType.get()) != nullptr;;

			jassert(type.getTypedComplexType<IndexBase>() != nullptr);
			jassert(d->args[0] != nullptr);
			jassert(d->args[0]->getType() == Types::ID::Integer);
			jassert(d->object->getTypeInfo() == type);
			auto& cc = d->gen.cc;

			

			jassert(assignOp);
			
			auto tempReg = d->gen.registerPool->getNextFreeRegister(d->target->getScope(), TypeInfo(Types::ID::Integer));

			tempReg->createRegister(cc);

			INT_OP(cc.mov, tempReg, d->object);
			INT_OP(cc.add, tempReg, d->args[0]);

			cc.setInlineComment("assign-op");

			AsmInlineData assignData(d->gen);

			d->target->createRegister(cc);
			assignData.object = d->target;
			assignData.target = d->target;
			assignData.args.add(d->target);
			assignData.args.add(tempReg);
			
			auto r = assignOp(&assignData);

			if (r.failed())
				return r;

			return Result::ok();
		});

		indexOperators->addFunction(movedFunction);
	}

	return indexOperators;
}

snex::jit::Inliner::Func SpanType::Wrapped::getAsmFunction(FunctionClass::SpecialSymbols s) 
{
	auto wrapSize = dynamic_cast<SpanType*>(parentType.get())->getNumElements();

	return getAsmFunctionWithFixedSize(s, wrapSize);
}

}
}
