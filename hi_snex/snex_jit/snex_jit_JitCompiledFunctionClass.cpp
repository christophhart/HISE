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


namespace snex {
namespace jit {
using namespace juce;


JitCompiledFunctionClass::JitCompiledFunctionClass(BaseScope* parentScope, const Symbol& classInstanceId)
{
	pimpl = new ClassScope(parentScope, classInstanceId);
}


JitCompiledFunctionClass::~JitCompiledFunctionClass()
{
	if(pimpl != nullptr)
		delete pimpl;
}


VariableStorage JitCompiledFunctionClass::getVariable(const Identifier& id)
{
	auto s = BaseScope::Symbol({ {}, id }, Types::ID::Dynamic);

	if (auto r = pimpl->rootData->contains(s))
	{
		return VariableStorage(pimpl->rootData->get(s));
	}

	jassertfalse;
	return {};
}


snex::VariableStorage* JitCompiledFunctionClass::getVariablePtr(const Identifier& id)
{
	auto s = BaseScope::Symbol({ {}, id }, Types::ID::Dynamic);

	if (pimpl->rootData->contains(s))
		return &pimpl->rootData->get(s);

	return nullptr;
}

juce::Array<juce::Identifier> JitCompiledFunctionClass::getFunctionIds() const
{
	return pimpl->getFunctionIds();
}

FunctionData JitCompiledFunctionClass::getFunction(const Identifier& functionId)
{
	auto s = pimpl->getClassName().getChildSymbol(functionId);

	if (pimpl->hasFunction(s))
	{
		Array<FunctionData> matches;
		pimpl->addMatchingFunctions(matches, s);

		// We don't allow overloaded functions for JIT compilation anyway...
		return matches.getFirst();
	}

	return {};
}

snex::VariableStorage* JitObject::getVariablePtr(const Identifier& id) const
{
	if (*this)
		return functionClass->getVariablePtr(id);

	return nullptr;
}

snex::jit::FunctionData JitObject::operator[](const Identifier& functionId) const
{
	if (*this)
	{
		return functionClass->getFunction(functionId);
	}
		

	return {};
}


juce::Array<juce::Identifier> JitObject::getFunctionIds() const
{
	if (*this)
	{
		return functionClass->getFunctionIds();
	}

	return {};
}

JitObject::operator bool() const
{
	return functionClass != nullptr;
}

void JitObject::rebuildDebugInformation()
{
	functionClass->pimpl->createDebugInfo(functionClass->debugInformation);
}

hise::DebugableObjectBase* JitObject::getDebugObject(const juce::String& token)
{
	for (auto& f : functionClass->debugInformation)
	{
		if (token == f->getCodeToInsert())
		{
			if (f->getType() == Types::ID::Block)
				return functionClass->pimpl->getSubFunctionClass(Symbol::createRootSymbol("Block"));
			if (f->getType() == Types::ID::Event)
				return functionClass->pimpl->getSubFunctionClass(Symbol::createRootSymbol("Message"));
		}
	}

	return ApiProviderBase::getDebugObject(token);
}

juce::ValueTree JitObject::createValueTree()
{
	auto c = dynamic_cast<GlobalScope*>(functionClass->pimpl->getParent())->getGlobalFunctionClass("Console");
	auto v = functionClass->pimpl->getApiValueTree();
	v.addChild(FunctionClass::createApiTree(c), -1, nullptr);
	return v;
}

void JitObject::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	return ApiHelpers::getColourAndLetterForType(type, colour, letter);
}

}
}