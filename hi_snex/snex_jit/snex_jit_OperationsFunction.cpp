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
		data.description = comment;

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
			scope->getRootData()->addFunction(classData);
		else
			ownedMemberFunction = classData;

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
			TemplateParameterResolver resolver(collectParametersFromParentClass(this, data.templateParameters));
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

			WeakReference<Statement> statementCopy = statements;

			classData->templateParameters = data.templateParameters;



			auto classDataCopy = FunctionData(*classData);

			auto createInliner = scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::Inlining);

			if (createInliner)
			{
				classData->inliner = Inliner::createHighLevelInliner(data.id, [statementCopy, classDataCopy](InlineData* b)
				{
					jassert(statementCopy.get() != nullptr);
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

		auto ok = runtime->add(&data.function, ch);

		jassert(data.function != nullptr);

		if (scope->getRootClassScope() == scope)
		{
			auto ok = scope->getRootData()->injectFunctionPointer(data);
			jassert(ok);
		}
		else
		{
			if (auto cs = findParentStatementOfType<ClassStatement>(this))
			{
				dynamic_cast<StructType*>(cs->classType.get())->injectMemberFunctionPointer(data, data.function);
			}
			else
			{
				// Should have been catched by the other branch...
				jassertfalse;
			}
		}

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

		compiler->setCurrentPass(BaseCompiler::FunctionCompilation);
	}
}




void Operations::FunctionCall::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	if (Statement::isCodeGenPass(compiler) && findParentStatementOfType<VectorOp>(this) != nullptr)
	{
		// We don't want vector functions to be emitted
		return;
	}

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
					fc = compiler->getInbuiltFunctionClass();
					fc->addMatchingFunctions(possibleMatches, function.id);
					callType = InbuiltFunction;
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
			else if (!function.id.isExplicit())
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
					ComplexType::Ptr p;

					if (cs->isRootClass())
						p = compiler->namespaceHandler.getComplexType(function.id.getParent());
					else
						p = cs->typePtr;

					fc = p->getFunctionClass();
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
			if (!function.canBeInlined(true))
			{
				// Will be done at parser level
				jassert(function.isResolved());
				return;	
			}
		}

		if (possibleMatches.isEmpty())
		{
			String s;

			s << "Can't resolve function " << function.id.toString();

			throwError(s);
		}

		Array<TypeInfo> parameterTypes;

		for (int i = 0; i < getNumArguments(); i++)
			parameterTypes.add(compiler->convertToNativeTypeIfPossible(getArgument(i)->getTypeInfo()));

		for (auto& f : possibleMatches)
		{
			if (f.templateParameters.size() != function.templateParameters.size())
			{
				bool foundBetterMatch = false;

				for (auto& of : possibleMatches)
				{
					if (of.matchesArgumentTypes(parameterTypes) && of.matchesTemplateArguments(function.templateParameters))
					{
						// This is a better candidate with a template amount match so we'll skip this...
						foundBetterMatch = true;
						break;
					}
				}

				if (foundBetterMatch)
					continue;

				TypeInfo::List originalList;
				for (auto a : f.args)
					originalList.add(a.typeInfo);

				auto r = Result::ok();
				auto resolved = TemplateParameter::ListOps::mergeWithCallParameters(f.templateParameters, function.templateParameters, originalList, parameterTypes, r);

				location.test(r);

				f.templateParameters = resolved;
				function.templateParameters = resolved;
			}


			if (TemplateParameter::ListOps::isArgument(f.templateParameters))
			{
				// Externally defined functions don't have a specialized instantiation, so we
				// need to resolve the template parameters here...
				jassert(TemplateParameter::ListOps::isParameter(function.templateParameters));

				auto r = Result::ok();
				f.templateParameters = TemplateParameter::ListOps::merge(f.templateParameters, function.templateParameters, r);
				location.test(r);
			}

			for (auto& a : f.args)
				a.typeInfo = compiler->convertToNativeTypeIfPossible(a.typeInfo);

			jassert(function.id == f.id);

			if ((f.matchesArgumentTypes(parameterTypes) || possibleMatches.size() == 1) && f.matchesTemplateArguments(function.templateParameters))
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
					setTypeForChild(i + getObjectExpression() != nullptr ? 1 : 0, f.args[i].typeInfo);

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
					auto alignment = pType.getRequiredAlignment();
					auto size = pType.getRequiredByteSize();

					// This will be initialised using SSE instructions...
					if (size % 16 == 0 && size > 0)
						alignment = 16;

					auto objCopy = asg.cc.newStack(size, alignment);
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
				{
					if (function.templateParameters.isEmpty())
						throwError("Can't find function pointer to JIT function " + function.functionName);
					else
					{


						//throwError("The function template " + function.getSignature({}) + " was not instantiated");

						return;
					}
				}
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
				bool willBeInlined = function.canBeInlined(false);

				if (willBeInlined)
				{
					parameterRegs.set(i, existingReg);
				}
				else if (pReg->hasCustomMemoryLocation())
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

#if REMOVE_REUSABLE_REG
		for (int i = 0; i < parameterRegs.size(); i++)
		{
			if (!function.args[i].isReference())
				parameterRegs[i]->flagForReuse();
		}
#endif
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
			function = compiler->getInbuiltFunctionClass()->getNonOverloadedFunctionRaw(function.id);

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

bool Operations::FunctionCall::canBeAliasParameter(Ptr e)
{
	return dynamic_cast<SymbolStatement*>(e.get()) != nullptr || dynamic_cast<MemoryReference*>(e.get()) != nullptr;
}

Operations::FunctionCall::FunctionCall(Location l, Ptr f, const Symbol& id, const Array<TemplateParameter>& tp) :
	Expression(l)
{
	for (auto& p : tp)
	{
		jassert(!p.isTemplateArgument());
	}

	function.id = id.id;
	function.returnType = id.typeInfo;
	function.templateParameters = tp;

	if (auto dp = dynamic_cast<DotOperator*>(f.get()))
	{
		setObjectExpression(dp->getDotParent());
	}
}

void Operations::FunctionCall::setObjectExpression(Ptr e)
{
	if (hasObjectExpression)
	{
		getObjectExpression()->replaceInParent(e);
	}
	else
	{
		hasObjectExpression = true;
		addStatement(e.get(), true);
	}
}

}
}