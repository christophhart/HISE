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

#if 0
	struct Reference : public ReferenceCountedObject,
					   public DebugableObjectBase
	{
	private:

		friend class BaseScope;

		Reference();

		Reference(BaseScope* p, const Identifier& s, Types::ID type);;

		// =================================================================== Debug methods

		Identifier getObjectName() const override { return "ClassVariable"; };

		Identifier getInstanceName() const override 
		{ 
			return id.toString();
		}

		int getTypeNumber() const override
		{
			return (int)getType();
		}

		juce::String getCategory() const override { return "Global variable"; };

		juce::String getDebugDataType() const override
		{
			return Types::Helpers::getTypeName(getType());
		}

		juce::String getDebugValue() const override
		{
			return Types::Helpers::getCppValueString(getDataCopy());
		}

		// =================================================================================

	public:

		int getNumReferences() const;;

		bool operator==(const Reference& other) const;

		WeakReference<BaseScope> scope;
		Symbol id;
		bool isConst = false;

		void* getDataPointer() const;

		VariableStorage getDataCopy() const;

		VariableStorage& getDataReference(bool allowConstInitialisation = false) const;

		Types::ID getType() const;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Reference);
	};


	using RefPtr = ReferenceCountedObjectPtr<Reference>;

	RefPtr get(const Symbol& s);


	snex::jit::BaseScope::RefPtr get(const Symbol& s)
	{
		for (auto er : referencedVariables)
		{
			if (er->id == s)
				return er;
		}

		for (auto c : constants)
		{
			if (c.id == s)
			{
				RefPtr r = new Reference(this, s.id, c.id.type);
				r->isConst = true;
				referencedVariables.add(r);
				return r;
			}
		}

		auto& v = getRootClassScope()->rootData->get(s);

		RefPtr r = new Reference(this, s.id, v.getType());
		referencedVariables.add(r);
		return r;

		return nullptr;
	}
#endif

	ScopeType getScopeType() const;;

	GlobalScope* getGlobalScope();

	ClassScope* getRootClassScope() const;

	Symbol getScopeSymbol() const { return scopeId; }

	bool addConstant(const Identifier& id, VariableStorage v);

	BaseScope* getParent();

	RootClassData* getRootData() const;

public:

	BaseScope(const Symbol& s, BaseScope* parent_ = nullptr, int numVariables = 1024);;

	virtual ~BaseScope();;

	Result allocate(const Identifier& id, VariableStorage v);

	BaseScope* getScopeForSymbol(const Symbol& s);

	virtual bool hasVariable(const Identifier& id) const;

	Array<Symbol> getAllVariables() const;

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

	bool isClassObjectScope() const
	{
		return scopeType == Class && scopeId;
	}

	int getNumChildScopes() const
	{
		return childScopes.size();
	}

protected:

	ObjectTypeRegister classTypes;

	Symbol scopeId;
	WeakReference<BaseScope> parent;
	VariableStorage empty;

	Array<ChildClass> childClasses;

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


struct RootClassData
{
	RootClassData()
	{
		data.allocate(1024, true);
	}
	
	struct TableEntry
	{
		Symbol s;
		VariableStorage* data;
		WeakReference<BaseScope> scope;
	};

	TableEntry* begin() const
	{
		return symbolTable.begin();
	}

	TableEntry* end() const
	{
		return symbolTable.end();
	}

	Result allocate(BaseScope* scope, const Symbol& s)
	{
		jassert(scope->getScopeType() == BaseScope::Class);

		if (contains(s))
			return Result::fail(s.toString() + "already exists");

		TableEntry newEntry;
		newEntry.s = s;
		newEntry.data = data + numUsed++;
		newEntry.scope = scope;

		symbolTable.add(newEntry);
		return Result::ok();
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
				return ts.data->getType();
		}

		return Types::ID::Dynamic;
	}

	VariableStorage& get(const Symbol& s) const
	{
		for (const auto& ts : symbolTable)
			if (ts.s == s)
				return *ts.data;

		jassertfalse;
		return empty;
	}

private:

	Array<TableEntry> symbolTable;
	mutable VariableStorage empty;
	HeapBlock<VariableStorage> data;
	int numUsed = 0;
	int allocatedSize;
};



}
}