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


namespace hnode {
namespace jit {
using namespace juce;


JitCompiledFunctionClass::JitCompiledFunctionClass(GlobalScope& memory)
{
	pimpl = new ClassScope(memory);
}


JitCompiledFunctionClass::~JitCompiledFunctionClass()
{
	delete pimpl;
}


VariableStorage JitCompiledFunctionClass::getVariable(const Identifier& id)
{
	auto s = BaseScope::Symbol({}, id, Types::ID::Dynamic);

	if (auto r = pimpl->get(s))
	{
		return r->getDataCopy();
	}
}


hnode::VariableStorage* JitCompiledFunctionClass::getVariablePtr(const Identifier& id)
{
	auto s = BaseScope::Symbol({}, id, Types::ID::Dynamic);

	if (auto r = pimpl->get(s))
		return &r->getDataReference(false);

	return nullptr;
}

FunctionData JitCompiledFunctionClass::getFunction(const Identifier& functionId)
{
	if (pimpl->hasFunction({}, functionId))
	{
		Array<FunctionData> matches;
		pimpl->addMatchingFunctions(matches, {}, functionId);

		// We don't allow overloaded functions for JIT compilation anyway...
		return matches.getFirst();
	}

	return {};
}

hnode::VariableStorage* JitObject::getVariablePtr(const Identifier& id) const
{
	if (*this)
		return functionClass->getVariablePtr(id);

	return nullptr;
}

hnode::jit::FunctionData JitObject::operator[](const Identifier& functionId) const
{
	if (*this)
	{
		return functionClass->getFunction(functionId);
	}
		

	return {};
}


JitObject::operator bool() const
{
	return functionClass != nullptr;
}

}
}