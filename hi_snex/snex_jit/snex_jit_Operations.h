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


struct Operations::StatementBlock : public Statement,
							        public ScopeStatementBase
{
	SET_EXPRESSION_ID(StatementBlock);

	StatementBlock(Location l) :
		Statement(l)
	{}

	TypeInfo getTypeInfo() const override { return {}; };

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Statement::process(compiler, scope);

		if (blockScope == nullptr)
			blockScope = new RegisterScope(scope, location.createAnonymousScopeId());

		for (int i = 0; i < getNumChildStatements(); i++)
			getChildStatement(i)->process(compiler, blockScope);
	}

	ScopedPointer<RegisterScope> blockScope;
};

struct Operations::Noop : public Expression
{
	SET_EXPRESSION_ID(Noop);

	Noop(Location l) :
		Expression(l)
	{}

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

			reg = compiler->getRegFromPool(scope, getType());
			reg->setDataPointer(v.getDataPointer());

			reg->createMemoryLocation(getFunctionCompiler(compiler));
		}
	}

	VariableStorage v;
};


struct Operations::VariableReference : public Expression
{
	SET_EXPRESSION_ID(VariableReference);

	VariableReference(Location l, const Symbol& id_) :
		Expression(l),
		id(id_)
	{
		jassert(id);
	};

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

