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

struct Operations::InlinedArgument : public Expression,
									 public SymbolStatement
{
	SET_EXPRESSION_ID(InlinedArgument);

	InlinedArgument(Location l, int argIndex_, const Symbol& s_, Ptr target):
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
		auto n = new InlinedArgument(l, argIndex, s, dynamic_cast<Expression*>(c1.get()));
		return n;
	}

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		jassert(scope->getScopeType() == BaseScope::Anonymous);

		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (s.typeInfo.isComplexType() && !s.isReference())
			{
				auto acg = CREATE_ASM_COMPILER(getTypeInfo().getType());
				
				auto stackPtr = acg.cc.newStack(s.typeInfo.getRequiredByteSize(), s.typeInfo.getRequiredAlignment());

				auto target = compiler->getRegFromPool(scope, s.typeInfo);

				target->setCustomMemoryLocation(stackPtr, false);

				auto source = getSubRegister(0);

				acg.emitComplexTypeCopy(target, source, s.typeInfo.getComplexType());

				getSubExpr(0)->reg = target;
			}
		}
	}

	int argIndex;
	Symbol s;
};


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

	static bool isRealStatement(Statement* s);

	bool isConstExpr() const override
	{
		int numStatements = 0;

		for (auto s : *this)
		{
			if (!s->isConstExpr())
				return false;
		}

		return true;
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

	static InlinedArgument* findInlinedParameterInParentBlocks(Statement* p, const Symbol& s)
	{
		if (p == nullptr)
			return nullptr;

		if (auto parentInlineArgument = findParentStatementOfType<InlinedArgument>(p))
		{
			auto parentBlock = findParentStatementOfType<StatementBlock>(parentInlineArgument);

			return findInlinedParameterInParentBlocks(parentBlock->parent, s);
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

				return nullptr;
			}
		}

		p = p->parent.get();

		if (p != nullptr)
			return findInlinedParameterInParentBlocks(p, s);

		return nullptr;
	}

	BaseScope* createOrGetBlockScope(BaseScope* parent)
	{
		if (parent->getScopeType() == BaseScope::Class)
			return parent;

		if (blockScope == nullptr)
			blockScope = new RegisterScope(parent, getPath());

		return blockScope;
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		auto bs = createOrGetBlockScope(scope);

		Expression::process(compiler, bs);

		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			if (hasReturnType())
			{
				if (!isInlinedFunction)
				{
					allocateReturnRegister(compiler, bs);
				}
			}

			reg = returnRegister;
		}
	}

	ScopedPointer<RegisterScope> blockScope;
	bool isInlinedFunction = false;
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
		Statement::process(compiler, scope);
	}

	TypeInfo getTypeInfo() const override { return {}; };
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

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			// We don't need to use the target register from the 
			// assignment for immediates
			reg = nullptr;

			reg = compiler->getRegFromPool(scope, getTypeInfo());
			reg->setDataPointer(v.getDataPointer());

			reg->createMemoryLocation(getFunctionCompiler(compiler));
		}
	}

	VariableStorage v;
};



