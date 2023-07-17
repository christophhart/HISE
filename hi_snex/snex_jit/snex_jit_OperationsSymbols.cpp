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


snex::jit::Operations::Statement::Ptr Operations::InlinedParameter::clone(Location l) const
{
	// This will get resolved to an inlined parameter later again...
	return new VariableReference(l, s);
}

void Operations::InlinedParameter::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		reg = source->reg;
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (source->currentPass != BaseCompiler::CodeGeneration)
		{
			source->process(compiler, scope);
		}

		if (reg == nullptr)
			reg = source->reg;

		if (reg == nullptr)
			location.throwError("Can't find source reg");

		jassert(reg != nullptr);
	}
#endif
}

Operations::TokenType Operations::VariableReference::getWriteAccessType()
{
	if (auto as = dynamic_cast<Assignment*>(parent.get()))
	{
		if (as->getSubExpr(1).get() == this)
			return as->assignmentType;
	}
	else if (auto inc = dynamic_cast<Operations::Increment*>(parent.get()))
		return inc->isDecrement ? JitTokens::minus : JitTokens::plus;

	return JitTokens::void_;
}

void Operations::VariableReference::process(BaseCompiler* compiler, BaseScope* scope)
{
	jassert(parent != nullptr);

	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		if (variableScope != nullptr)
			return;

		if (auto f = getFunctionClassForSymbol(scope))
		{
			id.typeInfo = TypeInfo(Types::ID::Pointer, true);
			return;
		}

		auto nSymbolType = compiler->namespaceHandler.getSymbolType(id.id);

		// Should have been replaced by the resolver...
		jassert(nSymbolType != NamespaceHandler::TemplateConstant);

		if (NamespaceHandler::isConstantSymbol(nSymbolType))
		{
			Ptr n = new Immediate(location, compiler->namespaceHandler.getConstantValue(id.id));

			if (compiler->namespaceHandler.isClassEnumValue(id.id))
			{
				if (auto pCast = dynamic_cast<Cast*>(parent.get()))
				{
					if (pCast->targetType.getType() == Types::ID::Integer)
					{
						pCast->replaceInParent(n);
						return;
					}
				}

				throwError("Can't implicitely cast " + id.id.toString() + " to int");
			}


			replaceInParent(n);
			return;
		}

		if (auto ie = StatementBlock::findInlinedParameterInParentBlocks(this, id))
		{
			auto n = new InlinedParameter(location, ie->getSymbol(), ie->getSubExpr(0));

			replaceInParent(n);
			n->process(compiler, scope);

			return;
		}

		if (auto f = getFunctionClassForParentSymbol(scope))
		{
			if (f->hasConstant(id.id))
			{
				id.constExprValue = f->getConstantValue(id.id);
				variableScope = scope;
				return;
			}
		}

		// walk up the dot operators to get the proper symbol...
		if (auto dp = dynamic_cast<DotOperator*>(parent.get()))
		{
			if (auto st = dp->getDotParent()->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				if (dp->getDotParent().get() == this)
				{
					jassert(id.resolved);
				}
				else
				{
					auto newId = Symbol(st->id.getChildId(id.getName()), st->getMemberTypeInfo(id.getName()));

					// set the type if it's not templated and the existing type is already instantiated...
					if (newId.typeInfo.getTypedIfComplexType<TemplatedComplexType>() == nullptr || !id.resolved)
						id = newId;

					variableScope = scope;
					objectAdress = VariableStorage((int)(st->getMemberOffset(id.id.getIdentifier())));
					objectPtr = st;
					objectExpression = dp->getDotParent();
					return;
				}
			}
			if (auto ss = dynamic_cast<SymbolStatement*>(dp->getDotParent().get()))
			{
				if (compiler->namespaceHandler.isStaticFunctionClass(ss->getSymbol().id))
				{
					if (ss == this)
						return;

					jassert(id.id.isExplicit());
					jassert(!id.resolved);

					auto cId = ss->getSymbol().id.getChildId(id.getName());

					auto fc = scope->getGlobalScope()->getSubFunctionClass(ss->getSymbol().id);

					if (fc == nullptr)
						location.throwError("Can't resolve function class");

					id.constExprValue = fc->getConstantValue(cId);


					// will be replaced with a constant soon...
					return;
				}
			}

		}


		if (!id.resolved)
		{
			if (compiler->namespaceHandler.getSymbolType(id.id) == NamespaceHandler::Unknown)
				throwError("Can't find symbol" + id.toString());

			auto type = compiler->namespaceHandler.getVariableType(id.id);

			if (!type.isDynamic())
			{
				id = Symbol(id.id, type);

				if (!id.resolved)
					throwError("Can't resolve type for symbol" + id.toString());
			}
		}

		jassert(id.resolved || getTypeInfo().isDynamic());

		if (isLocalDefinition)
		{
			variableScope = scope;
		}
		else if (auto vScope = scope->getScopeForSymbol(id.id))
		{
			if (auto fScope = dynamic_cast<FunctionScope*>(vScope))
				parameterIndex = fScope->parameters.indexOf(id.id.getIdentifier());

			variableScope = vScope;
		}
		else if (auto typePtr = compiler->namespaceHandler.getVariableType(id.id).getTypedIfComplexType<ComplexType>())
		{
			objectAdress = VariableStorage(scope->getRootData()->getDataPointer(id.id), (int)typePtr->getRequiredByteSize());
			objectPtr = typePtr;

			id.typeInfo = TypeInfo(typePtr, id.isConst());
		}
		else
			location.throwError("Can't resolve symbol " + id.toString());

		if (auto cScope = dynamic_cast<ClassScope*>(variableScope.get()))
		{
			if (getType() == Types::ID::Dynamic)
				location.throwError("Use of undefined variable " + id.toString());

			if (auto subClassType = dynamic_cast<StructType*>(cScope->typePtr.get()))
			{
				jassert(subClassType->id == id.id.getParent());

				auto memberId = id.id.getIdentifier();

				objectAdress = VariableStorage((int)subClassType->getMemberOffset(memberId));
				objectPtr = subClassType;

				bool hasAlreadyThisPointer = false;
				bool isClassDefinition = dynamic_cast<ClassScope*>(scope) != nullptr;

				if (auto pDot = as<DotOperator>(parent.get()))
				{
					hasAlreadyThisPointer = pDot->getDotChild().get() == this || 
											as<ThisPointer>(pDot->getDotParent()) != nullptr;
				}

				if (!hasAlreadyThisPointer && !isClassDefinition)
				{
					auto tp = new ThisPointer(location, TypeInfo(subClassType, false, true));
					auto dot = new DotOperator(location, tp, this->clone(location));
					auto p = parent.get();

					Ptr keepAlive(this);
					replaceInParent(dot);
					SyntaxTreeInlineData::processUpToCurrentPass(p, dot);
					return;
				}
			}

		}

		jassert(variableScope != nullptr);
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		jassert(variableScope != nullptr);

		if (objectExpression != nullptr && objectExpression->getType() != Types::ID::Pointer)
			objectExpression->location.throwError("expression must have class type");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (isConstExpr())
		{
			Ptr c = new Immediate(location, getConstExprValue());
			replaceInParent(c);
			return;
		}

		// We need to initialise parameter registers before the rest
		if (parameterIndex != -1)
		{
			reg = compiler->registerPool.getRegisterForVariable(scope, id);

			if (isFirstReference())
			{
				if (auto fScope = scope->getParentScopeOfType<FunctionScope>())
				{
					AsmCodeGenerator asg(getFunctionCompiler(compiler), &compiler->registerPool, getType(), location, compiler->getOptimizations());
					asg.emitParameter(dynamic_cast<Function*>(fScope->parentFunction), nullptr, reg, parameterIndex);
				}
			}

			return;
		}
	}


	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (parameterIndex != -1)
			return;

		if (isApiClass(scope))
			return;

		// It might already be assigned to a reused register
		if (reg == nullptr)
		{
			if (objectExpression == nullptr && objectAdress.getType() == Types::ID::Integer)
			{
				if (auto fs = scope->getParentScopeOfType<FunctionScope>())
				{
					if (auto pf = dynamic_cast<Function*>(fs->parentFunction))
					{
						if (pf->objectPtr != nullptr)
						{
							if (auto cs = dynamic_cast<ClassScope*>(variableScope.get()))
							{
								if (cs->typePtr != nullptr && compiler->fitsIntoNativeRegister(cs->typePtr))
								{
									auto ot = pf->objectPtr->getType();
									auto dt = compiler->getRegisterType(TypeInfo(cs->typePtr.get()));
									jassertEqual(ot, dt);
									reg = pf->objectPtr;
									return;
								}
							}

							auto regType = compiler->getRegisterType(getTypeInfo());
							auto acg = CREATE_ASM_COMPILER(regType);

							if (regType == Types::ID::Pointer)
								reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());
							else
								reg = compiler->registerPool.getNextFreeRegister(scope, TypeInfo(regType, true));

							reg->setReference(scope, id);
							acg.emitThisMemberAccess(reg, pf->objectPtr, objectAdress);

							replaceMemoryWithExistingReference(compiler);
							return;

						}
						else
						{
							location.throwError("Can't find this pointer");
						}
					}
				}
			}

			if (!objectAdress.isVoid() || objectExpression != nullptr)
			{
				if (objectExpression != nullptr && compiler->fitsIntoNativeRegister(objectExpression->getTypeInfo().getComplexType().get()))
				{
					auto t = compiler->getRegisterType(getTypeInfo());
					auto ot = compiler->getRegisterType(objectExpression->getTypeInfo());

					jassertEqual(ot, t);

					return;
				}

				// the object address is either the pointer to the object or a offset to the
				// given objectPointer
				jassert((objectAdress.getType() == Types::ID::Pointer && objectExpression == nullptr) ||
					(objectAdress.getType() == Types::ID::Integer && objectExpression != nullptr));

				if (objectPtr == nullptr)
				{
					if (objectExpression == nullptr)
						location.throwError("Can't resolve object pointer");

					objectPtr = objectExpression->getTypeInfo().getComplexType().get();
				}

				auto regType = objectAdress.getType() == Types::ID::Pointer ? TypeInfo(objectPtr.get()) : TypeInfo(Types::ID::Integer);

				auto asg = CREATE_ASM_COMPILER(Types::ID::Pointer);
				reg = compiler->registerPool.getNextFreeRegister(scope, regType);
				reg->setDataPointer(objectAdress.getDataPointer(), true);
				reg->createMemoryLocation(asg.cc);



				return;
			}
			else
			{
				if (variableScope != nullptr)
					reg = compiler->registerPool.getRegisterForVariable(variableScope, id);
				else
					location.throwError("Can't resolve variable " + id.toString());
			}
		}

		if (reg->isActive() && findParentStatementOfType<ConditionalBranch>(this) != nullptr)
		{
			// the code generation has already happened before the branch so that we have the global register
			// available in any case
			return;
		}

		auto asg = CREATE_ASM_COMPILER(getType());

		isFirstOccurence = (!reg->isActive() && !reg->isMemoryLocation()) || isFirstReference();

		if (isFirstOccurence)
		{
			auto assignmentType = getWriteAccessType();
			auto rd = scope->getRootClassScope()->rootData.get();

			if (variableScope->getScopeType() == BaseScope::Class && rd->contains(id.id))
			{
				auto dataPointer = rd->getDataPointer(id.id);

				if (assignmentType != JitTokens::void_)
				{
					reg->setDataPointer(dataPointer, true);

					auto ass = findParentStatementOfType<Assignment>(this);


					if (assignmentType != JitTokens::assign_ || ass == nullptr || ass->loadDataBeforeAssignment() || forceLoadData)
						reg->loadMemoryIntoRegister(asg.cc);
					else
						reg->createRegister(asg.cc);
				}
				else
				{
					reg->setDataPointer(dataPointer, true);
					reg->createMemoryLocation(asg.cc);

					if (reg->getType() == Types::ID::Pointer)
						reg->loadMemoryIntoRegister(asg.cc);
				}
			}
		}
	}
