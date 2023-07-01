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


#if SNEX_ASMJIT_BACKEND
AsmCodeGenerator::AsmCodeGenerator(Compiler& cc_, AssemblyRegisterPool* pool, Types::ID type_, ParserHelpers::CodeLocation l, const StringArray& opt) :
	cc(cc_),
	type(type_),
	registerPool(pool),
	location(l),
	optimizations(opt)
{
	
}

void AsmCodeGenerator::emitComment(const char* m)
{
	cc.setInlineComment(m);
}

snex::jit::AsmCodeGenerator::RegPtr AsmCodeGenerator::emitLoadIfNativePointer(RegPtr source, Types::ID nativeType)
{
	

	auto sourceRegType = source->getTypeInfo();

	if (sourceRegType.isNativePointer())
	{
		AsmCodeGenerator::TemporaryRegister tmpReg(*this, source->getScope(), nativeType);

		source->loadMemoryIntoRegister(cc);
		tmpReg.tempReg->setCustomMemoryLocation(x86::ptr(PTR_REG_R(source)), source->isGlobalMemory());
		source = tmpReg.tempReg;
	}

	return source;
}

void AsmCodeGenerator::emitStore(RegPtr target, RegPtr value)
{
	if (target == value)
	{
		writeRegisterToMemory(value);
		return;
	}

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

			if (target->shouldWriteToMemoryAfterStore())
				writeRegisterToMemory(target);

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
					auto mem = cc.newConst(ConstPoolScope::kGlobal, d.getData(), d.size());

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
			target->loadMemoryIntoRegister(cc);

			if (value->isMemoryLocation())
				cc.lea(PTR_REG_W(target), value->getAsMemoryLocation());
			else
				cc.lea(PTR_REG_W(target), x86::ptr(PTR_REG_R(value)));
		}
	}
	
	if (target->shouldWriteToMemoryAfterStore())
		writeRegisterToMemory(target);
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

	IF_(int)	ok = cc.mov(target.cloneResized(4), INT_REG_R(source));
	IF_(float)	ok = cc.movss(target.cloneResized(4), FP_REG_R(source));
	IF_(double) ok = cc.movsd(target, FP_REG_R(source));
	
	if (source->isSimd4Float())
	{
		cc.movaps(target, FP_REG_R(source));
	}

	// Don't undirty it when debugging...
	if (ptrToUse == nullptr)
		source->setUndirty();
}






