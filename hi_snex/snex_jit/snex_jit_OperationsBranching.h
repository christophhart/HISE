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


struct Operations::StatementBlock : public Expression,
	public ScopeStatementBase
{
	Identifier getStatementId() const override
	{
		if (isInlinedFunction)
		{
			RETURN_STATIC_IDENTIFIER("InlinedFunction");
		}
		else
		{
			RETURN_STATIC_IDENTIFIER("StatementBlock");
		}
	}

	StatementBlock(Location l, const NamespacedIdentifier& ns) :
		Expression(l),
		ScopeStatementBase(ns)
	{}

	TypeInfo getTypeInfo() const override { return returnType; };

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		Statement::Ptr c = new StatementBlock(l, getPath());
		auto b = dynamic_cast<StatementBlock*>(c.get());
		b->isInlinedFunction = isInlinedFunction;
		cloneScopeProperties(b);
		cloneChildren(c);
		return c;
	}

	ValueTree toValueTree() const override
	{
		auto v = Statement::toValueTree();
		v.setProperty("ScopeId", getPath().toString(), nullptr);

		if (isInlinedFunction)
			v.setProperty("ReturnType", getTypeInfo().toStringWithoutAlias(), nullptr);

		return v;
	}

	static bool isRealStatement(Statement* s);

	bool isConstExpr() const override
	{
		for (auto s : *this)
		{
			if (!s->isConstExpr())
				return false;
		}

		return true;
	}

	Ptr getThisExpression();

	bool hasSideEffect() const override
	{
		return isInlinedFunction;
	}

	VariableStorage getConstExprValue() const override
	{
		int numStatements = getNumChildStatements();

		if (numStatements == 0)
			return VariableStorage(Types::ID::Void, 0);

		return getSubExpr(numStatements - 1)->getConstExprValue();
	}

	void addInlinedParameter(int index, const Symbol& s, Ptr e)
	{
		auto ia = new InlinedArgument(location, index, s, e);
		addStatement(ia, true);
	}

	static InlinedArgument* findInlinedParameterInParentBlocks(Statement* p, const Symbol& s);

	BaseScope* createOrGetBlockScope(BaseScope* parent);

	void addInlinedReturnJump(AsmJitX86Compiler& cc);

	void process(BaseCompiler* compiler, BaseScope* scope);

	ScopedPointer<RegisterScope> blockScope;
	bool isInlinedFunction = false;

	

private:

	AsmJitLabel endLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatementBlock);
};


struct Operations::AnonymousBlock : public Expression
{
	AnonymousBlock(Location l) :
		Expression(l)
	{};

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	virtual void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		processBaseWithChildren(compiler, scope);
	}

	virtual Ptr clone(Location l) const override
	{
		auto ab = new AnonymousBlock(l);

		for (auto e : *this)
		{
			ab->addStatement(e->clone(l));
		}
		return ab;
	}

	Identifier getStatementId() const override { return "AnonymousBlock"; }
};


struct Operations::ReturnStatement : public Expression,
								     public StatementWithControlFlowEffectBase
{
	ReturnStatement(Location l, Expression::Ptr expr) :
		Expression(l)
	{
		if (expr != nullptr)
			addStatement(expr);
	}

	Identifier getStatementId() const override
	{
		if (findInlinedRoot() != nullptr)
			return Identifier("InlinedReturnValue");
		else
			return Identifier("ReturnStatement");
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		Statement::Ptr p = !isVoid() ? getSubExpr(0)->clone(l) : nullptr;
		return new ReturnStatement(l, p);
	}

	bool isConstExpr() const override
	{
		return isVoid() || getSubExpr(0)->isConstExpr();
	}

	VariableStorage getConstExprValue() const override
	{
		return isVoid() ? VariableStorage(Types::ID::Void, 0) : getSubExpr(0)->getConstExprValue();
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		t.setProperty("Type", getTypeInfo().toString(), nullptr);

		if (getTypeInfo().isComplexType() && !getTypeInfo().isRef())
			t.setProperty("ReturnBlockSize", getTypeInfo().getRequiredByteSizeNonZero(), nullptr);

		return t;
	}

	TypeInfo getTypeInfo() const override
	{
		if (auto sl = ScopeStatementBase::getStatementListWithReturnType(const_cast<ReturnStatement*>(this)))
			return sl->getReturnType();

		jassertfalse;
		return {};
	}

	bool isVoid() const
	{
		return getTypeInfo() == Types::ID::Void;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	StatementBlock* findInlinedRoot() const;;

	ScopeStatementBase* findRoot() const override;

	bool shouldAddDestructor(ScopeStatementBase* b, const Symbol& id) const override
	{
		return findRoot() == b;
	}

#if 0
	ScopeStatementBase* findRoot() const override
	{
		return ScopeStatementBase::getStatementListWithReturnType(const_cast<ReturnStatement*>(this));
	}
#endif


	Ptr getReturnValue()
	{
		return !isVoid() ? getSubExpr(0) : nullptr;
	}

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReturnStatement);
};



