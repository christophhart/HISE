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


bool Operations::StatementBlock::isRealStatement(Statement* s)
{
	if (dynamic_cast<InlinedArgument*>(s) != nullptr)
		return false;

	if (dynamic_cast<Noop*>(s) != nullptr)
		return false;

	if (as<ReturnStatement>(s))
		return s->getType() != Types::ID::Void;

	if (dynamic_cast<VariableReference*>(s) != nullptr)
		return false;

	return true;
}

void Operations::StatementBlock::process(BaseCompiler* compiler, BaseScope* scope)
{
	auto bs = createOrGetBlockScope(scope);

	processBaseWithChildren(compiler, bs);

	auto path = getPath();

	

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		removeStatementsAfterReturn();
		addDestructors(scope);
	}

	COMPILER_PASS(BaseCompiler::RegisterAllocation)
	{
		if (hasReturnType())
		{
			bool useReturnRegister = true;

			if (isInlinedFunction)
			{
				int returnStatements = 0;

				forEachRecursive([&](Ptr p)
				{
					if (auto rt = as<ReturnStatement>(p))
					{
						jassert(rt->findRoot() == as<ScopeStatementBase>(this));
						returnStatements++;
					}

					return false;
				}, IterationType::NoChildInlineFunctionBlocks);

				jassert(returnStatements > 0);

				useReturnRegister = returnStatements >= 2;
			}

			if (useReturnRegister)
				allocateReturnRegister(compiler, bs);
		}

		reg = returnRegister;
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (isInlinedFunction && endLabel.isValid())
		{
			auto acg = CREATE_ASM_COMPILER(getType());
			acg.cc.setInlineComment("end of inline function");
			acg.cc.bind(endLabel);
		}
	}
#endif
}

snex::jit::BaseScope* Operations::StatementBlock::createOrGetBlockScope(BaseScope* parent)
{
	if (parent->getScopeType() == BaseScope::Class)
		return parent;

	if (blockScope == nullptr)
		blockScope = new RegisterScope(parent, getPath());

	return blockScope;
}

snex::jit::Operations::InlinedArgument* Operations::StatementBlock::findInlinedParameterInParentBlocks(Statement* p, const Symbol& s)
{
	if (p == nullptr)
		return nullptr;

	if (auto parentInlineArgument = findParentStatementOfType<InlinedArgument>(p))
	{
		auto parentBlock = findParentStatementOfType<StatementBlock>(parentInlineArgument);

		auto ipInParent = findInlinedParameterInParentBlocks(parentBlock->parent, s);

		if (ipInParent != nullptr)
			return ipInParent;

	}


	if (auto sb = dynamic_cast<StatementBlock*>(p))
	{
		if (sb->isInlinedFunction)
		{
			for (auto c : *sb)
			{
				if (auto ia = dynamic_cast<InlinedArgument*>(c))
				{
					if (ia->s == s)
						return ia;
				}
			}

			//return nullptr;
		}
	}

	p = p->parent.get();

	if (p != nullptr)
		return findInlinedParameterInParentBlocks(p, s);

	return nullptr;
}

void Operations::StatementBlock::addInlinedReturnJump(AsmJitX86Compiler& cc)
{
	jassert(isInlinedFunction);

#if SNEX_ASMJIT_BACKEND
	if (!endLabel.isValid())
		endLabel = cc.newLabel();

	cc.jmp(endLabel);
#endif
}

Operations::Statement::Ptr Operations::StatementBlock::getThisExpression()
{
	Ptr expr;

	forEachRecursive([&expr](Ptr p)
	{
		if (auto ia = as<InlinedArgument>(p))
		{
			if (ia->argIndex == -1)
			{
				expr = ia->getSubExpr(0);

				if (auto sb = as<StatementBlock>(ia->getSubExpr(0)))
					expr = sb->getThisExpression();
				
				return true;
			}
		}

		return false;
	}, IterationType::NoChildInlineFunctionBlocks);

	if (expr == nullptr)
		location.throwError("Can't find this pointer");

	return expr;
}