void AsmCodeGenerator::writeRegisterToMemory(RegPtr p)
{
	if (p->hasCustomMemoryLocation() && p->isActive())
	{
		auto mem = p->getMemoryLocationForReference();

		IF_(int) cc.mov(mem, INT_REG_R(p));
		IF_(float) cc.movss(mem, FP_REG_R(p));
		IF_(double) cc.movsd(mem, FP_REG_W(p));
	}

	if (p->isGlobalMemory() && p->isActive())
	{
		emitMemoryWrite(p);
	}
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
	auto sType = source->getTypeInfo().getComplexType();

	jassert(sType == type);

	const bool allow16ByteCopy = false;

	auto ptr = target->getAsMemoryLocation();
	auto numBytesToCopy = type->getRequiredByteSize();

	int wordSize = 0;

	if (allow16ByteCopy && numBytesToCopy % 16 == 0)
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

	cc.setInlineComment("Copy complex type argument");

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

	int byteSize = jmin<int>(8, (int)target->getTypeInfo().getRequiredByteSize());

	if (parent->isMemoryLocation())
	{
        auto p = createValid64BitPointer(cc, parent->getAsMemoryLocation(), memberOffset.toInt(), byteSize);
        target->setCustomMemoryLocation(p, parent->isGlobalMemory());
	}
	else
	{
		if (!parent->isActive())
		{
			location.throwError("this pointer is unresolved");
		}

		auto ptr = x86::ptr(PTR_REG_R(parent), memberOffset.toInt());
		target->setCustomMemoryLocation(ptr.cloneResized(byteSize), parent->isGlobalMemory());
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

#define FLOAT_BINARY_OP(token, floatOp, doubleOp) if(op == token) { IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

#define BINARY_OP(token, intOp, floatOp, doubleOp) if(op == token) { IF_(int) INT_OP(intOp, l, r); IF_(float) FP_OP(floatOp, l, r); IF_(double) FP_OP(doubleOp, l, r); }

void AsmCodeGenerator::emitParameter(Operations::Function* f, FuncNode* fn, RegPtr parameterRegister, int parameterIndex)
{
	emitParameter(f->data, fn, parameterRegister, parameterIndex, f->hasObjectPtr);
}

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
		if (op == JitTokens::assign_)
			FP_OP(cc.movaps, l, r);
	}
	else
	{
		l = emitLoadIfNativePointer(l, type);
		r = emitLoadIfNativePointer(r, type);

		jassert(l != nullptr && r != nullptr);
		jassert(l->getType() != Types::ID::Pointer);

		l->loadMemoryIntoRegister(cc);

		if (type == Types::ID::Integer && (op == JitTokens::modulo || op == JitTokens::divide))
		{
			if (r->isImmediate() && r->getImmediateIntValue() != 0 && isPowerOfTwo(r->getImmediateIntValue()))
			{
				if (op == JitTokens::modulo)
				{
					auto mask = (uint64_t)r->getImmediateIntValue() - 1;
					cc.and_(INT_REG_W(l), mask);
				}
				else
				{
					auto shiftAmount = (uint64_t)log2(r->getImmediateIntValue());
					cc.shr(INT_REG_W(l), shiftAmount);
				}
			}
			else
			{
				auto gs = r->getScope()->getGlobalScope();
				auto checkZeroDivision = gs->isRuntimeErrorCheckEnabled();
				auto okBranch = cc.newLabel();
				auto errorBranch = cc.newLabel();

				if (checkZeroDivision)
				{
					if (r->isMemoryLocation())
					{
						if (r->getImmediateIntValue() == 0)
							location.throwError("Divide by zero");
					}
					else
					{
						cc.cmp(INT_REG_R(r), 0);
						cc.je(errorBranch);
					}
				}

				TemporaryRegister dummy(*this, r->getScope(), TypeInfo(Types::ID::Integer));
				cc.cdq(dummy.get(), INT_REG_W(l));

				if (r->isMemoryLocation())
				{
					auto forcedMemory = cc.newInt32Const(ConstPoolScope::kLocal, static_cast<int>(INT_IMM(r)));
					cc.idiv(dummy.get(), INT_REG_W(l), forcedMemory);
				}
				else
					cc.idiv(dummy.get(), INT_REG_W(l), INT_REG_R(r));

				if (op == JitTokens::modulo)
					cc.mov(INT_REG_W(l), dummy.get());

				if (checkZeroDivision)
				{
					cc.jmp(okBranch);
					cc.bind(errorBranch);

					auto flagReg = cc.newGpd();

					auto errorFlagReg = cc.newGpq();
					cc.mov(errorFlagReg, gs->getRuntimeErrorFlag());

					auto errorFlag = x86::ptr(errorFlagReg).cloneResized(4);

					

					cc.mov(flagReg, (int)RuntimeError::ErrorType::IntegerDivideByZero);
					cc.mov(errorFlag, flagReg);

					cc.mov(flagReg, (int)location.getLine());
					cc.mov(errorFlag.cloneAdjustedAndResized(4, 4), flagReg);
					cc.mov(flagReg, (int)location.getColNumber());
					cc.mov(errorFlag.cloneAdjustedAndResized(8, 4), flagReg);

					cc.mov(INT_REG_W(l), 0);
					cc.bind(okBranch);
				}
			}
		}
		else
		{
			BINARY_OP(JitTokens::plus, cc.add, cc.addss, cc.addsd);
			BINARY_OP(JitTokens::minus, cc.sub, cc.subss, cc.subsd);
			BINARY_OP(JitTokens::times, cc.imul, cc.mulss, cc.mulsd);
			FLOAT_BINARY_OP(JitTokens::divide, cc.divss, cc.divsd);
			BINARY_OP(JitTokens::assign_, cc.mov, cc.movss, cc.movsd);
		}
	}
	
	auto target = l->getMemoryLocationForReference();

	if (target.hasBase())
	{
		IF_(int)		cc.mov(target, INT_REG_R(l));
		IF_(float)		cc.movss(target, FP_REG_R(l));
		IF_(double)		cc.movsd(target, FP_REG_R(l));
		IF_(void*)
		{
			jassert(l->isSimd4Float());
			cc.movaps(target, FP_REG_R(l));
		}

		if (l->isDirtyGlobalMemory())
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

			target->setDirtyFloat4(address, idx * sizeof(float));

			return;
		}
		else
		{
			auto m = cc.newStack((uint32_t)address->getTypeInfo().getRequiredByteSize(), (uint32_t)address->getTypeInfo().getRequiredAlignment());

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

		// We can't use the register for a named variable because
		// it might get changed later on before the pointer is loaded
		if (index->getVariableId().resolved)
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
            p = ptr.cloneAdjustedAndResized(imm2ptr(INT_IMM(index) * (int64_t)elementSizeInBytes), jmin<int>((int)elementSizeInBytes, 8));
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

	
	if (type == Types::ID::Pointer)
	{
        if(target->isSimd4Float())
        {
			target->setCustomMemoryLocation(p, address->isGlobalMemory());
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
		auto leftWasDirty = l->isDirtyGlobalMemory();

		INT_OP(cc.cmp, l, r);

		if (!leftWasDirty)
			l->setUndirty();

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

		X86Mem ptr;

		if (expr->isMemoryLocation())
		{
			ptr = expr->getAsMemoryLocation();
			target->createRegister(cc);
			cc.lea(PTR_REG_W(target), ptr);
			cc.ret(PTR_REG_W(target));
		}
		else
		{
			//ptr = x86::ptr(PTR_REG_R(expr)).cloneResized(expr->getTypeInfo().getRequiredByteSize());
			cc.ret(PTR_REG_W(expr));
		}
		
		return;
	}

	if (expr != nullptr && (expr->isMemoryLocation() || expr->isDirtyGlobalMemory()))
	{
		if (auto am = c->registerPool.getActiveRegisterForCustomMem(expr))
		{
			rToUse = am;
			 am->loadMemoryIntoRegister(cc);
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
		// We need to check if the actual register is an address (eg. a function call that returns a reference)
		bool needsPointerToTypeConversion = target->getType() != Types::ID::Pointer && expr->getType() == Types::ID::Pointer;

		if (needsPointerToTypeConversion)
		{
			X86Mem ptr;

			if (expr->isMemoryLocation())
				ptr = expr->getAsMemoryLocation();
			else
				ptr = x86::ptr(PTR_REG_R(expr)).cloneResized((uint32_t)expr->getTypeInfo().getRequiredByteSize());

			IF_(int)
				cc.mov(INT_REG_W(rToUse), ptr);
			IF_(float)
				cc.movss(FP_REG_W(rToUse), ptr);
			IF_(double)
				cc.movsd(FP_REG_W(rToUse), ptr);
		}

		jassert(rToUse->isActive());

		switch (type)
		{
		case Types::ID::Float:
		case Types::ID::Double: 
			cc.ret(FP_REG_R(rToUse)); break;
		case Types::ID::Integer:
			cc.ret(INT_REG_R(rToUse)); break;
		case Types::ID::Block:
		case Types::ID::Pointer:
			cc.ret(PTR_REG_R(rToUse));
			break;
		default:
			jassertfalse;
		}

		rToUse->clearAfterReturn();
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

Result AsmCodeGenerator::emitStackInitialisation(RegPtr target, ComplexType::Ptr typePtr, RegPtr expr, InitialiserList::Ptr list)
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

			return typePtr->initialise(d);
		}
		else
		{
			if (typePtr->getRequiredByteSize() == 0)
			{

				jassert(target->isMemoryLocation());
				auto mem = target->getAsMemoryLocation().cloneResized(1);
				cc.mov(mem, 0);
				return Result::ok();
			}

			HeapBlock<uint8> data;
			data.allocate(typePtr->getRequiredByteSize() + 16, true);

            int alignOffset = 0;
            
            if((uint64_t)data.get() % 16 != 0)
                alignOffset = 16 - ((uint64_t)data.get() % 16);
            
            auto start = data.get() + alignOffset;
			auto end = start + typePtr->getRequiredByteSize();
			int numBytesToInitialise = end - start;

			ComplexType::InitData initData;
			initData.dataPointer = start;
			initData.initValues = list;

			// Do not call the constructor, since it will be called later on...
			initData.callConstructor = false;

			auto r = typePtr->initialise(initData);

			bool isNonZero = false;

			for (int i = 0; i < numBytesToInitialise; i++)
			{
				if (start[i] != 0)
				{
					isNonZero = true;
					break;
				}
			}

			if (!r.wasOk())
				return r;

			auto tmpSSE = cc.newXmm();

			if(numBytesToInitialise >= 16 && !isNonZero)
				cc.xorps(tmpSSE, tmpSSE);

			auto dst = target->getAsMemoryLocation().cloneResized(16);

			while (numBytesToInitialise >= 16)
			{
				jassert(reinterpret_cast<uint64_t>(start) % 16 == 0);

				if (isNonZero)
				{
					uint64_t* s = reinterpret_cast<uint64_t*>(start);

					auto d = Data128::fromU64(s[0], s[1]);
					auto c = cc.newConst(ConstPoolScope::kLocal, d.getData(), d.size());
					cc.movaps(tmpSSE, c);
				}
				
				cc.movaps(dst, tmpSSE);
				dst = dst.cloneAdjusted(16);
				numBytesToInitialise -= 16;
				start += 16;
			}

			dst = dst.cloneResized(4);

			while (numBytesToInitialise >= 4)
			{
				jassert(reinterpret_cast<uint64_t>(start) % 4 == 0);

				if (isNonZero)
				{
					cc.mov(dst, *reinterpret_cast<int*>(start));
				}
				else
					cc.mov(dst, 0);

				dst = dst.cloneAdjusted(4);
				start += 4;
				numBytesToInitialise -= 4;
			}
			
			return Result::ok();
		}
	}
	else
	{
		auto numToCopy = (int)typePtr->getRequiredByteSize();

		target->loadMemoryIntoRegister(cc, true);
		expr->loadMemoryIntoRegister(cc, true);

		auto dst = x86::ptr(PTR_REG_R(target));
		auto src = x86::ptr(PTR_REG_R(expr));

		int offset = 0;

		cc.setInlineComment("Copy complex type");

#if 0
		while (numToCopy >= 16)
		{
			auto tmp = cc.newXmmPs();
			cc.movaps(tmp, src.cloneAdjustedAndResized(offset, 16));
			cc.movaps(dst.cloneAdjustedAndResized(offset, 16), tmp);

			numToCopy -= 16;
			offset += 16;
		}
#endif

		while(numToCopy >= 8)
		{
			auto tmp = cc.newGpq();

			cc.mov(tmp, src.cloneAdjustedAndResized(offset, 8));
			cc.mov(dst.cloneAdjustedAndResized(offset, 8), tmp);

			numToCopy -= 8;
			offset += 8;
		}

		while (numToCopy >= 4)
		{
			auto tmp = cc.newGpd();

			cc.mov(tmp, src.cloneAdjustedAndResized(offset, 4));
			cc.mov(dst.cloneAdjustedAndResized(offset, 4), tmp);

			numToCopy -= 4;
			offset += 4;
		}

		return Result::ok();
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


	if (returnType == Types::ID::Void)
	{
		Operations::ConditionalBranch::preallocateVariableRegistersBeforeBranching(trueBranch, c, s);

		if (falseBranch != nullptr)
			Operations::ConditionalBranch::preallocateVariableRegistersBeforeBranching(falseBranch, c, s);
	}


	if (condition->reg->isMemoryLocation())
	{
		auto dummy = c->registerPool.getNextFreeRegister(s, TypeInfo(Types::ID::Integer));
		dummy->createRegister(cc);

		if (condition->reg->hasCustomMemoryLocation())
		{
			cc.mov(dummy->getRegisterForWriteOp().as<X86Gp>(), condition->reg->getAsMemoryLocation());
		}
		else
		{
			cc.mov(dummy->getRegisterForWriteOp().as<X86Gp>(), imm2ptr(condition->reg->getImmediateIntValue()));
		}

		cc.cmp(dummy->getRegisterForReadOp().as<X86Gp>(), 0);

#if REMOVE_REUSABLE_REG
		dummy->flagForReuse(true);
#endif
	}
	else
		cc.cmp(INT_REG_R(condition->reg), 0);

	cc.setInlineComment("Branch test");
	cc.jnz(l);

#if REMOVE_REUSABLE_REG
	condition->reg->flagForReuseIfAnonymous();
#endif

	if (falseBranch != nullptr)
	{
		cc.setInlineComment("false branch");

		falseBranch->process(c, s);

		writeDirtyGlobals(c);

		if (returnReg != nullptr)
		{
			if (auto fAsExpr = dynamic_cast<Operations::Expression*>(falseBranch))
			{
				emitStore(returnReg, fAsExpr->reg);

#if REMOVE_REUSABLE_REG
				fAsExpr->reg->flagForReuseIfAnonymous();
#endif
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
#if REMOVE_REUSABLE_REG
			tAsExpr->reg->flagForReuseIfAnonymous();
#endif
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

		emitStore(target, expr);
	}
	else
	{
		jassert(target != nullptr);
		expr->loadMemoryIntoRegister(cc);

		emitStore(target, expr);

		if (isDecrement)
			cc.add(INT_REG_W(expr), -1);
		else
			cc.add(INT_REG_W(expr), 1);
	}
}

void AsmCodeGenerator::emitCast(RegPtr target, RegPtr expr, Types::ID sourceType)
{
	expr = emitLoadIfNativePointer(expr, sourceType);

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
			
			if (IS_REG(expr))    cc.cvtsi2sd(FP_REG_W(target), INT_REG_R(expr));
			else if (IS_IMM(expr))
			{
				auto m = cc.newDoubleConst(ConstPoolScope::kLocal, static_cast<double>(INT_IMM(expr)));
				target->setCustomMemoryLocation(m, false);
			}
			else if (IS_CMEM(expr) || IS_MEM(expr))   
				cc.cvtsi2sd(FP_REG_W(target), INT_MEM(expr));
			else
				cc.cvtsi2sd(FP_REG_W(target), INT_REG_R(expr));
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
			if     (IS_REG(expr))    cc.cvtsi2ss(FP_REG_W(target), INT_REG_R(expr));
			else if (IS_IMM(expr))
			{
				auto m = cc.newFloatConst(ConstPoolScope::kLocal, static_cast<float>(INT_IMM(expr)));
				target->setCustomMemoryLocation(m, false);
			}
		    else if(IS_CMEM(expr) || IS_MEM(expr))   cc.cvtsi2ss(FP_REG_W(target), INT_MEM(expr));
			else                     
				cc.cvtsi2ss(FP_REG_W(target), INT_REG_R(expr));
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
		cc.mulss(FP_REG_W(target), cc.newFloatConst(ConstPoolScope::kLocal, -1.0f));
	IF_(float)
		cc.mulsd(FP_REG_W(target), cc.newDoubleConst(ConstPoolScope::kLocal, -1.0));
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

	if (!f.isConstOrHasConstArgs())
	{
		for (auto vr : registerPool->getListOfAllNamedRegisters())
		{
			RegPtr p(vr);

			// Not loaded yet
			if (!vr->isActive())
				continue;

			// Pointers can't be changed
			if (p->getType() == Types::ID::Pointer)
				continue;

			// most likely a parameter register or a local native type
			if (!p->hasCustomMemoryLocation() && !p->isGlobalMemory())
				continue;

			// We don't know if the function might alter this register, 
			// so we have to free it
			p->clearAfterReturn();
		}
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
	
	auto scope = returnReg != nullptr ? returnReg->getScope() : objectAddress->getScope();

	TemporaryRegister o(*this, scope, TypeInfo(Types::ID::Block));
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

    InvokeNode* call;
    
	cc.invoke(&call, (uint64_t)f.function, sig);

	//call->setInlineComment(f.functionName.getCharPointer().getAddress());

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

	auto isStackVariable = !sourceReg->isGlobalMemory();

	if (isStackVariable && !sourceReg->hasCustomMemoryLocation())
	{
		jassert(sourceReg->isActive());

		auto byteSize = Types::Helpers::getSizeForType(type);
		mem = cc.newStack((uint32_t)byteSize, 0);

		IF_(int) cc.mov(mem, INT_REG_R(sourceReg));
		IF_(float) cc.movss(mem, FP_REG_R(sourceReg));
		IF_(double) cc.movsd(mem, FP_REG_R(sourceReg));

		sourceReg->setCustomMemoryLocation(mem, false);
	}

	cc.lea(INT_REG_R(parameterReg), mem);
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
			auto c = cc.newFloatConst(asmjit::ConstPoolScope::kLocal, v.toFloat());
			cc.movss(FP_REG_W(target), c);
		}
		IF_(double)
		{
			auto c = cc.newFloatConst(asmjit::ConstPoolScope::kLocal, v.toDouble());
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

void AsmCodeGenerator::writeRegisterToMemory(RegPtr target, RegPtr source)
{
    // The target must be a register holding a pointer value
    jassert(target->getType() == Types::ID::Pointer);
    
    auto sourceType = source->getType();
    source->loadMemoryIntoRegister(cc);
    
    auto ptr = x86::ptr(PTR_REG_R(target));
    
    switch(sourceType)
    {
        case Types::ID::Integer: cc.mov(ptr, INT_REG_R(source)); break;
        case Types::ID::Float: cc.movss(ptr, FP_REG_R(source)); break;
        case Types::ID::Double: cc.movsd(ptr, FP_REG_R(source)); break;
    }
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
	cc.mov(ad.get(), void2ptr(data));
	auto target = x86::qword_ptr(ad.get());

	cc.mov(target, lineNumber);
}

void AsmCodeGenerator::emitLoopControlFlow(Operations::ConditionalBranch* parentLoop, bool isBreak)
{
	auto l = parentLoop->getJumpTargetForEnd(!isBreak);

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
	auto s = (uint32_t)Types::Helpers::getSizeForType(t);

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
            auto startMem = AsmCodeGenerator::createValid64BitPointer(cc, ptr, 0, (int)typePtr->getElementSize());
            auto endMem = AsmCodeGenerator::createValid64BitPointer(cc, ptr, (int)offset, (int)typePtr->getElementSize());
            
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
        
        cc.lea(end, x86::ptr(start, (uint32_t)offset));
    }
    
	auto loopStart = cc.newLabel();
	continuePoint = cc.newLabel();
	loopEnd = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);

	//itReg->setIsIteratorRegister(true);
	itReg->setCustomMemoryLocation(x86::ptr(start), loopTarget->isGlobalMemory());
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop {");
	cc.bind(loopStart);

	loopBody->process(compiler, scope);
    
	cc.bind(continuePoint);

	cc.add(start, (int64_t)typePtr->getElementSize());
	cc.cmp(start, end);
	cc.setInlineComment("loop}");
	cc.jne(loopStart);

	cc.bind(loopEnd);

#if REMOVE_REUSABLE_REG
	itReg->setUndirty();
	itReg->flagForReuse(true);
#endif
    
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
	//itReg->createRegister(cc);
	itReg->setIsIteratorRegister(true);

	cc.setInlineComment("loop {");
	cc.bind(loopStart);

	itReg->setCustomMemoryLocation(x86::ptr(beg.get()), loopTarget->isGlobalMemory());

#if 0
	if (loadIterator)
	{
		


#if 0
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
#endif
	}

	itReg->setUndirty();
#endif

	loopBody->process(compiler, scope);

	cc.bind(continuePoint);

	jassert(!itReg->isDirtyGlobalMemory());
#if 0
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
#endif

	cc.add(beg.get(), (int64_t)elementSize);
	cc.cmp(beg.get(), end.get());
	cc.setInlineComment("loop }");
	cc.jl(loopStart);

	cc.bind(loopEnd);

	//itReg->setUndirty();
	//itReg->flagForReuse(true);
    
}

void CustomLoopEmitter::emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope)
{
	auto& cc = gen.cc;
	AsmInlineData d(gen);
	d.object = loopTarget;
	
	AsmCodeGenerator::TemporaryRegister beginReg(gen, loopTarget->getScope(), TypeInfo(Types::ID::Pointer, true));
	AsmCodeGenerator::TemporaryRegister sizeReg(gen, loopTarget->getScope(), sizeFunction.returnType);
	AsmCodeGenerator::TemporaryRegister endReg(gen, loopTarget->getScope(), TypeInfo(Types::ID::Pointer, true));

	d.target = sizeReg.tempReg;
	sizeFunction.inlineFunction(&d);

	bool isSingleElementLoop = sizeReg.tempReg->isUnloadedImmediate() && sizeReg.tempReg->getImmediateIntValue() == 1;

	auto elementSize = beginFunction.returnType.getRequiredByteSize();

	d.target = beginReg.tempReg;

	if (auto s = loopTarget->getTypeInfo().getTypedIfComplexType<StructType>())
		d.templateParameters = s->getTemplateInstanceParameters();

	beginFunction.inlineFunction(&d);

	beginReg.tempReg->loadMemoryIntoRegister(cc);

	if (isSingleElementLoop)
	{

	}
	else if (sizeReg.tempReg->isUnloadedImmediate())
	{
		auto offset = sizeReg.tempReg->getImmediateIntValue();

		cc.lea(endReg.get(), x86::ptr(beginReg.get()).cloneAdjustedAndResized(offset * (int64_t)elementSize, (uint32_t)elementSize));
	}
	else
	{
		cc.imul(sizeReg.get(), (int)elementSize);

		cc.mov(endReg.get(), beginReg.get());
		cc.add(endReg.get(), sizeReg.get().r64());
	}

	auto loopStart = cc.newLabel();
	continuePoint = cc.newLabel();
	loopEnd = cc.newLabel();
	auto itScope = loopBody->blockScope.get();

	auto itReg = compiler->registerPool.getRegisterForVariable(itScope, iterator);

	itReg->setCustomMemoryLocation(x86::ptr(beginReg.get()), loopTarget->isGlobalMemory());
	
	itReg->setIsIteratorRegister(true);

	if (!isSingleElementLoop)
	{
		cc.setInlineComment("loop {");
		cc.bind(loopStart);
	}
	
	loopBody->process(compiler, scope);

	if (!isSingleElementLoop)
	{
		cc.bind(continuePoint);

		cc.add(beginReg.get(), (int64_t)elementSize);
		cc.cmp(beginReg.get(), endReg.get());
		cc.setInlineComment("loop }");
		cc.jl(loopStart);

		cc.bind(loopEnd);
	}
}
#endif

}

}