struct Operations::InlinedParameter : public Expression,
									  public SymbolStatement
{
	SET_EXPRESSION_ID(InlinedParameter);

	InlinedParameter(Location l, const Symbol& s_, Ptr source_):
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

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			reg = source->reg;
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (source->currentPass != BaseCompiler::CodeGeneration)
			{
				source->process(compiler, scope);
			}



			if (reg == nullptr)
				reg = source->reg;

			
			
			jassert(reg != nullptr);
		}
	}

	Symbol s;
	Ptr source;
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
		auto t = Expression::toValueTree();
		t.setProperty("Symbol", id.toString(), nullptr);
		return t;
	}

	/** This scans the tree and checks whether it's the last reference.
	
		It ignores the control flow, so when the variable is part of a true
		branch, it might return true if the variable is used in the false
		branch.
	*/
	bool isLastVariableReference() const
	{
		SyntaxTreeWalker walker(this);

		auto lastOne = walker.getNextStatementOfType<VariableReference>();;

		bool isLast = lastOne == this;

		while (lastOne != nullptr)
		{
            auto isOtherVariable = lastOne->id != id;
            
            lastOne = walker.getNextStatementOfType<VariableReference>();
            
            if(isOtherVariable)
                continue;
            
            isLast = lastOne == this;
		}

		return isLast;
	}

	int getNumWriteAcesses()
	{
		int numWriteAccesses = 0;

		SyntaxTreeWalker walker(this);

		while (auto v = walker.getNextStatementOfType<VariableReference>())
		{
			if (v->id == id && v->isBeingWritten())
				numWriteAccesses++;
		}

		return numWriteAccesses;
	}

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

	bool isReferencedOnce() const
	{
		SyntaxTreeWalker w(this);

		int numReferences = 0;

		if (auto v = w.getNextStatementOfType<VariableReference>())
		{
			if (v->id == id)
				numReferences++;
		}

		return numReferences == 1;
	}

	bool isParameter(BaseScope* scope) const
	{
		if (auto fScope = dynamic_cast<FunctionScope*>(scope->getScopeForSymbol(id.id)))
		{
			return fScope->parameters.contains(id.getName());
		}

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

    bool isFirstReference()
	{
		SyntaxTreeWalker walker(this);

		while (auto v = walker.getNextStatementOfType<VariableReference>())
		{
			if (v->id == id && v->variableScope == variableScope)
				return v == this;
		}

		jassertfalse;
		return true;
	}

	bool validateLocalDefinition(BaseCompiler* compiler, BaseScope* scope)
	{
		jassert(isLocalDefinition);
		
		if (auto es = scope->getScopeForSymbol(id.id))
		{
			bool isAlreadyDefinedSubClassMember = false;

			if (auto cs = dynamic_cast<ClassScope*>(es))
			{
				isAlreadyDefinedSubClassMember = cs->typePtr != nullptr;
			}

			juce::String w;
			w << "declaration of " << id.toString() << " hides ";

			switch (es->getScopeType())
			{
			case BaseScope::Class:  w << "class member"; break;
			case BaseScope::Global: w << "global variable"; break;
			default:			    w << "previous declaration"; break;
			}

			if (!isAlreadyDefinedSubClassMember)
				logWarning(w);
		}

		// The type must have been set or it is a undefined variable
		if (getType() == Types::ID::Dynamic)
			location.throwError("Use of undefined variable " + id.toString());
        
        return true;
	}

	int parameterIndex = -1;

	Ptr inlinedParameterExpression;


	Symbol id;
	WeakReference<BaseScope> variableScope;
	bool isFirstOccurence = false;
	bool isLocalDefinition = false;

	// can be either the data pointer or the member offset
	VariableStorage objectAdress;
	ComplexType::WeakPtr objectPtr;

	// Contains the expression that leads to the pointer of the member object
	Ptr objectExpression;
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
		return new Cast(l, dynamic_cast<Expression*>(cc.get()), targetType.getType());
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
};

struct Operations::DotOperator : public Expression
{
	DotOperator(Location l, Ptr parent, Ptr child):
		Expression(l)
	{
		addStatement(parent);
		addStatement(child);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto cp = getSubExpr(0)->clone(l);
		auto cc = getSubExpr(1)->clone(l);
		return new DotOperator(l, dynamic_cast<Expression*>(cp.get()), dynamic_cast<Expression*>(cc.get()));
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

	TypeInfo getTypeInfo() const override
	{
		return getDotChild()->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;
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

	Array<NamespacedIdentifier> getInstanceIds() const override { return { getTargetVariable()->id.id }; };

	TypeInfo getTypeInfo() const override { return getSubExpr(1)->getTypeInfo(); }

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Assignment"); }

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto ce = getSubExpr(0)->clone(l);
		auto ct = getSubExpr(1)->clone(l);
		return new Assignment(l, dynamic_cast<Expression*>(ct.get()), assignmentType, dynamic_cast<Expression*>(ce.get()), isFirstAssignment);
	}

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		auto t = Expression::toValueTree();
		t.setProperty("First", isFirstAssignment, nullptr);
		t.setProperty("AssignmentType", assignmentType, nullptr);
		
		return t;
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

	VariableReference* getTargetVariable() const
	{
		auto targetType = getTargetType();
		jassert(targetType == TargetType::Variable || targetType == TargetType::Reference);
		auto v = getSubExpr(1).get();
		return dynamic_cast<VariableReference*>(v);
	}

	DotOperator* getMemberTarget() const
	{
		jassert(getTargetType() == TargetType::ClassMember);
		return dynamic_cast<DotOperator*>(getSubExpr(1).get());
	}

	void initClassMembers(BaseCompiler* compiler, BaseScope* scope);

	TokenType assignmentType;
	bool isFirstAssignment = false;
	FunctionData overloadedAssignOperator;
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
		return new Compare(l, dynamic_cast<Expression*>(c1.get()), dynamic_cast<Expression*>(c2.get()), op);
	}

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Comparison"); }

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		auto t = Expression::toValueTree();
		t.setProperty("OpType", op, nullptr);

