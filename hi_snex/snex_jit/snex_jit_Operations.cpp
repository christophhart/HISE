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



void Operations::Function::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::FunctionParsing)
	{
		functionScope = new FunctionScope(scope, id.id);

		for (int i = 0; i < data.args.size(); i++)
		{
			data.args.getReference(i).id = parameters[i];
		}


		functionScope->data = data;

		functionScope->parameters.addArray(parameters);
		functionScope->parentFunction = dynamic_cast<ReferenceCountedObject*>(this);

		auto classData = new FunctionData(data);

		if (scope->getRootClassScope() == scope)
		{
			functionClass = scope->getRootData();
		}
		else if (auto cs = dynamic_cast<ClassScope*>(scope))
		{
			jassert(cs->typePtr != nullptr);

			functionClass = cs->typePtr->getFunctionClass();

			jassert(functionClass != nullptr);
		}
		else
			location.throwError("Can't define function at this location");


		functionClass->addFunction(classData);

		try
		{
			FunctionParser p(compiler, id, *this);

			BlockParser::ScopedScopeStatementSetter svs(&p, findParentStatementOfType<ScopeStatementBase>(this));

			p.currentScope = functionScope;
			statements = p.parseStatementList();

			auto sTree = dynamic_cast<SyntaxTree*>(statements.get());

			sTree->setReturnType(classData->returnType);

			compiler->executePass(BaseCompiler::PreSymbolOptimization, functionScope, sTree);
			// add this when using stack...
			//compiler->executePass(BaseCompiler::DataSizeCalculation, functionScope, statements);
			compiler->executePass(BaseCompiler::DataAllocation, functionScope, sTree);
			compiler->executePass(BaseCompiler::DataInitialisation, functionScope, sTree);
			
			compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, sTree);
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, sTree);
			compiler->executePass(BaseCompiler::SyntaxSugarReplacements, functionScope, sTree);
			compiler->executePass(BaseCompiler::PostSymbolOptimization, functionScope, sTree);

			compiler->setCurrentPass(BaseCompiler::FunctionParsing);
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			statements = nullptr;
			functionScope = nullptr;

			throw e;
		}
	}

	COMPILER_PASS(BaseCompiler::FunctionCompilation)
	{
		ScopedPointer<asmjit::StringLogger> l = new asmjit::StringLogger();

		auto runtime = getRuntime(compiler);

		ScopedPointer<asmjit::CodeHolder> ch = new asmjit::CodeHolder();
		ch->setLogger(l);
		ch->setErrorHandler(this);
		ch->init(runtime->codeInfo());
		
		//code->setErrorHandler(this);

		ScopedPointer<asmjit::X86Compiler> cc = new asmjit::X86Compiler(ch);

		FuncSignatureX sig;

		hasObjectPtr = scope->getParent()->getScopeType() == BaseScope::Class;

		AsmCodeGenerator::fillSignature(data, sig, hasObjectPtr);
		cc->addFunc(sig);

		dynamic_cast<ClassCompiler*>(compiler)->setFunctionCompiler(cc);

		compiler->registerPool.clear();

		if (hasObjectPtr)
		{
			objectPtr = compiler->registerPool.getNextFreeRegister(functionScope, Types::ID::Pointer);

			auto asg = CREATE_ASM_COMPILER(Types::ID::Pointer);
			asg.emitParameter(this, objectPtr, -1);
		}

		auto sTree = dynamic_cast<SyntaxTree*>(statements.get());

		compiler->executePass(BaseCompiler::PreCodeGenerationOptimization, functionScope, sTree);
		compiler->executePass(BaseCompiler::RegisterAllocation, functionScope, sTree);
		compiler->executePass(BaseCompiler::CodeGeneration, functionScope, sTree);

		cc->endFunc();
		cc->finalize();
		cc = nullptr;

		runtime->add(&data.function, ch);

        jassert(data.function != nullptr);
        

		bool success = functionClass->injectFunctionPointer(data);

		ignoreUnused(success);
		jassert(success);

		auto& as = dynamic_cast<ClassCompiler*>(compiler)->assembly;

		as << "; function " << data.getSignature() << "\n";
		as << l->data();

		ch->setLogger(nullptr);
		l = nullptr;
		ch = nullptr;

		jassert(scope->getScopeType() == BaseScope::Class);

		if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
		{
			st->addJitCompiledMemberFunction(data);
		}

		compiler->setCurrentPass(BaseCompiler::FunctionCompilation);
	}
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

	Expression::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		if (variableScope != nullptr)
			return;

		if (auto f = getFunctionClassForSymbol(scope))
		{
			id.typeInfo = TypeInfo(Types::ID::Pointer, true);
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
			if (f->hasConstant(id))
			{
				id.constExprValue = f->getConstantValue(id);
				variableScope = scope->getRootClassScope();
				return;
			}
		}

		// walk up the dot operators to get the proper symbol...
		if (auto dp = dynamic_cast<DotOperator*>(parent.get()))
		{
			if (auto sType = dp->getDotParent()->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				if (dp->getDotParent().get() != this)
				{
					objectPointer = dp->getDotParent();

					if (objectPointer.get() == this)
						jassertfalse;

					id.typeInfo = sType->getMemberTypeInfo(id.id);
					auto byteSize = id.typeInfo.getRequiredByteSize();

					if (byteSize == 0)
						location.throwError("Can't deduce type size");

					objectAdress = VariableStorage((int)(sType->getMemberOffset(id.id)));
					return;
				}
			}
		}

		if (isLocalDefinition)
		{
			variableScope = scope;

			if (!variableScope->updateSymbol(id))
				location.throwError("Can't update symbol" + id.toString());
		}
		else if (auto vScope = scope->getScopeForSymbol(id))
		{
			if (!vScope->updateSymbol(id))
				location.throwError("Can't update symbol " + id.toString());

			if (auto fScope = dynamic_cast<FunctionScope*>(vScope))
				parameterIndex = fScope->parameters.indexOf(id.id);

			variableScope = vScope;
		}
		else if (auto typePtr = scope->getRootData()->getComplexTypeForVariable(id))
		{
			objectAdress = VariableStorage(scope->getRootData()->getDataPointer(id), typePtr->getRequiredByteSize());

			id.typeInfo = TypeInfo(typePtr, id.isConst());
		}
		else
			location.throwError("Can't resolve symbol " + id.toString());

		if (auto cScope = dynamic_cast<ClassScope*>(variableScope.get()))
		{
			if (getType() == Types::ID::Dynamic)
				location.throwError("Use of undefined variable " + id.toString());

			if (auto subClassType = dynamic_cast<StructType*>(cScope->typePtr.get()))
				objectAdress = VariableStorage((int)subClassType->getMemberOffset(id.id));
		}
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (objectPointer != nullptr && objectPointer->getType() != Types::ID::Pointer)
			objectPointer->location.throwError("expression must have class type");

		
	}

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
					AsmCodeGenerator asg(getFunctionCompiler(compiler), &compiler->registerPool, getType());
					asg.emitParameter(dynamic_cast<Function*>(fScope->parentFunction), reg, parameterIndex);
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
			if (objectPointer == nullptr && objectAdress.getType() == Types::ID::Integer)
			{
				if (auto fs = scope->getParentScopeOfType<FunctionScope>())
				{
					if (auto pf = dynamic_cast<Function*>(fs->parentFunction))
					{
						reg = compiler->registerPool.getNextFreeRegister(scope, getType());
						reg->setReference(scope, id);
						auto acg = CREATE_ASM_COMPILER(getType());
						acg.emitThisMemberAccess(reg, pf->objectPtr, objectAdress);
						return;
					}
				}
			}

			if (!objectAdress.isVoid() || objectPointer != nullptr)
			{
				// the object address is either the pointer to the object or a offset to the
				// given objectPointer
				jassert((objectAdress.getType() == Types::ID::Pointer && objectPointer == nullptr) || 
					    (objectAdress.getType() == Types::ID::Integer && objectPointer != nullptr));

				auto asg = CREATE_ASM_COMPILER(Types::ID::Pointer);
				reg = compiler->registerPool.getNextFreeRegister(scope, objectAdress.getType());
				reg->setDataPointer(objectAdress.getDataPointer());
				reg->createMemoryLocation(asg.cc);
				return;
			}
			else
			{
				jassert(variableScope != nullptr);
				reg = compiler->registerPool.getRegisterForVariable(variableScope, id);
			}
		}
			
		if (reg->isActiveOrDirtyGlobalRegister() && findParentStatementOfType<ConditionalBranch>(this) != nullptr)
		{
			// the code generation has already happened before the branch so that we have the global register
			// available in any case
			return;
		}

		if (reg->isIteratorRegister())
			return;

		auto asg = CREATE_ASM_COMPILER(getType());

		isFirstOccurence = (!reg->isActiveOrDirtyGlobalRegister() && !reg->isMemoryLocation()) || isFirstReference();

		if (isFirstOccurence)
		{
			auto assignmentType = getWriteAccessType();
			auto rd = scope->getRootClassScope()->rootData.get();

			if (variableScope->getScopeType() == BaseScope::Class && rd->contains(id))
			{
				auto dataPointer = rd->getDataPointer(id);

				if (assignmentType != JitTokens::void_)
				{
					if (assignmentType != JitTokens::assign_)
					{
						reg->setDataPointer(dataPointer);
						reg->loadMemoryIntoRegister(asg.cc);
					}
					else
						reg->createRegister(asg.cc);
				}
				else
				{
					reg->setDataPointer(dataPointer);
					reg->createMemoryLocation(asg.cc);

					if (!isReferencedOnce())
						reg->loadMemoryIntoRegister(asg.cc);
				}
			}
		}
	}
}

