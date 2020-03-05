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




/** This class holds all data that is used by either the compiler or the JIT itself.

	The lifetime of this object depends on the ScopeType property - local scopes are
	supposed to be destroyed after the compilation, since they are just used to resolve
	local variable values, but the Global and ClassScopes have a longer lifetime that
	is at least as long as the JITted functions that use it.
*/
class BaseScope
{
public:

	using Symbol = snex::jit::Symbol;

	enum ScopeType
	{
		Global,		// The global memory pool
		Class,		// The class variable pool
		Function,	// The local variables & parameters
		Anonymous	// the variables of the current block
	};

	struct Constant
	{
		Symbol id;
		const VariableStorage v;
	};

	struct ChildClass
	{
		juce::String::CharPointerType classStart;
		juce::String::CharPointerType wholeProgram;
		Identifier id;
		int length;
		int typeIndex;
	};

	ScopeType getScopeType() const;;

	GlobalScope* getGlobalScope();

	ClassScope* getRootClassScope() const;

	Symbol getScopeSymbol() const { return scopeId; }

	bool addConstant(const Identifier& id, VariableStorage v);

	BaseScope* getParent();

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

	BaseScope(const Symbol& s, BaseScope* parent_ = nullptr, int numVariables = 1024);;

	virtual ~BaseScope();;

	BaseScope* getScopeForSymbol(const Symbol& s);

	/** Override this, allocate the variable and return true if success. */
	virtual bool addVariable(const Symbol& s)
	{
		jassertfalse;
		return false;
	}

	virtual bool hasVariable(const Identifier& id) const;

	/** Override this and update the symbol type and constness. */
	virtual bool updateSymbol(Symbol& symbolToBeUpdated);

	Array<Symbol> getAllVariables() const;

#if 0
	void getChildClassCode(const Identifier& classId, juce::String::CharPointerType& start, juce::String::CharPointerType& wholeProgram, int& length) const
	{
		if (!hasChildClass(classId))
			jassertfalse;

		for (auto& c : childClasses)
		{
			if (c.id == classId)
			{
				start = c.classStart;
				wholeProgram = c.wholeProgram;
				length = c.length;
				return;
			}
		}
	}

	bool hasChildClass(const Identifier& classId) const
	{
		return classTypes.getTypeIndex(classId) != -1;
	}

	void registerClass(const Identifier& classId, const juce::String::CharPointerType& start, const juce::String::CharPointerType& whole, int length)
	{
		if (hasChildClass(classId))
			throw juce::String("Already defined");

		childClasses.add({ start, whole, classId,  length, classTypes.registerType(classId) });
	}
#endif

	bool isClassObjectScope() const
	{
		return scopeType == Class && scopeId;
	}

	BaseScope* getChildScope(const Symbol& s) const
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

	Symbol scopeId;
	WeakReference<BaseScope> parent;
	VariableStorage empty;

	//Array<ChildClass> childClasses;

	ScopeType scopeType;
	Array<Constant> constants;

	//ReferenceCountedArray<Reference> referencedVariables;

	//Array<InternalReference> allocatedVariables;

	private:

	Array<WeakReference<BaseScope>> childScopes;

	BaseScope* getScopeForSymbolInternal(const Symbol& s);
	
	bool hasSymbol(const Symbol& s);

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


class RootClassData: public FunctionClass
{
	struct TableEntry : public ReferenceCountedObject
	{
		Symbol s;
		void* data;
		WeakReference<BaseScope> scope;
	};

public:

	juce::String dumpTable() const;

	RootClassData();

	bool updateSymbol(Symbol& toBeUpdated)
	{
		for (const auto& td : *this)
		{
			if (td.s == toBeUpdated)
			{
				toBeUpdated = td.s;
				return true;
			}
		}

		return false;
	}

	void enlargeAllocatedSize(const TypeInfo& t)
	{
		jassert(data == nullptr);
		allocatedSize += t.getRequiredByteSize();
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
			numUsed += numToPad;
			return true;
		}

		return false;
	}

	

	bool contains(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
			if (ts.s == s)
				return true;

		return false;
	}