		return t;
	}

	TypeInfo getTypeInfo() const override { return TypeInfo(Types::Integer); }

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			auto l = getSubExpr(0);
			auto r = getSubExpr(1);

			if (l->getType() != r->getType())
			{
				Ptr implicitCast = new Operations::Cast(location, getSubExpr(1), l->getType());
				logWarning("Implicit cast to int for comparison");
				replaceChildStatement(1, implicitCast);
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			{
				auto asg = CREATE_ASM_COMPILER(getType());

				auto l = getSubExpr(0);
				auto r = getSubExpr(1);

				reg = compiler->getRegFromPool(scope, getTypeInfo());

				auto tReg = getSubRegister(0);
				auto value = getSubRegister(1);

				asg.emitCompare(op, reg, l->reg, r->reg);

				VariableReference::reuseAllLastReferences(this);

				l->reg->flagForReuseIfAnonymous();
				r->reg->flagForReuseIfAnonymous();
			}
		}
	}

	TokenType op;

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

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (getSubExpr(0)->getType() != Types::ID::Integer)
				throwError("Wrong type for logic operation");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			reg = asg.emitLogicalNot(getSubRegister(0));
		}
	}
};

struct Operations::TernaryOp : public Expression,
							   public BranchingStatement
{
public:

	SET_EXPRESSION_ID(TernaryOp);

	TernaryOp(Location l, Expression::Ptr c, Expression::Ptr t, Expression::Ptr f) :
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
		return new TernaryOp(l, dynamic_cast<Expression*>(c1.get()), dynamic_cast<Expression*>(c2.get()), dynamic_cast<Expression*>(c3.get()));
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		// We need to have precise control over the code generation
		// for the subexpressions to avoid execution of both branches
		if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration)
			Statement::process(compiler, scope);
		else
			Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			type = checkAndSetType(1, type);
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());
			reg = asg.emitTernaryOp(this, compiler, scope);
			jassert(reg->isActive());
		}
	}

private:

	TypeInfo type;

};

struct Operations::CastedSimd : public Expression
{
	SET_EXPRESSION_ID(CastedSimd);

	CastedSimd(Location l, Ptr original) :
		Expression(l)
	{
		addStatement(original);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		jassertfalse;
		return nullptr;
	}

	TypeInfo getTypeInfo() const override
	{
		return simdSpanType;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (auto ost = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				int numSimd = ost->getNumElements() / 4;

				auto float4Type = compiler->namespaceHandler.getComplexType(NamespacedIdentifier("float4"));

				jassert(float4Type != nullptr);
				auto s = new SpanType(TypeInfo(float4Type), numSimd);
				simdSpanType = TypeInfo(compiler->namespaceHandler.registerComplexTypeOrReturnExisting(s));
			}
			else
				throwError("Can't convert non-span to SIMD span");
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (auto ost = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				if (ost->getElementType() != Types::ID::Float)
					throwError("Can't convert non-float to SIMD");

				if (ost->getNumElements() % 4 != 0)
					throwError("Span needs to be multiple of 4 to be convertible to SIMD");
			}
			else
				throwError("Can't convert non-span to SIMD span");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			reg = getSubRegister(0);
		}
	}

	TypeInfo simdSpanType;
};

struct Operations::FunctionCall : public Expression
{
	SET_EXPRESSION_ID(FunctionCall);

	enum CallType
	{
		Unresolved,
		MemberFunction,
		ExternalObjectFunction,
		RootFunction,
		GlobalFunction,
		ApiFunction,
		NativeTypeCall, // either block or event
		numCallTypes
	};



	FunctionCall(Location l, Ptr f, const Symbol& id, const Array<TemplateParameter>& tp) :
		Expression(l)
	{
		function.id = id.id;
		function.returnType = id.typeInfo;
		function.templateParameters = tp;

		if (auto dp = dynamic_cast<DotOperator*>(f.get()))
		{
			setObjectExpression(dp->getDotParent());
		}
	};

	void setObjectExpression(Ptr e)
	{
		objExpr = e;
		addStatement(e.get(), true);
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto newFC = new FunctionCall(l, nullptr, Symbol(function.id, function.returnType), function.templateParameters);

		if (objExpr != nullptr)
		{
			auto clonedObject = objExpr->clone(l);
			newFC->setObjectExpression(dynamic_cast<Expression*>(clonedObject.get()));
		}

		return newFC;
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("Signature", function.getSignature(), nullptr);

		const StringArray resolveNames = { "Unresolved", "MemberFunction", "ExternalObjectFunction", "RootFunction", "GlobalFunction", "ApiFunction", "NativeTypeCall" };

		t.setProperty("CallType", resolveNames[(int)callType], nullptr);

		return t;
	}

	void addArgument(Ptr arg)
	{
		addStatement(arg);
	}

	Expression* getArgument(int index)
	{

		return getSubExpr(objExpr == nullptr ? index : (index + 1));
	}

	int getNumArguments() const
	{
		if (objExpr == nullptr)
			return getNumChildStatements();
		else
			return getNumChildStatements() - 1;
	}

