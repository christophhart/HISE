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


void Operations::Function::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::FunctionTemplateParsing)
	{
		data.description = comment;

		functionScope = new FunctionScope(scope, data.id, isHardcodedFunction);

		if (isHardcodedFunction)
			functionScope->setHardcodedClassType(hardcodedObjectType);
		
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

		if (statements == nullptr)
		{
			try
			{
				FunctionParser p(compiler, *this);

				auto ssb = findParentStatementOfType<ScopeStatementBase>(this);

				BlockParser::ScopedScopeStatementSetter svs(&p, ssb);

				p.currentScope = functionScope;

				{
					NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, data.id);

					
					sns.clearCurrentNamespace();

					auto fNamespace = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

					

					for (auto arg : classData->args)
					{
                        compiler->namespaceHandler.addSymbol(fNamespace.getChildId(arg.id.id), arg.typeInfo, NamespaceHandler::Variable, NamespaceHandler::SymbolDebugInfo());
					}

					// Might be set manually from a precodegen function
					if (statements == nullptr)
						statements = p.parseStatementList();

					auto specialFunctionType = (FunctionClass::SpecialSymbols)classData->getSpecialFunctionType();

					if (specialFunctionType == FunctionClass::Constructor || specialFunctionType == FunctionClass::Destructor)
					{
						if (auto cs = dynamic_cast<ClassScope*>(scope))
						{
							if (auto st = dynamic_cast<StructType*>(cs->typePtr.get()))
							{
								auto baseSpecialFunctions = st->getBaseSpecialFunctions(specialFunctionType);

								if (!baseSpecialFunctions.isEmpty())
								{
									List l;

									for (auto& m : baseSpecialFunctions)
									{
										// We need to change the parent ID to the derived class
										//auto specialFunctionId = st->id.getChildId(m.id.getIdentifier());

										//auto fc = new FunctionCall(location, nullptr, { specialFunctionId, m.returnType }, {});

										auto fc = new FunctionCall(location, nullptr, {m.id, m.returnType}, {});
										fc->setObjectExpression(new ThisPointer(location, TypeInfo(st, false, true)));

										l.add(fc);
									}

									if (specialFunctionType == FunctionClass::Constructor)
									{
										for (int i = l.size() - 1; i >= 0; i--)
											statements->addStatement(l[i], true);
									}

#if 0
									if (specialFunctionType == FunctionClass::Destructor)
									{
										for (int i = l.size() - 1; i >= 0; i--)
										{
											Symbol s(NamespacedIdentifier("this"), TypeInfo(cs->typePtr.get()));
											StatementWithControlFlowEffectBase::addDestructorToAllChildStatements(statements, s);
										}
									}
#endif
								}
							}
						}
					}

					// Now we might set the return type...
					if (classData->returnType.isDynamic())
					{
						Array<TypeInfo> returnTypes;

						statements->forEachRecursive([&returnTypes, compiler](Ptr s)
						{
							if (auto rt = as<ReturnStatement>(s))
							{
								if (auto r = rt->getSubExpr(0))
								{
									if (r->tryToResolveType(compiler))
									{
										returnTypes.addIfNotAlreadyThere(r->getTypeInfo());
									}
								}
							}

							return false;
						}, IterationType::NoChildInlineFunctionBlocks);

						switch (returnTypes.size())
						{
						case 0: location.throwError("expected return statement with type");
						case 1: 
						{
							auto prevT = data.returnType;

							auto newT = returnTypes[0].withModifiers(prevT.isConst(), prevT.isRef(), prevT.isStatic());

							newT.setRefCounted(false);

							classData->returnType = newT;
							data.returnType = newT;
							compiler->namespaceHandler.setTypeInfo(fNamespace, NamespaceHandler::Function, newT);

							if (auto cs = dynamic_cast<ClassScope*>(scope))
							{
								if (auto st = dynamic_cast<StructType*>(cs->typePtr.get()))
									st->setTypeForDynamicReturnFunction(data);
							}

							break;
						}
						default: location.throwError("Ambigous return types");
						}
					}
				}
			}
			catch (ParserHelpers::Error& e)
			{
				statements = nullptr;
				functionScope = nullptr;

				throw e;
			}
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

            {
                NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classData->id);
                
                compiler->executePass(BaseCompiler::PreSymbolOptimization, functionScope, sTree);
            }
            
            compiler->executePass(BaseCompiler::DataAllocation, functionScope, sTree);
            compiler->executePass(BaseCompiler::DataInitialisation, functionScope, sTree);
        
			if (classData->isConst())
			{
				sTree->forEachRecursive([](Ptr p)
				{
					if (auto a = as<Assignment>(p))
					{
						if (auto dp = as<DotOperator>(a->getSubExpr(1)))
						{
							if (as<ThisPointer>(dp->getDotParent()) != nullptr)
							{
								dp->location.throwError("Can't modify const object variables");
							}
						}
					}

					return false;
				}, IterationType::AllChildStatements);
			}

            {
                NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classData->id);
                
                compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, sTree);
            }
            
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, sTree);
			compiler->executePass(BaseCompiler::PostSymbolOptimization, functionScope, sTree);

			compiler->setCurrentPass(BaseCompiler::FunctionParsing);

			WeakReference<Statement> statementCopy = statements.get();

			classData->templateParameters = data.templateParameters;

			auto fParameters = classData->args;

			auto createInliner = scope->getGlobalScope()->shouldInlineFunction(classData->id.getIdentifier());

			auto inlineScore = sTree->getInlinerScore();

			createInliner &= isPositiveAndBelow(inlineScore, InlineScoreThreshhold);

			if (createInliner || isHardcodedFunction)
			{
				classData->inliner = Inliner::createHighLevelInliner(data.id, [sTree, fParameters](InlineData* b)
				{
					return b->toSyntaxTreeData()->makeInlinedStatementBlock(sTree, fParameters);
				});
			}

			if (!isHardcodedFunction)
			{
				if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
					st->addJitCompiledMemberFunction(*classData);
			}
		}
		catch (ParserHelpers::Error& e)
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

		statements->forEachRecursive([&](Statement::Ptr p)
		{
			if (auto fc = as<FunctionCall>(p))
			{
				auto inliner = fc->function.inliner;
				
				if (fc->function.function == nullptr && inliner != nullptr && inliner->precodeGenFunc)
				{
					PreCodeGenInlineData d;

					d.rootObject = fc->getObjectExpression();
					d.templateParameters = fc->function.templateParameters;
					
					auto ok = inliner->precodeGenFunc(&d);
					location.test(ok);

					for (auto f : d.functionsToCompile)
					{
						ScopedPointer<ClassScope> internalClassScope = new ClassScope(scope, f.functionToCompile.id.getParent(), f.objType);

						ScopedPointer<Function> inf = new Function(location, f.functionToCompile.toSymbol());

						inf->data = f.functionToCompile;
						inf->hardcodedObjectType = f.objType;
						inf->isHardcodedFunction = true;
						

						auto stree = new SyntaxTree(location, f.functionToCompile.id);

						auto innerFunc = new FunctionCall(location, nullptr, f.functionToCompile.toSymbol(), {});

						auto obj = new MemoryReference(location, d.rootObject->clone(location), TypeInfo(f.objType, false, true), f.offsetFromRoot);
						innerFunc->setObjectExpression(obj);

						for (auto& a: f.functionToCompile.args)
						{
							inf->parameters.add(a.id.getIdentifier());
							auto realId = f.functionToCompile.id.getChildId(a.id.getIdentifier());
							auto p = new VariableReference(location, { realId, a.typeInfo });
							innerFunc->addArgument(p);
						}

#if 0
						for (int i = 0; i < fc->getNumArguments(); i++)
						{
							innerFunc->addArgument(fc->getArgument(i)->clone(location));
						}
#endif

						stree->addStatement(innerFunc);

						inf->statements = stree;

						compiler->executeScopedPass(BaseCompiler::FunctionTemplateParsing, internalClassScope, inf);
						compiler->executeScopedPass(BaseCompiler::FunctionParsing, internalClassScope, inf);
						compiler->executeScopedPass(BaseCompiler::FunctionCompilation, internalClassScope, inf);
						
						fc->function.function = inf->data.function;
#if 0
						auto fInliner = f.functionToCompile.inliner;

						FunctionCompileData fcd(f.functionToCompile, compiler, scope);

						auto innerFunc = new FunctionCall(location, nullptr, { f.functionToCompile.id, f.functionToCompile.returnType }, {});
						auto obj = new MemoryReference(location, d.rootObject->clone(location), TypeInfo(f.objType, false, true), f.offsetFromRoot);
						innerFunc->setObjectExpression(obj);
						
						fcd.statementToCompile = innerFunc;

						for (int i = 0; i < fc->getNumArguments(); i++)
						{
							innerFunc->addArgument(fc->getArgument(i)->clone(location));
						}

						if (fInliner->highLevelFunc)
						{
							statements->addStatement(innerFunc);
							SyntaxTreeInlineData::processUpToCurrentPass(statements, innerFunc);
							
							innerFunc->inlineAndSetType(compiler, f.functionToCompile);

							auto block = statements->getLastStatement();

							block->replaceInParent(new Noop(location));

							auto inf = new Function(location, {f.functionToCompile.id, f.functionToCompile.returnType});

							// Continue here...
							jassertfalse;
							
							

#if 0
							SyntaxTreeInlineData b(innerFunc, as<SyntaxTree>(statements)->getPath(), f.functionToCompile);
							b.object = obj;

							for (int i = 0; i < innerFunc->getNumArguments(); i++)
							{
								b.args.add(innerFunc->getArgument(i)->clone(location));
							}

							

							auto ok = fInliner->process(&b);
							location.test(ok);

							b.replaceIfSuccess();
							b.target->replaceInParent(new Noop(location));
#endif

							if (auto asf = as<FunctionCall>(fcd.statementToCompile))
							{
								if (auto fPointer = asf->function.function)
								{
									dynamic_cast<StructType*>(f.objType.get())->injectMemberFunctionPointer(f.functionToCompile, fPointer);
									continue;
								}
							}
						}
						else
						{
							fcd.statementToCompile = innerFunc;
						}



						void* fPointer = nullptr;
						
						if (as<FunctionCall>(fcd.statementToCompile))
						{
							fPointer = compileFunction(fcd, BIND_MEMBER_FUNCTION_1(Function::compileAsmInlinerBeforeCodegen));
						}
						else if (auto sb = as<StatementBlock>(fcd.statementToCompile))
						{
							ScopedPointer<FunctionScope> thisFunctionScope = new FunctionScope(scope, f.functionToCompile.id, true);
							thisFunctionScope->setHardcodedClassType(f.objType);

							thisFunctionScope->data = f.functionToCompile;

							thisFunctionScope->parameters.addArray(parameters);
							

							fcd.functionScopeToUse = thisFunctionScope;

							fPointer = compileFunction(fcd, BIND_MEMBER_FUNCTION_1(Function::compileSyntaxTree));
						}
						
						dynamic_cast<StructType*>(f.objType.get())->injectMemberFunctionPointer(f.functionToCompile, fPointer);

						fc->function.function = fPointer;
#endif
					}
				};
			}

			return false;
		}, IterationType::AllChildStatements);

		FunctionCompileData fcd(data, compiler, scope);

		fcd.statementToCompile = statements;
		fcd.functionScopeToUse = functionScope.get();

		compileFunction(fcd, BIND_MEMBER_FUNCTION_1(Function::compileSyntaxTree));
		

		if (scope->getRootClassScope() == scope)
		{
			auto ok = scope->getRootData()->injectFunctionPointer(data);
			jassertEqual(ok, true);
		}
		else
		{
			if (auto cs = findParentStatementOfType<ClassStatement>(this))
			{
				dynamic_cast<StructType*>(cs->classType.get())->injectMemberFunctionPointer(data, data.function);
			}
			else if (auto asClassScope = dynamic_cast<ClassScope*>(scope))
			{
				if (auto st = dynamic_cast<StructType*>(asClassScope->typePtr.get()))
				{
					st->injectMemberFunctionPointer(data, data.function);
				}
			}
			else
			{
				// Should have been catched by the other branch...
				jassertfalse;
			}
		}

		

		jassert(scope->getScopeType() == BaseScope::Class);

		compiler->setCurrentPass(BaseCompiler::FunctionCompilation);
	}
}

