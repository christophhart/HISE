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

BaseScope* BaseScope::getParent()
{
	return parent.get();
}

NamespaceHandler& BaseScope::getNamespaceHandler()
{
	auto h = getRootClassScope()->handler;
	jassert(h != nullptr);
	return *h;
}

RootClassData* BaseScope::getRootData() const
{
	return getRootClassScope()->rootData.get();
}

BaseScope::BaseScope(const NamespacedIdentifier& id, BaseScope* parent_ /*= nullptr*/) :
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

snex::jit::BaseScope* BaseScope::findScopeWithId(const NamespacedIdentifier& id)
{
	if (scopeId == id)
		return this;

	for (auto cs : childScopes)
	{
		if (auto found = cs->findScopeWithId(id))
			return found;
	}

	if (getRootClassScope() == this)
	{
		if (getNamespaceHandler().rootHasNamespace(id))
			return this;
	}

	return nullptr;
}

BaseScope* BaseScope::getScopeForSymbolInternal(const NamespacedIdentifier& s)
{
	auto parentSymbol = s.getParent();

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

	if (auto fc = dynamic_cast<FunctionClass*>(this))
	{
		if (auto c = fc->getSubFunctionClass(parentSymbol))
		{
			if (auto cs = dynamic_cast<BaseScope*>(c))
				return cs;
			else
				return this;
		}
	}

	return nullptr;
}



bool BaseScope::hasSymbol(const NamespacedIdentifier& s)
{
	jassert(s.getParent() == scopeId || getNamespaceHandler().rootHasNamespace(s.getParent()));

	auto type = getNamespaceHandler().getSymbolType(s);

	return type != NamespaceHandler::Unknown;
}

BaseScope* BaseScope::getScopeForSymbol(const NamespacedIdentifier& s)
{
	if (getScopeType() == Global)
	{
		if (auto fc = getGlobalScope()->getGlobalFunctionClass(s))
			return this;

		if (hasSymbol(s))
			return this;
	}

	auto bs = getRootClassScope()->findScopeWithId(s.getParent());

	jassert(bs->hasSymbol(s));

	return bs;
}


#if 0
bool BaseScope::updateSymbol(Symbol& symbolToBeUpdated)
{
	jassert(getScopeForSymbol(symbolToBeUpdated) == this);

	for (auto c : constants)
	{
		if (c.id == symbolToBeUpdated)
		{
			jassert(Types::Helpers::isFixedType((c.v.getType())));

			symbolToBeUpdated.typeInfo = TypeInfo(c.v.getType(), true, false);
			symbolToBeUpdated.constExprValue = c.v;
			return true;
		}
	}

	if (getScopeType() != Global && getRootData()->contains(symbolToBeUpdated))
	{
		return getRootData()->updateSymbol(symbolToBeUpdated);
	}

	if (getRootClassScope() == this)
	{
		if (getRootData()->hasFunction(symbolToBeUpdated))
			return true;

		if (auto c = getRootData()->getSubFunctionClass(symbolToBeUpdated.getParentSymbol()))
		{
			if (c->hasFunction(symbolToBeUpdated))
				return true;

			auto v = c->getConstantValue(symbolToBeUpdated);
			symbolToBeUpdated.constExprValue = v;
			symbolToBeUpdated.typeInfo = TypeInfo(v.getType(), true);

			return true;
		}
	}

	return false;
}
#endif

juce::String RootClassData::dumpTable() const
{
	
	juce::String s;
	juce::String nl("\n");

	s << "Dumping root data table" << nl;


	for (const auto& st : symbolTable)
	{
		if (st.s.typeInfo.isComplexType())
		{
			int il = 0;

			s << st.s.typeInfo.getComplexType()->toString() << " ";
				
			s << st.s.toString() << "\n";
			st.s.typeInfo.getComplexType()->dumpTable(s, il, data.get(), st.data);
		}
		else
		{
			Types::Helpers::dumpNativeData(s, 0, st.s.toString(), data.get(), st.data, Types::Helpers::getSizeForType(st.s.typeInfo.getType()), st.s.typeInfo.getType());
		}
	}

	return s;
}

RootClassData::RootClassData() :
	FunctionClass(NamespacedIdentifier(Identifier()))
{
	
}

juce::Result RootClassData::initData(BaseScope* scope, const Symbol& s, InitialiserList::Ptr initValues)
{
	if (scope == scope->getRootClassScope())
	{
		for (const auto& ts : symbolTable)
		{
			if (ts.s == s)
			{
				if (ts.s.typeInfo.isComplexType())
				{
					return ts.s.typeInfo.getComplexType()->initialise(ts.data, initValues);
				}
				else
				{
					VariableStorage initValue;
					initValues->getValue(0, initValue);

					if (ts.s.typeInfo.getType() == initValue.getType())
					{
						ComplexType::writeNativeMemberType(ts.data, 0, initValue);
						return Result::ok();
					}

					return Result::fail("type mismatch");
				}
			}
		}
	}
	
	if (auto cs = dynamic_cast<ClassScope*>(scope))
	{
		if (cs->typePtr != nullptr)
		{
			return initSubClassMembers(cs->typePtr.get(), s.id.getIdentifier(), initValues);
		}
	}

	return Result::fail("not found");
}

void* RootClassData::getDataPointer(const NamespacedIdentifier& s) const
{
	for (const auto& ts : symbolTable)
	{
		if (ts.s.id == s)
			return ts.data;

		uint64_t data = reinterpret_cast<uint64_t>(ts.data);

		auto found = checkSubClassMembers(ts, s, [&data](StructType* sc, const Identifier& id)
		{
			data += sc->getMemberOffset(id);
		});

		if (found)
			return (void*)data;
	}

	jassertfalse;
	return nullptr;
}

juce::Result RootClassData::initSubClassMembers(ComplexType::Ptr type, const Identifier& memberId, InitialiserList::Ptr initList)
{
	for (const auto& ts : symbolTable)
	{
		if (ts.s.typeInfo.isComplexType())
		{
			ts.s.typeInfo.getComplexType()->forEach([memberId, initList](ComplexType::Ptr p, void* dataPointer)
			{
				if (auto structType = dynamic_cast<StructType*>(p.get()))
				{
					auto offset = structType->getMemberOffset(memberId);

					if (structType->isNativeMember(memberId))
					{
						VariableStorage initValue;

						jassert(initList->size() == 1);

						initList->getValue(0, initValue);

						ComplexType::writeNativeMemberType(dataPointer, offset, initValue);
						return false;
					}
					else
					{
						auto memberType = structType->getMemberComplexType(memberId);
						auto ptr = ComplexType::getPointerWithOffset(dataPointer, offset);
						memberType->initialise(ptr, initList);
						return false;
					}
				}

				p->initialise(dataPointer, initList);

				return false;
			}, type, ts.data);
		}
	}

	return Result::ok();
}

bool RootClassData::checkSubClassMembers(const TableEntry& ts, const NamespacedIdentifier& s, const std::function<void(StructType* sc, const Identifier& id)>& f) const
{
	if (ts.s.id ==  s.getParent() && ts.s.typeInfo.isComplexType())
	{
		auto path = s.getIdList();
		path.remove(0);

		auto tp = ts.s.typeInfo.getComplexType();

		for (int i = 0; i < path.size(); i++)
		{
			auto thisMember = path[i];
			auto st = dynamic_cast<StructType*>(tp.get());

			if (st == nullptr)
				break;

			f(st, thisMember);

			tp = st->getMemberComplexType(thisMember).get();
		}

		return true;
	}

	return false;
}

}
}
