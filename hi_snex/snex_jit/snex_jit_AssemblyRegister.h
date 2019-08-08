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

#pragma once

namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;


class SyntaxTree;
class BlockScope;

/** A high level, reference counted assembly register. */
class AssemblyRegister : public ReferenceCountedObject
{
public:

#if JUCE_64BIT
	using IntRegisterType = X86Gpq;
#else
	using IntRegisterType = X86Gpd;
#endif
	using FloatRegisterType = X86Xmm;
	using DoubleRegisterType = X86Xmm;
	using FloatMemoryType = X86Mem;
	using Ptr = ReferenceCountedObjectPtr<AssemblyRegister>;

	enum State
	{
		UnloadedMemoryLocation,
		LoadedMemoryLocation,
		InactiveRegister,
		ActiveRegister,
		DirtyGlobalRegister,
		ReusableRegister,
		numStates
	};

private:

	AssemblyRegister(Types::ID type_);

public:

	void setReference(BaseScope::RefPtr ref);

	BaseScope::Reference* getVariableId() const;

	bool operator==(const BaseScope::RefPtr s) const;

	bool isDirtyGlobalMemory() const;

	void flagForReuseIfAnonymous();

	void flagForReuse();

	bool canBeReused() const;

	Types::ID getType() const { return type; }

	void* getGlobalDataPointer();

	X86Reg getRegisterForReadOp();

	/** Returns the register and flags global registers as dirty. 
	
		Use this for write operations.
	*/
	X86Reg getRegisterForWriteOp();

	X86Mem getAsMemoryLocation();

	int64 getImmediateIntValue();

	/** Loads the memory into the register. */
	void loadMemoryIntoRegister(asmjit::X86Compiler& cc);

	bool isGlobalVariableRegister() const;

	bool isActive() const;

	bool isActiveOrDirtyGlobalRegister() const;

	/** Creates a memory location for the given constant. */
	void createMemoryLocation(asmjit::X86Compiler& cc);

	void createRegister(asmjit::X86Compiler& cc);

	bool isMemoryLocation() const;

	void setDataPointer(VariableStorage* memLoc);

	void setIsIteratorRegister(bool isIterator)
	{
		isIter = isIterator;
	}

	bool isIteratorRegister() const
	{
		if (isIter)
		{
			jassert(state == ActiveRegister);
			return true;
		}

		return false;
	}

private:

	friend class AssemblyRegisterPool;

	X86Reg partReg1;
	X86Reg partReg2;

	bool isIter = false;

	State state = State::InactiveRegister;
	bool initialised = false;
	bool dirty = false;
	bool reusable = false;
	int immediateIntValue = 0;
	Types::ID type;
	asmjit::X86Mem memory;
	asmjit::X86Reg reg;
	VariableStorage* memoryLocation = nullptr;
	WeakReference<BaseScope::Reference> variableId;
};

class AssemblyRegisterPool
{
public:

	using RegPtr = AssemblyRegister::Ptr;
	using RegList = ReferenceCountedArray<AssemblyRegister>;

	AssemblyRegisterPool();

	void clear();
	RegList getListOfAllDirtyGlobals();
	RegPtr getRegisterForVariable(const BaseScope::RefPtr variableId);

	void removeIfUnreferenced(AssemblyRegister::Ptr ref);
	AssemblyRegister::Ptr getNextFreeRegister(Types::ID type);

private:

	ReferenceCountedArray<AssemblyRegister> currentRegisterPool;
};

}
}