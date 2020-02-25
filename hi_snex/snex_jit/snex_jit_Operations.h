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
public:

	StatementBlock(Location l) :
		Statement(l)
	{}

	Types::ID getType() const override { return Types::ID::Void; };

	

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
	Noop(Location l) :
		Expression(l)
	{}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Statement::process(compiler, scope);
	}

	Types::ID getType() const override { return Types::ID::Void; };
};


struct Operations::Immediate : public Expression
{
	Immediate(Location loc, VariableStorage value) :
		Expression(loc),
		v(value)
	{};

	Types::ID getType() const override { return v.getType(); }

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
	VariableReference(Location l, const Symbol& id_) :
		Expression(l),
		id(id_)
	{
		jassert(id);
	};

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

	ComplexType::Ptr getComplexType() const override
	{
		return id.typePtr;
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

	Types::ID getType() const override { return id.type; }

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
	Cast(Location l, Expression::Ptr expression, Types::ID targetType) :
		Expression(l)
	{
		addStatement(expression);
		type = targetType;
	};

	// Make sure the target type is used.
	Types::ID getType() const override { return type; }

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
			auto asg = CREATE_ASM_COMPILER(type);
			auto sourceType = getSubExpr(0)->getType();
			reg = compiler->getRegFromPool(scope, type);
			asg.emitCast(reg, getSubRegister(0), sourceType);
		}
	}
};

struct Operations::DotOperator : public Expression
{
	DotOperator(Location l, Ptr parent, Ptr child):
		Expression(l)
	{
		addStatement(parent);
		addStatement(child);
	}

	Ptr getDotParent() const
	{
		return getSubExpr(0);
	}

	Ptr getDotChild() const
	{
		return getSubExpr(1);
	}

	Types::ID getType() const override
	{
		return getDotChild()->getType();
	}

	

	ComplexType::Ptr getComplexType() const override
	{
		return getDotChild()->getComplexType();
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

	ComplexType::Ptr getTypePtr() const override { return nullptr; }
	Identifier getInstanceId() const override { return getTargetVariable()->id.id; };
	Types::ID getNativeType() const override { return getTargetVariable()->getType(); }

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
		return dynamic_cast<VariableReference*>(getSubExpr(1).get());
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
		type = Types::Integer;
		addStatement(l);
		addStatement(r);
	};

	Types::ID getType() const override { return Types::Integer; }

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
				logWarning("Implicit cast to float for comparison");
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
	LogicalNot(Location l, Ptr expr) :
		Expression(l)
	{
		addStatement(expr);
	};

	Types::ID getType() const override { return Types::ID::Integer; }

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
			auto asg = CREATE_ASM_COMPILER(Types::ID::Integer);

			reg = asg.emitLogicalNot(getSubRegister(0));

			//getSubRegister(0)->flagForReuseIfAnonymous();
		}
	}
};

struct Operations::TernaryOp : public Expression,
							   public BranchingStatement
{
public:

	TernaryOp(Location l, Expression::Ptr c, Expression::Ptr t, Expression::Ptr f) :
		Expression(l)
	{
		addStatement(c);
		addStatement(t);
		addStatement(f);
	}

	Types::ID getType() const override
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
			checkAndSetType(1);
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());
			reg = asg.emitTernaryOp(this, compiler, scope);
		}
	}

private:


};

struct Operations::CastedSimd : public Expression
{
	CastedSimd(Location l, Ptr original) :
		Expression(l)
	{
		addStatement(original);
	}

