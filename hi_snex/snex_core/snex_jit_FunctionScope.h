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
USE_ASMJIT_NAMESPACE;



struct RegisterScope : public BaseScope
{
	RegisterScope(BaseScope* parentScope, const NamespacedIdentifier& id) :
		BaseScope({}, parentScope)
	{
		jassert(id.isValid());

		if (!(parentScope->getScopeSymbol() == id.getParent()))
			scopeId = parentScope->getScopeSymbol().getChildId(id.getIdentifier());
		else
			scopeId = id;

		//jassert();
		jassert(getScopeType() >= BaseScope::Function);
	}

	Array<Symbol> localVariables;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegisterScope);
};


class FunctionScope : public RegisterScope
{
public:
	FunctionScope(BaseScope* parent, const NamespacedIdentifier& functionName, bool allowDifferentParentId=false) :
		RegisterScope(parent, functionName)
	{
		if (allowDifferentParentId)
			scopeId = functionName;

		scopeType = BaseScope::Function;
	}

	~FunctionScope() {}

	AssemblyRegister* getRegister(const Symbol& ref)
	{
		for (auto r : allocatedRegisters)
		{
			if (r->getVariableId() == ref)
				return r;
		}

		return nullptr;
	}

	ComplexType::Ptr getClassType()
	{
		if (hardcodedClassType != nullptr)
			return hardcodedClassType.get();

		if (auto cs = getParentScopeOfType<ClassScope>())
			return cs->typePtr.get();

		return nullptr;
	}

	void setHardcodedClassType(ComplexType::Ptr cs)
	{
		hardcodedClassType = cs.get();
	}

	void addAssemblyRegister(AssemblyRegister* newRegister)
	{
		allocatedRegisters.add(newRegister);
	}

	ReferenceCountedArray<AssemblyRegister> allocatedRegisters;

	ReferenceCountedObject* parentFunction = nullptr;
	FunctionData data;
	Array<Identifier> parameters;

private:

	WeakReference<ComplexType> hardcodedClassType;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionScope);
};



}
}
