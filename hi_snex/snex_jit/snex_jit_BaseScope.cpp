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
	if (scopeType != Global && getRootClassScope()->rootData->contains(scopeId.getChildSymbol(id)))
		return false;

	for (auto c : constants)
	{
		if (c.id.id == id)
			return false;
	}

	constants.add({ Symbol::createRootSymbol(id).withType(v.getType()), v });
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

BaseScope* BaseScope::getScopeForSymbolInternal(const Symbol& s)
{
	Symbol sToUse = s;
	auto parentSymbol = sToUse.getParentSymbol();

	// Here we exchange the member variable ID with the class ID
	if (auto ct = dynamic_cast<StructType*>(getRootData()->getComplexTypeForVariable(parentSymbol).get()))
	{
		sToUse = s.relocate(ct->id);
		parentSymbol = sToUse.getParentSymbol();
	}

	if (parentSymbol == scopeId)
	{
		if (hasSymbol(sToUse))
			return this;
	}


	for (auto c : childScopes)
	{
		if (auto m = c->getScopeForSymbolInternal(sToUse))
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



bool BaseScope::hasSymbol(const Symbol& s)
{
	for (auto& c : constants)
		if (c.id == s)
			return true;

	if (hasVariable(s.id))
		return true;

	if (auto fc = dynamic_cast<FunctionClass*>(this))
	{
		if (fc->hasFunction(s))
			return true;

		if (fc->hasConstant(s))
			return true;
	}
    
    return false;
}

BaseScope* BaseScope::getScopeForSymbol(const Symbol& s)
{
	if (getScopeType() == Global)
	{
		if (auto fc = getGlobalScope()->getGlobalFunctionClass(s.id))
			return this;

		if (hasSymbol(s))
			return this;
	}

	if (auto pSymbol = s.getParentSymbol())
	{
		if (auto pScope = getScopeForSymbol(pSymbol))
		{
			return pScope;
		}
	}

	if (hasSymbol(s))
		return this;
	else
	{
		if (auto p = getParent())
			return p->getScopeForSymbol(s);
		else
			return nullptr;
	}

#if 0
	auto isExplicitSymbol = (bool)s.getParentSymbol();

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
#endif
}

bool BaseScope::hasVariable(const Identifier& id) const
{
	auto s = scopeId.getChildSymbol(id);
	return getRootData()->contains(s);
}

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

juce::Array<snex::jit::BaseScope::Symbol> BaseScope::getAllVariables() const
{
	return getRootData()->getAllVariables();
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
	FunctionClass(Symbol::createRootSymbol("Root"))
{
	addFunctionClass(new MathFunctions());
	addFunctionClass(new MessageFunctions());
	addFunctionClass(new BlockFunctions());
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
			return initSubClassMembers(cs->typePtr, s.id, initValues);
		}
	}

	return Result::fail("not found");
}

}
}