void* Operations::Function::compileFunction(FunctionCompileData& f, const FunctionCompileData::InnerFunction& func)
{
#if !SNEX_MIR_BACKEND
	f.assemblyLogger = new asmjit::StringLogger();

	auto runtime = getRuntime(f.compiler);

	ScopedPointer<asmjit::CodeHolder> ch = new asmjit::CodeHolder();
	ch->setLogger(f.assemblyLogger);
	ch->setErrorHandler(f.errorHandler);
	ch->init(runtime->environment());

	//code->setErrorHandler(this);

	f.cc = new asmjit::X86Compiler(ch);

	AsmCleanupPass* p = nullptr;

	if (f.scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::AsmOptimisation))
	{
		f.cc->addPass(p = new AsmCleanupPass());
	}

	dynamic_cast<ClassCompiler*>(f.compiler)->setFunctionCompiler(f.cc);

	f.compiler->registerPool.clear();

	func(f);
	
	f.cc->endFunc();
	f.cc->finalize();

	if (p != nullptr)
		f.cc->deletePass(p);

	f.cc = nullptr;

	(asmjit::ErrorCode)runtime->add(&f.data.function, ch);

	jassert(f.data.function != nullptr);

	auto& as = dynamic_cast<ClassCompiler*>(f.compiler)->assembly;

	juce::String fName = f.data.getSignature();

	as << "; function " << fName << "\n";
	as << f.assemblyLogger->data();

	ch->setLogger(nullptr);
	f.assemblyLogger = nullptr;
	ch = nullptr;

	return f.data.function;
