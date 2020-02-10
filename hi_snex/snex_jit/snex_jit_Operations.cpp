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
		jassert(scope->getScopeSymbol() == id.getParentSymbol());

		functionScope = new FunctionScope(scope, id.id);

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
			FunctionParser p(compiler, id, *this);

			statements = p.parseStatementList();
			
			compiler->executePass(BaseCompiler::PreSymbolOptimization, functionScope, statements);
			compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, statements);
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, statements);
			compiler->executePass(BaseCompiler::SyntaxSugarReplacements, functionScope, statements);
			compiler->executePass(BaseCompiler::PostSymbolOptimization, functionScope, statements);

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
	Expression::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		// Abort further processing, it'll be resolved in PostSymbolOptimization
		if (BlockAccess::isWrappedBufferReference(this, scope))
			return;

		// We will create the Reference to the according scope for this
		// variable, check it's type and if it's a parameter, get the index...

		if (auto vScope = scope->getScopeForSymbol(id))
		{
			if (!vScope->updateSymbol(id))
				location.throwError("Can't update symbol " + id.toString());

			if (auto fScope = dynamic_cast<FunctionScope*>(vScope))
				parameterIndex = fScope->parameters.indexOf(id.id);
		}
		else if(auto rScope = dynamic_cast<RegisterScope*>(scope))
		{
			// The type must have been set or it is a undefined variable
			if (getType() == Types::ID::Dynamic)
				location.throwError("Use of undefined variable " + id.toString());

			jassert(id.type != Types::ID::Dynamic);

			// This will be a definition of a local variable.
			rScope->localVariables.add(id);
		}
		else if (auto cScope = dynamic_cast<ClassScope*>(scope))
		{
			if (getType() == Types::ID::Dynamic)
				location.throwError("Use of undefined variable " + id.toString());

			if (auto subClassType = dynamic_cast<StructType*>(cScope->typePtr.get()))
				memberOffset = subClassType->getMemberOffset(id.id);
			else
				cScope->allocate(id);
		}
		else
		{
			location.throwError("Can't resolve symbol " + id.toString());
		}

		

		

#if 0
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
#endif

		if (auto st = findParentStatementOfType<SyntaxTree>(this))
			st->addVariableReference(this);
		else
			jassertfalse;
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		
		// Nothing to do...
	}

#if 0
	bool initialiseVariables = (currentPass == BaseCompiler::RegisterAllocation && parameterIndex != -1) ||
		(currentPass == BaseCompiler::CodeGeneration && parameterIndex == -1);
#endif

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
					auto asg = CREATE_ASM_COMPILER(type);
					asg.emitParameter(dynamic_cast<Function*>(fScope->parentFunction), reg, parameterIndex);
				}
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
		{
			reg = compiler->registerPool.getRegisterForVariable(scope, id);
			reg->setOffset(memberOffset);
		}
			
		if (reg->isActiveOrDirtyGlobalRegister() && findParentStatementOfType<ConditionalBranch>(this) != nullptr)
		{
			// the code generation has already happened before the branch so that we have the global register
			// available in any case
			return;
		}

		if (reg->isIteratorRegister())
			return;

		auto asg = CREATE_ASM_COMPILER(type);

		isFirstWrite = (!reg->isActiveOrDirtyGlobalRegister() && !reg->isMemoryLocation()) || isFirstReference();

		if (isFirstWrite)
		{
			auto assignmentType = getWriteAccessType();

			if (id.isReference() && assignmentType == JitTokens::assign_)
			{
				reg->createRegister(asg.cc);
				return;
			}

			auto rd = scope->getRootClassScope()->rootData.get();

			if (auto cScope = dynamic_cast<ClassScope*>(scope->getScopeForSymbol(id)))
			{
				if (cScope->typePtr != nullptr)
				{
					if (auto fScope = scope->getParentScopeOfType<FunctionScope>())
					{
						dynamic_cast<Function*>(fScope->parentFunction)->objectPtr;

						jassertfalse;

						return;
					}
				}
			}

			if (rd->contains(id))
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
		if (getSubExpr(0)->isConstExpr() && scope->getScopeType() == BaseScope::Class)
		{
			auto rd = scope->getRootClassScope()->rootData.get();

			auto target = getTargetVariable()->id;
			auto initValue = getSubExpr(0)->getConstExprValue();

			if (auto complexType = dynamic_cast<ClassScope*>(scope)->typePtr)
			{
				rd->initMemberData(complexType, target.id, initValue);
			}
			else if (rd->contains(getTargetVariable()->id))
			{
				auto target = getTargetVariable()->id;
				auto initValue = getSubExpr(0)->getConstExprValue();
				rd->initGlobalData(target, initValue);
			}
			else
				throwError("Weirdness 2000");
		}

		auto v = getTargetVariable();

		if (v->id.isConst() && !isFirstAssignment)
			throwError("Can't change constant variable");

		if (getTargetVariable()->id.isReference() && isFirstAssignment)
		{
			auto source = getSubExpr(0);
			auto pRef = new PointerReference(location);
			source->replaceInParent(pRef);
			pRef->addStatement(source);
			pRef->process(compiler, scope);
		}
	}


	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto expectedType = getTargetVariable()->getType();

		
		if (!(isFirstAssignment && getTargetVariable()->id.isReference()))
			checkAndSetType(0, expectedType);
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto targetV = getTargetVariable();

		auto acg = CREATE_ASM_COMPILER(type);

		if (isFirstAssignment && targetV->id.isReference())
		{
			targetV->process(compiler, scope);
			getSubExpr(0)->process(compiler, scope);

			auto tReg = targetV->reg;
			auto vReg = getSubRegister(0);

			acg.copyPointerAddress(tReg, vReg);
			return;
		}
		
		getSubExpr(0)->process(compiler, scope);

		targetV->process(compiler, scope);

		auto value = getSubRegister(0);
		auto tReg = getSubRegister(1);

		if (assignmentType == JitTokens::assign_)
		{
			if (tReg != value)
				acg.emitStore(tReg, value);
		}
		else
			acg.emitBinaryOp(assignmentType, tReg, value);

		if (tReg->isDirtyGlobalMemory())
		{
			acg.emitMemoryWrite(tReg);
		}
#if 0
		if (scope->getRootClassScope()->rootData->contains(targetV->id))
		{
			acg.cc.setInlineComment("Write member variable");
			acg.emitMemoryWrite(targetV->reg);
		}
#endif
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

void Operations::PointerReference::process(BaseCompiler* compiler, BaseScope* scope)
{
	Expression::process(compiler, scope);

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		if (auto v = dynamic_cast<VariableReference*>(getSubExpr(0).get()))
		{
			auto vScope = scope->getScopeForSymbol(v->id);

			if (vScope->getScopeType() == BaseScope::Class)
			{
				VariableStorage ptr((int)v->getType(), vScope->getRootData()->getDataPointer(v->id), true);
				auto n = new Immediate(location, ptr);
				v->replaceInParent(n);
			}
			else
				throwError("Can't take reference to local variable");
		}
		if (auto tOp = dynamic_cast<TernaryOp*>(getSubExpr(0).get()))
		{
			auto tb = tOp->getSubExpr(1);
			auto fb = tOp->getSubExpr(2);

			auto ptb = new PointerReference(location);
			tb->replaceInParent(ptb);
			ptb->addStatement(tb);

			auto pfb = new PointerReference(location);
			fb->replaceInParent(pfb);
			pfb->addStatement(fb);

			pfb->process(compiler, scope);
			ptb->process(compiler, scope);
		}
	}

	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		reg = getSubRegister(0);
	}
}

}
}