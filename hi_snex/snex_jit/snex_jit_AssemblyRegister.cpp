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

#if SNEX_ASMJIT_BACKEND

// Just for debugging purposes...
static int reg_counter = 0;

AssemblyRegister::AssemblyRegister(BaseCompiler* compiler_, TypeInfo type_) :
	type(type_),
	compiler(compiler_)
{
	debugId = reg_counter++;
}

bool AssemblyRegister::matchesMemoryLocation(Ptr other) const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->matchesMemoryLocation(other);

	auto bothAreMemory = hasCustomMemoryLocation() &&
		other->hasCustomMemoryLocation();

	auto typeMatch = other->getTypeInfo() == getTypeInfo();

	if (typeMatch && bothAreMemory)
	{
        auto m = other->getMemoryLocationForReference();
		return m == memory;
	}

	return false;
}

bool AssemblyRegister::isGlobalMemory() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isGlobalMemory();

    return (hasCustomMem && globalMemory) || isGlobalVariableRegister();// || id.isReference();
}

bool AssemblyRegister::shouldLoadMemoryIntoRegister() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->shouldLoadMemoryIntoRegister();

	return numMemoryReferences > 0;
}

void AssemblyRegister::setReference(BaseScope* s, const Symbol& ref)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setReference(s, ref);
		return;
	}

	auto refScope = s->getScopeForSymbol(ref.id);

	if (refScope != nullptr)
		scope = refScope;
	else
		scope = s;

	id = ref;
	jassert(compiler->getRegisterType(id.typeInfo) == getType());
}


const Symbol& AssemblyRegister::getVariableId() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getVariableId();

	return id;
}


bool AssemblyRegister::isDirtyGlobalMemory() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isDirtyGlobalMemory();

	return dirty && isGlobalMemory();
}



void AssemblyRegister::reinterpretCast(const TypeInfo& newType)
{
	type = newType;
}

snex::Types::ID AssemblyRegister::getType() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getType();

	return compiler->getRegisterType(type);
}

void* AssemblyRegister::getGlobalDataPointer()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getGlobalDataPointer();

	if (getType() == Types::ID::Pointer)
	{
		jassert(memoryLocation != nullptr);
		return memoryLocation;
	}

	jassert(scope != nullptr);

	if (isGlobalVariableRegister())
		return scope->getRootClassScope()->rootData->getDataPointer(id.id);

	// No need to fetch / write the data for non-globals
	jassertfalse;
	return nullptr;
}

asmjit::X86Reg AssemblyRegister::getRegisterForReadOp()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getRegisterForReadOp();

	jassert(state == ActiveRegister ||
		state == DirtyGlobalRegister);

	jassert(reg.isValid());
	return reg;
}


asmjit::X86Reg AssemblyRegister::getRegisterForWriteOp()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getRegisterForWriteOp();

	jassert(state == ActiveRegister ||
		state == DirtyGlobalRegister);

	jassert(scope != nullptr);

	if (isGlobalMemory())
	{
		dirty = true;
		state = DirtyGlobalRegister;
	}

	if (id)
	{
		if (isIter)
			dirty = true;

		auto sToUse = scope->getScopeForSymbol(id.id);

		//jassert(sToUse != nullptr);

		if (sToUse == nullptr)
			sToUse = scope;

		auto scopeType = sToUse->getScopeType();

		if (!isIter && (sToUse->getRootClassScope() == sToUse || id.isReference()))
		{
            if(memoryLocation != nullptr)
            {
                dirty = true;
                state = DirtyGlobalRegister;
            }
		}
		else if (scopeType == BaseScope::Global)
			throw juce::String("can't write to global variables");
	}

	jassert(reg.isValid());
	return reg;
}


asmjit::X86Mem AssemblyRegister::getAsMemoryLocation()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getAsMemoryLocation();

	jassert(memory.isMem());

	jassert(state == LoadedMemoryLocation);
	//jassert(type != Types::ID::Integer);

	return memory;
}


asmjit::X86Mem AssemblyRegister::getMemoryLocationForReference()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getMemoryLocationForReference();

	jassert(memory.isMem());

	return memory;
}

juce::int64 AssemblyRegister::getImmediateIntValue()
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->getImmediateIntValue();

	jassert(state == LoadedMemoryLocation || state == UnloadedMemoryLocation);
	jassert(getType() == Types::ID::Integer);
 	jassert(!hasCustomMem);
	jassert(hasImmediateValue);

	return static_cast<int64>(immediateIntValue);
}

