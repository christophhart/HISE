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

Engine::Engine(jit::GlobalScope& scope, const Statement::Collection& createdNodes):
  globalScope(scope),
  rootTree(createdNodes.root),
	rootScope(new Scope(nullptr, this, {}))
{
	Statement::List initialisers;

	using namespace nodes;

	if (auto st = rootTree->getTyped<SyntaxTree>())
	{
		for (auto c : st->children)
		{
			if (auto as = c->getTyped<Assignment>())
			{
				if (auto v = as->target->getTyped<VariableReference>())
				{
					if (v->symbol.id.getParent() == rootScope->id &&
						rootScope->find(v->symbol.id.id) == nullptr)
					{
						auto& type = v->symbol.typeInfo;
						auto result = rootScope->addSymbol(v->symbol, type.getRequiredByteSize(),
							type.getRequiredAlignment());
						jassert(result.wasOk());
					}
				}

				initialisers.add(as);
			}

			if (auto f = c->getTyped<Function>())
			{
				auto d = new DispatchData();

				d->functionData = f->fData;
				d->functionScope = new Scope(rootScope.get(), this, f->fData.id);

				for (const auto& a : f->fData.args)
				{
					auto result = d->functionScope->addSymbol(a, a.typeInfo.getRequiredByteSize(),
						a.typeInfo.getRequiredAlignment());
					jassert(result.wasOk());
				}

				auto result = d->functionScope->finalise();
				bool validParameters = result.wasOk();
				jassert(validParameters);

				for (const auto& a : f->fData.args)
				{
					auto ptr = d->functionScope->getAddress(a.id.id, a.typeInfo.getRequiredByteSize());
					validParameters &= ptr != nullptr;
					d->parameterOffsets.push_back(ptr);
				}

				d->functionData.object = d;
				d->functionData.function = validParameters ? DispatchData::getFromFunctionMap(d->getSigString()) : nullptr;
				jassert(d->functionData.function != nullptr);
				d->functionStatement = f;
				dispatchData.add(d);
			}
		}
	}

	auto rootResult = rootScope->finalise();
	jassert(rootResult.wasOk());

	createdNodes.template callRecursive<VariableReference>(rootTree->astNode, [&](VariableReference* v)
	{
		auto scopeId = v->symbol.id.getParent();
		auto idWithinScope = v->symbol.id.id;

		if (auto s = rootScope->getScope(scopeId))
		{
			if (auto entry = s->find(idWithinScope))
			{
				if (s->getAddress(idWithinScope, v->symbol.typeInfo.getRequiredByteSize()) != nullptr)
				{
					v->resolvedScope = s;
					v->resolvedOffset = entry->offset;
					v->symbol.resolved = true;
				}
			}
		}

		return false;
	});

	for (auto i : initialisers)
	{
		i->perform(rootScope.get(), nullptr);
	}
}

Engine::~Engine()
{
}

Array<jit::NamespacedIdentifier> Engine::getFunctionIds() const
{
	Array<jit::NamespacedIdentifier> ids;

	for (const auto& f : dispatchData)
	{
		ids.add(f->functionData.id);
	}

	return ids;
}

jit::FunctionData Engine::getFunction(const jit::NamespacedIdentifier& functionId)
{
	for (const auto& f : dispatchData)
	{
		if (f->functionData.id == functionId)
			return f->functionData;
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
		if (auto data = rootScope->getAddress(id, entry->size))
		{
			switch (entry->symbol.typeInfo.getType())
			{
			case Types::ID::Integer: return VariableStorage(*static_cast<int*>(data));
			case Types::ID::Float:   return VariableStorage(*static_cast<float*>(data));
			case Types::ID::Double:  return VariableStorage(*static_cast<double*>(data));
			default: break;
			}
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
	if (auto entry = rootScope->find(id))
		return rootScope->getAddress(id, entry->size);

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



String Engine::DispatchData::getSigString() const
{
	String sig;

	auto getChar = [](const TypeInfo& t)
		{
			switch (t.getType())
			{
			case Types::ID::Void:    return 'v';
			case Types::ID::Integer: return 'i';
			case Types::ID::Float:   return 'f';
			case Types::ID::Double:  return 'd';
			case Types::ID::Pointer: return 'p';
			default:                 return '?';
			}
		};

	sig << getChar(functionData.returnType);

	for (const auto& a : functionData.args)
		sig << getChar(a.typeInfo);

	return sig;
}

void* Engine::DispatchData::getFromFunctionMap(const String& sig)
{
	static const std::map<String, void*> functionMap = []
	{
		std::map<String, void*> functions;
		addReturnSignatures<void>(functions, "v");
		addReturnSignatures<int>(functions, "i");
		addReturnSignatures<float>(functions, "f");
		addReturnSignatures<double>(functions, "d");
		addReturnSignatures<void*>(functions, "p");
		return functions;
	}();

	auto match = functionMap.find(sig);
	return match != functionMap.end() ? match->second : nullptr;
}

} // namespace interpreter
} // namespace snex
