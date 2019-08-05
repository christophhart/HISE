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


struct Operations::StatementBlock : public Statement
{
public:

	StatementBlock(Location l) :
		Statement(l)
	{}

	Types::ID getType() const override { return Types::ID::Void; };

	void addStatement(Statement* s)
	{
		statements.add(s);
		s->setParent(this);
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Statement::process(compiler, scope);

		if (compiler->getCurrentPass() == BaseCompiler::ResolvingSymbols)
		{
			if (blockScope == nullptr)
				blockScope = new RegisterScope(scope);
		}

		for (auto s : statements)
			s->process(compiler, blockScope);
	}

	ScopedPointer<RegisterScope> blockScope;

	ReferenceCountedArray<Statement> statements;
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
			reg = compiler->getRegFromPool(getType());
			reg->setDataPointer(&v);
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
	{};

	bool isLastVariableReference() const
	{
		SyntaxTreeWalker walker(this);

		VariableReference* lastOne = nullptr;

		bool isLast = false;

		while (lastOne = walker.getNextStatementOfType<VariableReference>())
		{
			isLast = lastOne == this;
		}

		return isLast;
	}

	bool isReferencedOnce() const
	{
		// The class variable definition is also a reference so we 
		// need to take it into account.
		return ref->getNumReferences() == (isClassVariable ? 2 : 1);
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::ResolvingSymbols)
		{
			// We will create the Reference to the according scope for this
			// variable, check it's type and if it's a parameter, get the index...

			if (isLocalToScope)
			{
				type = id.type;

				if (scope->getScopeForSymbol(id) != nullptr)
					logWarning("Declaration hides previous variable definition");

				auto r = scope->allocate(id.id, VariableStorage(getType(), 0));

				if (r.failed())
					throwError(r.getErrorMessage() + id.toString());

				ref = scope->get(id);

				if (isLocalConst)
					ref->isConst = true;

				isClassVariable = false;
			}
			else if (auto vScope = scope->getScopeForSymbol(id))
			{
				ref = vScope->get(id);
				type = ref->id.type;

				if (vScope == scope)
				{
					isLocalToScope = true;
					isLocalConst = ref->isConst;
				}
				else
					isClassVariable = vScope == findClassScope(scope);

				if (auto fScope = dynamic_cast<FunctionScope*>(vScope))
				{
					parameterIndex = fScope->parameters.indexOf(id.id);

					if (parameterIndex != -1)
						type = fScope->data.args[parameterIndex];
				}
			}
			else
				throwError("Can't resolve variable " + id.toString());

			if (auto st = findParentStatementOfType<SyntaxTree>(this))
				st->addVariableReference(this);
			else
				jassertfalse;

			// Reset the ID, because we don't need it anymore...
			id = {};
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			// Nothing to do...
		}

		bool initialiseVariables = (currentPass == BaseCompiler::RegisterAllocation && parameterIndex != -1) ||
			(currentPass == BaseCompiler::CodeGeneration && parameterIndex == -1);

		if (initialiseVariables)
		{
			reg = compiler->registerPool.getRegisterForVariable(ref);

			if (reg->isIteratorRegister())
				return;

			auto asg = CREATE_ASM_COMPILER(type);

			if (parameterIndex != -1 && isFirstReference())
			{
				asg.emitParameter(reg, parameterIndex);
				return;
			}

			if (isFirstReference())
			{
				auto assignmentType = isWriteAccess();

				auto* dataPointer = &ref.get()->getDataReference(assignmentType == JitTokens::assign_);

				reg->setDataPointer(dataPointer);

				if (assignmentType != JitTokens::void_)
				{
					if (assignmentType != JitTokens::assign_)
					{
						reg->loadMemoryIntoRegister(asg.cc);
					}
					else
					{
						reg->createRegister(asg.cc);
					}
				}
				else
				{
					reg->createMemoryLocation(asg.cc);

					if (!isReferencedOnce())
						reg->loadMemoryIntoRegister(asg.cc);
				}
			}
		}
	}

	TokenType isWriteAccess();

	bool isFirstReference()
	{
		SyntaxTreeWalker walker(this);

		while (auto v = walker.getNextStatementOfType<VariableReference>())
		{
			if (v->ref == ref)
				return v == this;
		}

		jassertfalse;
		return true;
	}

	//bool isFirstReference = false;

	int parameterIndex = -1;
	Symbol id;					// The Symbol from the parser
	BaseScope::RefPtr ref;		// The Reference from the resolver
	bool isLocalToScope = false;
	bool isLocalConst = false;
	bool isClassVariable = false;
};