bool AssemblyRegister::operator==(const Symbol& s) const
{
	if (isReferencingOtherRegister())
		return *getReferenceTargetRegister().get() == s;

	return id == s;
}

void AssemblyRegister::loadMemoryIntoRegister(asmjit::X86Compiler& cc, bool forceLoad)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->loadMemoryIntoRegister(cc, forceLoad);
		return;
	}

	if (!forceLoad && reg.isValid())
		return;

	if (state == UnloadedMemoryLocation)
		createMemoryLocation(cc);

	// Global variables will be loaded into a register already.
	if (!forceLoad && state == ActiveRegister)
	{
		jassert(reg.isValid());
		return;
	}

	createRegister(cc);

	asmjit::Error e = asmjit::kErrorOk;

	jassert(!memory.isNone());

	switch (getType())
	{
	case Types::ID::Float: e = cc.movss(reg.as<X86Xmm>(), memory); break;
	case Types::ID::Double: e = cc.movsd(reg.as<X86Xmm>(), memory); break;
	case Types::ID::Block:
	case Types::ID::Integer:
	{
		if (hasImmediateValue)
			e = cc.mov(reg.as<IntRegisterType>(), static_cast<int64_t>(immediateIntValue));
		else
			e = cc.mov(reg.as<IntRegisterType>(), memory);

		break;
	}
	case Types::ID::Pointer:
	{
		if (isSimd4Float())
		{
            auto p = AsmCodeGenerator::createValid64BitPointer(cc, memory, 0, 16);
            
            cc.movaps(reg.as<X86Xmm>(), p);
			jassert(reg.isXmm());
		}
		else
		{
			if (hasCustomMem)
				e = cc.lea(reg.as<X86Gpq>(), memory);
			else if (memory.hasOffset() && !memory.hasBaseOrIndex())
				e = cc.mov(reg.as<X86Gpq>(), memory.offset());
			else if (isGlobalMemory())
				e = cc.mov(reg.as<X86Gpq>(), (uint64_t)memoryLocation);
		}

		break;
	}
	default: jassertfalse;
	}
	
	state = ActiveRegister;
	jassert(e == 0);
}


bool AssemblyRegister::isValid() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isValid();

	return isActive() && reg.isValid();
}

bool AssemblyRegister::isGlobalVariableRegister() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isGlobalVariableRegister();

	return scope->getRootClassScope()->rootData->contains(id.id);
}

bool AssemblyRegister::isActive() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isActive();

	return state == ActiveRegister || state == DirtyGlobalRegister;
}

bool AssemblyRegister::isSameRegisterSource(Ptr other) const
{
	auto thisRef = getReferenceTargetRegister();
	auto otherRef = other->getReferenceTargetRegister();

	return thisRef == otherRef;
}

bool AssemblyRegister::matchesScopeAndSymbol(BaseScope* scopeToCheck, const Symbol& symbol) const
{

	// Here we do not look for a reference since we really want the actual register...
#if 0
	if (isReferencingOtherRegister())
		getReferenceTargetRegister()->matchesScopeAndSymbol(scopeToCheck, symbol);
#endif

	auto scopeMatches = scopeToCheck->getScopeForSymbol(symbol.id) == scope;
	auto symbolMatches = symbol == id;

	return scopeMatches && symbolMatches;
}

void AssemblyRegister::createMemoryLocation(asmjit::X86Compiler& cc)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->createMemoryLocation(cc);
		return;
	}

	jassert(memoryLocation != nullptr || getType() == Types::ID::Integer);

	if (getType() != Types::ID::Pointer && isGlobalVariableRegister() && !id.isConst())
	{
		auto t = getType();
		
		hasCustomMem = false;
		state = State::LoadedMemoryLocation;

		bool useQword = (t == Types::ID::Double ||
			t == Types::ID::Block ||
			t == Types::ID::Pointer);

		auto r = cc.newGpq();

		cc.mov(r, (uint64_t)memoryLocation);

		memory = useQword ? x86::qword_ptr(r) : x86::dword_ptr(r);
	}
	else
	{
		if (getType() == Types::ID::Float)
		{
			auto v = *reinterpret_cast<float*>(memoryLocation);
			isZeroValue = v == 0.0f;
			
			memory = cc.newFloatConst(ConstPoolScope::kLocal, v);
		}
		if (getType() == Types::ID::Double)
		{
			auto v = *reinterpret_cast<double*>(memoryLocation);
			isZeroValue = v == 0.0;

			memory = cc.newDoubleConst(ConstPoolScope::kLocal, v);
		}
		if (getType() == Types::ID::Integer)
		{
			if (memoryLocation != nullptr)
			{
				immediateIntValue = *reinterpret_cast<int*>(memoryLocation);
				hasImmediateValue = true;
			}

			isZeroValue = immediateIntValue == 0;
		}
		if (getType() == Types::ID::Pointer)
		{
            // If it's not aligned to 16 byte it's definitely not a VariableStorage
            
            if((uint64_t)memoryLocation % 16 == 0)
                memory = x86::qword_ptr((uint64_t)reinterpret_cast<VariableStorage*>(memoryLocation)->getDataPointer());
            else
                memory = x86::qword_ptr((uint64_t)memoryLocation);
		}
			
		state = State::LoadedMemoryLocation;
		jassert(memory.isMem());
	}
}