bool Operations::VariableReference::isApiClass(BaseScope* s) const
{
	if (auto fc = getFunctionClassForSymbol(s))
	{
		return dynamic_cast<ComplexType*>(fc) == nullptr;
	}
    
    return false;
}

void Operations::Assignment::process(BaseCompiler* compiler, BaseScope* scope)
{
	/*
	ResolvingSymbols: check that target is not const
	TypeCheck, = > check type match
	DeadCodeElimination, = > remove unreferenced local variables
	Inlining, = > make self assignment
	CodeGeneration, = > Store or Op
	*/

	

	/** Assignments might use the target register OR have the same symbol from another scope
	    so we need to customize the execution order in these passes... */
	if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration ||
		compiler->getCurrentPass() == BaseCompiler::DataAllocation)
	{
		Statement::process(compiler, scope);
	}
	else
	{
		Expression::process(compiler, scope);
	}

	auto e = getSubExpr(0);



	COMPILER_PASS(BaseCompiler::DataSizeCalculation)
	{
		if (getTargetType() == TargetType::Variable && isFirstAssignment && scope->getRootClassScope() == scope)
		{
			auto typeToAllocate = getTargetVariable()->getTypeInfo();

			if (typeToAllocate.isInvalid())
			{
				typeToAllocate = getSubExpr(0)->getTypeInfo();

				if (typeToAllocate.isInvalid())
					location.throwError("Can't deduce type");

				getTargetVariable()->id.typeInfo = typeToAllocate;
			}

			scope->getRootData()->enlargeAllocatedSize(getTargetVariable()->getTypeInfo());
		}
	}

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		getSubExpr(0)->process(compiler, scope);

		auto targetType = getTargetType();

		if ((targetType == TargetType::Variable || targetType == TargetType::Reference) && isFirstAssignment)
		{
			auto type = getTargetVariable()->getType();

			if (!Types::Helpers::isFixedType(type))
			{
				type = getSubExpr(0)->getType();

				if (!Types::Helpers::isFixedType(type))
				{
					BaseCompiler::ScopedPassSwitcher rs(compiler, BaseCompiler::ResolvingSymbols);
					getSubExpr(0)->process(compiler, scope);

					BaseCompiler::ScopedPassSwitcher tc(compiler, BaseCompiler::TypeCheck);
					getSubExpr(0)->process(compiler, scope);

					type = getSubExpr(0)->getType();

					if (!Types::Helpers::isFixedType(type))
						location.throwError("Can't deduce auto type");
				}

				getTargetVariable()->id.typeInfo.setType(type);
			}

			getTargetVariable()->isLocalDefinition = true;
			scope->addVariable(getTargetVariable()->id);
		}

		getSubExpr(1)->process(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::DataInitialisation)
	{
		if (isFirstAssignment)
			initClassMembers(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		switch (getTargetType())
		{
		case TargetType::Variable:
		{
			auto e = getSubExpr(1);
			auto v = getTargetVariable();

			if (v->id.isConst() && !isFirstAssignment)
				throwError("Can't change constant variable");
		}
		case TargetType::Reference:
		{
			break;
		}
		case TargetType::ClassMember:
		{
			//...
			break;
		}
		case TargetType::Span:
		{
			// nothing to do...
			break;
		}
		}
	}


	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto targetIsSimd = SpanType::isSimdType(getSubExpr(1)->getTypeInfo());

		if (targetIsSimd)
		{
			auto valueIsSimd = SpanType::isSimdType(getSubExpr(0)->getTypeInfo());

			if (!valueIsSimd)
				setTypeForChild(0, TypeInfo(Types::ID::Float));
		}
		else
		{
			if (auto ct = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<ComplexType>())
			{
				FunctionClass::Ptr fc = ct->getFunctionClass();

				if (fc != nullptr && fc->hasSpecialFunction(FunctionClass::AssignOverload))
				{
					Array<TypeInfo> argTypes;

					argTypes.add(getSubExpr(1)->getTypeInfo());
					argTypes.add(getSubExpr(0)->getTypeInfo());

					Array<FunctionData> matches;

					fc->addSpecialFunctions(FunctionClass::AssignOverload, matches);

					for (auto& m : matches)
					{
						if (m.matchesArgumentTypes(getSubExpr(1)->getTypeInfo(), argTypes))
						{
							overloadedAssignOperator = m;
							return;
						}
					}
				}
			}



			checkAndSetType(0, getSubExpr(1)->getTypeInfo());
		}
		
		
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto acg = CREATE_ASM_COMPILER(getType());

		getSubExpr(0)->process(compiler, scope);
		getSubExpr(1)->process(compiler, scope);

		auto value = getSubRegister(0);
		auto tReg = getSubRegister(1);

		if (overloadedAssignOperator)
		{
			AssemblyRegister::List l;
			l.add(tReg);
			l.add(value);

			acg.emitFunctionCall(tReg, overloadedAssignOperator, tReg, l);
			return;
		}

		// jassertfalse;
		// here you should do something with the reference...

		

		if (auto dt = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<DynType>())
		{
			acg.emitStackInitialisation(tReg, dt, value, nullptr);
			return;
		}

		if (getTargetType() == TargetType::Reference && isFirstAssignment)
		{
			jassert(value->hasCustomMemoryLocation() || value->isMemoryLocation());
			tReg->setCustomMemoryLocation(value->getMemoryLocationForReference());
		}
		else
		{
			if (assignmentType == JitTokens::assign_)
			{
				if (tReg != value)
					acg.emitStore(tReg, value);
			}
			else
				acg.emitBinaryOp(assignmentType, tReg, value);


			if (auto wt = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<WrapType>())
			{
				jassertfalse;
				acg.emitWrap(wt, tReg, WrapType::OpType::Set);
			}
		}
	}
}

