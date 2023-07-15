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


#if SNEX_ASMJIT_BACKEND
USE_ASMJIT_NAMESPACE;



class SyntaxTree;
class BlockScope;
class BaseScope;

struct AssemblyMemory
{
	AssemblyMemory cloneWithOffset(int offset)
	{
		AssemblyMemory c;
		c.cc = cc;
		c.memory = memory.cloneAdjusted(offset);
		return c;
	}

	X86Mem memory;
	void* cc = nullptr;
};

#define GET_COMPILER_FROM_INIT_DATA(initData) *static_cast<asmjit::X86Compiler*>(initData.asmPtr->cc);


/** A high level, reference counted assembly register. */
class AssemblyRegister : public ReferenceCountedObject
{
public:

	using IntRegisterType = x86::Gpd;
	
	using FloatRegisterType = X86Xmm;
	using DoubleRegisterType = X86Xmm;
	using FloatMemoryType = X86Mem;
	using Ptr = ReferenceCountedObjectPtr<AssemblyRegister>;
	using List = ReferenceCountedArray<AssemblyRegister>;

	enum State
	{
		UnloadedMemoryLocation,
		LoadedMemoryLocation,
		InactiveRegister,
		ActiveRegister,
		DirtyGlobalRegister,
		RedirectedRegister,
		numStates
	};

private:

	AssemblyRegister(BaseCompiler* compiler, TypeInfo type_);
	
public:

	bool matchesMemoryLocation(Ptr other) const;

	bool isGlobalMemory() const;

	bool shouldLoadMemoryIntoRegister() const;

	void setReference(BaseScope* scope, const Symbol& ref);

	const Symbol& getVariableId() const;

	bool operator==(const Symbol& s) const;

	bool isDirtyGlobalMemory() const;

	void reinterpretCast(const TypeInfo& newType);

	Types::ID getType() const;

	TypeInfo getTypeInfo() const { return type; }

	void* getGlobalDataPointer();

	template <typename T> bool getImmediateValue(T& v)
	{
		if (memoryLocation != nullptr)
		{
			jassert(Types::Helpers::getTypeFromTypeId<T>() == getType());

			v = *reinterpret_cast<T*>(memoryLocation);
			return true;
		}

		return false;
	}

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

	void changeComplexType(ComplexType::Ptr newTypePtr)
	{
		jassert(type.isComplexType());
		type = TypeInfo(newTypePtr);
	}

	bool isValid() const;

	bool isGlobalVariableRegister() const;

	bool isActive() const;

	bool isSameRegisterSource(Ptr other) const;

	bool matchesScopeAndSymbol(BaseScope* scopeToCheck, const Symbol& symbol) const;

#if 0
	bool isActiveOrDirtyGlobalRegister() const
	{
		if (isReferencingOtherRegister())
			return getReferenceTargetRegister()->isActiveOrDirtyGlobalRegister();

		return state == ActiveRegister || state == DirtyGlobalRegister;
	}
#endif

	/** Creates a memory location for the given constant. */
	void createMemoryLocation(asmjit::X86Compiler& cc);

	void createRegister(asmjit::X86Compiler& cc);

	bool isMemoryLocation() const;

	void setCustomMemoryLocation(X86Mem newLocation, bool isGlobalMemory);

	void setDataPointer(void* memLoc, bool globalMemory_);

	void setImmediateValue(int64 value);

	bool isImmediate() const;

	bool isUnloadedImmediate() const 
	{ 
		return hasImmediateValue && state == UnloadedMemoryLocation; 
	}

	void setIsIteratorRegister(bool isIterator)
	{
		isIter = isIterator;
	}

	void invalidateRegisterForCustomMemory();

	void clearAfterReturn();

	bool isIteratorRegister() const;

	bool isSimd4Float() const;

	void setDirtyFloat4(Ptr source, int byteOffset);

	void setUndirty();

	bool hasCustomMemoryLocation() const noexcept;

	bool isZero() const;

	void setReferToReg(Ptr otherPtr)
	{
		if (otherPtr->getTypeInfo().isNativePointer())
		{
			auto ptr = x86::ptr(otherPtr->getRegisterForReadOp().as<X86Gpq>());
			setCustomMemoryLocation(ptr, otherPtr->isGlobalMemory());
		}
		else
		{
			referenceTarget = otherPtr->getReferenceTargetRegister();
			state = RedirectedRegister;
			memory = {};
			reg = {};
		}
	}

	bool isReferencingOtherRegister() const { return getReferenceTargetRegister() != this; }

	Ptr getReferenceTargetRegister() const
	{
		if (referenceTarget != nullptr)
			return referenceTarget->getReferenceTargetRegister();

		return const_cast<AssemblyRegister*>(this);
	}

	void setWriteBackToMemory(bool shouldWriteBackToMemory);

	bool shouldWriteToMemoryAfterStore() const;

private:

	void setMemoryState();

	Ptr referenceTarget;

	int numMemoryReferences = 0;

	int debugId = 0;
	friend class AssemblyRegisterPool;

	X86Reg partReg1;
	X86Reg partReg2;

	bool hasCustomMem = false;
	bool globalMemory = false;
	bool isIter = false;
	bool isZeroValue = false;

	State state = State::InactiveRegister;
	bool initialised = false;
	bool dirty = false;

#if REMOVE_REUSABLE_REG
	bool reusable = false;
#endif

	bool hasImmediateValue = false;
	int immediateIntValue = 0;

	bool writeBackToMemory = false;

	TypeInfo type;
	asmjit::X86Mem memory;
	asmjit::X86Reg reg;
	void* memoryLocation = nullptr;
	WeakReference<BaseScope> scope;
	WeakReference<BaseCompiler> compiler;
	Symbol id;
};

#else

struct AssemblyMemory
{
	void* cc = nullptr;
};

class AssemblyRegister : public ReferenceCountedObject
{
public:

	AssemblyRegister(BaseCompiler* compiler, TypeInfo type_) { jassertfalse; };

	using Ptr = ReferenceCountedObjectPtr<AssemblyRegister>;
	using List = ReferenceCountedArray<AssemblyRegister>;

	TypeInfo getTypeInfo() const { jassertfalse; return TypeInfo(Types::ID::Void); }
	void loadMemoryIntoRegister(AsmJitX86Compiler& cc, bool forceLoad = false) { jassertfalse; }
	void createMemoryLocation(AsmJitX86Compiler& cc) { jassertfalse; }
	bool isValid() const { jassertfalse; return false; }
	bool isActive() const { return isValid(); }

	const Symbol& getVariableId() const
	{
		jassertfalse;
        return s;
	}
    
    Symbol s;
};

#endif

class AssemblyRegisterPool
{
public:

	using RegPtr = AssemblyRegister::Ptr;
	using RegList = ReferenceCountedArray<AssemblyRegister>;

	AssemblyRegisterPool(BaseCompiler* c);

	void clear();
	RegList getListOfAllDirtyGlobals();
	RegPtr getRegisterForVariable(BaseScope* scope, const Symbol& variableId);

	RegPtr getActiveRegisterForCustomMem(RegPtr regWithCustomMem);

	void removeIfUnreferenced(AssemblyRegister::Ptr ref);
	RegPtr getNextFreeRegister(BaseScope* scope, TypeInfo type);

	RegPtr getRegisterWithMemory(RegPtr other);

	RegList getListOfAllNamedRegisters();

	Types::ID getRegisterType(const TypeInfo& t) const;

private:

	ReferenceCountedArray<AssemblyRegister> currentRegisterPool;
	BaseCompiler* compiler;
};



}
}
