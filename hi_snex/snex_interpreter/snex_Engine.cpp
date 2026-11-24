#include "snex_Engine.h"
/* ===========================================================================
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

namespace snex
{
namespace interpreter
{

Engine::Engine(jit::GlobalScope& scope, const Statement::Ptr rootTree_):
  globalScope(scope),
  rootTree(rootTree_),
	rootScope(new Scope(Scope::ScopeType::Root, nullptr, this, {}))
{
	Statement::List initialisers;

	using namespace nodes;

	if (auto st = rootTree->getTyped<SyntaxTree>())
	{
		for (auto c : st->childStatements)
		{
			if (auto f = c->getTyped<Function>())
				rootFunctions.add(f);
			else if (auto tf = c->getTyped<TemplatedFunction>())
			{
				for (auto f : tf->childStatements)
				{
					if (auto df = f->getTyped<Function>())
						rootFunctions.add(df);
				}
			}
			else if (auto td = c->getTyped<TemplateDefinition>())
			{
				for (auto tdc : td->childStatements)
				{
					if (auto cs = tdc->getTyped<ClassStatement>())
						rootClassDefinitions.add(cs);
				}
			}
			else if (auto cs = c->getTyped<ClassStatement>())
				rootClassDefinitions.add(cs);
			else if (auto as = c->getTyped<Assignment>())
				rootVariableDefinitions.add(as);
			else if (auto cd = c->getTyped<ComplexTypeDefinition>())
				rootVariableDefinitions.add(cd);
		}
	}

	

	// PASS 1: allocate scopes
	DefinitionBase::allocateChildStatements(rootTree, rootScope);

	auto rootResult = rootScope->finalise();
	jassert(rootResult.wasOk());

	// PASS 2: resolve data pointers
	rootTree->resolveDataPointersRecursive(rootScope.get());

	// PASS 3: initialise root members
	for (auto i : rootVariableDefinitions)
		i->perform(rootScope.get(), nullptr);
}

Engine::~Engine()
{
}

Array<jit::NamespacedIdentifier> Engine::getFunctionIds() const
{
	Array<jit::NamespacedIdentifier> ids;

	for (const auto& f : rootFunctions)
	{
		ids.add(f->getTyped<nodes::Function>()->fData.id);
	}

	return ids;
}

jit::FunctionData Engine::resolveInterpretedFunctionCall(const jit::FunctionData& fd)
{
	// A specialized root template ID has a parent (eg. getArgs::9), but is still a root function.
	for (const auto& f : rootFunctions)
	{
		if (f->getTyped<nodes::Function>()->fData.matchIdArgsAndTemplate(fd))
			return f->getTyped<nodes::Function>()->fData;
	}

	if (!fd.id.isExplicit())
	{
		for (auto c : rootClassDefinitions)
		{
			if (auto f = c->getTyped<nodes::ClassStatement>()->resolveFunctionCall(fd))
				return *f;
		}
	}

	return {};
}

jit::FunctionData Engine::getFunction(const jit::NamespacedIdentifier& functionId)
{
	for (const auto& f : rootFunctions)
	{
		if (f->getTyped<nodes::Function>()->fData.id == functionId)
			return f->getTyped<nodes::Function>()->fData;
	}

	return {};
}



Array<jit::Symbol> Engine::getAllVariables() const
{
	return rootScope->getSymbols();
}

VariableStorage Engine::getVariable(const Identifier& id) const
{
	if (auto entry = rootScope->find(id))
	{
		if (auto data = rootScope->resolveDataPointer(rootScope->id.getChildId(id)))
		{
			
		}
	}

	return {};
}

size_t Engine::getMainObjectSize() const
{
	return rootScope->getDataSize();
}

void* Engine::getVariablePtr(const Identifier& id) const
{
	if (auto data = rootScope->resolveDataPointer(rootScope->id.getChildId(id)))
		return data.getDataPointer();

	return nullptr;
}

juce::String Engine::dumpTable()
{
	return juce::String();
}

ValueTree Engine::getDataLayout(int dataIndex) const
{
	jassertfalse;
	return ValueTree();
}




} // namespace interpreter
} // namespace snex
