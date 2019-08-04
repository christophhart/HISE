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

#if 0
RegisterAllocator::RegisterAllocator(Compiler& cc_):
	cc(cc_)
{}


bool RegisterAllocator::isLeftOfAssignment(ExprPtr vRef, bool dontCareAboutSelfAssign)
{
	if (auto as = dynamic_cast<Operations::Assignment*>(vRef->parent.get()))
	{
		return (as->getSubExpr(0) == vRef) && 
			   (dontCareAboutSelfAssign || as->assignmentType == JitTokens::assign_);
	}

	return false;
}

void RegisterAllocator::createAssemblyRegister(ExprPtr ptr)
{
	jassert(ptr->currentPass == BaseCompiler::RegisterAllocation);

	RegPtr reg = ptr->getRegister();
	reg->createRegister(cc);
}


void RegisterAllocator::allocateBinaryOp(ExprPtr binaryOp)
{
	auto l = binaryOp->getSubExpr(0);

	RegPtr regToUse;

	if (l->optData.isTemporary)
		regToUse = createRegisterPtr(l);
	else
		regToUse = createRegisterPtr(binaryOp);
	
	binaryOp->optData.isTemporary = true;
	binaryOp->setRegister(regToUse);
	createAssemblyRegister(binaryOp);
}


void RegisterAllocator::allocateReturnRegister(ExprPtr returnNode)
{
	auto type = returnNode->getType();

	if (returnNode->getType() == Types::ID::Void) 
		return;

	auto newReg = new AssemblyRegister(returnNode->getType());
	returnNode->setRegister(newReg);

	newReg->createRegister(cc);
}
#endif




juce::Result Inliner::process(SyntaxTree* tree)
{
	for (auto s : *tree)
	{
		if (auto v = as<Operations::VariableReference>(s))
		{
#if 0
			bool canBeInlined = v->ref->getNumReferences() == 2 && // single usage
				!v->isFirstReference() &&		  // not the definition
				!v->isClassVariable;			  // not a class variable

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

	return Result::ok();
}

void Inliner::swapBinaryOpIfPossible(ExprPtr binaryOp)
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

void Inliner::createSelfAssignmentFromBinaryOp(ExprPtr assignment)
{
	auto as = dynamic_cast<Operations::Assignment*>(assignment.get());

	if (as->assignmentType != JitTokens::assign_)
		return;

	if (auto bOp = dynamic_cast<Operations::BinaryOp*>(assignment->getSubExpr(1).get()))
	{
		bool isSelfAssignment = false;

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
				as->replaceSubExpr(1, bOp->getSubExpr(1));
				return;
			}
		}

		if (auto v_r = dynamic_cast<Operations::VariableReference*>(bOp->getSubExpr(1).get()))
		{
			if (v_r->ref == as->getTargetVariable()->ref)
			{
				as->logOptimisationMessage("Create self assign");
				as->assignmentType = bOp->op;
				as->replaceSubExpr(1, bOp->getSubExpr(0));
				return;
			}
		}
	}
}


bool Inliner::containsVariableReference(ExprPtr p, BaseScope::RefPtr refToCheck)
{
	if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
	{
		return v->ref == refToCheck;
	}

	for (int i = 0; i < p->getNumSubExpressions(); i++)
	{
		if (containsVariableReference(p->getSubExpr(i), refToCheck))
			return true;
	}

	return false;
}


juce::Result DeadcodeEliminator::process(SyntaxTree* tree)
{
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
				(v->isClassVariable || v->parameterIndex != -1))
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
			bool notGlobal = !target->isClassVariable;


			if (singleReference && notGlobal)
				a->optimizeAway();
		}
	}
#endif

	return Result::ok();
}

}
}