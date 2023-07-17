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
using namespace Operations;

void OptimizationPass::replaceWithNoop(StatementPtr s)
{
	replaceExpression(s, new Noop(s->location));
}

void OptimizationPass::processPreviousPasses(BaseCompiler* c, BaseScope* s, StatementPtr st)
{
	for (int i = BaseCompiler::ComplexTypeParsing; i < (int)c->getCurrentPass(); i++)
	{
		auto p = (BaseCompiler::Pass)i;

		if (BaseCompiler::isOptimizationPass(p))
			continue;

		BaseCompiler::ScopedPassSwitcher svs(c, p);

		c->executePass(p, s, st.get());
	}
}

struct VOps
{
#define VAR_OP(name, opChar) static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(l.getType(), l.toDouble() opChar r.toDouble()); }
#define VAR_OP_INT(name, opChar) static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(l.getType(), l.toInt() opChar r.toInt()); }
#define VAR_OP_CMP(name, opChar)  static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(Types::ID::Integer, l.toDouble() opChar r.toDouble()); }

	VAR_OP(plus, +);
	VAR_OP(times, *);
	VAR_OP(divide, / );
	VAR_OP(minus, -);
	VAR_OP_INT(modulo, %);
	VAR_OP_CMP(greaterThan, >);
	VAR_OP_CMP(lessThan, < );
	VAR_OP_CMP(greaterThanOrEqual, >= );
	VAR_OP_CMP(lessThanOrEqual, <= );
	VAR_OP_CMP(equals, == );
	VAR_OP_CMP(notEquals, != );
	VAR_OP_INT(logicalAnd, &&);
	VAR_OP_INT(logicalOr, ||);

#undef VAR_OP
#undef VAR_OP_INT
};

ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalDotOperator(BaseScope* s, Operations::DotOperator* dot)
{
	if (auto pv = dynamic_cast<Operations::VariableReference*>(dot->getDotParent().get()))
	{
		if (auto fc = pv->getFunctionClassForSymbol(s))
		{
			if (auto cv = dynamic_cast<Operations::VariableReference*>(dot->getDotChild().get()))
			{
				if (fc->hasConstant(cv->id.id))
				{
					return new Operations::Immediate(dot->location, fc->getConstantValue(cv->id.id));
				}
			}
			if (auto cf = dynamic_cast<Operations::FunctionCall*>(dot->getDotChild().get()))
			{
				return evalConstMathFunction(cf);
			}
		}
	}

	return nullptr;
}



bool ConstExprEvaluator::processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	using namespace Operations;

	// run both times, since something could become constexpr during symbol resolving...
	if(compiler->getCurrentPass() == BaseCompiler::PostSymbolOptimization ||
	   compiler->getCurrentPass() == BaseCompiler::PreSymbolOptimization)
	{
		

		if (auto fc = as<FunctionCall>(statement))
		{
			if (auto constResult = evalConstMathFunction(fc))
			{
				statement->logOptimisationMessage("Remove const Math function call");
				replaceExpression(statement, constResult);
				return true;
			}
		}

		if (auto dot = as<DotOperator>(statement))
		{
			if (auto constResult = evalDotOperator(s, dot))
			{
				statement->logOptimisationMessage("Remove const dot operator");
				replaceExpression(statement, constResult);
				return true;
			}
		}

		if (auto is = as<BranchingStatement>(statement))
		{
			is->getCondition()->process(compiler, s);

			auto cond = as<Expression>(statement->getSubExpr(0));

			if (is->getCondition()->isConstExpr())
			{
				int v = cond->getConstExprValue().toInt();

				statement->logOptimisationMessage("Remove dead branch");

				if (v == 1)
					replaceExpression(statement, is->getTrueBranch());
				else
				{
					if (auto falseBranch = is->getFalseBranch())
						replaceExpression(statement, falseBranch);
					else
						replaceWithNoop(statement);
				}

				return true;
			}
		}

		if (auto c = as<Cast>(statement))
		{
			if (c->getTypeInfo().isTemplateType())
				jassertfalse;

			if (auto imm = evalCast(c->getSubExpr(0), c->getType()))
			{
				replaceExpression(c, imm);
				return true;
			}
		}

		if (auto a = as<Assignment>(statement))
		{
			a->getSubExpr(0)->process(compiler, s);

			if (a->getSubExpr(0)->isConstExpr())
			{
				auto value = a->getSubExpr(0)->getConstExprValue();

				a->getSubExpr(1)->process(compiler, s);
			}
		}

		if (auto cOp = as<Compare>(statement))
		{
			cOp->getSubExpr(0)->process(compiler, s);
			cOp->getSubExpr(1)->process(compiler, s);

			if (auto constexprResult = evalBinaryOp(cOp->getSubExpr(0), cOp->getSubExpr(1), cOp->op))
			{
				statement->logOptimisationMessage("Folded comparison");
				replaceExpression(statement, constexprResult);
				return true;
			}
		}

		if (auto bOp = as<BinaryOp>(statement))
		{
			bOp->getSubExpr(0)->process(compiler, s);
			bOp->getSubExpr(1)->process(compiler, s);

			if (auto constExprBinaryOp = evalBinaryOp(bOp->getSubExpr(0), bOp->getSubExpr(1), bOp->op))
			{
				statement->logOptimisationMessage("Folded binary op");
				replaceExpression(statement, constExprBinaryOp);
				return true;
			}
			else if (bOp->isLogicOp())
			{
				if (bOp->getSubExpr(1)->isConstExpr())
				{
					bOp->swapSubExpressions(0, 1);
					return true;
				}

				if (bOp->getSubExpr(0)->isConstExpr())
				{
					auto lValue = bOp->getSubExpr(0)->getConstExprValue();

					auto r = bOp->getSubExpr(1);

					if (bOp->op == JitTokens::logicalAnd && lValue.toInt() == 0)
					{
						statement->logOptimisationMessage("short-circuit constant && op");
						replaceExpression(statement, new Operations::Immediate(statement->location, 0));
						return true;
					}
					else if (bOp->op == JitTokens::logicalOr && lValue.toInt() == 1)
					{
						statement->logOptimisationMessage("short-circuit constant || op");
						replaceExpression(statement, new Operations::Immediate(statement->location, 1));
						return true;
					}
					else
					{
						statement->logOptimisationMessage("removed constant condition in logic op");
						replaceExpression(statement, bOp->getSubExpr(1));
						return true;
					}
				}
			}

		}
	}

	return false;

#if 0

	for (auto s : *tree)
	{
		if (auto v = as<Operations::VariableReference>(s))
		{
			// Keep the first reference to a const variable alive
			// so that the parent assignment can do it's job
			bool isConstInitialisation = v->ref->isConst && v->isFirstReference();

			if (!v->isClassVariable() && v->ref->isConst && !isConstInitialisation)
				replaceWithImmediate(v, v->ref->getDataCopy());
		}
		if (auto a = as<Operations::Assignment>(s))
		{
			auto target = a->getTargetVariable();

			if (target->isLocalToScope && a->isLastAssignmentToTarget() && target->isFirstReference() && a->getSubExpr(1)->isConstExpr())
				target->ref->isConst = true;

			if (a->assignmentType == JitTokens::assign_ &&
				a->getSubExpr(1)->isConstExpr())
			{
				bool allowInitialisation = !target->isLocalConst ||
					target->isFirstReference();

				auto& value = target->ref->getDataReference(allowInitialisation);
				value = dynamic_cast<Operations::Immediate*>(a->getSubExpr(1).get())->v;
			}
		}
		if (auto c = as<Operations::Cast>(s))
		{
			if (auto ce = evalCast(c->getSubExpr(0), c->getType()))
				c->replaceInParent(ce);
		}
		if (auto b = as<Operations::BinaryOp>(s))
		{
			if (auto ce = evalBinaryOp(b->getSubExpr(0), b->getSubExpr(1), b->op))
				b->replaceInParent(ce);
			else
			{
				if (b->getSubExpr(1)->isConstExpr())
				{
					ExprPtr old = b->getSubExpr(1);

					ExprPtr newPtr = ConstExprEvaluator::createInvertImmediate(b->getSubExpr(1), b->op);

					if (old != newPtr)
					{
						auto oldOp = b->op;
						old->replaceInParent(newPtr);

						if (b->op == JitTokens::divide) b->op = JitTokens::times;
						if (b->op == JitTokens::minus) b->op = JitTokens::plus;
						String m;
						m << "Replaced " << oldOp << " with " << b->op;
						b->logOptimisationMessage(m);
					}

				}
			}
		}
		if (auto n = as<Operations::Negation>(s))
		{
			if (auto ce = evalNegation(n->getSubExpr(0)))
				n->replaceInParent(ce);
		}
}

