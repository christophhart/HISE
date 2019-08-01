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

namespace hnode {
namespace jit {
using namespace juce;

/** This class holds all data that is used by either the compiler or the JIT itself.

	The lifetime of this object depends on the ScopeType property - local scopes are
	supposed to be destroyed after the compilation, since they are just used to resolve
	local variable values, but the Global and ClassScopes have a longer lifetime that
	is at least as long as the JITted functions that use it.
*/
class BaseScope
{
public:

	/** A Symbol is used to identifiy the data slot. */
	struct Symbol
	{
		Symbol();
		Symbol(const Identifier& parent_, const Identifier& id_, Types::ID t_);

		bool operator==(const Symbol& other) const;

		bool matchesIdAndType(const Symbol& other) const;

		String toString() const;

		operator bool() const;

		Identifier parent;
		Identifier id;
		Types::ID type = Types::ID::Dynamic;
	};

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

	struct Reference : public ReferenceCountedObject
	{
	private:

		friend class BaseScope;

		Reference();

		Reference(BaseScope* p, const Identifier& s, Types::ID type);;

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

	ScopeType getScopeType() const;;

	RefPtr get(const Symbol& s);

	bool addConstant(const Identifier& id, VariableStorage v);

	BaseScope* getParent();

public:

	BaseScope(const Identifier& id, BaseScope* parent_ = nullptr, int numVariables = 1024);;

	virtual ~BaseScope();;

	Result allocate(const Identifier& id, VariableStorage v);

	BaseScope* getScopeForSymbol(const Symbol& s);


	Result deallocate(Reference v);

	Array<Symbol> getAllVariables() const;

protected:

	VariableStorage & getVariableReference(const Identifier& id);

	/** Used for resolving the outside reference. */
	struct InternalReference
	{
		bool operator==(const InternalReference& other) const;

		bool operator==(const Symbol& s) const;

		InternalReference(const Identifier& id_, VariableStorage& r);;

		Identifier id;
		VariableStorage& ref;
	};

	Identifier scopeId;
	WeakReference<BaseScope> parent;
	VariableStorage empty;

	int size = 0;
	HeapBlock<VariableStorage> data;

	ScopeType scopeType;
	Array<Constant> constants;

	ReferenceCountedArray<Reference> referencedVariables;

	Array<InternalReference> allocatedVariables;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseScope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(BaseScope);
};


}
}