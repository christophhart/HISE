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

class GlobalScope;
class ClassScope;
class RootClassData;
struct StructType;



/** This class holds all data that is used by either the compiler or the JIT itself.

	The lifetime of this object depends on the ScopeType property - local scopes are
	supposed to be destroyed after the compilation, since they are just used to resolve
	local variable values, but the Global and ClassScopes have a longer lifetime that
	is at least as long as the JITted functions that use it.
*/
class BaseScope
{
public:

	enum ScopeType
	{
		Global,		// The global memory pool
		Class,		// The class variable pool
		Function,	// The local variables & parameters
		Anonymous	// the variables of the current block
	};

	ScopeType getScopeType() const;;

	GlobalScope* getGlobalScope();

	ClassScope* getRootClassScope() const;

	NamespacedIdentifier getScopeSymbol() const { return scopeId; }

	BaseScope* getParent();

	BaseScope* getParentWithPath(NamespacedIdentifier& id);

	NamespaceHandler& getNamespaceHandler();

	RootClassData* getRootData() const;

	template <class T> T* getParentScopeOfType()
	{
		if (auto thisAsT = dynamic_cast<T*>(this))
			return thisAsT;

		if (auto p = getParent())
			return p->getParentScopeOfType<T>();
		else
			return nullptr;
	}

public:

	BaseScope(const NamespacedIdentifier& s, BaseScope* parent_ = nullptr);;

	virtual ~BaseScope();;

	BaseScope* getScopeForSymbol( const NamespacedIdentifier& s);

	bool isClassObjectScope() const
	{
		return scopeType == Class && scopeId.isValid();
	}

	BaseScope* getChildScope(const NamespacedIdentifier& s) const
	{
		if (s == scopeId)
			return const_cast<BaseScope*>(this);

		for (auto cs : childScopes)
		{
			if (auto bs = cs->getChildScope(s))
				return bs;
		}

		return nullptr;
	}

	int getNumChildScopes() const
	{
		return childScopes.size();
	}

protected:

	NamespacedIdentifier scopeId;
	WeakReference<BaseScope> parent;
	VariableStorage empty;

	ScopeType scopeType;

private:

	BaseScope* findScopeWithId(const NamespacedIdentifier& id);

	Array<WeakReference<BaseScope>> childScopes;

	BaseScope* getScopeForSymbolInternal(const NamespacedIdentifier& s);
	
	bool hasSymbol(const NamespacedIdentifier& s);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseScope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(BaseScope);
};

struct SymbolWithScope
{
	bool operator==(const SymbolWithScope& other) const
	{
		return s == other.s && scope == other.scope;
	}

	operator bool() const
	{
		return s && scope.get() != nullptr;
	}

	Symbol s;
	WeakReference<BaseScope> scope;
};

struct TableEntry : public ReferenceCountedObject
{
    ~TableEntry()
    {
        data = nullptr;
    };
    
	Symbol s;
	void* data;
	InitialiserList::Ptr initValues;
};

class RootClassData: public FunctionClass
{
	

public:

	juce::String dumpTable() const;

	RootClassData();

	~RootClassData()
	{
		callRootDestructors();
	}

	void enlargeAllocatedSize(const TypeInfo& t)
	{
		jassert(data == nullptr);
		allocatedSize += (int)(t.getRequiredByteSize() + t.getRequiredAlignment());
	}

	void finalise()
	{
		if (allocatedSize % 16 != 0)
		{
			allocatedSize += 16 - (allocatedSize % 16);
		}

		data.allocate(allocatedSize, true);
	}

	bool zeroPadAlign(size_t alignment)
	{
		if (alignment == 0)
		{
			return Result::fail("No data size specified");
		}

		int unAligned = (numUsed % alignment);

		if (unAligned != 0)
		{
			auto numToPad = alignment - unAligned;

			memset(data + numUsed, 0, numToPad);
			numUsed += (int)numToPad;
			return true;
		}

		return false;
	}

	bool contains(const NamespacedIdentifier& s) const
	{
		for (const auto& ts : symbolTable)
			if (ts.s.id == s)
				return true;

		return false;
	}

	bool checkSubClassMembers(const TableEntry& ts, const NamespacedIdentifier& s, const std::function<void(StructType* sc, const Identifier& id)>& f) const;

	Array<Symbol> getAllVariables() const
	{
		Array<Symbol> list;

		for (const auto& t : *this)
			list.add(t.s);

		return list;
	}

	Result initSubClassMembers(ComplexType::Ptr type, const Identifier& memberId, InitialiserList::Ptr initList);

	Result initData(BaseScope* scope, const Symbol& s, InitialiserList::Ptr initValues);

	Result callRootDestructors();

	Result callRootConstructors();

	void* getDataPointer(const NamespacedIdentifier& s) const;

	VariableStorage getDataCopy(const NamespacedIdentifier& s) const
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s.id == s)
			{
				switch (ts.s.typeInfo.getType())
				{
				case Types::ID::Integer: return VariableStorage(*(int*)ts.data);
				case Types::ID::Float:   return VariableStorage(*(float*)ts.data);
				case Types::ID::Double:  return VariableStorage(*(double*)ts.data);
				case Types::ID::Block:   return VariableStorage(*(block*)ts.data);
                default:                 return VariableStorage();
				}
			}
		}

		jassertfalse;
		return {};
	}

	Result allocate(BaseScope* scope, const Symbol& s)
	{
		jassert(scope->getScopeType() == BaseScope::Class);

		// This is the last time you'll get the chance to set the type...
		jassert(s.typeInfo.getType() != Types::ID::Dynamic);

		if (contains(s.id))
			return Result::fail(s.toString() + "already exists");

		auto requiredAlignment = s.typeInfo.getRequiredAlignment();

		zeroPadAlign(requiredAlignment);

		auto size = s.typeInfo.getRequiredByteSize();

		TableEntry newEntry;
		newEntry.s = s;
		newEntry.data = data + numUsed;

		numUsed += (int)size;
		memset(newEntry.data, 0, size);

		jassert(numUsed <= allocatedSize);

		symbolTable.add(newEntry);
		return Result::ok();
	}

	HeapBlock<char> data;

private:

	TableEntry* getTableEntry(const Symbol& s)
	{
		for (auto& t : symbolTable)
		{
			if (t.s == s)
				return &t;
		}

		return nullptr;
	}

	TableEntry* begin() { return symbolTable.begin(); }
	TableEntry* end() { return symbolTable.end(); }

	const TableEntry* begin() const { return symbolTable.begin(); }
	const TableEntry* end() const { return symbolTable.end(); }

	Array<TableEntry> symbolTable;
	
	int numUsed = 0;
	int allocatedSize = 0;
};



}
}