struct Operations::TernaryOp : public Expression,
	public BranchingStatement
{
public:

	SET_EXPRESSION_ID(TernaryOp);

	TernaryOp(Location l, Ptr c, Ptr t, Ptr f) :
		Expression(l)
	{
		addStatement(c);
		addStatement(t);
		addStatement(f);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);
		auto c3 = getSubExpr(2)->clone(l);
		return new TernaryOp(l, c1, c2, c3);
	}

	TypeInfo getTypeInfo() const override {	return type; }

	void process(BaseCompiler* compiler, BaseScope* scope) override;

private:

	TypeInfo type;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TernaryOp);

};

struct Operations::WhileLoop : public Statement,
	public Operations::ConditionalBranch
{
	enum class LoopType
	{
		While,
		For,
		numLoopTypes
	};

	enum class ChildStatementType
	{
		Initialiser,
		Condition,
		Body,
		PostBodyOp,
		numChildstatementTypes
	};

	SET_EXPRESSION_ID(WhileLoop);

	WhileLoop(Location l, Ptr condition, Ptr body) :
		Statement(l),
		loopType(LoopType::While)
	{
		addStatement(condition);
		addStatement(body);
	}

	WhileLoop(Location l, Ptr initialiser, Ptr condition, Ptr body, Ptr postOp):
		Statement(l),
		loopType(LoopType::For)
	{
		addStatement(initialiser);
		addStatement(condition);
		addStatement(body);
		addStatement(postOp);
	}

	ValueTree toValueTree() const override
	{
		auto v = Statement::toValueTree();

		StringArray types = { "While", "For" };
		
		v.setProperty("LoopType", types[(int)loopType], nullptr);

		return v;
	}

	Statement::Ptr clone(Location l) const override
	{
		if (loopType == LoopType::While)
		{
			auto c = getSubExpr(0)->clone(l);
			auto b = getSubExpr(1)->clone(l);

			auto w = new WhileLoop(l, c, b);

			return w;
		}
		else
		{
			auto b1 = getSubExpr(0)->clone(l);
			auto b2 = getSubExpr(1)->clone(l);
			auto b3 = getSubExpr(2)->clone(l);
			auto b4 = getSubExpr(3)->clone(l);

			auto w = new WhileLoop(l, b1, b2, b3, b4);
			return w;
		}
	}

	TypeInfo getTypeInfo() const override
	{
		jassertfalse;
		return {};
	}

	Compare* getCompareCondition();

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	const LoopType loopType;

	BaseScope* getScopeToUse(BaseScope* outerScope);

	Ptr getLoopChildStatement(ChildStatementType t);

	AsmJitLabel getJumpTargetForEnd(bool getContinue) override
	{
		return getContinue ? continueTarget : breakTarget;
	}


	AsmJitLabel continueTarget;
	AsmJitLabel breakTarget;

	ScopedPointer<RegisterScope> forScope;
};

