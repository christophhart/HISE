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

static int counter = 0;

AssemblyRegister::AssemblyRegister(TypeInfo type_) :
	type(type_)
{
	debugId = counter++;
}

bool AssemblyRegister::matchesMemoryLocation(Ptr other) const
{
	auto bothAreMemory = hasCustomMemoryLocation() &&
		other->hasCustomMemoryLocation();

	auto typeMatch = other->getTypeInfo() == getTypeInfo();

	if (typeMatch && bothAreMemory)
	{
		auto m = other->getAsMemoryLocation();
		return m == memory;
	}

	return false;
}

bool AssemblyRegister::isGlobalMemory() const
{
	return (hasCustomMem && globalMemory) || isGlobalVariableRegister() || id.isReference();
}

bool AssemblyRegister::shouldLoadMemoryIntoRegister() const
{
	return numMemoryReferences > 0;
}

void AssemblyRegister::setReference(BaseScope* s, const Symbol& ref)
{
	scope = s->getScopeForSymbol(ref.id);
	id = ref;
	jassert(id.typeInfo == type);
}


const Symbol& AssemblyRegister::getVariableId() const
{
	return id;
}


bool AssemblyRegister::isDirtyGlobalMemory() const
{
	return dirty;
}


void AssemblyRegister::flagForReuseIfAnonymous()
{
	if (!id)
		flagForReuse();
}


void AssemblyRegister::flagForReuse(bool forceReuse)
{
	if (!forceReuse)
	{
		if (!isActive())
			return;

		if (isIter)
			return;

		if (dirty)
			return;
	}

	reusable = true;
	state = ReusableRegister;
}


void AssemblyRegister::removeReuseFlag()
{
	reusable = false;
	state = ActiveRegister;
}

bool AssemblyRegister::canBeReused() const
{
	return reusable;
}


void* AssemblyRegister::getGlobalDataPointer()
{
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
	jassert(state == ActiveRegister ||
		state == ReusableRegister ||
		state == DirtyGlobalRegister);

	jassert(reg.isValid());
	return reg;
}


asmjit::X86Reg AssemblyRegister::getRegisterForWriteOp()
{
	jassert(state == ActiveRegister ||
		state == ReusableRegister ||
		state == DirtyGlobalRegister);

	jassert(scope != nullptr);

	if (isGlobalMemory())
		dirty = true;

	if (id)
	{
		if (isIter)
			dirty = true;

		auto sToUse = scope->getScopeForSymbol(id.id);

		jassert(sToUse != nullptr);

		auto scopeType = sToUse->getScopeType();

		if (!isIter && (scopeType == BaseScope::Class || id.isReference()))
		{
			dirty = true;
			state = DirtyGlobalRegister;
		}
		else if (scopeType == BaseScope::Global)
			throw juce::String("can't write to global variables");
	}

	jassert(reg.isValid());
	return reg;
}


asmjit::X86Mem AssemblyRegister::getAsMemoryLocation()
{
	jassert(state == LoadedMemoryLocation);
	//jassert(type != Types::ID::Integer);

	return memory;
}


asmjit::X86Mem AssemblyRegister::getMemoryLocationForReference()
{
	jassert(memory.isMem());

	return memory;
}

juce::int64 AssemblyRegister::getImmediateIntValue()
{
	jassert(state == LoadedMemoryLocation);
	jassert(getType() == Types::ID::Integer);
	jassert(!hasCustomMem);

	return static_cast<int64>(immediateIntValue);
}

bool AssemblyRegister::operator==(const Symbol& s) const
{
	return id == s;
}

