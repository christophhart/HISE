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
	}

	return nullptr;
}



void Operations::Expression::attachAsmComment(const juce::String& message)
{
	asmComment = message;
}

TypeInfo Operations::Expression::checkAndSetType(int offset, TypeInfo expectedType)
{
	TypeInfo exprType = expectedType;

	if (expectedType.isInvalid())
	{
		// Native types have precedence so that the complex types can call their cast operators...
		for (int i = offset; i < getNumChildStatements(); i++)
		{
			auto thisType = getChildStatement(i)->getTypeInfo();
			if (thisType.isComplexType())
				continue;

			exprType = thisType;
		}
	}

	for (int i = offset; i < getNumChildStatements(); i++)
	{
		exprType = setTypeForChild(i, exprType);
	}

	return exprType;
}




TypeInfo Operations::Expression::setTypeForChild(int childIndex, TypeInfo expectedType)
{
	auto e = getSubExpr(childIndex);

	if (auto v = dynamic_cast<VariableReference*>(e.get()))
	{
		bool isDifferentType = expectedType != Types::ID::Dynamic && expectedType != v->getConstExprValue().getType();

		if (v->isConstExpr() && isDifferentType)
		{
			// Internal cast of a constexpr variable...
			v->id.constExprValue = VariableStorage(expectedType.getType(), v->id.constExprValue.toDouble());
			return expectedType;
		}
	}

	auto thisType = e->getTypeInfo();

	if (expectedType.isInvalid())
	{
		return thisType;
	}

	if (!thisType.isComplexType() && (thisType == expectedType.getType()))
	{
		// the expected complex type can be implicitely casted to the native type
		return expectedType;
	}

	if (!expectedType.isComplexType() && expectedType == thisType.getType())
	{
		// this type can be implicitely casted to the native expected type;
		return thisType;
	}
		

	if (expectedType != thisType)
	{
		if (auto targetType = expectedType.getTypedIfComplexType<ComplexType>())
		{
			if(!targetType->isValidCastSource(thisType.getType(), thisType.getTypedIfComplexType<ComplexType>()))
				throwError(juce::String("Can't cast ") + thisType.toString() + " to " + expectedType.toString());
		}

		if (auto sourceType = thisType.getTypedIfComplexType<ComplexType>())
		{
			if (!sourceType->isValidCastTarget(expectedType.getType(), expectedType.getTypedIfComplexType<ComplexType>()))
				throwError(juce::String("Can't cast ") + thisType.toString() + " to " + expectedType.toString());
		}

		
			

		logWarning("Implicit cast, possible lost of data");

		if (e->isConstExpr())
		{
			replaceChildStatement(childIndex, ConstExprEvaluator::evalCast(e.get(), expectedType.getType()).get());
		}
		else
		{
			Ptr implCast = new Operations::Cast(e->location, e, expectedType.getType());
			implCast->attachAsmComment("Implicit cast");

			replaceChildStatement(childIndex, implCast);
		}
	}

	return expectedType;
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

void Operations::Expression::processChildrenIfNotCodeGen(BaseCompiler* compiler, BaseScope* scope)
{
	if (isCodeGenPass(compiler))
		Statement::process(compiler, scope);
	else
		Expression::process(compiler, scope);
}

bool Operations::Expression::isCodeGenPass(BaseCompiler* compiler) const
{
	return compiler->getCurrentPass() == BaseCompiler::RegisterAllocation ||
		   compiler->getCurrentPass() == BaseCompiler::CodeGeneration;
}

bool Operations::Expression::preprocessCodeGenForChildStatements(BaseCompiler* compiler, BaseScope* scope, const std::function<bool()>& abortFunction)
{
	if (reg != nullptr)
		return false;

	if(compiler->getCurrentPass() == BaseCompiler::RegisterAllocation)
	{
		BaseCompiler::ScopedPassSwitcher svs(compiler, BaseCompiler::RegisterAllocation);
		getSubExpr(0)->process(compiler, scope);
		getSubExpr(1)->process(compiler, scope);

		if (!abortFunction())
			return false;;
	}

	BaseCompiler::ScopedPassSwitcher svs(compiler, BaseCompiler::CodeGeneration);
	getSubExpr(0)->process(compiler, scope);
	getSubExpr(1)->process(compiler, scope);

	return true;
}

void Operations::Expression::replaceMemoryWithExistingReference(BaseCompiler* compiler)
{
	jassert(reg != nullptr);

	auto prevReg = compiler->registerPool.getRegisterWithMemory(reg);

	if (prevReg != reg)
		reg = prevReg;
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

snex::VariableStorage Operations::Expression::getPointerValue() const
{
	location.throwError("Can't use address of temporary register");
	return {};
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


SyntaxTree::SyntaxTree(ParserHelpers::CodeLocation l, const NamespacedIdentifier& ns) :
	Statement(l),
	ScopeStatementBase(ns)
{
}

bool SyntaxTree::isFirstReference(Operations::Statement* v_) const
{
	SyntaxTreeWalker m(v_);

	if(auto v = m.getNextStatementOfType<Operations::VariableReference>())
	{
		return v == v_;
	}

	return false;
}


snex::jit::Operations::Statement::Ptr SyntaxTree::clone(ParserHelpers::CodeLocation l) const
{
	Statement::Ptr c = new Operations::StatementBlock(l, getPath());
	dynamic_cast<Operations::StatementBlock*>(c.get())->isInlinedFunction = true;

	cloneChildren(c);
	return c;
}

Operations::Statement::Statement(Location l) :
	location(l)
{

}

void Operations::Statement::process(BaseCompiler* compiler, BaseScope* scope)
{
	jassert(scope != nullptr);

	if (parent == nullptr)
		return;

	currentCompiler = compiler;
	currentScope = scope;
	currentPass = compiler->getCurrentPass();



	if (BaseCompiler::isOptimizationPass(currentPass))
	{
		bool found = false;

		for (int i = 0; i < parent->getNumChildStatements(); i++)
			found |= parent->getChildStatement(i).get() == this;

		if(found)
			compiler->executeOptimization(this, scope);
	}
}

void Operations::Statement::throwError(const juce::String& errorMessage)
{
	ParserHelpers::CodeLocation::Error e(location.program, location.location);
	e.errorMessage = errorMessage;
	throw e;
}

void Operations::Statement::logOptimisationMessage(const juce::String& m)
{
	logMessage(currentCompiler, BaseCompiler::VerboseProcessMessage, m);
}

void Operations::Statement::logWarning(const juce::String& m)
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
				//parent = nullptr;
				return f;
			}
		}
	}

	
	return nullptr;
}


Operations::Statement::Ptr Operations::Statement::replaceChildStatement(int index, Ptr newExpr)
{
	Ptr returnExpr;

	if ((returnExpr = getChildStatement(index)))
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

void Operations::Statement::logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const juce::String& message)
{
	if (!compiler->hasLogger())
		return;

	juce::String m;

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
		if (v->isClassVariable(s) && v->isFirstReference())
			v->process(c, s);
	}
}

Operations::Statement::Ptr Operations::ScopeStatementBase::createChildBlock(Location l) const
{
	return new StatementBlock(l, l.createAnonymousScopeId(getPath()));
}

}
}