#endif
}

void ConstExprEvaluator::addConstKeywordToSingleWriteVariables(Operations::VariableReference* , BaseScope* , BaseCompiler* )
{
	// This is flawed: globals, function return values, etc. need to be detected properly...
#if 0
	if (!v->isLocalConst && !v->isParameter(s))
	{
		SyntaxTreeWalker w(v);

		int numWriteAccesses = 0;

		while (auto otherV = w.getNextStatementOfType<Operations::VariableReference>())
		{
			if (otherV->isBeingWritten())
				numWriteAccesses++;
		}

		if (numWriteAccesses == 1)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Added const to " + v->id.toString());
			v->isLocalConst = true;
		}
	}
#endif
}

void ConstExprEvaluator::replaceWithImmediate(ExprPtr e, const VariableStorage& value)
{

	StatementPtr s(e.get());

	replaceExpression(s, new Operations::Immediate(e->location, value));
}

snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalBinaryOp(ExprPtr left, ExprPtr right, OpType op)
{
	if (left->isConstExpr() && right->isConstExpr())
	{
		VariableStorage result;
		VariableStorage leftValue = left->getConstExprValue();
		VariableStorage rightValue = right->getConstExprValue();

#define REPLACE_WITH_CONST_OP(x) if (op == JitTokens::x) result = VOps::x(leftValue, rightValue);

		REPLACE_WITH_CONST_OP(plus);
		REPLACE_WITH_CONST_OP(minus);
		REPLACE_WITH_CONST_OP(times);
		REPLACE_WITH_CONST_OP(divide);
		REPLACE_WITH_CONST_OP(modulo);
		REPLACE_WITH_CONST_OP(greaterThan);
		REPLACE_WITH_CONST_OP(greaterThanOrEqual);
		REPLACE_WITH_CONST_OP(lessThan);
		REPLACE_WITH_CONST_OP(lessThanOrEqual);
		REPLACE_WITH_CONST_OP(equals);
		REPLACE_WITH_CONST_OP(notEquals);
		REPLACE_WITH_CONST_OP(logicalAnd);
		REPLACE_WITH_CONST_OP(logicalOr);

		return new Operations::Immediate(left->location, result);
	}

	return nullptr;
}


snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalNegation(ExprPtr expr)
{
	if (expr->isConstExpr())
	{
		auto result = VOps::times(expr->getConstExprValue(), -1.0);
		return new Operations::Immediate(expr->location, result);
	}

	return nullptr;
}



ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalCast(ExprPtr expression, Types::ID targetType)
{
	jassert(targetType != Types::ID::Dynamic);

	if (expression->isConstExpr())
	{
		auto value = expression->getConstExprValue();
		return new Operations::Immediate(expression->location, VariableStorage(targetType, value.toDouble()));
	}

	return nullptr;
}


snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::createInvertImmediate(ExprPtr immediate, OpType op)
{
	auto v = immediate->getConstExprValue().toDouble();

	if (op == JitTokens::minus) v *= -1.0;
	else if (op == JitTokens::divide) v = 1.0 / v;
	else return immediate;

	return new Operations::Immediate(immediate->location, VariableStorage(immediate->getType(), v));
}



snex::jit::OptimizationPass::ExprPtr ConstExprEvaluator::evalConstMathFunction(Operations::FunctionCall* functionCall)
{
	if (functionCall->callType == Operations::FunctionCall::ApiFunction)
	{
		
		auto& m = functionCall->currentCompiler->getMathFunctionClass();

		if (auto t = dynamic_cast<Operations::VariableReference*>(functionCall->getObjectExpression().get()))
		{
			if (!(t->id.id == m.getClassName()))
				return nullptr;
		}

		Array<FunctionData> matches;
		auto symbol = m.getClassName().getChildId(functionCall->function.id.id);

		m.addMatchingFunctions(matches, symbol);

		if (!matches.isEmpty())
		{
			Array<VariableStorage> constArgs;
			Array<TypeInfo> argTypes;

			for (int i = 0; i < functionCall->getNumArguments(); i++)
			{
				if (functionCall->getArgument(i)->isConstExpr())
				{
					auto value = functionCall->getArgument(i)->getConstExprValue();
					constArgs.add(value);
					argTypes.add(TypeInfo(value.getType()));
				}

				else
					return nullptr;
			}

			for (auto& match : matches)
			{
				// Don't constexpr the random function...
				if (match.id.toString().contains("rand"))
					continue;

				if (match.matchesArgumentTypes(argTypes))
				{
					VariableStorage result;

#define RETURN_T(x) match.returnType == Types::Helpers::getTypeFromTypeId<x>()
#define ARG_T(i, x) argTypes[i] == Types::Helpers::getTypeFromTypeId<x>()

					if (argTypes.size() == 1)
					{
#define CALL_IF(x) if (RETURN_T(x) && ARG_T(0, x)) result = match.call<x, x>((x)constArgs[0])
						CALL_IF(int);
						CALL_IF(float);
						CALL_IF(double);
#undef CALL_IF
					}

					if (argTypes.size() == 2)
					{
#define CALL_IF(x) if (RETURN_T(x) && ARG_T(0, x) && ARG_T(1, x)) result = match.call<x, x, x>((x)constArgs[0], (x)constArgs[1])
						CALL_IF(int);
						CALL_IF(float);
						CALL_IF(double);
#undef CALL_IF
					}
					if (argTypes.size() == 3)
					{
#define CALL_IF(x) if (RETURN_T(x) && ARG_T(0, x) && ARG_T(1, x) && ARG_T(2, x)) result = match.call<x, x, x>((x)constArgs[0], (x)constArgs[1], (x)constArgs[2])
						CALL_IF(int);
						CALL_IF(float);
						CALL_IF(double);
#undef CALL_IF
					}
#undef RETURN_T
#undef ARG_T

					return new Operations::Immediate(functionCall->location, result);
				}
			}
		}
	}

	return nullptr;
}

#undef IS




bool FunctionInliner::processStatementInternal(BaseCompiler* compiler, BaseScope* scope, StatementPtr s)
{
	if (auto is = dynamic_cast<Operations::IfStatement*>(s.get()))
	{
		bool hasReturnStatement = is->getTrueBranch()->forEachRecursive([](StatementPtr p)
		{
			return as<ReturnStatement>(p) != nullptr;
		}, IterationType::NoChildInlineFunctionBlocks);

		bool hasNoFalseBranch = is->getFalseBranch() == nullptr;

		if (hasReturnStatement && hasNoFalseBranch)
		{
			Operations::Statement::Ptr newFalseBranch;

			bool moveIntoNewFalseBranch = false;

			for (int i = 0; i < is->parent->getNumChildStatements(); i++)
			{
				auto s = is->parent->getChildStatement(i);

				if (s.get() == is)
				{
					moveIntoNewFalseBranch = true;

					continue;
				}

				if (moveIntoNewFalseBranch)
				{
					if (newFalseBranch == nullptr)
					{
						auto base = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(s.get());

						newFalseBranch = new Operations::StatementBlock(s->location, compiler->namespaceHandler.createNonExistentIdForLocation(base->getPath(), s->location.getLine()));
					}
						
					replaceWithNoop(s);
					newFalseBranch->addStatement(s);
				}
			}

			if (newFalseBranch != nullptr)
			{
				is->addStatement(newFalseBranch);
				return true;
			}
		}
	}

	if (auto fc = dynamic_cast<Operations::FunctionCall*>(s.get()))
	{
		if (compiler->getCurrentPass() == BaseCompiler::PreSymbolOptimization)
			return false;

		if (fc->function.canBeInlined(true))
		{
			jassertfalse;
		}

#if 0
		auto rd = fc->fc;

		jassert(rd != nullptr);

		auto s = rd->getClassName().getChildSymbol(fc->function.id);

		if (rd->hasFunction(s))
		{

			Array<FunctionData> matches;
			rd->addMatchingFunctions(matches, s);

			FunctionData toUse;

			if (matches.size() > 1)
			{
				BaseCompiler::ScopedPassSwitcher sps2(compiler, BaseCompiler::ResolvingSymbols);
				fc->process(compiler, scope);
				BaseCompiler::ScopedPassSwitcher sps3(compiler, BaseCompiler::TypeCheck);
				fc->process(compiler, scope);

				toUse = fc->function;
			}
			else
				toUse = matches.getFirst();

			auto s = dynamic_cast<ClassCompiler*>(compiler)->syntaxTree.get();

			SyntaxTreeWalker w(s);

			while (auto f = w.getNextStatementOfType<Operations::Function>())
			{
				auto& ofd = f->data;

				if (toUse.id == ofd.id && toUse.matchesArgumentTypes(ofd, true))
					return inlineRootFunction(compiler, scope, f, fc);
			}
		}
#endif
	}

	return false;
}