#endif
}

bool Operations::VariableReference::isLastVariableReference() const
{
	SyntaxTreeWalker walker(this);

	auto lastOne = walker.getNextStatementOfType<VariableReference>();;

	bool isLast = lastOne == this;

	while (lastOne != nullptr)
	{
		auto isOtherVariable = lastOne->id != id;

		lastOne = walker.getNextStatementOfType<VariableReference>();

		if (isOtherVariable)
			continue;

		isLast = lastOne == this;
	}

	return isLast;
}

int Operations::VariableReference::getNumWriteAcesses()
{
	int numWriteAccesses = 0;

	SyntaxTreeWalker walker(this);

	while (auto v = walker.getNextStatementOfType<VariableReference>())
	{
		if (v->id == id && v->isBeingWritten())
			numWriteAccesses++;
	}

	return numWriteAccesses;
}

bool Operations::VariableReference::isReferencedOnce() const
{
	SyntaxTreeWalker w(this);

	int numReferences = 0;

	while (auto v = w.getNextStatementOfType<VariableReference>())
	{
		if (v->id == id)
			numReferences++;
	}

	return numReferences == 1;
}

bool Operations::VariableReference::isFirstReference()
{
	SyntaxTreeWalker walker(this);

	while (auto v = walker.getNextStatementOfType<VariableReference>())
	{
		if (v->id == id && v->variableScope == variableScope)
			return v == this;
	}

	jassertfalse;
	return true;
}

