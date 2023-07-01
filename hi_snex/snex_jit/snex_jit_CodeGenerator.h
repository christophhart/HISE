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


#define FP_REG_W(x) x->getRegisterForWriteOp().as<X86Xmm>()
#define FP_REG_R(x) x->getRegisterForReadOp().as<X86Xmm>()
#define FP_MEM(x) x->getAsMemoryLocation()
#define IS_MEM(x) x->isMemoryLocation()
#define IS_IMM(x) x->isImmediate()
#define IS_CMEM(x) x->hasCustomMemoryLocation() && !x->isActive()
#define IS_REG(x)  x->isActive()


#define INT_REG_W(x) x->getRegisterForWriteOp().as<X86Gp>()
#define INT_REG_R(x) x->getRegisterForReadOp().as<X86Gp>()
#define INT_IMM(x) x->getImmediateIntValue()
#define INT_MEM(x) x->getAsMemoryLocation()

#define PTR_REG_W(x) x->getRegisterForWriteOp().as<X86Gpq>()
#define PTR_REG_R(x) x->getRegisterForReadOp().as<X86Gpq>()

#define MEMBER_PTR(x) base.cloneAdjustedAndResized((uint32_t)obj->getMemberOffset(#x), (uint32_t)obj->getMemberTypeInfo(#x).getRequiredByteSize())

#define INT_OP_WITH_MEM(op, l, r) { if(IS_MEM(r)) op(INT_REG_W(l), INT_MEM(r)); else op(INT_REG_W(l), INT_REG_R(r)); }



#define FP_OP(op, l, r) { if(IS_REG(r)) op(FP_REG_W(l), FP_REG_R(r)); \
					 else if(IS_CMEM(r)) op(FP_REG_W(l), FP_MEM(r)); \
					 else if(IS_MEM(r)) op(FP_REG_W(l), FP_MEM(r)); \
					               else op(FP_REG_W(l), FP_REG_R(r)); }

#define INT_OP(op, l, r) { if(IS_REG(r))    op(INT_REG_W(l), INT_REG_R(r)); \
					  else if(IS_CMEM(r))   op(INT_REG_W(l), INT_MEM(r)); \
					  else if(IS_IMM(r))    op(INT_REG_W(l), static_cast<int>(INT_IMM(r))); \
					  else if(IS_MEM(r))    op(INT_REG_W(l), INT_MEM(r)); \
					  else op(INT_REG_W(l), INT_REG_R(r)); }

struct AsmCodeGenerator
{
	using RegPtr = AssemblyRegister::Ptr;
	using Compiler = X86Compiler;
	using OpType = const char*;
	using AddressType = uint64_t;


	AddressType void2ptr(void* d)
	{
		return reinterpret_cast<AddressType>(d);
	}

	AddressType imm2ptr(int64 t)
	{
		return static_cast<AddressType>(t);
	}


	struct LoopEmitterBase
	{
		LoopEmitterBase(BaseCompiler* c, const Symbol& iterator_, RegPtr loopTarget_, Operations::StatementBlock* loopBody_, bool loadIterator_) :
			iterator(iterator_),
			loopTarget(loopTarget_),
			loadIterator(loadIterator_),
			loopBody(loopBody_),
			type(c->getRegisterType(iterator.typeInfo))
		{};

		virtual ~LoopEmitterBase() {};

		virtual void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) = 0;

		asmjit::Label getLoopPoint(bool getContinue) const
		{
			return getContinue ? continuePoint : loopEnd;
		}

	protected:

		asmjit::Label continuePoint;
		asmjit::Label loopEnd;

