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
		functionScope = new FunctionScope(scope, functionId);

		for (int i = 0; i < data.args.size(); i++)
		{
			data.args.getReference(i).parameterName = parameters[i].toString();
		}


		functionScope->data = data;

		functionScope->parameters.addArray(parameters);
		functionScope->parentFunction = dynamic_cast<ReferenceCountedObject*>(this);

		auto classData = new FunctionData(data);


		dynamic_cast<ClassScope*>(scope)->addFunction(classData);

		for (int i = 0; i < parameters.size(); i++)
		{
			auto initValue = VariableStorage(data.args[i].type, 0);
			auto initType = initValue.getType();

			ignoreUnused(initType);
			jassert(data.args[i] == initType);

			//functionScope->allocate(parameters[i], initValue);

			if (auto cs = dynamic_cast<ClassScope*>(findClassScope(scope)))
			{
				ClassScope::LocalVariableInfo pInfo;
				//pInfo.id = parameters[i].toString();
				pInfo.isParameter = true;
				pInfo.type = initType;

				cs->addLocalVariableInfo(pInfo);
			}
		}

		try
		{
			FunctionParser p(compiler, *this);

			statements = p.parseStatementList();
			
			compiler->executePass(BaseCompiler::PreSymbolOptimization, functionScope, statements);
			compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, statements);
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, statements);
			compiler->executePass(BaseCompiler::SyntaxSugarReplacements, functionScope, statements);
			compiler->executePass(BaseCompiler::PostSymbolOptimization, functionScope, statements);

			functionScope->parentFunction = nullptr;

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

		AsmCodeGenerator::fillSignature(data, sig, false);
		cc->addFunc(sig);

		dynamic_cast<ClassCompiler*>(compiler)->setFunctionCompiler(cc);

		compiler->registerPool.clear();

		compiler->executePass(BaseCompiler::PreCodeGenerationOptimization, functionScope, statements);
		compiler->executePass(BaseCompiler::RegisterAllocation, functionScope, statements);
		compiler->executePass(BaseCompiler::CodeGeneration, functionScope, statements);

		cc->endFunc();
		cc->finalize();
		cc = nullptr;

		runtime->add(&data.function, ch);

		auto fClass = dynamic_cast<FunctionClass*>(scope);

		bool success = fClass->injectFunctionPointer(data);

		ignoreUnused(success);
		jassert(success);

		auto& as = dynamic_cast<ClassCompiler*>(compiler)->assembly;

		as << "; function " << data.getSignature() << "\n";
		as << l->data();

		ch->setLogger(nullptr);
		l = nullptr;
		ch = nullptr;

		compiler->setCurrentPass(BaseCompiler::FunctionCompilation);
	}
}

void Operations::SmoothedVariableDefinition::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		jassert(scope->getScopeType() == BaseScope::Class);

		if (auto cs = dynamic_cast<ClassScope*>(scope))
		{
			if (type == Types::ID::Float)
				cs->addFunctionClass(new SmoothedFloat<float>(id, iv.toFloat()));
			else if (type == Types::ID::Double)
				cs->addFunctionClass(new SmoothedFloat<double>(id, iv.toDouble()));
			else
				throwError("Wrong type for smoothed value");
		}

	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (type != iv.getType())
			logWarning("Type mismatch at smoothed value");
	}
}

void Operations::WrappedBlockDefinition::process(BaseCompiler* compiler, BaseScope* scope)
{
	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		jassert(scope->getScopeType() == BaseScope::Class);

		if (auto cs = dynamic_cast<ClassScope*>(scope))
		{
			cs->addFunctionClass(new WrappedBuffer<BufferHandler::WrapAccessor<>>(id, b));
		}
	}
}