bool FunctionInliner::inlineRootFunction(BaseCompiler* compiler, BaseScope* scope, Operations::Function* f, Operations::FunctionCall* fc)
{
	auto clone = f->statements->clone(fc->location);
	auto cs = dynamic_cast<Operations::StatementBlock*>(clone.get());

	cs->setReturnType(f->data.returnType);

	if (fc->callType == Operations::FunctionCall::MemberFunction)
	{
		auto fObjectExpr = fc->getObjectExpression();

		jassert(fObjectExpr != nullptr);

		auto thisSymbol = Symbol("this");

		Operations::Expression::Ptr e = as<Operations::Expression>(fObjectExpr->clone(fc->location).get());

		cs->addInlinedParameter(-1, thisSymbol, e);

		if (auto st = fObjectExpr->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			clone->forEachRecursive([st, e](Operations::Statement::Ptr p)
			{
				if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
				{
					if (st->hasMember(v->id.id.getIdentifier()))
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
			}, Operations::IterationType::AllChildStatements);
		}
		else
			return false;
		


	}

	for (int i = 0; i < fc->getNumArguments(); i++)
	{
		auto pVarSymbol = f->data.args[i];

		Operations::Expression::Ptr e = dynamic_cast<Operations::Expression*>(fc->getArgument(i)->clone(fc->location).get());

		cs->addInlinedParameter(i, pVarSymbol, e);
	}

	replaceExpression(fc, clone);

	{
		BaseCompiler::ScopedPassSwitcher s1(compiler, BaseCompiler::ComplexTypeParsing);
		clone->process(compiler, scope);
		BaseCompiler::ScopedPassSwitcher s2(compiler, BaseCompiler::DataAllocation);
		clone->process(compiler, scope);
		BaseCompiler::ScopedPassSwitcher s3(compiler, BaseCompiler::DataInitialisation);
		clone->process(compiler, scope);
		BaseCompiler::ScopedPassSwitcher s4(compiler, BaseCompiler::ResolvingSymbols);
		clone->process(compiler, scope);
		BaseCompiler::ScopedPassSwitcher s5(compiler, BaseCompiler::TypeCheck);
		clone->process(compiler, scope);
	}
	return true;
}

bool DeadcodeEliminator::processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	COMPILER_PASS(BaseCompiler::PostSymbolOptimization)
	{
        if (auto imm = as<Operations::Immediate>(statement))
        {
			if (imm->isAnonymousStatement())
			{
				replaceWithNoop(imm);
				return true;
			}
                
        }
        
		if (auto a = as<Operations::Assignment>(statement))
		{
			if (a->getTargetType() != Operations::Assignment::TargetType::Variable)
				return false;

			auto v = as<Operations::VariableReference>(a->getTargetVariable());

			if (v == nullptr)
				return false;

			if (v->isClassVariable(s))
				return false;

			if (v->objectPtr != nullptr)
				return false;

			if (a->getSubExpr(0)->hasSideEffect())
				return false;

			if (Operations::findParentStatementOfType<Operations::MemoryReference>(v) != nullptr)
				return false;

			if (auto ls = Operations::findParentStatementOfType<Operations::Loop>(v))
			{
				if (ls->iterator == v->id)
					return false;
			}

			if (v->isParameter(s))
				return false;

			int numReferences = 0;

			SyntaxTreeWalker w(a);

			while (auto other = w.getNextStatementOfType<Operations::VariableReference>())
			{
				if (other->id == v->id)
					numReferences++;
			}

			if (numReferences == 1)
			{
				v->logWarning("Unused variable " + v->id.toString());
				OptimizationPass::replaceWithNoop(statement);
				return true;
			}
		}
	}

	return false;

#if 0

	for (auto s : *tree)
	{
		if (auto im = as<Operations::Immediate>(s))
		{
			if (im->isAnonymousStatement())
				im->optimizeAway();
		}
		if (auto v = as<Operations::VariableReference>(s))
		{
			// Just remove empty global variable and parameter
			// statements. Unreferenced local variables will be removed 
			// in its assignment.
			if (v->isAnonymousStatement() &&
				(v->isClassVariable() || v->parameterIndex != -1))
			{
				v->optimizeAway();
			}

		}
		if (auto a = as<Operations::Assignment>(s))
		{
			/* Optimize away if:
			- no side effect (TODO)
			*/

			auto target = a->getTargetVariable();

			bool singleReference = target->isReferencedOnce();
			bool notGlobal = !target->isClassVariable();


			if (singleReference && notGlobal)
				a->optimizeAway();
		}
	}
#endif

	
}

bool BinaryOpOptimizer::processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	using namespace Operations;

	COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
	{
		if (auto inc = as<Increment>(statement))
		{
			if (!inc->isPreInc)
			{
				auto parentStatement = inc->parent;

				auto cantBeSwapped = false;

				cantBeSwapped |= findParentStatementOfType<Assignment>(statement.get()) != nullptr;
				cantBeSwapped |= findParentStatementOfType<BinaryOp>(statement.get()) != nullptr;
				cantBeSwapped |= findParentStatementOfType<FunctionCall>(statement.get()) != nullptr;
				cantBeSwapped |= findParentStatementOfType<ComplexTypeDefinition>(statement.get()) != nullptr;
				cantBeSwapped |= findParentStatementOfType<ReturnStatement>(statement.get()) != nullptr;
				cantBeSwapped |= findParentStatementOfType<Compare>(statement.get()) != nullptr;
								
				if (!cantBeSwapped)
				{
					inc->isPreInc = true;
					return true;
				}
			}
		}

		if (auto bOp = as<BinaryOp>(statement))
		{
			if (simplifyOp(bOp->getSubExpr(0), bOp->getSubExpr(1), bOp->op, compiler, s))
				return true;

			if (swapIfBetter(bOp, bOp->op, compiler, s))
				return true;
		}
	}


	COMPILER_PASS(BaseCompiler::PostSymbolOptimization)
	{
		if (auto bOp = as<BinaryOp>(statement))
		{
			if (swapIfBetter(bOp, bOp->op, compiler, s))
				return true;
		}

		if (auto a = as<Assignment>(statement))
		{
			if (simplifyOp(a->getSubExpr(1), a->getSubExpr(0), a->assignmentType, compiler, s))
				return true;

			if (a->getTargetType() == Assignment::TargetType::Variable)
			{
				currentlyAssignedId = a->getTargetSymbolStatement()->getSymbol();

				a->getSubExpr(0)->process(compiler, s);

				if (auto bOp = as<BinaryOp>(a->getSubExpr(0)))
				{
					if (isAssignedVariable(bOp->getSubExpr(0)) && !SpanType::isSimdType(a->getSubExpr(1)->getTypeInfo()))
					{
						a->logOptimisationMessage("Replace " + juce::String(bOp->op) + " with self assignment");
						a->assignmentType = bOp->op;

						auto right = bOp->getSubExpr(1);

						replaceExpression(bOp, right);
						return true;
					}
				}

				currentlyAssignedId = {};
			}
		}
	}

	return false;
	
}

bool BinaryOpOptimizer::swapIfBetter(ExprPtr bOp, const char* op, BaseCompiler* compiler, BaseScope* s)
{
	if (currentlyAssignedId)
		return false;

	StatementPtr rightOp(bOp->getSubExpr(1).get());

	rightOp->process(compiler, s);

	auto l = bOp->getSubExpr(0);
	auto r = bOp->getSubExpr(1);

	if (isAssignedVariable(l))
	{
		bOp->logOptimisationMessage("Good order");
	}
	else if (isAssignedVariable(r))
	{
		if (Helpers::isSwappable(op))
		{
			bOp->logOptimisationMessage("Wrong order, swap them");
			bOp->swapSubExpressions(0, 1);
			return true;
		}
	}
	else if (l->isConstExpr() && !r->isConstExpr())
	{
		if (Helpers::isSwappable(op))
		{
			bOp->logOptimisationMessage("Wrong order, swap them");
			bOp->swapSubExpressions(0, 1);
			return true;
		}
	}

	return false;
}