void Operations::Assignment::initClassMembers(BaseCompiler* compiler, BaseScope* scope)
{
	if (getSubExpr(0)->isConstExpr() && scope->getScopeType() == BaseScope::Class)
	{
		auto target = getTargetVariable()->id;
		auto initValue = getSubExpr(0)->getConstExprValue();

		if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
		{
			auto ok = st->setDefaultValue(target.id, InitialiserList::makeSingleList(initValue));

			if (!ok)
				throwError("Can't initialise default value");
		}
		else
		{
			// this will initialise the class members to constant values...
			auto rd = scope->getRootClassScope()->rootData.get();

			auto ok = rd->initData(scope, target, InitialiserList::makeSingleList(initValue));

			if (!ok.wasOk())
				location.throwError(ok.getErrorMessage());
		}
	}
}

Operations::Assignment::Assignment(Location l, Expression::Ptr target, TokenType assignmentType_, Expression::Ptr expr, bool firstAssignment_) :
	Expression(l),
	assignmentType(assignmentType_),
	isFirstAssignment(firstAssignment_)
{
	addStatement(expr);
	addStatement(target); // the target must be evaluated after the expression
}

Operations::Assignment::TargetType Operations::Assignment::getTargetType() const
{
	auto target = getSubExpr(1);

	if (auto v = dynamic_cast<VariableReference*>(target.get()))
	{
		return v->id.isReference() ? TargetType::Reference : TargetType::Variable;
	}
	else if (dynamic_cast<DotOperator*>(target.get()))
		return TargetType::ClassMember;
	else if (dynamic_cast<Subscript*>(target.get()))
		return TargetType::Span;

	jassertfalse;
	return TargetType::numTargetTypes;
}

