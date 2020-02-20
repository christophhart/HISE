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
#define IS_CMEM(x) x->hasCustomMemoryLocation()
#define IS_REG(x)  x->isActive()


#define INT_REG_W(x) x->getRegisterForWriteOp().as<X86Gp>()
#define INT_REG_R(x) x->getRegisterForReadOp().as<X86Gp>()
#define INT_IMM(x) x->getImmediateIntValue()
#define INT_MEM(x) x->getAsMemoryLocation()



#define INT_OP_WITH_MEM(op, l, r) { if(IS_MEM(r)) op(INT_REG_W(l), INT_MEM(r)); else op(INT_REG_W(l), INT_REG_R(r)); }


#define FP_OP(op, l, r) { if(IS_REG(r)) op(FP_REG_W(l), FP_REG_R(r)); \
					 else if(IS_MEM(r)) op(FP_REG_W(l), FP_MEM(r)); \
					               else op(FP_REG_W(l), FP_REG_R(r)); }

#define INT_OP(op, l, r) { if(IS_REG(r))    op(INT_REG_W(l), INT_REG_R(r)); \
					  else if(IS_CMEM(r))   op(INT_REG_W(l), INT_MEM(r)); \
					  else if(IS_MEM(r))    op(INT_REG_W(l), static_cast<int>(INT_IMM(r))); \
					  else op(INT_REG_W(l), INT_REG_R(r)); }

AsmCodeGenerator::AsmCodeGenerator(Compiler& cc_, AssemblyRegisterPool* pool, Types::ID type_) :
	cc(cc_),
	type(type_),
	registerPool(pool)
{
	
}

void AsmCodeGenerator::emitComment(const char* m)
{
	cc.setInlineComment(m);
}

void AsmCodeGenerator::emitStore(RegPtr target, RegPtr value)
{
	if (target->hasCustomMemoryLocation())
	{
		value->loadMemoryIntoRegister(cc);

		IF_(int)		cc.mov(INT_MEM(target), INT_REG_R(value));
		IF_(float)		cc.movss(FP_MEM(target), FP_REG_R(value));
		IF_(double)		cc.movsd(FP_MEM(target), FP_REG_R(value));
		IF_(HiseEvent)  cc.mov(INT_MEM(target), INT_REG_R(value));
	}
	else
	{
		target->createRegister(cc);

		IF_(void) jassertfalse;
		IF_(float)  FP_OP(cc.movss, target, value);
		IF_(double) FP_OP(cc.movsd, target, value);
		IF_(int)   INT_OP(cc.mov, target, value);
		IF_(HiseEvent) INT_OP(cc.mov, target, value);
		IF_(block)
		{
			value->createRegister(cc);
			cc.mov(INT_REG_W(target), INT_REG_R(value));
		}
	}
	
	IF_(void*)
	{
		jassertfalse;
#if 0
		target->createRegister(cc);

		X86Mem ptr;

		if (value->isMemoryLocation())
			cc.mov(INT_REG_R(target), (uint64_t)value->getGlobalDataPointer());
		else
			cc.mov(INT_REG_R(target), INT_REG_R(value));
#endif
	}
	
}


void AsmCodeGenerator::emitMemoryWrite(RegPtr source, void* ptrToUse)
{
	ScopedTypeSetter st(*this, source->getType());

	X86Mem target;

	cc.setInlineComment("Write class variable ");

	int ok;

	if (source->hasCustomMemoryLocation())
	{
		target = source->getMemoryLocationForReference();
	}
	else
	{
		auto data = ptrToUse != nullptr ? ptrToUse : source->getGlobalDataPointer();
        
        x86::Gp r;
        cc.mov(r, void2ptr(data));
        
		target = x86::qword_ptr(r);
	}

	IF_(int)	ok = cc.mov(target, INT_REG_R(source));
	IF_(float)	ok = cc.movss(target, FP_REG_R(source));
	IF_(double) ok = cc.movsd(target, FP_REG_R(source));

	// Don't undirty it when debugging...
	if (ptrToUse == nullptr)
		source->setUndirty();
}