bool BinaryOpOptimizer::simplifyOp(ExprPtr l, ExprPtr r, const char* op, BaseCompiler* , BaseScope* )
{
	using namespace Operations;

	auto parent = l->parent;

	auto replaceOp = [this](Statement* p, const char* o)
	{
		if (auto bOp = as<BinaryOp>(p))
			bOp->op = o;
		else if (auto a = as<Assignment>(p))
			a->assignmentType = o;
	};

	
	if (op == JitTokens::minus)
	{
		if (r->isConstExpr())
		{
			parent->logOptimisationMessage("Replace minus");
			
			auto value = r->getConstExprValue();
			auto invValue = VariableStorage(value.getType(), value.toDouble() * -1.0);

			replaceExpression(r, new Immediate(r->location, invValue));
			replaceOp(parent, JitTokens::plus);

			return true;
		}
	}

	if (op == JitTokens::divide)
	{
		if (r->isConstExpr() && r->getType() != Types::ID::Integer)
		{
			parent->logOptimisationMessage("Replace division");

			auto value = r->getConstExprValue();

			if (value.toDouble() == 0.0)
				r->throwError("Division by zero");

			auto invValue = VariableStorage(value.getType(), 1.0 / value.toDouble());

			replaceExpression(r, new Immediate(r->location, invValue));
			replaceOp(parent, JitTokens::times);

			return true;
		}
	}

	return false;
}

bool BinaryOpOptimizer::isAssignedVariable(ExprPtr e) const
{
	using namespace Operations;

	if (auto v = as<SymbolStatement>(e))
	{
		return v->getSymbol() == currentlyAssignedId;
	}
	else
	{
		for (int i = 0; i < e->getNumChildStatements(); i++)
		{
			if(isAssignedVariable(e->getSubExpr(i)))
				return true;
		}
	}
	
	return false;
}

snex::jit::Operations::BinaryOp* BinaryOpOptimizer::getFirstOp(ExprPtr e)
{
	using namespace Operations;

	if (e == nullptr)
		return nullptr;

	if (auto bOp = getFirstOp(e->getSubExpr(0)))
		return bOp;

	if(auto thisOp = as<BinaryOp>(e))
		return thisOp;

	return nullptr;
}

bool BinaryOpOptimizer::containsVariableReference(ExprPtr p, const Symbol& refToCheck)
{
	if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
	{
		return v->id == refToCheck;
	}

	for (int i = 0; i < p->getNumChildStatements(); i++)
	{
		if (containsVariableReference(p->getSubExpr(i), refToCheck))
			return true;
	}

	return false;
}

void BinaryOpOptimizer::swapBinaryOpIfPossible(ExprPtr binaryOp)
{
	auto l = binaryOp->getSubExpr(0);
	auto r = binaryOp->getSubExpr(1);
	auto opType = dynamic_cast<Operations::BinaryOp*>(binaryOp.get())->op;

	if (!Helpers::isSwappable(opType))
		return;

	bool leftIsImmediate = Operations::isStatementType<Operations::Immediate>(l.get());

	bool rightContainsReferenceToTarget = false;

	Symbol targetRef;

	if (auto assignment = Operations::findParentStatementOfType<Operations::Assignment>(binaryOp.get()))
	{
		auto v = dynamic_cast<Operations::VariableReference*>(assignment->getSubExpr(0).get());

		targetRef = v->id;
	}

	if (targetRef && containsVariableReference(r, targetRef))
		rightContainsReferenceToTarget = true;

	if (leftIsImmediate || rightContainsReferenceToTarget)
		binaryOp->swapSubExpressions(0, 1);
}

void BinaryOpOptimizer::createSelfAssignmentFromBinaryOp(ExprPtr assignment)
{
	auto as = dynamic_cast<Operations::Assignment*>(assignment.get());

	if (as->assignmentType != JitTokens::assign_)
		return;

	if (auto bOp = dynamic_cast<Operations::BinaryOp*>(assignment->getSubExpr(1).get()))
	{
		// If the left side is a binary op, it means you can't assign it because it will
		// have a forced operator precedence
		if (Operations::isStatementType<Operations::BinaryOp>(bOp->getSubExpr(0).get()))
			return;

		if (auto v_l = dynamic_cast<Operations::VariableReference*>(bOp->getSubExpr(0).get()))
		{
			if (v_l->id == as->getTargetSymbolStatement()->getSymbol())
			{
				if (auto bOp_r = dynamic_cast<Operations::BinaryOp*>(bOp->getSubExpr(1).get()))
				{
					auto rLevel = Helpers::getPrecedenceLevel(bOp_r->op);
					auto lLevel = Helpers::getPrecedenceLevel(bOp->op);

					// Can't fold to self assignment because of operator precedence
					if (lLevel > rLevel)
						return;
				}

				as->logOptimisationMessage("Create self assign");
				as->assignmentType = bOp->op;
				as->replaceChildStatement(1, bOp->getSubExpr(1));
				return;
			}
		}

		if (auto v_r = dynamic_cast<Operations::VariableReference*>(bOp->getSubExpr(1).get()))
		{
			if (v_r->id == as->getTargetSymbolStatement()->getSymbol())
			{
				as->logOptimisationMessage("Create self assign");
				as->assignmentType = bOp->op;
				as->replaceChildStatement(1, bOp->getSubExpr(0));
				return;
			}
		}
	}
}

bool LoopOptimiser::processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	if (auto fc = as<Operations::FunctionCall>(statement))
	{
		fc->tryToResolveType(compiler);
		return false;
	}
	if (auto l = as<Operations::Loop>(statement))
	{
		currentLoop = l;
		currentScope = s;
		currentCompiler = compiler;

		if (combineLoops(compiler, s, l))
		{
			return true;
		}

		COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
		{
			

			if (unroll(compiler, s, l))
			{
				return true;
			}
		}
	}

	return false;
}

bool LoopOptimiser::replaceWithVectorLoop(BaseCompiler* compiler, BaseScope* s, Ptr binaryOpToReplace)
{
	jassertfalse;

	return false;


}

bool LoopOptimiser::unroll(BaseCompiler* c, BaseScope* s, Operations::Loop* l)
{
	auto hasControlFlow = l->forEachRecursive([](Ptr p)
	{
		return as<Operations::ControlFlowStatement>(p) != nullptr;
	}, IterationType::NoChildInlineFunctionBlocks);
		
	if (hasControlFlow)
		return false;

	l->tryToResolveType(c);

	if (l->iterator.typeInfo.getTypedIfComplexType<DynType>() != nullptr)
		return false;

	int compileTimeLoopCount = Helpers::getCompileTimeAmount(l->getTarget());

	if (compileTimeLoopCount > 0 && compileTimeLoopCount <= 8)
	{
		auto loopParent = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(l);

		jassert(loopParent != nullptr);

		Ptr lp = new Operations::StatementBlock(l->location, c->namespaceHandler.createNonExistentIdForLocation(loopParent->getPath(), l->location.getLine()));
		Ptr target = l->getTarget();

		for (int i = 0; i < compileTimeLoopCount; i++)
		{
			Identifier uId("Unroll" + juce::String(i));

			auto cb = l->getLoopBlock()->clone(l->location);

			juce::String comment;
			comment << "unroll " << target->getTypeInfo().toString() << "[" << juce::String(i) << "]";


			auto newParentScopeId = as<Operations::StatementBlock>(lp)->getPath().getChildId(uId);

			auto clb = as<Operations::StatementBlock>(cb);
			clb->setNewPath(c, newParentScopeId);

			clb->attachAsmComment(comment);

			auto iteratorSymbol = Symbol(clb->getPath().getChildId(l->iterator.getName()), l->iterator.typeInfo);

			jassert(l->iterator.resolved);

			NamespaceHandler::ScopedNamespaceSetter sns(c->namespaceHandler, iteratorSymbol.id.getParent());

            c->namespaceHandler.addSymbol(iteratorSymbol.id, iteratorSymbol.typeInfo, NamespaceHandler::Variable, NamespaceHandler::SymbolDebugInfo());

			iteratorSymbol.typeInfo = iteratorSymbol.typeInfo.withModifiers(iteratorSymbol.isConst(), true);

			if (l->evaluateIteratorLoad())
			{
				auto imm = new Operations::Immediate(l->location, VariableStorage(i));
				auto sus = new Operations::Subscript(l->location, target, imm);
				auto iv = new Operations::VariableReference(l->location, iteratorSymbol);

				if (auto childDynType = iteratorSymbol.typeInfo.getTypedIfComplexType<DynType>())
				{
					// Fix this when optimising span<dyn> unrolling...
					jassertfalse;

#if 0
					FunctionClass::Ptr fc = childDynType->getFunctionClass();

					auto f = fc->getNonOverloadedFunction(fc->getClassName().getChildId("referTo"));
					auto call = new Operations::FunctionCall(l->location, nullptr, { f.id, TypeInfo(Types::ID::Void) }, {});
					call->setObjectExpression(iv);
					call->addArgument(sus);

					cb->addStatement(call, true);
					cb->addStatement(new Operations::ComplexTypeDefinition(l->location, iteratorSymbol.id, iteratorSymbol.typeInfo), true);
#endif
				}
				else
				{
					auto ia = new Operations::Assignment(l->location, iv, JitTokens::assign_, sus, true);

					cb->addStatement(ia, true);
				}

				
			}

			jassert(iteratorSymbol.resolved);

			cb->forEachRecursive([this, c, s](Ptr f)
			{
				if (auto sl = as<Operations::Loop>(f))
				{
					unroll(c, s, sl);
				}

				return false;
			}, Operations::IterationType::NoChildInlineFunctionBlocks);

			lp->addStatement(cb);

			if (l->evaluateIteratorStore())
			{
				auto iv = new Operations::VariableReference(l->location, iteratorSymbol);
				auto imm = new Operations::Immediate(l->location, VariableStorage(i));
				auto sus = new Operations::Subscript(l->location, target, imm);
				auto ia = new Operations::Assignment(l->location, sus, JitTokens::assign_, iv, false);

				cb->addStatement(ia, false);
			}
		}

        SyntaxTreeInlineData::processUpToCurrentPass(l, lp);
        
		replaceExpression(l, lp);
        
		l->parent = nullptr;
		return true;

		return false;
	}

	return false;
}