void AssemblyRegister::createRegister(asmjit::X86Compiler& cc)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->createRegister(cc);
		return;
	}

	jassert(getType() != Types::ID::Dynamic);

	if (reg.isValid())
	{
		jassert(state == ActiveRegister || 
			    state == DirtyGlobalRegister);
		return;
	}

	if (getType() == Types::ID::Float)
		reg = cc.newXmmSs();
	if (getType() == Types::ID::Double)
		reg = cc.newXmmSd();
	if (getType() == Types::ID::Integer)
		reg = cc.newGpd();
	if (type == Types::Block)
		reg = cc.newGpq();
	if (getType() == Types::Pointer)
	{
		if (isSimd4Float())
			reg = cc.newXmmPs();
		else
			reg = cc.newGpq();
	}
		
	
	state = ActiveRegister;
}


bool AssemblyRegister::isMemoryLocation() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isMemoryLocation();

	return state == LoadedMemoryLocation;
}


void AssemblyRegister::setCustomMemoryLocation(X86Mem newLocation, bool isGlobalMemory_)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setCustomMemoryLocation(newLocation, isGlobalMemory_);
		return;
	}

	memory = newLocation;
	dirty = false;
	globalMemory = isGlobalMemory_;
	reg = {};
	jassert(memory.isMem());
	state = LoadedMemoryLocation;
	hasCustomMem = true;
}

void AssemblyRegister::setDataPointer(void* memLoc, bool globalMemory_)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setDataPointer(memLoc, globalMemory_);
		return;
	}

	memoryLocation = memLoc;
	reg = {};
	globalMemory = globalMemory_;
	state = State::UnloadedMemoryLocation;
	hasCustomMem = false;
}


void AssemblyRegister::setImmediateValue(int64 value)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setImmediateValue(value);
		return;
	}

	jassert(getType() == Types::ID::Integer);

	hasImmediateValue = true;
	immediateIntValue = value;
	state = UnloadedMemoryLocation;
	memoryLocation = nullptr;
	reg = {};
	hasCustomMem = false;
}

bool AssemblyRegister::isImmediate() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isImmediate();

	return hasImmediateValue;
}

void AssemblyRegister::invalidateRegisterForCustomMemory()
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->invalidateRegisterForCustomMemory();
		return;
	}

	jassert(hasCustomMemoryLocation());
	dirty = false;
	reg = {};

	state = LoadedMemoryLocation;
}

void AssemblyRegister::clearAfterReturn()
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->clearAfterReturn();
		return;
	}

	reg = {};

	setMemoryState();
}

bool AssemblyRegister::isIteratorRegister() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isIteratorRegister();

	if (isIter)
	{
		jassert(state == ActiveRegister);
		return true;
	}

	return false;
}

bool AssemblyRegister::isSimd4Float() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isSimd4Float();

	if (!compiler->getOptimizations().contains(OptimizationIds::AutoVectorisation))
		return false;

	if (auto st = type.getTypedIfComplexType<SpanType>())
	{
		return (st->getElementType() == TypeInfo(Types::ID::Float)) && st->getNumElements() == 4;
	}

	return false;
}

void AssemblyRegister::setDirtyFloat4(Ptr source, int byteOffset)
{
	if (source->isGlobalMemory())
	{
		memory = source->getMemoryLocationForReference().cloneAdjusted(byteOffset);
		globalMemory = true;
		dirty = true;
		hasCustomMem = true;
		
		if (isActive())
			state = DirtyGlobalRegister;
	}
}

