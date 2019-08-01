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
using namespace asmjit;

hnode::jit::BaseScope::ScopeType BaseScope::getScopeType() const
{
	return scopeType;
}

hnode::jit::BaseScope::RefPtr BaseScope::get(const Symbol& s)
{
	for (auto er : referencedVariables)
	{
		if (er->id == s)
			return er;
	}

	for (auto c : constants)
	{
		if (c.id == s)
		{
			RefPtr r = new Reference(this, s.id, c.id.type);
			r->isConst = true;
			referencedVariables.add(r);
			return r;
		}
	}

	for (auto ir : allocatedVariables)
	{
		if (ir.id == s.id)
		{
			RefPtr r = new Reference(this, s.id, ir.ref.getType());
			referencedVariables.add(r);
			return r;
		}
	}

	return nullptr;
}

bool BaseScope::addConstant(const Identifier& id, VariableStorage v)
{
	for (auto r : referencedVariables)
	{
		if (r->id.id == id)
			return false;
	}

	for (auto c : constants)
	{
		if (c.id.id == id)
			return false;
	}

	constants.add({ { {}, id, v.getType() }, v });
	return true;
}

hnode::jit::BaseScope* BaseScope::getParent()
{
	return parent.get();
}

BaseScope::BaseScope(const Identifier& id, BaseScope* parent_ /*= nullptr*/, int numVariables /*= 1024*/) :
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

	size = numVariables;
	data.allocate(size, true);
}

BaseScope::~BaseScope()
{

}

juce::Result BaseScope::allocate(const Identifier& id, VariableStorage v)
{
	for (auto v : allocatedVariables)
	{
		if (v.id == id)
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
}

hnode::jit::BaseScope* BaseScope::getScopeForSymbol(const Symbol& s)
{
	if (scopeId == s.parent)
	{
		for (const auto& c : constants)
			if (c.id.matchesIdAndType(s))
				return this;

		for (const auto& v : allocatedVariables)
			if (v == s)
				return this;

		if (scopeId.isValid())
			return nullptr;
	}

	if (parent != nullptr)
		return parent->getScopeForSymbol(s);

	return nullptr;
}

juce::Result BaseScope::deallocate(Reference v)
{
	for (const auto& ir : allocatedVariables)
	{
		if (v.id.id == ir.id)
		{
			ir.ref = {};
			allocatedVariables.remove(allocatedVariables.indexOf(ir));
			return Result::ok();
		}
	}

	return Result::fail("Can't deallocate Variable " + v.id.toString());
}

juce::Array<hnode::jit::BaseScope::Symbol> BaseScope::getAllVariables() const
{
	Array<Symbol> variableIds;

	variableIds.ensureStorageAllocated(allocatedVariables.size());

	for (auto v : allocatedVariables)
		variableIds.add({ scopeId, v.id, v.ref.getType() });

	return variableIds;
}

hnode::VariableStorage & BaseScope::getVariableReference(const Identifier& id)
{
	for (auto v : allocatedVariables)
	{
		if (v.id == id)
			return v.ref;
	}

	jassertfalse;
	empty = {};
	return empty;
}


bool BaseScope::Symbol::operator==(const Symbol& other) const
{
	return parent == other.parent && id == other.id;
}


BaseScope::Symbol::Symbol(const Identifier& parent_, const Identifier& id_, Types::ID t_) :
	parent(parent_),
	id(id_),
	type(t_)
{

}


BaseScope::Symbol::Symbol()
{

}


bool BaseScope::Symbol::matchesIdAndType(const Symbol& other) const
{
	return other == *this;// && other.type == type;
}

juce::String BaseScope::Symbol::toString() const
{
	if (id.isNull())
		return "undefined";

	String s;

	if (parent.isValid())
		s << parent << "." << id;
	else
		s << id;

	return s;
}

BaseScope::Symbol::operator bool() const
{
	return id.isValid();
}


bool BaseScope::Reference::operator==(const Reference& other) const
{
	return id == other.id && scope.get() == other.scope.get();
}


BaseScope::Reference::Reference(BaseScope* p, const Identifier& s, Types::ID type) :
	scope(p),
	id(p->scopeId, s, type)
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
	return scope.get()->getVariableReference(id.id).getDataPointer();
}

hnode::VariableStorage BaseScope::Reference::getDataCopy() const
{
	return VariableStorage(scope.get()->getVariableReference(id.id));
}

hnode::VariableStorage& BaseScope::Reference::getDataReference(bool allowConstInitialisation /*= false*/) const
{
	jassert(!(isConst && !allowConstInitialisation));

	return scope.get()->getVariableReference(id.id);
}

hnode::Types::ID BaseScope::Reference::getType() const
{
	return id.type;
}


bool BaseScope::InternalReference::operator==(const InternalReference& other) const
{
	auto thisType = ref.getType();
	auto otherType = other.ref.getType();

	return id == other.id;
}


bool BaseScope::InternalReference::operator==(const Symbol& s) const
{
	auto symbolType = s.type;
	auto thisType = ref.getType();

	return id == s.id && Types::Helpers::matchesType(symbolType, thisType);
}

BaseScope::InternalReference::InternalReference(const Identifier& id_, VariableStorage& r) :
	id(id_),
	ref(r)
{

}

}
}