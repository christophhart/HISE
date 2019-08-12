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
	s->logOptimisationMessage("Remove statement");
	replaceExpression(s, new Operations::Noop(s->location));
}

struct VOps
{
#define VAR_OP(name, opChar) static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(l.getType(), l.toDouble() opChar r.toDouble()); }
#define VAR_OP_INT(name, opChar) static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(l.getType(), l.toInt() opChar r.toInt()); }

	VAR_OP(plus, +);
	VAR_OP(times, *);
	VAR_OP(divide, / );
	VAR_OP(minus, -);
	VAR_OP_INT(modulo, %);
	VAR_OP(greaterThan, >);
	VAR_OP(lessThan, < );
	VAR_OP(greaterThanOrEqual, >= );
	VAR_OP(lessThanOrEqual, <= );
	VAR_OP(equals, == );
	VAR_OP(notEquals, != );
	VAR_OP_INT(logicalAnd, &&);
	VAR_OP_INT(logicalOr, ||);

#undef VAR_OP
#undef VAR_OP_INT
};

void ConstExprEvaluator::process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
	{
		if (auto v = as<Operations::VariableReference>(statement))
		{
			addConstKeywordToSingleWriteVariables(v, s, compiler);
		}

		if (auto is = as<Operations::IfStatement>(statement))
		{
			is->cond->process(compiler, s);

			if (is->cond->isConstExpr())
			{
				int v = is->cond->getConstExprValue().toInt();

				is->logOptimisationMessage("Remove dead branch");

				if (v == 1)
					replaceExpression(is, is->trueBranch.get());
				else
				{
					if (is->falseBranch != nullptr)
						replaceExpression(is, is->falseBranch);
					else
						replaceWithNoop(is);
				}
					
			}
		}

		if (auto a = as<Operations::Assignment>(statement))
		{
			process(compiler, s, a->getSubExpr(0));

			if (a->getSubExpr(0)->isConstExpr())
			{
				auto value = a->getSubExpr(0)->getConstExprValue();

				process(compiler, s, a->getSubExpr(1));

				if (a->getTargetVariable()->isLocalConst)
				{
					String s = "Replace with " + Types::Helpers::getCppValueString(value);

					compiler->logMessage(BaseCompiler::ProcessMessage, s);
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
			}
			else if (bOp->isLogicOp())
			{
				if (bOp->getSubExpr(1)->isConstExpr())
					bOp->swapSubExpressions(0, 1);

				if (bOp->getSubExpr(0)->isConstExpr())
				{
					auto lValue = bOp->getSubExpr(0)->getConstExprValue();

					if (bOp->op == JitTokens::logicalAnd && lValue.toInt() == 0)
					{
						statement->logOptimisationMessage("short-circuit constant && op");
						replaceExpression(statement, new Operations::Immediate(statement->location, 0));
					}
					else if (bOp->op == JitTokens::logicalOr && lValue.toInt() == 1)
					{
						statement->logOptimisationMessage("short-circuit constant || op");
						replaceExpression(statement, new Operations::Immediate(statement->location, 1));
					}
					else
					{
						statement->logOptimisationMessage("removed constant condition in logic op");
						replaceExpression(statement, bOp->getSubExpr(1));
					}
				}
			}

		}
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (auto a = as<Operations::Assignment>(statement))
		{
			if (a->getTargetVariable()->isLocalConst)
			{
				constantVariables.add(a->getTargetVariable()->ref);
			}
		}

		

		if (auto n = as<Operations::Negation>(statement))
		{
			if (auto ce = evalNegation(n->getSubExpr(0)))
				replaceExpression(statement, ce);
		}
	}

	

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

void ConstExprEvaluator::addConstKeywordToSingleWriteVariables(Operations::VariableReference* v, BaseScope* s, BaseCompiler* compiler)
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
	jassertfalse;

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



#undef IS


void Inliner::process(BaseCompiler* c, BaseScope* s, StatementPtr statement)
{
#if 0
	for (auto s : *tree)
	{
		if (auto v = as<Operations::VariableReference>(s))
		{
#if 0
			bool canBeInlined = v->ref->getNumReferences() == 2 && // single usage
				!v->isFirstReference() &&		  // not the definition
				!v->isClassVariable();			  // not a class variable

			if (canBeInlined)
			{
				if (auto as = v->findAssignmentForVariable(this, scope))
				{
					bool isNoSelfAssignment = !as->isOpAssignment(as);
					bool isSameLine = dynamic_cast<Operations::VariableReference*>(as->getSubExpr(0).get())->ref == v->ref;

					if (isNoSelfAssignment && !isSameLine && v->parameterIndex == -1) // Self assignments shouldn't be inlined
					{
						v->logOptimisationMessage("Inline variable " + v->ref->id.toString());

						ExprPtr replacement = as->getSubExpr(1);
						as->optimizeAway();
						v->replaceInParent(replacement);
					}
				}
			}
#endif
		}
		else if (auto a = as<Operations::Assignment>(s))
		{
			createSelfAssignmentFromBinaryOp(a);
		}
		else if (auto b = as<Operations::BinaryOp>(s))
		{
			// Move into seperate pass
			swapBinaryOpIfPossible(b);
		}
	}

#endif
}


void DeadcodeEliminator::process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	COMPILER_PASS(BaseCompiler::PostSymbolOptimization)
	{
        if (auto imm = as<Operations::Immediate>(statement))
        {
            if(imm->isAnonymousStatement())
                replaceWithNoop(imm);
        }
        
		if (auto a = as<Operations::Assignment>(statement))
		{
			auto v = a->getTargetVariable();

			if (v->isClassVariable())
				return;

			if (v->isParameter(s))
				return;

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
			}
		}
	}
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

