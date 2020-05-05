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
	if (target->hasCustomMemoryLocation() && !target->isActive())
	{
		if (target->shouldLoadMemoryIntoRegister())
		{
			target->createRegister(cc);

			IF_(int)   INT_OP(cc.mov, target, value);
			IF_(float) FP_OP(cc.movss, target, value);
			IF_(double) FP_OP(cc.movsd, target, value);
		}
		else
		{
			value->loadMemoryIntoRegister(cc);

			IF_(int)		cc.mov(INT_MEM(target), INT_REG_R(value));
			IF_(float)		cc.movss(FP_MEM(target), FP_REG_R(value));
			IF_(double)		cc.movsd(FP_MEM(target), FP_REG_R(value));
		}
	}
	else
	{
		target->createRegister(cc);

		if (value->isZero())
		{
			// Will be a nullptr call...
			

			IF_(float)  FP_OP(cc.xorps, target, target);
			IF_(double) FP_OP(cc.xorps, target, target);
			IF_(int)    INT_OP(cc.xor_, target, target);
			IF_(void*)
			{
				if (target->isSimd4Float())
				{
					FP_OP(cc.xorps, target, target);
				}
				else
				{
					jassert(type != Types::ID::Pointer);
					jassertfalse;
				}
			}
			return;
		}
		else
		{
			if (value->getTypeInfo().isNativePointer())
			{
				auto ptr = x86::ptr(PTR_REG_R(value));
				value->setCustomMemoryLocation(ptr, value->isGlobalMemory());
			}

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
	}
	
	IF_(void*)
	{
		if (target->isSimd4Float())
		{
			if (value->isSimd4Float())
			{
				if (target->hasCustomMemoryLocation())
				{
					value->loadMemoryIntoRegister(cc);

					target->invalidateRegisterForCustomMemory();
					cc.movaps(target->getAsMemoryLocation(), FP_REG_R(value));

				}
				else
				{
					jassert(!target->isMemoryLocation());

					FP_OP(cc.movaps, target, value);
				}
			}
			else
			{
				float immValue;
				X86Xmm v = cc.newXmm();

				if (value->getImmediateValue(immValue))
				{
					auto d = Data128::fromF32(immValue);
					auto mem = cc.newXmmConst(ConstPool::kScopeGlobal, d);

					if (target->hasCustomMemoryLocation())
					{
						cc.movaps(v, mem);
						cc.movaps(target->getMemoryLocationForReference(), v);
					}
					else
						cc.movaps(FP_REG_W(target), mem);
				}
				else
				{
					if (value->isMemoryLocation())
						cc.movss(v, value->getAsMemoryLocation());
					else
						v = FP_REG_R(value);

					cc.shufps(v, v, 0);

					if (target->hasCustomMemoryLocation())
						cc.movaps(target->getMemoryLocationForReference(), v);
					else
						cc.movaps(FP_REG_W(target), v);
				}
			}
		}
		else
		{
			if (value->isMemoryLocation())
				cc.lea(PTR_REG_W(target), value->getAsMemoryLocation());
			else
				cc.lea(PTR_REG_W(target), x86::ptr(PTR_REG_R(value)));
		}
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
        
        x86::Gp r = cc.newGpq();
        cc.mov(r, void2ptr(data));
        
		target = x86::qword_ptr(r);
	}

	IF_(int)	ok = cc.mov(target, INT_REG_R(source));
	IF_(float)	ok = cc.movss(target, FP_REG_R(source));
	IF_(double) ok = cc.movsd(target, FP_REG_R(source));
	
	if (source->isSimd4Float())
	{
		cc.movaps(target, FP_REG_R(source));
		
	}

	

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

	auto r = cc.newGpq();
    cc.mov(r, void2ptr(data));
    auto source = x86::ptr(r);
    
	// We use the REG_R ones to avoid flagging it dirty
	IF_(int)	cc.mov(INT_REG_R(target), source);
	IF_(float)	cc.movss(FP_REG_R(target), source);
	IF_(double) cc.movsd(FP_REG_R(target), source);
}