bool Operations::VariableReference::validateLocalDefinition(BaseCompiler* compiler, BaseScope* scope)
{
	jassert(isLocalDefinition);

	if (auto es = scope->getScopeForSymbol(id.id))
	{
		bool isAlreadyDefinedSubClassMember = false;

		if (auto cs = dynamic_cast<ClassScope*>(es))
		{
			isAlreadyDefinedSubClassMember = cs->typePtr != nullptr;
		}

		juce::String w;
		w << "declaration of " << id.toString() << " hides ";

		switch (es->getScopeType())
		{
		case BaseScope::Class:  w << "class member"; break;
		case BaseScope::Global: w << "global variable"; break;
		default:			    w << "previous declaration"; break;
		}

		if (!isAlreadyDefinedSubClassMember)
			logWarning(w);
	}

	// The type must have been set or it is a undefined variable
	if (getType() == Types::ID::Dynamic)
		location.throwError("Use of undefined variable " + id.toString());

	return true;
}

void Operations::DotOperator::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	if (getDotChild()->isConstExpr())
	{
		replaceInParent(new Immediate(location, getDotChild()->getConstExprValue()));
		return;
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (auto fc = dynamic_cast<FunctionCall*>(getDotChild().get()))
		{
			jassertfalse;
			bool isPointer = getDotParent()->getType() == Types::ID::Pointer;

			if (!isPointer)
				throwError("Can't call non-object");
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (auto vp = as<SymbolStatement>(getDotChild()))
		{
			if (auto st = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				auto classId = st->id;
				auto childId = vp->getSymbol();

				// If this happens, the inliner shouldn't have replaced the variable
				// with a dot operation...
				jassert(childId.id.getParent() == classId);
			}

			if (compiler->fitsIntoNativeRegister(getSubExpr(0)->getTypeInfo().getComplexType().get()))
				reg = getSubRegister(0);
			else
			{
				reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());
				reg->setReference(scope, vp->getSymbol());

				auto acg = CREATE_ASM_COMPILER(compiler->getRegisterType(getTypeInfo()));

				auto p = getSubRegister(0);
				auto c = getSubRegister(1);

				if (!p->isActive() && !p->isMemoryLocation())
				{
					auto dp = getDotParent();
					location.throwError("dot parent is unresolved");
				}
					

				acg.emitMemberAcess(reg, p, c);

				Expression::replaceMemoryWithExistingReference(compiler);
			}
		}
		else if (auto dp = as<DotOperator>(getDotChild()))
		{
			// Should be resolved already...
			jassert(getSubRegister(1) != nullptr);
			reg = getSubRegister(1);
		}
		else
		{
			auto c = getDotChild();
			jassertfalse;
		}
	}