#else
	return nullptr;
#endif
}

void Operations::Function::compileSyntaxTree(FunctionCompileData& f)
{
	auto compiler = f.compiler;
	auto scope = f.scope;

	ignoreUnused(compiler);

	hasObjectPtr = scope->getParent()->getScopeType() == BaseScope::Class && !f.data.returnType.isStatic();

#if !SNEX_MIR_BACKEND

	FuncSignatureX sig;

	auto objectType = hasObjectPtr ? compiler->getRegisterType(TypeInfo(dynamic_cast<ClassScope*>(scope)->typePtr.get())) : Types::ID::Void;

	AsmCodeGenerator::fillSignature(f.data, sig, objectType);
	auto funcNode = f.cc->addFunc(sig);

	if (hasObjectPtr)
	{
		auto rType = compiler->getRegisterType(TypeInfo(dynamic_cast<ClassScope*>(scope)->typePtr.get()));
		objectPtr = compiler->registerPool.getNextFreeRegister(f.functionScopeToUse, TypeInfo(rType, true));
		auto asg = CREATE_ASM_COMPILER(rType);
		asg.emitParameter(this, funcNode, objectPtr, -1);
	}
	
	compiler->executePass(BaseCompiler::RegisterAllocation, f.functionScopeToUse, f.statementToCompile.get());
	compiler->executePass(BaseCompiler::CodeGeneration, f.functionScopeToUse, f.statementToCompile.get());
#endif
}