	ComplexType::Ptr getComplexType() const override
	{
		return simdSpanType;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (auto ost = dynamic_cast<SpanType*>(getSubExpr(0)->getComplexType().get()))
			{
				int numSimd = ost->getNumElements() / 4;

				simdSpanType = new SpanType(compiler->getComplexTypeForAlias("float4"), numSimd);
				compiler->complexTypes.add(simdSpanType);
			}
			else
				throwError("Can't convert non-span to SIMD span");
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (auto ost = dynamic_cast<SpanType*>(getSubExpr(0)->getComplexType().get()))
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

	ComplexType::Ptr simdSpanType;
};

struct Operations::FunctionCall : public Expression
{
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
			else if (auto cptr = dynamic_cast<StructType*>(objExpr->getComplexType().get()))
			{
				if (fc = dynamic_cast<FunctionClass*>(scope->getRootClassScope()->getChildScope(cptr->id)))
				{
					auto id = cptr->id.getChildSymbol(function.id);
					fc->addMatchingFunctions(possibleMatches, id);
					callType = MemberFunction;
				}
			}
			else if (auto pv = dynamic_cast<VariableReference*>(objExpr.get()))
			{
				auto pType = pv->getType();

				if (pType == Types::ID::Event || pType == Types::ID::Block)
				{
					// substite the parent id with the Message or Block API class to resolve the pointers
					auto id = Symbol::createRootSymbol(type == Types::ID::Event ? "Message" : "Block").getChildSymbol(function.id, pType);

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
				else if (auto st = dynamic_cast<SpanType*>(pv->getComplexType().get()))
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
			Array<Types::ID> parameterTypes;

			for (int i = 0; i < getNumArguments(); i++)
				parameterTypes.add(getArgument(i)->getType());

			for (auto& f : possibleMatches)
			{
				jassert(function.id == f.id);

				if (f.matchesArgumentTypes(parameterTypes))
				{
					int numArgs = f.args.size();

					for (int i = 0; i < numArgs; i++)
					{
						if (f.args[i].isAlias)
						{
							if (!canBeAliasParameter(getArgument(i)))
							{
								throwError("Can't use rvalues for reference parameters");
							}
						}
					}

					function = f;
					type = function.returnType;
					return;
				}
			}

			throwError("Wrong argument types for function call");
		}

		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			reg = compiler->getRegFromPool(scope, type);

			//VariableReference::reuseAllLastReferences(this);

			for (int i = 0; i < getNumArguments(); i++)
			{
				bool isReference = function.args[i].isAlias;

				if (auto subReg = getSubRegister(i))
				{
					if (!subReg->getVariableId())
					{
						parameterRegs.add(subReg);
						continue;
					}
				}

				auto pReg = compiler->getRegFromPool(scope, getArgument(i)->getType());
				auto asg = CREATE_ASM_COMPILER(type);
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
			
			auto asg = CREATE_ASM_COMPILER(type);

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

				if (existingReg != nullptr && existingReg != pReg && existingReg->getVariableId())
				{
					auto pType = arg->getType();
					auto typedAcg = CREATE_ASM_COMPILER(pType);
					typedAcg.emitComment("Parameter Save");
					typedAcg.emitStore(pReg, existingReg);
				}
				else
				{
					parameterRegs.set(i, existingReg);
				}
			}
			
			if(function.functionName.isEmpty())
				function.functionName = function.getSignature({});

			asg.emitFunctionCall(reg, function, objExpr != nullptr ? objExpr->reg : nullptr, parameterRegs);

			for (int i = 0; i < parameterRegs.size(); i++)
			{
				if(!function.args[i].isAlias)
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

	Types::ID getType() const override
	{
		return type;
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

				checkAndSetType(0, expectedType);
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
};


struct Operations::ClassStatement : public Statement
{
	using Ptr = ReferenceCountedObjectPtr<ClassStatement>;

	ClassStatement(Location l, ComplexType::Ptr classType_, Statement::Ptr classBlock) :
		Statement(l),
		classType(classType_)
	{
		addStatement(classBlock);
	}

	Types::ID getType() const override { return Types::ID::Void; }

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
					if (auto ctPtr = td->getTypePtr())
						getStructType()->addComplexMember(td->getInstanceId(), ctPtr);
					else
					{
						auto type = td->getNativeType();

						if (!Types::Helpers::isFixedType(type))
							location.throwError("Can't use auto on member variables");

						getStructType()->addNativeMember(td->getInstanceId(), type);
					}
				}
			}
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

	void handleError(asmjit::Error, const char* message, asmjit::BaseEmitter* emitter) override
	{
		throwError(juce::String(message));
	}

	Types::ID getType() const override { return data.returnType; }

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
	BinaryOp(Location l, Expression::Ptr left, Expression::Ptr right, TokenType opType) :
		Expression(l),
		op(opType)
	{
		addStatement(left);
		addStatement(right);
	};

	bool isLogicOp() const { return op == JitTokens::logicalOr || op == JitTokens::logicalAnd; }

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
				checkAndSetType(0, Types::ID::Integer);
			}
			else
				checkAndSetType();
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




struct Operations::BlockAccess : public Expression
{
	BlockAccess(Location l, Ptr target, Ptr index) :
		Expression(l)
	{
		addStatement(index);
		addStatement(target);

		type = Types::ID::Float;
	}