void AsmCodeGenerator::emitMemoryLoad(RegPtr target)
{
	ScopedTypeSetter st(*this, target->getType());

	auto data = target->getGlobalDataPointer();

	IF_(block)
	{
		cc.setInlineComment("Load buffer pointer");
		cc.mov(target->getRegisterForWriteOp().as<X86Gp>(), void2ptr(data));

		return;
	}

	static char loadComment[128];
	memset(loadComment, 0, 40);
	auto name = "load " + target->getVariableId().toString();
	memcpy(loadComment, name.getCharPointer().getAddress(), name.length());

	cc.setInlineComment(loadComment);

	IF_(void*)
	{
		cc.mov(INT_REG_R(target), void2ptr(data));
		return;
	}

    x86::Gpq r;
    cc.mov(r, void2ptr(data));
    auto source = x86::ptr(r);
    
	// We use the REG_R ones to avoid flagging it dirty
	IF_(int)	cc.mov(INT_REG_R(target), source);
	IF_(float)	cc.movss(FP_REG_R(target), source);
	IF_(double) cc.movsd(FP_REG_R(target), source);
}

void AsmCodeGenerator::emitThisMemberAccess(RegPtr target, RegPtr parent, VariableStorage memberOffset)
{
	jassert(memberOffset.getType() == Types::ID::Integer);
	jassert(target->getType() == type);
	jassert(parent->getType() == Types::ID::Pointer);


	if (parent->isMemoryLocation())
	{
		auto ptr = parent->getAsMemoryLocation();
		target->setCustomMemoryLocation(ptr.cloneAdjusted(memberOffset.toInt()));
	}
	else
	{
		auto ptr = x86::ptr(INT_REG_R(parent), memberOffset.toInt());
		target->setCustomMemoryLocation(ptr);
	}
}

void AsmCodeGenerator::emitMemberAcess(RegPtr target, RegPtr parent, RegPtr child)
{
	jassert(target->getType() == type);					// the target register with the final type
	jassert(child->getType() == Types::ID::Integer);	// the index register containing the member offset in pointer sizes
	jassert(parent->getType() == Types::ID::Pointer);   // the pointer register containing the data slot pointer.

	// the child is always an immediate...
	jassert(child->isMemoryLocation() && !child->hasCustomMemoryLocation());

	auto immValue = VariableStorage((int)INT_IMM(child));

	emitThisMemberAccess(target, parent, immValue);

}

void AsmCodeGenerator::emitParameter(Operations::Function* f, RegPtr parameterRegister, int parameterIndex)
{
	if (f->hasObjectPtr)
		parameterIndex += 1;

	parameterRegister->createRegister(cc);
	cc.setArg(parameterIndex, parameterRegister->getRegisterForReadOp());
}

#define FLOAT_BINARY_OP(token, floatOp, doubleOp) if(op == token) { IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

#define BINARY_OP(token, intOp, floatOp, doubleOp) if(op == token) { IF_(int) INT_OP(intOp, l, r); IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