void Operations::Function::compileAsmInlinerBeforeCodegen(FunctionCompileData& f)
{
#if SNEX_ASMJIT_BACKEND
	auto fc = as<FunctionCall>(f.statementToCompile);

	jassert(fc != nullptr);

	auto st = fc->getObjectExpression()->getTypeInfo().getTypedComplexType<StructType>();

	auto compiler = f.compiler;

	FuncSignatureX sig;
	AsmCodeGenerator::fillSignature(f.data, sig, Types::ID::Pointer);
	auto funcNode = f.cc->addFunc(sig);

	auto rType = compiler->getRegisterType(TypeInfo(st));
	auto objectPtr = compiler->registerPool.getNextFreeRegister(f.scope, TypeInfo(st));

	auto acg = CREATE_ASM_COMPILER(rType);

	acg.emitParameter(f.data, funcNode, objectPtr, -1, true);

	AssemblyRegister::List parameters;

	int i = 0;

	for (auto a : f.data.args)
	{
		auto pReg = compiler->registerPool.getNextFreeRegister(f.scope, a.typeInfo);
		acg.emitParameter(f.data, funcNode, pReg, i++, true);
	}

	AssemblyRegister::Ptr returnReg;

	if (f.data.returnType != TypeInfo(Types::ID::Void))
	{
		returnReg = compiler->registerPool.getNextFreeRegister(f.scope, f.data.returnType);
	}

	auto ok = acg.emitFunctionCall(returnReg, f.data, objectPtr, parameters);

	location.test(ok);
#endif
}