	static Symbol isWrappedBufferReference(Statement::Ptr expr, BaseScope* scope);

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::SyntaxSugarReplacements)
		{
			jassertfalse; // neu denken
#if 0
			auto wrappedSymbol = isWrappedBufferReference(getChildStatement(1), scope);

			if (wrappedSymbol.id.isValid())
			{
				Symbol getter;
				getter = wrappedSymbol.getChildSymbol("getAt");

				auto indexStatement = getChildStatement(0);

				bool isZero = false;
				bool isInterpolating = indexStatement->getType() != Types::ID::Integer;

				if (isInterpolating)
				{
					getter.id = "getInterpolated";
				}
				else if (auto zeroImmediate = dynamic_cast<Immediate*>(indexStatement.get()))
				{
					if (zeroImmediate->v.toInt() == 0)
					{
						getter.id = "get";
						isZero = true;
					}
						
				}
				
				auto fc = new FunctionCall(location, getter);

				//fc->process(compiler, scope);

				compiler->setCurrentPass(BaseCompiler::ResolvingSymbols);

				fc->process(compiler, scope);

				if (!isZero)
					fc->addStatement(indexStatement);

				compiler->setCurrentPass(BaseCompiler::TypeCheck);
				fc->process(compiler, scope);
				compiler->setCurrentPass(BaseCompiler::SyntaxSugarReplacements);

				indexStatement->process(compiler, scope);

				Expression::replaceInParent(fc);

				return;
			}
#endif
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (!isWrappedBufferReference(getChildStatement(1), scope))
			{
				if (getSubExpr(0)->getType() != Types::ID::Integer)
					throwError("subscription index must be an integer");

				if (getSubExpr(1)->getType() != Types::ID::Block)
					throwError("Can't use [] on primitive types");
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (auto cs = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{
				jassertfalse; // neu denken...
#if 0
				Array<FunctionData> matches;
				cs->addMatchingFunctions(matches, { {"Block", "getSample"}, Types::ID::Float, true, false });

				reg = compiler->getRegFromPool(scope, type);

				auto asg = CREATE_ASM_COMPILER(type);

				ReferenceCountedArray<AssemblyRegister> parameters;

				getSubRegister(0)->loadMemoryIntoRegister(asg.cc);

				parameters.add(getSubRegister(1));
				parameters.add(getSubRegister(0));

				asg.emitFunctionCall(reg, matches[0], parameters);
#endif
			}
		}
	}
};

struct Operations::Increment : public UnaryOp
{
	Increment(Location l, Ptr expr, bool isPre_, bool isDecrement_) :
		UnaryOp(l, expr),
		isPreInc(isPre_),
		isDecrement(isDecrement_)
	{
		type = Types::ID::Integer;
	};

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::SyntaxSugarReplacements)
		{
			


			if (removed)
				return;

			auto symbol = BlockAccess::isWrappedBufferReference(getChildStatement(0), scope);

			if (symbol.id.isValid())
			{
				jassertfalse; // neu denken

#if 0
				Symbol incSymbol;

				if (isPreInc)
					incSymbol = symbol.getChildSymbol("getAndInc");
				else
					incSymbol = symbol.getChildSymbol("getAndPostInc");

				auto fc = new FunctionCall(location, incSymbol);

				compiler->setCurrentPass(BaseCompiler::ResolvingSymbols);

				fc->process(compiler, scope);
				compiler->setCurrentPass(BaseCompiler::TypeCheck);
				fc->process(compiler, scope);
				compiler->setCurrentPass(BaseCompiler::SyntaxSugarReplacements);

				replaceInParent(fc);

				removed = true;

				return;
#endif
			}

		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (BlockAccess::isWrappedBufferReference(getChildStatement(0), scope))
			{
				type = Types::ID::Float;
			}
			else
			{
				if (dynamic_cast<Increment*>(getSubExpr(0).get()) != nullptr)
					throwError("Can't combine incrementors");

					if (getSubExpr(0)->getType() != Types::ID::Integer)
						throwError("Can't increment non integer variables.");
			}
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(type);

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

			if (auto wt = dynamic_cast<WrapType*>(getSubExpr(0)->getComplexType().get()))
			{
				asg.emitWrap(wt, reg, isDecrement ? WrapType::OpType::Dec : WrapType::OpType::Inc);
			}
		}
	}