#endif
}


void Operations::ThisPointer::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto fScope = scope->getParentScopeOfType<FunctionScope>();

		jassert(fScope != nullptr);

		Symbol thisSymbol(NamespacedIdentifier("this"), getTypeInfo());

		if (auto ie = StatementBlock::findInlinedParameterInParentBlocks(this, thisSymbol))
		{
			reg = ie->getSubRegister(0);

			// If this happens, the ThisPointer will most likely point to another object
			if (reg != nullptr && reg->getTypeInfo() != thisSymbol.typeInfo)
				reg = nullptr;
		}

		if (reg == nullptr)
		{
			if (auto f = dynamic_cast<Function*>(fScope->parentFunction))
				reg = f->objectPtr;
		}

		if (reg == nullptr)
			location.throwError("can't resolve this pointer");

		auto objType = compiler->getRegisterType(reg->getTypeInfo());
		auto thisType = compiler->getRegisterType(getTypeInfo());
		jassertEqual(objType, thisType);
	

		// Fix inlining:
		// the scope will fuck up the object pointer of the function
		// check the object pointer from the inlined function (arg is -1)		
	}
#endif
}

void Operations::InlinedArgument::process(BaseCompiler* compiler, BaseScope* scope)
{
	jassert(scope->getScopeType() == BaseScope::Anonymous);

	processBaseWithChildren(compiler, scope);

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (s.typeInfo.isComplexType() && !s.isReference())
		{
			auto acg = CREATE_ASM_COMPILER(getTypeInfo().getType());
			auto stackPtr = acg.cc.newStack((uint32_t)s.typeInfo.getRequiredByteSize(), (uint32_t)s.typeInfo.getRequiredAlignment());
			auto target = compiler->getRegFromPool(scope, s.typeInfo);
			target->setCustomMemoryLocation(stackPtr, false);
			auto source = getSubRegister(0);

			acg.emitComplexTypeCopy(target, source, s.typeInfo.getComplexType());
			getSubExpr(0)->reg = target;

			reg = getSubRegister(0);
		}
	}