bool LoopOptimiser::combineLoops(BaseCompiler* c, BaseScope* s, Operations::Loop* l)
{
	using namespace Operations;

	auto loopParent = getRealParent(l);
	
	for (int i = 0; i < loopParent->getNumChildStatements(); i++)
	{
		auto c = loopParent->getChildStatement(i);
		auto tl = getLoopStatement(c.get());

		if (tl == l)
		{
			if (i < loopParent->getNumChildStatements())
			{
				auto ns = loopParent->getChildStatement(i + 1);
				if (auto nl = getLoopStatement(ns))
				{
					if (combineInternal(l, nl))
						return true;
				}
			}

			if (i > 0)
			{
				auto ps = loopParent->getChildStatement(i - 1);

				if (auto pl = getLoopStatement(ps))
				{
					if (combineInternal(pl, l))
					{
						return true;
					}
						
				}
			}
		}
	}

	return false;
}



bool LoopOptimiser::sameTarget(Operations::Loop* l1, Operations::Loop* l2)
{
	using namespace Operations;

	auto t1 = l1->getTarget();
	auto t2 = l2->getTarget();

	auto sameVariable = [](StatementPtr s1, StatementPtr s2)
	{
		if (auto v1 = as<SymbolStatement>(s1))
		{
			if (auto v2 = as<SymbolStatement>(s2))
			{
				if (getRealSymbol(s1) == getRealSymbol(s2))
					return true;

#if 0
				if (v1->getSymbol() == v2->getSymbol())
					return true;

				auto ip1 = StatementBlock::findInlinedParameterInParentBlocks(s1, v1->getSymbol());

				auto ip2 = StatementBlock::findInlinedParameterInParentBlocks(s2, v2->getSymbol());

				if (ip1 != nullptr && ip2 != nullptr)
				{
					auto is1 = as<SymbolStatement>(ip1->getSubExpr(0));
					auto is2 = as<SymbolStatement>(ip2->getSubExpr(0));

					return is1->getSymbol() == is2->getSymbol();
				}
#endif
			}
		}

		return false;
	};

	if (sameVariable(t1, t2))
		return true;

	auto sameFunctionCall = [sameVariable](StatementPtr s1, StatementPtr s2)
	{
		if (auto f1 = as<FunctionCall>(s1))
		{
			if (auto f2 = as<FunctionCall>(s2))
			{
				if (f1->function.id == f2->function.id)
				{
					auto o1 = f1->getObjectExpression();
					auto o2 = f2->getObjectExpression();

					if ((o1 == nullptr && o2 == nullptr) || sameVariable(o1, o2))
					{
						auto numMaxArguments = jmax(f1->getNumArguments(), f2->getNumArguments());

						for (int i = 0; i < numMaxArguments; i++)
						{
							if (!sameVariable(f1->getArgument(i), f2->getArgument(i)))
								return false;
						}

						return true;
					}
				}
			}
		}

		return false;
	};

	if (sameFunctionCall(t1, t2))
		return true;

	return false;
}

bool LoopOptimiser::isBlockWithSingleStatement(StatementPtr s)
{
	using namespace Operations;

	if (auto sb = as<StatementBlock>(s))
	{
		auto numStatements = sb->getNumChildStatements();
		auto numRealStatements = 0;

		for (int i = 0; i < numStatements; i++)
		{
			auto c = s->getChildStatement(i);

			if (StatementBlock::isRealStatement(c.get()))
				numRealStatements++;
		}

		if (numRealStatements == 1)
			return true;
	}

	return false;
}

bool LoopOptimiser::combineInternal(Operations::Loop* l, Operations::Loop* nl)
{
	using namespace Operations;

	if (sameTarget(l, nl))
	{
		bool atLastLoop = currentLoop == nl;
		auto fb = l->getLoopBlock();
		auto nb = nl->getLoopBlock();
		auto sourceBlock = atLastLoop ? fb : nb;
		auto targetBlock = atLastLoop ? nb : fb;
		auto lastTargetStatement = targetBlock->getChildStatement(targetBlock->getNumChildStatements() - 1);
		auto loc = lastTargetStatement->location;
		auto si = atLastLoop ? l->iterator : nl->iterator;
		auto ti = atLastLoop ? nl->iterator : l->iterator;

		sourceBlock->forEachRecursive([this, si, ti](Statement::Ptr b)
		{
			if (auto ip = as<InlinedParameter>(b))
			{
				if (auto ip1 = StatementBlock::findInlinedParameterInParentBlocks(ip, ip->getSymbol()))
				{
					auto newSymbol = as<SymbolStatement>(ip1->getSubExpr(0))->getSymbol();

					replaceExpression(ip, new VariableReference(ip->location, newSymbol));
				}
			}

			if (auto ss = as<VariableReference>(b))
			{
				if (ss->id == si)
					ss->id = ti;
			}

			return false;
		}, IterationType::AllChildStatements);

		auto newBlock = new StatementBlock(loc, targetBlock->getPath());
		
		for (int i = 0; i < fb->getNumChildStatements(); i++)
		{
			auto s = fb->getChildStatement(i)->clone(loc);
			newBlock->addStatement(s);
		}

		for (int i = 0; i < nb->getNumChildStatements(); i++)
		{
			auto s = nb->getChildStatement(i)->clone(loc);
			newBlock->addStatement(s);
		}

		

		replaceExpression(targetBlock, newBlock);
		processPreviousPasses(currentCompiler, currentScope, newBlock);

		replaceWithNoop(getRealParent(sourceBlock));

		return true;
	}

	return false;
}

snex::jit::Operations::Loop* LoopOptimiser::getLoopStatement(StatementPtr s)
{
	using namespace Operations;

	if (auto l = as<Loop>(s))
		return l;

	if (isBlockWithSingleStatement(s))
	{
		for (int i = 0; i < s->getNumChildStatements(); i++)
		{
			if (StatementBlock::isRealStatement(s->getChildStatement(i).get()))
			{
				return getLoopStatement(s->getChildStatement(i));
			}
		}
	}

	return nullptr;
}

snex::jit::OptimizationPass::StatementPtr LoopOptimiser::getRealParent(StatementPtr s)
{
	using namespace Operations;

	if (auto p = s->parent.get())
	{
		if (isBlockWithSingleStatement(p))
			return getRealParent(p);

		return p;
	}

	return nullptr;
}

