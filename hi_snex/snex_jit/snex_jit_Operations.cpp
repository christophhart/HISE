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
	processBaseWithoutChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::FunctionTemplateParsing)
	{
		functionScope = new FunctionScope(scope, data.id);

		{
			NamespaceHandler::ScopedNamespaceSetter(compiler->namespaceHandler, data.id);

			for (int i = 0; i < data.args.size(); i++)
				data.args.getReference(i).id = data.id.getChildId(parameters[i]);
		}

		functionScope->data = data;

		functionScope->parameters.addArray(parameters);
		functionScope->parentFunction = dynamic_cast<ReferenceCountedObject*>(this);

		classData = new FunctionData(data);

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
			FunctionParser p(compiler, *this);

			auto ssb = findParentStatementOfType<ScopeStatementBase>(this);

			BlockParser::ScopedScopeStatementSetter svs(&p, ssb);

			p.currentScope = functionScope;

			{
				NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, data.id);

				auto fNamespace = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

				for (auto arg : classData->args)
				{
					compiler->namespaceHandler.addSymbol(fNamespace.getChildId(arg.id.id), arg.typeInfo, NamespaceHandler::Variable);
				}

				statements = p.parseStatementList();
			}
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			statements = nullptr;
			functionScope = nullptr;

			throw e;
		}

		if (!data.templateParameters.isEmpty())
		{
			TemplateParameterResolver resolver(collectParametersFromParentClass(this,  data.templateParameters));
			resolver.process(statements);
		}
	}

	COMPILER_PASS(BaseCompiler::FunctionParsing)
	{
		try 
		{
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

			Statement::Ptr statementCopy = statements;

			classData->templateParameters = data.templateParameters;

			auto classDataCopy = FunctionData(*classData);

			auto createInliner = scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::Inlining);

			if (createInliner)
			{
				classData->inliner = Inliner::createHighLevelInliner(data.id, [statementCopy, classDataCopy](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					auto fc = dynamic_cast<Operations::FunctionCall*>(d->expression.get());
					jassert(fc != nullptr);
					d->target = statementCopy->clone(d->location);
					auto cs = dynamic_cast<Operations::StatementBlock*>(d->target.get());
					cs->setReturnType(classDataCopy.returnType);

					if (d->object != nullptr)
					{
						auto thisSymbol = Symbol("this");
						auto e = d->object->clone(d->location);
						cs->addInlinedParameter(-1, thisSymbol, dynamic_cast<Operations::Expression*>(e.get()));

						if (auto st = e->getTypeInfo().getTypedIfComplexType<StructType>())
						{
							if (!as<ThisPointer>(e))
							{
								d->target->forEachRecursive([st, e](Operations::Statement::Ptr p)
								{
									if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
									{
										auto canBeMember = st->id == v->id.id.getParent();
										auto hasMember = canBeMember && st->hasMember(v->id.id.getIdentifier());

										if (hasMember)
										{
											auto newParent = e->clone(v->location);
											auto newChild = v->clone(v->location);

											auto newDot = new Operations::DotOperator(v->location,
												dynamic_cast<Operations::Expression*>(newParent.get()),
												dynamic_cast<Operations::Expression*>(newChild.get()));

											v->replaceInParent(newDot);
										}
									}

									return false;
								});
							}
						}
					}

					for (int i = 0; i < fc->getNumArguments(); i++)
					{
						auto pVarSymbol = classDataCopy.args[i];

						Operations::Expression::Ptr e = dynamic_cast<Operations::Expression*>(fc->getArgument(i)->clone(fc->location).get());

						cs->addInlinedParameter(i, pVarSymbol, e);
					}

					return Result::ok();
				});
			}

			if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
			{
				st->addJitCompiledMemberFunction(*classData);
			}
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
		if (data.function != nullptr)
		{
			// this function is already compiled (maybe because of reordering
			// of variadic function calls)
			return;
		}
			

		ScopedPointer<asmjit::StringLogger> l = new asmjit::StringLogger();

		auto runtime = getRuntime(compiler);

		ScopedPointer<asmjit::CodeHolder> ch = new asmjit::CodeHolder();
		ch->setLogger(l);
		ch->setErrorHandler(this);
		ch->init(runtime->codeInfo());
		
		//code->setErrorHandler(this);

		ScopedPointer<asmjit::X86Compiler> cc = new asmjit::X86Compiler(ch);

		ScopedPointer<AsmCleanupPass> p = new AsmCleanupPass();

		if (scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::AsmOptimisation))
			cc->addPass(p.get());
		else
			p = nullptr;

		FuncSignatureX sig;

		hasObjectPtr = scope->getParent()->getScopeType() == BaseScope::Class && !classData->returnType.isStatic();

		auto objectType = hasObjectPtr ? compiler->getRegisterType(TypeInfo(dynamic_cast<ClassScope*>(scope)->typePtr.get())) : Types::ID::Void;

		AsmCodeGenerator::fillSignature(data, sig, objectType);
		cc->addFunc(sig);

		dynamic_cast<ClassCompiler*>(compiler)->setFunctionCompiler(cc);

		compiler->registerPool.clear();

		if (hasObjectPtr)
		{
			auto rType = compiler->getRegisterType(TypeInfo(dynamic_cast<ClassScope*>(scope)->typePtr.get()));
			objectPtr = compiler->registerPool.getNextFreeRegister(functionScope, TypeInfo(rType, true));
			auto asg = CREATE_ASM_COMPILER(rType);
			asg.emitParameter(this, objectPtr, -1);
		}

		auto sTree = dynamic_cast<SyntaxTree*>(statements.get());

		compiler->executePass(BaseCompiler::PreCodeGenerationOptimization, functionScope, sTree);
		compiler->executePass(BaseCompiler::RegisterAllocation, functionScope, sTree);
		compiler->executePass(BaseCompiler::CodeGeneration, functionScope, sTree);

		cc->endFunc();
		cc->finalize();

		if (p != nullptr)
			cc->deletePass(p);

		cc = nullptr;

		runtime->add(&data.function, ch);

        jassert(data.function != nullptr);
        

		bool success = functionClass->injectFunctionPointer(data);

		ignoreUnused(success);
		jassert(success);

		auto& as = dynamic_cast<ClassCompiler*>(compiler)->assembly;

		juce::String fName = data.getSignature();

		if (auto cs = findParentStatementOfType<ClassStatement>(this))
		{
			if (auto st = cs->getStructType())
			{
				auto name = st->id.toString();
				auto templated = st->toString();

				fName = fName.replace(name, templated);
			}
		}

		as << "; function " << fName << "\n";
		as << l->data();



		ch->setLogger(nullptr);
		l = nullptr;
		ch = nullptr;

		jassert(scope->getScopeType() == BaseScope::Class);

		if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
		{
			if (!st->injectMemberFunctionPointer(data, data.function))
				location.throwError("Can't inject function pointer to member function");
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
					//jassert(!id.resolved);

					id = Symbol(st->id.getChildId(id.getName()), st->getMemberTypeInfo(id.getName()));
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

			id = Symbol(id.id, type);

			if(!id.resolved)
				throwError("Can't find symbol" + id.toString());
		}

		jassert(id.resolved);

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
			objectAdress = VariableStorage(scope->getRootData()->getDataPointer(id.id), typePtr->getRequiredByteSize());
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
			if (objectExpression == nullptr && objectAdress.getType() == Types::ID::Integer)
			{
				if (auto fs = scope->getParentScopeOfType<FunctionScope>())
				{
					if (auto pf = dynamic_cast<Function*>(fs->parentFunction))
					{
						if (auto cs = dynamic_cast<ClassScope*>(variableScope.get()))
						{
							if (cs->typePtr != nullptr && compiler->fitsIntoNativeRegister(cs->typePtr))
							{
								auto ot = pf->objectPtr->getType();
								auto dt = compiler->getRegisterType(TypeInfo(cs->typePtr.get()));
								jassert(ot == dt);
								reg = pf->objectPtr;
								return;
							}
						}

						auto regType = compiler->getRegisterType(getTypeInfo());

						if(regType == Types::ID::Pointer)
							reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());
						else
							reg = compiler->registerPool.getNextFreeRegister(scope, TypeInfo(regType, true));

						
						reg->setReference(scope, id);
						auto acg = CREATE_ASM_COMPILER(regType);

						

						acg.emitThisMemberAccess(reg, pf->objectPtr, objectAdress);
						return;
					}
				}
			}

			if (!objectAdress.isVoid() || objectExpression != nullptr)
			{
				if (objectExpression != nullptr && compiler->fitsIntoNativeRegister(objectExpression->getTypeInfo().getComplexType()))
				{
					auto t = compiler->getRegisterType(getTypeInfo());
					auto ot = compiler->getRegisterType(objectExpression->getTypeInfo());

					jassert(ot == t);

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

					objectPtr = objectExpression->getTypeInfo().getComplexType();
				}
					
				auto regType = objectAdress.getType() == Types::ID::Pointer ? TypeInfo(objectPtr.get()) : TypeInfo(Types::ID::Integer);

				auto asg = CREATE_ASM_COMPILER(Types::ID::Pointer);
				reg = compiler->registerPool.getNextFreeRegister(scope, regType);
				reg->setDataPointer(objectAdress.getDataPointer());
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

			if (variableScope->getScopeType() == BaseScope::Class && rd->contains(id.id))
			{
				auto dataPointer = rd->getDataPointer(id.id);

				if (assignmentType != JitTokens::void_)
				{
					reg->setDataPointer(dataPointer);

                    auto ass = findParentStatementOfType<Assignment>(this);
                    
                    
                    if (assignmentType != JitTokens::assign_ || ass == nullptr || ass->loadDataBeforeAssignment())
						reg->loadMemoryIntoRegister(asg.cc);
					else
						reg->createRegister(asg.cc);
				}
				else
				{
					reg->setDataPointer(dataPointer);
					reg->createMemoryLocation(asg.cc);

                    if(reg->getType() == Types::ID::Pointer)
                        reg->loadMemoryIntoRegister(asg.cc);
                    
                    
#if 0
					if (!isReferencedOnce())
						reg->loadMemoryIntoRegister(asg.cc);
#endif
				}
			}
		}
	}
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
		processBaseWithoutChildren(compiler, scope);
	}
	else
	{
		processBaseWithChildren(compiler, scope);
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

			if (scope->getRootClassScope() == scope)
			{
				scope->getRootData()->allocate(scope, getTargetVariable()->id);
			}
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
		if (auto dot = dynamic_cast<DotOperator*>(getSubExpr(1).get()))
		{
			jassert(getTargetType() == TargetType::ClassMember);

			if (dot->getSubExpr(0)->getTypeInfo().isConst())
				location.throwError("Can't modify const object");
		}

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
				if (FunctionClass::Ptr fc = ct->getFunctionClass())
				{
					auto targetType = getSubExpr(1)->getTypeInfo();
					
					TypeInfo::List args = { targetType, getSubExpr(0)->getTypeInfo() };

					overloadedAssignOperator = fc->getSpecialFunction(FunctionClass::AssignOverload, targetType, args);

					if (overloadedAssignOperator.isResolved())
						return;
				}
					
			}

			checkAndSetType(0, getSubExpr(1)->getTypeInfo());
		}
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		

		getSubExpr(0)->process(compiler, scope);
		getSubExpr(1)->process(compiler, scope);

		auto value = getSubRegister(0);
		auto tReg = getSubRegister(1);

		auto acg = CREATE_ASM_COMPILER(tReg->getType());

		if (overloadedAssignOperator.isResolved())
		{
			AssemblyRegister::List l;
			l.add(tReg);
			l.add(value);

			auto r = acg.emitFunctionCall(tReg, overloadedAssignOperator, tReg, l);

			if (!r.wasOk())
				location.throwError(r.getErrorMessage());

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
			if (value->getTypeInfo().isNativePointer())
			{
				auto ptr = x86::ptr(PTR_REG_R(value));
				tReg->setCustomMemoryLocation(ptr, value->isGlobalMemory());
				return;
			}
			else if (!(value->hasCustomMemoryLocation() || value->isMemoryLocation()))
			{
				location.throwError("Can't create reference to rvalue");
			}

			tReg->setCustomMemoryLocation(value->getMemoryLocationForReference(), true);
		}
		else
		{
			if (assignmentType == JitTokens::assign_)
			{
				if (tReg != value)
					acg.emitStore(tReg, value);
			}
			else
			{
				acg.emitBinaryOp(assignmentType, tReg, value);

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
			auto ok = st->setDefaultValue(target.id.getIdentifier(), InitialiserList::makeSingleList(initValue));

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

	if (auto v = dynamic_cast<SymbolStatement*>(target.get()))
	{
		return v->getSymbol().isReference() ? TargetType::Reference : TargetType::Variable;
	}
	else if (dynamic_cast<DotOperator*>(target.get()))
		return TargetType::ClassMember;
	else if (dynamic_cast<Subscript*>(target.get()))
		return TargetType::Span;
	else if (dynamic_cast<MemoryReference*>(target.get()))
		return TargetType::Reference;

	getSubExpr(1)->throwError("Can't assign to target");

	jassertfalse;
	return TargetType::numTargetTypes;
}

void Operations::DotOperator::process(BaseCompiler* compiler, BaseScope* scope)
{
	processChildrenIfNotCodeGen(compiler, scope);

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

	if(isCodeGenPass(compiler))
	{
		auto abortFunction = []()
		{

			return false;
		};

		if (!Expression::preprocessCodeGenForChildStatements(compiler, scope, abortFunction))
			return;

		if (auto vp = dynamic_cast<SymbolStatement*>(getDotChild().get()))
		{
			if (auto st = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				auto classId = st->id;
				auto childId = vp->getSymbol();

				// If this happens, the inliner shouldn't have replaced the variable
				// with a dot operation...
 				jassert(childId.id.getParent() == classId);
			}

			if (compiler->fitsIntoNativeRegister(getSubExpr(0)->getTypeInfo().getComplexType()))
				reg = getSubRegister(0);
			else
			{
				reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());
				reg->setReference(scope, vp->getSymbol());

				auto acg = CREATE_ASM_COMPILER(compiler->getRegisterType(getTypeInfo()));

				auto p = getSubRegister(0);
				auto c = getSubRegister(1);

				acg.emitMemberAcess(reg, p, c);

				Expression::replaceMemoryWithExistingReference(compiler);
			}

			
			
		}
		else
		{
			auto c = getDotChild();
			jassertfalse;
		}
	}
}