void AsmCodeGenerator::emitComplexTypeCopy(RegPtr target, RegPtr source, ComplexType::Ptr type)
{
	jassert(target->hasCustomMemoryLocation());
	jassert(type != nullptr);
	jassert(source->getTypeInfo().getComplexType() == type);

	cc.setInlineComment("Copy complex type argument");

	auto ptr = target->getAsMemoryLocation();
	auto numBytesToCopy = type->getRequiredByteSize();

	auto canUse128bitForCopy = numBytesToCopy % 16 == 0;

	int wordSize = 0;

	if (numBytesToCopy % 16 == 0)
		wordSize = 16;
	else if (numBytesToCopy % 8 == 0)
		wordSize = 8;
	else if (numBytesToCopy % 4 == 0)
		wordSize = 4;
	else
		jassertfalse;

	auto numIterations = numBytesToCopy / wordSize;
	bool shouldUnroll = numIterations <= 8;

	X86Reg tempReg;

	if (wordSize == 16) tempReg = cc.newXmm();
	if (wordSize == 8) tempReg = cc.newGpq();
	if (wordSize == 4) tempReg = cc.newGpd();

	if (shouldUnroll)
	{
		X86Mem srcBegin;

		if (source->isMemoryLocation())
			srcBegin = source->getAsMemoryLocation();
		else
			srcBegin = x86::ptr(PTR_REG_R(source));

		auto dstBegin = ptr;

		

		for (int i = 0; i < numIterations; i++)
		{
			auto src = srcBegin.cloneAdjustedAndResized(i * wordSize, wordSize);
			auto dst = dstBegin.cloneAdjustedAndResized(i * wordSize, wordSize);

			switch (wordSize)
			{
			case 16: cc.movaps(tempReg.as<X86Xmm>(), src);
					 cc.movaps(dst, tempReg.as<X86Xmm>());
					 break;
			case 8:  cc.mov(tempReg.as<X86Gpq>(), src);
					 cc.mov(dst, tempReg.as<X86Gpq>());
					 break;
			case 4:  cc.mov(tempReg.as<x86::Gpd>(), src);
					 cc.mov(dst, tempReg.as<x86::Gpd>());
					 break;
			}
		}
	}
	else
	{
		auto src = cc.newGpq();
		auto dst = cc.newGpq();
		auto srcEnd = cc.newGpq();

		cc.lea(dst, ptr);

		if (source->isMemoryLocation())
			cc.lea(src, source->getAsMemoryLocation());
		else
			cc.mov(src, PTR_REG_R(source));

		cc.lea(srcEnd, x86::ptr(src).cloneAdjustedAndResized(numBytesToCopy, wordSize));

		auto startLabel = cc.newLabel();

		cc.bind(startLabel);

		switch (wordSize)
		{
		case 16: cc.movaps(tempReg.as<X86Xmm>(), x86::ptr(src));
				 cc.movaps(x86::ptr(dst), tempReg.as<X86Xmm>());
				 break;
		case 8:  cc.mov(tempReg.as<X86Gpq>(), x86::ptr(src));
				 cc.mov(x86::ptr(dst), tempReg.as<X86Gpq>());
				 break;
		case 4:  cc.mov(tempReg.as<x86::Gpd>(), x86::ptr(src));
				 cc.mov(x86::ptr(dst), tempReg.as<x86::Gpd>());
				 break;
		}

		cc.add(src, wordSize);
		cc.add(dst, wordSize);
		cc.cmp(src, srcEnd);
		cc.jne(startLabel);
	}

}

    
    
void AsmCodeGenerator::emitThisMemberAccess(RegPtr target, RegPtr parent, VariableStorage memberOffset)
{
	jassert(memberOffset.getType() == Types::ID::Integer);
	jassert(target->getType() == type);
	jassert(parent->getType() == Types::ID::Pointer);

	int byteSize = target->getTypeInfo().getRequiredByteSize();

	if (parent->isMemoryLocation())
	{
        auto p = createValid64BitPointer(cc, parent->getAsMemoryLocation(), memberOffset.toInt(), byteSize);
        target->setCustomMemoryLocation(p, true);
	}
	else
	{
		int byteSize = target->getTypeInfo().getRequiredByteSize();
		auto ptr = x86::ptr(INT_REG_R(parent), memberOffset.toInt());
		target->setCustomMemoryLocation(ptr.cloneResized(byteSize), true);
	}
}

void AsmCodeGenerator::emitMemberAcess(RegPtr target, RegPtr parent, RegPtr child)
{
	jassert(target->getType() == type);					// the target register with the final type
	jassert(child->getType() == Types::ID::Integer);	// the index register containing the member offset in pointer sizes

	if (parent->getType() == Types::ID::Pointer)
	{
		jassert(parent->getType() == Types::ID::Pointer);   // the pointer register containing the data slot pointer.

	// the child is always an immediate...
		jassert(child->isMemoryLocation() && !child->hasCustomMemoryLocation());

		auto immValue = VariableStorage((int)INT_IMM(child));

		emitThisMemberAccess(target, parent, immValue);
	}
	else
	{
		jassert(target->getType() == parent->getType());
		target->createRegister(cc);

		IF_(int) INT_OP(cc.mov, target, parent);
		IF_(float) FP_OP(cc.movss, target, parent);
		IF_(double) FP_OP(cc.movsd, target, parent);
	}
}

void AsmCodeGenerator::emitParameter(Operations::Function* f, RegPtr parameterRegister, int parameterIndex)
{
	if (f->hasObjectPtr)
		parameterIndex += 1;

	parameterRegister->createRegister(cc);

	auto useParameterAsAdress = f->data.args[parameterIndex].isReference() && parameterRegister->getType() != Types::ID::Pointer;

	if (useParameterAsAdress)
	{
		auto aReg = cc.newGpq();
		cc.setArg(parameterIndex, aReg);
		parameterRegister->setCustomMemoryLocation(x86::ptr(aReg), true);
	}
	else
		cc.setArg(parameterIndex, parameterRegister->getRegisterForReadOp());
}

#define FLOAT_BINARY_OP(token, floatOp, doubleOp) if(op == token) { IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

#define BINARY_OP(token, intOp, floatOp, doubleOp) if(op == token) { IF_(int) INT_OP(intOp, l, r); IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

