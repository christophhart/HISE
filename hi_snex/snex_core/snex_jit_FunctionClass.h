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


class BaseScope;
class NamespaceHandler;

struct InlineData;



/** A function class is a collection of functions. */
class FunctionClass : public DebugableObjectBase,
	public ReferenceCountedObject
{
public:

	using Map = Array<StaticFunctionPointer>;

	// Sort the matches so that resolved functions come first
			// This avoids templated functions without inliner to be picked over their
			// actual functions with proper inlining.

	struct ResolveSorter
	{
		static int compareElements(const FunctionData& f1, const FunctionData& f2)
		{
			bool firstResolvedOrNoT = f1.isResolved() || !f1.hasTemplatedArgumentOrReturnType();
			bool seconResolvedOrNoT = f2.isResolved() || !f2.hasTemplatedArgumentOrReturnType();

			if (firstResolvedOrNoT && !seconResolvedOrNoT)
				return -1;

			if (seconResolvedOrNoT && !firstResolvedOrNoT)
				return 1;

			return 0;
		}
	};

	using Ptr = ReferenceCountedObjectPtr<FunctionClass>;

	enum SpecialSymbols
	{
		AssignOverload = 0,
		IncOverload,
		DecOverload,
		PostIncOverload,
		PostDecOverload,
		BeginIterator,
		SizeFunction,
		NativeTypeCast,
		Subscript,
		ToSimdOp,
		Constructor,
		Destructor,
		GetFrom,
		numOperatorOverloads
	};

	struct Constant
	{
		Identifier id;
		VariableStorage value;
	};

	FunctionClass(const NamespacedIdentifier& id);;

	virtual ~FunctionClass();;


	static Identifier getSpecialSymbol(const NamespacedIdentifier& classId, SpecialSymbols s);

	bool hasSpecialFunction(SpecialSymbols s) const;
	void addSpecialFunctions(SpecialSymbols s, Array<FunctionData>& possibleMatches) const;

	FunctionData getConstructor(const Array<TypeInfo>& args);
	FunctionData getConstructor(InitialiserList::Ptr initList);
	static bool isConstructor(const NamespacedIdentifier& id);

	static bool isDestructor(const NamespacedIdentifier& id)
	{
		return id.getIdentifier().toString()[0] == '~';
	}

	FunctionData getSpecialFunction(SpecialSymbols s, TypeInfo returnType = {}, const TypeInfo::List& args = {}) const;
	FunctionData getNonOverloadedFunctionRaw(NamespacedIdentifier id) const;
	FunctionData getNonOverloadedFunction(NamespacedIdentifier id) const;

	// =========================================================== DebugableObject overloads

	static ValueTree createApiTree(FunctionClass* r);
	ValueTree getApiValueTree() const;
	FunctionData& createSpecialFunction(SpecialSymbols s);
	juce::String getCategory() const override { return "API call"; };
	Identifier getObjectName() const override { return classSymbol.toString(); }
	juce::String getDebugValue() const override { return classSymbol.toString(); };
	juce::String getDebugDataType() const override { return "Class"; };
	void getAllFunctionNames(Array<NamespacedIdentifier>& functions) const;;
	void setDescription(const juce::String& s, const StringArray& names);
	virtual void getAllConstants(Array<Identifier>& ids) const;;
	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override;
	const var getConstantValue(int index) const;;
	VariableStorage getConstantValue(const NamespacedIdentifier& s) const;

	// =====================================================================================

	virtual bool hasFunction(const NamespacedIdentifier& s) const;
	bool hasConstant(const NamespacedIdentifier& s) const;
	void addFunctionConstant(const Identifier& constantId, VariableStorage value);
	virtual void addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const;
	void addFunctionClass(FunctionClass* newRegisteredClass);
	void removeFunctionClass(const NamespacedIdentifier& id);
	void addFunction(FunctionData* newData);

	Array<NamespacedIdentifier> getFunctionIds() const;
	const NamespacedIdentifier& getClassName() const { return classSymbol; }

	bool fillJitFunctionPointer(FunctionData& dataWithoutPointer);
	bool injectFunctionPointer(FunctionData& dataToInject);

	FunctionClass* getSubFunctionClass(const NamespacedIdentifier& id);
	bool isInlineable(const NamespacedIdentifier& id) const;
	Inliner::Ptr getInliner(const NamespacedIdentifier& id) const;
	void addInliner(const Identifier& id, const Inliner::Func& func, Inliner::InlineType type=Inliner::Assembly);

	virtual Map getMap()
	{
		Map m;

		for (auto r : registeredClasses)
			m.addArray(r->getMap());

		for (auto f : functions)
		{
			if (f->function != nullptr)
			{
				StaticFunctionPointer item;
				item.signature = f->getSignature({}, false);

				auto l = f->id.toString().replace("::", "_");

                l = l.replace("~", "_dest");
                
				l << "_";

				l << Types::Helpers::getCppTypeName(f->returnType.getType())[0];

				for (const auto& a : f->args)
					l << Types::Helpers::getCppTypeName(a.typeInfo.getType())[0];

				item.label = l;
				item.function = f->function;

				m.add(item);
			}
		}

		return m;
	}

protected:

	ReferenceCountedArray<FunctionClass> registeredClasses;
	NamespacedIdentifier classSymbol;
	OwnedArray<FunctionData> functions;

	Array<Constant> constants;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionClass);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionClass)
};



}
}
