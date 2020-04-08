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
using namespace asmjit;

bool NamespaceHandler::Namespace::contains(const NamespacedIdentifier& symbol) const
{
	if (symbol.getParent() == id)
	{
		for (auto a : aliases)
		{
			if (symbol == a.id)
				return true;
		}
	}

	return false;
}

juce::String NamespaceHandler::Namespace::dump(int level)
{
	juce::String s;

	auto idName = id.isValid() ? id.toString() : "root";

	s << getIntendLevel(level) << "namespace " << idName << "\n";

	level++;

	for (auto u : usedNamespaces)
	{
		s << getIntendLevel(level) << "using " << u->id.toString() << "\n";
	}

	for (auto a : aliases)
	{
		s << getIntendLevel(level);
		s << a.toString() << "\n";
	}

	for (auto c : childNamespaces)
	{
		s << c->dump(level);
	};

	return s;
}

juce::String NamespaceHandler::Namespace::getIntendLevel(int level)
{
	juce::String s;

	for (int i = 0; i < level; i++)
		s << "  ";

	return s;
}

void NamespaceHandler::Namespace::addSymbol(const NamespacedIdentifier& aliasId, const TypeInfo& type, SymbolType symbolType)
{
	jassert(aliasId.getParent() == id);

	if (contains(aliasId))
		return;

	aliases.add({ aliasId, type, symbolType });
}

juce::String NamespaceHandler::Alias::toString() const
{
	juce::String s;

	switch (symbolType)
	{
	case Struct:   s << "struct " << id.toString(); break;
	case Function: s << "function " << id.toString() << "\n"; break;
	case Variable: s << type.toString() << " " << id.toString(); break;
	case UsingAlias:	   s << "using " << id.toString() << " = " << type.toString(); break;
	case Constant: s << "static " << type.toString() << " " << id.toString() << " = " << Types::Helpers::getCppValueString(constantValue); break;
	case StaticFunctionClass: s << "Function class " << id.toString(); break;
	default: jassertfalse;
	}

	return s;
}

snex::jit::ComplexType::Ptr NamespaceHandler::registerComplexTypeOrReturnExisting(ComplexType::Ptr ptr)
{
	for (auto c : complexTypes)
	{
		if (c->matchesOtherType(ptr))
			return c;
	}

	if (currentNamespace == nullptr)
		pushNamespace(Identifier());

	ptr->registerExternalAtNamespaceHandler(this);
	
	complexTypes.add(ptr);
	return ptr;
}

snex::jit::ComplexType::Ptr NamespaceHandler::getComplexType(NamespacedIdentifier id)
{
	resolve(id, true);

	for (auto c : complexTypes)
	{
		if (c->matchesId(id))
			return c;
	}

	return nullptr;
}

snex::jit::VariadicSubType::Ptr NamespaceHandler::getVariadicTypeForId(NamespacedIdentifier id) const
{
	resolve(id, true);

	for (auto vt : variadicTypes)
		if (vt->variadicId == id)
			return vt;

	return nullptr;
}

bool NamespaceHandler::isTemplatedMethod(NamespacedIdentifier functionId) const
{
	resolve(functionId, true);

	for (auto vt : variadicTypes)
	{
		for (const auto& f : vt->functions)
			if (f.id.getIdentifier() == functionId.getIdentifier())
				return true;
	}

	return false;
}

bool NamespaceHandler::changeSymbolType(NamespacedIdentifier id, SymbolType newType)
{
	resolve(id, false);

	if (auto e = get(id.getParent()))
	{
		for (auto& a : e->aliases)
		{
			if (a.id == id)
			{
				a.symbolType = newType;
				return true;
			}
		}
	}

	return false;
}

