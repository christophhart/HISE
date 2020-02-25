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
class BaseScope;

/** A high level, reference counted assembly register. */
class AssemblyRegister : public ReferenceCountedObject
{
public:

	using IntRegisterType = x86::Gpd;
	
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

	

	void setReference(BaseScope* scope, const Symbol& ref);

	const Symbol& getVariableId() const;

	bool operator==(const Symbol& s) const;

	bool isDirtyGlobalMemory() const;

	void flagForReuseIfAnonymous();

	void flagForReuse(bool forceReuse=false);

	bool canBeReused() const;

	Types::ID getType() const { return type; }

	void* getGlobalDataPointer();

	X86Reg getRegisterForReadOp();

	/** Returns the register and flags global registers as dirty. 
	
		Use this for write operations.
	*/
	X86Reg getRegisterForWriteOp();

	X86Mem getAsMemoryLocation();

	X86Mem getMemoryLocationForReference();

	int64 getImmediateIntValue();

	/** Loads the memory into the register. */
	void loadMemoryIntoRegister(asmjit::X86Compiler& cc, bool forceLoad=false);

	BaseScope* getScope() const { return scope.get(); }

	bool isGlobalVariableRegister() const;

	bool isActive() const;

	bool matchesScopeAndSymbol(BaseScope* scopeToCheck, const Symbol& symbol) const;

	bool isActiveOrDirtyGlobalRegister() const;

	/** Creates a memory location for the given constant. */
	void createMemoryLocation(asmjit::X86Compiler& cc);

	void createRegister(asmjit::X86Compiler& cc);

	bool isMemoryLocation() const;

	void setCustomMemoryLocation(X86Mem newLocation);

	void setDataPointer(void* memLoc);

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

	bool isSimd4Float() const
	{
		return id.typePtr != nullptr && id.typePtr->hasAlias() && id.typePtr->toString() == "float4";
	}

	void clearForReuse();

	void setUndirty();

	bool hasCustomMemoryLocation() const noexcept 
	{
		return hasCustomMem;
	}

	bool isZero() const
	{
		return isZeroValue;
	}

private:

	friend class AssemblyRegisterPool;

	X86Reg partReg1;
	X86Reg partReg2;

	bool hasCustomMem = false;
	bool isIter = false;
	bool isZeroValue = false;

	State state = State::InactiveRegister;
	bool initialised = false;
	bool dirty = false;
	bool reusable = false;
	int immediateIntValue = 0;
	Types::ID type;
	asmjit::X86Mem memory;
	asmjit::X86Reg reg;
	void* memoryLocation = nullptr;
	WeakReference<BaseScope> scope;
	Symbol id;
};

class AssemblyRegisterPool
{
public:

	using RegPtr = AssemblyRegister::Ptr;
	using RegList = ReferenceCountedArray<AssemblyRegister>;

	AssemblyRegisterPool();

	void clear();
	RegList getListOfAllDirtyGlobals();
	RegPtr getRegisterForVariable(BaseScope* scope, const Symbol& variableId);

	RegPtr getActiveRegisterForCustomMem(RegPtr regWithCustomMem);

	void removeIfUnreferenced(AssemblyRegister::Ptr ref);
	AssemblyRegister::Ptr getNextFreeRegister(BaseScope* scope, Types::ID type);

	RegList getListOfAllNamedRegisters();

private:

	ReferenceCountedArray<AssemblyRegister> currentRegisterPool;
};

}
}