snex::jit::Symbol LoopOptimiser::getRealSymbol(StatementPtr s)
{
	using namespace Operations;
	if (auto ie = StatementBlock::findInlinedParameterInParentBlocks(s.get(), as<SymbolStatement>(s)->getSymbol()))
		return as<SymbolStatement>(ie->getSubExpr(0))->getSymbol();
	else return as<SymbolStatement>(s)->getSymbol();
}

bool LoopVectoriser::processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	if (auto fc = as<Operations::FunctionCall>(statement))
	{
		fc->tryToResolveType(compiler);
		return false;
	}
	if (auto l = as<Operations::Loop>(statement))
	{
		COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
		{
			if (convertToSimd(compiler, l))
			{
				return true;
			}
		}
	}

	return false;
}


bool LoopVectoriser::convertToSimd(BaseCompiler* c, Operations::Loop* l)
{
	auto t = l->getTarget();

	if (t->getTypeInfo().isDynamic())
	{
		t->tryToResolveType(c);
	}

	if (auto at = t->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
	{
		bool isFloat = at->getElementType().getType() == Types::ID::Float;

		if (!isFloat)
			return false;

		auto isUnSimdable = l->getLoopBlock()->forEachRecursive(isUnSimdableOperation, IterationType::AllChildStatements);

		if (isUnSimdable)
			return false;

		if (auto asSpan = dynamic_cast<SpanType*>(at))
		{
			bool isNonSimdableSpan = (asSpan->getNumElements() % 4) != 0;

			if (isNonSimdableSpan)
				return false;

			changeIteratorTargetToSimd(l);
			
			return true;
		}
		if (auto asDyn = dynamic_cast<DynType*>(at))
		{
			auto oldPath = l->getLoopBlock()->getPath();

			if (oldPath.getIdentifier() == Identifier("Fallback"))
			{
				return false;
			}

			NamespacedIdentifier i("isSimdable");
			auto cond = new Operations::FunctionCall(l->location, nullptr, { i, TypeInfo(Types::ID::Integer) }, {});
			cond->setObjectExpression(t);

			

			auto fallbackPath = oldPath.getChildId("Fallback");
			auto simdPath = oldPath.getChildId("SimdPath");

			auto trueBranch = l->clone(l->location);

			auto simdLoop = as<Operations::Loop>(trueBranch);

			simdLoop->iterator.id.relocateSelf(oldPath, simdPath);
			simdLoop->getLoopBlock()->setNewPath(c, simdPath);
			changeIteratorTargetToSimd(simdLoop);

			auto falseBranch = l->clone(l->location);

			auto fallbackLoop = as<Operations::Loop>(falseBranch);

			fallbackLoop->iterator.id.relocateSelf(oldPath, fallbackPath);
			fallbackLoop->getLoopBlock()->setNewPath(c, fallbackPath);

			auto ifs = new Operations::IfStatement(l->location, cond, trueBranch, falseBranch);

			replaceExpression(l, ifs);
			l->parent = nullptr;

			return true;;
		}
	}

	return false;
}

juce::Result LoopVectoriser::changeIteratorTargetToSimd(Operations::Loop* l)
{
	auto t = l->getTarget();

	auto newCall = new Operations::FunctionCall(t->location, nullptr, { NamespacedIdentifier("toSimd"), TypeInfo(Types::ID::Dynamic) }, {});

	newCall->setObjectExpression(t->clone(t->location));
	replaceExpression(t, newCall);
	return Result::ok();
}

bool LoopVectoriser::isUnSimdableOperation(Ptr s)
{
	using namespace Operations;

	auto parentLoop = Operations::findParentStatementOfType<Operations::Loop>(s.get());

	if (auto cf = as<ControlFlowStatement>(s))
		return true;

	if (auto cf = as<FunctionCall>(s))
		return true;

	if (auto cf = as<Increment>(s))
		return true;

	if (auto v = as<VariableReference>(s))
	{
		auto writeType = v->getWriteAccessType();

		if (writeType != JitTokens::void_)
		{
			if (!(parentLoop->iterator == v->id))
				return true;
		}
	}

	return false;
}

#if SNEX_ASMJIT_BACKEND
namespace AsmSubPasses
{

struct Helpers
{
	constexpr static uint32 NoReg = 125500012;

	static bool isInstruction(BaseNode* node, Array<uint32_t> ids)
	{
		if (node == nullptr)
			return false;

		if (node->isInst())
		{
			auto inst = node->as<InstNode>();
			auto thisId = inst->baseInst().id();
			return ids.contains(thisId);
		}

		return false;
	}

	static bool isCallNode(BaseNode* n)
	{
		if (!n->isInst())
			return false;

		auto id = n->as<InstNode>()->id();
		return id == x86::Inst::kIdCall;
	}

	static bool sameMem(Operand o1, Operand o2)
	{
		if (o1.isMem() && o2.isMem())
		{
			auto m1 = o1.as<BaseMem>();
			auto m2 = o2.as<BaseMem>();

			

			// there should be no ptrs without a base reg
			// to fix the 64bit offset issue
			jassert(m1.hasBase() && m2.hasBase());

			if (m1.baseId() == m2.baseId())
			{
				auto sameIndexType = m1.hasIndexReg() == m2.hasIndexReg();

				if (sameIndexType)
				{
					if (m1.hasIndexReg())
					{
						auto i1 = m1.indexId();
						auto i2 = m2.indexId();

						if (i1 == i2)
						{
							return true;
						}
					}
					else
					{
						auto off1 = m1.offset();
						auto off2 = m2.offset();

						if (off1 == off2)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	static bool isInstructWithSameTarget(BaseNode* n1, BaseNode* n2)
	{
		// If it's not an instruction, it can't have the same target
		if (!n1->isInst() || !n2->isInst())
			return false;

		auto o1 = n1->as<InstNode>()->op(0);
		auto o2 = n2->as<InstNode>()->op(0);

		return opEqualOrSameReg(o1, o2);
	}

	static bool isInstructWithSourceAsTarget(BaseNode* sourceNode, BaseNode* targetNode)
	{
		if (sourceNode->isInst() && targetNode->isInst())
		{
			auto s = getSourceOp(sourceNode->as<InstNode>());
			auto t = getTargetOp(targetNode->as<InstNode>());

			return opEqualOrSameReg(s, t);
		}

		return false;
	}

	static bool isInstructWithMemRegAsTarget(BaseNode* sourceNode, BaseMem m)
	{
		if (!sourceNode->isInst())
			return false;

		if (!m.hasBaseOrIndex())
			return false;

		auto t = getTargetOp(sourceNode->as<InstNode>());

		if (t.isReg())
		{
			if (m.hasBaseReg() && m.baseId() == t.id())
				return true;

			if (m.hasIndexReg() && m.indexId() == t.id())
				return true;
		}

		return false;
	}

	static bool isInstructWithSameSource(BaseNode* n1, BaseNode* n2)
	{
		if (n1->isInst() && n2->isInst())
		{
			auto s1 = getSourceOp(n1->as<InstNode>());
			auto s2 = getSourceOp(n2->as<InstNode>());

			return opEqualOrSameReg(s1, s2);
		}

		return false;
	}

	static bool opEqualOrSameReg(const Operand& o1, const Operand& o2)
	{
		auto same = o1.equals(o2);
		auto isRegister = o1.isPhysReg() && o2.isPhysReg();

		if (isRegister)
		{
			auto firstId = o1.id();
			auto idMatch = o1.id() == o2.id();
			
			ignoreUnused(firstId);

			auto sameType = o1.as<X86Reg>().isXmm() == o2.as<X86Reg>().isXmm();

			return same || (isRegister && idMatch && sameType);
		}

		return same;
		
	}

	static bool isMemWithRegAsBaseOrIndex(Operand possibleMem, X86Reg r)
	{
		if (possibleMem.isMem())
		{
			auto m = possibleMem.as<BaseMem>();

			if (m.hasBaseOrIndex())
			{
				auto indexId = possibleMem.as<BaseMem>().indexId();
				auto baseId = possibleMem.as<BaseMem>().baseId();

				auto rId = r.id();

				return rId == baseId || rId == indexId;
			}
		}

		return false;
	}

	static bool isMemWithBaseReg(Operand o)
	{
		return o.isMem() && o.as<BaseMem>().hasBaseReg();
	}

	static Operand getTargetOp(InstNode* n)
	{
		return n->op(0);
	}

	static Operand getSourceOp(InstNode* n)
	{
		if (n->opCount() > 1)
			return n->op(1);
		else
			return n->op(0);
	}

	static uint32 getBaseRegIndex(Operand op)
	{
		return (op.isMem() && op.as<BaseMem>().hasBaseReg()) ? op.as<BaseMem>().baseId() : NoReg;
	}

	static uint32 getIndexRegIndex(Operand op)
	{
		return (op.isMem() && op.as<BaseMem>().hasIndexReg()) ? op.as<BaseMem>().indexId() : NoReg;
	}

	static bool isFpuInstruction(BaseNode* n)
	{
		using namespace x86;
		return isInstruction(n, { Inst::kIdFld, Inst::kIdFprem, Inst::kIdFld1, Inst::kIdFprem1,
								  Inst::kIdFstp });
	}

	static bool isInstructionWithIndexAsTarget(BaseNode* n, uint32 base, uint32 index)
	{
		if (base == NoReg && index == NoReg)
			return false;

		if (n->isInst())
		{
			auto target = getTargetOp(n->as<InstNode>());

			if (target.isPhysReg())
			{
				auto id = target.id();

				return id == base || id == index;
			}

			return false;
		}

		return false;
	}
};

struct Inc
{
	using ReturnType = InstNode;
	static bool matches(BaseNode* node)
	{
		using namespace x86;
		return Helpers::isInstruction(node, { Inst::kIdInc });
	}
};

struct Lea
{
	using ReturnType = InstNode;
	static bool matches(BaseNode* node)
	{
		using namespace x86;
		return Helpers::isInstruction(node, { Inst::kIdLea });
	}
};

struct InstructionFilter
{
	using ReturnType = InstNode;
	static bool matches(BaseNode* node)
	{
		return node->isInst();
	}
};

struct Mov
{
	using ReturnType = InstNode;
	static bool matches(BaseNode* node)
	{
		using namespace x86;
		return Helpers::isInstruction(node, { Inst::kIdMov, Inst::kIdMovss, Inst::kIdMovsd, Inst::kIdMovaps });
	}
};

struct Jump
{
	using ReturnType = InstNode;

	static bool matches(BaseNode* node)
	{
		if (node->isInst())
		{
			auto inst = node->as<InstNode>();
			auto thisId = inst->baseInst().id();

			using namespace x86;
			Range<uint32_t> r(Inst::kIdJa, Inst::kIdJz + 1);

			return r.contains(thisId);
		}

		return false;
	}
};

struct CompareInstructions
{
	using ReturnType = InstNode;

	static bool matches(BaseNode* node)
	{
		if (node->isInst())
		{
			auto inst = node->as<InstNode>();
			auto thisId = inst->baseInst().id();
			using namespace x86;
			return thisId == Inst::kIdCmp || thisId == Inst::kIdTest;
		}

		return false;
	}
};

struct ControlFlowBreakers
{
	using ReturnType = InstNode;

	static bool matches(BaseNode* node)
	{
		return Helpers::isCallNode(node) || Jump::matches(node);
	}
};

struct MathOp
{
	using ReturnType = InstNode;
	static bool matches(BaseNode* node)
	{
		using namespace x86;
		return Helpers::isInstruction(node, { Inst::kIdAdd, Inst::kIdAddss, Inst::kIdAddsd,
			Inst::kIdImul, Inst::kIdMulss, Inst::kIdMulsd,
			Inst::kIdSub, Inst::kIdSubss, Inst::kIdSubsd 
			});
	}

	static juce_wchar getMathOperator(BaseNode* node)
	{
		auto inst = node->as<InstNode>();
		auto thisId = inst->baseInst().id();
		using namespace x86;

		switch (thisId)
		{
		case Inst::kIdAdd:
		case Inst::kIdAddss:
		case Inst::kIdAddsd:
		case Inst::kIdSub:
		case Inst::kIdSubss:
		case Inst::kIdSubsd: return '+';
		case Inst::kIdImul:
		case Inst::kIdMulss:
		case Inst::kIdMulsd: return '*';
		}

		jassertfalse;
		return 0;
	}

	static Types::ID getType(BaseNode* node)
	{
		auto inst = node->as<InstNode>();
		auto thisId = inst->baseInst().id();
		using namespace x86;

		switch (thisId)
		{
		case Inst::kIdSub:
		case Inst::kIdAdd:
		case Inst::kIdImul: return Types::ID::Integer;
		case Inst::kIdAddss:
		case Inst::kIdMulss:
		case Inst::kIdSubss: return Types::ID::Float;
		case Inst::kIdAddsd:
		case Inst::kIdSubsd: 
		case Inst::kIdMulsd: return Types::ID::Double;
		}

		return Types::ID::Void;
	}

	static int getNoopValue(BaseNode* node)
	{
		auto op = getMathOperator(node);
		return op == '+' ? 0 : 1;
	}

	template <typename Type> static bool getConstPoolValue(FuncPass* parent, const Operand& op, Type& v)
	{
		if (op.isMem())
		{
			auto mem = op.as<x86::Mem>();

			if (mem.hasBaseLabel())
			{
				for (auto l : parent->cc()->labelNodes())
				{
					if (!l->isConstPool())
						continue;

					auto lId = l->labelId();
					auto mId = mem.id();

					if (lId == mId)
					{
						auto c = l->as<ConstPoolNode>();
						const auto& cp = c->constPool();
						auto offset = mem.offset();
						auto poolSize = cp.size();
						auto data = reinterpret_cast<uint8_t*>(alloca(poolSize));
						cp.fill(data);

						v = *reinterpret_cast<Type*>(data + offset);
						return true;
					}
				}
			}
		}

		return false;
	}
};

struct RemoveMovToSameOp : public AsmCleanupPass::SubPass<Mov>
{
	RemoveMovToSameOp(FuncPass* p) :
		SubPass<Mov>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			

			if (Helpers::getTargetOp(n).equals(Helpers::getSourceOp(n)))
				it.removeNode(n);
		}

		return false;
	}
};

struct RemoveLeaFromSameSource : public AsmCleanupPass::SubPass<Lea>
{
	RemoveLeaFromSameSource(FuncPass* p) :
		SubPass<Lea>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			auto source = Helpers::getSourceOp(n).as<x86::Mem>();
			auto target = Helpers::getTargetOp(n).as<x86::Reg>();

			if (source.hasBaseReg() && !source.hasOffset())
			{
				auto sameReg = source.baseReg().equals(target);
				auto hasIndexReg = source.hasIndexReg();


				if (sameReg && !hasIndexReg)
					it.removeNode(n);
			}
		}

		return false;
	}
};

struct RemoveSwappedMovCallsToMemory : public AsmCleanupPass::SubPass<Mov>
{
	/** Removes memory writes and loads from the same register to the same memory location. 
	
		movss [rcx], xmm0
		movss xmm0, [rcx]			// <= This one will be removed
		addss xmm0, dword [L2+4]
	
	*/
	RemoveSwappedMovCallsToMemory(FuncPass* p) :
		SubPass<Mov>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			auto nextNode = n->next();

			if (Mov::matches(nextNode))
			{
				if (Helpers::isInstructWithSourceAsTarget(n, nextNode))
				{
					if (Helpers::isInstructWithSourceAsTarget(nextNode, n))
					{
						it.removeNode(nextNode);
						return true;
					}
				}
			}
		}

		return false;
	}
};


struct RemoveDoubleMemoryWrites : public AsmCleanupPass::SubPass<InstructionFilter>
{
	/** 
	
		movss [rcx], xmm0			//< removes this instruction
		addss xmm0, dword [L2+4]
		movss [rcx], xmm0
	*/
	RemoveDoubleMemoryWrites(FuncPass* p) :
		SubPass<InstructionFilter>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto thisNode = it.next())
		{
			auto thisTarget = Helpers::getTargetOp(thisNode);

			if (Helpers::isFpuInstruction(thisNode))
				continue;

			if (thisTarget.isMem())
			{
				auto nextIt = createIterator(thisNode);

				while (auto nextNode = nextIt.next())
				{
					if (thisNode == nextNode)
						continue;

					auto nextSource = Helpers::getSourceOp(nextNode);

					if (Helpers::opEqualOrSameReg(nextSource, thisTarget))
						break;

					if (Helpers::isInstructWithMemRegAsTarget(nextNode, thisTarget.as<BaseMem>()))
						break;

					if (ControlFlowBreakers::matches(nextNode))
						break;

					if (Mov::matches(nextNode))
					{
						auto nextTarget = Helpers::getTargetOp(nextNode);

						if (Helpers::sameMem(nextTarget, thisTarget))
						{
							
							auto thisSize = thisTarget.size();
							auto nextSize = nextTarget.size();

							auto thisIsBigger = thisSize != 0 && thisSize <= nextSize;

							if (thisIsBigger)
							{
								it.removeNode(thisNode);
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}
};


struct RemoveDoubleRegisterWrites : public AsmCleanupPass::SubPass<InstructionFilter>
{
	RemoveDoubleRegisterWrites(FuncPass* p) :
		SubPass<InstructionFilter>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto thisNode = it.next())
		{
			auto thisTarget = Helpers::getTargetOp(thisNode);

			if (thisTarget.isPhysReg() && Mov::matches(thisNode))
			{
				

				auto nextIt = createIterator(thisNode);

				while (auto nextNode = nextIt.next())
				{
					if (thisNode == nextNode)
						continue;

					auto nextSource = Helpers::getSourceOp(nextNode);

					// Sometimes the cmp instruction is used before
					// moving a value to the operand.
					if (CompareInstructions::matches(nextNode))
						break;

					if (ControlFlowBreakers::matches(nextNode))
						break;

					if (Helpers::opEqualOrSameReg(thisTarget, nextSource))
						break;

					if (Helpers::isMemWithRegAsBaseOrIndex(nextSource, thisTarget.as<X86Reg>()))
						break;

					if (Mov::matches(nextNode))
					{
						auto nextTarget = Helpers::getTargetOp(nextNode);

						if (Helpers::isMemWithRegAsBaseOrIndex(nextTarget, thisTarget.as<X86Reg>()))
							break;

						if (Helpers::opEqualOrSameReg(nextTarget, thisTarget))
						{
							auto idThatIsRemoved = thisTarget.id();
							auto isMemTarget = nextTarget.isMem();

							ignoreUnused(idThatIsRemoved, isMemTarget);

							it.removeNode(thisNode);
							return true;
						}
					}
				}
			}
		}

		return false;
	}
};



struct RemoveMathNoops : public AsmCleanupPass::SubPass<MathOp>
{
	RemoveMathNoops(FuncPass* p) :
		SubPass<MathOp>(p)
	{};

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			auto noopValue = MathOp::getNoopValue(n);

			auto source = Helpers::getSourceOp(n);

			switch (MathOp::getType(n))
			{
			case Types::ID::Integer:
			{
				if (source.isImm() && source.as<Imm>().valueAs<int>() == noopValue)
				{
					it.removeNode(n);
				}
				break;
			}
			case Types::ID::Float:
			{
				float value;

				if (MathOp::getConstPoolValue(parent, source, value))
				{
					if (value == (float)noopValue)
						it.removeNode(n);
				}

				break;
			}
			case Types::ID::Double:
			{
				double value;

				if (MathOp::getConstPoolValue(parent, source, value))
				{
					if (value == (double)noopValue)
						it.removeNode(n);
				}

				break;
			}
			}
		}

		return false;
	}
};

struct RemoveSubsequentMovCalls : public AsmCleanupPass::SubPass<Mov>
{
	RemoveSubsequentMovCalls(FuncPass* p) :
		SubPass<Mov>(p)
	{};

	BaseNode* checkNextMovToSameTarget(BaseNode* current)
	{
		AsmCleanupPass::Iterator<> it(parent, current->next());

		auto currentSourceOp = Helpers::getSourceOp(current->as<InstNode>());

		auto currentBase = Helpers::getBaseRegIndex(currentSourceOp);
		auto currentIndex = Helpers::getIndexRegIndex(currentSourceOp);

		while (auto sn = it.next())
		{
			if (Helpers::isInstructWithSourceAsTarget(current, sn))
				return nullptr;

			if (ControlFlowBreakers::matches(sn))
				return nullptr;

			if (Helpers::isInstructionWithIndexAsTarget(sn, currentBase, currentIndex))
				return nullptr;

			if (Helpers::isInstructWithSameTarget(current, sn))
			{
				if (Mov::matches(sn))
				{
					auto seekedTarget = Helpers::getTargetOp(sn->as<InstNode>());

					if (Helpers::isMemWithBaseReg(seekedTarget))
						return nullptr;

					auto currentSource = Helpers::getSourceOp(current->as<InstNode>());
					auto seekedSource = Helpers::getSourceOp(sn->as<InstNode>());

					if (Helpers::opEqualOrSameReg(currentSource, seekedSource))
						return sn;
					else
						return nullptr;
				}
					
				else
					return nullptr;
			}
		}

		return nullptr;
	}

	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			if (auto nextNode = it.peekNextMatch())
			{
				auto currentTarget = Helpers::getTargetOp(n);
				auto nextTarget = Helpers::getTargetOp(nextNode);

				if (currentTarget.equals(nextTarget))
				{
					auto nextSource = Helpers::getSourceOp(nextNode);

					if (Helpers::isMemWithBaseReg(nextSource))
						continue;

					it.removeNode(n);
					return true;
				}
			}

			if (auto sn = checkNextMovToSameTarget(n))
			{
				auto instMatch = n->baseInst().id() == sn->as<InstNode>()->baseInst().id();

				if (instMatch)
				{
					
					it.removeNode(sn);
					return true;
				}
			}
		}

		return false;
	}
};

