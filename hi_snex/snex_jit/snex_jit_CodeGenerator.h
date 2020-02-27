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



struct AsmCodeGenerator
{
	using RegPtr = AssemblyRegister::Ptr;
	using Compiler = asmjit::X86Compiler;
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
		LoopEmitterBase(const Symbol& iterator_, RegPtr loopTarget_, Operations::StatementBlock* loopBody_, bool loadIterator_) :
			iterator(iterator_),
			loopTarget(loopTarget_),
			loadIterator(loadIterator_),
			loopBody(loopBody_),
			type(iterator.typeInfo.getType())
		{};

		virtual ~LoopEmitterBase() {};

		virtual void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) = 0;

	protected:

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
		TemporaryRegister(AsmCodeGenerator& acg, BaseScope* scope, Types::ID type)
		{
			if (acg.registerPool != nullptr)
			{
				tempReg = acg.registerPool->getNextFreeRegister(scope, type);
				tempReg->createRegister(acg.cc);
			}
			else
			{
				switch (type)
				{
				case Types::ID::Float:	 uncountedReg = acg.cc.newXmmSs(); break;
				case Types::ID::Double:	 uncountedReg = acg.cc.newXmmSd(); break;
				case Types::ID::Integer: uncountedReg = acg.cc.newGpd(); break;
				case Types::ID::Event:
				case Types::ID::Block:	 uncountedReg = acg.cc.newIntPtr(); break;
                default:                 break;
				}
			}
		}

		~TemporaryRegister()
		{
			if (tempReg != nullptr)
				tempReg->flagForReuse(true);
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

	AsmCodeGenerator(Compiler& cc_, AssemblyRegisterPool* pool, Types::ID type_);;

	void emitComment(const char* m);

	void emitStore(RegPtr target, RegPtr value);

	void emitMemoryWrite(RegPtr source, void* ptrToUse=nullptr);

	void emitMemoryLoad(RegPtr reg);

	void emitThisMemberAccess(RegPtr target, RegPtr parent, VariableStorage memberOffset);

	void emitMemberAcess(RegPtr target, RegPtr parent, RegPtr child);

	void emitImmediate(RegPtr target, VariableStorage value);
	
	void emitLogicOp(Operations::BinaryOp* op);

	void emitSpanReference(RegPtr target, RegPtr address, RegPtr index, size_t elementSizeInBytes);

	void emitSpanIteration(BaseCompiler* c, BaseScope* s, const Symbol& iterator, SpanType* typePtr, RegPtr spanTarget, Operations::Statement* loopBody, bool loadIterator);

	void emitParameter(Operations::Function* f, RegPtr parameterRegister, int parameterIndex);

	RegPtr emitBinaryOp(OpType op, RegPtr l, RegPtr r);

	void emitCompare(OpType op, RegPtr target, RegPtr l, RegPtr r);

	void emitReturn(BaseCompiler* c, RegPtr target, RegPtr expr);

	void emitInitialiserList(RegPtr target, ComplexType::Ptr typePtr, InitialiserList::Ptr list);

	RegPtr emitBranch(Types::ID returnType, Operations::Expression* cond, Operations::Statement* trueBranch, Operations::Statement* falseBranch, BaseCompiler* c, BaseScope* s);

	RegPtr emitTernaryOp(Operations::TernaryOp* op, BaseCompiler* c, BaseScope* s);

	RegPtr emitLogicalNot(RegPtr expr);

	void emitIncrement(RegPtr target, RegPtr expr, bool isPre, bool isDecrement);

	void emitCast(RegPtr target, RegPtr expr, Types::ID sourceType);

	void emitNegation(RegPtr target, RegPtr expr);

	void emitFunctionCall(RegPtr returnReg, const FunctionData& f, RegPtr objectAddress, ReferenceCountedArray<AssemblyRegister>& parameterRegisters);

	void emitFunctionParameterReference(RegPtr sourceReg, RegPtr parameterReg);

	void writeToPointerAddress(RegPtr target, RegPtr value);

	void emitInlinedMathAssembly(Identifier id, RegPtr target, const ReferenceCountedArray<AssemblyRegister>& args);
	
	void emitWrap(WrapType* t, RegPtr target, WrapType::OpType op);

	static Array<Identifier> getInlineableMathFunctions();

	void dumpVariables(BaseScope* s, uint64_t lineNumber);

	

	Compiler& cc;

	static void fillSignature(const FunctionData& data, FuncSignatureX& sig, bool createObjectPointer);

private:

	X86Mem createStack(Types::ID type);
	X86Mem createFpuMem(RegPtr ptr);
	void writeMemToReg(RegPtr target, X86Mem);

	AssemblyRegisterPool* registerPool;

	static void createRegistersForArguments(X86Compiler& cc, ReferenceCountedArray<AssemblyRegister>& parameters, const FunctionData& f);
	Types::ID type;
};

struct SpanLoopEmitter : public AsmCodeGenerator::LoopEmitterBase
{
	SpanLoopEmitter(const Symbol& s, AssemblyRegister::Ptr t, Operations::StatementBlock* body, bool l) :
		LoopEmitterBase(s, t, body, l)
	{};

	void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) override;

	SpanType* typePtr = nullptr;
};

struct BlockLoopEmitter : public AsmCodeGenerator::LoopEmitterBase
{
	BlockLoopEmitter(const Symbol& s, AssemblyRegister::Ptr t, Operations::StatementBlock* body, bool l) :
		LoopEmitterBase(s, t, body, l)
	{};

	void emitLoop(AsmCodeGenerator& gen, BaseCompiler* compiler, BaseScope* scope) override;

};

}
}