void NamespaceHandler::pushNamespace(const NamespacedIdentifier& namespaceId)
{
	if (auto e = get(namespaceId))
	{
		currentNamespace = e;
		currentParent = e->parent;
		return;
	}

	if (namespaceId.isExplicit())
	{
		pushNamespace(namespaceId.getIdentifier());
		return;
	}
		
	jassert(!namespaceId.isExplicit());

	auto p = namespaceId.getParent();

	pushNamespace(p);
	pushNamespace(namespaceId.getIdentifier());
}

void NamespaceHandler::pushNamespace(const Identifier& id)
{
	if (currentNamespace == nullptr)
	{
		jassert(!id.isValid());

		currentNamespace = new Namespace();
		currentNamespace->id = NamespacedIdentifier();
		existingNamespace.add(currentNamespace);
		return;
	}

	if (!id.isValid())
	{
		currentNamespace = getRoot();
		return;
	}

	jassert(id.isValid());

	auto newId = currentNamespace->id.getChildId(id);
	currentParent = currentNamespace;

	if (auto existing = get(newId))
	{
		currentNamespace = existing;
	}
	else
	{
		currentNamespace = new Namespace();
		currentNamespace->id = newId;
		currentNamespace->parent = currentParent;
		existingNamespace.add(currentNamespace);

		currentParent->childNamespaces.add(currentNamespace);
	}

	jassert(currentParent->childNamespaces.contains(currentNamespace));
}

snex::jit::NamespacedIdentifier NamespaceHandler::getRootId() const
{
	return getRoot()->id;
}

bool NamespaceHandler::isNamespace(const NamespacedIdentifier& possibleNamespace) const
{
	return get(possibleNamespace) != nullptr;
}

juce::Result NamespaceHandler::popNamespace()
{
	if (currentNamespace == getRoot())
		return Result::ok();

	if (currentParent == nullptr)
		return Result::fail("Can't pop namespace");

	currentNamespace = currentNamespace->parent;

	return Result::ok();
}

snex::jit::NamespacedIdentifier NamespaceHandler::getCurrentNamespaceIdentifier() const
{
	if (currentNamespace == nullptr)
		return {};

	return currentNamespace->id;
}

juce::String NamespaceHandler::dump()
{
	return getRoot()->dump(0);
}

juce::Result NamespaceHandler::addUsedNamespace(const NamespacedIdentifier& usedNamespace)
{
	if (auto existing = get(usedNamespace))
	{
		currentNamespace->usedNamespaces.add(existing);
		return Result::ok();
	}
	else
		return Result::fail("Can't find namespace " + usedNamespace.toString());
}

juce::Result NamespaceHandler::resolve(NamespacedIdentifier& id, bool allowZeroMatch /*= false*/) const
{
	if (currentNamespace == nullptr)
		return Result::fail("no namespace available");

	if (currentNamespace->contains(id))
	{
		return Result::ok();
	}

	auto name = id.getIdentifier();

	

	auto isExplicitId = id.getParent().isValid() && !(id.getParent() == currentNamespace->id);

	if (!isExplicitId)
	{
		Namespace::WeakPtr p = currentNamespace->parent;

		while (p != nullptr)
		{
			auto parentId = p->id.getChildId(name);

			if (p->contains(parentId))
			{
				id = parentId;
				return Result::ok();
			}

			p = p->parent;
		}
	}

	auto parent = id.getParent();
	Array<NamespacedIdentifier> matches;

	if (auto existing = get(parent))
	{
		auto possibleSymbol = existing->id.getChildId(name);

		if (existing->contains(possibleSymbol))
			matches.add(possibleSymbol);
		else
		{
			for (auto un : existing->usedNamespaces)
			{
				possibleSymbol = un->id.getChildId(name);

				if (un->contains(possibleSymbol))
					matches.add(possibleSymbol);
			}
		}
	}

	if (!allowZeroMatch && matches.size() == 0)
		return Result::fail(name.toString() + " can't be resolved");

	if (matches.size() > 1)
		return Result::fail(name.toString() + " is ambiguous");

	if (matches.size() == 1)
		id = matches.getFirst();

	return Result::ok();
}

