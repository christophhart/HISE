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

#define IF_(typeName) if(type == Types::Helpers::getTypeFromTypeId<typeName>())
#define FP_REG_W(x) x->getRegisterForWriteOp().as<X86Xmm>()
#define FP_REG_R(x) x->getRegisterForReadOp().as<X86Xmm>()
#define FP_MEM(x) x->getAsMemoryLocation()
#define IS_MEM(x) x->isMemoryLocation()
#define FP_OP(op, l, r) { if(IS_MEM(r)) op(FP_REG_W(l), FP_MEM(r)); else op(FP_REG_W(l), FP_REG_R(r)); }


#define INT_REG_W(x) x->getRegisterForWriteOp().as<X86Gp>()
#define INT_REG_R(x) x->getRegisterForReadOp().as<X86Gp>()
#define INT_MEM(x) x->getImmediateIntValue()

#define INT_OP(op, l, r) { if(IS_MEM(r)) op(INT_REG_W(l), (uint64_t)INT_MEM(r)); else op(INT_REG_W(l), INT_REG_R(r)); }

AsmCodeGenerator::AsmCodeGenerator(Compiler& cc_, Types::ID type_) :
	cc(cc_),
	type(type_)
{

}

void AsmCodeGenerator::emitComment(const char* m)
{
	cc.setInlineComment(m);
}

void AsmCodeGenerator::emitStore(RegPtr target, RegPtr value)
{
	target->createRegister(cc);

	IF_(void) jassertfalse;
	IF_(float)  FP_OP(cc.movss, target, value);
	IF_(double) FP_OP(cc.movsd, target, value);
	IF_(int)   INT_OP(cc.mov,   target, value);
	IF_(HiseEvent) INT_OP(cc.mov, target, value);
	IF_(block)
    {
        value->createRegister(cc);
        cc.mov(INT_REG_W(target), INT_REG_R(value));
        
    }
}


void AsmCodeGenerator::emitMemoryWrite(RegPtr source)
{
	type = source->getType();

	cc.setInlineComment("Write class variable");

    auto data = source->getGlobalDataPointer();
    
#if JUCE_64BIT
	X86Gp address = cc.newGpq();
	cc.mov(address, reinterpret_cast<uint64_t>(data));
	auto target = x86::qword_ptr(address);
#else
	auto target = x86::dword_ptr(reinterpret_cast<uint64_t>(data));
#endif

	IF_(int)	cc.mov  (target, source->getRegisterForReadOp().as<X86Gp>());
	IF_(float)	cc.movss(target, source->getRegisterForReadOp().as<X86Xmm>());
	IF_(double) cc.movsd(target, source->getRegisterForReadOp().as<X86Xmm>());
}


void AsmCodeGenerator::emitMemoryLoad(RegPtr target)
{
	type = target->getType();

	cc.setInlineComment("Load class variable");

	auto data = target->getGlobalDataPointer();

#if JUCE_64BIT
	X86Gp address = cc.newGpq();
	cc.mov(address, reinterpret_cast<uint64_t>(data));
	auto source = x86::qword_ptr(address);
#else
	auto source = x86::dword_ptr(reinterpret_cast<uint64_t>(data));
#endif

	// We use the REG_R ones to avoid flagging it dirty
	IF_(int)	cc.mov(INT_REG_R(target), source);
	IF_(float)	cc.movss(FP_REG_R(target), source);
	IF_(double) cc.movsd(FP_REG_R(target), source);
}

#if 0
AsmCodeGenerator::RegPtr AsmCodeGenerator::emitImmediate(VariableStorage value)
{
	jassert(type == value.getType());
	RegPtr newReg = new AssemblyRegister(type);
	emitImmediate(newReg, value);
	return newReg;
}


void AsmCodeGenerator::emitImmediate(RegPtr target, VariableStorage value)
{
	IF_(float)	target->setImmediateFloatingPointValue(cc, value);
	IF_(double)	target->setImmediateFloatingPointValue(cc, value);
	IF_(int)	target->setImmediateIntValue(static_cast<int>(value));
}
#endif

#if 0
AsmCodeGenerator::RegPtr AsmCodeGenerator::emitParameter(int parameterIndex)
{
	asmjit::Error error;
	RegPtr reg = new AssemblyRegister(type);

	IF_(double)
	{
		reg->reg = cc.newXmmSs("Parameter Register");
		cc.setArg(parameterIndex, reg->reg);
	}
	IF_(float)
	{
		reg->reg = cc.newXmmSd();
		cc.setArg(parameterIndex, reg->reg);
	}
	IF_(int)
	{
		reg->reg = cc.newGpd();
		cc.setArg(parameterIndex, reg->reg);
	}

	return reg;
}
#endif