void FunctionCall::resolveBaseClassMethods()
{
	if(possibleMatches.isEmpty())
	{
		if(auto st = getObjectExpression()->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			st->findMatchesFromBaseClasses(possibleMatches, function.id, baseOffset, baseClass);

			if(fc->isConstructor(function.id))
				possibleMatches = st->getBaseSpecialFunctions(FunctionClass::SpecialSymbols::Constructor);

			if(fc->isDestructor(function.id))
				possibleMatches = st->getBaseSpecialFunctions(FunctionClass::SpecialSymbols::Destructor);

			if(!possibleMatches.isEmpty())
				callType = BaseMemberFunction;
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

			if (!function.id.isExplicit())
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

						resolveBaseClassMethods();

						return;
					}
				}
			}

			if (auto nfc = compiler->getInbuiltFunctionClass())
			{
				if (nfc->hasFunction(function.id))
				{
					fc = compiler->getInbuiltFunctionClass();
					fc->addMatchingFunctions(possibleMatches, function.id);
					callType = InbuiltFunction;

					if (!function.isResolved() && possibleMatches.size() == 1)
					{
						function = possibleMatches[0];
					}

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
			else if (scope->getRootData()->hasFunction(function.id))
			{
				fc = scope->getRootData();
				fc->addMatchingFunctions(possibleMatches, function.id);
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

			throwError("Unknown function " + function.getSignature());
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

				resolveBaseClassMethods();

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
		{
			auto e = getArgument(i);
			e->tryToResolveType(compiler);
			parameterTypes.add(compiler->convertToNativeTypeIfPossible(e->getTypeInfo()));
		}

		if (possibleMatches.size() > 0)
		{
			FunctionClass::ResolveSorter sorter;
			possibleMatches.sort(sorter);
		}
		

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

				for (auto& a : f.args)
				{
					if (auto tcd = a.typeInfo.getTypedIfComplexType<TemplatedComplexType>())
					{
						a.typeInfo = TypeInfo(tcd->createTemplatedInstance(f.templateParameters, r), a.typeInfo.isConst(), a.typeInfo.isRef());

						location.test(r);
					}
				}
			}

			for (auto& a : f.args)
				a.typeInfo = compiler->convertToNativeTypeIfPossible(a.typeInfo);

			jassert(function.id.getIdentifier() == f.id.getIdentifier());

			if (f.matchesArgumentTypes(parameterTypes) && f.matchesTemplateArguments(function.templateParameters))
			{
				inlineAndSetType(compiler, f);
				return;
			}
		}

		jassert(compiler == currentCompiler);

		if (resolveWithParameters(parameterTypes))
			return;
			

		// The initial type check failed, now we try to "upcast" the numeric arguments

		convertNumericTypes(parameterTypes);

		if (resolveWithParameters(parameterTypes))
		{
			logWarning("Implicit cast of arguments");
			return;
		}
		

		String s;
		
		s << "Can't resolve " << function.id.toString() << "(";

		for (auto pt : parameterTypes)
		{
			s << pt.toString() << ", ";
		};

		s = s.upToLastOccurrenceOf(", ", false, false);
		s << ")";

		throwError(s);
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (isVectorOpFunction())
			return;

		jassert(fc != nullptr);

		auto t = getTypeInfo().toPointerIfNativeRef();
		reg = compiler->getRegFromPool(scope, t);

		if (shouldInlineFunctionCall(compiler, scope))
		{
			return;
		}
		else
		{
			for (int i = 0; i < getNumArguments(); i++)
			{
				if (auto arg = getArgument(i))
				{
					if (arg->reg != nullptr && !arg->reg->getVariableId())
					{
						parameterRegs.add(arg->reg);
						continue;
					}
				}

				tryToResolveType(compiler);


				auto pType = function.args[i].typeInfo;
				
				if (pType.isDynamic())
					pType = getArgument(i)->getTypeInfo().withModifiers(pType.isConst(), pType.isRef());
				
				jassert(pType.isValid());

				pType = pType.toPointerIfNativeRef();

				
				
				auto asg = CREATE_ASM_COMPILER(getType());

				if (pType.isComplexType() && !pType.isRef())
				{
					auto alignment = pType.getRequiredAlignment();
					auto size = pType.getRequiredByteSize();

					// This will be initialised using SSE instructions...
					if (size % 16 == 0 && size > 0)
						alignment = 16;

					auto objCopy = asg.cc.newStack((uint32_t)size, (uint32_t)alignment);
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
		if (isVectorOpFunction())
			return;

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
                {
                    auto cs = dynamic_cast<ClassScope*>(scope->getScopeForSymbol(function.id));
                    
                    if(cs != nullptr)
                        classType = cs->typePtr;
                    else
                        throwError("Can't find class scope");
                }

				fc = classType->getFunctionClass();
				ownedFc = fc;
			}
		}

		if (!function)
			fc->fillJitFunctionPointer(function);

		adjustBaseClassPointer(compiler, scope);

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
				if (function.canBeInlined(false)) // function.inliner != nullptr)
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
			location.calculateLineIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());

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
	}