void NamespaceHandler::addSymbol(const NamespacedIdentifier& id, const TypeInfo& t, SymbolType symbolType)
{
	jassert(id.getParent() == currentNamespace->id);

	currentNamespace->addSymbol(id, t, symbolType);
}

Result NamespaceHandler::addConstant(const NamespacedIdentifier& id, const VariableStorage& v)
{
	for (auto& a : currentNamespace->aliases)
	{
		if (a.id == id)
		{
			if (a.type == v.getType())
				a.constantValue = v;
			else
				a.constantValue = VariableStorage(a.type.getType(), (double)v);
			
			return Result::ok();
		}
	}

	return Result::fail("fail");
}

juce::Result NamespaceHandler::setTypeInfo(const NamespacedIdentifier& id, SymbolType expectedType, const TypeInfo& t)
{
	if (auto p = get(id.getParent()))
	{
		for (auto& a : p->aliases)
		{
			if (a.id == id)
			{
				if (a.symbolType == expectedType)
				{
					a.type = t;
					return Result::ok();
				}
				else
					return Result::fail("Symbol type mismatch");
			}
		}

		return Result::fail("Can't find symbol");
	}

	return Result::fail("Can't find namespace");
}

bool NamespaceHandler::rootHasNamespace(const NamespacedIdentifier& id) const
{
	auto type = getSymbolType(id);
	auto ns = get(id);
	
	return type == Unknown || type == Struct && ns != nullptr;
}

NamespaceHandler::SymbolType NamespaceHandler::getSymbolType(const NamespacedIdentifier& id) const
{
	if (auto p = get(id.getParent()))
	{
		for (const auto& a : p->aliases)
		{
			if (a.id == id)
				return a.symbolType;
		}
	}

	return NamespaceHandler::SymbolType::Unknown;
}

TypeInfo NamespaceHandler::getAliasType(const NamespacedIdentifier& aliasId) const
{
	return getTypeInfo(aliasId, { SymbolType::Struct, SymbolType::UsingAlias });
}

TypeInfo NamespaceHandler::getVariableType(const NamespacedIdentifier& variableId) const
{
	return getTypeInfo(variableId, { SymbolType::Variable, SymbolType::Constant, SymbolType::Function });
}

snex::VariableStorage NamespaceHandler::getConstantValue(const NamespacedIdentifier& variableId) const
{
	auto p = variableId.getParent();

	if (auto existing = get(p))
	{
		for (auto a : existing->aliases)
		{
			if (a.id == variableId && a.symbolType == Constant)
				return a.constantValue;
		}
	}

	return {};
}

bool NamespaceHandler::isStaticFunctionClass(const NamespacedIdentifier& classId) const
{
	auto p = classId.getParent();

	if (auto existing = get(p))
	{
		for (auto& a : existing->aliases)
		{
			if (a.id == classId && a.symbolType == StaticFunctionClass)
				return true;
		}
	}

	return false;
}

TypeInfo NamespaceHandler::getTypeInfo(const NamespacedIdentifier& aliasId, const Array<SymbolType>& t) const
{
	auto p = aliasId.getParent();

	if (auto existing = get(p))
	{
		for (auto a : existing->aliases)
		{
			if (a.id == aliasId && t.contains(a.symbolType))
				return a.type;
		}
	}

	return {};
}

snex::jit::NamespaceHandler::Namespace::WeakPtr NamespaceHandler::getRoot() const
{
	Namespace::WeakPtr r = existingNamespace.getFirst().get();

	if (r == nullptr)
		return nullptr;

	while (r->parent != nullptr)
		r = r->parent;

	return r;
}

snex::jit::NamespaceHandler::Namespace::Ptr NamespaceHandler::get(const NamespacedIdentifier& id) const
{
	for (auto e : existingNamespace)
	{
		if (e->id == id)
			return e;
	}

	return nullptr;
}

}
}