void Operations::FunctionCall::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		tryToResolveType(compiler);

		if (callType != Unresolved)
			return;

		if (!hasObjectExpression)
		{
			// Functions without parent

			auto id = scope->getRootData()->getClassName().getChildId(function.id.getIdentifier());

			if (auto nfc = compiler->getInbuiltFunctionClass())
			{
				if (nfc->hasFunction(function.id))
				{
					callType = InbuiltFunction;
					fc = compiler->getInbuiltFunctionClass();
					jassert(function.isResolved());
					return;
				}
			}
			if (scope->getRootData()->hasFunction(id))
			{
				fc = scope->getRootData();
				fc->addMatchingFunctions(possibleMatches, id);
				callType = RootFunction;
				return;

			}
			else if (scope->getGlobalScope()->hasFunction(function.id))
			{
				fc = scope->getGlobalScope();
				fc->addMatchingFunctions(possibleMatches, function.id);
				callType = ApiFunction;
				return;
			}
			else if(!function.id.isExplicit())
			{
				auto scopeId = scope->getScopeSymbol();
				auto fP = function.id.getParent();

				if (fP.isParentOf(scopeId))
				{
					if (auto cs = dynamic_cast<ClassScope*>(scope->getScopeForSymbol(function.id)))
					{
						fc = cs->typePtr->getFunctionClass();
						ownedFc = fc;
						fc->addMatchingFunctions(possibleMatches, function.id);
						callType = MemberFunction;

						TypeInfo thisType(cs->typePtr.get());

						setObjectExpression(new ThisPointer(location, thisType));

						return;
					}
				}
			}

			if (function.returnType.isStatic())
			{
				if (auto cs = dynamic_cast<ClassScope*>(scope->getScopeForSymbol(function.id)))
				{
					fc = cs->typePtr->getFunctionClass();
					ownedFc = fc;
					fc->addMatchingFunctions(possibleMatches, function.id);
					callType = StaticFunction;
					return;
				}
			}

			
			throwError("Fuuck");
		}

		if (getObjectExpression()->getTypeInfo().isComplexType())
		{
			if (fc = getObjectExpression()->getTypeInfo().getComplexType()->getFunctionClass())
			{
				ownedFc = fc.get();

				if (function.id.isExplicit())
				{
					function.id = fc->getClassName().getChildId(function.id.getIdentifier());
				}

				fc->addMatchingFunctions(possibleMatches, function.id);
				callType = MemberFunction;

				return;
			}
		}

		if (auto ss = dynamic_cast<SymbolStatement*>(getObjectExpression().get()))
		{
			auto symbol = ss->getSymbol();

			if (fc = scope->getRootData()->getSubFunctionClass(symbol.id))
			{
				// Function with registered parent object (either API class or JIT callable object)

				auto id = function.id;
				fc->addMatchingFunctions(possibleMatches, id);

				callType = ss->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				return;
			}
			if (scope->getGlobalScope()->hasFunction(symbol.id))
			{
				jassert(function.id.isExplicit());

				// Function with globally registered object (either API class or JIT callable object)
				fc = scope->getGlobalScope()->getGlobalFunctionClass(symbol.id);

				auto id = fc->getClassName().getChildId(function.id.getIdentifier());
				fc->addMatchingFunctions(possibleMatches, id);

				callType = ss->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				return;
			}
		}

		location.throwError("Can't resolve function call " + function.getSignature());
	}

	
	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		jassert(fc != nullptr);

		if (callType == InbuiltFunction)
		{
			// Will be done at parser level
			jassert(function.isResolved());
			return;
		}

		Array<TypeInfo> parameterTypes;

		for (int i = 0; i < getNumArguments(); i++)
			parameterTypes.add(compiler->convertToNativeTypeIfPossible(getArgument(i)->getTypeInfo()));

		for (auto& f : possibleMatches)
		{
			if (TemplateParameter::ListOps::isArgument(f.templateParameters))
			{
				// Externally defined functions don't have a specialized instantiation, so we
				// need to resolve the template parameters here...
				jassert(TemplateParameter::ListOps::isParameter(function.templateParameters));

				auto r = Result::ok();
				f.templateParameters = TemplateParameter::ListOps::merge(f.templateParameters, function.templateParameters, r);
				location.test(r);
			}

			for (auto& a: f.args)
				a.typeInfo = compiler->convertToNativeTypeIfPossible(a.typeInfo);

			jassert(function.id == f.id);

			if (f.matchesArgumentTypes(parameterTypes) && f.matchesTemplateArguments(function.templateParameters))
			{
				int numArgs = f.args.size();

				if (f.canBeInlined(true))
				{
					auto path = findParentStatementOfType<ScopeStatementBase>(this)->getPath();

					SyntaxTreeInlineData d(this, path);
					d.object = getObjectExpression();
					
					for (int i = 0; i < getNumArguments(); i++)
						d.args.add(getArgument(i));

					d.templateParameters = function.templateParameters;

					auto r = f.inlineFunction(&d);

					if (!r.wasOk())
						location.throwError(r.getErrorMessage());

					d.replaceIfSuccess();
					return;
				}

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

				if (function.templateParameters.size() != 0)
				{
					auto tempParameters = function.templateParameters;
					TypeInfo t;

					if (!function.returnType.isDynamic())
						t = function.returnType;

					function = f;
					function.templateParameters = tempParameters;
					function.returnType = t.getType() != Types::ID::Dynamic ? t : getTypeInfo();
				}
				else
					function = f;

				tryToResolveType(compiler);

				return;
			}
		}

		throwError("Wrong argument types for function call");
	}

	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		jassert(fc != nullptr);

		auto t = getTypeInfo().toPointerIfNativeRef();

		reg = compiler->getRegFromPool(scope, t);

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

				auto pType = function.args[i].isReference() ? TypeInfo(Types::ID::Pointer, true) : getArgument(i)->getTypeInfo();
				auto asg = CREATE_ASM_COMPILER(getType());

				if (pType.isComplexType())
				{
					auto objCopy = asg.cc.newStack(pType.getRequiredByteSize(), pType.getRequiredAlignment());
					auto pReg = compiler->getRegFromPool(scope, TypeInfo(Types::ID::Pointer, true));
					pReg->setCustomMemoryLocation(objCopy, false);
					parameterRegs.add(pReg);
				}
				else
				{
					auto pReg = compiler->getRegFromPool(scope, pType);
					pReg->createRegister(asg.cc);
					parameterRegs.add(pReg);
				}
			}
		}
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto t = getTypeInfo();

		auto asg = CREATE_ASM_COMPILER(reg->getType());

		if (callType == MemberFunction || callType == StaticFunction)
		{
			// It might be possible that the JIT compiled member function
			// might not have been injected into the old function class yet

			if (!function)
				fc->fillJitFunctionPointer(function);

			if (!function)
			{
				ComplexType::Ptr classType;

				if (callType == MemberFunction)
					classType = getObjectExpression()->getTypeInfo().getComplexType();
				else
					classType = dynamic_cast<ClassScope*>(scope->getScopeForSymbol(function.id))->typePtr;

				fc = classType->getFunctionClass();
				ownedFc = fc;
			}
		}

		if (!function)
			fc->fillJitFunctionPointer(function);


		if (shouldInlineFunctionCall(compiler, scope))
		{
			inlineFunctionCall(asg);

			return;
		}

		if (!function)
		{
			if (fc == nullptr)
				throwError("Can't resolve function class");

			if (!fc->fillJitFunctionPointer(function))
			{
				if (function.inliner != nullptr)
				{
					inlineFunctionCall(asg);
					return;
				}
				else
					throwError("Can't find function pointer to JIT function " + function.functionName);
			}
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
				if (pReg->hasCustomMemoryLocation())
				{
					acg.emitComplexTypeCopy(pReg, existingReg, getArgument(i)->getTypeInfo().getComplexType());

					auto ptr = pReg->getAsMemoryLocation();
					pReg->createRegister(acg.cc);
					acg.cc.lea(PTR_REG_W(pReg), ptr);

					parameterRegs.set(i, pReg);
				}
				else if (existingReg != nullptr && existingReg != pReg && existingReg->getVariableId())
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

		auto r = asg.emitFunctionCall(reg, function, hasObjectExpression ? getObjectExpression()->reg : nullptr, parameterRegs);

		if (!r.wasOk())
			location.throwError(r.getErrorMessage());

		for (int i = 0; i < parameterRegs.size(); i++)
		{
			if (!function.args[i].isReference())
				parameterRegs[i]->flagForReuse();
		}
	}
}