#endif
}


bool Operations::FunctionCall::shouldInlineFunctionCall(BaseCompiler* compiler, BaseScope* scope) const
{
	if (!allowInlining)
		return false;

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

			ok = !function.returnType.isTemplateType() && !function.returnType.isDynamic();
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
			if (auto cType = compiler->namespaceHandler.getComplexType(function.id))
			{
				// might be a constructor with the syntax auto obj = MyObject(...);
				fc = cType->getFunctionClass();

				function.id = function.id.getChildId(function.id.getIdentifier());
				function.returnType = TypeInfo(cType, false, false);
#if 0
                Array<TypeInfo> args;
                
                for(auto& a: function.args)
                    args.add(a.typeInfo);
                
                auto cf = fc->getConstructor(args);
                
                
                
                if(cf.inliner != nullptr)
                {
                    function.inliner = cf.inliner;
                }
                else
                {
                    function.inliner = Inliner::createAsmInliner(function.id, [](InlineData* b)
                    {
                        auto d = b->toAsmInlineData();

                        auto typeToInitialise = d->target->getTypeInfo().getComplexType();
                        auto& cc = d->gen.cc;

                        auto mem = cc.newStack(typeToInitialise->getRequiredByteSize(), typeToInitialise->getRequiredAlignment());
                        
                        d->target->setCustomMemoryLocation(mem, false);

                        d->gen.emitStackInitialisation(d->target, typeToInitialise, nullptr, typeToInitialise->makeDefaultInitialiserList());

                        FunctionClass::Ptr fc = typeToInitialise->getFunctionClass();

                        Array<TypeInfo> argTypes;

                        for (auto a : d->args)
                            argTypes.add(a->getTypeInfo());

                        auto f = fc->getConstructor(argTypes);

                        // Remove the inliner
                        f.inliner = nullptr;
                        
                        if(f.function == nullptr)
                            return Result::fail("Function must be inlined before codegen");
                        
                        auto ok = d->gen.emitFunctionCall(nullptr, f, d->target, d->args);

                        
                        return Result::ok();
                    });
                }

				for (int i = 0; i < getNumArguments(); i++)
					function.addArgs("a" + String(i + 1), getArgument(i)->getTypeInfo());

				possibleMatches.add(function);
				callType = StaticFunction;
#endif
				return true;
			}

			function = compiler->getInbuiltFunctionClass()->getNonOverloadedFunctionRaw(function.id);

			jassert(function.inliner != nullptr);
		}

		if (function.returnType.isDynamic() && function.inliner != nullptr)
		{
			jassert(function.inliner->returnTypeFunction);

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

Operations::FunctionCall::FunctionCall(Location l, Ptr f, const Symbol& id, const Array<TemplateParameter>& tp) :
	Expression(l)
{
#if JUCE_DEBUG
	for (auto& p : tp)
	{
		jassert(!p.isTemplateArgument());
	}
#endif

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

void Operations::FunctionCall::inlineAndSetType(BaseCompiler* compiler, const FunctionData& f)
{
	int numArgs = f.args.size();

	for (int i = 0; i < numArgs; i++)
	{
		setTypeForChild(i + ((getObjectExpression() != nullptr) ? 1 : 0), f.args[i].typeInfo);

		if (f.args[i].isReference())
		{
			if (!canBeReferenced(getArgument(i)))
			{
				throwError("Can't use rvalues for reference parameters");
			}
		}
	}

	bool shouldInline = allowInlining;

	shouldInline &= f.canBeInlined(true);
	
	if (!compiler->getOptimizations().contains(OptimizationIds::Inlining))
	{
		if (f.function != nullptr)
			shouldInline = false;
	}

	if (shouldInline)
	{
		auto path = findParentStatementOfType<ScopeStatementBase>(this)->getPath();

		SyntaxTreeInlineData d(this, path, f);
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

	if (!allowInlining)
		function.inliner = nullptr;

	tryToResolveType(compiler);
}

void Operations::FunctionCall::addDefaultParameterExpressions(const FunctionData& f)
{
	int numDefinedArguments = getNumArguments();

	Statement::List args;

	for (int i = 0; i < getNumArguments(); i++)
		args.add(getArgument(i));

	for (int i = numDefinedArguments; i < f.args.size(); i++)
	{
		auto e = f.getDefaultExpression(f.args[i]);

		auto path = findParentStatementOfType<ScopeStatementBase>(this)->getPath();

		SyntaxTreeInlineData d(this, path, f);
		d.object = getObjectExpression();
		
		std::swap(d.args, args);
		
		d.templateParameters = function.templateParameters;
		auto ok = e(&d);

		location.test(ok);
		std::swap(d.args, args);
	}
	
	Array<TypeInfo> typesAfterDefault;

	for (auto a : args)
		typesAfterDefault.add(a->getTypeInfo());

	if (!f.matchesArgumentTypes(typesAfterDefault))
		location.throwError("Can't deduce proper default values");

	for (int i = numDefinedArguments; i < args.size(); i++)
	{
		addArgument(args[i]);
		SyntaxTreeInlineData::processUpToCurrentPass(this, args[i]);
	}
}

bool Operations::FunctionCall::isVectorOpFunction() const
{
	return findParentStatementOfType<VectorOp>(this) != nullptr &&
		   VectorOp::getFunctionSignatureId(function.id.getIdentifier().toString(), false) != 0;
}

void Operations::FunctionCall::adjustBaseClassPointer(BaseCompiler* compiler, BaseScope* scope)
{
	if (auto obj = getObjectExpression())
	{
		jassert(obj->reg != nullptr);

		if (auto st = obj->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			auto bindex = st->getBaseClassIndexForMethod(function);

			if (bindex != -1 && st->hasMember(bindex))
			{
				if (auto byteOffset = st->getMemberOffset(bindex))
				{
#if SNEX_ASMJIT_BACKEND
					auto asg = CREATE_ASM_COMPILER(obj->reg->getType());
					AsmCodeGenerator::TemporaryRegister tempReg(asg, scope, obj->reg->getTypeInfo());

					if (obj->reg->isMemoryLocation())
					{
						auto mem = obj->reg->getAsMemoryLocation().cloneAdjusted(byteOffset);
						tempReg.tempReg->setCustomMemoryLocation(mem, obj->reg->isGlobalMemory());
					}
					else
					{
						auto mem = x86::ptr(PTR_REG_R(obj->reg)).cloneAdjusted(byteOffset);
						tempReg.tempReg->setCustomMemoryLocation(mem, obj->reg->isGlobalMemory());
					}

					obj->reg = tempReg.tempReg;
#else
					jassertfalse;
#endif
				}
			}
		}
	}
}

bool Operations::FunctionCall::resolveWithParameters(Array<TypeInfo> parameterTypes)
{
	for (auto& f : possibleMatches)
	{
		if (f.matchesArgumentTypesWithDefault(parameterTypes))
		{
			addDefaultParameterExpressions(f);
			inlineAndSetType(currentCompiler, f);
			return true;
		}
	}

	return false;
}

void Operations::FunctionCall::convertNumericTypes(Array<TypeInfo>& types)
{
	Types::ID bestType = Types::ID::Integer;

	for (auto& t : types)
	{
		if (t.getType() == Types::ID::Double)
			bestType = Types::ID::Double;

		if (t.getType() == Types::ID::Float && bestType != Types::ID::Double)
			bestType = Types::ID::Float;
	}

	for (int i = 0; i < types.size(); i++)
	{
		if (types[i].isComplexType())
			continue;

		getArgument(i)->replaceInParent(new Cast(location, getArgument(i)->clone(location), bestType));

		SyntaxTreeInlineData::processUpToCurrentPass(this, getArgument(i));

		types.set(i, TypeInfo(bestType, types[i].isConst(), types[i].isRef()));
	}
}

void FunctionData::setDefaultParameter(const Identifier& argId, const VariableStorage& immediateValue)
{
	auto newDefaultParameter = new DefaultParameter();
	newDefaultParameter->id = Symbol(id.getChildId(argId), TypeInfo(immediateValue.getType()));
	newDefaultParameter->expressionBuilder = [immediateValue](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();
		d->args.add(new Operations::Immediate(d->location, immediateValue));
		return Result::ok();
	};

	defaultParameters.add(newDefaultParameter);
}

}
}