	bool shouldInlineFunctionCall(BaseCompiler* compiler, BaseScope* scope) const;

	static bool canBeAliasParameter(Ptr e)
	{
		return dynamic_cast<VariableReference*>(e.get()) != nullptr;
	}

	void inlineFunctionCall(AsmCodeGenerator& acg);

	TypeInfo getTypeInfo() const override;

	void process(BaseCompiler* compiler, BaseScope* scope);

	CallType callType = Unresolved;
	Array<FunctionData> possibleMatches;
	mutable FunctionData function;
	WeakReference<FunctionClass> fc;
	FunctionClass::Ptr ownedFc;
	
	Ptr objExpr;

	ReferenceCountedArray<AssemblyRegister> parameterRegs;
};

struct Operations::MemoryReference : public Expression
{
	SET_EXPRESSION_ID(MemoryReference);

	MemoryReference(Location l, Ptr base, const TypeInfo& type_, int offsetInBytes_):
		Expression(l),
		offsetInBytes(offsetInBytes_),
		type(type_)
	{
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
		v.setProperty("Offset", offsetInBytes, nullptr);
		return v;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			reg = compiler->registerPool.getNextFreeRegister(scope, type);
			
			auto baseReg = getSubRegister(0);

			X86Mem ptr;

			if (baseReg->isMemoryLocation())
				ptr = baseReg->getAsMemoryLocation().cloneAdjustedAndResized(offsetInBytes, 8);
			else if (baseReg->isGlobalVariableRegister())
				ptr = x86::qword_ptr(reinterpret_cast<uint64_t>(baseReg->getGlobalDataPointer()) + offsetInBytes);
			else
				ptr = x86::ptr(PTR_REG_W(baseReg)).cloneAdjustedAndResized(offsetInBytes, 8);

			reg->setCustomMemoryLocation(ptr, true);

			reg = compiler->registerPool.getRegisterWithMemory(reg);
		}
	}

	int offsetInBytes = 0;
	TypeInfo type;
};

struct Operations::ReturnStatement : public Expression
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
		return new ReturnStatement(l, dynamic_cast<Expression*>(p.get()));
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

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (auto fScope = dynamic_cast<FunctionScope*>(findFunctionScope(scope)))
			{
				TypeInfo actualType(Types::ID::Void);

				if (auto first = getSubExpr(0))
					actualType = first->getTypeInfo();

				if (isVoid() && actualType != Types::ID::Void)
					throwError("function must return a value");
				if (!isVoid() && actualType == Types::ID::Void)
					throwError("Can't return a value from a void function.");

				checkAndSetType(0, getTypeInfo());
			}
			else
				throwError("Can't deduce return type.");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			bool isFunctionReturn = true;

			if (!isVoid())
			{
				if (auto sb = findInlinedRoot())
				{
					reg = getSubRegister(0);
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

				asg.emitReturn(compiler, reg, sourceReg);
			}
		}
	}

	ScopeStatementBase* findRoot() const
	{
		return ScopeStatementBase::getStatementListWithReturnType(const_cast<ReturnStatement*>(this));
	}

	StatementBlock* findInlinedRoot() const
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
	};

};


struct Operations::ClassStatement : public Statement
{
	SET_EXPRESSION_ID(ClassStatement)

	using Ptr = ReferenceCountedObjectPtr<ClassStatement>;

	ClassStatement(Location l, ComplexType::Ptr classType_, Statement::Ptr classBlock) :
		Statement(l),
		classType(classType_)
	{
		addStatement(classBlock);
	}

	~ClassStatement()
	{
		classType = nullptr;
		int x = 5;
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		jassertfalse;
		return nullptr;
	}

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();
		t.setProperty("Type", classType->toString(), nullptr);
		return t;
	}

	TypeInfo getTypeInfo() const override { return {}; }

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const override
	{
		jassert(compiler->getCurrentPass() > BaseCompiler::ComplexTypeParsing);
		return classType->getRequiredByteSize();
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		if (subClass == nullptr)
			subClass = new ClassScope(scope, getStructType()->id, classType);

		Statement::process(compiler, subClass);

		getChildStatement(0)->process(compiler, subClass);

		COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
		{
			for (auto s : *getChildStatement(0))
			{
				if (auto td = dynamic_cast<TypeDefinitionBase*>(s))
				{
					if (s->getTypeInfo().isInvalid())
						location.throwError("Can't use auto on member variables");

					for(auto& id: td->getInstanceIds())
						getStructType()->addMember(id.getIdentifier(), s->getTypeInfo());
				}
			}

			getStructType()->finaliseAlignment();
		}

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			getStructType()->finaliseAlignment();
		}
	}

	StructType* getStructType()
	{
		return dynamic_cast<StructType*>(classType.get());
	}

	ComplexType::Ptr classType;
	ScopedPointer<ClassScope> subClass;
};




