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



struct NamespaceHandler
{
	enum SymbolType
	{
		Unknown,
		Struct,
		Function,
		Variable,
		UsingAlias,
		Constant,
		StaticFunctionClass,
		numSymbolTypes
	};

private:

	struct Alias
	{
		NamespacedIdentifier id;
		TypeInfo type;
		SymbolType symbolType;
		VariableStorage constantValue;
		
		juce::String toString() const;
	};

	struct Namespace : public ReferenceCountedObject
	{
		using WeakPtr = WeakReference<Namespace>;
		using Ptr = ReferenceCountedObjectPtr<Namespace>;
		using List = ReferenceCountedArray<Namespace>;
		using WeakList = Array<WeakReference<Namespace>>;

		bool contains(const NamespacedIdentifier& symbol) const;

		juce::String dump(int level);

		static juce::String getIntendLevel(int level);

		void addSymbol(const NamespacedIdentifier& aliasId, const TypeInfo& type, SymbolType symbolType);

		NamespacedIdentifier id;
		Array<Alias> aliases;
		ReferenceCountedArray<Namespace> usedNamespaces;
		ReferenceCountedArray<Namespace> childNamespaces;
		WeakPtr parent;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Namespace);
	};

public:

	struct ScopedNamespaceSetter
	{
		ScopedNamespaceSetter(NamespaceHandler& h, const NamespacedIdentifier& id):
			handler(h),
			prevNamespace(handler.getCurrentNamespaceIdentifier())
		{
			h.pushNamespace(id);
		}

		ScopedNamespaceSetter(NamespaceHandler& h, const Identifier& id) :
			handler(h),
			prevNamespace(handler.getCurrentNamespaceIdentifier())
		{
			h.pushNamespace(prevNamespace.getChildId(id));
		}

		~ScopedNamespaceSetter()
		{
			handler.pushNamespace(prevNamespace);
		}

		NamespaceHandler& handler;
		NamespacedIdentifier prevNamespace;
	};

	NamespaceHandler() = default;

	ComplexType::Ptr registerComplexTypeOrReturnExisting(ComplexType::Ptr ptr);
	ComplexType::Ptr getComplexType(NamespacedIdentifier id);
	VariadicSubType::Ptr getVariadicTypeForId(NamespacedIdentifier id) const;
	bool isTemplatedMethod(NamespacedIdentifier functionId) const;

	bool changeSymbolType(NamespacedIdentifier id, SymbolType newType);

	NamespacedIdentifier getRootId() const;
	bool isNamespace(const NamespacedIdentifier& possibleNamespace) const;
	
	NamespacedIdentifier getCurrentNamespaceIdentifier() const;
	juce::String dump();
	Result addUsedNamespace(const NamespacedIdentifier& usedNamespace);
	Result resolve(NamespacedIdentifier& id, bool allowZeroMatch = false) const;
	void addSymbol(const NamespacedIdentifier& id, const TypeInfo& t, SymbolType symbolType);

	Result addConstant(const NamespacedIdentifier& id, const VariableStorage& v);
	
	Result setTypeInfo(const NamespacedIdentifier& id, SymbolType expectedType, const TypeInfo& t);

	void addVariadicType(VariadicSubType::Ptr p)
	{
		variadicTypes.add(p);
	}

	bool rootHasNamespace(const NamespacedIdentifier& id) const;

	SymbolType getSymbolType(const NamespacedIdentifier& id) const;

	TypeInfo getAliasType(const NamespacedIdentifier& aliasId) const;

	TypeInfo getVariableType(const NamespacedIdentifier& variableId) const;

	VariableStorage getConstantValue(const NamespacedIdentifier& variableId) const;

	bool isStaticFunctionClass(const NamespacedIdentifier& classId) const;

	Result switchToExistingNamespace(const NamespacedIdentifier& id);

private:

	void pushNamespace(const Identifier& childId);
	void pushNamespace(const NamespacedIdentifier& id);
	Result popNamespace();

	TypeInfo getTypeInfo(const NamespacedIdentifier& aliasId, const Array<SymbolType>& t) const;

	ReferenceCountedArray<ComplexType> complexTypes;
	ReferenceCountedArray<VariadicSubType> variadicTypes;

	Namespace::WeakPtr getRoot() const;
	Namespace::Ptr get(const NamespacedIdentifier& id) const;
	Namespace::List existingNamespace;
	Namespace::WeakPtr currentNamespace;
	Namespace::WeakPtr currentParent;
};


}
}