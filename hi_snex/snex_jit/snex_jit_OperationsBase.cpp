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



snex::jit::Operations::FunctionCompiler& Operations::getFunctionCompiler(BaseCompiler* c)
{
	return *dynamic_cast<ClassCompiler*>(c)->asmCompiler;
}

BaseScope* Operations::findClassScope(BaseScope* scope)
{
	if (scope == nullptr)
		return nullptr;

	if (scope->getScopeType() == BaseScope::Class)
		return scope;
	else
		return findClassScope(scope->getParent());
}

snex::jit::BaseScope* Operations::findFunctionScope(BaseScope* scope)
{
	if (scope == nullptr)
		return nullptr;

	if (scope->getScopeType() == BaseScope::Function)
		return scope;
	else
		return findFunctionScope(scope->getParent());
}

asmjit::Runtime* Operations::getRuntime(BaseCompiler* c)
{
	return dynamic_cast<ClassCompiler*>(c)->getRuntime();
}



bool Operations::isOpAssignment(Expression::Ptr p)
{
	if (auto as = dynamic_cast<Assignment*>(p.get()))
		return as->assignmentType != JitTokens::assign_;

	return false;
}

Operations::Expression* Operations::findAssignmentForVariable(Expression::Ptr variable, BaseScope*)
{
	if (auto sBlock = findParentStatementOfType<SyntaxTree>(variable.get()))
	{
		for (auto s : *sBlock)
		{
			if (isStatementType<Assignment>(s))
				return dynamic_cast<Expression*>(s);
		}

		return nullptr;
	}
	else
	{
		return nullptr;
	}



#if 0
	if (auto fScope = dynamic_cast<FunctionScope*>(findFunctionScope(scope)))
	{
		auto pf = dynamic_cast<Function*>(fScope->parentFunction.get());

		auto vPtr = dynamic_cast<VariableReference*>(variable.get());

		for (auto s : *pf->statements)
		{
			if (auto as = dynamic_cast<Assignment*>(s))
			{
				auto tv = dynamic_cast<VariableReference*>(as->getSubExpr(0).get());

				if (tv->ref == vPtr->ref)
					return as;
			}
		}

		return nullptr;
	}
#endif
}


snex::Types::ID Operations::Expression::getType() const
{
	return type;
}

void Operations::Expression::attachAsmComment(const String& message)
{
	asmComment = message;
}

void Operations::Expression::checkAndSetType(int offset /*= 0*/, Types::ID expectedType)
{
	Types::ID exprType = expectedType;

	for (int i = offset; i < getNumChildStatements(); i++)
	{
		auto e = getSubExpr(i);

		if (auto v = dynamic_cast<VariableReference*>(e.get()))
		{
			if (!v->functionClassConstant.isVoid())
			{
				v->type = expectedType;
				continue;
			}
		}

		auto thisType = e->getType();

		if (!Types::Helpers::matchesTypeStrict(thisType, exprType))
		{
			logWarning("Implicit cast, possible lost of data");

			if (e->isConstExpr())
			{
				replaceChildStatement(i, ConstExprEvaluator::evalCast(e.get(), exprType).get());
			}
			else
			{
				Ptr implCast = new Operations::Cast(e->location, e, exprType);
				implCast->attachAsmComment("Implicit cast");

				replaceChildStatement(i, implCast);
			}
		}
		else
			exprType = Types::Helpers::getMoreRestrictiveType(exprType, thisType);
	}

	type = exprType;
}




void Operations::Expression::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::process(compiler, scope);

	for (int i = 0; i < getNumChildStatements(); i++)
	{
		if (getChildStatement(i) != nullptr)
			getChildStatement(i)->process(compiler, scope);
	}
}

bool Operations::Expression::isAnonymousStatement() const
{
	return isStatementType<StatementBlock>(parent) ||
		isStatementType<SyntaxTree>(parent);
}

snex::VariableStorage Operations::Expression::getConstExprValue() const
{
	if (isConstExpr())
		return dynamic_cast<const Immediate*>(this)->v;

	return {};
}

bool Operations::Expression::hasSubExpr(int index) const
{
	return isPositiveAndBelow(index, getNumChildStatements());
}

Operations::Expression::Ptr Operations::Expression::getSubExpr(int index) const
{
	return Expression::Ptr(dynamic_cast<Expression*>(getChildStatement(index).get()));
}