struct Operations::Assignment : public Expression
{
	Assignment(Location l, Expression::Ptr target, TokenType assignmentType_, Expression::Ptr expr) :
		Expression(l),
		assignmentType(assignmentType_)
	{
		addSubExpression(expr);
		addSubExpression(target); // the target must be evaluated after the expression
	}

	bool isLastAssignmentToTarget() const
	{
		auto tree = findParentStatementOfType<SyntaxTree>(this);
		return this == tree->getLastAssignmentForReference(getTargetVariable()->ref);
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		/*
		ResolvingSymbols: check that target is not const
		TypeCheck, = > check type match
		DeadCodeElimination, = > remove unreferenced local variables
		Inlining, = > make self assignment
		CodeGeneration, = > Store or Op
		*/

		Expression::process(compiler, scope);


		COMPILER_PASS(BaseCompiler::ResolvingSymbols)
		{
			if (getSubExpr(0)->isConstExpr() && findClassScope(scope) == scope)
			{
				auto& ref = getTargetVariable()->ref->getDataReference(true);

				ref = getSubExpr(0)->getConstExprValue();
			}

			auto v = getTargetVariable();

			if (v->isLocalConst && !v->isFirstReference())
				throwError("Can't change constant variable");
		}


		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			checkAndSetType();
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto acg = CREATE_ASM_COMPILER(type);

			auto value = getSubRegister(0);
			auto tReg = getSubRegister(1);

			if (assignmentType == JitTokens::assign_)
			{
				if (tReg != value)
					acg.emitStore(tReg, value);
			}
			else
				acg.emitBinaryOp(assignmentType, tReg, value);

			//getSubRegister(0)->flagForReuseIfAnonymous();
		}
	}

	VariableReference* getTargetVariable() const
	{
		jassert(currentPass >= BaseCompiler::ResolvingSymbols);
		return dynamic_cast<VariableReference*>(getSubExpr(1).get());
	}

	TokenType assignmentType;
	bool isLocalDefinition = false;
};


Operations::TokenType Operations::VariableReference::isWriteAccess()
{
	if (auto as = dynamic_cast<Assignment*>(parent.get()))
	{
		if (as->getTargetVariable() == this)
			return as->assignmentType;
	}

	return JitTokens::void_;
}


struct Operations::Compare : public Expression
{
	Compare(Location location, Expression::Ptr l, Expression::Ptr r, TokenType op_) :
		Expression(location),
		op(op_)
	{
		type = Types::Integer;
		addSubExpression(l);
		addSubExpression(r);
	};

	Types::ID getType() const override { return Types::Integer; }

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			auto l = getSubExpr(0);
			auto r = getSubExpr(1);

			reg = compiler->getRegFromPool(getType());

			auto tReg = getSubRegister(0);
			auto value = getSubRegister(1);

			asg.emitCompare(op, reg, l->reg, r->reg);

			l->reg->flagForReuseIfAnonymous();
			r->reg->flagForReuseIfAnonymous();
		}
	}

	TokenType op;
};

