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


struct Operations::InlinedArgument : public Expression,
	public SymbolStatement
{
	SET_EXPRESSION_ID(InlinedArgument);

	InlinedArgument(Location l, int argIndex_, const Symbol& s_, Ptr target) :
		Expression(l),
		argIndex(argIndex_),
		s(s_)
	{
		addStatement(target);
	}

	Symbol getSymbol() const override
	{
		return s;
	}

	bool isConstExpr() const override
	{
		return getSubExpr(0)->isConstExpr();
	}

	VariableStorage getConstExprValue() const override
	{
		return getSubExpr(0)->getConstExprValue();
	}

	ValueTree toValueTree() const override
	{
		auto v = Expression::toValueTree();
		v.setProperty("Arg", argIndex, nullptr);
		v.setProperty("ParameterName", s.toString(), nullptr);
		//v.addChild(getSubExpr(0)->toValueTree(), -1, nullptr);

		return v;
	}

	Statement::Ptr clone(Location l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto n = new InlinedArgument(l, argIndex, s, c1);
		return n;
	}

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	int argIndex;
	Symbol s;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InlinedArgument);
};


struct Operations::Noop : public Expression
{
	SET_EXPRESSION_ID(Noop);

	Noop(Location l) :
		Expression(l)
	{}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new Noop(l);
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		processBaseWithoutChildren(compiler, scope);
	}

	TypeInfo getTypeInfo() const override { return {}; };

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Noop);
};



struct Operations::Immediate : public Expression
{
	SET_EXPRESSION_ID(Immediate);

	Immediate(Location loc, VariableStorage value) :
		Expression(loc),
		v(value)
	{};

	TypeInfo getTypeInfo() const override { return TypeInfo(v.getType(), true, false); }

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new Immediate(l, v);
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		t.setProperty("Value", var(Types::Helpers::getCppValueString(v)), nullptr);
		return t;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	VariableStorage v;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Immediate);
};



struct Operations::InlinedParameter : public Expression,
	public SymbolStatement
{
	SET_EXPRESSION_ID(InlinedParameter);

	InlinedParameter(Location l, const Symbol& s_, Ptr source_) :
		Expression(l),
		s(s_),
		source(source_)
	{}

	Statement::Ptr clone(Location l) const override;

	Symbol getSymbol() const override { return s; };

	ValueTree toValueTree() const override
	{
		auto v = Expression::toValueTree();
		v.setProperty("Symbol", s.toString(), nullptr);
		return v;
	}


	TypeInfo getTypeInfo() const override
	{
		return source->getTypeInfo();
	}

	bool isConstExpr() const override
	{
		return source->isConstExpr();
	}

	VariableStorage getConstExprValue() const override
	{
		return source->getConstExprValue();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	Symbol s;
	Ptr source;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InlinedParameter);
};

