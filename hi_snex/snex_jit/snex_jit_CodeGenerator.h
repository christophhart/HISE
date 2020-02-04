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

	struct TemporaryRegister
	{
		TemporaryRegister(AsmCodeGenerator& acg, Types::ID type)
		{
			if (acg.registerPool != nullptr)
			{
				tempReg = acg.registerPool->getNextFreeRegister(type);
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

	void emitImmediate(RegPtr target, VariableStorage value);
	
	void emitLogicOp(Operations::BinaryOp* op);

	void emitParameter(RegPtr parameterRegister, int parameterIndex);

	RegPtr emitBinaryOp(OpType op, RegPtr l, RegPtr r);

	void emitCompare(OpType op, RegPtr target, RegPtr l, RegPtr r);

	void emitReturn(BaseCompiler* c, RegPtr target, RegPtr expr);

	RegPtr emitBranch(Types::ID returnType, Operations::Expression* cond, Operations::Statement* trueBranch, Operations::Statement* falseBranch, BaseCompiler* c, BaseScope* s);

	RegPtr emitTernaryOp(Operations::TernaryOp* op, BaseCompiler* c, BaseScope* s);

	RegPtr emitLogicalNot(RegPtr expr);

	void emitIncrement(RegPtr target, RegPtr expr, bool isPre, bool isDecrement);

	void emitCast(RegPtr target, RegPtr expr, Types::ID sourceType);

	void emitNegation(RegPtr target, RegPtr expr);

	void emitFunctionCall(RegPtr returnReg, const FunctionData& f, ReferenceCountedArray<AssemblyRegister>& parameterRegisters);

	void dumpVariables(BaseScope* s, uint64_t lineNumber);

	Compiler& cc;

	static void fillSignature(const FunctionData& data, FuncSignatureX& sig, bool createObjectPointer);

private:

	AssemblyRegisterPool* registerPool;

	static void createRegistersForArguments(X86Compiler& cc, ReferenceCountedArray<AssemblyRegister>& parameters, const FunctionData& f);
	Types::ID type;
};

}
}