	bool isDecrement;
	bool isPreInc;
	bool removed = false;
};

struct Operations::BlockAssignment : public Expression
{
	BlockAssignment(Location l, Ptr target, TokenType opType, Ptr v) :
		Expression(l),
		op(opType)
	{
		addStatement(v);
		addStatement(target);
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		COMPILER_PASS(BaseCompiler::SyntaxSugarReplacements)
		{
			jassertfalse; // neu denken

#if 0
			if (replaced)
				return;

			auto bVar = getChildStatement(1)->getChildStatement(1);

			auto wrappedSymbol = BlockAccess::isWrappedBufferReference(bVar, scope);

			if(wrappedSymbol.id.isValid())
			{
				auto setter = wrappedSymbol.getChildSymbol("setAt");

				auto indexStateement = getChildStatement(1)->getChildStatement(0);
				auto valueStatement = getChildStatement(0);

				bool isZero = false;
				
				if (auto zeroImmediate = dynamic_cast<Immediate*>(indexStateement.get()))
				{
					if (zeroImmediate->v.toInt() == 0)
					{
						setter.id = "set";
						isZero = true;
					}
				}
				
				auto fc = new FunctionCall(location, setter);

				compiler->setCurrentPass(BaseCompiler::ResolvingSymbols);
				fc->process(compiler, scope);

				if (!isZero)
					fc->addStatement(indexStateement);

				fc->addStatement(valueStatement);

				compiler->setCurrentPass(BaseCompiler::TypeCheck);
				fc->process(compiler, scope);
				compiler->setCurrentPass(BaseCompiler::SyntaxSugarReplacements);

				indexStateement->process(compiler, scope);
				valueStatement->process(compiler, scope);

				Expression::replaceInParent(fc);
				replaced = true;

				return;
			}
#endif
		}

		// We need to move this before the base class process()
		// in order to prevent code generation for b[x]...
		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (auto cs = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{

				jassertfalse; // neu denken
#if 0
				Ptr index = getSubExpr(1)->getSubExpr(0);
				Ptr bl = getSubExpr(1)->getSubExpr(1);
				Ptr vl = getSubExpr(0);

				index->process(compiler, scope);
				bl->process(compiler, scope);
				vl->process(compiler, scope);

				auto asg = CREATE_ASM_COMPILER(Types::ID::Void);

				index->reg->loadMemoryIntoRegister(asg.cc);
				bl->reg->loadMemoryIntoRegister(asg.cc);
				vl->reg->loadMemoryIntoRegister(asg.cc);

				ReferenceCountedArray<AssemblyRegister> parameters;
				parameters.add(bl->reg);
				parameters.add(index->reg);
				parameters.add(vl->reg);

				Array<FunctionData> matches;
				cs->addMatchingFunctions(matches, { {"Block", "setSample"}, Types::ID::Float , false, false});

				asg.emitFunctionCall(nullptr, matches.getFirst(), parameters);
#endif
			}

			return;
		}

		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (getSubExpr(0)->getType() != Types::ID::Float)
			{
				Ptr implicitCast = new Operations::Cast(location, getSubExpr(0), Types::ID::Float);
				logWarning("Implicit cast to float for []-operation");
				replaceChildStatement(0, implicitCast);
			}
		}
	}

	bool replaced = false;

	TokenType op;
};