		Operations::StatementBlock* loopBody;
		Symbol iterator;
		RegPtr loopTarget;
		Types::ID type;
		bool loadIterator;
	};

	struct ScopedTypeSetter
	{
		ScopedTypeSetter(AsmCodeGenerator& p, Types::ID newType):
			parent(p)
		{
			oldType = parent.type;
			parent.type = newType;
		}

		~ScopedTypeSetter()
		{
			parent.type = oldType;
		}

		Types::ID oldType;
		AsmCodeGenerator& parent;
	};

	

	struct TemporaryRegister
	{
		TemporaryRegister(AsmCodeGenerator& acg, BaseScope* scope, TypeInfo type)
		{
			if (acg.registerPool != nullptr)
			{
				tempReg = acg.registerPool->getNextFreeRegister(scope, type);
				tempReg->createRegister(acg.cc);
			}
			else
			{
				switch (type.getType())
				{
				case Types::ID::Float:	 uncountedReg = acg.cc.newXmmSs(); break;
				case Types::ID::Double:	 uncountedReg = acg.cc.newXmmSd(); break;
				case Types::ID::Integer: uncountedReg = acg.cc.newGpd(); break;
				case Types::ID::Block:	 
				case Types::ID::Pointer: uncountedReg = acg.cc.newIntPtr(); break;
				default:                 jassertfalse; break;
				}
			}
		}

		~TemporaryRegister()
		{
#if REMOVE_REUSABLE_REG
			if (tempReg != nullptr)
				tempReg->flagForReuse(true);
#endif
		}

		X86Gp get()
		{
			if (tempReg != nullptr)
				return tempReg->getRegisterForWriteOp().as<X86Gp>();
			else
				return uncountedReg.as<X86Gp>();
		}

		X86Reg uncountedReg;
		RegPtr tempReg;
	};

	AsmCodeGenerator(Compiler& cc_, AssemblyRegisterPool* pool, Types::ID type_, ParserHelpers::CodeLocation l, const StringArray& optimizations);;

	void emitComment(const char* m);

	RegPtr emitLoadIfNativePointer(RegPtr source, Types::ID nativeType);

	void emitStore(RegPtr target, RegPtr value);

	void emitMemoryWrite(RegPtr source, void* ptrToUse=nullptr);

	void writeRegisterToMemory(RegPtr p);

	void emitMemoryLoad(RegPtr reg);

	void emitComplexTypeCopy(RegPtr target, RegPtr source, ComplexType::Ptr type);

	void emitThisMemberAccess(RegPtr target, RegPtr parent, VariableStorage memberOffset);

	void emitMemberAcess(RegPtr target, RegPtr parent, RegPtr child);

	void emitImmediate(RegPtr target, VariableStorage value);
	
	void emitLogicOp(Operations::BinaryOp* op);

	void emitSpanReference(RegPtr target, RegPtr address, RegPtr index, size_t elementSizeInBytes, int additionalOffsetInBytes=0);

	void emitParameter(const FunctionData& f, FuncNode* fn, RegPtr parameterRegister, int parameterIndex, bool hasObjectPointer=false)
	{
		parameterRegister->createRegister(cc);

		auto useParameterAsAdress = parameterIndex != -1 && ((f.args[parameterIndex].isReference() && parameterRegister->getType() != Types::ID::Pointer));

		if (f.object != nullptr || hasObjectPointer)
			parameterIndex += 1;

		if (fn == nullptr)
			fn = cc.func();

		if (useParameterAsAdress)
		{
			auto aReg = cc.newGpq();

			fn->setArg(parameterIndex, aReg);

			parameterRegister->setCustomMemoryLocation(x86::ptr(aReg), true);
		}
		else

			fn->setArg(parameterIndex, parameterRegister->getRegisterForReadOp());
	}

	void emitParameter(Operations::Function* f, FuncNode* fn, RegPtr parameterRegister, int parameterIndex);

	RegPtr emitBinaryOp(OpType op, RegPtr l, RegPtr r);

	void emitCompare(bool useAsmFlags, OpType op, RegPtr target, RegPtr l, RegPtr r);

	void emitReturn(BaseCompiler* c, RegPtr target, RegPtr expr);

	void writeDirtyGlobals(BaseCompiler* c);

	Result emitStackInitialisation(RegPtr target, ComplexType::Ptr typePtr, RegPtr expr, InitialiserList::Ptr list);

	RegPtr emitBranch(TypeInfo returnType, Operations::Expression* cond, Operations::Statement* trueBranch, Operations::Statement* falseBranch, BaseCompiler* c, BaseScope* s);

	RegPtr emitTernaryOp(Operations::TernaryOp* op, BaseCompiler* c, BaseScope* s);

	RegPtr emitLogicalNot(RegPtr expr);

	void emitIncrement(RegPtr target, RegPtr expr, bool isPre, bool isDecrement);

	void emitCast(RegPtr target, RegPtr expr, Types::ID sourceType);

	void emitNegation(RegPtr target, RegPtr expr);

	Result emitFunctionCall(RegPtr returnReg, const FunctionData& f, RegPtr objectAddress, ReferenceCountedArray<AssemblyRegister>& parameterRegisters);

	void emitFunctionParameterReference(RegPtr sourceReg, RegPtr parameterReg);

	Result emitSimpleToComplexTypeCopy(RegPtr target, InitialiserList::Ptr initValues, RegPtr source);

	void dumpVariables(BaseScope* s, uint64_t lineNumber);

	void emitLoopControlFlow(Operations::ConditionalBranch* parentLoop, bool isBreak);

	Compiler& cc;

	static void fillSignature(const FunctionData& data, FuncSignatureX& sig, Types::ID objectType);

	X86Mem createStack(Types::ID type);
	X86Mem createFpuMem(RegPtr ptr);
	void writeMemToReg(RegPtr target, X86Mem);

    
    void writeRegisterToMemory(RegPtr target, RegPtr source);
    
	AssemblyRegisterPool* registerPool;

    static X86Mem createValid64BitPointer(X86Compiler& cc, X86Mem source, int offset, int byteSize)
    {
        if (source.hasBaseReg())
        {
            return source.cloneAdjustedAndResized(offset, byteSize);
        }
        else
        {
            auto memReg = cc.newGpq();
            
            int64_t address = source.offset() + (int64_t)offset;
            cc.mov(memReg, address);
            
            return x86::ptr(memReg).cloneResized(byteSize);
        }
    }
    
	bool canVectorize() const { return optimizations.contains(OptimizationIds::AutoVectorisation); };

	static bool isRuntimeErrorCheckEnabled(BaseScope* scope)
	{
		return scope->getGlobalScope()->isRuntimeErrorCheckEnabled();
	}

	ParserHelpers::CodeLocation location;
	
	void setType(Types::ID newType)
	{
		type = newType;
	}