void AssemblyRegister::setUndirty()
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setUndirty();
		return;
	}

	if (dirty && isActive())
	{
		jassert(state == DirtyGlobalRegister || isIter || isGlobalMemory());

		dirty = false;
		state = ActiveRegister;
	}
}

bool AssemblyRegister::hasCustomMemoryLocation() const noexcept
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->hasCustomMemoryLocation();

	return hasCustomMem;
}

bool AssemblyRegister::isZero() const
{
	if (isReferencingOtherRegister())
		return getReferenceTargetRegister()->isZero();

	return !isActive() && isZeroValue;
}

void AssemblyRegister::setWriteBackToMemory(bool shouldWriteBackToMemory)
{
	if (isReferencingOtherRegister())
	{
		getReferenceTargetRegister()->setWriteBackToMemory(shouldWriteBackToMemory);
		return;
	}

	writeBackToMemory = shouldWriteBackToMemory;
}

bool AssemblyRegister::shouldWriteToMemoryAfterStore() const
{
	if (isReferencingOtherRegister())
	{
		return getReferenceTargetRegister()->shouldWriteToMemoryAfterStore();
	}

	return writeBackToMemory;
}

void AssemblyRegister::setMemoryState()
{
	auto memIsValid = memory.hasBase();
	memIsValid |= memory.offset() != 0;

	state = memIsValid ? LoadedMemoryLocation : UnloadedMemoryLocation;
}

#endif

AssemblyRegisterPool::AssemblyRegisterPool(BaseCompiler* c):
	compiler(c)
{
}


void AssemblyRegisterPool::clear()
{
	currentRegisterPool.clear();
}


snex::jit::AssemblyRegisterPool::RegList AssemblyRegisterPool::getListOfAllDirtyGlobals()
{
	RegList l;

#if SNEX_ASMJIT_BACKEND
	for (auto r : currentRegisterPool)
	{
		if (r->isDirtyGlobalMemory())
			l.add(r);
	}
#endif

	return l;
}


snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getRegisterForVariable(BaseScope* scope, const Symbol& s)
{
#if SNEX_ASMJIT_BACKEND
	for (const auto r : currentRegisterPool)
	{
		if (r->matchesScopeAndSymbol(scope, s))
			return r;
	}
#endif

	auto newReg = getNextFreeRegister(scope, s.typeInfo);
	ASMJIT_ONLY(newReg->setReference(scope, s));
	return newReg;
}


snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getActiveRegisterForCustomMem(RegPtr regWithCustomMem)
{
#if SNEX_ASMJIT_BACKEND
	for (auto r : currentRegisterPool)
	{
		if (r->hasCustomMemoryLocation() && r->isActive())
		{
			if (r->getMemoryLocationForReference() == regWithCustomMem->getMemoryLocationForReference())
			{
				return r;
			}
		}
	}
#endif

	return regWithCustomMem;
}

void AssemblyRegisterPool::removeIfUnreferenced(AssemblyRegister::Ptr ref)
{
	auto refCount = ref->getReferenceCount();

	if (refCount == 2)
		currentRegisterPool.removeObject(ref);
}


AssemblyRegister::Ptr AssemblyRegisterPool::getNextFreeRegister(BaseScope* scope, TypeInfo type)
{
	RegPtr newReg = new AssemblyRegister(compiler, type);
	ASMJIT_ONLY(newReg->scope = scope);

	currentRegisterPool.add(newReg);

	return newReg;
}

snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getRegisterWithMemory(RegPtr other)
{
#if SNEX_ASMJIT_BACKEND
	if (!other->hasCustomMemoryLocation())
		return other;

	for (auto r : currentRegisterPool)
	{
		if (!r->isMemoryLocation() && !r->isActive())
			continue;

		if (r == other.get())
			continue;

		if (r->matchesMemoryLocation(other))
		{
			r->numMemoryReferences++;
			return r;
		}
			
	}
#endif

	return other;
}

AssemblyRegisterPool::RegList AssemblyRegisterPool::getListOfAllNamedRegisters()
{
	RegList list;

	for (auto r : currentRegisterPool)
	{
		if (auto ref = r->getVariableId())
			list.add(r);
	}

	return list;
}

snex::Types::ID AssemblyRegisterPool::getRegisterType(const TypeInfo& t) const
{
	return compiler->getRegisterType(t);
}

}
}