struct Operations::Function : public Statement,
	public asmjit::ErrorHandler
{
	SET_EXPRESSION_ID(Function);

	Function(Location l, const Symbol& id_) :
		Statement(l),
		code(nullptr),
		id(id_)
	{};

	~Function()
	{
		data = {};
		functionScope = nullptr;
		statements = nullptr;
		parameters.clear();
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		jassertfalse;
		return nullptr;
	}

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();
		t.setProperty("Signature", data.getSignature(parameters), nullptr);

		t.addChild(statements->toValueTree(), -1, nullptr);

		return t;
	}

	void handleError(asmjit::Error, const char* message, asmjit::BaseEmitter* emitter) override
	{
		throwError(juce::String(message));
	}

	TypeInfo getTypeInfo() const override { return TypeInfo(data.returnType); }

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	juce::String::CharPointerType code;
	int codeLength = 0;


	FunctionClass::Ptr functionClass;

	ScopedPointer<FunctionScope> functionScope;
	Statement::Ptr statements;
	FunctionData data;
	
	Array<Identifier> parameters;
	Symbol id;
	RegPtr objectPtr;
	bool hasObjectPtr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Function);
};

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

		return new BinaryOp(l, dynamic_cast<Expression*>(c1.get()), dynamic_cast<Expression*>(c2.get()), op);
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		
		t.setProperty("OpType", op, nullptr);
		t.setProperty("UseTempRegister", usesTempRegister, nullptr);
		return t;
	}

	bool isLogicOp() const { return op == JitTokens::logicalOr || op == JitTokens::logicalAnd; }

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		// Defer evaluation of the children for operators with short circuiting...
		bool processChildren = !(isLogicOp() && (compiler->getCurrentPass() == BaseCompiler::CodeGeneration));

		if (processChildren)
			Expression::process(compiler, scope);
		else
			Statement::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (op == JitTokens::logicalAnd ||
				op == JitTokens::logicalOr)
			{
				checkAndSetType(0, TypeInfo(Types::ID::Integer));
			}
			else
				checkAndSetType(0, {});
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			if (isLogicOp())
			{
				asg.emitLogicOp(this);
			}
			else
			{
				auto l = getSubRegister(0);

				if (auto childOp = dynamic_cast<BinaryOp*>(getSubExpr(0).get()))
				{
					if (childOp->usesTempRegister)
						l->flagForReuse();
				}

				usesTempRegister = false;

				if (l->canBeReused())
				{
					reg = l;
					reg->removeReuseFlag();
					jassert(!reg->isMemoryLocation());
				}
				else
				{
					if (reg == nullptr)
					{
						asg.emitComment("temp register for binary op");
						reg = compiler->getRegFromPool(scope, getTypeInfo());
						usesTempRegister = true;
					}

					asg.emitStore(reg, getSubRegister(0));
				}

				auto le = getSubExpr(0);
				auto re = getSubExpr(1);

				asg.emitBinaryOp(op, reg, getSubRegister(1));

				VariableReference::reuseAllLastReferences(getChildStatement(0));
				VariableReference::reuseAllLastReferences(getChildStatement(1));
			}
		}
	}

	bool usesTempRegister = false;
	TokenType op;
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
		Expression::process(compiler, scope);
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

		return new Increment(l, dynamic_cast<Expression*>(c1.get()), isPreInc, isDecrement);
	}

	TypeInfo getTypeInfo() const override
	{
		return getSubExpr(0)->getTypeInfo();
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("IsPre", isPreInc, nullptr);
		t.setProperty("IsDec", isDecrement, nullptr);

		return t;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::SyntaxSugarReplacements)
		{
			if (removed)
				return;
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (dynamic_cast<Increment*>(getSubExpr(0).get()) != nullptr)
				throwError("Can't combine incrementors");

			if (getType() != Types::ID::Integer)
				throwError("Can't increment non integer variables.");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			if (isPreInc)
			{
				asg.emitIncrement(nullptr, getSubRegister(0), isPreInc, isDecrement);
				reg = getSubRegister(0);
			}
			else
			{
				reg = compiler->getRegFromPool(scope, TypeInfo(Types::ID::Integer));
				asg.emitIncrement(reg, getSubRegister(0), isPreInc, isDecrement);
			}

			if (auto wt = getTypeInfo().getTypedIfComplexType<WrapType>())
			{
				jassertfalse;
				//asg.emitWrap(wt, reg, isDecrement ? WrapType::OpType::Dec : WrapType::OpType::Inc);
			}
		}
	}

	bool isDecrement;
	bool isPreInc;
	bool removed = false;
};