AsmCodeGenerator::RegPtr AsmCodeGenerator::emitBinaryOp(OpType op, RegPtr l, RegPtr r)
{
	jassert(l != nullptr && r != nullptr);
	jassert(l->getType() != Types::ID::Pointer);

	TemporaryRegister tempAddressRegister(*this, r->getScope(), r->getType());
	RegPtr ptrRegister;

	l->loadMemoryIntoRegister(cc);
	

	if (type == Types::ID::Integer && (op == JitTokens::modulo || op == JitTokens::divide))
	{
		TemporaryRegister dummy(*this, r->getScope(), Types::ID::Integer);
		
		//cc.xor_(dummy.get(), dummy.get());
		
		cc.cdq(dummy.get(), INT_REG_W(l));

		if (r->isMemoryLocation())
		{
			auto forcedMemory = cc.newInt32Const(ConstPool::kScopeLocal, static_cast<int>(INT_IMM(r)));
			cc.idiv(dummy.get(), INT_REG_W(l), forcedMemory);
		}
		else					   
			cc.idiv(dummy.get(), INT_REG_W(l), INT_REG_R(r));

		if (op == JitTokens::modulo)
			cc.mov(INT_REG_W(l), dummy.get());

		return l;
	}

	BINARY_OP(JitTokens::plus, cc.add, cc.addss, cc.addsd);
	BINARY_OP(JitTokens::minus, cc.sub, cc.subss, cc.subsd);
	BINARY_OP(JitTokens::times, cc.imul, cc.mulss, cc.mulsd);
	FLOAT_BINARY_OP(JitTokens::divide, cc.divss, cc.divsd);

	if (ptrRegister != nullptr)
	{
		writeToPointerAddress(ptrRegister, l);
		l = ptrRegister;
	}

	return l;
}

void AsmCodeGenerator::emitLogicOp(Operations::BinaryOp* op)
{
	auto lExpr = op->getSubExpr(0);
	lExpr->process(op->currentCompiler, op->currentScope);
	auto l = lExpr->reg;

	
	

	l->loadMemoryIntoRegister(cc);

	auto shortCircuit = cc.newLabel();
	int shortCircuitValue = (op->op == JitTokens::logicalAnd) ? 0 : 1;


	cc.cmp(INT_REG_W(lExpr->reg), shortCircuitValue);
	cc.setInlineComment("short circuit test");
	cc.je(shortCircuit);

	

	auto rExpr = op->getSubExpr(1);

	rExpr->process(op->currentCompiler, op->currentScope);
	auto r = rExpr->reg;
	r->loadMemoryIntoRegister(cc);

	//cc.and_(INT_REG_W(r), 1);

	if (op->op == JitTokens::logicalAnd)
	{
		INT_OP_WITH_MEM(cc.and_, l, r);
	}
	else
	{
		INT_OP_WITH_MEM(cc.or_, l, r);
	}

	cc.setInlineComment("Short circuit path");
	cc.bind(shortCircuit);

	op->reg = op->currentCompiler->getRegFromPool(op->currentScope, Types::ID::Integer);
	op->reg->createRegister(cc);
	INT_OP_WITH_MEM(cc.mov, op->reg, l);
	

#if 0
	auto end = cc.newLabel();

	cc.jmp(end);
	cc.setInlineComment("Short circuit path");
	cc.bind(shortCircuit);
	op->reg = op->currentCompiler->getRegFromPool(Types::ID::Integer);
	op->reg->createRegister(cc);
	INT_OP_WITH_MEM(cc.mov, op->reg, l);
	cc.bind(end);
#endif
}