struct Operations::VariableReference : public Expression,
	public SymbolStatement
{
	SET_EXPRESSION_ID(VariableReference);

	VariableReference(Location l, const Symbol& id_) :
		Expression(l),
		id(id_)
	{
		jassert(id);
	};


	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new VariableReference(l, id);
	}


	Symbol getSymbol() const override { return id; };

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();


		t.setProperty("Symbol", id.toString(false), nullptr);
		return t;
	}

	juce::String toString(Statement::TextFormat f) const override
	{
		return id.id.toString();
	}

	/** This scans the tree and checks whether it's the last reference.

		It ignores the control flow, so when the variable is part of a true
		branch, it might return true if the variable is used in the false
		branch.
	*/
	bool isLastVariableReference() const;

	int getNumWriteAcesses();

	/** This flags all variables that are not referenced later as ready for
		reuse.

		The best place to call this is after a child statement was processed
		with the BaseCompiler::CodeGeneration pass.
		It makes sure that if the register is used by a parent expression that
		it will not be flagged for reuse (eg. when used as target register of
		a binary operation).
	*/
	static void reuseAllLastReferences(Statement::Ptr parentStatement)
	{
#if REMOVE_REUSABLE_REG
		ReferenceCountedArray<AssemblyRegister> parentRegisters;

		auto pExpr = dynamic_cast<Expression*>(parentStatement.get());

		while (pExpr != nullptr)
		{
			if (pExpr->reg != nullptr)
				parentRegisters.add(pExpr->reg);

			pExpr = dynamic_cast<Expression*>(pExpr->parent.get());
		}


		SyntaxTreeWalker w(parentStatement, false);

		while (auto v = w.getNextStatementOfType<VariableReference>())
		{
			if (parentRegisters.contains(v->reg))
				continue;

			if (v->isLastVariableReference())
			{
				if (v->parameterIndex != -1)
					continue;

				v->reg->flagForReuse();
			}
		}
#endif
	}

	bool tryToResolveType(BaseCompiler* c) override
	{
		if (id.resolved)
			return true;

		auto newType = c->namespaceHandler.getVariableType(id.id);

		if (!newType.isDynamic())
			id = { id.id, newType };

		return id.resolved;
	}

	bool isConstExpr() const override
	{
		return !id.constExprValue.isVoid();
	}

	TypeInfo getTypeInfo() const override
	{
		return id.typeInfo;
	}

	FunctionClass* getFunctionClassForParentSymbol(BaseScope* scope) const
	{
		if (id.id.getParent().isValid())
		{
			return scope->getRootData()->getSubFunctionClass(id.id.getParent());
		}

		return nullptr;
	}

	VariableStorage getConstExprValue() const override
	{
		return id.constExprValue;
	}

	bool isReferencedOnce() const;

	bool isParameter(BaseScope* scope) const
	{
		if (auto fScope = dynamic_cast<FunctionScope*>(scope->getScopeForSymbol(id.id)))
			return fScope->parameters.contains(id.getName());

		return false;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	bool isBeingWritten()
	{
		return getWriteAccessType() != JitTokens::void_;
	}

	bool isInlinedParameter() const
	{
		return inlinedParameterExpression != nullptr;
	}

	TokenType getWriteAccessType();

	bool isClassVariable(BaseScope* scope) const
	{
		return scope->getRootClassScope()->rootData->contains(id.id);
	}

	bool isFirstReference();

	bool validateLocalDefinition(BaseCompiler* compiler, BaseScope* scope);

	int parameterIndex = -1;

	Ptr inlinedParameterExpression;


	Symbol id;
	WeakReference<BaseScope> variableScope;
	bool isFirstOccurence = false;
	bool isLocalDefinition = false;
	
	// This will be set to true for variable references
	// within conditional branches to avoid uninitialised values
	bool forceLoadData = false;

	// can be either the data pointer or the member offset
	VariableStorage objectAdress;
	ComplexType::WeakPtr objectPtr;

	// Contains the expression that leads to the pointer of the member object
	Ptr objectExpression;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariableReference);


};


struct Operations::ThisPointer : public Statement
{
	SET_EXPRESSION_ID(ThisPointer);

	ThisPointer(Location l, TypeInfo t) :
		Statement(l),
		type(t.getComplexType().get())
	{

	}

	Statement::Ptr clone(Location l) const
	{
		return new ThisPointer(l, getTypeInfo());
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(type.get());
	}

	ValueTree toValueTree() const override
	{
		auto v = Expression::toValueTree();
		v.setProperty("Type", getTypeInfo().toString(), nullptr);
		return v;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	ComplexType::WeakPtr type;
};

struct Operations::MemoryReference : public Expression
{
	SET_EXPRESSION_ID(MemoryReference);

	MemoryReference(Location l, Ptr base, const TypeInfo& type_, int offsetInBytes_) :
		Expression(l),
		offsetInBytes(offsetInBytes_),
		type(type_)
	{

		if (auto st = base->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			//jassert(st->hasMemberAtOffset(offsetInBytes, type));
		}

		addStatement(base);
	}

	Statement::Ptr clone(Location l) const
	{
		Statement::Ptr p = getSubExpr(0)->clone(l);
		return new MemoryReference(l, dynamic_cast<Expression*>(p.get()), type, offsetInBytes);
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	ValueTree toValueTree() const override
	{
		auto v = Expression::toValueTree();
		v.setProperty("ObjectType", getSubExpr(0)->getTypeInfo().toString(), nullptr);
		v.setProperty("MemberType", type.toString(), nullptr);
		v.setProperty("Offset", offsetInBytes, nullptr);
		return v;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	int offsetInBytes = 0;
	TypeInfo type;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryReference);
};


}
}