struct Operations::Loop : public Expression,
							   public Operations::ConditionalBranch
{
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

			if (auto sp = dynamic_cast<SpanType*>(getTarget()->getComplexType().get()))
			{
				loopTargetType = Span;

				if (iterator.type == Types::ID::Dynamic)
					iterator.type = sp->getElementType();
				else if (iterator.type != sp->getElementType())
					location.throwError("iterator type mismatch: " + Types::Helpers::getTypeName(iterator.type) + " expected: " + Types::Helpers::getTypeName(sp->getElementType()));

				if (iterator.type == Types::ID::Pointer)
					iterator = iterator.withComplexType(sp->getComplexElementType());
			}
			else if (getTarget()->getType() == Types::ID::Block)
			{
				loopTargetType = Block;

				iterator.type = Types::ID::Float;
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
			auto acg = CREATE_ASM_COMPILER(iterator.type);

			getTarget()->process(compiler, scope);

			auto r = getTarget()->reg;

			jassert(r != nullptr && r->getScope() != nullptr);

			allocateDirtyGlobalVariables(getLoopBlock(), compiler, scope);

			if (loopTargetType == Span)
			{
				auto sp = dynamic_cast<SpanType*>(getTarget()->getComplexType().get());
					
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


#if 0		// the codegen for the block iteration, add later in a subclass
			
#endif
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
	Negation(Location l, Expression::Ptr e) :
		Expression(l)
	{
		addStatement(e);
	};

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
		}
	}
};

struct Operations::IfStatement : public Statement,
								 public Operations::ConditionalBranch,
								 public Operations::BranchingStatement
{
	IfStatement(Location loc, Expression::Ptr cond, Ptr trueBranch, Ptr falseBranch):
		Statement(loc)
	{
		addStatement(cond.get());
		addStatement(trueBranch);

		if (falseBranch != nullptr)
			addStatement(falseBranch);
	}

	Types::ID getType() const override { return Types::ID::Void; }

	bool hasFalseBranch() const { return getNumChildStatements() > 2; }
	
	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Statement::process(compiler, scope);

		if (compiler->getCurrentPass() != BaseCompiler::CodeGeneration)
			processAllChildren(compiler, scope);
		
		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			processAllChildren(compiler, scope);

			if (getCondition()->getType() != Types::ID::Integer)
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

	ComplexType::Ptr getComplexType() const override
	{
		return elementType;
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			spanType = dynamic_cast<SpanType*>(getSubExpr(0)->getComplexType().get());

			if (spanType != nullptr)
			{
				subscriptType = Span;

				if (elementType = spanType->getComplexElementType())
					type = Types::ID::Pointer;
				else
					type = spanType->getElementType();
			}
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (getSubExpr(1)->getType() != Types::ID::Integer)
				throwError("index must be an integer value");
			
			

				
			if (spanType == nullptr)
			{
				if (getSubExpr(0)->getType() == Types::ID::Block)
					type = Types::ID::Float;
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
					auto wt = dynamic_cast<WrapType*>(getSubExpr(1)->getComplexType().get());

					if (wt == nullptr)
						throwError("Can't use non-constant or non-wrapped index");

					if (!isPositiveAndBelow(wt->size-1, size))
						throwError("wrap limit exceeds span size");
				}
			}

			
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			reg = compiler->registerPool.getNextFreeRegister(scope, type);
			
			auto acg = CREATE_ASM_COMPILER(type);
			acg.emitSpanReference(reg, getSubRegister(0), getSubRegister(1), getElementSize());
		}
	}

	size_t getElementSize() const
	{
		if (elementType != nullptr)
			return elementType->getRequiredByteSize();
		else
			return Types::Helpers::getSizeForType(type);
	}

	SubscriptType subscriptType = Undefined;
	bool isWriteAccess = false;
	SpanType* spanType = nullptr;
	ComplexType::Ptr elementType;
};


#if 0
struct Operations::WrappedBlockDefinition : public Statement
{
	WrappedBlockDefinition(Location l, BaseScope::Symbol id_, Expression::Ptr expr) :
		Statement(l),
		id(id_)
	{
		if (auto im = dynamic_cast<Immediate*>(expr.get()))
		{
			b = im->v.toBlock();
		}
	};

	BaseScope::Symbol id;
	
	block b;
	
	void process(BaseCompiler* compiler, BaseScope* scope);

	Types::ID getType() const override { return Types::ID::Block; }
};
#endif

