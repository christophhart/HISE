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



struct Operations::BinaryOp : public Expression
{
	SET_EXPRESSION_ID(BinaryOp);

	BinaryOp(Location l, Expression::Ptr left, Expression::Ptr right, TokenType opType) :
		Expression(l),
		op(opType)
	{
		addStatement(left);
		addStatement(right);
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);

		return new BinaryOp(l, c1, c2, op);
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		t.setProperty("OpType", op, nullptr);
		return t;
	}

	bool isLogicOp() const { return op == JitTokens::logicalOr || op == JitTokens::logicalAnd; }

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

#if REMOVE_REUSABLE_REG
	bool usesTempRegister = false;
#endif

	TokenType op;
};

/** TODO:
	- support span
	- add more functions (Math.max, clip, etc)
	- support size runtime check)
*/
struct Operations::VectorOp : public Expression
{
	VectorOp(Location l, Ptr target, TokenType opType_, Ptr opPtr) :
		Expression(l),
		opType(opType_)
	{
		if (!BlockParser::isVectorOp(opType, target) && BlockParser::isVectorOp(opType, opPtr) && as<FunctionCall>(target) == nullptr)
			l.throwError("left operator must be vector");

		isSimd4 = SpanType::isSimdType(target->getTypeInfo());

		const String validOps = "*+-=";

		if (!validOps.containsChar(*opType))
		{
			String e;
			e << opType << ": illegal operation for vectors";
			throwError(e);
		}

		addStatement(opPtr);
		addStatement(target);
	}

	SET_EXPRESSION_ID(VectorOp);

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto cOp = getSubExpr(0)->clone(l);
		auto cTarget = getSubExpr(1)->clone(l);

		return new VectorOp(l, cTarget, opType, cOp);
	}

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(1)->getTypeInfo();
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		t.setProperty("OpType", opType, nullptr);
		t.setProperty("Scalar", !getSubExpr(0)->getTypeInfo().isComplexType(), nullptr);
		t.setProperty("TargetType", getSubExpr(1)->getTypeInfo().toStringWithoutAlias(), nullptr);
		
		if (auto spanType = dynamic_cast<SpanType*>(getSubExpr(1)->getTypeInfo().getComplexType().get()))
		{
			t.setProperty("NumElements", spanType->getNumElements(), nullptr);
		}

		return t;
	}

	void initChildOps();

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	static uint32 getFunctionSignatureId(const String& functionName, bool isSimd);

	struct SerialisedVectorOp;

	void emitVectorOp(BaseCompiler* compiler, BaseScope* scope);

	bool isSimd4 = false;
	bool isChildOp = false;
	TokenType opType;
};

struct Operations::UnaryOp : public Expression
{
	UnaryOp(Location l, Ptr expr) :
		Expression(l)
	{
		addStatement(expr);
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		processBaseWithChildren(compiler, scope);
	}
};



struct Operations::Increment : public UnaryOp
{
	SET_EXPRESSION_ID(Increment);

	Increment(Location l, Ptr expr, bool isPre_, bool isDecrement_) :
		UnaryOp(l, expr),
		isPreInc(isPre_),
		isDecrement(isDecrement_)
	{
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);

		return new Increment(l, c1, isPreInc, isDecrement);
	}

	TypeInfo getTypeInfo() const override
	{
		return resolvedType;
	}

	FunctionClass::SpecialSymbols getOperatorId() const;

	bool tryToResolveType(BaseCompiler* c) override;

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("IsPre", isPreInc, nullptr);
		t.setProperty("IsDec", isDecrement, nullptr);

		return t;
	}

	bool hasSideEffect() const override
	{
		return true;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	static void getOrSetIncProperties(Array<TemplateParameter>& tp, bool& isPre, bool& isDec);

	bool isDecrement;
	bool isPreInc;
	bool removed = false;

	TypeInfo resolvedType = Types::ID::Void;
};


struct Operations::Cast : public Expression
{
	SET_EXPRESSION_ID(Cast);