void AsmCodeGenerator::emitParameter(RegPtr parameterRegister, int parameterIndex)
{
	parameterRegister->createRegister(cc);
	cc.setArg(parameterIndex, parameterRegister->getRegisterForReadOp());
}

#define BINARY_OP(token, intOp, floatOp, doubleOp) if(op == token) { IF_(int) INT_OP(intOp, l, r); IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); return l; }

AsmCodeGenerator::RegPtr AsmCodeGenerator::emitBinaryOp(OpType op, RegPtr l, RegPtr r)
{
	if (type == Types::ID::Integer && (op == JitTokens::modulo || op == JitTokens::divide))
	{
		X86Gp dummy = cc.newInt32("dummy");

		cc.xor_(dummy, dummy);
		cc.cdq(dummy, INT_REG_W(l));

		if (r->isMemoryLocation())
		{
			auto forcedMemory = cc.newInt32Const(kConstScopeLocal, static_cast<int>(INT_MEM(r)));
			cc.idiv(dummy, INT_REG_W(l), forcedMemory);
		}
		else					   cc.idiv(dummy, INT_REG_W(l), INT_REG_R(r));
		
		if (op == JitTokens::modulo)
			cc.mov(INT_REG_W(l), dummy);

		return l;
	}

	BINARY_OP(JitTokens::plus,   cc.add, cc.addss, cc.addsd);
	BINARY_OP(JitTokens::minus,  cc.sub, cc.subss, cc.subsd);
	BINARY_OP(JitTokens::times,  cc.imul, cc.mulss, cc.mulsd);
	BINARY_OP(JitTokens::divide, cc.idiv, cc.divss, cc.divsd);

	return l;
}

void AsmCodeGenerator::emitLogicOp(Operations::BinaryOp* op)
{
	auto lExpr = op->getSubExpr(0);
	lExpr->process(op->currentCompiler, op->currentScope);
	auto l = lExpr->reg;

	auto shortCircuit = cc.newLabel();
	
	l->loadMemoryIntoRegister(cc);

	int shortCircuitValue = (op->op == JitTokens::logicalAnd) ? 1 : 0;

	cc.test(INT_REG_W(lExpr->reg), shortCircuitValue);
	cc.je(shortCircuit);

	auto rExpr = op->getSubExpr(1);

	rExpr->process(op->currentCompiler, op->currentScope);
	auto r = rExpr->reg;
	r->loadMemoryIntoRegister(cc);

	cc.and_(INT_REG_W(r), 1);

	if (op->op == JitTokens::logicalAnd)
	{
		INT_OP(cc.and_, l, r);
	}
	else
	{
		INT_OP(cc.or_, l, r);
	}

	auto end = cc.newLabel();

	cc.jmp(end);
	cc.bind(shortCircuit);
	op->reg = op->currentCompiler->getRegFromPool(Types::ID::Integer);
	op->reg->createRegister(cc);
	INT_OP(cc.mov, op->reg, l);
	cc.bind(end);
}



void AsmCodeGenerator::emitCompare(OpType op, RegPtr target, RegPtr l, RegPtr r)
{
	type = l->getType();

	l->loadMemoryIntoRegister(cc);
	target->createRegister(cc);

	IF_(int)
	{
		INT_OP(cc.cmp, l, r);

#define INT_COMPARE(token, command) if (op == token) command(INT_REG_W(target));

		INT_COMPARE(JitTokens::greaterThan, cc.setg);
		INT_COMPARE(JitTokens::lessThan, cc.setl);
		INT_COMPARE(JitTokens::lessThanOrEqual, cc.setle);
		INT_COMPARE(JitTokens::greaterThanOrEqual, cc.setge);
		INT_COMPARE(JitTokens::equals, cc.sete);
		INT_COMPARE(JitTokens::notEquals, cc.setne);

#undef INT_COMPARE

		cc.and_(INT_REG_W(target), 1);

		return;
	}
	IF_(float) FP_OP(cc.ucomiss, l, r);
	IF_(double) FP_OP(cc.ucomisd, l, r);

#define FLOAT_COMPARE(token, command) if (op == token) command(INT_REG_R(target), condReg);

	auto condReg = cc.newGpq();
	
	cc.mov(INT_REG_W(target), 0);
	cc.mov(condReg, 1);

	FLOAT_COMPARE(JitTokens::greaterThan, cc.cmova);
	FLOAT_COMPARE(JitTokens::lessThan, cc.cmovb);
	FLOAT_COMPARE(JitTokens::lessThanOrEqual, cc.cmovbe);
	FLOAT_COMPARE(JitTokens::greaterThanOrEqual, cc.cmovae);
	FLOAT_COMPARE(JitTokens::equals, cc.cmove);
	FLOAT_COMPARE(JitTokens::notEquals, cc.cmovne);

#undef FLOAT_COMPARE
	
}

