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

AssemblyRegister::AssemblyRegister(Types::ID type_) :
	type(type_)
{

}


void AssemblyRegister::setReference(BaseScope* s, const Symbol& ref)
{
	scope = s;
	id = ref;
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


bool AssemblyRegister::canBeReused() const
{
	return reusable;
}


void* AssemblyRegister::getGlobalDataPointer()
{
	jassert(scope != nullptr);

	if (isGlobalVariableRegister())
		return scope->getRootClassScope()->rootData->get(id).getDataPointer();

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

	if (id)
	{
		auto scopeType = scope->getScopeForSymbol(id)->getScopeType();

		if (scopeType == BaseScope::Class)
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


juce::int64 AssemblyRegister::getImmediateIntValue()
{
	jassert(state == LoadedMemoryLocation);
	jassert(type == Types::ID::Integer);

	return immediateIntValue;
}

bool AssemblyRegister::operator==(const Symbol& s) const
{
	return id == s;
}

void AssemblyRegister::loadMemoryIntoRegister(asmjit::X86Compiler& cc)
{
	if (reg.isValid())
		return;

	if (state == UnloadedMemoryLocation)
		createMemoryLocation(cc);

	// Global variables will be loaded into a register already.
	if (state == ActiveRegister)
	{
		jassert(reg.isValid());
		return;
	}

	createRegister(cc);

	asmjit::Error e = asmjit::kErrorOk;

	if (type == Types::ID::Float)
		e = cc.movss(reg.as<X86Xmm>(), memory);
	else if (type == Types::ID::Double)
		e = cc.movsd(reg.as<X86Xmm>(), memory);
	else if (type == Types::ID::Integer)
		e = cc.mov(reg.as<IntRegisterType>(), immediateIntValue);
	else if (type == Types::ID::Block || type == Types::ID::Event)
		e = cc.mov(reg.as<X86Gpq>(), memory);
	else
		jassertfalse;

	state = ActiveRegister;
	jassert(e == 0);
}


bool AssemblyRegister::isGlobalVariableRegister() const
{
	return scope->getRootClassScope()->rootData->contains(id);
}

bool AssemblyRegister::isActive() const
{
	return state == ActiveRegister;
}

bool AssemblyRegister::matchesScopeAndSymbol(BaseScope* scopeToCheck, const Symbol& symbol) const
{
	return scopeToCheck == scope && symbol == id;
}

bool AssemblyRegister::isActiveOrDirtyGlobalRegister() const
{
	return state == ActiveRegister || state == DirtyGlobalRegister;
}

void AssemblyRegister::createMemoryLocation(asmjit::X86Compiler& cc)
{
	jassert(memoryLocation != nullptr);

	bool isConst = false;
	jassertfalse;

	if (isGlobalVariableRegister() && !isConst)
	{
		// We can't use the value as constant memory location because it might be changed
		// somewhere else.
		AsmCodeGenerator acg(cc, nullptr, type);
		createRegister(cc);
		acg.emitMemoryLoad(this);
	}
	else
	{
		if (type == Types::ID::Float)
			memory = cc.newFloatConst(ConstPool::kScopeLocal, memoryLocation->toFloat());
		if (type == Types::ID::Double)
			memory = cc.newDoubleConst(ConstPool::kScopeLocal, memoryLocation->toDouble());
		if (type == Types::ID::Integer)
			immediateIntValue = memoryLocation->toInt();
		if (type == Types::ID::Event || type == Types::ID::Block)
		{
			uint8* d = reinterpret_cast<uint8*>(memoryLocation);
			asmjit::Data128 data = asmjit::Data128::fromU8(d[0], d[1], d[2], d[3],
				d[4], d[5], d[6], d[7],
				d[8], d[9], d[10], d[11],
				d[12], d[13], d[14], d[15]);

			memory = cc.newXmmConst(ConstPool::kScopeLocal, data);
		}

		state = State::LoadedMemoryLocation;
		jassert(memory.isMem());
	}
}


void AssemblyRegister::createRegister(asmjit::X86Compiler& cc)
{
	if (reg.isValid())
	{
		// From now on we can use it just like a regular register
		if (state == ReusableRegister)
			state = ActiveRegister;

		jassert(state == ActiveRegister || 
			    state == DirtyGlobalRegister);
		return;
	}

	if (type == Types::ID::Float)
		reg = cc.newXmmSs();
	if (type == Types::ID::Double)
		reg = cc.newXmmSd();
	if (type == Types::ID::Integer)
		reg = cc.newGpd();
	if (type == Types::Event || type == Types::Block)
		reg = cc.newIntPtr();

	state = ActiveRegister;
}


bool AssemblyRegister::isMemoryLocation() const
{
	return state == LoadedMemoryLocation;
}


void AssemblyRegister::setDataPointer(VariableStorage* memLoc)
{
	memoryLocation = memLoc;
	reg = {};
	state = State::UnloadedMemoryLocation;
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
	jassert(state == DirtyGlobalRegister);

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

	auto newReg = getNextFreeRegister(scope, s.type);
	newReg->setReference(scope, s);
	return newReg;
}


void AssemblyRegisterPool::removeIfUnreferenced(AssemblyRegister::Ptr ref)
{
	auto refCount = ref->getReferenceCount();

	if (refCount == 2)
		currentRegisterPool.removeObject(ref);
}


AssemblyRegister::Ptr AssemblyRegisterPool::getNextFreeRegister(BaseScope* scope, Types::ID type)
{
	for (auto r : currentRegisterPool)
	{
		if (r->getType() == type && r->canBeReused())
		{
			r->clearForReuse();
			r->scope = scope;
			return r;
		}
	}

	RegPtr newReg = new AssemblyRegister(type);
	newReg->scope = scope;

	currentRegisterPool.add(newReg);

	return newReg;
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