void Operations::ReturnStatement::process(BaseCompiler* compiler, BaseScope* scope)
{
	if (compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
		processBaseWithChildren(compiler, scope);
	else
		processBaseWithoutChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (auto fScope = dynamic_cast<FunctionScope*>(findFunctionScope(scope)))
		{
			TypeInfo actualType(Types::ID::Void);

			if (auto first = getReturnValue())
				actualType = first->getTypeInfo();

			if (isVoid() && actualType != Types::ID::Void)
				throwError("Can't return a value from a void function.");
			if (!isVoid() && actualType == Types::ID::Void)
				throwError("function must return a value");

			if(!isVoid())
				setTypeForChild(0, getTypeInfo());

			//checkAndSetType(0, getTypeInfo());
		}
		else
			throwError("Can't deduce return type.");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto t = getTypeInfo().toPointerIfNativeRef();

		auto asg = CREATE_ASM_COMPILER(t.getType());

		AsmCodeGenerator::TemporaryRegister tmpReg(asg, scope, getTypeInfo());
		bool useCopyReg = false;
		
		auto isNonReferenceReturn = !isVoid() && !getTypeInfo().isRef();
		auto hasDestructors = getNumChildStatements() > 1;

		if (isNonReferenceReturn && hasDestructors)
		{
			// process the return expression
			getSubExpr(0)->process(compiler, scope);

			// There are a few destructors around, which might alter the return
			// value, so we need to copy the return value
			asg.emitStore(tmpReg.tempReg, getSubRegister(0));
			useCopyReg = true;

			for (int i = 1; i < getNumChildStatements(); i++)
			{
				// Now we can execute the destructors...
				getSubExpr(i)->process(compiler, scope);
			}
		}
		else if (isVoid())
		{
			for (auto s : *this)
				s->process(compiler, scope);
		}
		else
		{
			getSubExpr(0)->process(compiler, scope);
		}

		if (!isVoid())
		{
			if (auto sb = findInlinedRoot())
			{
				reg = getSubRegister(0);

				// if it has a register, it means that there is branching going on...
				if (sb->reg != nullptr)
				{
					asg.emitStore(sb->reg, reg);
					reg->clearAfterReturn();
					sb->addInlinedReturnJump(asg.cc);
				}
				else
					sb->reg = reg;

				if (reg != nullptr && reg->isActive())
					jassert(reg->isValid());
			}
			else if (auto sl = findRoot())
			{
				reg = sl->getReturnRegister();

				if (reg != nullptr && reg->isActive())
					jassert(reg->isValid());
			}

			if (reg == nullptr)
				throwError("Can't find return register");

			if (reg->isActive())
				jassert(reg->isValid());
		}

		if (findInlinedRoot() == nullptr)
		{
			auto sourceReg = isVoid() ? nullptr : getSubRegister(0);

			if (useCopyReg)
				sourceReg = tmpReg.tempReg;

			asg.emitReturn(compiler, reg, sourceReg);
		}
		else
		{
			asg.writeDirtyGlobals(compiler);
		}
	}
#endif
}

snex::jit::Operations::StatementBlock* Operations::ReturnStatement::findInlinedRoot() const
{
	if (auto sl = findRoot())
	{
		if (auto sb = dynamic_cast<StatementBlock*>(sl))
		{
			if (sb->isInlinedFunction)
			{
				return sb;
			}
		}
	}

	return nullptr;
}

snex::jit::Operations::ScopeStatementBase* Operations::ReturnStatement::findRoot() const
{
	Ptr p = parent.get();

	while (p != nullptr)
	{
		if (auto st = as<SyntaxTree>(p))
		{
			return st;
		}

		if (auto sb = as<StatementBlock>(p))
		{
			if (sb->isInlinedFunction)
				return sb;
		}

		p = p->parent.get();
	}

	jassertfalse;
	return nullptr;

	//return ScopeStatementBase::getStatementListWithReturnType(const_cast<ReturnStatement*>(this));
}