void Operations::VariableReference::process(BaseCompiler* compiler, BaseScope* scope)
{
	Expression::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		// Abort further processing, it'll be resolved in PostSymbolOptimization
		if (BlockAccess::isWrappedBufferReference(this, scope))
			return;

		// We will create the Reference to the according scope for this
		// variable, check it's type and if it's a parameter, get the index...

		if (isLocalToScope)
		{
			if (scope->getScopeForSymbol(id) != nullptr)
				logWarning("Declaration hides previous variable definition");

			if (auto regScope = dynamic_cast<RegisterScope*>(scope))
			{
				regScope->localVariables.add(id);
			}

			auto initValue = VariableStorage(getType(), 0);

			if (type == Types::ID::Block)
			{
				auto globalScope = GlobalScope::getFromChildScope(scope);

				initValue = globalScope->getBufferHandler().getData(id.id);
			}

			if (isLocalConst)
				jassertfalse;
		}
		else if (auto vScope = scope->getScopeForSymbol(id))
		{
			if (auto rScope = dynamic_cast<RegisterScope*>(vScope))
			{
				for (const auto& l : rScope->localVariables)
					if (l == id)
						type = l.type;

				isLocalToScope = true;
				return;
			}
			if (auto fScope = dynamic_cast<FunctionScope*>(vScope))
			{
				parameterIndex = fScope->parameters.indexOf(id.id);

				if (parameterIndex != -1)
				{
					// This might be changed by the constant folding
					// so we have to reset it here...
					isLocalConst = false;

					type = fScope->data.args[parameterIndex].type;

					if (fScope == scope)
						isLocalToScope = true;

					return;
				}
			}
			
			type = scope->getRootData()->getTypeForVariable(id);

			if (vScope == scope)
			{
				isLocalToScope = true;
				jassertfalse;
				isLocalConst = false;
			}
		}
		else if (auto fc = dynamic_cast<FunctionClass*>(findClassScope(scope)))
		{
			if (fc->hasConstant(id))
			{
				functionClassConstant = fc->getConstantValue(id);
				type = functionClassConstant.getType();
			}

			

			if (functionClassConstant.isVoid())
				throwError(id.getParentSymbol().toString() + " does not have constant " + id.id.toString());
		}
		else
			throwError("Can't resolve variable " + id.toString());

		if (auto st = findParentStatementOfType<SyntaxTree>(this))
			st->addVariableReference(this);
		else
			jassertfalse;
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		id = id.withType(getType());
		// Nothing to do...
	}

#if 0
	bool initialiseVariables = (currentPass == BaseCompiler::RegisterAllocation && parameterIndex != -1) ||
		(currentPass == BaseCompiler::CodeGeneration && parameterIndex == -1);