struct Operations::LogicalNot : public Expression
{
	LogicalNot(Location l, Ptr expr) :
		Expression(l)
	{
		addSubExpression(expr);
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

struct Operations::TernaryOp : public Expression
{
public:

	TernaryOp(Location l, Expression::Ptr c, Expression::Ptr t, Expression::Ptr f) :
		Expression(l)
	{
		addSubExpression(c);
		addSubExpression(t);
		addSubExpression(f);
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

struct Operations::FunctionCall : public Expression
{
	FunctionCall(Location l, const Symbol& symbol_) :
		Expression(l),
		symbol(symbol_)
	{};

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::ResolvingSymbols)
		{
			auto eventSymbol = BaseScope::Symbol({}, symbol.parent, Types::ID::Event);

			if (auto eventScope = scope->getScopeForSymbol(eventSymbol))
			{
				VariableReference* object = new VariableReference(location, eventSymbol);
				addSubExpression(object, 0);
				object->process(compiler, scope);
				symbol.parent = "Message";
			}

			auto blockSymbol = BaseScope::Symbol({}, symbol.parent, Types::ID::Block);

			if (auto blockScope = scope->getScopeForSymbol(blockSymbol))
			{
				VariableReference* object = new VariableReference(location, blockSymbol);
				addSubExpression(object, 0);
				object->process(compiler, scope);
				symbol.parent = "Block";
			}

			if (auto fc = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{


				if (fc->hasFunction(symbol.parent, symbol.id))
				{
					fc->addMatchingFunctions(possibleMatches, symbol.parent, symbol.id);
					return;
				}
			}

			throwError("Can't find function " + symbol.toString());
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			Array<Types::ID> parameterTypes;

			for (int i = 0; i < getNumSubExpressions(); i++)
				parameterTypes.add(getSubExpr(i)->getType());

			for (auto f : possibleMatches)
			{
				if (f.matchesArgumentTypes(parameterTypes))
				{
					function = f;
					type = function.returnType;
					return;
				}
			}

			throwError("Wrong argument types for function call " + symbol.toString());
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (!function)
			{
				auto fClass = dynamic_cast<FunctionClass*>(findClassScope(scope));

				if (!fClass->fillJitFunctionPointer(function))
					throwError("Can't find function pointer to JIT function " + function.functionName);
			}

			auto asg = CREATE_ASM_COMPILER(type);

			for (int i = 0; i < getNumSubExpressions(); i++)
				parameterRegs.add(compiler->getRegFromPool(getSubExpr(i)->getType()));

			reg = compiler->getRegFromPool(type);

			for (int i = 0; i < getNumSubExpressions(); i++)
			{
				auto pType = getSubExpr(i)->getType();
				auto typedAcg = CREATE_ASM_COMPILER(pType);
				typedAcg.emitStore(parameterRegs[i], getSubExpr(i)->reg);
			}

			asg.emitFunctionCall(reg, function, parameterRegs);

			for (auto pRegs : parameterRegs)
				pRegs->flagForReuse();
		}
	}


	Symbol symbol;
	Array<FunctionData> possibleMatches;
	FunctionData function;

	ReferenceCountedArray<AssemblyRegister> parameterRegs;
};

struct Operations::ReturnStatement : public Expression
{
	ReturnStatement(Location l, Expression::Ptr expr) :
		Expression(l)
	{
		if (expr != nullptr)
			addSubExpression(expr);
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
		Statement::process(compiler, scope);

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
				reg = compiler->registerPool.getNextFreeRegister(type);
			}

			auto sourceReg = isVoid() ? nullptr : getSubRegister(0);

			asg.emitReturn(compiler, reg, sourceReg);
		}
	}
};


struct Operations::Function : public Statement,
	public asmjit::ErrorHandler
{
	Function(Location l) :
		Statement(l),
		code(nullptr)
	{};

	~Function()
	{
		data = {};
		functionScope = nullptr;
		statements = nullptr;
		parameters.clear();
	}

	bool handleError(asmjit::Error, const char* message, CodeEmitter*)
	{
		throwError(String(message));
		return true;
	}

	Types::ID getType() const override { return data.returnType; }

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	String::CharPointerType code;
	int codeLength = 0;


	ScopedPointer<FunctionScope> functionScope;
	ScopedPointer<SyntaxTree> statements;
	FunctionData data;
	Array<Identifier> parameters;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Function);
};