void AsmCodeGenerator::emitSpanReference(RegPtr target, RegPtr address, RegPtr index, size_t elementSizeInBytes)
{
	jassert(index->getType() == Types::ID::Integer);

	if (address->getType() == Types::ID::Block)
	{
		address->loadMemoryIntoRegister(cc);
		auto blockAddress = INT_REG_R(address);

		auto bptr = x86::ptr(blockAddress).cloneAdjusted(8);

		TemporaryRegister fptr(*this, target->getScope(), Types::ID::Pointer);
		cc.mov(fptr.get(), bptr);

		X86Mem ptr;

		if (index->isMemoryLocation())
			ptr = x86::ptr(fptr.get(), index->getImmediateIntValue() * sizeof(float));
		else
			ptr = x86::ptr(fptr.get(), INT_REG_R(index), sizeof(float));

		target->setCustomMemoryLocation(ptr);
	}
	else
	{
		jassert(address->getType() == Types::ID::Pointer);
		jassert(index->getType() == Types::ID::Integer);
		jassert(address->getType() == Types::ID::Pointer || address->isMemoryLocation());

		int shift = (int)log2(elementSizeInBytes);
		bool canUseShift = isPowerOfTwo(elementSizeInBytes) && shift < 4;

		x86::Gpd indexReg;

		if (!index->isMemoryLocation())
		{
			indexReg = index->getRegisterForReadOp().as<x86::Gpd>();

			if (index->getVariableId())
			{
				auto newIndexReg = cc.newGpd().as<x86::Gpd>();
				cc.mov(newIndexReg, indexReg);
				indexReg = newIndexReg;
			}

			if (!canUseShift)
			{
				if (isPowerOfTwo(elementSizeInBytes))
					cc.sal(indexReg, shift);
				else
					cc.imul(indexReg, (uint64_t)elementSizeInBytes);

				shift = 0;
			}
		}

		X86Mem p;

		if (address->isMemoryLocation())
		{
			auto ptr = address->getAsMemoryLocation();

			if (index->isMemoryLocation())
				p = ptr.cloneAdjusted(imm2ptr(INT_IMM(index) * elementSizeInBytes));
			else
			{
				// for some reason uint64_t base + index reg doesn't create a 64bit address...
				// so we need to use a register for the base
				auto b_ = cc.newGpq();

				if (ptr.hasBaseReg())
					cc.mov(b_, ptr.baseReg().as<X86Gpq>());
				else
					cc.mov(b_, ptr.offset());

				p = x86::ptr(b_, indexReg, shift);
			}
		}
		else
		{
			if (index->isMemoryLocation())
				p = x86::ptr(INT_REG_R(address), imm2ptr(INT_IMM(index) * elementSizeInBytes));
			else
			{
				p = x86::ptr(INT_REG_R(address), indexReg, shift);
			}
		}

		if (type == Types::ID::Pointer)
		{
			target->createRegister(cc);
			cc.lea(INT_REG_W(target), p);
		}
		else
			target->setCustomMemoryLocation(p);
	}

	
}

void AsmCodeGenerator::emitSpanIteration(BaseCompiler* c, BaseScope* s, const Symbol& iterator, SpanType* typePtr, RegPtr spanTarget, Operations::Statement* loopBody, bool loadIterator)
{
	

#if 0
	auto acg = CREATE_ASM_COMPILER(Types::ID::Float);

	target->process(compiler, scope);
	target->reg->loadMemoryIntoRegister(acg.cc);

	auto endRegPtr = compiler->registerPool.getNextFreeRegister(scope, Types::ID::Integer);

	allocateDirtyGlobalVariables(getLoopBlock(), compiler, scope);

	auto itReg = compiler->registerPool.getRegisterForVariable(scope, iterator);
	itReg->createRegister(acg.cc);
	itReg->setIsIteratorRegister(true);

	auto& cc = getFunctionCompiler(compiler);

	auto blockAddress = target->reg->getRegisterForReadOp().as<X86Gp>();

	endRegPtr->createRegister(acg.cc);

	auto wpReg = acg.cc.newIntPtr();
	auto endReg = endRegPtr->getRegisterForWriteOp().as<X86Gp>();

	auto sizeAddress = x86::dword_ptr(blockAddress, 4);
	auto writePointerAddress = x86::dword_ptr(blockAddress, 8);

	cc.setInlineComment("block size");
	cc.mov(endReg, sizeAddress);
	cc.setInlineComment("block data ptr");
	cc.mov(wpReg, writePointerAddress);

	auto loopStart = cc.newLabel();

	cc.setInlineComment("loop_block {");
	cc.bind(loopStart);

	auto adress = x86::dword_ptr(wpReg);

	if (loadIterator)
		cc.movss(itReg->getRegisterForWriteOp().as<X86Xmm>(), adress);

	getLoopBlock()->process(compiler, scope);

	cc.movss(adress, itReg->getRegisterForReadOp().as<X86Xmm>());
	cc.add(wpReg, 4);
	cc.dec(endReg);
	cc.setInlineComment("loop_block }");
	cc.jnz(loopStart);
	itReg->flagForReuse(true);
	target->reg->flagForReuse(true);
	endRegPtr->flagForReuse(true);
#endif
}