struct Operations::Loop : public Expression,
							   public Operations::ConditionalBranch
{
	SET_EXPRESSION_ID(Loop);

	enum LoopTargetType
	{
		Undefined,
		Span,
		Dyn,
		Block,
		CustomObject,
		numLoopTargetTypes
	};

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto c1 = getSubExpr(0)->clone(l);
		auto c2 = getSubExpr(1)->clone(l);

		auto path = findParentStatementOfType<ScopeStatementBase>(this)->getPath();

		auto newLoop = new Loop(l, iterator, dynamic_cast<Expression*>(c1.get()), c2);
		return newLoop;
	}

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

		static const StringArray loopTypes = { "Undefined", "Span", "Block", "CustomObject" };
		t.setProperty("LoopType", loopTypes[loopTargetType], nullptr);
		t.setProperty("LoadIterator", loadIterator, nullptr);
		t.setProperty("Iterator", iterator.toString(), nullptr);
		
		return t;
	}

	TypeInfo getTypeInfo() const override { return {}; }

	Expression::Ptr getTarget()
	{
		return getSubExpr(0);
	}

	StatementBlock* getLoopBlock()
	{
		auto b = getChildStatement(1);

		return dynamic_cast<StatementBlock*>(b.get());
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Statement::process(compiler, scope);

		if (compiler->getCurrentPass() != BaseCompiler::DataAllocation &&
			compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
		{
			getTarget()->process(compiler, scope);
			getLoopBlock()->process(compiler, scope);
		}

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			getTarget()->process(compiler, scope);

#if 0
			auto b = getLoopBlock()->blockScope.get();
			
			if (b != nullptr)
			{
				jassert(b->localVariables.isEmpty());
				jassert(iterator.id.getParent() == b->getScopeSymbol());
			}
			else
			{
				getLoopBlock()->blockScope = new RegisterScope(scope, iterator.id.getParent());
			}
#endif

			jassert(iterator.id.getParent().getParent() == scope->getScopeSymbol());

			

			if (auto sp = getTarget()->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				loopTargetType = Span;

				if (iterator.typeInfo.isDynamic())
					iterator.typeInfo = sp->getElementType();
				else if (iterator.typeInfo != sp->getElementType())
					location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + sp->getElementType().toString());
			}
			else if (auto dt = getTarget()->getTypeInfo().getTypedIfComplexType<DynType>())
			{
				loopTargetType = Dyn;

				if (iterator.typeInfo.isDynamic())
					iterator.typeInfo = dt->elementType;
				else if (iterator.typeInfo != dt->elementType)
					location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + sp->getElementType().toString());
			}
			else if (getTarget()->getType() == Types::ID::Block)
			{
				loopTargetType = Block;

				if (iterator.typeInfo.isDynamic())
					iterator.typeInfo = TypeInfo(Types::ID::Float, iterator.isConst(), iterator.isReference());
				else if (iterator.typeInfo.getType() != Types::ID::Float)
					location.throwError("Illegal iterator type");
			}
			
			compiler->namespaceHandler.setTypeInfo(iterator.id, NamespaceHandler::Variable, iterator.typeInfo);
			
			getLoopBlock()->process(compiler, scope);

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
							});

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
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto acg = CREATE_ASM_COMPILER(iterator.typeInfo.getType());

			getTarget()->process(compiler, scope);

			auto r = getTarget()->reg;

			jassert(r != nullptr && r->getScope() != nullptr);

			allocateDirtyGlobalVariables(getLoopBlock(), compiler, scope);

			if (loopTargetType == Span)
			{
				auto le = new SpanLoopEmitter(iterator, getTarget()->reg, getLoopBlock(), loadIterator);
				le->typePtr = getTarget()->getTypeInfo().getTypedComplexType<SpanType>();
				loopEmitter = le;
			}
			else if (loopTargetType == Dyn)
			{
				auto le = new DynLoopEmitter(iterator, getTarget()->reg, getLoopBlock(), loadIterator);
				le->typePtr = getTarget()->getTypeInfo().getTypedComplexType<DynType>();
				loopEmitter = le;
			}
			else if (loopTargetType == Block)
			{
				auto le = new BlockLoopEmitter(iterator, getTarget()->reg, getLoopBlock(), loadIterator);
				loopEmitter = le;
			}

			if(loopEmitter != nullptr)
				loopEmitter->emitLoop(acg, compiler, scope);
		}
	}

	RegPtr iteratorRegister;
	Symbol iterator;
	
	bool loadIterator = true;

	LoopTargetType loopTargetType;

	asmjit::Label loopStart;
	asmjit::Label loopEnd;

	ScopedPointer<AsmCodeGenerator::LoopEmitterBase> loopEmitter;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Loop);
};