	Types::ID getTypeForVariable(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s == s)
				return ts.s.typeInfo.getType();
		}

		return Types::ID::Dynamic;
	}

	bool checkSubClassMembers(const TableEntry& ts, const Symbol& s, const std::function<void(StructType* sc, const Identifier& id)>& f) const
	{
		if (ts.s.isParentOf(s) && ts.s.typeInfo.isComplexType())
		{
			auto path = s.getPath();
			path.remove(0);

			auto tp = ts.s.typeInfo.getComplexType();

			for (int i = 0; i < path.size(); i++)
			{
				auto thisMember = path[i];
				auto st = dynamic_cast<StructType*>(tp.get());

				if (st == nullptr)
					break;

				f(st, thisMember);

				tp = st->getMemberComplexType(thisMember).get();
			}

			return true;
		}

		return false;
	}

	ComplexType::Ptr getComplexTypeForVariable(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s == s)
				return ts.s.typeInfo.getComplexType();

			ComplexType::Ptr p;

			if(checkSubClassMembers(ts, s, [&p](StructType* c, const Identifier& id)
			{
				p = c->getMemberComplexType(id);
			}))
				return p;
		}

		return nullptr;
	}

	Array<Symbol> getAllVariables() const
	{
		Array<Symbol> list;

		for (const auto& t : *this)
			list.add(t.s);

		return list;
	}

	Result initSubClassMembers(ComplexType::Ptr type, const Identifier& memberId, InitialiserList::Ptr initList)
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s.typeInfo.isComplexType())
			{
				ts.s.typeInfo.getComplexType()->forEach([memberId, initList](ComplexType::Ptr p, void* dataPointer)
				{
					if (auto structType = dynamic_cast<StructType*>(p.get()))
					{
						auto offset = structType->getMemberOffset(memberId);

						if (structType->isNativeMember(memberId))
						{
							VariableStorage initValue;

							jassert(initList->size() == 1);

							initList->getValue(0, initValue);

							ComplexType::writeNativeMemberType(dataPointer, offset, initValue);
							return false;
						}
						else
						{
							auto memberType = structType->getMemberComplexType(memberId);
							auto ptr = ComplexType::getPointerWithOffset(dataPointer, offset);
							memberType->initialise(ptr, initList);
							return false;
						}
					}

					p->initialise(dataPointer, initList);

					return false;
				}, type, ts.data);
			}
		}

		return Result::ok();
	}


	Result initData(BaseScope* scope, const Symbol& s, InitialiserList::Ptr initValues);

	void* getDataPointer(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s == s)
				return ts.data;

			uint64_t data = reinterpret_cast<uint64_t>(ts.data);
			
			if(checkSubClassMembers(ts, s, [&data](StructType* sc, const Identifier& id)
			{
				 data += sc->getMemberOffset(id);
			}))
				return (void*)data;
		}
			
		jassertfalse;
		return nullptr;
	}

	VariableStorage getDataCopy(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s == s)
			{
				switch (ts.s.typeInfo.getType())
				{
				case Types::ID::Integer: return VariableStorage(*(int*)ts.data);
				case Types::ID::Float: return VariableStorage(*(float*)ts.data);
				case Types::ID::Double: return VariableStorage(*(double*)ts.data);
				case Types::ID::Event: return VariableStorage(*(HiseEvent*)ts.data);
				case Types::ID::Block: return VariableStorage(*(block*)ts.data);
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

		if (contains(s))
			return Result::fail(s.toString() + "already exists");

		auto requiredAlignment = s.typeInfo.getRequiredAlignment();

		zeroPadAlign(requiredAlignment);

		auto size = s.typeInfo.getRequiredByteSize();

		TableEntry newEntry;
		newEntry.s = s;
		newEntry.data = data + numUsed;

		numUsed += size;
		memset(newEntry.data, 0, size);
		newEntry.scope = scope;

		jassert(numUsed <= allocatedSize);

		symbolTable.add(newEntry);
		return Result::ok();
	}

	HeapBlock<char> data;

private:

	

	TableEntry* begin() const
	{
		return symbolTable.begin();
	}

	TableEntry* end() const
	{
		return symbolTable.end();
	}

	Array<TableEntry> symbolTable;
	
	int numUsed = 0;
	int allocatedSize = 0;
};



}
}