bool Operations::FunctionCall::shouldInlineFunctionCall(BaseCompiler* compiler, BaseScope* scope) const
{
	if (callType == InbuiltFunction)
		return true;

	if (function.inliner == nullptr)
		return false;

	return scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::Inlining);
}

void Operations::FunctionCall::inlineFunctionCall(AsmCodeGenerator& asg)
{
	AsmInlineData d(asg);
	d.target = reg;
	d.object = hasObjectExpression ? getObjectExpression()->reg : nullptr;
	d.templateParameters = function.templateParameters;

	for (int i = 0; i < getNumArguments(); i++)
		d.args.add(getArgument(i)->reg);

	auto r = function.inlineFunction(&d);

	reg = d.target;

	if (!r.wasOk())
		throwError(r.getErrorMessage());
}

snex::jit::TypeInfo Operations::FunctionCall::getTypeInfo() const
{
	return TypeInfo(function.returnType);
}

bool Operations::FunctionCall::tryToResolveType(BaseCompiler* compiler)
{
	location.test(compiler->namespaceHandler.checkVisiblity(function.id));

	bool ok = Statement::tryToResolveType(compiler);

	if (function.returnType.isTemplateType())
	{
		if (TemplateParameter::ListOps::readyToResolve(function.templateParameters))
		{
			auto l = collectParametersFromParentClass(this, function.templateParameters);

			TemplateParameterResolver resolver(l);
			auto r = resolver.process(function);
			location.test(r);
		}
	}

	if (function.returnType.isDynamic())
	{
		auto prevTemplateParameters = function.templateParameters;

		if (hasObjectExpression)
		{
			auto objectType = getObjectExpression()->getTypeInfo().getComplexType();
			FunctionClass::Ptr objectFunctions = objectType->getFunctionClass();
			function = objectFunctions->getNonOverloadedFunction(function.id);
		}
		else
		{
			function = compiler->getInbuiltFunctionClass()->getNonOverloadedFunction(function.id);

			jassert(function.inliner != nullptr);
		}

		if (function.returnType.isDynamic() && function.inliner != nullptr)
		{
			ReturnTypeInlineData rData(function);
			rData.object = this;
			rData.object->currentCompiler = compiler;
			rData.templateParameters = prevTemplateParameters;
			rData.f = function;
			
			auto r = function.inliner->process(&rData);

			if (!r.wasOk())
				location.throwError(r.getErrorMessage());
		}

		return function.returnType.isDynamic();
	}

	

	return ok;
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
	}
	COMPILER_PASS(BaseCompiler::DataSizeCalculation)
	{
		if (!isStackDefinition(scope) && scope->getRootClassScope() == scope)
			scope->getRootData()->enlargeAllocatedSize(type);
	}
	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		for (auto s : getSymbols())
		{
			if (isStackDefinition(scope))
			{
				if (scope->getScopeForSymbol(s.id) != scope)
					jassertfalse;
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
	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (isStackDefinition(scope))
		{
			if (type.isRef())
			{
				if (auto s = getSubExpr(0))
				{
					reg = s->reg;
					
					if(reg != nullptr)
						reg->setReference(scope, getSymbols().getFirst());
				}
			}
			else
			{
				auto acg = CREATE_ASM_COMPILER(getType());

				for (auto s : getSymbols())
				{
					auto reg = compiler->registerPool.getRegisterForVariable(scope, s);

					if (reg->getType() == Types::ID::Pointer && type.getRequiredByteSize() > 0)
					{
						auto c = acg.cc.newStack(type.getRequiredByteSize(), type.getRequiredAlignment(), "funky");

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

				if (FunctionClass::Ptr fc = getNumChildStatements() > 0 ? type.getComplexType()->getFunctionClass() : nullptr)
				{
					overloadedAssignOp = fc->getSpecialFunction(FunctionClass::AssignOverload, type, { type, getSubExpr(0)->getTypeInfo() });
				}

				for (auto s : stackLocations)
				{
					if (type.getRequiredByteSize() > 0)
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
									acg.emitStackInitialisation(s, type.getComplexType(), nullptr, initValues);
								else if (getSubExpr(0) != nullptr)
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
	}
}

void Operations::Cast::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto sourceType = getSubExpr(0)->getTypeInfo();
		auto targetType = getTypeInfo();

		if (sourceType == targetType)
		{
			replaceInParent(getSubExpr(0));
			return;
		}

	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto sourceType = getSubExpr(0)->getTypeInfo();

		if (sourceType.isComplexType())
		{
			if (compiler->getRegisterType(sourceType) == getType())
			{
				reg = getSubRegister(0);
				return;
			}

			FunctionClass::Ptr fc = sourceType.getComplexType()->getFunctionClass();
			complexCastFunction = fc->getSpecialFunction(FunctionClass::NativeTypeCast, targetType, {});
		}

		auto asg = CREATE_ASM_COMPILER(getType());
		reg = compiler->getRegFromPool(scope, getTypeInfo());

		if (complexCastFunction.isResolved())
		{
			AssemblyRegister::List l;
			auto r = asg.emitFunctionCall(reg, complexCastFunction, getSubRegister(0), l);

			if (!r.wasOk())
			{
				location.throwError(r.getErrorMessage());
			}
		}
		else
		{
			auto sourceType = getSubExpr(0)->getType();
			asg.emitCast(reg, getSubRegister(0), sourceType);
		}
	}
}

void Operations::ClassStatement::process(BaseCompiler* compiler, BaseScope* scope)
{
	

	if (subClass == nullptr)
	{
		subClass = new ClassScope(scope, getStructType()->id, classType);
	}

	processBaseWithChildren(compiler, subClass);

#if 0
	if (auto st = as<SyntaxTree>(getSubExpr(0)))
	{
		DBG(st->dump());
	}
#endif


	COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
	{
		auto cType = getStructType();

		forEachRecursive([cType](Ptr p)
		{
			if (auto f = as<Function>(p))
			{
				cType->addJitCompiledMemberFunction(f->data);
			}

			if (auto tf = as<TemplatedFunction>(p))
			{
				auto fData = tf->data;
				fData.templateParameters = tf->templateParameters;

				cType->addJitCompiledMemberFunction(fData);
			}

			return false;
		});


		addMembersFromStatementBlock(getStructType(), getChildStatement(0));
	}

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		getStructType()->finaliseAlignment();
	}
}

void Operations::ThisPointer::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto fScope = scope->getParentScopeOfType<FunctionScope>();

		jassert(fScope != nullptr);

		Symbol thisSymbol(NamespacedIdentifier("this"), getTypeInfo());

		if (auto ie = StatementBlock::findInlinedParameterInParentBlocks(this, thisSymbol))
			reg = ie->getSubRegister(0);

		if (reg == nullptr)
		{
			if (auto f = dynamic_cast<Function*>(fScope->parentFunction))
				reg = f->objectPtr;
		}
		
		if (reg == nullptr)
			location.throwError("can't resolve this pointer");

		auto objType = compiler->getRegisterType(reg->getTypeInfo());
		auto thisType = compiler->getRegisterType(getTypeInfo());
		jassert(objType == thisType);

		// Fix inlining:
		// the scope will fuck up the object pointer of the function
		// check the object pointer from the inlined function (arg is -1)		
	}
}

void Operations::InlinedArgument::process(BaseCompiler* compiler, BaseScope* scope)
{
	jassert(scope->getScopeType() == BaseScope::Anonymous);

	processChildrenIfNotCodeGen(compiler, scope);

	if (isCodeGenPass(compiler))
	{
		auto f = [this]()
		{
			auto child = getSubExpr(0);

			if (auto sb = as<StatementBlock>(child))
			{
				jassert(sb->isInlinedFunction);
				return false;
			}

			return true;
		};

		if (!preprocessCodeGenForChildStatements(compiler, scope, f))
			return;

		if (s.typeInfo.isComplexType() && !s.isReference())
		{
			auto acg = CREATE_ASM_COMPILER(getTypeInfo().getType());

			auto stackPtr = acg.cc.newStack(s.typeInfo.getRequiredByteSize(), s.typeInfo.getRequiredAlignment());

			auto target = compiler->getRegFromPool(scope, s.typeInfo);

			target->setCustomMemoryLocation(stackPtr, false);

			auto source = getSubRegister(0);

			acg.emitComplexTypeCopy(target, source, s.typeInfo.getComplexType());

			getSubExpr(0)->reg = target;

			reg = getSubRegister(0);
		}
	}
}

}
}