void Operations::TernaryOp::process(BaseCompiler* compiler, BaseScope* scope)
{
	// We need to have precise control over the code generation
	// for the subexpressions to avoid execution of both branches
	if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration)
		processBaseWithoutChildren(compiler, scope);
	else
		processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		type = checkAndSetType(1, type);
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto asg = CREATE_ASM_COMPILER(getType());
		reg = asg.emitTernaryOp(this, compiler, scope);
		jassert(reg->isActive());
	}
#endif
}

void Operations::WhileLoop::process(BaseCompiler* compiler, BaseScope* scope)
{
	scope = getScopeToUse(scope);

	if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration)
		Statement::processBaseWithoutChildren(compiler, scope);
	else
		Statement::processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto cond = getLoopChildStatement(ChildStatementType::Condition);

		if (cond->isConstExpr())
		{
			auto v = cond->getConstExprValue();

			if (v.toInt() != 0)
				throwError("Endless loop detected");
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		preallocateVariableRegistersBeforeBranching(getLoopChildStatement(ChildStatementType::Body), compiler, scope);

		if (auto li = getLoopChildStatement(ChildStatementType::Initialiser))
			li->process(compiler, scope);

		auto acg = CREATE_ASM_COMPILER(Types::ID::Integer);
		auto safeCheck = scope->getGlobalScope()->isRuntimeErrorCheckEnabled();
		auto cond = acg.cc.newLabel();
		auto exit = acg.cc.newLabel();
		continueTarget = acg.cc.newLabel();
		auto why = acg.cc.newGpd();

		if (safeCheck)
			acg.cc.xor_(why, why);

		acg.cc.nop();
		acg.cc.bind(cond);

		breakTarget = exit;

		auto cp = getCompareCondition();

		if (cp != nullptr)
			cp->useAsmFlag = true;

		getLoopChildStatement(ChildStatementType::Condition)->process(compiler, scope);
		auto cReg = getSubRegister(0);

		if (cp != nullptr)
		{
#define INT_COMPARE(token, command) if (cp->op == token) command(exit);

			INT_COMPARE(JitTokens::greaterThan, acg.cc.jle);
			INT_COMPARE(JitTokens::lessThan, acg.cc.jge);
			INT_COMPARE(JitTokens::lessThanOrEqual, acg.cc.jg);
			INT_COMPARE(JitTokens::greaterThanOrEqual, acg.cc.jl);
			INT_COMPARE(JitTokens::equals, acg.cc.jne);
			INT_COMPARE(JitTokens::notEquals, acg.cc.je);

#undef INT_COMPARE

			if (safeCheck)
			{
				acg.cc.inc(why);
				acg.cc.cmp(why, 10000000);
				auto okBranch = acg.cc.newLabel();
				acg.cc.jb(okBranch);

				auto errorMemReg = acg.cc.newGpq();
				acg.cc.mov(errorMemReg, scope->getGlobalScope()->getRuntimeErrorFlag());
				auto errorFlag = x86::ptr(errorMemReg).cloneResized(4);
				acg.cc.mov(why, (int)RuntimeError::ErrorType::WhileLoop);
				acg.cc.mov(errorFlag, why);
				acg.cc.mov(why, (int)location.getLine());
				acg.cc.mov(errorFlag.cloneAdjustedAndResized(4, 4), why);
				acg.cc.mov(why, (int)location.getColNumber());
				acg.cc.mov(errorFlag.cloneAdjustedAndResized(8, 4), why);
				acg.cc.jmp(exit);
				acg.cc.bind(okBranch);
			}
		}
		else
		{
			acg.cc.setInlineComment("check condition");
			acg.cc.cmp(INT_REG_R(cReg), 0);
			acg.cc.je(exit);

			if (safeCheck)
			{
				acg.cc.inc(why);
				acg.cc.cmp(why, 10000000);
				auto okBranch = acg.cc.newLabel();
				acg.cc.jb(okBranch);

				auto errorFlagReg = acg.cc.newGpq();
				acg.cc.mov(errorFlagReg, scope->getGlobalScope()->getRuntimeErrorFlag());
				auto errorFlag = x86::ptr(errorFlagReg).cloneResized(4);
				acg.cc.mov(why, (int)RuntimeError::ErrorType::WhileLoop);
				acg.cc.mov(errorFlag, why);
				acg.cc.mov(why, (int)location.getLine());
				acg.cc.mov(errorFlag.cloneAdjustedAndResized(4, 4), why);
				acg.cc.mov(why, (int)location.getColNumber());
				acg.cc.mov(errorFlag.cloneAdjustedAndResized(8, 4), why);
				acg.cc.jmp(exit);
				acg.cc.bind(okBranch);
			}
		}

		getLoopChildStatement(ChildStatementType::Body)->process(compiler, scope);


		if (auto pb = getLoopChildStatement(ChildStatementType::PostBodyOp))
		{
			acg.cc.bind(continueTarget);
			pb->process(compiler, scope);
		}
		else
		{
			continueTarget = cond;
		}
		
		acg.cc.jmp(cond);
		acg.cc.bind(exit);
	}