void AsmCodeGenerator::emitReturn(BaseCompiler* c, RegPtr target, RegPtr expr)
{
	// Store the return value before writing the globals

	RegPtr rToUse;

	if (expr != nullptr && (expr->isMemoryLocation() || expr->isDirtyGlobalMemory()))
	{
		emitStore(target, expr);
		rToUse = target;
	}
	else
		rToUse = expr;

	auto dirtyGlobalList = c->registerPool.getListOfAllDirtyGlobals();

	for (auto reg : dirtyGlobalList)
	{
		emitMemoryWrite(reg);
	}

	dirtyGlobalList.clear();

	if (expr == nullptr || target == nullptr)
		cc.ret();
	else
	{
		if (type == Types::ID::Float || type == Types::ID::Double)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Xmm>());
		else if (type == Types::ID::Integer)
			cc.ret(rToUse->getRegisterForWriteOp().as<AssemblyRegister::IntRegisterType>());
		else if (type == Types::ID::Event)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Gpq>());
		else if (type == Types::ID::Block)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Gpq>());
	}
}


AsmCodeGenerator::RegPtr AsmCodeGenerator::emitTernaryOp(Operations::TernaryOp* tOp, BaseCompiler* c, BaseScope* s)
{
	auto returnReg = c->registerPool.getNextFreeRegister(tOp->getType());

	auto condition = tOp->getSubExpr(0);
	auto trueBranch = tOp->getSubExpr(1);
	auto falseBranch = tOp->getSubExpr(2);

	returnReg->createRegister(cc);
	auto vv = returnReg->getRegisterForWriteOp();

	

	auto l = cc.newLabel();
	auto e = cc.newLabel();

	// emit the code for the condition here
	condition->process(c, s);

	if (condition->reg->isMemoryLocation())
	{
		auto dummy = cc.newInt32();
		cc.mov(dummy, (uint64_t)condition->reg->getImmediateIntValue());
		cc.cmp(dummy, 0);
	}
	else
		cc.cmp(INT_REG_R(condition->reg), 0);

	cc.jnz(l);

	condition->reg->flagForReuseIfAnonymous();
	
	cc.setInlineComment("false branch");
	falseBranch->process(c, s);
	emitStore(returnReg, falseBranch->reg);
	falseBranch->reg->flagForReuseIfAnonymous();
	cc.jmp(e);
	cc.bind(l);
	cc.setInlineComment("true branch");
	trueBranch->process(c, s);
	emitStore(returnReg, trueBranch->reg);
	trueBranch->reg->flagForReuseIfAnonymous();
	cc.bind(e);

	return returnReg;
}


AsmCodeGenerator::RegPtr AsmCodeGenerator::emitLogicalNot(RegPtr expr)
{
	expr->loadMemoryIntoRegister(cc);
	cc.xor_(expr->getRegisterForWriteOp().as<X86Gp>(), 1);
	return expr;
}


void AsmCodeGenerator::emitIncrement(RegPtr target, RegPtr expr, bool isPre, bool isDecrement)
{
	if (isPre)
	{
		expr->loadMemoryIntoRegister(cc);

		if (isDecrement)
			cc.add(INT_REG_W(expr), -1);
		else
			cc.add(INT_REG_W(expr), 1);
	}
	else
	{
		jassert(target != nullptr);

		emitStore(target, expr);

		if (isDecrement)
			cc.add(INT_REG_W(expr), -1);
		else
			cc.add(INT_REG_W(expr), 1);
	}
}