snex::jit::Operations::RegPtr Operations::Expression::getSubRegister(int index) const
{
	// If you hit this, you have either forget to call the 
	// Statement::process() or you try to access an register 
	// way too early...
	jassert(currentPass >= BaseCompiler::Pass::RegisterAllocation);

	if (auto e = getSubExpr(index))
		return e->reg;

	// Can't find the subexpression you want
	jassertfalse;

	return nullptr;
}


SyntaxTree::SyntaxTree(ParserHelpers::CodeLocation l) :
	Statement(l)
{

}

void SyntaxTree::addVariableReference(Operations::Statement* s)
{
	variableReferences.add(s);
}

bool SyntaxTree::isFirstReference(Operations::Statement* v_) const
{
	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (variable->ref == dynamic_cast<Operations::VariableReference*>(v_)->ref)
				return v == v_;
		}
	}

	return false;
}

Operations::Statement* SyntaxTree::getLastVariableForReference(BaseScope::RefPtr ref) const
{
	Statement* lastRef = nullptr;

	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (ref == variable->ref)
				lastRef = variable;
		}
	}

	return lastRef;
}

Operations::Statement* SyntaxTree::getLastAssignmentForReference(BaseScope::RefPtr ref) const
{
	Statement* lastAssignment = nullptr;

	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (ref == variable->ref)
			{
				if (auto as = dynamic_cast<Operations::Assignment*>(v->parent.get()))
				{
					if (as->getTargetVariable() == variable)
						lastAssignment = as;
				}
			}
		}
	}

	return lastAssignment;
}


Operations::Statement::Statement(Location l) :
	location(l)
{

}

void Operations::Statement::process(BaseCompiler* compiler, BaseScope* scope)
{
	currentCompiler = compiler;
	currentScope = scope;
	currentPass = compiler->getCurrentPass();

	if (BaseCompiler::isOptimizationPass(currentPass))
	{
		compiler->executeOptimization(this, scope);
	}
}

void Operations::Statement::throwError(const String& errorMessage)
{
	ParserHelpers::CodeLocation::Error e(location.program, location.location);
	e.errorMessage = errorMessage;
	throw e;
}

void Operations::Statement::logOptimisationMessage(const String& m)
{
	logMessage(currentCompiler, BaseCompiler::VerboseProcessMessage, m);
}

void Operations::Statement::logWarning(const String& m)
{
	logMessage(currentCompiler, BaseCompiler::Warning, m);
}

bool Operations::Statement::isConstExpr() const
{
	return isStatementType<Immediate>(static_cast<const Statement*>(this));
}

void Operations::Statement::addStatement(Statement* b, bool addFirst/*=false*/)
{
	if (addFirst)
		childStatements.insert(0, b);
	else
		childStatements.add(b);

	b->setParent(this);
}

Operations::Statement::Ptr Operations::Statement::replaceInParent(Statement::Ptr newExpression)
{
	if (parent != nullptr)
	{
		for (int i = 0; i < parent->getNumChildStatements(); i++)
		{
			if (parent->getChildStatement(i).get() == this)
			{
				Ptr f(this);
				parent->childStatements.set(i, newExpression.get());
				newExpression->parent = parent;
				return f;
			}
		}
	}

	jassertfalse;
	return nullptr;
}


Operations::Statement::Ptr Operations::Statement::replaceChildStatement(int index, Ptr newExpr)
{
	Ptr returnExpr;

	if (returnExpr = getChildStatement(index))
	{
		childStatements.set(index, newExpr.get());
		newExpr->parent = this;

		if (returnExpr->parent == this)
			returnExpr->parent = nullptr;
	}
	else
		jassertfalse;

	return returnExpr;
}

void Operations::Statement::logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const String& message)
{
	if (!compiler->hasLogger())
		return;

	String m;

	m << "Line " << location.getLineNumber(location.program, location.location) << ": ";
	m << message;

	DBG(m);

	compiler->logMessage(type, m);
}

void Operations::ConditionalBranch::allocateDirtyGlobalVariables(Statement::Ptr statementToSearchFor, BaseCompiler* c, BaseScope* s)
{
	SyntaxTreeWalker w(statementToSearchFor, false);

	while (auto v = w.getNextStatementOfType<VariableReference>())
	{
		// If use a class variable, we need to create the register
		// outside the loop
		if (v->isClassVariable() && v->isFirstReference())
			v->process(c, s);
	}
}

}
}