	FunctionClass* getFunctionClassForSymbol(BaseScope* scope) const
	{
		if(auto gfc = scope->getGlobalScope()->getGlobalFunctionClass(id.id))
			return gfc;

		if (scope->getScopeType() == BaseScope::Global)
			return nullptr;

		return scope->getRootClassScope()->getSubFunctionClass(id);
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
		if (auto fScope = dynamic_cast<FunctionScope*>(scope->getScopeForSymbol(id)))
		{
			return fScope->parameters.contains(id.id);
		}

		return false;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	bool isBeingWritten()
	{
		return getWriteAccessType() != JitTokens::void_;
	}

	TokenType getWriteAccessType();

	bool isClassVariable(BaseScope* scope) const
	{
		return scope->getRootClassScope()->rootData->contains(id);
	}

	bool isApiClass(BaseScope* s) const;

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
		
		if (auto es = scope->getScopeForSymbol(id))
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

	Symbol id;
	WeakReference<BaseScope> variableScope;
	bool isFirstOccurence = false;
	bool isLocalDefinition = false;
	VariableStorage dataPointer;
	VariableStorage memberOffset;

	// Contains the expression that leads to the pointer of the member object
	Ptr objectPointer;
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

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			auto sourceType = getSubExpr(0)->getType();
			auto targetType = getType();

			if (sourceType == Types::ID::Block)
				throwError("Can't cast from block to other type");

			if (targetType == Types::ID::Block)
				throwError("Can't cast to block from other type");

			if (sourceType == targetType)
				replaceInParent(getSubExpr(0));
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());
			auto sourceType = getSubExpr(0)->getType();
			reg = compiler->getRegFromPool(scope, getType());
			asg.emitCast(reg, getSubRegister(0), sourceType);
		}
	}

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

	Array<Identifier> getInstanceIds() const override { return { getTargetVariable()->id.id }; };

	TypeInfo getTypeInfo() const override { return getSubExpr(1)->getTypeInfo(); }

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("Assignment"); }

	ValueTree toValueTree() const override
	{
		auto sourceType = getSubExpr(0)->getType();
		auto targetType = getType();

		auto t = Expression::toValueTree();
		t.setProperty("First", isFirstAssignment, nullptr);
		t.setProperty("AssignmentType", assignmentType, nullptr);
		
		return t;
	}

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
		jassert(targetType == TargetType::Variable || targetType == TargetType::Reference);
		auto v = getSubExpr(1).get();
		return dynamic_cast<VariableReference*>(v);
	}

	DotOperator* getMemberTarget() const
	{
		jassert(targetType == TargetType::ClassMember);
		return dynamic_cast<DotOperator*>(getSubExpr(1).get());
	}

	void initClassMembers(BaseCompiler* compiler, BaseScope* scope);

	TokenType assignmentType;
	bool isFirstAssignment = false;
	TargetType targetType;
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

				reg = compiler->getRegFromPool(scope, getType());

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

				simdSpanType = TypeInfo(new SpanType(compiler->getComplexTypeForAlias("float4"), numSimd));
				compiler->complexTypes.add(simdSpanType.getComplexType());
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

	FunctionCall(Location l, Ptr f, const Symbol& id_) :
		Expression(l)
	{
		function.id = id_.id;

		if (auto dp = dynamic_cast<DotOperator*>(f.get()))
		{
			objExpr = dp->getDotParent();
			addStatement(objExpr.get(), true);
		}
	};

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

	static bool canBeAliasParameter(Ptr e)
	{
		return dynamic_cast<VariableReference*>(e.get()) != nullptr;
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(function.returnType);
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (objExpr != nullptr && function.id == Identifier("toSimd"))
			{
				auto sc = new CastedSimd(location, objExpr);

				replaceInParent(sc);
				sc->process(compiler, scope);
				return;
			}
		}

		COMPILER_PASS(BaseCompiler::ResolvingSymbols)
		{
			if (callType != Unresolved)
				return;

			if (objExpr == nullptr)
			{
				// Functions without parent

				auto id = Symbol::createRootSymbol(function.id);

				if (scope->getRootClassScope()->hasFunction(id))
				{
					fc = scope->getRootClassScope();
					fc->addMatchingFunctions(possibleMatches, id);
					callType = RootFunction;
					
				}
				else if (scope->getGlobalScope()->hasFunction(id))
				{
					fc = scope->getGlobalScope();
					fc->addMatchingFunctions(possibleMatches, id);
					callType = GlobalFunction;
				}
			}
			else if (auto cptr = objExpr->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				if (fc = dynamic_cast<FunctionClass*>(scope->getRootClassScope()->getChildScope(cptr->id)))
				{
					auto id = cptr->id.getChildSymbol(function.id);
					fc->addMatchingFunctions(possibleMatches, id);
					callType = MemberFunction;
				}
				else if (fc = cptr->getExternalMemberFunctions())
				{
					auto id = cptr->id.getChildSymbol(function.id);
					fc->addMatchingFunctions(possibleMatches, id);
					callType = MemberFunction;
				}
			}
			else if (auto pv = dynamic_cast<VariableReference*>(objExpr.get()))
			{
				auto pType = pv->getTypeInfo();
				auto pNativeType = pType.getType();

				if (pNativeType == Types::ID::Event || pNativeType == Types::ID::Block)
				{
					// substitute the parent id with the Message or Block API class to resolve the pointers
					auto id = Symbol::createRootSymbol(pNativeType == Types::ID::Event ? "Message" : "Block").getChildSymbol(function.id, pType);

					fc = scope->getRootClassScope()->getSubFunctionClass(id.getParentSymbol());
					fc->addMatchingFunctions(possibleMatches, id);
					callType = NativeTypeCall;
				}
				else if (fc = scope->getRootClassScope()->getSubFunctionClass(pv->id))
				{
					// Function with registered parent object (either API class or JIT callable object)

					auto id = fc->getClassName().getChildSymbol(function.id);
					fc->addMatchingFunctions(possibleMatches, id);

					callType = pv->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				}
				else if (scope->getGlobalScope()->hasFunction(pv->id))
				{
					// Function with globally registered object (either API class or JIT callable object)
					fc = scope->getGlobalScope()->getGlobalFunctionClass(pv->id.id);

					auto id = fc->getClassName().getChildSymbol(function.id);
					fc->addMatchingFunctions(possibleMatches, id);

					callType = pv->isApiClass(scope) ? ApiFunction : ExternalObjectFunction;
				}
				else if (auto st = pv->getTypeInfo().getTypedIfComplexType<SpanType>())
				{
					if (function.id == Identifier("size"))
					{
						replaceInParent(new Immediate(location, VariableStorage((int)st->getNumElements())));
						return;
					}
				}
			}
			

			if (callType == Unresolved)
				location.throwError("Can't resolve function call " + function.getSignature());

		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			Array<TypeInfo> parameterTypes;

			for (int i = 0; i < getNumArguments(); i++)
				parameterTypes.add(getArgument(i)->getTypeInfo());

			for (auto& f : possibleMatches)
			{
				jassert(function.id == f.id);

				if (f.matchesArgumentTypes(parameterTypes))
				{
					int numArgs = f.args.size();

					for (int i = 0; i < numArgs; i++)
					{
						if (f.args[i].isReference())
						{
							if (!canBeAliasParameter(getArgument(i)))
							{
								throwError("Can't use rvalues for reference parameters");
							}
						}
					}

					function = f;
					return;
				}
			}

			throwError("Wrong argument types for function call");
		}

		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			reg = compiler->getRegFromPool(scope, getType());

			//VariableReference::reuseAllLastReferences(this);

			for (int i = 0; i < getNumArguments(); i++)
			{
				if (auto subReg = getSubRegister(i))
				{
					if (!subReg->getVariableId())
					{
						parameterRegs.add(subReg);
						continue;
					}
				}

				auto pType = function.args[i].isReference() ? Types::ID::Pointer : getArgument(i)->getType();
				auto pReg = compiler->getRegFromPool(scope, pType);
				auto asg = CREATE_ASM_COMPILER(getType());
				pReg->createRegister(asg.cc);
				parameterRegs.add(pReg);
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (!function)
			{
				if (fc == nullptr)
					throwError("Can't resolve function class");

				if (!fc->fillJitFunctionPointer(function))
					throwError("Can't find function pointer to JIT function " + function.functionName);
			}
			
			auto asg = CREATE_ASM_COMPILER(getType());

			if (function.id.toString() == "stop")
			{
				asg.dumpVariables(scope, location.getLine());

				function.functionName = "";
				function.functionName << "Line " << juce::String(location.getLine()) << " Breakpoint";
			}
			else
			{
				for (auto dv : compiler->registerPool.getListOfAllDirtyGlobals())
				{
					auto asg = CREATE_ASM_COMPILER(dv->getType());
					asg.emitMemoryWrite(dv);
				}
			}

			VariableReference::reuseAllLastReferences(this);

			for (int i = 0; i < parameterRegs.size(); i++)
			{
				auto arg = getArgument(i);
				auto existingReg = arg->reg;
				auto pReg = parameterRegs[i];
				auto acg = CREATE_ASM_COMPILER(arg->getTypeInfo().getType());

				if (function.args[i].isReference() && function.args[i].typeInfo.getType() != Types::ID::Pointer)
				{
					acg.emitComment("arg reference -> stack");
					acg.emitFunctionParameterReference(existingReg, pReg);
				}
				else
				{
					if (existingReg != nullptr && existingReg != pReg && existingReg->getVariableId())
					{
						acg.emitComment("Parameter Save");
						acg.emitStore(pReg, existingReg);
					}
					else
						parameterRegs.set(i, existingReg);
				}
			}
			
			if(function.functionName.isEmpty())
				function.functionName = function.getSignature({});

			asg.emitFunctionCall(reg, function, objExpr != nullptr ? objExpr->reg : nullptr, parameterRegs);

			for (int i = 0; i < parameterRegs.size(); i++)
			{
				if(!function.args[i].isReference())
					parameterRegs[i]->flagForReuse();
			}	
		}
	}

	CallType callType = Unresolved;
	Array<FunctionData> possibleMatches;
	FunctionData function;
	WeakReference<FunctionClass> fc;
	
	Ptr objExpr;

	ReferenceCountedArray<AssemblyRegister> parameterRegs;
};