void AsmCodeGenerator::emitCompare(OpType op, RegPtr target, RegPtr l, RegPtr r)
{
	ScopedTypeSetter st(*this, l->getType());

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

#define FLOAT_COMPARE(token, command) if (op == token) command(INT_REG_R(target), condReg.get());

	TemporaryRegister condReg(*this, target->getScope(), Types::ID::Block);

	cc.mov(INT_REG_W(target), 0);
	cc.mov(condReg.get(), 1);

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

	auto dirtyGlobalList = c->registerPool.getListOfAllDirtyGlobals();

	

	for (auto reg : dirtyGlobalList)
	{
		emitMemoryWrite(reg);
	}



	if (expr != nullptr && (expr->isMemoryLocation() || expr->isDirtyGlobalMemory()))
	{
		if (auto am = c->registerPool.getActiveRegisterForCustomMem(expr))
		{
			rToUse = expr;
			expr->loadMemoryIntoRegister(cc);
		}
		else
		{
			emitStore(target, expr);
			rToUse = target;
		}
	}
	else
		rToUse = expr;

	

	dirtyGlobalList.clear();

	if (expr == nullptr || target == nullptr)
		cc.ret();
	else
	{
		if (type == Types::ID::Float || type == Types::ID::Double)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Xmm>());
		else if (type == Types::ID::Integer)
            cc.ret(rToUse->getRegisterForWriteOp().as<x86::Gpd>());
		else if (type == Types::ID::Event)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Gpq>());
		else if (type == Types::ID::Block)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Gpq>());
	}
}


AsmCodeGenerator::RegPtr AsmCodeGenerator::emitBranch(Types::ID returnType, Operations::Expression* condition, Operations::Statement* trueBranch, Operations::Statement* falseBranch, BaseCompiler* c, BaseScope* s)
{
	RegPtr returnReg;

	if (returnType != Types::ID::Void)
	{
		returnReg = c->registerPool.getNextFreeRegister(s, returnType);
		returnReg->createRegister(cc);
	}


	auto l = cc.newLabel();
	auto e = cc.newLabel();

	// emit the code for the condition here
	condition->process(c, s);

	if (condition->reg->isMemoryLocation())
	{
		auto dummy = c->registerPool.getNextFreeRegister(s, Types::ID::Integer);
		dummy->createRegister(cc);

		cc.mov(dummy->getRegisterForWriteOp().as<X86Gp>(), imm2ptr(condition->reg->getImmediateIntValue()));
		cc.cmp(dummy->getRegisterForReadOp().as<X86Gp>(), 0);

		dummy->flagForReuse(true);
	}
	else
		cc.cmp(INT_REG_R(condition->reg), 0);

	cc.setInlineComment("Branch test");
	cc.jnz(l);

	condition->reg->flagForReuseIfAnonymous();

	if (falseBranch != nullptr)
	{
		cc.setInlineComment("false branch");

		falseBranch->process(c, s);

		if (returnReg != nullptr)
		{
			if (auto fAsExpr = dynamic_cast<Operations::Expression*>(falseBranch))
			{
				emitStore(returnReg, fAsExpr->reg);
				fAsExpr->reg->flagForReuseIfAnonymous();
			}
			else
				jassertfalse;
		}
	}

	cc.jmp(e);
	cc.bind(l);
	cc.setInlineComment("true branch");
	trueBranch->process(c, s);

	if (returnReg != nullptr)
	{
		if (auto tAsExpr = dynamic_cast<Operations::Expression*>(trueBranch))
		{
			emitStore(returnReg, tAsExpr->reg);
			tAsExpr->reg->flagForReuseIfAnonymous();
		}
	}

	cc.bind(e);

	return returnReg;
}