#endif
}

snex::jit::Operations::Compare* Operations::WhileLoop::getCompareCondition()
{
	if (auto cp = as<Compare>(getLoopChildStatement(ChildStatementType::Condition)))
		return cp;

	if (auto sb = as<StatementBlock>(getLoopChildStatement(ChildStatementType::Condition)))
	{
		for (auto s : *sb)
		{
			if (auto cb = as<ConditionalBranch>(s))
				return nullptr;

			if (auto rt = as<ReturnStatement>(s))
			{
				return as<Compare>(rt->getSubExpr(0));
			}
		}
	}

	return nullptr;
}

snex::jit::BaseScope* Operations::WhileLoop::getScopeToUse(BaseScope* outerScope)
{
	if (loopType == LoopType::For)
	{
		if (forScope == nullptr)
		{
			forScope = new RegisterScope(outerScope, outerScope->getScopeSymbol().getChildId("for_loop"));
			
		}

		return forScope;
	}

	return outerScope;
}

Operations::Statement::Ptr Operations::WhileLoop::getLoopChildStatement(ChildStatementType t)
{
	if (loopType == LoopType::While)
	{
		switch (t)
		{
		case ChildStatementType::Initialiser:
		case ChildStatementType::PostBodyOp: return nullptr;

		case ChildStatementType::Condition: return getSubExpr(0);
		case ChildStatementType::Body:		return getSubExpr(1);
		default: jassertfalse; return nullptr;
		}
	}
	else
	{
		switch (t)
		{
		case ChildStatementType::Initialiser: return getSubExpr(0);
		case ChildStatementType::Condition: return getSubExpr(1);
		case ChildStatementType::Body:		return getSubExpr(2);
		case ChildStatementType::PostBodyOp: return getSubExpr(3);
		default: jassertfalse; return nullptr;
		}
	}
}

