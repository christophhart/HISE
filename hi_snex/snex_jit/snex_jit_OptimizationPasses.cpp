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
using namespace asmjit;

void OptimizationPass::replaceWithNoop(StatementPtr s)
{
	replaceExpression(s, new Operations::Noop(s->location));
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
	// run both times, since something could become constexpr during symbol resolving...
	if(compiler->getCurrentPass() == BaseCompiler::PostSymbolOptimization ||
	   compiler->getCurrentPass() == BaseCompiler::PreSymbolOptimization)
	{
		

		if (auto fc = as<Operations::FunctionCall>(statement))
		{
			if (auto constResult = evalConstMathFunction(fc))
			{
				statement->logOptimisationMessage("Remove const Math function call");
				replaceExpression(statement, constResult);
				return true;
			}
		}

		if (auto dot = as<Operations::DotOperator>(statement))
		{
			if (auto constResult = evalDotOperator(s, dot))
			{
				statement->logOptimisationMessage("Remove const dot operator");
				replaceExpression(statement, constResult);
				return true;
			}
		}

		if (auto is = as<Operations::BranchingStatement>(statement))
		{
			is->getCondition()->process(compiler, s);

			auto cond = dynamic_cast<Operations::Expression*>(statement->getChildStatement(0).get());

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

		if (auto c = as<Operations::Cast>(statement))
		{
			if (auto imm = evalCast(c->getSubExpr(0), c->getType()))
			{
				replaceExpression(c, imm);
				return true;
			}
		}

		if (auto a = as<Operations::Assignment>(statement))
		{
			a->getSubExpr(0)->process(compiler, s);

			if (a->getSubExpr(0)->isConstExpr())
			{
				auto value = a->getSubExpr(0)->getConstExprValue();

				a->getSubExpr(1)->process(compiler, s);


				if (a->getTargetType() == Operations::Assignment::TargetType::Variable)
				{
					auto target = a->getTargetVariable();

					if (target != nullptr)
					{
						int numWriteAccesses = target->getNumWriteAcesses();

						if (s->getScopeType() != BaseScope::Class && numWriteAccesses == 1 && !target->id.isConst())
						{
							a->logWarning("const value is declared as non-const");
						}
					}
				}
			}
		}

		if (auto cOp = as<Operations::Compare>(statement))
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

		if (auto bOp = as<Operations::BinaryOp>(statement))
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
		MathFunctions m;

		if (auto t = dynamic_cast<Operations::VariableReference*>(functionCall->objExpr.get()))
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
			if (dynamic_cast<Operations::ReturnStatement*>(p.get()) != nullptr)
				return true;

			return false;
		});

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
						auto base = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(s);

						newFalseBranch = new Operations::StatementBlock(s->location, s->location.createAnonymousScopeId(base->getPath()));
					}
						
					replaceWithNoop(s);
					newFalseBranch->addStatement(s);
				}
			}

			is->addStatement(newFalseBranch);

			return true;
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
		jassert(fc->objExpr != nullptr);

		auto thisSymbol = Symbol("this");

		Operations::Expression::Ptr e = as<Operations::Expression>(fc->objExpr->clone(fc->location).get());

		cs->addInlinedParameter(-1, thisSymbol, e);

		if (auto st = fc->objExpr->getTypeInfo().getTypedIfComplexType<StructType>())
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
			});
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
		BaseCompiler::ScopedPassSwitcher s1(compiler, BaseCompiler::DataSizeCalculation);
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

			

			auto v = a->getTargetVariable();

			if (v->isClassVariable(s))
				return false;

			if (v->objectPtr != nullptr)
				return false;

			if (Operations::findParentStatementOfType<Operations::MemoryReference>(v) != nullptr)
				return false;

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
				compiler->logMessage(BaseCompiler::Warning, "Unused variable " + v->id.toString());
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
	COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
	{
		if (auto bOp = as<Operations::BinaryOp>(statement))
		{
			if (simplifyOp(bOp->getSubExpr(0), bOp->getSubExpr(1), bOp->op, compiler, s))
				return true;

			if (swapIfBetter(bOp, bOp->op, compiler, s))
				return true;
		}
	}

	COMPILER_PASS(BaseCompiler::PostSymbolOptimization)
	{
		if (auto bOp = as<Operations::BinaryOp>(statement))
		{
			if (swapIfBetter(bOp, bOp->op, compiler, s))
				return true;
		}

		if (auto a = as<Operations::Assignment>(statement))
		{
			if (simplifyOp(a->getSubExpr(1), a->getSubExpr(0), a->assignmentType, compiler, s))
				return true;

			if (a->getTargetType() == Operations::Assignment::TargetType::Variable)
			{
				currentlyAssignedId.s = a->getTargetVariable()->id;
				currentlyAssignedId.scope = a->getTargetVariable()->variableScope;

				a->getSubExpr(0)->process(compiler, s);

				if (auto bOp = dynamic_cast<Operations::BinaryOp*>(a->getSubExpr(0).get()))
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
	auto parent = l->parent;

	auto replaceOp = [this](Operations::Statement* p, const char* o)
	{
		if (auto bOp = as<Operations::BinaryOp>(p))
			bOp->op = o;
		else if (auto a = as<Operations::Assignment>(p))
			a->assignmentType = o;
	};

	
	if (op == JitTokens::minus)
	{
		if (r->isConstExpr())
		{
			parent->logOptimisationMessage("Replace minus");
			
			auto value = r->getConstExprValue();
			auto invValue = VariableStorage(value.getType(), value.toDouble() * -1.0);

			replaceExpression(r, new Operations::Immediate(r->location, invValue));
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

			replaceExpression(r, new Operations::Immediate(r->location, invValue));
			replaceOp(parent, JitTokens::times);

			return true;
		}
	}

	return false;
}

bool BinaryOpOptimizer::isAssignedVariable(ExprPtr e) const
{
	if (auto v = dynamic_cast<Operations::VariableReference*>(e.get()))
	{
		return SymbolWithScope({ v->id, v->variableScope}) == currentlyAssignedId;
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
	if (e == nullptr)
		return nullptr;

	if (auto bOp = getFirstOp(e->getSubExpr(0)))
		return bOp;

	if(auto thisOp = dynamic_cast<Operations::BinaryOp*>(e.get()))
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
			if (v_l->id == as->getTargetVariable()->id)
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
			if (v_r->id == as->getTargetVariable()->id)
			{
				as->logOptimisationMessage("Create self assign");
				as->assignmentType = bOp->op;
				as->replaceChildStatement(1, bOp->getSubExpr(0));
				return;
			}
		}
	}
}

}
}