#endif
}

void Operations::Immediate::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		// We don't need to use the target register from the 
		// assignment for immediates
		reg = nullptr;

		reg = compiler->getRegFromPool(scope, getTypeInfo());
		reg->setDataPointer(v.getDataPointer(), true);

		reg->createMemoryLocation(getFunctionCompiler(compiler));
	}
#endif
}

void Operations::MemoryReference::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto registerType = compiler->getRegisterType(type);

		ignoreUnused(registerType);

		auto baseReg = getSubRegister(0);

		jassert(baseReg != nullptr);

		reg = compiler->registerPool.getNextFreeRegister(scope, type);

		X86Mem ptr;

		if (baseReg->isMemoryLocation())
			ptr = baseReg->getAsMemoryLocation().cloneAdjustedAndResized(offsetInBytes, 8);
		else if (baseReg->isGlobalVariableRegister())
		{
			auto acg = CREATE_ASM_COMPILER(Types::ID::Pointer);
			auto b_ = acg.cc.newGpq();

			acg.cc.mov(b_, reinterpret_cast<int64_t>(baseReg->getGlobalDataPointer()) + (int64_t)offsetInBytes);

			ptr = x86::qword_ptr(b_);
		}
		else
			ptr = x86::ptr(PTR_REG_W(baseReg)).cloneAdjustedAndResized(offsetInBytes, 8);



		reg->setCustomMemoryLocation(ptr, true);

		reg = compiler->registerPool.getRegisterWithMemory(reg);
	}
#endif
}

}
}