#endif

	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (!functionClassConstant.isVoid())
		{
			auto immValue = VariableStorage(parent.get()->getType(), functionClassConstant.toDouble());


			Ptr c = new Immediate(location, immValue);

			replaceInParent(c);

			return;
		}

		// We need to initialise parameter registers before the rest
		if (parameterIndex != -1)
		{
			reg = compiler->registerPool.getRegisterForVariable(scope, id.withType(type));

			//reg = compiler->registerPool.getRegisterForVariable(ref);

			if (isFirstReference())
			{
				auto asg = CREATE_ASM_COMPILER(type);
				asg.emitParameter(reg, parameterIndex);
			}

			return;
		}
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (BlockAccess::isWrappedBufferReference(this, scope))
			return;

		if (parameterIndex != -1)
			return;

		// It might already be assigned to a reused register
		if (reg == nullptr)
			reg = compiler->registerPool.getRegisterForVariable(scope, id);

		if (reg->isActiveOrDirtyGlobalRegister() && findParentStatementOfType<ConditionalBranch>(this) != nullptr)
		{
			// the code generation has already happened before the branch so that we have the global register
			// available in any case
			return;
		}

		if (reg->isIteratorRegister())
			return;

		auto asg = CREATE_ASM_COMPILER(type);

		if (isFirstReference())
		{
			auto assignmentType = getWriteAccessType();

			auto rd = scope->getRootClassScope()->rootData.get();

			if (rd->contains(id))
			{
				auto* dataPointer = &rd->get(id);

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

void Operations::Assignment::process(BaseCompiler* compiler, BaseScope* scope)
{
	/*
	ResolvingSymbols: check that target is not const
	TypeCheck, = > check type match
	DeadCodeElimination, = > remove unreferenced local variables
	Inlining, = > make self assignment
	CodeGeneration, = > Store or Op
	*/

	/** Assignments might use the target register, so we need to process the target first*/
	if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration)
	{
		Statement::process(compiler, scope);
	}
	else
	{
		Expression::process(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		if (getSubExpr(0)->isConstExpr())
		{
			auto rd = scope->getRootClassScope()->rootData.get();

			if (rd->contains(getTargetVariable()->id))
				rd->get(getTargetVariable()->id) = getSubExpr(0)->getConstExprValue();
		}

		auto v = getTargetVariable();

		if (isLocalDefinition)
		{
			jassertfalse;
		}

		if (v->isLocalConst && !v->isFirstReference())
			throwError("Can't change constant variable");
	}


	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto expectedType = getTargetVariable()->getType();

		if (getTargetVariable()->isLocalToScope && getTargetVariable()->isFirstReference())
		{
			if (auto sc = dynamic_cast<ClassScope*>(findClassScope(scope)))
			{
				ClassScope::LocalVariableInfo localInfo;
				localInfo.isParameter = false;
				localInfo.id = getTargetVariable()->id;
				localInfo.type = expectedType;

				sc->localVariableInfo.add(localInfo);
			}
		}

		checkAndSetType(0, expectedType);
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto targetV = getTargetVariable();

		getSubExpr(0)->process(compiler, scope);


		targetV->process(compiler, scope);

		auto acg = CREATE_ASM_COMPILER(type);

		auto value = getSubRegister(0);
		auto tReg = getSubRegister(1);

		if (assignmentType == JitTokens::assign_)
		{
			if (tReg != value)
				acg.emitStore(tReg, value);
		}
		else
			acg.emitBinaryOp(assignmentType, tReg, value);

		if (scope->getRootClassScope()->rootData->contains(targetV->id))
		{
			acg.cc.setInlineComment("Write member variable");
			acg.emitMemoryWrite(targetV->reg);
		}

		//getSubRegister(0)->flagForReuseIfAnonymous();
	}
}

void Operations::ClassInstance::process(BaseCompiler* compiler, BaseScope* scope)
{
	COMPILER_PASS(BaseCompiler::SubclassCompilation)
	{
		if (scope->hasChildClass(classId))
		{
			auto cLocation = location;
			int length;

			scope->getChildClassCode(classId, cLocation.location, cLocation.program, length);

			for (auto instanceId : instanceIds)
			{
				ClassCompiler c(scope, instanceId);

				c.parentRuntime = dynamic_cast<ClassCompiler*>(compiler)->getRuntime();

				ScopedPointer<JitCompiledFunctionClass> classWrapper = c.compileAndGetScope(cLocation, length);

				if (!c.lastResult.wasOk())
					throwError("Error at class instantiation: " + c.lastResult.getErrorMessage());

				if (auto fc = dynamic_cast<FunctionClass*>(scope))
				{
					auto cScope = classWrapper->releaseClassScope();
					cScope->classTypeId = classId;
					fc->addFunctionClass(cScope);
				}
				else
					location.throwError("Illegal class instantiation");

				if (auto cc = dynamic_cast<ClassCompiler*>(compiler))
				{
					cc->assembly << "; " << classId << " instance " << instanceId.toString() << "\n";
					cc->assembly << c.assembly;
				}
			}
		}
	}
}

snex::jit::Symbol Operations::BlockAccess::isWrappedBufferReference(Statement::Ptr expr, BaseScope* scope)
{
	if (auto var = dynamic_cast<Operations::VariableReference*>(expr.get()))
	{
		if (auto s = scope->getScopeForSymbol(var->id))
		{
			if (auto cs = dynamic_cast<ClassScope*>(s))
			{
				if (auto wrappedBuffer = dynamic_cast<WrappedBufferBase*>(cs->getSubFunctionClass(var->id)))
				{
					return var->id;
				}
			}
		}

	}

	return {};
}

}
}