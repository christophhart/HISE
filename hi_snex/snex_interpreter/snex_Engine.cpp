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

Engine::Engine(jit::GlobalScope& scope, const StatementCollection& createdNodes):
  globalScope(scope),
  rootTree(createdNodes.root),
  rootScope(new Scope(nullptr, this, getMainId()))
{
	if (auto st = dynamic_cast<SyntaxTree*>(rootTree.get()))
	{
		for (auto c : st->children)
		{
			if (auto as = dynamic_cast<Assignment*>(c))
			{
				if (auto v = dynamic_cast<VariableReference*>(as->target.get()))
				{
					
				}
			}

			if (auto f = dynamic_cast<Function*>(c))
			{
				f->fData.object = this;
				f->fData.function = (void*)Engine::dispatch1<0, int, int>;

				DispatchData d;
				d.functionData = f->fData;
				d.functionScope = new Scope(rootScope, this, f->fData.id);

				size_t parameterByteSize = 0;

				for (const auto& a : f->fData.args)
				{
					auto alignment = a.typeInfo.getRequiredAlignment();
					if (alignment != 0)
						parameterByteSize += (alignment - (parameterByteSize % alignment)) % alignment;
					d.functionScope->offsets[a.id.id] = parameterByteSize;
					d.parameterOffsets.push_back(parameterByteSize);
					parameterByteSize += a.typeInfo.getRequiredByteSize();
				}
				
				d.functionScope->scopeData.calloc(parameterByteSize);
				d.functionStatement = f;
				dispatchData.push_back(d);
			}
		}
	}

	createdNodes.template callRecursive<VariableReference>(rootTree->astNode, [&](VariableReference* v)
	{
		auto scopeId = v->symbol.id.getParent();
		auto idWithinScope = v->symbol.id.id;

		if (auto s = rootScope->getScope(scopeId))
		{
			v->resolvedScope = s;
			v->resolvedOffset = s->getOffset(idWithinScope);
			v->symbol.resolved = true;
		}

		return false;
	});
}

Engine::~Engine()
{
}

jit::FunctionData Engine::getFunction(const jit::NamespacedIdentifier& functionId)
{
	for (const auto& f : dispatchData)
	{
		if (f.functionData.id == functionId)
			return f.functionData;
	}

	return {};
}

Array<jit::Symbol> Engine::getAllVariables() const
{
	
	return Array<jit::Symbol>();
}

VariableStorage Engine::getVariable(const Identifier& id) const
{
	
	return VariableStorage();
}

size_t Engine::getMainObjectSize() const
{
	jassertfalse;
	return size_t();
}

void* Engine::getVariablePtr(const Identifier& id) const
{
	jassertfalse;
	return nullptr;
}

juce::String Engine::dumpTable()
{
	jassertfalse;
	return juce::String();
}

ValueTree Engine::getDataLayout(int dataIndex) const
{
	jassertfalse;
	return ValueTree();
}

Engine::Statement::Ptr Engine::StatementFactory::create(const ValueTree& v)
{
	if (createFunctions.find(v.getType()) != createFunctions.end())
	{
		if (auto t = createFunctions.at(v.getType())(*this, v))
		{
			createdNodes.flatList.push_back({ v, t });

			return t;
		}
	}

	return nullptr;
}

} // namespace interpreter
} // namespace snex