void AsmCodeGenerator::emitCast(RegPtr target, RegPtr expr, Types::ID sourceType)
{
	target->createRegister(cc);

	IF_(int) // TARGET TYPE
	{
		type = sourceType;

		IF_(float) // SOURCE TYPE
		{
			if(IS_MEM(expr)) cc.cvttss2si(INT_REG_W(target), FP_MEM(expr));
			else			 cc.cvttss2si(INT_REG_W(target), FP_REG_R(expr));
		}
		IF_(double) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvttsd2si(INT_REG_W(target), FP_MEM(expr));
			else			  cc.cvttsd2si(INT_REG_W(target), FP_REG_R(expr));
		}

		return;
	}
	IF_(double) // TARGET TYPE
	{
		type = sourceType;

		IF_(float) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvtss2sd(FP_REG_W(target), FP_MEM(expr));
			else			  cc.cvtss2sd(FP_REG_W(target), FP_REG_R(expr));
		}
		IF_(int) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvtsi2sd(FP_REG_W(target), INT_MEM(expr));
			else			  cc.cvtsi2sd(FP_REG_W(target), INT_REG_R(expr));
		}

		return ;
	}
	IF_(float) // TARGET TYPE
	{
		type = sourceType;

		IF_(double) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvtsd2ss(FP_REG_W(target), FP_MEM(expr));
			else			  cc.cvtsd2ss(FP_REG_W(target), FP_REG_R(expr));
		}
		IF_(int) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvtsi2ss(FP_REG_W(target), INT_MEM(expr));
			else			  cc.cvtsi2ss(FP_REG_W(target), INT_REG_R(expr));
		}

		return;
	}
}


void AsmCodeGenerator::emitNegation(RegPtr target, RegPtr expr)
{
	emitStore(target, expr);

	IF_(int)
		cc.neg(INT_REG_W(target));
	IF_(float)
		cc.mulss(FP_REG_W(target), cc.newFloatConst(kConstScopeLocal, -1.0f));
	IF_(float)
		cc.mulsd(FP_REG_W(target), cc.newDoubleConst(kConstScopeLocal, -1.0));
}


void AsmCodeGenerator::emitFunctionCall(RegPtr returnReg, const FunctionData& f, ReferenceCountedArray<AssemblyRegister>& parameterRegisters)
{
	FuncSignatureX sig;

	bool isMemberFunction = f.object != nullptr;
	fillSignature(f, sig, isMemberFunction);

	X86Gp o;

	if (isMemberFunction)
	{
		o = cc.newIntPtr("obj");
		cc.mov(o, imm_ptr(f.object));
	}

	// Push the function pointer
	X86Gp fn = cc.newIntPtr("fn");
	cc.mov(fn, imm_ptr(f.function));

	CCFuncCall* call = cc.call(fn, sig);

	call->setInlineComment(f.functionName.getCharPointer().getAddress());

	int offset = 0;

	if (isMemberFunction)
	{
		call->setArg(0, o);
		offset = 1;
	}

	for (int i = 0; i < parameterRegisters.size(); i++)
	{
		call->setArg(i + offset, parameterRegisters[i]->getRegisterForReadOp());
	}
	
	if (f.returnType != Types::ID::Void)
	{
		returnReg->createRegister(cc);
		call->setRet(0, returnReg->getRegisterForWriteOp());
	}
}


void AsmCodeGenerator::fillSignature(const FunctionData& data, FuncSignatureX& sig, bool createObjectPointer)
{

	if (data.returnType == Types::ID::Float) sig.setRetT<float>();
	if (data.returnType == Types::ID::Double) sig.setRetT<double>();
	if (data.returnType == Types::ID::Integer) sig.setRetT<uint64_t>();
	if (data.returnType == Types::ID::Event) sig.setRet(TypeId::kIntPtr);
	if (data.returnType == Types::ID::Block) sig.setRet(TypeId::kIntPtr);

	if (createObjectPointer)
		sig.addArgT<PointerType>();

	for (auto p : data.args)
	{
		if (p == Types::ID::Float)	 sig.addArgT<float>();
		if (p == Types::ID::Double)  sig.addArgT<double>();
		if (p == Types::ID::Integer) sig.addArgT<int>();
		if (p == Types::ID::Event)   sig.addArg(TypeId::kIntPtr);
		if (p == Types::ID::Block)   sig.addArg(TypeId::kIntPtr);
	}
}

struct VOps
{
#define VAR_OP(name, opChar) static VariableStorage name(VariableStorage l, VariableStorage r) { return VariableStorage(l.getType(), l.toDouble() opChar r.toDouble()); }

	VAR_OP(sum, +);
	VAR_OP(mul, *);
	VAR_OP(div, /);
	VAR_OP(sub, -);

#undef VAR_OP
};



snex::VariableStorage ConstExprEvaluator::binaryOp(TokenType t, VariableStorage left, VariableStorage right)
{
	if (t == JitTokens::plus || t == JitTokens::plusEquals) return VOps::sum(left, right);
	if (t == JitTokens::minus || t == JitTokens::minusEquals) return VOps::sub(left, right);
	if (t == JitTokens::times || t == JitTokens::timesEquals) return VOps::mul(left, right);
	if (t == JitTokens::divide || t == JitTokens::divideEquals) return VOps::div(left, right);

	return left;
}