struct Operations::BinaryOp : public Expression
{
	BinaryOp(Location l, Expression::Ptr left, Expression::Ptr right, TokenType opType) :
		Expression(l),
		op(opType)
	{
		addSubExpression(left);
		addSubExpression(right);
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

				if (l->canBeReused())
				{
					reg = l;
				}
				{
					asg.emitComment("temp register for binary op");
					reg = compiler->getRegFromPool(getType());
					asg.emitStore(reg, getSubRegister(0));
				}

				asg.emitBinaryOp(op, reg, getSubRegister(1));
			}


		}
	}

	TokenType op;
};

struct Operations::UnaryOp : public Expression
{
	UnaryOp(Location l, Ptr expr) :
		Expression(l)
	{
		addSubExpression(expr);
	}

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Expression::process(compiler, scope);
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

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (dynamic_cast<Increment*>(getSubExpr(0).get()) != nullptr)
				throwError("Can't combine incrementors");

			if (getSubExpr(0)->getType() != Types::ID::Integer)
				throwError("Can't increment non integer variables.");
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
				reg = compiler->getRegFromPool(Types::ID::Integer);
				asg.emitIncrement(reg, getSubRegister(0), isPreInc, isDecrement);
			}
		}
	}

	bool isDecrement;
	bool isPreInc;
};

struct Operations::Cast : public Expression
{
	Cast(Location l, Expression::Ptr expression, Types::ID targetType) :
		Expression(l)
	{
		addSubExpression(expression);
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

			if (sourceType == targetType)
				replaceInParent(getSubExpr(0));
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			auto asg = CREATE_ASM_COMPILER(type);
			auto sourceType = getSubExpr(0)->getType();
			reg = compiler->getRegFromPool(type);
			asg.emitCast(reg, getSubRegister(0), sourceType);
		}
	}
};


struct Operations::BlockAccess : public Expression
{
	BlockAccess(Location l, Ptr target, Ptr index) :
		Expression(l)
	{
		addSubExpression(index);
		addSubExpression(target);

		type = Types::ID::Float;
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			if (getSubExpr(0)->getType() != Types::ID::Integer)
				throwError("subscription index must be an integer");

			if (getSubExpr(1)->getType() != Types::ID::Block)
				throwError("Can't use [] on primitive types");
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (auto cs = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{
				Array<FunctionData> matches;
				cs->addMatchingFunctions(matches, "Block", "getSample");

				reg = compiler->getRegFromPool(type);

				auto asg = CREATE_ASM_COMPILER(type);

				ReferenceCountedArray<AssemblyRegister> parameters;

				getSubRegister(0)->loadMemoryIntoRegister(asg.cc);

				parameters.add(getSubRegister(1));
				parameters.add(getSubRegister(0));

				asg.emitFunctionCall(reg, matches[0], parameters);
			}
		}
	}
};

struct Operations::BlockAssignment : public Expression
{
	BlockAssignment(Location l, Ptr target, TokenType opType, Ptr v) :
		Expression(l),
		op(opType)
	{

		addSubExpression(v);
		addSubExpression(target);
	}

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		// We need to move this before the base class process()
		// in order to prevent code generation for b[x]...
		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (auto cs = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{
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
				cs->addMatchingFunctions(matches, "Block", "setSample");


				asg.emitFunctionCall(nullptr, matches.getFirst(), parameters);
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
				replaceSubExpr(0, implicitCast);
			}
		}
	}

	TokenType op;
};


