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
USE_ASMJIT_NAMESPACE;


void Operations::ClassStatement::process(BaseCompiler* compiler, BaseScope* scope)
{
	if (subClass == nullptr)
	{
		subClass = new ClassScope(scope, getStructType()->id, classType);
	}

	processBaseWithChildren(compiler, subClass);

	COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
	{
		jassert(getStructType()->isFinalised());
	}

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		getStructType()->finaliseAlignment();
	}
}

Operations::ClassStatement::ClassStatement(Location l, ComplexType::Ptr classType_, Statement::Ptr classBlock, const Array<TemplateInstance>& baseClasses_) :
	Statement(l),
	classType(classType_),
	baseClasses(baseClasses_)
{
	addStatement(classBlock);

	classBlock->forEachRecursive([this](Ptr p)
	{
		if (auto ip = as<InternalProperty>(p))
		{
			if (findParentStatementOfType<ClassStatement>(ip) == this)
				getStructType()->setInternalProperty(ip->id, ip->v);
		}

		return false;
	}, IterationType::AllChildStatements);
}

juce::Result Operations::ClassStatement::addBaseClasses()
{
	for (auto c : baseClasses)
	{
		auto compiler = getChildStatement(0)->currentCompiler;

		jassert(compiler != nullptr);

		if (c.tp.isEmpty())
		{
			jassert(compiler != nullptr);

			auto st = dynamic_cast<StructType*>(compiler->namespaceHandler.getComplexType(c.id).get());

			if (st == nullptr)
				location.throwError("Can't resolve base class " + c.toString());

			baseClassTypes.add(st);

			getStructType()->addBaseClass(st);
		}
		else
		{
			auto r = Result::ok();
			auto tId = TemplateInstance(c.id, {});
			auto st = compiler->namespaceHandler.createTemplateInstantiation(tId, c.tp, r);

			baseClassTypes.add(st);

			location.test(r);
			getStructType()->addBaseClass(dynamic_cast<StructType*>(st.get()));
		}
	}

	return Result::ok();
}

void Operations::ClassStatement::createMembersAndFinalise()
{
	auto cType = getStructType();

	addBaseClasses();

	forEachRecursive([cType, this](Ptr p)
	{
		if (auto f = as<Function>(p))
		{
			if (f->data.id.getParent() == cType->id)
				cType->addJitCompiledMemberFunction(f->data);
		}

		if (auto tf = as<TemplatedFunction>(p))
		{
			auto fData = tf->data;
			fData.templateParameters = tf->templateParameters;

			cType->addJitCompiledMemberFunction(fData);
		}

		return false;
	}, IterationType::AllChildStatements);

	for (auto bc : baseClassTypes)
		bc->finaliseAlignment();


	addMembersFromStatementBlock(getStructType(), getChildStatement(0));
}