Result ConstExprEvaluator::process(SyntaxTree* tree)
{
	jassertfalse;

	for (auto s : *tree)
	{
		if (auto v = as<Operations::VariableReference>(s))
		{
			// Keep the first reference to a const variable alive
			// so that the parent assignment can do it's job
			bool isConstInitialisation = v->ref->isConst && v->isFirstReference();

			if (!v->isClassVariable && v->ref->isConst && !isConstInitialisation)
				replaceWithImmediate(v, v->ref->getDataCopy());
		}
		if (auto a = as<Operations::Assignment>(s))
		{
			auto target = a->getTargetVariable();

			if (target->isLocalToScope && a->isLastAssignmentToTarget() && target->isFirstReference() && a->getSubExpr(1)->isConstExpr())
				target->ref->isConst = true;

			if (a->assignmentType == JitTokens::assign_ &&
				a->getSubExpr(1)->isConstExpr())
			{
				bool allowInitialisation = !target->isLocalConst ||
					target->isFirstReference();

				auto& value = target->ref->getDataReference(allowInitialisation);
				value = dynamic_cast<Operations::Immediate*>(a->getSubExpr(1).get())->v;
			}
		}
		if (auto c = as<Operations::Cast>(s))
		{
			if (auto ce = evalCast(c->getSubExpr(0), c->getType()))
				c->replaceInParent(ce);
		}
		if (auto b = as<Operations::BinaryOp>(s))
		{
			if (auto ce = evalBinaryOp(b->getSubExpr(0), b->getSubExpr(1), b->op))
				b->replaceInParent(ce);
			else
			{
				if (b->getSubExpr(1)->isConstExpr())
				{
					ExprPtr old = b->getSubExpr(1);

					ExprPtr newPtr = ConstExprEvaluator::createInvertImmediate(b->getSubExpr(1), b->op);

					if (old != newPtr)
					{
						auto oldOp = b->op;
						old->replaceInParent(newPtr);

						if (b->op == JitTokens::divide) b->op = JitTokens::times;
						if (b->op == JitTokens::minus) b->op = JitTokens::plus;
						String m;
						m << "Replaced " << oldOp << " with " << b->op;
						b->logOptimisationMessage(m);
					}

				}
			}
		}
		if (auto n = as<Operations::Negation>(s))
		{
			if (auto ce = evalNegation(n->getSubExpr(0)))
				n->replaceInParent(ce);
		}
	}

	return Result::ok();
}


void ConstExprEvaluator::replaceWithImmediate(ExprPtr e, const VariableStorage& value)
{
	jassertfalse;

	e->replaceInParent(new Operations::Immediate(e->location, value));
}

snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalBinaryOp(ExprPtr left, ExprPtr right, OpType op)
{
	if (left->isConstExpr() && right->isConstExpr())
	{
		VariableStorage result;
		VariableStorage leftValue = left->getConstExprValue();
		VariableStorage rightValue = right->getConstExprValue();

		if (op == JitTokens::plus) result = VOps::sum(leftValue, rightValue);
		if (op == JitTokens::minus) result = VOps::sub(leftValue, rightValue);
		if (op == JitTokens::times) result = VOps::mul(leftValue, rightValue);
		if (op == JitTokens::divide) result = VOps::div(leftValue, rightValue);

		return new Operations::Immediate(left->location, result);
	}

	return nullptr;
}


snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalNegation(ExprPtr expr)
{
	if (expr->isConstExpr())
	{
		auto result = VOps::mul(expr->getConstExprValue(), -1.0);
		return new Operations::Immediate(expr->location, result);
	}

	return nullptr;
}



ConstExprEvaluator::ExprPtr ConstExprEvaluator::evalCast(ExprPtr expression, Types::ID targetType)
{
	if (expression->isConstExpr())
	{
		auto value = expression->getConstExprValue();
		return new Operations::Immediate(expression->location, VariableStorage(targetType, value.toDouble()));
	}

	return nullptr;
}


snex::jit::ConstExprEvaluator::ExprPtr ConstExprEvaluator::createInvertImmediate(ExprPtr immediate, OpType op)
{
	auto v = immediate->getConstExprValue().toDouble();

	if (op == JitTokens::minus) v *= -1.0;
	else if (op == JitTokens::divide) v = 1.0 / v;
	else return immediate;

	return new Operations::Immediate(immediate->location, VariableStorage(immediate->getType(), v));
}

}

#undef IS

}