	Cast(Location l, Expression::Ptr expression, Types::ID targetType_) :
		Expression(l)
	{
		addStatement(expression);
		targetType = TypeInfo(targetType_);
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto cc = getSubExpr(0)->clone(l);
		return new Cast(l, cc, targetType.getType());
	}

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		auto t = Expression::toValueTree();
		t.setProperty("Source", Types::Helpers::getTypeName(sourceType), nullptr);
		t.setProperty("Target", Types::Helpers::getTypeName(targetType), nullptr);
		return t;
	}

	// Make sure the target type is used.
	TypeInfo getTypeInfo() const override { return targetType; }

	void process(BaseCompiler* compiler, BaseScope* scope);

	FunctionData complexCastFunction;
	TypeInfo targetType;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Cast);
};

struct Operations::DotOperator : public Expression
{
	DotOperator(Location l, Ptr parent, Ptr child) :
		Expression(l)
	{
		addStatement(parent);
		addStatement(child);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto cp = getSubExpr(0)->clone(l);
		auto cc = getSubExpr(1)->clone(l);
		return new DotOperator(l, cp, cc);
	}

	ValueTree toValueTree() const override
	{
		auto v = Expression::toValueTree();

		v.setProperty("ObjectType", getDotParent()->getTypeInfo().toStringWithoutAlias(), nullptr);
		v.setProperty("MemberType", getTypeInfo().toStringWithoutAlias(), nullptr);

		return v;
	}

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Dot"); }

	Ptr getDotParent() const
	{
		return getSubExpr(0);
	}

	Ptr getDotChild() const
	{
		return getSubExpr(1);
	}

	bool tryToResolveType(BaseCompiler* compiler) override;

	TypeInfo getTypeInfo() const override
	{
		if (resolvedType.isValid())
			return resolvedType;

		return getDotChild()->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	TypeInfo resolvedType;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotOperator);
};



struct Operations::Subscript : public Expression,
	public ArrayStatementBase
{
	SET_EXPRESSION_ID(Subscript);

	Subscript(Location l, Ptr expr, Ptr index) :
		Expression(l)
	{
		addStatement(expr);
		addStatement(index);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);

		auto newSubscript = new Subscript(l, dynamic_cast<Expression*>(c1.get()), dynamic_cast<Expression*>(c2.get()));
		newSubscript->elementType = elementType;
		newSubscript->isWriteAccess = isWriteAccess;

		return newSubscript;
	}

	TypeInfo getTypeInfo() const override
	{
		return elementType;
	}

	ArrayType getArrayType() const override
	{
		return subscriptType;
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("Write", isWriteAccess, nullptr);
		t.setProperty("ElementType", elementType.toStringWithoutAlias(), nullptr);
		t.setProperty("ElementSize", (int)elementType.getRequiredByteSize(), nullptr);
		t.setProperty("ParentType", getSubExpr(0)->getTypeInfo().toStringWithoutAlias(), nullptr);

		return t;
	}

	bool tryToResolveType(BaseCompiler* compiler) override;

	void process(BaseCompiler* compiler, BaseScope* scope);

	ArrayType subscriptType = Undefined;
	bool isWriteAccess = false;
	SpanType* spanType = nullptr;
	DynType* dynType = nullptr;
	TypeInfo elementType;
	FunctionData subscriptOperator;
};


