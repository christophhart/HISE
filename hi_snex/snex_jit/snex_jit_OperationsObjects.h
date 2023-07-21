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

#pragma once

namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


struct Operations::ClassStatement : public Statement,
	public Operations::ClassDefinitionBase
{
	SET_EXPRESSION_ID(ClassStatement)

	ClassStatement(Location l, ComplexType::Ptr classType_, Statement::Ptr classBlock, const Array<TemplateInstance>& baseClasses);

	~ClassStatement()
	{
		classType = nullptr;
	}

	Result addBaseClasses();

	void createMembersAndFinalise();

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new ClassStatement(l, classType, getChildStatement(0)->clone(l), baseClasses);
	}

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();
		t.setProperty("Type", classType->toString(), nullptr);

		if(auto st = dynamic_cast<StructType*>(classType.get()))
		{
			String memberInfo;

			for (int i = 0; i < st->getNumMembers(); i++)
			{
				auto mId = st->getMemberName(i);
				memberInfo << st->getMemberTypeInfo(mId).toStringWithoutAlias() << " " << mId << "(" << st->getMemberOffset(i) << ")";

				if(i != st->getNumMembers()-1)
					memberInfo << "$";
			}

			t.setProperty("MemberInfo", memberInfo, nullptr);
            
            if(!st->getTemplateInstanceParameters().isEmpty())
            {
                // fix missing template arguments in function call ObjectType property etc..
                // simply replace ClassId:: with ClassId<ARGS...>::
                auto classIdWithoutT = st->id.toString() + "::";
                auto classId = classType->toString() + "::";
                
                Operations::callRecursive(t, [classIdWithoutT, classId](ValueTree& v)
                {
                    if(v.getType() == Identifier("FunctionCall") && v.hasProperty("ObjectType"))
                    {
                        auto objectId = v["ObjectType"].toString();
                        
                        if(objectId.startsWith(classIdWithoutT))
                        {
                            objectId = objectId.replace(classIdWithoutT, classId);
                            v.setProperty("ObjectType", objectId, nullptr);
                            DBG("Replace ObjectType property with " + objectId);
                        }
                    }
                    
                    return false;
                });
            }
		}

        
        
        
		return t;
	}

	bool isTemplate() const override { return false; }

	TypeInfo getTypeInfo() const override { return {}; }

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const override
	{
		jassert(compiler->getCurrentPass() > BaseCompiler::ComplexTypeParsing);
		return classType->getRequiredByteSize();
	}

	void process(BaseCompiler* compiler, BaseScope* scope);

	StructType* getStructType()
	{
		return dynamic_cast<StructType*>(classType.get());
	}

	Array<TemplateInstance> baseClasses;
	ComplexType::Ptr classType;
	ScopedPointer<ClassScope> subClass;

	Array<ComplexType::Ptr> baseClassTypes;
};


struct Operations::InternalProperty : public Expression
{
	SET_EXPRESSION_ID(InternalProperty);

	InternalProperty(Location l, const Identifier& id_, const var& v_):
		Expression(l),
		id(id_),
		v(v_)
	{

	}

	Ptr clone(Location l) const override
	{
		return new InternalProperty(l, id, v);
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("ID", id.toString(), nullptr);
		t.setProperty("Value", v.toString(), nullptr);

		return t;
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(Types::ID::Void);
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	Identifier id;
	var v;
};

struct Operations::ComplexTypeDefinition : public Expression,
	public TypeDefinitionBase
{
	SET_EXPRESSION_ID(ComplexTypeDefinition);

	ComplexTypeDefinition(Location l, const Array<NamespacedIdentifier>& ids_, TypeInfo type_) :
		Expression(l),
		ids(ids_),
		type(type_)
	{}

	void addInitValues(InitialiserList::Ptr l)
	{
		initValues = l;

		int expressionIndex = 0;

		initValues->forEach([this, &expressionIndex](InitialiserList::ChildBase* b)
		{
			if (auto ec = dynamic_cast<InitialiserList::ExpressionChild*>(b))
			{
				ec->expressionIndex = expressionIndex++;
				this->addStatement(ec->expression);
			}

			return false;
		});
	}

	Array<NamespacedIdentifier> getInstanceIds() const override { return ids; }

	Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		Ptr n = new ComplexTypeDefinition(l, ids, type);

		cloneChildren(n);

		if (initValues != nullptr)
			as<ComplexTypeDefinition>(n)->initValues = initValues;

		return n;
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		juce::String names;

		for (auto id : ids)
			names << id.toString() << ",";

		t.setProperty("Type", type.toStringWithoutAlias(), nullptr);

		t.setProperty("Ids", names, nullptr);

		t.setProperty("NumBytes", (int)type.getRequiredByteSize(), nullptr);

		if (initValues != nullptr)
		{
			auto numBytes = type.getRequiredByteSizeNonZero();

			if (numBytes % 8 != 0)
				numBytes += 8 - (numBytes % 8);

			MemoryBlock mb(numBytes);

			memset(mb.getData(), 0, numBytes);

			ComplexType::InitData d;
			d.callConstructor = false;
			d.dataPointer = mb.getData();
			d.initValues = initValues;

			type.getComplexType()->initialise(d);

			auto x = mb.toBase64Encoding();


			t.setProperty("InitValues", initValues->toString(), nullptr);
			t.setProperty("InitValuesB64", x, nullptr);
		}
			

		return t;
	}

	Array<Symbol> getSymbols() const
	{
		Array<Symbol> symbols;

		for (auto id : ids)
		{
			symbols.add({ id, getTypeInfo() });
		}

		return symbols;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	bool isStackDefinition(BaseScope* scope) const
	{
		return dynamic_cast<RegisterScope*>(scope) != nullptr;
	}



	Array<NamespacedIdentifier> ids;
	TypeInfo type;



	InitialiserList::Ptr initValues;

	ReferenceCountedArray<AssemblyRegister> stackLocations;
};


}
}