void Operations::Loop::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

	if (compiler->getCurrentPass() != BaseCompiler::DataAllocation &&
		compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
	{
		getTarget()->process(compiler, scope);
		getLoopBlock()->process(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		tryToResolveType(compiler);

		getTarget()->process(compiler, scope);

		auto targetType = getTarget()->getTypeInfo();

		if (auto sp = targetType.getTypedIfComplexType<SpanType>())
		{
			loopTargetType = Span;

            numElements = sp->getNumElements();
            
			if (iterator.typeInfo.isDynamic())
				iterator.typeInfo = sp->getElementType();
			else if (iterator.typeInfo != sp->getElementType())
				location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + sp->getElementType().toString());
		}
		else if (auto dt = targetType.getTypedIfComplexType<DynType>())
		{
			loopTargetType = Dyn;

			if (iterator.typeInfo.isDynamic())
				iterator.typeInfo = dt->elementType;
			else if (iterator.typeInfo != dt->elementType)
				location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + sp->getElementType().toString());
		}
		else if (targetType.getType() == Types::ID::Block)
		{
			loopTargetType = Dyn;

			if (iterator.typeInfo.isDynamic())
				iterator.typeInfo = TypeInfo(Types::ID::Float, iterator.isConst(), iterator.isReference());
			else if (iterator.typeInfo.getType() != Types::ID::Float)
				location.throwError("Illegal iterator type");
		}
		else
		{
			if (auto st = targetType.getTypedIfComplexType<StructType>())
			{
				FunctionClass::Ptr fc = st->getFunctionClass();

				customBegin = fc->getSpecialFunction(FunctionClass::BeginIterator);
				customSizeFunction = fc->getSpecialFunction(FunctionClass::SizeFunction);

				if (!customBegin.isResolved() || !customSizeFunction.isResolved())
					throwError(st->toString() + " does not have iterator methods");



				loopTargetType = CustomObject;

				if (iterator.typeInfo.isDynamic())
					iterator.typeInfo = customBegin.returnType;
				else if (iterator.typeInfo != customBegin.returnType)
					location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + customBegin.returnType.toString());

			}
			else
			{
				throwError("Can't deduce loop target type");
			}


		}


		compiler->namespaceHandler.setTypeInfo(iterator.id, NamespaceHandler::Variable, iterator.typeInfo);

		getLoopBlock()->process(compiler, scope);

		evaluateIteratorLoad();
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto acg = CREATE_ASM_COMPILER(compiler->getRegisterType(iterator.typeInfo));

		getTarget()->process(compiler, scope);

		auto t = getTarget();

		auto r = getTarget()->reg;

		jassert(r != nullptr && r->getScope() != nullptr);

		preallocateVariableRegistersBeforeBranching(getLoopBlock(), compiler, scope);

		if (loopTargetType == Span)
		{
			auto le = new SpanLoopEmitter(compiler, iterator, getTarget()->reg, getLoopBlock(), loadIterator);
			le->typePtr = getTarget()->getTypeInfo().getTypedComplexType<SpanType>();
			loopEmitter = le;
		}
		else if (loopTargetType == Dyn)
		{
			auto le = new DynLoopEmitter(compiler, iterator, getTarget()->reg, getLoopBlock(), loadIterator);
			le->typePtr = getTarget()->getTypeInfo().getTypedComplexType<DynType>();
			loopEmitter = le;
		}
		else if (loopTargetType == CustomObject)
		{
			auto le = new CustomLoopEmitter(compiler, iterator, getTarget()->reg, getLoopBlock(), loadIterator);
			le->beginFunction = customBegin;
			le->sizeFunction = customSizeFunction;
			loopEmitter = le;
		}

		if (loopEmitter != nullptr)
			loopEmitter->emitLoop(acg, compiler, scope);
	}
#endif
}

bool Operations::Loop::evaluateIteratorLoad()
{
	if (!loadIterator)
		return false;

	SyntaxTreeWalker w(getLoopBlock(), false);

	while (auto v = w.getNextStatementOfType<VariableReference>())
	{
		if (v->id == iterator)
		{
			if (auto a = findParentStatementOfType<Assignment>(v))
			{
				if (a->getSubExpr(1).get() == v && a->assignmentType == JitTokens::assign_)
				{
					auto sId = v->id;

					bool isSelfAssign = a->getSubExpr(0)->forEachRecursive([sId](Operations::Statement::Ptr p)
					{
						if (auto v = dynamic_cast<VariableReference*>(p.get()))
						{
							if (v->id == sId)
								return true;
						}

						return false;
					}, IterationType::AllChildStatements);

					loadIterator = isSelfAssign;
				}

				if (a->assignmentType != JitTokens::assign_)
					loadIterator = true;

				if (a->getSubExpr(1).get() != v)
					loadIterator = true;
			}

			break;
		}
	}

	return loadIterator;
}

