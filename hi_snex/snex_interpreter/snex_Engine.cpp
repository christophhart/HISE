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
			if (auto tf = c->getTyped<TemplatedFunction>())
			{
				for (auto f : tf->childStatements)
				{
					if (auto df = createInterpretedFunction(f, rootScope.get(), false))
						rootFunctions.add(df);
				}
			}
			else if (auto df = createInterpretedFunction(c, rootScope.get(), false))
				rootFunctions.add(df);
			else if (auto cs = createInterpretedClass(c, rootScope.get()))
				classes.add(cs);
			else if (auto as = createScopeInitialiser(c, rootScope.get()))
				initialisers.add(dynamic_cast<Statement*>(as));
		}
	}

	auto rootResult = rootScope->finalise();
	jassert(rootResult.wasOk());

	rootTree->callRecursive<Statement>([&](Statement* s)
	{
		s->resolveDataPointers(rootScope.get());
		return false;
	});

	for (auto i : initialisers)
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
		ids.add(f->functionData.id);
	}

	return ids;
}

jit::FunctionData Engine::getFunction(const jit::NamespacedIdentifier& functionId)
{
	if (functionId.isExplicit())
	{
		for (const auto& f : rootFunctions)
		{
			if (f->functionData.id == functionId)
				return f->functionData;
		}

		return {};
	}
	else
	{
		for (auto c : classes)
		{
			auto fd = c->getFunction(functionId);

			if (fd.id != NamespacedIdentifier())
				return fd;
		}

		return {};
	}
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
	return juce::String();
}

ValueTree Engine::getDataLayout(int dataIndex) const
{
	jassertfalse;
	return ValueTree();
}

Engine::InterpretedFunction* Engine::createInterpretedFunction(Statement::Ptr s, Scope* scopeToUse, bool addThisPointer)
{
	if (auto f = s->getTyped<nodes::Function>())
	{
		auto d = new InterpretedFunction();

		d->addThisPointer = addThisPointer;
		d->functionData = f->fData;
		d->functionScope = new Scope(Scope::ScopeType::Function, scopeToUse, this, f->fData.id);


		if (addThisPointer)
		{
			// Keep this first in the function scope: ThisPointer reads its object pointer from offset zero.
			jit::Symbol thisObj({ d->functionData.id.getChildId("this"), TypeInfo(Types::ID::Pointer) });
			d->functionScope->addSymbol(thisObj, thisObj.typeInfo.getRequiredByteSize(), thisObj.typeInfo.getRequiredAlignment());
		}

		for (const auto& a : f->fData.args)
		{
			auto result = d->functionScope->addSymbol(a, a.typeInfo.getRequiredByteSize(),
				a.typeInfo.getRequiredAlignment());
			jassert(result.wasOk());
		}

		DefinitionBase::allocateChildStatements(f, d->functionScope);

		auto result = d->functionScope->finalise();
		bool validParameters = result.wasOk();
		jassert(validParameters);

		if (addThisPointer)
			d->parameterOffsets.push_back(d->functionScope->getDataPointer());
		
		for (const auto& a : f->fData.args)
		{
			auto ptr = d->functionScope->getAddress(a.id.id, a.typeInfo.getRequiredByteSize());
			validParameters &= ptr != nullptr;
			d->parameterOffsets.push_back(ptr);
		}

		d->functionData.object = d;
		d->functionData.function = validParameters ? InterpretedFunction::getFromFunctionMap(d->getSigString()) : nullptr;

		if (d->functionData.function == nullptr)
		{
			d->functionData.function = (void*)InterpretedFunction::callDynamic;
		}

		if (d->functionData.returnType.toString() == "block&")
			d->convertToBlock = true;

		jassert(d->functionData.function != nullptr);
		d->functionStatement = f;
		return d;
	}

	return nullptr;
}

Engine::InterpretedClass* Engine::createInterpretedClass(Statement::Ptr c, Scope* s)
{
	if (auto cs = c->getTyped<nodes::ClassStatement>())
	{
		InterpretedClass* newClass = new InterpretedClass();
		newClass->classScope = new Scope(Scope::ScopeType::Class, s, this, cs->classType->id);

		if (auto st = cs->childStatements[0]->getTyped<nodes::SyntaxTree>())
		{
			for (auto c : st->childStatements)
			{
				if (auto df = createInterpretedFunction(c, newClass->classScope, true))
					newClass->members.add(df);
				else if (auto cs = createInterpretedClass(c, newClass->classScope))
					newClass->subclasses.add(cs);
				else if (auto as = createScopeInitialiser(c, newClass->classScope))
					newClass->initialisers.add(c);
			}
		}

		newClass->classScope->finalise();

		for (auto p : newClass->initialisers)
			p->perform(newClass->classScope, nullptr);

		return newClass;
	}

	return nullptr;
}

DefinitionBase* Engine::createScopeInitialiser(Statement::Ptr c, Scope* s)
{
	auto ok = DefinitionBase::allocateChildStatements(c, s);
	jassert(ok.wasOk());

	return c->getTyped<DefinitionBase>();
}

String Engine::InterpretedFunction::getSigString() const
{
	return Statement::OpFunctions::getSigString(functionData, addThisPointer);
}

void* Engine::InterpretedFunction::getFromFunctionMap(const String& sig)
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

void Engine::InterpretedFunction::writeWithTypeInfo(void* ptr, const TypeInfo& argType, const VariableStorage& argValue)
{
	if (Statement::OpFunctions::Data::isBlockType(argType))
		ComplexType::writeNativeMemberType(ptr, 0, Statement::OpFunctions::Data::toBlock(argValue));
	else
		ComplexType::writeNativeMemberType(ptr, 0, argValue);
}

VariableStorage Engine::InterpretedFunction::callDynamic(void* obj, VariableStorage* args, int numArgs)
{
	auto data = static_cast<InterpretedFunction*>(obj);

	try
	{
		jassert(numArgs == data->parameterOffsets.size());

		ScopedValueSetter<bool> svs(data->active, true);

		data->functionScope->clear();

		for (int i = 0; i < numArgs; i++)
			jit::ComplexType::writeNativeMemberType(data->parameterOffsets[i], 0, args[i]);

		VariableStorage rv;
		data->functionStatement->perform(data->functionScope.get(), &rv);

		return rv;
	}
	catch (UriError& e)
	{
		CallError ne;
		ne.callstackId = data->functionData.getSignature();
		ne.e = e;
		throw ne;
	}
}

} // namespace interpreter
} // namespace snex