AsmCodeGenerator::RegPtr AsmCodeGenerator::emitBinaryOp(OpType op, RegPtr l, RegPtr r)
{
	if (l->isSimd4Float())
	{
		l->loadMemoryIntoRegister(cc);

		if (!r->isSimd4Float())
		{
			jassert(r->getType() == Types::ID::Float);
			r->loadMemoryIntoRegister(cc);
			cc.shufps(FP_REG_R(r), FP_REG_R(r), 0);
		}

		if (op == JitTokens::plus)
			FP_OP(cc.addps, l, r);
		if (op == JitTokens::times)
			FP_OP(cc.mulps, l, r);
		if (op == JitTokens::minus)
			FP_OP(cc.subps, l, r);
	}
	else
	{
		jassert(l != nullptr && r != nullptr);
		jassert(l->getType() != Types::ID::Pointer);

		l->loadMemoryIntoRegister(cc);


		if (type == Types::ID::Integer && (op == JitTokens::modulo || op == JitTokens::divide))
		{
			TemporaryRegister dummy(*this, r->getScope(), TypeInfo(Types::ID::Integer));
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
	}
	
	if (l->getTypeInfo().isRef() || l->hasCustomMemoryLocation())
	{
		auto target = l->getMemoryLocationForReference();
		
		IF_(int)		cc.mov(target, INT_REG_R(l));
		IF_(float)		cc.movss(target, FP_REG_R(l));
		IF_(double)		cc.movsd(target, FP_REG_R(l));
		IF_(void*)
		{
			jassert(l->isSimd4Float());
			cc.movaps(target, FP_REG_R(l));
		}
		
        if(l->isDirtyGlobalMemory())
            l->setUndirty();
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

	op->reg = op->currentCompiler->getRegFromPool(op->currentScope, TypeInfo(Types::ID::Integer));
	op->reg->createRegister(cc);
	INT_OP_WITH_MEM(cc.mov, op->reg, l);
}



void AsmCodeGenerator::emitSpanReference(RegPtr target, RegPtr address, RegPtr index, size_t elementSizeInBytes, int additionalOffsetInBytes)
{
	jassert(index->getType() == Types::ID::Integer);

	if (address->isSimd4Float() && !address->isMemoryLocation())
	{
		jassert(additionalOffsetInBytes == 0);

		address->loadMemoryIntoRegister(cc);

		jassert(target->getType() == Types::ID::Float);

		target->createRegister(cc);

		if (index->isMemoryLocation())
		{
			int idx = index->getImmediateIntValue();

			jassert(isPositiveAndBelow(idx, 4));

			auto x = cc.newXmmPs();

			switch (idx)
			{
			case 0: cc.movss(FP_REG_W(target), FP_REG_R(address));;
					break;

			case 1: cc.movaps(x, FP_REG_R(address)); 
					cc.shufps(x, x, 229); 
					cc.movaps(FP_REG_W(target), x);
					break;
			case 2: cc.movaps(x, FP_REG_R(address)); 
					cc.unpckhpd(x, x);
					cc.movaps(FP_REG_W(target), x); 
					break;
			case 3: cc.movaps(x, FP_REG_R(address)); 
					cc.shufps(x, x, 231);
					cc.movaps(FP_REG_W(target), x); 
					break;
			default: jassertfalse;
			}

			

			return;
		}
		else
		{
			auto m = cc.newStack(address->getTypeInfo().getRequiredByteSize(), address->getTypeInfo().getRequiredAlignment());

			cc.movaps(m, FP_REG_R(address));

			auto c = cc.newGpq();
			cc.lea(c, m);
			auto p = x86::ptr(c, INT_REG_R(index), 2, 0, 4);

			target->setCustomMemoryLocation(p, false);
			return;
		}
	}

	jassert(address->getType() == Types::ID::Pointer);
	jassert(index->getType() == Types::ID::Integer);
	jassert(address->getType() == Types::ID::Pointer || address->isMemoryLocation());

	int shift = (int)log2(elementSizeInBytes);
	bool canUseShift = isPowerOfTwo(elementSizeInBytes) && shift < 4;

	x86::Gpd indexReg;

	if (index->hasCustomMemoryLocation())
		index->loadMemoryIntoRegister(cc);

	if (!index->isMemoryLocation())
	{
		indexReg = index->getRegisterForReadOp().as<x86::Gpd>();

		if (!canUseShift && index->getVariableId())
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

	bool isDyn = address->getTypeInfo().getTypedComplexType<DynType>();

	if (address->isMemoryLocation())
	{
		auto ptr = address->getAsMemoryLocation();

		if (isDyn)
		{
			auto b_ = cc.newGpq();
			cc.mov(b_, ptr.cloneAdjustedAndResized(8, 8));
			ptr = x86::ptr(b_);
		}


        
        
		if (index->isMemoryLocation())
        {
            p = ptr.cloneAdjustedAndResized(imm2ptr(INT_IMM(index) * elementSizeInBytes), elementSizeInBytes);
        }
		else
		{
			// for some reason uint64_t base + index reg doesn't create a 64bit address...
			// so we need to use a register for the base
			auto b_ = cc.newGpq();

			cc.lea(b_, ptr);

			p = x86::ptr(b_, indexReg, shift);
		}
	}
	else
	{
		address->loadMemoryIntoRegister(cc);

		X86Gp adToUse = PTR_REG_R(address);


		if (isDyn)
		{
			adToUse = cc.newGpq();
			cc.mov(adToUse, x86::ptr(PTR_REG_R(address)).cloneAdjustedAndResized(8, 8));
		}

		if (index->isMemoryLocation())
			p = x86::ptr(adToUse, imm2ptr(INT_IMM(index) * elementSizeInBytes));
		else
			p = x86::ptr(adToUse, indexReg, shift);
	}

	if (additionalOffsetInBytes != 0)
	{
		p = p.cloneAdjusted(additionalOffsetInBytes);
	}

	target->setCustomMemoryLocation(p, address->isGlobalMemory());

	if (type == Types::ID::Pointer)
	{

        if(target->isSimd4Float())
        {
            target->createRegister(cc);
            cc.movaps(FP_REG_W(target), p);
        }
        else
        {
            target->createRegister(cc);
            cc.lea(INT_REG_W(target), p);
        }
        
		
	}
	else
    {
        target->setCustomMemoryLocation(p, address->isGlobalMemory());
    }


}

void AsmCodeGenerator::emitCompare(bool useAsmFlags, OpType op, RegPtr target, RegPtr l, RegPtr r)
{
	ScopedTypeSetter st(*this, l->getType());

	l->loadMemoryIntoRegister(cc);
	target->createRegister(cc);

	IF_(int)
	{
		INT_OP(cc.cmp, l, r);

		if (!useAsmFlags)
		{

#define INT_COMPARE(token, command) if (op == token) command(INT_REG_W(target));

			INT_COMPARE(JitTokens::greaterThan, cc.setg);
			INT_COMPARE(JitTokens::lessThan, cc.setl);
			INT_COMPARE(JitTokens::lessThanOrEqual, cc.setle);
			INT_COMPARE(JitTokens::greaterThanOrEqual, cc.setge);
			INT_COMPARE(JitTokens::equals, cc.sete);
			INT_COMPARE(JitTokens::notEquals, cc.setne);

#undef INT_COMPARE

			cc.and_(INT_REG_W(target), 1);
		}

		return;
	}
	IF_(float) FP_OP(cc.ucomiss, l, r);
	IF_(double) FP_OP(cc.ucomisd, l, r);

#define FLOAT_COMPARE(token, command) if (op == token) command(INT_REG_R(target), condReg.get());

	TemporaryRegister condReg(*this, target->getScope(), TypeInfo(Types::ID::Block));

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

	writeDirtyGlobals(c);

	if (target != nullptr && target->getTypeInfo().isNativePointer())
	{
		jassert(expr != nullptr);

		auto mem = expr->getMemoryLocationForReference();

		if(mem.isNone())
			jassertfalse;
		else
		{
			target->createRegister(cc);
			cc.lea(PTR_REG_W(target), mem);
			cc.ret(PTR_REG_W(target));
			return;
		}
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

	

	

	if (expr == nullptr || target == nullptr)
		cc.ret();
	else
	{
		if (type == Types::ID::Float || type == Types::ID::Double)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Xmm>());
		else if (type == Types::ID::Integer)
            cc.ret(rToUse->getRegisterForWriteOp().as<x86::Gpd>());
		else if (type == Types::ID::Block || type == Types::ID::Pointer)
			cc.ret(rToUse->getRegisterForWriteOp().as<X86Gpq>());
	}
}


void AsmCodeGenerator::writeDirtyGlobals(BaseCompiler* c)
{
	auto dirtyGlobalList = c->registerPool.getListOfAllDirtyGlobals();

	for (auto reg : dirtyGlobalList)
	{
		emitMemoryWrite(reg);
	}

	dirtyGlobalList.clear();
}

void AsmCodeGenerator::emitStackInitialisation(RegPtr target, ComplexType::Ptr typePtr, RegPtr expr, InitialiserList::Ptr list)
{
	jassert(typePtr != nullptr);
	jassert(list == nullptr || expr == nullptr);
	jassert(expr == nullptr || expr->getType() == target->getType());

	if (list != nullptr)
	{
		using InitElement = InitialiserList::ChildBase;
		using ExprElement = InitialiserList::ExpressionChild;

		auto listHasExpressions = list->forEach([](InitElement* b)
		{
			return dynamic_cast<ExprElement*>(b) != nullptr;
		});
		
		if (listHasExpressions)
		{
			AssemblyMemory m;
			m.cc = &cc;
			m.memory = target->getAsMemoryLocation().clone();

			ComplexType::InitData d;
			d.asmPtr = &m;
			d.initValues = list;

			typePtr->initialise(d);
		}
		else
		{
			HeapBlock<uint8> data;
			data.allocate(typePtr->getRequiredByteSize() + typePtr->getRequiredAlignment(), true);

			auto start = data + typePtr->getRequiredAlignment();
			auto end = start + typePtr->getRequiredByteSize();
			int numBytesToInitialise = end - start;

			ComplexType::InitData initData;
			initData.dataPointer = start;
			initData.initValues = list;

			typePtr->initialise(initData);

			float* d = reinterpret_cast<float*>(start);

			if (numBytesToInitialise % 16 == 0)
			{
				uint64_t* s = reinterpret_cast<uint64_t*>(start);
				auto dst = target->getAsMemoryLocation().clone();

				auto r = cc.newXmm();

				for (int i = 0; i < numBytesToInitialise; i += 16)
				{
					auto d = Data128::fromU64(s[0], s[1]);
					auto c = cc.newXmmConst(ConstPool::kScopeLocal, d);

					cc.movaps(r, c);
					cc.movaps(dst, r);
					dst.addOffset(16);
					s += 2;
				}
			}
			else if (numBytesToInitialise % 4 == 0)
			{
				int* s = reinterpret_cast<int*>(start);
				auto dst = target->getAsMemoryLocation().clone();
				dst.setSize(4);

				auto r = cc.newGpd();

				for (int i = 0; i < numBytesToInitialise; i += 4)
				{
					cc.mov(dst, *s);
					dst.addOffset(4);
					s++;
				}
			}
		}


		
	}
	else
	{
		jassertfalse;
		

	}
}

AsmCodeGenerator::RegPtr AsmCodeGenerator::emitBranch(TypeInfo returnType, Operations::Expression* condition, Operations::Statement* trueBranch, Operations::Statement* falseBranch, BaseCompiler* c, BaseScope* s)
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
		auto dummy = c->registerPool.getNextFreeRegister(s, TypeInfo(Types::ID::Integer));
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
	return emitBranch(tOp->getTypeInfo(), tOp->getSubExpr(0).get(), tOp->getSubExpr(1).get(), tOp->getSubExpr(2).get(), c, s);
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


Result AsmCodeGenerator::emitFunctionCall(RegPtr returnReg, const FunctionData& f, RegPtr objectAddress,  ReferenceCountedArray<AssemblyRegister>& parameterRegisters)
{
	if (f.canBeInlined(false))
	{
		AsmInlineData d(*this);
		d.args = parameterRegisters;
		d.target = returnReg;
		d.templateParameters = f.templateParameters;
		d.object = objectAddress;

		jassert(f.object == nullptr);

		return f.inlineFunction(&d);
	}

	asmjit::FuncSignatureBuilder sig;

	bool isMemberFunction = objectAddress != nullptr;
	auto objectType = isMemberFunction ? objectAddress->getType() : Types::ID::Void;

	if (f.object != nullptr)
	{
		objectType = Types::ID::Pointer;
	}

	fillSignature(f, sig, objectType);

	if(objectAddress != nullptr)
		objectAddress->loadMemoryIntoRegister(cc);
	
	TemporaryRegister o(*this, returnReg->getScope(), TypeInfo(Types::ID::Block));
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

	return Result::ok();
}


void AsmCodeGenerator::emitFunctionParameterReference(RegPtr sourceReg, RegPtr parameterReg)
{
	jassert(parameterReg->getType() == Types::ID::Pointer);

	auto mem = sourceReg->getMemoryLocationForReference();
	auto isStackVariable = !(sourceReg->getScope()->getScopeType() == BaseScope::Class);

	if (isStackVariable)
	{
		auto byteSize = Types::Helpers::getSizeForType(type);
		mem = cc.newStack(byteSize, 0);

		IF_(int) cc.mov(mem, INT_REG_R(sourceReg));
		IF_(float) cc.movss(mem, FP_REG_R(sourceReg));
		IF_(double) cc.movsd(mem, FP_REG_R(sourceReg));

		sourceReg->setCustomMemoryLocation(mem, false);
	}


	cc.lea(INT_REG_R(parameterReg), mem);
}

juce::Array<juce::Identifier> AsmCodeGenerator::getInlineableMathFunctions()
{
	Array<Identifier> ids;

	ids.add("abs");
	ids.add("sin");
	ids.add("cos");
	ids.add("min");
	ids.add("max");
	ids.add("map");
	ids.add("range");
	ids.add("sign");
	ids.add("fmod");

	return ids;
}

void AsmCodeGenerator::emitInlinedMathAssembly(Identifier id, RegPtr target, const ReferenceCountedArray<AssemblyRegister>& args)
{
	auto isQword = type == Types::ID::Double;

	jassert(getInlineableMathFunctions().contains(id));

	target->createRegister(cc);

#define IF_FUNCTION(x) if(id.toString() == #x)

	IF_FUNCTION(abs)
	{
		IF_(float)
		{
			auto c = cc.newXmmConst(asmjit::ConstPool::kScopeGlobal, Data128::fromU32(0x7fffffff));
			cc.movss(FP_REG_W(target), FP_REG_R(args[0]));
			cc.andps(FP_REG_W(target), c);
		}
		IF_(double)
		{
			auto c = cc.newXmmConst(asmjit::ConstPool::kScopeGlobal, Data128::fromU64(0x7fffffffffffffff));
			cc.movsd(FP_REG_W(target), FP_REG_R(args[0]));
			cc.andps(FP_REG_W(target), c);
		}
	}
	IF_FUNCTION(max)
	{
		IF_(float)
		{
			FP_OP(cc.movss, target, args[0]);
			FP_OP(cc.maxss, target, args[1]);
		}
		IF_(double)
		{
			FP_OP(cc.movsd, target, args[0]);
			FP_OP(cc.maxsd, target, args[1]);
		}
	}
	IF_FUNCTION(min)
	{
		IF_(float)
		{
			FP_OP(cc.movss, target, args[0]);
			FP_OP(cc.minss, target, args[1]);
		}
		IF_(double)
		{
			FP_OP(cc.movsd, target, args[0]);
			FP_OP(cc.minsd, target, args[1]);
		}
	}
	IF_FUNCTION(range)
	{
		IF_(float)
		{
			FP_OP(cc.movss, target, args[0]);
			FP_OP(cc.maxss, target, args[1]);
			FP_OP(cc.minss, target, args[2]);
			
		}
		IF_(double)
		{
			FP_OP(cc.movsd, target, args[0]);
			FP_OP(cc.maxsd, target, args[1]);
			FP_OP(cc.minsd, target, args[2]);
		}
	}
	IF_FUNCTION(sign)
	{
		args[0]->loadMemoryIntoRegister(cc);

		auto pb = cc.newLabel();
		auto nb = cc.newLabel();

		IF_(float)
		{
			auto input = FP_REG_R(args[0]);
			auto t = FP_REG_W(target);
			auto mem = cc.newMmConst(ConstPool::kScopeGlobal, Data64::fromF32(-1.0f, 1.0f));
			auto i = cc.newGpq();
			auto r2 = cc.newGpq();
			auto zero = cc.newXmmSs();

			cc.xorps(zero, zero);
			cc.xor_(i, i);
			cc.ucomiss(input, zero);
			cc.seta(i.r8());
			cc.lea(r2, mem);
			cc.movss(t, x86::ptr(r2, i, 2, 0, 4));
		}
		IF_(double)
		{
			auto input = FP_REG_R(args[0]); 
			auto t = FP_REG_W(target); 
			auto mem = cc.newXmmConst(ConstPool::kScopeGlobal, Data128::fromF64(-1.0, 1.0));
			auto i = cc.newGpq();
			auto r2 = cc.newGpq();
			auto zero = cc.newXmmSd();

			cc.xorps(zero, zero);
			cc.xor_(i, i);
			cc.ucomisd(input, zero);
			cc.seta(i.r8());
			cc.lea(r2, mem);
			cc.movsd(t, x86::ptr(r2, i, 3, 0, 8));
		}
	}
	IF_FUNCTION(map)
	{
		IF_(float)
		{
			FP_OP(cc.movss, target, args[2]);
			FP_OP(cc.subss, target, args[1]);
			FP_OP(cc.mulss, target, args[0]);
			FP_OP(cc.addss, target, args[1]);
		}
		IF_(double)
		{
			FP_OP(cc.movsd, target, args[2]);
			FP_OP(cc.subsd, target, args[1]);
			FP_OP(cc.mulsd, target, args[0]);
			FP_OP(cc.addsd, target, args[1]);
		}
	}
	IF_FUNCTION(fmod)
	{
		X86Mem x = createFpuMem(args[0]);
		X86Mem y = createFpuMem(args[1]);
		auto u = createStack(target->getType());

		cc.fld(y);
		cc.fld(x);
		cc.fprem();
		cc.fstp(x);
		cc.fstp(u);

		writeMemToReg(target, x);
	}
	IF_FUNCTION(sin)
	{
		X86Mem x = createFpuMem(args[0]);

		cc.fld(x);
		cc.fsin();
		cc.fstp(x);

		writeMemToReg(target, x);
	}

	IF_FUNCTION(cos)
	{
		X86Mem x = createFpuMem(args[0]);

		cc.fld(x);
		cc.fcos();
		cc.fstp(x);

		writeMemToReg(target, x);
	}
}



Result AsmCodeGenerator::emitSimpleToComplexTypeCopy(RegPtr target, InitialiserList::Ptr initValues, RegPtr source)
{
	
	jassert(type == target->getType());
	jassert(source == nullptr || type == source->getType());

	target->createRegister(cc);

	if (source != nullptr)
	{
		IF_(int) INT_OP(cc.mov, target, source);
		IF_(float) FP_OP(cc.movss, target, source);
		IF_(double) FP_OP(cc.movsd, target, source);

		return Result::ok();
	}
	else if (initValues != nullptr)
	{
		VariableStorage v;
		auto r = initValues->getValue(0, v);

		if (r.failed())
			return r;

		if (v.getType() != type)
			return Result::fail("Type mismatch");


		IF_(int) cc.mov(INT_REG_W(target), v.toInt());
		IF_(float)
		{
			auto c = cc.newFloatConst(asmjit::ConstPool::kScopeLocal, v.toFloat());
			cc.movss(FP_REG_W(target), c);
		}
		IF_(double)
		{
			auto c = cc.newFloatConst(asmjit::ConstPool::kScopeLocal, v.toDouble());
			cc.movsd(FP_REG_W(target), c);
		}
		

		return Result::ok();
	}
	else
	{
		return Result::fail("not found");
	}
}

asmjit::X86Mem AsmCodeGenerator::createFpuMem(RegPtr ptr)
{
	auto qword = ptr->getType() == Types::ID::Double;
	X86Mem x;

	if (ptr->isMemoryLocation())
		x = FP_MEM(ptr);
	else
	{
		x = createStack(ptr->getType());

		if (qword)
			cc.movsd(x, FP_REG_R(ptr));
		else
			cc.movss(x, FP_REG_R(ptr));
	}

	return x;
}



void AsmCodeGenerator::writeMemToReg(RegPtr target, X86Mem mem)
{
	if (target->getType() == Types::ID::Double)
		cc.movsd(FP_REG_W(target), mem);
	else
		cc.movss(FP_REG_W(target), mem);
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

	TemporaryRegister ad(*this, s, TypeInfo(Types::ID::Block));
	int ok = cc.mov(ad.get(), void2ptr(data));
	auto target = x86::qword_ptr(ad.get());

	cc.mov(target, lineNumber);
}

void AsmCodeGenerator::emitLoopControlFlow(Operations::Loop* parentLoop, bool isBreak)
{
	auto l = parentLoop->loopEmitter->getLoopPoint(!isBreak);

	jassert(l.isLabel());
	jassert(l.isValid());

	cc.jmp(l);
}

void AsmCodeGenerator::fillSignature(const FunctionData& data, FuncSignatureX& sig, Types::ID objectType)
{
	if (data.returnType.isRef())
	{
		sig.setRetT<PointerType>();
	}
	else
	{
		if (data.returnType == Types::ID::Float) sig.setRetT<float>();
		if (data.returnType == Types::ID::Double) sig.setRetT<double>();
		if (data.returnType == Types::ID::Integer) sig.setRetT<int>();
		if (data.returnType == Types::ID::Block) sig.setRetT<PointerType>();
		if (data.returnType == Types::ID::Pointer) sig.setRetT<PointerType>();
	}

	

	if (objectType != Types::ID::Void)
	{
		if(objectType == Types::ID::Integer) sig.addArgT<int>();
		if (objectType == Types::ID::Float) sig.addArgT<float>();
		if (objectType == Types::ID::Double) sig.addArgT<double>();
		if (objectType == Types::ID::Pointer) sig.addArgT<PointerType>();
	}
		

	for (auto p : data.args)
	{
		auto t = p.typeInfo.getType();

		if (p.typeInfo.isRef())
		{
			sig.addArgT<PointerType>();
		}
		else
		{
			if (t == Types::ID::Float)	 sig.addArgT<float>();
			if (t == Types::ID::Double)  sig.addArgT<double>();
			if (t == Types::ID::Integer) sig.addArgT<int>();
			if (t == Types::ID::Block)   sig.addArgT<PointerType>();
			if (t == Types::ID::Pointer) sig.addArgT<PointerType>();
		}
	}
}


asmjit::X86Mem AsmCodeGenerator::createStack(Types::ID t)
{
	auto s = Types::Helpers::getSizeForType(t);

	auto st = cc.newStack(s, s);
	st.setSize(s);
	return st;
}

void SpanLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	jassert(loopTarget->getType() == Types::ID::Pointer);
	jassert(loopTarget->getScope() != nullptr);
	jassert(typePtr != nullptr);
	jassert(iterator.typeInfo == typePtr->getElementType());
    
	int numLoops = typePtr->getNumElements();


	auto& cc = gen.cc;
    auto start = cc.newGpq();
    auto end = cc.newGpq();

	auto offset = typePtr->getElementSize() * (typePtr->getNumElements());

    if(loopTarget->isSimd4Float())
    {
        auto ptr = loopTarget->getMemoryLocationForReference();
        
        int e = 0;
        
        
        if(!ptr.hasBaseReg())
        {
            int64_t startOffset = ptr.offset();
            int64_t endOffset = startOffset + offset;
            e = cc.mov(start, startOffset);
        
            e = cc.mov(end, endOffset);
        }
        else
        {
            auto startMem = AsmCodeGenerator::createValid64BitPointer(cc, ptr, 0, typePtr->getElementSize());
            auto endMem = AsmCodeGenerator::createValid64BitPointer(cc, ptr, offset, typePtr->getElementSize());
            
            cc.lea(start, startMem);
            cc.lea(end, endMem);

        }
        
        jassert(e == 0);
    }
    else
    {
        if (loopTarget->isMemoryLocation())
            cc.lea(start, loopTarget->getAsMemoryLocation());
        else
            cc.mov(start, PTR_REG_R(loopTarget));
        
        cc.lea(end, x86::ptr(start, offset));
    }
    
	auto loopStart = cc.newLabel();
	continuePoint = cc.newLabel();
	loopEnd = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);

	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop_span {");
	cc.bind(loopStart);

    itReg->setCustomMemoryLocation(x86::ptr(start), false);

	if (loadIterator)
	{
        itReg->loadMemoryIntoRegister(cc);
        
#if 0
		IF_(int)    cc.mov(INT_REG_W(itReg), x86::dword_ptr(start));
		IF_(float)  cc.movss(FP_REG_W(itReg), x86::dword_ptr(start));
		IF_(double) cc.movsd(FP_REG_W(itReg), x86::qword_ptr(start));
		IF_(void*)
		{
			if (itReg->isSimd4Float())
				cc.movaps(FP_REG_W(itReg), x86::ptr(start));
			else
				cc.mov(PTR_REG_W(itReg), start);
		}
#endif

	}

	itReg->setUndirty();

	loopBody->process(compiler, scope);


    
	cc.bind(continuePoint);

	if (itReg->isDirtyGlobalMemory())
	{
		IF_(int)    cc.mov(x86::ptr(start), INT_REG_R(itReg));
		IF_(float)  cc.movss(x86::ptr(start), FP_REG_R(itReg));
		IF_(double) cc.movsd(x86::ptr(start), FP_REG_R(itReg));
		IF_(void*)
		{
			if (itReg->isSimd4Float())
				cc.movaps(x86::ptr(start), FP_REG_R(itReg));
			else
				jassertfalse; // cc.mov(x86::ptr(INT_REG_R(loopTarget), INT_REG_W(itReg));
		}
		// no storing needed for pointer iterators...
	}


    
	cc.add(start, (int64_t)typePtr->getElementSize());
	cc.cmp(start, end);
	cc.setInlineComment("loop_span }");
	cc.jne(loopStart);

	cc.bind(loopEnd);

	itReg->setUndirty();
	itReg->flagForReuse(true);
    
}


void DynLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	jassert(loopTarget->getType() == Types::ID::Pointer);
	jassert(loopTarget->getScope() != nullptr);
	
	jassert(typePtr != nullptr);
	jassert(iterator.typeInfo == typePtr->elementType);

	AsmCodeGenerator::TemporaryRegister beg(gen, loopTarget->getScope(), loopTarget->getTypeInfo());
	AsmCodeGenerator::TemporaryRegister end(gen, loopTarget->getScope(), loopTarget->getTypeInfo());

	auto& cc = gen.cc;

	X86Mem fatPointerAddress;
	
	
	if (loopTarget->hasCustomMemoryLocation() || loopTarget->isMemoryLocation())
		fatPointerAddress = loopTarget->getMemoryLocationForReference();
	else
		fatPointerAddress = x86::ptr(PTR_REG_R(loopTarget));
	
	auto sizeAddress = fatPointerAddress.cloneAdjusted(4);
	sizeAddress.setSize(4);
	auto objectAddress = fatPointerAddress.cloneAdjusted(8);
	objectAddress.setSize(8);
	

	auto elementSize = (uint32)typePtr->elementType.getRequiredByteSize();

	auto sizeReg = cc.newGpd();
	cc.mov(sizeReg, sizeAddress);
	cc.imul(sizeReg, elementSize);

	cc.mov(beg.get(), objectAddress);
	cc.lea(end.get(), x86::ptr(beg.get(), sizeReg));

	auto loopStart = cc.newLabel();
	continuePoint = cc.newLabel();
	loopEnd = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);
	itReg->createRegister(cc);
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop_span {");
	cc.bind(loopStart);

	if (loadIterator)
	{
		IF_(int)    cc.mov(INT_REG_W(itReg), x86::ptr(beg.get()));
		IF_(float)  cc.movss(FP_REG_W(itReg), x86::ptr(beg.get()));
		IF_(double) cc.movsd(FP_REG_W(itReg), x86::ptr(beg.get()));
		IF_(void*)
		{
			if (itReg->isSimd4Float())
				cc.movaps(FP_REG_W(itReg), x86::ptr(beg.get()));
			else
				cc.mov(PTR_REG_W(itReg), beg.get());
		}
	}

	itReg->setUndirty();

	loopBody->process(compiler, scope);

	cc.bind(continuePoint);

	if (itReg->isDirtyGlobalMemory())
	{
		IF_(int)    cc.mov(x86::ptr(beg.get()), INT_REG_R(itReg));
		IF_(float)  cc.movss(x86::ptr(beg.get()), FP_REG_R(itReg));
		IF_(double) cc.movsd(x86::ptr(beg.get()), FP_REG_R(itReg));
		IF_(void*)
		{
			jassert(itReg->isSimd4Float());
			cc.movaps(x86::ptr(beg.get()), FP_REG_R(itReg));
		}
	}

	cc.add(beg.get(), (int64_t)elementSize);
	cc.cmp(beg.get(), end.get());
	cc.setInlineComment("loop_span }");
	cc.jl(loopStart);

	cc.bind(loopEnd);

	itReg->setUndirty();
	itReg->flagForReuse(true);
    
}

void CustomLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	auto& cc = gen.cc;


	AsmInlineData d(gen);
	d.object = loopTarget;
	
	AsmCodeGenerator::TemporaryRegister beginReg(gen, loopTarget->getScope(), TypeInfo(Types::ID::Pointer, true));
	AsmCodeGenerator::TemporaryRegister sizeReg(gen, loopTarget->getScope(), sizeFunction.returnType);
	AsmCodeGenerator::TemporaryRegister endReg(gen, loopTarget->getScope(), TypeInfo(Types::ID::Pointer, true));

	auto elementSize = beginFunction.returnType.getRequiredByteSize();

	d.target = beginReg.tempReg;

	if (auto s = loopTarget->getTypeInfo().getTypedIfComplexType<StructType>())
		d.templateParameters = s->getTemplateInstanceParameters();

	beginFunction.inlineFunction(&d);

	d.target = sizeReg.tempReg;

	sizeFunction.inlineFunction(&d);

	if (sizeReg.tempReg->isUnloadedImmediate())
	{
		auto offset = sizeReg.tempReg->getImmediateIntValue();
		cc.lea(endReg.get(), x86::ptr(beginReg.get()).cloneAdjustedAndResized(offset * elementSize, elementSize));
	}
	else
	{
		auto shift = log2(elementSize);
		cc.lea(endReg.get(), x86::ptr(beginReg.get(), sizeReg.get(), shift, 0, elementSize));
	}

	auto loopStart = cc.newLabel();
	continuePoint = cc.newLabel();
	loopEnd = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);
	itReg->createRegister(cc);
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop_span {");
	cc.bind(loopStart);

	if (loadIterator)
	{
		IF_(int)    cc.mov(INT_REG_W(itReg), x86::ptr(beginReg.get()));
		IF_(float)  cc.movss(FP_REG_W(itReg), x86::ptr(beginReg.get()));
		IF_(double) cc.movsd(FP_REG_W(itReg), x86::ptr(beginReg.get()));
		IF_(void*)
		{
			if (itReg->isSimd4Float())
				cc.movaps(FP_REG_W(itReg), x86::ptr(beginReg.get()));
			else
				cc.mov(PTR_REG_W(itReg), beginReg.get());
		}
	}

	itReg->setUndirty();

	loopBody->process(compiler, scope);


	cc.bind(continuePoint);

	if (itReg->isDirtyGlobalMemory())
	{
		IF_(int)    cc.mov(x86::ptr(beginReg.get()), INT_REG_R(itReg));
		IF_(float)  cc.movss(x86::ptr(beginReg.get()), FP_REG_R(itReg));
		IF_(double) cc.movsd(x86::ptr(beginReg.get()), FP_REG_R(itReg));
		IF_(void*)
		{
			jassert(itReg->isSimd4Float());
			cc.movaps(x86::ptr(beginReg.get()), FP_REG_R(itReg));
		}
	}

	cc.add(beginReg.get(), (int64_t)elementSize);
	cc.cmp(beginReg.get(), endReg.get());
	cc.setInlineComment("loop_span }");
	cc.jl(loopStart);

	cc.bind(loopEnd);

	itReg->setUndirty();
	itReg->flagForReuse(true);



}

}

}