AsmCodeGenerator::RegPtr AsmCodeGenerator::emitTernaryOp(Operations::TernaryOp* tOp, BaseCompiler* c, BaseScope* s)
{
	return emitBranch(tOp->getType(), tOp->getSubExpr(0).get(), tOp->getSubExpr(1).get(), tOp->getSubExpr(2).get(), c, s);
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
		ScopedTypeSetter st(*this, sourceType);

		IF_(float) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvttss2si(INT_REG_W(target), FP_MEM(expr));
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
		ScopedTypeSetter st(*this, sourceType);

		IF_(float) // SOURCE TYPE
		{
			if (IS_MEM(expr)) cc.cvtss2sd(FP_REG_W(target), FP_MEM(expr));
			else			  cc.cvtss2sd(FP_REG_W(target), FP_REG_R(expr));
		}
		IF_(int) // SOURCE TYPE
		{
			
			if (IS_MEM(expr)) cc.cvtsi2sd(FP_REG_W(target), expr->getAsMemoryLocation());
			else			  cc.cvtsi2sd(FP_REG_W(target), INT_REG_R(expr));
		}

		return;
	}
	IF_(float) // TARGET TYPE
	{
		ScopedTypeSetter st(*this, sourceType);

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
		cc.mulss(FP_REG_W(target), cc.newFloatConst(ConstPool::kScopeLocal, -1.0f));
	IF_(float)
		cc.mulsd(FP_REG_W(target), cc.newDoubleConst(ConstPool::kScopeLocal, -1.0));
}


void AsmCodeGenerator::emitFunctionCall(RegPtr returnReg, const FunctionData& f, RegPtr objectAddress,  ReferenceCountedArray<AssemblyRegister>& parameterRegisters)
{
	asmjit::FuncSignatureBuilder sig;

	bool isMemberFunction = objectAddress != nullptr;
	fillSignature(f, sig, isMemberFunction || f.object != nullptr);

	if(objectAddress != nullptr)
		objectAddress->loadMemoryIntoRegister(cc);
	
#if 0
	// Push the function pointer
	X86Gp fn = cc.newIntPtr("fn");
	cc.setInlineComment("FunctionPointer");
	cc.mov(fn, imm(f.function));
#endif

	TemporaryRegister o(*this, returnReg->getScope(), Types::ID::Block);
	bool useTempReg = false;

	if (!isMemberFunction && f.object != nullptr)
	{
		cc.setInlineComment("Push function object");
		cc.mov(o.get(), imm(f.object));
		useTempReg = true;
	}

	for (auto pr : parameterRegisters)
	{
		jassert(pr != nullptr);

		pr->loadMemoryIntoRegister(cc);
	}

	FuncCallNode* call = cc.call((uint64_t)f.function, sig);

	call->setInlineComment(f.functionName.getCharPointer().getAddress());

	int offset = 0;

	if (useTempReg)
	{
		call->setArg(0, o.get());
		offset = 1;
	}
	else if (objectAddress != nullptr)
	{
		
		call->setArg(0, objectAddress->getRegisterForReadOp().as<X86Gp>());
		offset = 1;
	}
	
	for(int i = 0; i < parameterRegisters.size(); i++)
		call->setArg(i + offset, parameterRegisters[i]->getRegisterForReadOp());

	if (f.returnType != Types::ID::Void)
	{
		returnReg->createRegister(cc);
		call->setRet(0, returnReg->getRegisterForWriteOp());
	}
}


void AsmCodeGenerator::writeToPointerAddress(RegPtr targetPointer, RegPtr source)
{
	type = source->getType();

	auto target = x86::qword_ptr(targetPointer->getRegisterForReadOp().as<X86Gpq>());

	cc.setInlineComment("Write to reference address");

	bool ok;

	if (source->isMemoryLocation())
	{
		IF_(int)	ok = cc.mov(target, source->getImmediateIntValue());
		//IF_(float)	ok = cc.movss(target, source->getAsMemoryLocation());
		//IF_(double) ok = cc.movsd(target, FP_MEM(source));
	}
	else
	{
		IF_(int)	ok = cc.mov(target, INT_REG_R(source));
		IF_(float)	ok = cc.movss(target, FP_REG_R(source));
		IF_(double) ok = cc.movsd(target, FP_REG_R(source));
	}

	
}