void Operations::ComplexTypeDefinition::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
	{
		if (auto tcd = type.getTypedComplexType<TemplatedComplexType>())
		{
			jassertfalse;
		}

		if (type.isComplexType())
			type.getComplexType()->finaliseAlignment();

		if (!isStackDefinition(scope) && scope->getRootClassScope() == scope)
			scope->getRootData()->enlargeAllocatedSize(type);
	}
	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		for (auto s : getSymbols())
		{
			if (isStackDefinition(scope))
			{

			}
			else if (scope->getRootClassScope() == scope)
				scope->getRootData()->allocate(scope, s);
		}
	}
	COMPILER_PASS(BaseCompiler::DataInitialisation)
	{
		if (getNumChildStatements() == 0 && initValues == nullptr)
		{
			initValues = type.makeDefaultInitialiserList();
		}

		// Override the init values for the constructor here...
		// (They should have been passed to the constructor call afterwards
		if (isStackDefinition(scope) &&					// only stack created objects
			type.getComplexType()->hasConstructor() &&  // only objects with a constructor
			as<FunctionCall>(getSubExpr(0)) == nullptr) // only definitions that do not have a function call as sub expression:
														// auto x = MyObject(...);
		{
			initValues = type.makeDefaultInitialiserList();
		}
			

		if (!isStackDefinition(scope))
		{
			for (auto s : getSymbols())
			{
				if (scope->getRootClassScope() == scope)
				{
					auto r = scope->getRootData()->initData(scope, s, initValues);

					if (!r.wasOk())
						location.throwError(r.getErrorMessage());
				}
				else if (auto cScope = dynamic_cast<ClassScope*>(scope))
				{
					if (auto st = dynamic_cast<StructType*>(cScope->typePtr.get()))
						st->setDefaultValue(s.id.getIdentifier(), initValues);
				}
			}
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (isStackDefinition(scope))
		{
			if (type.isRef())
			{
				if (auto s = getSubExpr(0))
				{
					if (!canBeReferenced(s))
					{
						location.throwError("Can't assign reference to temporary type");
					}

					reg = s->reg;

					if (reg != nullptr)
						reg->setReference(scope, getSymbols().getFirst());
				}
			}
			else
			{
				auto acg = CREATE_ASM_COMPILER(getType());

				for (auto s : getSymbols())
				{
					auto reg = compiler->registerPool.getRegisterForVariable(scope, s);

					if (reg->getType() == Types::ID::Pointer)
					{
						auto c = acg.cc.newStack(type.getRequiredByteSizeNonZero(), type.getRequiredAlignmentNonZero());
						reg->setCustomMemoryLocation(c, false);
					}

					stackLocations.add(reg);
				}
			}
		}
	}
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (isStackDefinition(scope))
		{
			if (type.isRef())
			{
				if (auto s = getSubExpr(0))
				{
					reg = s->reg;
					reg->setReference(scope, getSymbols().getFirst());
				}
			}
			else
			{
				auto acg = CREATE_ASM_COMPILER(compiler->getRegisterType(type));

				FunctionData overloadedAssignOp;

				FunctionClass::Ptr fc = type.getComplexType()->getFunctionClass();



				if (getNumChildStatements() > 0)
				{
					overloadedAssignOp = fc->getSpecialFunction(FunctionClass::AssignOverload, type, { type, getSubExpr(0)->getTypeInfo() });
				}

				for (auto s : stackLocations)
				{
					if (initValues == nullptr && overloadedAssignOp.canBeInlined(false))
					{
						AsmInlineData d(acg);

						d.object = s;
						d.target = s;
						d.args.add(s);
						d.args.add(getSubRegister(0));

						auto r = overloadedAssignOp.inlineFunction(&d);

						if (!r.wasOk())
							location.throwError(r.getErrorMessage());
					}
					else
					{
						if (s->getType() == Types::ID::Pointer)
						{
							if (initValues != nullptr)
							{
								auto r = acg.emitStackInitialisation(s, type.getComplexType(), nullptr, initValues);

								location.test(r);
							}
							else if (getSubRegister(0) != nullptr)
								acg.emitComplexTypeCopy(s, getSubRegister(0), type.getComplexType());
						}
						else
						{
							acg.emitSimpleToComplexTypeCopy(s, initValues, getSubExpr(0) != nullptr ? getSubRegister(0) : nullptr);
						}
					}
				}
			}
		}
	}
#endif
}

void Operations::InternalProperty::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
	{
		if (auto cs = findParentStatementOfType<ClassStatement>(this))
		{
			if (auto st = cs->getStructType())
			{
				if (!st->hasInternalProperty(id))
					location.throwError("Internal property not found");

				
			}
		}
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (auto cs = findParentStatementOfType<ClassStatement>(this))
		{
			if (auto st = cs->getStructType())
			{
				if (id == jit::WrapIds::IsNode)
				{
					FunctionClass::Ptr fc = st->getFunctionClass();

					auto hasParameterFunction = fc->hasFunction(st->id.getChildId("setParameter"));

					if (!hasParameterFunction)
					{
						String s;
						s << st->toString() << "::setParameter not defined";
						location.throwError(s);
					}
				}
				if (id == scriptnode::PropertyIds::NodeId)
				{
					if (st->id.getIdentifier( ).toString() != "metadata")
					{
						if (v.toString() != st->id.getIdentifier().toString())
						{
							location.throwError(st->toString() + ": node id mismatch: " + v.toString());
						}
					}
				}
			}
		}
		
	}
}

}
}