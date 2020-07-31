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


JitCompiledFunctionClass::JitCompiledFunctionClass(BaseScope* parentScope, const NamespacedIdentifier& classInstanceId)
{
	pimpl = new ClassScope(parentScope, classInstanceId, nullptr);
}


JitCompiledFunctionClass::~JitCompiledFunctionClass()
{
	if(pimpl != nullptr)
		delete pimpl;
}


VariableStorage JitCompiledFunctionClass::getVariable(const Identifier& id)
{
	auto s = pimpl->rootData->getClassName().getChildId(id);

	if (auto r = pimpl->rootData->contains(s))
	{
		return pimpl->rootData->getDataCopy(s);
	}

	jassertfalse;
	return {};
}


void* JitCompiledFunctionClass::getVariablePtr(const Identifier& id)
{
	auto s = pimpl->rootData->getClassName().getChildId(id);

	if (pimpl->rootData->contains(s))
		return pimpl->rootData->getDataPointer(s);

	return nullptr;
}

juce::String JitCompiledFunctionClass::dumpTable()
{
	return pimpl->getRootData()->dumpTable();
}

Array<NamespacedIdentifier> JitCompiledFunctionClass::getFunctionIds() const
{
	return pimpl->getRootData()->getFunctionIds();
}

FunctionData JitCompiledFunctionClass::getFunction(const NamespacedIdentifier& functionId)
{
	auto s = pimpl->getRootData()->getClassName().getChildId(functionId.getIdentifier());

	if (pimpl->getRootData()->hasFunction(s))
	{
		Array<FunctionData> matches;
		pimpl->getRootData()->addMatchingFunctions(matches, s);

		// We don't allow overloaded functions for JIT compilation anyway...
		return matches.getFirst();
	}

	return {};
}



snex::jit::FunctionData JitObject::operator[](const NamespacedIdentifier& functionId) const
{
	if (*this)
	{
		return functionClass->getFunction(functionId);
	}
		

	return {};
}


FunctionData JitObject::operator[](const Identifier& functionId) const
{
	return operator[](NamespacedIdentifier(functionId));
}

Array<NamespacedIdentifier> JitObject::getFunctionIds() const
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
#if 0
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
#endif

	return ApiProviderBase::getDebugObject(token);
}

juce::ValueTree JitObject::createValueTree()
{
	auto c = dynamic_cast<GlobalScope*>(functionClass->pimpl->getParent())->getGlobalFunctionClass(NamespacedIdentifier("Console"));
	auto v = functionClass->pimpl->getRootData()->getApiValueTree();
	v.addChild(FunctionClass::createApiTree(c), -1, nullptr);
	return v;
}

void JitObject::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	return ApiHelpers::getColourAndLetterForType(type, colour, letter);
}

snex::jit::FunctionData JitCompiledClassBase::getFunction(const NamespacedIdentifier& id)
{
	Array<FunctionData> matches;

	auto typePtr = dynamic_cast<StructType*>(classType.get());
	auto sId = typePtr->id.getChildId(id.getIdentifier());

	memberFunctions->addMatchingFunctions(matches, sId);

	if (matches.size() == 1)
		return matches[0];

	return {};
}

}
}