void BinaryOpOptimizer::process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement)
{
	COMPILER_PASS(BaseCompiler::PreSymbolOptimization)
	{
		if (auto a = as<Operations::Assignment>(statement))
		{
			currentlyAssignedId = a->getTargetVariable()->id;

			a->getSubExpr(0)->process(compiler, s);

			DBG("Optimize assignment + " + currentlyAssignedId.toString());

			if (auto bOp = dynamic_cast<Operations::BinaryOp*>(a->getSubExpr(0).get()))
			{
				if (isAssignedVariable(bOp->getSubExpr(0)))
				{
					a->logOptimisationMessage("Replace " + String(bOp->op) + " with self assignment");
                    
                    a->assignmentType = bOp->op;
                    
                    auto right = bOp->getSubExpr(1);
                    
                    replaceExpression(bOp, right);
				}
			}

			currentlyAssignedId = {};

		}

		if (auto bOp = as<Operations::BinaryOp>(statement))
		{
			if (!currentlyAssignedId)
				return;

			StatementPtr rightOp(bOp->getSubExpr(1).get());

			rightOp->process(compiler, s);

			
			DBG("Optimize op " + String(bOp->op));

			if (isAssignedVariable(bOp->getSubExpr(0)))
			{
				bOp->logOptimisationMessage("Good order");
			}
			else if (isAssignedVariable(bOp->getSubExpr(1)))
			{
				if (Helpers::isSwappable(bOp->op))
				{
					bOp->logOptimisationMessage("Wrong order, swap them");
					bOp->swapSubExpressions(0, 1);
				}
			}
		}
	}
	
}

bool BinaryOpOptimizer::isAssignedVariable(ExprPtr e) const
{
	if (auto v = dynamic_cast<Operations::VariableReference*>(e.get()))
	{
		return v->id == currentlyAssignedId;
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

bool BinaryOpOptimizer::containsVariableReference(ExprPtr p, BaseScope::RefPtr refToCheck)
{
	if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
	{
		return v->ref == refToCheck;
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

	BaseScope::RefPtr targetRef;

	if (auto assignment = Operations::findParentStatementOfType<Operations::Assignment>(binaryOp.get()))
	{
		auto v = dynamic_cast<Operations::VariableReference*>(assignment->getSubExpr(0).get());

		targetRef = v->ref;
	}

	if (targetRef != nullptr && containsVariableReference(r, targetRef))
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
			if (v_l->ref == as->getTargetVariable()->ref)
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
			if (v_r->ref == as->getTargetVariable()->ref)
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