void Operations::DotOperator::process(BaseCompiler* compiler, BaseScope* scope)
{
	Expression::process(compiler, scope);

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

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (auto vp = dynamic_cast<SymbolStatement*>(getDotChild().get()))
		{
			reg = compiler->registerPool.getNextFreeRegister(scope, getType());
			reg->setReference(scope, vp->getSymbol());
			
			auto acg = CREATE_ASM_COMPILER(getType());

			auto p = getSubRegister(0);
			auto c = getSubRegister(1);

			acg.emitMemberAcess(reg, p, c);
		}
		else
		{
			jassertfalse;

			// just steal the register from the child...
			reg = getSubRegister(1);
			return;
		}
	}
}

void Operations::FunctionCall::process(BaseCompiler* compiler, BaseScope* scope)
{
	Expression::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		if (objExpr != nullptr && function.id == Identifier("toSimd"))
		{
			auto sc = new CastedSimd(location, objExpr);

			replaceInParent(sc);
			sc->process(compiler, scope);
			return;
		}
	}

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		if (callType != Unresolved)
			return;

		if (objExpr == nullptr)
		{
			// Functions without parent

			auto id = scope->getRootData()->getClassName().getChildSymbol(function.id);

			if (scope->getRootData()->hasFunction(id))
			{
				fc = scope->getRootData();
				fc->addMatchingFunctions(possibleMatches, id);
				callType = RootFunction;
				return;

			}
			else if (scope->getGlobalScope()->hasFunction(id))
			{
				fc = scope->getGlobalScope();
				fc->addMatchingFunctions(possibleMatches, id);
				callType = GlobalFunction;
				return;
			}
		}

		if (objExpr->getTypeInfo().isComplexType())
		{
			if (fc = objExpr->getTypeInfo().getComplexType()->getFunctionClass())
			{
				auto id = fc->getClassName().getChildSymbol(function.id);
				fc->addMatchingFunctions(possibleMatches, id);
				callType = MemberFunction;
				return;
			}
		}

		if (auto pv = dynamic_cast<VariableReference*>(objExpr.get()))
		{
			auto pType = pv->getTypeInfo();
			auto pNativeType = pType.getType();

			if (pNativeType == Types::ID::Event || pNativeType == Types::ID::Block)
			{
				// substitute the parent id with the Message or Block API class to resolve the pointers
				auto id = Symbol::createRootSymbol(pNativeType == Types::ID::Event ? "Message" : "Block").getChildSymbol(function.id, pType);

				fc = scope->getRootData()->getSubFunctionClass(id.getParentSymbol());
				fc->addMatchingFunctions(possibleMatches, id);
				callType = NativeTypeCall;
				return;
			}

			if (fc = scope->getRootData()->getSubFunctionClass(pv->id))
			{
				// Function with registered parent object (either API class or JIT callable object)

				auto id = fc->getClassName().getChildSymbol(function.id);
				fc->addMatchingFunctions(possibleMatches, id);

				callType = pv->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				return;
			}

			else if (auto st = pv->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				if (function.id == Identifier("size"))
				{
					replaceInParent(new Immediate(location, VariableStorage((int)st->getNumElements())));
					return;
				}
			}
			if (scope->getGlobalScope()->hasFunction(pv->id))
			{
				// Function with globally registered object (either API class or JIT callable object)
				fc = scope->getGlobalScope()->getGlobalFunctionClass(pv->id.id);

				auto id = fc->getClassName().getChildSymbol(function.id);
				fc->addMatchingFunctions(possibleMatches, id);

				callType = pv->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				return;
			}
		}

		location.throwError("Can't resolve function call " + function.getSignature());
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		Array<TypeInfo> parameterTypes;

		for (int i = 0; i < getNumArguments(); i++)
			parameterTypes.add(getArgument(i)->getTypeInfo());

		for (auto& f : possibleMatches)
		{
			jassert(function.id == f.id);

			if (f.matchesArgumentTypes(parameterTypes))
			{
				int numArgs = f.args.size();

				for (int i = 0; i < numArgs; i++)
				{
					if (f.args[i].isReference())
					{
						if (!canBeAliasParameter(getArgument(i)))
						{
							throwError("Can't use rvalues for reference parameters");
						}
					}
				}

				function = f;
				return;
			}
		}

		throwError("Wrong argument types for function call");
	}

	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		reg = compiler->getRegFromPool(scope, getType());

		//VariableReference::reuseAllLastReferences(this);

		if (shouldInlineFunctionCall(compiler, scope))
		{
			return;
		}
		else
		{
			for (int i = 0; i < getNumArguments(); i++)
			{
				if (auto subReg = getSubRegister(i))
				{
					if (!subReg->getVariableId())
					{
						parameterRegs.add(subReg);
						continue;
					}
				}

				auto pType = function.args[i].isReference() ? Types::ID::Pointer : getArgument(i)->getType();
				auto pReg = compiler->getRegFromPool(scope, pType);
				auto asg = CREATE_ASM_COMPILER(getType());
				pReg->createRegister(asg.cc);
				parameterRegs.add(pReg);
			}
		}
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto asg = CREATE_ASM_COMPILER(getType());

		if (shouldInlineFunctionCall(compiler, scope))
		{
			InlineData d(asg);
			d.target = reg;
			d.object = objExpr != nullptr ? objExpr->reg : nullptr;

			for (int i = 0; i < getNumArguments(); i++)
				d.args.add(getArgument(i)->reg);

			auto r = function.inliner->f(&d);

			if (!r.wasOk())
				throwError(r.getErrorMessage());

			return;
		}

		if (!function)
		{
			if (fc == nullptr)
				throwError("Can't resolve function class");

			if (!fc->fillJitFunctionPointer(function))
				throwError("Can't find function pointer to JIT function " + function.functionName);
		}

		if (function.id.toString() == "stop")
		{
			asg.dumpVariables(scope, location.getLine());

			function.functionName = "";
			function.functionName << "Line " << juce::String(location.getLine()) << " Breakpoint";
		}
		else
		{
			for (auto dv : compiler->registerPool.getListOfAllDirtyGlobals())
			{
				auto asg = CREATE_ASM_COMPILER(dv->getType());
				asg.emitMemoryWrite(dv);
			}
		}

		VariableReference::reuseAllLastReferences(this);

		for (int i = 0; i < parameterRegs.size(); i++)
		{
			auto arg = getArgument(i);
			auto existingReg = arg->reg;
			auto pReg = parameterRegs[i];
			auto acg = CREATE_ASM_COMPILER(arg->getTypeInfo().getType());

			if (function.args[i].isReference() && function.args[i].typeInfo.getType() != Types::ID::Pointer)
			{
				acg.emitComment("arg reference -> stack");
				acg.emitFunctionParameterReference(existingReg, pReg);
			}
			else
			{
				if (existingReg != nullptr && existingReg != pReg && existingReg->getVariableId())
				{
					acg.emitComment("Parameter Save");
					acg.emitStore(pReg, existingReg);
				}
				else
					parameterRegs.set(i, existingReg);
			}
		}

		if (function.functionName.isEmpty())
			function.functionName = function.getSignature({});

		asg.emitFunctionCall(reg, function, objExpr != nullptr ? objExpr->reg : nullptr, parameterRegs);

		for (int i = 0; i < parameterRegs.size(); i++)
		{
			if (!function.args[i].isReference())
				parameterRegs[i]->flagForReuse();
		}
	}
}

bool Operations::FunctionCall::shouldInlineFunctionCall(BaseCompiler* compiler, BaseScope* scope) const
{
	if (function.inliner == nullptr)
		return false;

	return scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::Inlining);
}

bool Operations::StatementBlock::isRealStatement(Statement* s)
{
	if (dynamic_cast<InlinedArgument*>(s) != nullptr)
		return false;

	if (dynamic_cast<Noop*>(s) != nullptr)
		return false;

	if (dynamic_cast<VariableReference*>(s) != nullptr)
		return false;

	return true;
}

snex::jit::Operations::Statement::Ptr Operations::InlinedParameter::clone(Location l) const
{
	// This will get resolved to an inlined parameter later again...
	return new VariableReference(l, s);
}

}
}