struct Operations::ReturnStatement : public Expression
{
	SET_EXPRESSION_ID(ReturnStatement);

	ReturnStatement(Location l, Expression::Ptr expr) :
		Expression(l)
	{
		if (expr != nullptr)
        {
			addStatement(expr);
            type = Types::ID::Dynamic;
        }
		else
			type = Types::ID::Void;
	}

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
		t.setProperty("Type", Types::Helpers::getTypeName(type), nullptr);
		return t;
	}

	TypeInfo getTypeInfo() const override
	{
		return TypeInfo(type);
	}

	bool isVoid() const
	{
		return type == Types::ID::Void;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (auto fScope = dynamic_cast<FunctionScope*>(findFunctionScope(scope)))
			{
				auto expectedType = fScope->data.returnType;

				if (auto first = getSubExpr(0))
					type = first->getType();

				if (isVoid() && expectedType != Types::ID::Void)
					throwError("function must return a value");
				if (!isVoid() && expectedType == Types::ID::Void)
					throwError("Can't return a value from a void function.");

				checkAndSetType(0, TypeInfo(expectedType));
			}
			else
				throwError("Can't deduce return type.");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			if (!isVoid())
			{
				reg = compiler->registerPool.getNextFreeRegister(scope, type);
			}

			auto sourceReg = isVoid() ? nullptr : getSubRegister(0);

			asg.emitReturn(compiler, reg, sourceReg);
		}
	}

	Types::ID type;
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

	ValueTree toValueTree() const override
	{
		auto t = toValueTree();
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
						getStructType()->addMember(id, s->getTypeInfo());
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

	ScopedPointer<FunctionScope> functionScope;
	ScopedPointer<SyntaxTree> statements;
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
					jassert(!reg->isMemoryLocation());
				}
				else
				{
					if (reg == nullptr)
					{
						asg.emitComment("temp register for binary op");
						reg = compiler->getRegFromPool(scope, getType());
						usesTempRegister = true;
					}

					asg.emitStore(reg, getSubRegister(0));
				}

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
				reg = compiler->getRegFromPool(scope, Types::ID::Integer);
				asg.emitIncrement(reg, getSubRegister(0), isPreInc, isDecrement);
			}

			if (auto wt = getTypeInfo().getTypedIfComplexType<WrapType>())
				asg.emitWrap(wt, reg, isDecrement ? WrapType::OpType::Dec : WrapType::OpType::Inc);
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
		Block,
		CustomObject,
		numLoopTargetTypes
	};

	Loop(Location l, const Symbol& it_, Expression::Ptr t, Statement::Ptr b_) :
		Expression(l),
		iterator(it_)
	{
		addStatement(t);
		
		Statement::Ptr b;

		if (dynamic_cast<StatementBlock*>(b_.get()) == nullptr)
		{
			b = new StatementBlock(b_->location);
			b->addStatement(b_);
		}
		else
			b = b_;

		addStatement(b);
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

			if (auto sp = getTarget()->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				loopTargetType = Span;

				if (iterator.typeInfo.isInvalid())
					iterator.typeInfo = sp->getElementType();
				else if (iterator.typeInfo != sp->getElementType())
					location.throwError("iterator type mismatch: " + iterator.typeInfo.toString() + " expected: " + sp->getElementType().toString());

			}
			else if (getTarget()->getType() == Types::ID::Block)
			{
				loopTargetType = Block;

				iterator.typeInfo.setType(Types::ID::Float);
			}
			
			if (getLoopBlock()->blockScope == nullptr)
			{
				getLoopBlock()->blockScope = new RegisterScope(scope, location.createAnonymousScopeId());
			}

			getLoopBlock()->blockScope->addVariable(iterator);
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
				auto sp = dynamic_cast<SpanType*>(getTarget()->getTypeInfo().getComplexType().get());
					
				jassert(sp != nullptr);

				auto le = new SpanLoopEmitter(iterator, getTarget()->reg, getLoopBlock(), loadIterator);
				le->typePtr = sp;
				loopEmitter = le;
			}
			else
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

	ScopedPointer<AsmCodeGenerator::LoopEmitterBase> loopEmitter;
};