struct Operations::ControlFlowStatement : public Expression
{
	

	ControlFlowStatement(Location l, bool isBreak_):
		Expression(l),
		isBreak(isBreak_)
	{

	}

	Identifier getStatementId() const override
	{
		if (isBreak)
		{
			RETURN_STATIC_IDENTIFIER("break");
		}
		else
		{
			RETURN_STATIC_IDENTIFIER("continue");
		}
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		return new ControlFlowStatement(l, isBreak);
	}

	TypeInfo getTypeInfo() const override
	{
		return {};
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			parentLoop = findParentStatementOfType<Loop>(this);

			if (parentLoop == nullptr)
			{
				juce::String s;
				s << "a " << getStatementId().toString() << " may only be used within a loop or switch";
				throwError(s);
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto acg = CREATE_ASM_COMPILER(Types::ID::Integer);
			acg.emitLoopControlFlow(parentLoop, isBreak);
		}
	};

	WeakReference<Loop> parentLoop;
	bool isBreak;
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

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (!isConstExpr())
			{
				auto asg = CREATE_ASM_COMPILER(getType());

				reg = compiler->getRegFromPool(scope, getTypeInfo());

				asg.emitNegation(reg, getSubRegister(0));

				getSubRegister(0)->flagForReuseIfAnonymous();
			}
			else
			{
				// supposed to be optimized away by now...
				jassertfalse;
			}
		}
	}
};

struct Operations::IfStatement : public Statement,
								 public Operations::ConditionalBranch,
								 public Operations::BranchingStatement
{
	SET_EXPRESSION_ID(IfStatement);

	IfStatement(Location loc, Expression::Ptr cond, Ptr trueBranch, Ptr falseBranch):
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

	TypeInfo getTypeInfo() const override { return {}; }

	bool hasFalseBranch() const { return getNumChildStatements() > 2; }
	
	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Statement::process(compiler, scope);

		if (compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
			processAllChildren(compiler, scope);
		
		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			processAllChildren(compiler, scope);

			if (getCondition()->getTypeInfo() != Types::ID::Integer)
				throwError("Condition must be boolean expression");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto acg = CREATE_ASM_COMPILER(Types::ID::Integer);

			allocateDirtyGlobalVariables(getTrueBranch(), compiler, scope);

			if(hasFalseBranch())
				allocateDirtyGlobalVariables(getFalseBranch(), compiler, scope);
			
			auto cond = dynamic_cast<Expression*>(getCondition().get());
			auto trueBranch = getTrueBranch();
			auto falseBranch = getFalseBranch();

			acg.emitBranch(TypeInfo(Types::ID::Void), cond, trueBranch, falseBranch, compiler, scope);
		}
	}
};





struct Operations::Subscript : public Expression
{
	SET_EXPRESSION_ID(Subscript);

	enum SubscriptType
	{
		Undefined,
		Span,
		Block,
		CustomObject,
		numSubscriptTypes
	};

	Subscript(Location l, Ptr expr, Ptr index):
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

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		t.setProperty("Write", isWriteAccess, nullptr);
		t.setProperty("ElementType", elementType.toString(), nullptr);
		t.setProperty("ParentType", getSubExpr(0)->getTypeInfo().toString(), nullptr);

		return t;
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		processChildrenIfNotCodeGen(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			spanType = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<SpanType>();

			if (spanType != nullptr)
			{
				subscriptType = Span;
				elementType = spanType->getElementType();
			}

			if (getSubExpr(0)->getType() == Types::ID::Block)
			{
				subscriptType = Block;
				elementType = TypeInfo(Types::ID::Float, false, true);
			}
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			auto indexType = getSubExpr(1)->getTypeInfo();

			if (indexType.getType() != Types::ID::Integer && indexType.getTypedIfComplexType<WrapType>() == 0)
				throwError("index must be an integer value");
			
			if (spanType == nullptr)
			{
				if (getSubExpr(0)->getType() == Types::ID::Block)
					elementType = TypeInfo(Types::ID::Float, false, true);
				else
					location.throwError("Can't use []-operator");
			}
			else
			{
				auto size = spanType->getNumElements();

				if (getSubExpr(1)->isConstExpr())
				{
					int index = getSubExpr(1)->getConstExprValue().toInt();

					if (!isPositiveAndBelow(index, size))
						throwError("constant index out of bounds");
				}
				else
				{
					auto wt = indexType.getTypedIfComplexType<WrapType>();

					if (wt == nullptr)
						throwError("Can't use non-constant or non-wrapped index");

					if (!isPositiveAndBelow(wt->size-1, size))
						throwError("wrap limit exceeds span size");
				}
			}
		}

		
		if(isCodeGenPass(compiler))
		{
			auto abortFunction = [this]()
			{
				if (!getSubExpr(1)->isConstExpr())
					return false;

				if (dynamic_cast<InlinedParameter*>(getSubExpr(0).get()) != nullptr)
					return false;

				if (SpanType::isSimdType(getSubExpr(0)->getTypeInfo()))
					return false;

				if (subscriptType == Block)
					return false;

				return true;
			};

			if (!preprocessCodeGenForChildStatements(compiler, scope, abortFunction))
				return;

			reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());