struct RemoveUndirtyMovCallsFromSameSource : public AsmCleanupPass::SubPass<Mov>
{
	RemoveUndirtyMovCallsFromSameSource(FuncPass* p) :
		SubPass<Mov>(p)
	{};

	InstNode* getNextUndirtyMovWithSameTarget(BaseNode* current)
	{
		AsmCleanupPass::Iterator<AsmCleanupPass::Base> all(parent, current);

		auto currentSourceOp = Helpers::getSourceOp(current->as<InstNode>());

		auto currentBase = Helpers::getBaseRegIndex(currentSourceOp);
		auto currentIndex = Helpers::getIndexRegIndex(currentSourceOp);

		while (auto n = all.next())
		{
			if (current == n)
				continue;

			if (ControlFlowBreakers::matches(n))
				return nullptr;

			if (Helpers::isInstructWithSourceAsTarget(current, n))
				return nullptr;
			
			if (Helpers::isInstructionWithIndexAsTarget(n, currentBase, currentIndex))
				return nullptr;

			if (Helpers::isInstructWithSameTarget(current, n))
			{
				// If the target is a pointer with a base register
				// it might have been changed.

				auto nextTarget = Helpers::getTargetOp(n->as<InstNode>());

				if (Helpers::isMemWithBaseReg(nextTarget))
					continue;

				if (Mov::matches(n))
				{
					return n->as<InstNode>();
				}
				else
				{
					return nullptr;
				}
			}
		}

		return nullptr;
	}