struct Operations::Negation : public Expression
{
	SET_EXPRESSION_ID(Negation);

	Negation(Location l, Expression::Ptr e) :
		Expression(l)
	{
		addStatement(e);
	};

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

				reg = compiler->getRegFromPool(scope, getType());

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

			acg.emitBranch(Types::ID::Void, cond, trueBranch, falseBranch, compiler, scope);
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
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			spanType = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<SpanType>();

			if (spanType != nullptr)
			{
				subscriptType = Span;
				elementType = spanType->getElementType();
			}
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (getSubExpr(1)->getType() != Types::ID::Integer)
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
					auto wt = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<WrapType>();

					if (wt == nullptr)
						throwError("Can't use non-constant or non-wrapped index");

					if (!isPositiveAndBelow(wt->size-1, size))
						throwError("wrap limit exceeds span size");
				}
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			reg = compiler->registerPool.getNextFreeRegister(scope, getType());
			
			auto acg = CREATE_ASM_COMPILER(getType());
			acg.emitSpanReference(reg, getSubRegister(0), getSubRegister(1), elementType.getRequiredByteSize());
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

	ComplexTypeDefinition(Location l, const Array<Identifier>& ids_, TypeInfo type_) :
		Expression(l),
		ids(ids_),
		type(type_)
	{

	}

	Array<Identifier> getInstanceIds() const override { return ids; }

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
			symbols.add(Symbol({ id }, getTypeInfo()));
		}

		return symbols;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
		{
			if(type.isComplexType())
				type.getComplexType()->finaliseAlignment();
		}
		COMPILER_PASS(BaseCompiler::DataSizeCalculation)
		{
			if (!isStackDefinition(scope))
				scope->getRootData()->enlargeAllocatedSize(type);
		}
		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			for (auto s : getSymbols())
			{
				if (isStackDefinition(scope))
				{
					if (!scope->addVariable(s))
						throwError("Can't allocate variable " + s.toString());
				}
				else
					scope->getRootData()->allocate(scope, s);
			}
		}
		COMPILER_PASS(BaseCompiler::DataInitialisation)
		{
			if (getNumChildStatements() == 0 && initValues == nullptr)
			{
				initValues = type.makeDefaultInitialiserList();
			}

			if (getNumChildStatements() == 1 && getSubExpr(0)->isConstExpr())
			{
				jassertfalse;
			}

			if (!isStackDefinition(scope))
			{
				for (auto s : getSymbols())
				{
					if (scope->getRootClassScope() == scope)
					{
						auto r = scope->getRootData()->initData(scope, s, initValues);

						if (!r.wasOk())
							location.throwError(r.getErrorMessage());
					}
					else if (auto cScope = dynamic_cast<ClassScope*>(scope))
					{
						if (auto st = dynamic_cast<StructType*>(cScope->typePtr.get()))
							st->setDefaultValue(s.id, initValues);
					}
				}
			}
		}
		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			if (isStackDefinition(scope))
			{
				auto acg = CREATE_ASM_COMPILER(getType());

				for (auto s : getSymbols())
				{
					auto c = acg.cc.newStack(type.getRequiredByteSize(), type.getRequiredAlignment(), "funky");

					auto reg = compiler->registerPool.getRegisterForVariable(scope, s);
					reg->setCustomMemoryLocation(c);
					stackLocations.add(reg);
				}
			}
		}
		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (isStackDefinition(scope))
			{
				auto acg = CREATE_ASM_COMPILER(getType());

				for (auto s : stackLocations)
				{
					RegPtr expr;

					if(getNumChildStatements() > 0)
						expr = getSubRegister(0);

					acg.emitStackInitialisation(s, type.getComplexType(), expr, initValues);

					if (auto wt = type.getTypedIfComplexType<WrapType>())
						acg.emitWrap(wt, s, WrapType::OpType::Set);
				}
					
			}
		}
	}

	bool isStackDefinition(BaseScope* scope) const
	{
		return dynamic_cast<RegisterScope*>(scope) != nullptr;
	}

	Array<Identifier> ids;
	TypeInfo type;

	InitialiserList::Ptr initValues;

	ReferenceCountedArray<AssemblyRegister> stackLocations;
};


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

}
}