			auto acg = CREATE_ASM_COMPILER(getType());

			auto cType = getSubRegister(0)->getTypeInfo().getTypedIfComplexType<ComplexType>();

			FunctionData subscriptOperator;

			if (cType != nullptr)
			{
				if(FunctionClass::Ptr fc = cType->getFunctionClass())
				{
					subscriptOperator = fc->getSpecialFunction(FunctionClass::Subscript, getTypeInfo(), { getSubRegister(1)->getTypeInfo() });
				}
			}

			if (subscriptOperator.isResolved())
			{
				AssemblyRegister::List l;
				l.add(getSubRegister(1));
				auto r = acg.emitFunctionCall(reg, subscriptOperator, getSubRegister(0), l);

				if (!r.wasOk())
					location.throwError(r.getErrorMessage());
			}

			RegPtr indexReg;

			if (auto wt = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<WrapType>())
			{
				auto fc = wt->getFunctionClass();
				auto fData = fc->getSpecialFunction(FunctionClass::NativeTypeCast, TypeInfo(Types::ID::Integer), {});

				indexReg = compiler->registerPool.getNextFreeRegister(scope, TypeInfo(Types::ID::Integer));

				AssemblyRegister::List l;
				acg.emitFunctionCall(indexReg, fData, getSubRegister(1), l);
			}
			else
				indexReg = getSubRegister(1);

			acg.emitSpanReference(reg, getSubRegister(0), indexReg, elementType.getRequiredByteSize());

			replaceMemoryWithExistingReference(compiler);
		}
	}

	SubscriptType subscriptType = Undefined;
	bool isWriteAccess = false;
	SpanType* spanType = nullptr;
	TypeInfo elementType;
};





struct Operations::ComplexTypeDefinition : public Expression,
											public TypeDefinitionBase
{
	SET_EXPRESSION_ID(ComplexTypeDefinition);

	ComplexTypeDefinition(Location l, const Array<NamespacedIdentifier>& ids_, TypeInfo type_) :
		Expression(l),
		ids(ids_),
		type(type_)
	{

	}

	Array<NamespacedIdentifier> getInstanceIds() const override { return ids; }

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto n = new ComplexTypeDefinition(l, ids, type);

		cloneChildren(n);

		if (initValues != nullptr)
			n->initValues = initValues;

		return n;
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();

		juce::String names;

		for (auto id : ids)
			names << id.toString() << ",";

		t.setProperty("Type", type.toString(), nullptr);

		t.setProperty("Ids", names, nullptr);
		
		if (initValues != nullptr)
			t.setProperty("InitValues", initValues->toString(), nullptr);

		return t;
	}

	Array<Symbol> getSymbols() const
	{
		Array<Symbol> symbols;

		for (auto id : ids)
		{
			symbols.add({ id, getTypeInfo() });
		}

		return symbols;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	bool isStackDefinition(BaseScope* scope) const
	{
		return dynamic_cast<RegisterScope*>(scope) != nullptr;
	}

	Array<NamespacedIdentifier> ids;
	TypeInfo type;

	InitialiserList::Ptr initValues;

	ReferenceCountedArray<AssemblyRegister> stackLocations;
};

#if 0 // REMOVE INLINE_MATH
struct Operations::InlinedExternalCall : public Expression
{
	SET_EXPRESSION_ID(InlinedExternalCall);

	InlinedExternalCall(FunctionCall* functionCallToBeReplaced):
		Expression(functionCallToBeReplaced->location),
		f(functionCallToBeReplaced->function)
	{
		for (int i = 0; i < functionCallToBeReplaced->getNumArguments(); i++)
		{
			addStatement(functionCallToBeReplaced->getArgument(i));
		}
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(f.returnType);
	}

	ValueTree toValueTree() const override
	{
		auto t = toValueTree();
		t.setProperty("Function", f.getSignature(), nullptr);
		return t;
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto acg = CREATE_ASM_COMPILER(getType());

			reg = compiler->registerPool.getNextFreeRegister(scope, getType());

			ReferenceCountedArray<AssemblyRegister> args;

			for (int i = 0; i < getNumChildStatements(); i++)
				args.add(getSubExpr(i)->reg);

			acg.emitInlinedMathAssembly(f.id, reg, args);
		}
	}

	FunctionData f;
};
#endif

}
}