struct Operations::Assignment : public Expression,
	public TypeDefinitionBase
{
	enum class TargetType
	{
		Variable,
		Reference,
		Span,
		ClassMember,
		numTargetTypes
	};

	Assignment(Location l, Expression::Ptr target, TokenType assignmentType_, Expression::Ptr expr, bool firstAssignment_);

	~Assignment()
	{

	};

	Array<NamespacedIdentifier> getInstanceIds() const override { return { getTargetSymbolStatement()->getSymbol().id }; };

	TypeInfo getTypeInfo() const override { return getSubExpr(1)->getTypeInfo(); }

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Assignment"); }

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto ce = getSubExpr(0)->clone(l);
		auto ct = getSubExpr(1)->clone(l);
		return new Assignment(l, ct, assignmentType, ce, isFirstAssignment);
	}

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		ignoreUnused(sourceType, targetType);

		auto t = Expression::toValueTree();
		t.setProperty("First", isFirstAssignment, nullptr);
		t.setProperty("AssignmentType", assignmentType, nullptr);

        auto targetTypeInfo = getSubExpr(1)->getTypeInfo();
        
        
        if(targetTypeInfo.isComplexType() && !targetTypeInfo.isRef())
        {
            t.setProperty("NumBytesToCopy", targetTypeInfo.getRequiredByteSizeNonZero(), nullptr);
        }
        
		return t;
	}

	bool hasSideEffect() const override
	{
		return true;
	}

	TargetType getTargetType() const;

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const override
	{

		if (scope->getScopeType() == BaseScope::Class && isFirstAssignment)
		{
			jassert(getSubExpr(0)->isConstExpr());
			return Types::Helpers::getSizeForType(getSubExpr(0)->getType());
		}

		return 0;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	SymbolStatement* getTargetSymbolStatement() const
	{
		return as<SymbolStatement>(getTargetVariable());
	}

	Ptr getTargetVariable() const
	{
		auto targetType = getTargetType();
		jassert(targetType == TargetType::Variable || targetType == TargetType::Reference);
		ignoreUnused(targetType);
		auto s = getSubExpr(1);

		if (as<Cast>(s) != nullptr)
			s = s->getSubExpr(0);

		return s;
	}

	bool loadDataBeforeAssignment() const
	{
		if (assignmentType != JitTokens::assign_)
			return true;

		if (overloadedAssignOperator.isResolved())
			return true;

		return false;
	}

	DotOperator* getMemberTarget() const
	{
		jassert(getTargetType() == TargetType::ClassMember);
		return dynamic_cast<DotOperator*>(getSubExpr(1).get());
	}

	void initClassMembers(BaseCompiler* compiler, BaseScope* scope);

	TokenType assignmentType;
	const bool isFirstAssignment = false;
	FunctionData overloadedAssignOperator;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Assignment);
};


struct Operations::Compare : public Expression
{
	Compare(Location location, Expression::Ptr l, Expression::Ptr r, TokenType op_) :
		Expression(location),
		op(op_)
	{
		addStatement(l);
		addStatement(r);
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);
		return new Compare(l, c1, c2, op);
	}

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Comparison"); }

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		ignoreUnused(sourceType, targetType);

		auto t = Expression::toValueTree();
		t.setProperty("OpType", op, nullptr);

		return t;
	}

	TypeInfo getTypeInfo() const override { return TypeInfo(Types::Integer); }

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	TokenType op;
	bool useAsmFlag = false;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compare);
};

struct Operations::LogicalNot : public Expression
{
	SET_EXPRESSION_ID(LogicalNot);

	LogicalNot(Location l, Ptr expr) :
		Expression(l)
	{
		addStatement(expr);
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		return new LogicalNot(l, dynamic_cast<Expression*>(c1.get()));
	}

	TypeInfo getTypeInfo() const override { return TypeInfo(Types::ID::Integer); }

	void process(BaseCompiler* compiler, BaseScope* scope);

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogicalNot);
};


struct Operations::PointerAccess : public Expression
{
	SET_EXPRESSION_ID(PointerAccess);

	ValueTree toValueTree() const
	{
		return Statement::toValueTree();
	}

	Ptr clone(Location l) const override
	{
		return new PointerAccess(l, getSubExpr(0)->clone(l));
	}

	TypeInfo getTypeInfo() const
	{
		return getSubExpr(0)->getTypeInfo();
	}

	PointerAccess(Location l, Ptr target) :
		Statement(l)
	{
		addStatement(target);
	}

	void process(BaseCompiler* compiler, BaseScope* s);
};


struct Operations::Negation : public Expression
{
	SET_EXPRESSION_ID(Negation);

	Negation(Location l, Expression::Ptr e) :
		Expression(l)
	{
		addStatement(e);
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);

		return new Negation(l, dynamic_cast<Expression*>(c1.get()));
	}

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope);
};



}
}