void AsmCodeGenerator::dumpVariables(BaseScope* s, uint64_t lineNumber)
{
	auto globalScope = s->getGlobalScope();
	auto& bpHandler = globalScope->getBreakpointHandler();

	Array<Identifier> ids;
	
	for (auto r : registerPool->getListOfAllNamedRegisters())
	{
		if (auto ref = r->getVariableId())
		{
			if (auto address = bpHandler.getNextFreeTable(ref))
				emitMemoryWrite(r, address);
		}
	}

	
	auto data = reinterpret_cast<void*>(bpHandler.getLineNumber());

#if JUCE_64BIT

	TemporaryRegister ad(*this, s, Types::ID::Block);
	int ok = cc.mov(ad.get(), void2ptr(data));
	auto target = x86::qword_ptr(ad.get());
#else
	auto target = x86::dword_ptr(reinterpret_cast<uint64_t>(data));

#endif

	cc.mov(target, lineNumber);
}

void AsmCodeGenerator::fillSignature(const FunctionData& data, FuncSignatureX& sig, bool createObjectPointer)
{
	if (data.returnType == Types::ID::Float) sig.setRetT<float>();
	if (data.returnType == Types::ID::Double) sig.setRetT<double>();
	if (data.returnType == Types::ID::Integer) sig.setRetT<int>();
	if (data.returnType == Types::ID::Event) sig.setRet(asmjit::Type::kIdIntPtr);
	if (data.returnType == Types::ID::Block) sig.setRet(asmjit::Type::kIdIntPtr);

	if (createObjectPointer)
		sig.addArgT<PointerType>();

	for (auto p : data.args)
	{
		if (p == Types::ID::Float)	 sig.addArgT<float>();
		if (p == Types::ID::Double)  sig.addArgT<double>();
		if (p == Types::ID::Integer) sig.addArgT<int>();
		if (p == Types::ID::Event)   sig.addArg(asmjit::Type::kIdIntPtr);
		if (p == Types::ID::Block)   sig.addArg(asmjit::Type::kIdIntPtr);
	}
}


void SpanLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	jassert(loopTarget->getType() == Types::ID::Pointer);
	jassert(typePtr != nullptr);
	jassert(iterator.type == typePtr->getElementType());

	AsmCodeGenerator::TemporaryRegister end(gen, loopTarget->getScope(), Types::ID::Pointer);

	auto& cc = gen.cc;

	loopTarget->loadMemoryIntoRegister(cc, true);

	auto offset = typePtr->getElementSize() * (typePtr->getNumElements());

	cc.lea(end.get(), x86::ptr(INT_REG_R(loopTarget), offset));

	auto loopStart = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);
	itReg->createRegister(cc);
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop_span {");
	cc.bind(loopStart);

	if (loadIterator)
	{
		IF_(int)    cc.mov(INT_REG_W(itReg), x86::ptr(INT_REG_R(loopTarget)));
		IF_(float)  cc.movss(FP_REG_W(itReg), x86::ptr(INT_REG_R(loopTarget)));
		IF_(double) cc.movsd(FP_REG_W(itReg), x86::ptr(INT_REG_R(loopTarget)));
		IF_(void*)  cc.mov(INT_REG_W(itReg), INT_REG_R(loopTarget));
	}

	itReg->setUndirty();

	loopBody->process(compiler, scope);

	if (itReg->isDirtyGlobalMemory())
	{
		IF_(int)    cc.mov(x86::ptr(INT_REG_R(loopTarget)), INT_REG_R(itReg));
		IF_(float)  cc.movss(x86::ptr(INT_REG_R(loopTarget)), FP_REG_R(itReg));
		IF_(double) cc.movsd(x86::ptr(INT_REG_R(loopTarget)), FP_REG_W(itReg));
		// no storing needed for pointer iterators...
	}

	cc.add(INT_REG_W(loopTarget), (int64_t)typePtr->getElementSize());
	cc.cmp(INT_REG_R(loopTarget), end.get());
	cc.setInlineComment("loop_span }");
	cc.jne(loopStart);

	itReg->setUndirty();
	itReg->flagForReuse(true);
}

void BlockLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	jassert(loopTarget->getType() == Types::ID::Block);
	jassert(iterator.type == Types::ID::Float);
	auto& cc = gen.cc;


	loopTarget->loadMemoryIntoRegister(cc);
	auto blockAddress = INT_REG_R(loopTarget);

	AsmCodeGenerator::TemporaryRegister beg(gen, loopTarget->getScope(), Types::ID::Pointer);
	AsmCodeGenerator::TemporaryRegister end(gen, loopTarget->getScope(), Types::ID::Pointer);

	

	cc.mov(beg.get(), x86::ptr(blockAddress).cloneAdjusted(8));
	cc.mov(end.get(), x86::ptr(blockAddress).cloneAdjusted(4));
	cc.and_(end.get(), 0xFFFFFFF);
	cc.imul(end.get(), (uint64_t)sizeof(float));
	cc.add(end.get(), beg.get());

	auto loopStart = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);
	itReg->createRegister(cc);
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop_block {");
	cc.bind(loopStart);

	if (loadIterator)
		cc.movss(FP_REG_W(itReg), x86::ptr(beg.get()));

	itReg->setUndirty();

	loopBody->process(compiler, scope);

	if (itReg->isDirtyGlobalMemory())
		cc.movss(x86::ptr(beg.get()), FP_REG_R(itReg));

    cc.add(beg.get().as<x86::Gpd>(), (uint64_t)sizeof(float));
	cc.cmp(beg.get().as<x86::Gpd>(), end.get());
	cc.setInlineComment("loop_block }");
	cc.jne(loopStart);

	itReg->setUndirty();
	itReg->flagForReuse(true);




















#if 0
	loopTarget->loadMemoryIntoRegister(gen.cc, true);

	auto endRegPtr = compiler->registerPool.getNextFreeRegister(scope, Types::ID::Integer);

	allocateDirtyGlobalVariables(getLoopBlock(), compiler, scope);

	auto itReg = compiler->registerPool.getRegisterForVariable(scope, iterator);
	itReg->createRegister(acg.cc);
	itReg->setIsIteratorRegister(true);

	auto& cc = getFunctionCompiler(compiler);

	

	endRegPtr->createRegister(acg.cc);

	auto wpReg = acg.cc.newIntPtr();
	auto endReg = endRegPtr->getRegisterForWriteOp().as<X86Gp>();

	

	cc.setInlineComment("block size");
	cc.mov(endReg, sizeAddress);
	cc.setInlineComment("block data ptr");
	cc.mov(wpReg, writePointerAddress);

	auto loopStart = cc.newLabel();

	cc.setInlineComment("loop_block {");
	cc.bind(loopStart);

	auto adress = x86::dword_ptr(wpReg);

	if (loadIterator)
		cc.movss(itReg->getRegisterForWriteOp().as<X86Xmm>(), adress);

	getLoopBlock()->process(compiler, scope);

	cc.movss(adress, itReg->getRegisterForReadOp().as<X86Xmm>());
	cc.add(wpReg, 4);
	cc.dec(endReg);
	cc.setInlineComment("loop_block }");
	cc.jnz(loopStart);
	itReg->flagForReuse(true);
	target->reg->flagForReuse(true);
	endRegPtr->flagForReuse(true);

#endif

}

}

}