#if 0
struct Operations::ClassInstance : public Statement,
	public TypeDefinitionBase
{
	ClassInstance(Location l, const Symbol& classId_, const Array<Symbol>& instanceIds_, InitialiserList::Ptr initList_) :
		Statement(l),
		classId(classId_),
		instanceIds(instanceIds_),
		initList(initList_)
	{}

	Types::ID getType() const override { return Types::ID::Pointer; }

	Types::ID getNativeType() const override { return Types::ID::Dynamic; };

	Identifier getInstanceId() const override { return instanceIds.getFirst().id; };

	ComplexType::Ptr getTypePtr() const override { return typePtr; };

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
		{
			StructType t(classId);

			for (auto ct : compiler->complexTypes)
			{
				if (ct->getActualTypeString() == t.getActualTypeString())
				{
					typePtr = ct;
					break;
				}
			}

			if (typePtr == nullptr)
				throwError("Use of undefined type " + classId.toString());
		}

		COMPILER_PASS(BaseCompiler::DataSizeCalculation)
		{
			if (scope->getRootClassScope() == scope)
			{
				scope->getRootData()->enlargeAllocatedSize(typePtr->getRequiredByteSize());
			}
		}

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (scope->getRootClassScope() == scope)
			{
				for (auto instanceId : instanceIds)
				{
					scope->getRootClassScope()->rootData->allocate(scope, instanceId.withComplexType(typePtr));
				}
			}
		}

		COMPILER_PASS(BaseCompiler::DataInitialisation)
		{
			if (initList == nullptr)
				initList = typePtr->makeDefaultInitialiserList();

			for (auto id : instanceIds)
			{
				auto ok = scope->getRootData()->initData(scope, id, initList);

				if (!ok.wasOk())
					location.throwError(ok.getErrorMessage());
			}
		}
	}

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const override
	{
		if (typePtr != nullptr)
			return typePtr->getRequiredByteSize();
		else
		{
			jassertfalse;
			return 0;
		}

	}

	WeakReference<ClassScope> instanceScope;

	Symbol classId;
	Array<Symbol> instanceIds;

	ClassStatement::Ptr classStatement;
	ComplexType::Ptr typePtr;
	InitialiserList::Ptr initList;
};

struct Operations::WrapDefinition : public Statement,
									public TypeDefinitionBase
{
	WrapDefinition(Location l, const Symbol& s, InitialiserList::Ptr initList):
		Statement(l),
		id(s),
		initValue(initList)
	{
		jassert(id.type == Types::ID::Integer);
		jassert(id.typePtr != nullptr);
	}

	Types::ID getType() const { return id.type; }
	
	Types::ID getNativeType() const override { return id.type; }
	ComplexType::Ptr getTypePtr() const override { return id.typePtr; }
	Identifier getInstanceId() const override { return id.id; }

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		if (scope->getScopeType() != BaseScope::Class)
			throwError("Can't define spans in functions");

		COMPILER_PASS(BaseCompiler::DataSizeCalculation)
		{
			if (scope->getRootClassScope() == scope)
				scope->getRootData()->enlargeAllocatedSize(id.typePtr->getRequiredByteSize());
		}

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (scope->getRootClassScope() == scope)
				scope->getRootData()->allocate(scope, id);
		}

		COMPILER_PASS(BaseCompiler::DataInitialisation)
		{
			if (initValue != nullptr)
			{
				VariableStorage v;

				initValue->getValue(0, v);

				auto wt = dynamic_cast<WrapType*>(id.typePtr.get());

				auto actualValue = v.toInt() % wt->size;

				initValue = new InitialiserList();
				initValue->addImmediateValue(VariableStorage(actualValue));

				auto r = scope->getRootData()->initData(scope, id, initValue);

				if (!r.wasOk())
					location.throwError(r.getErrorMessage());
			}
		}
	}

	Symbol id;
	InitialiserList::Ptr initValue;
};

struct Operations::SpanDefinition : public Statement,
	public TypeDefinitionBase
{
	SpanDefinition(Location l, SpanType::Ptr spanType_, const Symbol& s, InitialiserList::Ptr initValues_) :
		Statement(l),
		id(s.withType(Types::ID::Pointer)),
		spanType(spanType_),
		initValues(initValues_)
	{
	}

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* s) const override
	{
		return spanType->getRequiredByteSize();
	}

	Types::ID getNativeType() const override { return Types::ID::Dynamic; }
	ComplexType::Ptr getTypePtr() const override { return spanType; }
	Identifier getInstanceId() const override { return id.id; }

	Types::ID getType() const override
	{
		return Types::ID::Pointer;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		if (scope->getScopeType() != BaseScope::Class)
			throwError("Can't define spans in functions");

		COMPILER_PASS(BaseCompiler::DataSizeCalculation)
		{
			if (scope->getRootClassScope() == scope)
				scope->getRootData()->enlargeAllocatedSize(spanType->getRequiredByteSize());
		}

		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			if (scope->getRootClassScope() == scope)
				scope->getRootData()->allocate(scope, id.withComplexType(spanType));
		}

		COMPILER_PASS(BaseCompiler::DataInitialisation)
		{
			if (initValues == nullptr)
				initValues = spanType->makeDefaultInitialiserList();

			if (scope->getRootClassScope() == scope)
			{
				auto r = scope->getRootData()->initData(scope, id, initValues);

				if (!r.wasOk())
					location.throwError(r.getErrorMessage());
			}
			else if (auto cScope = dynamic_cast<ClassScope*>(scope))
			{
				if (auto st = dynamic_cast<StructType*>(cScope->typePtr.get()))
				{
					st->setDefaultValue(id.id, initValues);
				}
			}
		}
	}

	Symbol id;
	SpanType::Ptr spanType;
	InitialiserList::Ptr initValues;
};
#endif