	/** optimizes next mov calls to the same target with the same source
	  if the target hasn't been used in the meantime.
	*/
	bool run(BaseNode* n) override
	{
		auto it = createIterator(n);

		while (auto n = it.next())
		{
			if (auto undirtyMov = getNextUndirtyMovWithSameTarget(n))
			{
				auto sameInst = n->baseInst().id() == undirtyMov->baseInst().id();

				auto thisSource = Helpers::getSourceOp(n);
				auto nextTarget = Helpers::getTargetOp(undirtyMov);
				auto nextSource = Helpers::getSourceOp(undirtyMov);

				if (sameInst && Helpers::opEqualOrSameReg(thisSource, nextSource))
				{
					it.removeNode(undirtyMov);
					return true;
				}
			}
		}

		return false;
	}
};

}

AsmCleanupPass::AsmCleanupPass() :
	FuncPass("Simple removals of redundant instructions")
{
	addSubPass<AsmSubPasses::RemoveLeaFromSameSource>();
	addSubPass<AsmSubPasses::RemoveMathNoops>();
	addSubPass<AsmSubPasses::RemoveMovToSameOp>();
	addSubPass<AsmSubPasses::RemoveUndirtyMovCallsFromSameSource>();
	addSubPass<AsmSubPasses::RemoveSubsequentMovCalls>();
	addSubPass<AsmSubPasses::RemoveDoubleRegisterWrites>();
	addSubPass<AsmSubPasses::RemoveSwappedMovCallsToMemory>();
	addSubPass<AsmSubPasses::RemoveDoubleMemoryWrites>();
}
#endif

}
}