struct Operations::Loop : public Expression,
	public Operations::ConditionalBranch,
	public Operations::ArrayStatementBase
{
	SET_EXPRESSION_ID(Loop);

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);
		auto newLoop = new Loop(l, iterator, c1, c2);
		return newLoop;
	}

	ArrayType getArrayType() const override { return loopTargetType; }

	Loop(Location l, const Symbol& it_, Expression::Ptr t, Statement::Ptr body) :
		Expression(l),
		iterator(it_)
	{
		addStatement(t);
		addStatement(body);

		jassert(dynamic_cast<StatementBlock*>(body.get()) != nullptr);
	};

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		static const StringArray loopTypes = { "Undefined", "Span", "Dyn", "CustomObject" };
		t.setProperty("LoopType", loopTypes[loopTargetType], nullptr);
		t.setProperty("LoadIterator", loadIterator, nullptr);
		t.setProperty("Iterator", iterator.toString(), nullptr);
		t.setProperty("ElementType", iterator.typeInfo.toStringWithoutAlias(), nullptr);
        t.setProperty("ElementSize", (int)iterator.typeInfo.getRequiredByteSizeNonZero(), nullptr);
        
        if(loopTargetType == ArrayType::Span)
        {
            t.setProperty("NumElements", numElements, nullptr);
        }
		if (loopTargetType == ArrayStatementBase::CustomObject)
		{
			t.setProperty("ObjectType", getSubExpr(0)->getTypeInfo().toStringWithoutAlias(), nullptr);
		}
        
		return t;
	}

	TypeInfo getTypeInfo() const override { return getSubExpr(0)->getTypeInfo(); }

	Expression::Ptr getTarget() { return getSubExpr(0); }

	StatementBlock* getLoopBlock()
	{
		return dynamic_cast<StatementBlock*>(getChildStatement(1).get());
	}

	bool tryToResolveType(BaseCompiler* compiler) override;

	bool evaluateIteratorStore();

	bool evaluateIteratorLoad();

	AsmJitLabel getJumpTargetForEnd(bool getContinue) override
	{
		return loopEmitter->getLoopPoint(getContinue);
	}

	void process(BaseCompiler* compiler, BaseScope* scope);

	RegPtr iteratorRegister;
	Symbol iterator;

	bool loadIterator = true;
	bool storeIterator = false;

	ArrayType loopTargetType;

	AsmJitLabel loopStart;
	AsmJitLabel loopEnd;

	FunctionData customBegin;
	FunctionData customSizeFunction;

	ScopedPointer<AsmCodeGenerator::LoopEmitterBase> loopEmitter;

    int numElements = -1;
    
	JUCE_DECLARE_WEAK_REFERENCEABLE(Loop);
};

struct Operations::ControlFlowStatement : public Expression,
										  public StatementWithControlFlowEffectBase
{


	ControlFlowStatement(Location l, bool isBreak_) :
		Expression(l),
		isBreak(isBreak_)
	{}

	SET_EXPRESSION_ID(ControlFlowStatement);

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("command", isBreak ? "break" : "continue", nullptr);

		return t;
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new ControlFlowStatement(l, isBreak);
	}

	TypeInfo getTypeInfo() const override { return {}; }

	void process(BaseCompiler* compiler, BaseScope* scope) override;;

	virtual ScopeStatementBase* findRoot() const override;

	bool shouldAddDestructor(ScopeStatementBase* b, const Symbol& id) const override
	{
		/** Return if 

		- b is child of loopblock or loopblock
		- (check definition after...)
		*/

		auto loopPath = findRoot()->getPath();
		auto thisP = b->getPath();

		return thisP == loopPath || loopPath.isParentOf(thisP);
	}

	WeakReference<ConditionalBranch> parentLoop;
	bool isBreak;
};


struct Operations::IfStatement : public Statement,
	public Operations::ConditionalBranch,
	public Operations::BranchingStatement
{
	SET_EXPRESSION_ID(IfStatement);

	IfStatement(Location loc, Expression::Ptr cond, Ptr trueBranch, Ptr falseBranch) :
		Statement(loc)
	{
		addStatement(cond.get());
		addStatement(trueBranch);

		if (falseBranch != nullptr)
			addStatement(falseBranch);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getChildStatement(0)->clone(l);
		auto c2 = getChildStatement(1)->clone(l);
		Statement::Ptr c3 = hasFalseBranch() ? getChildStatement(2)->clone(l) : nullptr;

		return new IfStatement(l, dynamic_cast<Expression*>(c1.get()), c2, c3);
	}

	AsmJitLabel getJumpTargetForEnd(bool getContinue) override
	{
		location.throwError("Can't jump to end of if");

		RETURN_DEBUG_ONLY({});
	}

	TypeInfo getTypeInfo() const override { return {}; }

	bool hasFalseBranch() const { return getNumChildStatements() > 2; }

	void process(BaseCompiler* compiler, BaseScope* scope) override;
};



}
}