struct Operations::BlockLoop : public Expression
{
	BlockLoop(Location l, Identifier it_, Expression::Ptr t, Statement::Ptr b_) :
		Expression(l),
		iterator(it_)
	{
		addSubExpression(t);
		//addSubExpression(b_.get());

		if (dynamic_cast<StatementBlock*>(b_.get()) == nullptr)
		{
			b = new StatementBlock(b_->location);
			b->addStatement(b_);
		}
		else
			b = dynamic_cast<StatementBlock*>(b_.get());

		b->setParent(this);
		target = t;
	};

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Statement::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::ResolvingSymbols)
		{
			target->process(compiler, scope);

			b->blockScope = new RegisterScope(scope);
			b->blockScope->allocate(iterator, VariableStorage(0.0f));

			b->process(compiler, scope);
		}

		COMPILER_PASS(BaseCompiler::TypeCheck)
		{
			target->process(compiler, scope);
			b->process(compiler, scope);

			if (target->getType() != Types::ID::Block)
				throwError("Can't iterate over non-blocks");
		}

		COMPILER_PASS(BaseCompiler::RegisterAllocation)
		{
			target->process(compiler, scope);
			b->process(compiler, scope);
		}

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (auto cs = dynamic_cast<FunctionClass*>(findClassScope(scope)))
			{
				auto acg = CREATE_ASM_COMPILER(Types::ID::Float);

				target->process(compiler, scope);
				target->reg->loadMemoryIntoRegister(acg.cc);

				// getWritePointer
				auto wpReg = acg.cc.newIntPtr();

				{
					FuncSignatureX sig;
					sig.setRet(asmjit::TypeId::kIntPtr);
					sig.addArg(asmjit::TypeId::kIntPtr);

					// Push the function pointer
					auto fn = acg.cc.newIntPtr("fn");
					acg.cc.mov(fn, imm_ptr(BlockFunctions::Wrapper::getWritePointer));

					auto call = acg.cc.call(fn, sig);

					call->setInlineComment("getWritePointer");
					call->setArg(0, target->reg->getRegisterForReadOp());
					call->setRet(0, wpReg);
				}

				// size()
				auto endReg = acg.cc.newGpd();

				{
					FuncSignatureX sig;
					sig.setRet(asmjit::TypeId::kI32);

					// Push the function pointer
					auto fn = acg.cc.newIntPtr("fn");
					acg.cc.mov(fn, imm_ptr(BlockFunctions::Wrapper::size));

					CCFuncCall* call = acg.cc.call(fn, sig);

					call->setInlineComment("size()");
					call->setRet(0, endReg);
				}

				BaseScope::RefPtr r = b->blockScope->get({ {}, iterator, Types::ID::Float });

				auto itReg = compiler->registerPool.getRegisterForVariable(r);
				itReg->createRegister(acg.cc);
				itReg->setIsIteratorRegister(true);

				auto& cc = getFunctionCompiler(compiler);

				auto loopStart = cc.newLabel();

				cc.setInlineComment("loop_block {");
				cc.bind(loopStart);

				auto adress = x86::qword_ptr(wpReg);

				cc.movss(itReg->getRegisterForWriteOp().as<X86Xmm>(), adress);

				b->process(compiler, scope);

				cc.movss(adress, itReg->getRegisterForReadOp().as<X86Xmm>());
				cc.add(wpReg, 4);
				cc.dec(endReg);
				cc.setInlineComment("loop_block }");
				cc.jnz(loopStart);
			}
		}
	}

	RegPtr iteratorRegister;
	Identifier iterator;
	Expression::Ptr target;
	ReferenceCountedObjectPtr<StatementBlock> b;
};


struct Operations::Negation : public Expression
{
	Negation(Location l, Expression::Ptr e) :
		Expression(l)
	{
		addSubExpression(e);
	};

	void process(BaseCompiler* compiler, BaseScope* scope)
	{
		Expression::process(compiler, scope);

		COMPILER_PASS(BaseCompiler::CodeGeneration)
		{
			if (!isConstExpr())
			{
				auto asg = CREATE_ASM_COMPILER(getType());

				reg = compiler->getRegFromPool(getType());

				asg.emitNegation(reg, getSubRegister(0));

				getSubRegister(0)->flagForReuseIfAnonymous();
			}
		}
	}
};

}
}