struct Operations::ComplexStackDefinition : public Expression,
											public TypeDefinitionBase
{
	ComplexStackDefinition(Location l, const Array<Identifier>& ids_, ComplexType::Ptr typePtr_) :
		Expression(l),
		ids(ids_),
		typePtr(typePtr_)
	{

	}

	Types::ID getNativeType() const override { return Types::ID::Pointer; }
	ComplexType::Ptr getTypePtr() const override { return getComplexType(); }
	Identifier getInstanceId() const override { return ids.getFirst(); }

	Array<Symbol> getSymbols() const
	{
		Array<Symbol> symbols;

		for (auto id : ids)
		{
			auto s = Symbol::createRootSymbol(id).withComplexType(typePtr);
			s.const_ = false;
			s.type = typePtr->getDataType();
			symbols.add(s);
		}

		return symbols;
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

#if 0
		COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
		{
			

			StructType t(id);

			for (auto ct : compiler->complexTypes)
			{
				if (ct->getActualTypeString() == t.getActualTypeString())
				{
					id = id.withComplexType(ct);
					break;
				}
			}

			if (getComplexType() == nullptr)
				throwError("Use of undefined type " + id.toString());
		}
#endif
		COMPILER_PASS(BaseCompiler::DataSizeCalculation)
		{
			if (!isStackDefinition(scope))
				scope->getRootData()->enlargeAllocatedSize(getComplexType()->getRequiredByteSize());
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
				initValues = getComplexType()->makeDefaultInitialiserList();
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
				auto t = getComplexType();

				auto acg = CREATE_ASM_COMPILER(getType());

				for (auto s : getSymbols())
				{
					auto c = acg.cc.newStack(t->getRequiredByteSize(), t->getRequiredAlignment(), "funky");

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

				for(auto s: stackLocations)
					acg.emitInitialiserList(s, getComplexType(), initValues);
			}
		}
	}

	bool isStackDefinition(BaseScope* scope) const
	{
		return dynamic_cast<RegisterScope*>(scope) != nullptr;
	}

	ComplexType::Ptr getComplexType() const override
	{
		return typePtr;
	}

	Types::ID getType() const override
	{
		return Types::ID::Pointer;
	}

	Array<Identifier> ids;
	ComplexType::Ptr typePtr;

	InitialiserList::Ptr initValues;

	ReferenceCountedArray<AssemblyRegister> stackLocations;
};

struct Operations::SmoothedVariableDefinition : public Statement
{
	SmoothedVariableDefinition(Location l, BaseScope::Symbol id_, Types::ID t, VariableStorage initialValue) :
		Statement(l),
		type(t),
		id(id_),
		iv(initialValue)
	{
		
	}

	void process(BaseCompiler* compiler, BaseScope* scope);

	BaseScope::Symbol id;
	VariableStorage iv;

	Types::ID getType() const override { return type; }

	Types::ID type;
};

struct Operations::InlinedExternalCall : public Expression
{
	InlinedExternalCall(FunctionCall* functionCallToBeReplaced):
		Expression(functionCallToBeReplaced->location),
		f(functionCallToBeReplaced->function)
	{
		for (int i = 0; i < functionCallToBeReplaced->getNumArguments(); i++)
		{
			addStatement(functionCallToBeReplaced->getArgument(i));
		}
	}

	Types::ID getType() const override
	{
		return f.returnType;
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