void AssemblyRegister::loadMemoryIntoRegister(asmjit::X86Compiler& cc, bool forceLoad)
{
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

	switch (getType())
	{
	case Types::ID::Float: e = cc.movss(reg.as<X86Xmm>(), memory); break;
	case Types::ID::Double: e = cc.movsd(reg.as<X86Xmm>(), memory); break;
	case Types::ID::Block:
	case Types::ID::Integer:
	{
		if (hasCustomMem)
			e = cc.mov(reg.as<IntRegisterType>(), memory);
		else
			e = cc.mov(reg.as<IntRegisterType>(), static_cast<int64_t>(immediateIntValue));

		break;
	}
	case Types::ID::Pointer:
	{
		if (isSimd4Float())
		{
			jassert(reg.isXmm());
			e = cc.movaps(reg.as<X86Xmm>(), memory);
		}
		else
		{
			if (hasCustomMem)
				e = cc.lea(reg.as<X86Gpq>(), memory);
			else if (memory.hasOffset() && !memory.hasBaseOrIndex())
				e = cc.mov(reg.as<X86Gpq>(), memory.offset());
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
	return state == ActiveRegister && reg.isValid();
}

bool AssemblyRegister::isGlobalVariableRegister() const
{
	return scope->getRootClassScope()->rootData->contains(id.id);
}

bool AssemblyRegister::isActive() const
{
	return state == ActiveRegister;
}

bool AssemblyRegister::matchesScopeAndSymbol(BaseScope* scopeToCheck, const Symbol& symbol) const
{
	auto scopeMatches = scopeToCheck->getScopeForSymbol(symbol.id) == scope;
	auto symbolMatches = symbol == id;

	return scopeMatches && symbolMatches;
}

bool AssemblyRegister::isActiveOrDirtyGlobalRegister() const
{
	return state == ActiveRegister || state == DirtyGlobalRegister;
}

void AssemblyRegister::createMemoryLocation(asmjit::X86Compiler& cc)
{
	jassert(memoryLocation != nullptr);

	if (getType() != Types::ID::Pointer && isGlobalVariableRegister() && !id.isConst())
	{
		auto t = getType();

		bool useQword = (t == Types::ID::Double ||
			t == Types::ID::Block ||
			t == Types::ID::Pointer);

        auto r = cc.newGpq();
        
        cc.mov(r, (uint64_t)memoryLocation);
        
		memory = useQword ? x86::qword_ptr(r) : x86::dword_ptr(r);
		hasCustomMem = true;
		state = State::LoadedMemoryLocation;
	}
	else
	{
		if (getType() == Types::ID::Float)
		{
			auto v = *reinterpret_cast<float*>(memoryLocation);
			isZeroValue = v == 0.0f;
			
			memory = cc.newFloatConst(ConstPool::kScopeLocal, v);
		}
		if (getType() == Types::ID::Double)
		{
			auto v = *reinterpret_cast<double*>(memoryLocation);
			isZeroValue = v == 0.0;

			memory = cc.newDoubleConst(ConstPool::kScopeLocal, v);
		}
		if (getType() == Types::ID::Integer)
		{
			immediateIntValue = *reinterpret_cast<int*>(memoryLocation);
			isZeroValue = immediateIntValue == 0;
		}
		if (type == Types::ID::Block)
		{
			jassertfalse;
#if 0
			uint8* d = reinterpret_cast<uint8*>(memoryLocation);
			asmjit::Data128 data = asmjit::Data128::fromU8(d[0], d[1], d[2], d[3],
				d[4], d[5], d[6], d[7],
				d[8], d[9], d[10], d[11],
				d[12], d[13], d[14], d[15]);

			memory = cc.newXmmConst(ConstPool::kScopeLocal, data);
#endif
		}
		if (getType() == Types::ID::Pointer)
		{
			memory = x86::qword_ptr((uint64_t)reinterpret_cast<VariableStorage*>(memoryLocation)->getDataPointer());
		}
			
		state = State::LoadedMemoryLocation;
		jassert(memory.isMem());
	}
}


void AssemblyRegister::createRegister(asmjit::X86Compiler& cc)
{
	jassert(getType() != Types::ID::Dynamic);

	if (reg.isValid())
	{
		// From now on we can use it just like a regular register
		if (state == ReusableRegister)
			state = ActiveRegister;

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
	return state == LoadedMemoryLocation;
}


void AssemblyRegister::setCustomMemoryLocation(X86Mem newLocation, bool isGlobalMemory_)
{
	memory = newLocation;
	dirty = false;
	globalMemory = isGlobalMemory_;
	reg = {};
	jassert(memory.isMem());
	state = LoadedMemoryLocation;
	hasCustomMem = true;
}

void AssemblyRegister::setDataPointer(void* memLoc)
{
	memoryLocation = memLoc;
	reg = {};
	state = State::UnloadedMemoryLocation;
	hasCustomMem = false;
}


void AssemblyRegister::clearForReuse()
{
	isIter = false;
	state = ReusableRegister;
	dirty = false;
	reusable = false;
	immediateIntValue = 0;
	memoryLocation = nullptr;
	scope = nullptr;
	id = {};
}

void AssemblyRegister::setUndirty()
{
	jassert(state == DirtyGlobalRegister || isIter || isGlobalMemory());

	dirty = false;
	state = ActiveRegister;
}

AssemblyRegisterPool::AssemblyRegisterPool()
{
}


void AssemblyRegisterPool::clear()
{
	currentRegisterPool.clear();
}


snex::jit::AssemblyRegisterPool::RegList AssemblyRegisterPool::getListOfAllDirtyGlobals()
{
	RegList l;

	for (auto r : currentRegisterPool)
	{
		if (r->isDirtyGlobalMemory())
			l.add(r);
	}

	return l;
}


snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getRegisterForVariable(BaseScope* scope, const Symbol& s)
{
	for (const auto r : currentRegisterPool)
	{
		if (r->matchesScopeAndSymbol(scope, s))
			return r;
	}

	auto newReg = getNextFreeRegister(scope, s.typeInfo);
	newReg->setReference(scope, s);
	return newReg;
}


snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getActiveRegisterForCustomMem(RegPtr regWithCustomMem)
{
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

	return nullptr;
}

void AssemblyRegisterPool::removeIfUnreferenced(AssemblyRegister::Ptr ref)
{
	auto refCount = ref->getReferenceCount();

	if (refCount == 2)
		currentRegisterPool.removeObject(ref);
}


AssemblyRegister::Ptr AssemblyRegisterPool::getNextFreeRegister(BaseScope* scope, TypeInfo type)
{
	for (auto r : currentRegisterPool)
	{
		if (r->getType() == type.getType() && r->canBeReused())
		{
			r->clearForReuse();
			r->scope = scope;
			r->type = type;
			return r;
		}
	}

	RegPtr newReg = new AssemblyRegister(type);
	newReg->scope = scope;

	currentRegisterPool.add(newReg);

	return newReg;
}

snex::jit::AssemblyRegisterPool::RegPtr AssemblyRegisterPool::getRegisterWithMemory(RegPtr other)
{
	if (!other->hasCustomMemoryLocation())
		return other;

	for (auto r : currentRegisterPool)
	{
		if (!r->isMemoryLocation())
			continue;

		if (r == other.get())
			continue;

		if (r->matchesMemoryLocation(other))
		{
			other->clearForReuse();
			r->numMemoryReferences++;
			return r;
		}
			
	}

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

}
}
