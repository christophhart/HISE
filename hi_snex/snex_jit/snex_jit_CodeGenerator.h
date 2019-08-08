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

class ConstExprEvaluator: public OptimizationPass
{
public:

	using TokenType = const char*;

	static VariableStorage binaryOp(TokenType t, VariableStorage left, VariableStorage right);
		
	using ExprPtr = Operations::Expression::Ptr;
	using OpType = const char*;

	String getName() const { return "Constant folding"; };

	Result process(SyntaxTree* tree) override;

	void replaceWithImmediate(ExprPtr e, const VariableStorage& value);

	/** Checks if the two expressions are constant.

		Returns nullptr if it can't be optimized, otherwise an
		Immediate expression with the result.
	*/
	static ExprPtr evalBinaryOp(ExprPtr left, ExprPtr right, OpType op);

	static ExprPtr evalNegation(ExprPtr expr);

	static ExprPtr evalCast(ExprPtr expression, Types::ID targetType);

	static ExprPtr createInvertImmediate(ExprPtr immediate, OpType op);
};

struct AsmCodeGenerator
{
	using RegPtr = AssemblyRegister::Ptr;
	using Compiler = asmjit::X86Compiler;
	using OpType = const char*;

	AsmCodeGenerator(Compiler& cc_, Types::ID type_);;

	void emitComment(const char* m);

	void emitStore(RegPtr target, RegPtr value);

	void emitMemoryWrite(RegPtr source);

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

	Compiler& cc;

	static void fillSignature(const FunctionData& data, FuncSignatureX& sig, bool createObjectPointer);

private:

	static void createRegistersForArguments(X86Compiler& cc, ReferenceCountedArray<AssemblyRegister>& parameters, const FunctionData& f);
	Types::ID type;
};

}
}