private:

	StringArray optimizations;

	static void createRegistersForArguments(X86Compiler& cc, ReferenceCountedArray<AssemblyRegister>& parameters, const FunctionData& f);
	Types::ID type;
};

struct SpanLoopEmitter : public AsmCodeGenerator::LoopEmitterBase
{
	SpanLoopEmitter(BaseCompiler* c, const Symbol& s, AssemblyRegister::Ptr t, Operations::StatementBlock* body, bool l) :
		LoopEmitterBase(c, s, t, body, l)
	{};

	void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) override;

	SpanType* typePtr = nullptr;
};

struct CustomLoopEmitter : public AsmCodeGenerator::LoopEmitterBase
{
	CustomLoopEmitter(BaseCompiler* c, const Symbol& s, AssemblyRegister::Ptr t, Operations::StatementBlock* body, bool l) :
		LoopEmitterBase(c, s, t, body, l)
	{};

	void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) override;

	FunctionData beginFunction;
	FunctionData sizeFunction;
};

struct DynLoopEmitter : public AsmCodeGenerator::LoopEmitterBase
{
	DynLoopEmitter(BaseCompiler* c, const Symbol& s, AssemblyRegister::Ptr t, Operations::StatementBlock* body, bool l) :
		LoopEmitterBase(c, s, t, body, l)
	{};

	void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) override;

	DynType* typePtr = nullptr;
};

#else

// dummy...
struct AsmCodeGenerator
{
	struct LoopEmitterBase
	{
		AsmJitLabel getLoopPoint(bool getContinue) { return {}; }
	};

	AsmJitX86Compiler& cc;
};

#endif

}
}