bool Operations::Loop::evaluateIteratorStore()
{
	if (storeIterator)
		return true;

	SyntaxTreeWalker w(getLoopBlock(), false);

	while (auto v = w.getNextStatementOfType<VariableReference>())
	{
		if (v->id == iterator)
		{
			if (v->parent->hasSideEffect())
			{
				if (auto a = as<Assignment>(v->parent.get()))
				{
					if (a->getSubExpr(0).get() == v)
						continue;
				}

				storeIterator = true;
				break;
			}
		}
	}

	return storeIterator;
}

bool Operations::Loop::tryToResolveType(BaseCompiler* compiler)
{
	getTarget()->tryToResolveType(compiler);

	auto tt = getTarget()->getTypeInfo();

	if (auto targetType = tt.getTypedIfComplexType<ArrayTypeBase>())
	{
		auto r = compiler->namespaceHandler.setTypeInfo(iterator.id, NamespaceHandler::Variable, targetType->getElementType());

		auto iteratorType = targetType->getElementType().withModifiers(iterator.isConst(), iterator.isReference());

		iterator = { iterator.id, iteratorType };

		if (r.failed())
			throwError(r.getErrorMessage());
	}

	if (auto fpType = tt.getTypedIfComplexType<StructType>())
	{
		if (fpType->id == NamespacedIdentifier("FrameProcessor"))
		{
			TypeInfo floatType(Types::ID::Float, false, true);

			auto r = compiler->namespaceHandler.setTypeInfo(iterator.id, NamespaceHandler::Variable, floatType);

			iterator = { iterator.id, floatType };

			if (r.failed())
				throwError(r.getErrorMessage());
		}
	}

	Statement::tryToResolveType(compiler);

	return true;
}

void Operations::ControlFlowStatement::process(BaseCompiler* compiler, BaseScope* scope)
{
	if (parentLoop == nullptr)
	{
		Ptr p = parent.get();

		while (p != nullptr)
		{
			if (as<WhileLoop>(p) || as<Loop>(p))
			{
				parentLoop = as<ConditionalBranch>(p);
				break;
			}

			p = p->parent;
		}
	}

	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		

		if (parentLoop == nullptr)
		{
			juce::String s;
			s << "a " << getStatementId().toString() << " may only be used within a loop or switch";
			throwError(s);
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto acg = CREATE_ASM_COMPILER(Types::ID::Integer);
		acg.emitLoopControlFlow(parentLoop, isBreak);
	}
#endif
}

snex::jit::Operations::ScopeStatementBase* Operations::ControlFlowStatement::findRoot() const
{
	jassert(parentLoop != nullptr);

	Ptr p = parent.get();

	auto parentLoopAsPtr = dynamic_cast<Statement*>(parentLoop.get());

	while (p != nullptr)
	{
		if (p->parent == parentLoopAsPtr)
		{
			return dynamic_cast<ScopeStatementBase*>(p.get());
		}

		p = p->parent;
	}

	return nullptr;
}

void Operations::IfStatement::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithoutChildren(compiler, scope);

	if (compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
		processAllChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		processAllChildren(compiler, scope);

		if (getCondition()->getTypeInfo() != Types::ID::Integer)
			throwError("Condition must be boolean expression");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto acg = CREATE_ASM_COMPILER(Types::ID::Integer);

		

		auto cond = dynamic_cast<Expression*>(getCondition().get());
		auto trueBranch = getTrueBranch();
		auto falseBranch = getFalseBranch();

		acg.emitBranch(TypeInfo(Types::ID::Void), cond, trueBranch.get(), falseBranch.get(), compiler, scope);
	}
#endif
}

}
}
