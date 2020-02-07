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

snex::jit::BaseScope::ScopeType BaseScope::getScopeType() const
{
	return scopeType;
}

snex::jit::GlobalScope* BaseScope::getGlobalScope()
{
	BaseScope* c = this;

	while (c != nullptr && c->getScopeType() != Global)
		c = c->getParent();

	return dynamic_cast<GlobalScope*>(c);
}

snex::jit::ClassScope* BaseScope::getRootClassScope() const
{
	BaseScope* c = const_cast<BaseScope*>(this);

	while (c != nullptr && c->getParent() != nullptr && c->getParent()->getScopeType() != Global)
		c = c->getParent();

	return dynamic_cast<ClassScope*>(c);
}

bool BaseScope::addConstant(const Identifier& id, VariableStorage v)
{
	if (getRootClassScope()->rootData->contains(scopeId.getChildSymbol(id)))
		return false;

	for (auto c : constants)
	{
		if (c.id.id == id)
			return false;
	}

	constants.add({ scopeId.getChildSymbol(id, v.getType()), v });
	return true;
}

snex::jit::BaseScope* BaseScope::getParent()
{
	return parent.get();
}

RootClassData* BaseScope::getRootData() const
{
	return getRootClassScope()->rootData.get();
}

BaseScope::BaseScope(const Symbol& id, BaseScope* parent_ /*= nullptr*/, int numVariables /*= 1024*/) :
	scopeId(id),
	parent(parent_)
{
	if (parent == nullptr)
		scopeType = Global;
	else if (parent->getParent() == nullptr)
		scopeType = Class;
	else if (parent->getParent()->getParent() == nullptr)
		scopeType = Function;
	else
		scopeType = Anonymous;

	if (parent != nullptr)
		parent->childScopes.add(this);
}

BaseScope::~BaseScope()
{
	if (parent != nullptr)
	{
		parent->childScopes.removeAllInstancesOf(this);
	}
}

juce::Result BaseScope::allocate(const Identifier& id, VariableStorage v)
{
	return getRootClassScope()->rootData->allocate(this, scopeId.getChildSymbol(id, v.getType()));

#if 0
	for (auto av : allocatedVariables)
	{
		if (av.id == id)
			return Result::fail("already exists");
	};

	for (int i = 0; i < size; i++)
	{
		if (data[i].getType() == Types::ID::Void)
		{
			data[i] = v;
			allocatedVariables.add({ id, *(data + i) });
			return Result::ok();
		}
	}

	return Result::fail("Can't allocate");
#endif
}

BaseScope* BaseScope::getScopeForSymbolInternal(const Symbol& s)
{
	auto parentSymbol = s.getParentSymbol();

	if (parentSymbol == scopeId)
	{
		if (hasSymbol(s))
			return this;
	}

	for (auto c : childScopes)
	{
		if (auto m = c->getScopeForSymbolInternal(s))
			return m;
	}

	return nullptr;
}



bool BaseScope::hasSymbol(const Symbol& s)
{
	for (auto& c : constants)
		if (c.id == s)
			return this;

	if (hasVariable(s.id))
		return this;

#if 0
	for (auto v : allocatedVariables)
		if (v.id == s.id)
			return this;
#endif

	if (auto fc = dynamic_cast<FunctionClass*>(this))
	{
		if (fc->hasFunction(s))
			return this;
	}
}

BaseScope* BaseScope::getScopeForSymbol(const Symbol& s)
{
	if (getScopeType() == Global)
		return nullptr;

	auto isExplicitSymbol = s.getParentSymbol();

	if (isExplicitSymbol)
	{
		

		if (getRootClassScope() == this)
		{
			return getScopeForSymbolInternal(s);
		}
		else
		{
			return getRootClassScope()->getScopeForSymbol(s);
		}
	}
	else
	{
		if (hasSymbol(s))
			return this;
		else
		{
			if (auto p = getParent())
				return p->getScopeForSymbol(s);
			else
				return nullptr;
		}
	}


#if 0
	for (auto& c : constants)
		if (c.id == s)
			return this;

	auto parentSymbol = s.getParentSymbol();

	if (hasVariable(s.id))
		return this;

	for (auto v : allocatedVariables)
		if (v.id == s.id)
			return this;

	if (auto fc = dynamic_cast<FunctionClass*>(this))
	{
		if (fc->hasFunction(s))
			return this;

		if (auto match = fc->getChildScope(s))
			return match;
	}

	if (lookInParentScope)
	{
		if (auto p = getParent())
			return p->getScopeForSymbol(s);
	}
	

	return nullptr;
#endif
}

bool BaseScope::hasVariable(const Identifier& id) const
{
	auto s = scopeId.getChildSymbol(id);
	return getRootClassScope()->rootData->contains(s);
}

juce::Array<snex::jit::BaseScope::Symbol> BaseScope::getAllVariables() const
{
	Array<Symbol> variableIds;

	for (const auto& td : *getRootClassScope()->rootData)
		variableIds.add(td.s);

	return variableIds;
}


#if 0
bool BaseScope::Reference::operator==(const Reference& other) const
{
	return id == other.id && scope.get() == other.scope.get();
}


BaseScope::Reference::Reference(BaseScope* p, const Identifier& s, Types::ID type) :
	scope(p),
	id(p->scopeId.getChildSymbol(s, type))
{
}


BaseScope::Reference::Reference() : scope(nullptr), id({})
{

}

int BaseScope::Reference::getNumReferences() const
{
	return getReferenceCount() - 1;
}

void* BaseScope::Reference::getDataPointer() const
{
	return getDataReference().getDataPointer();
}

snex::VariableStorage BaseScope::Reference::getDataCopy() const
{
	return VariableStorage(getDataReference());
}

snex::VariableStorage& BaseScope::Reference::getDataReference(bool allowConstInitialisation /*= false*/) const
{
	ignoreUnused(allowConstInitialisation);
	jassert(!(isConst && !allowConstInitialisation));

	return scope->getRootClassScope()->rootData->get(id);
}

snex::Types::ID BaseScope::Reference::getType() const
{
	return id.type;
}
#endif


}
}