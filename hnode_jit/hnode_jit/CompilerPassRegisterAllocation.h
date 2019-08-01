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

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

#if 0
struct RegisterAllocator
{
	using ExprPtr = Operations::Expression::Ptr;
	using RegPtr = AssemblyRegister::Ptr;
	using Compiler = asmjit::X86Compiler;

	RegisterAllocator(Compiler& cc_);

	static bool isLeftOfAssignment(ExprPtr vRef, bool dontCareAboutSelfAssign);

	/** Creates the assembly register for the given expression. */
	void createAssemblyRegister(ExprPtr expr);

	/** Creates a AssemblyRegister wrapper for the given type. */

	RegPtr createRegisterPtr(ExprPtr ptr)
	{
		jassert(ptr->currentPass == BaseCompiler::RegisterAllocation);

		if (auto existing = ptr->getRegister())
			return existing;

		return new AssemblyRegister(ptr->getType());
	}

	void allocateBinaryOp(ExprPtr binaryOp);

	void allocateReturnRegister(ExprPtr returnNode);

	asmjit::X86Compiler& cc;
};
#endif

class DeadcodeEliminator : public OptimizationPass
{
	String getName() const override { return "Dead code elimination"; }

	Result process(SyntaxTree* tree) override;
};

class Inliner: public OptimizationPass
{
public:

	using TokenType = ParserHelpers::TokenType;

	struct Helpers
	{
		static int getPrecedenceLevel(TokenType t)
		{
			if (t == JitTokens::divide)	return 4;
			if (t == JitTokens::times )	return 3;
			if (t == JitTokens::minus)	return 2;
			if (t == JitTokens::plus)	return 1;
			else						return 0;
		}

		static int isSwappable(TokenType t)
		{
			if (t == JitTokens::divide)	return false;
			if (t == JitTokens::times)	return true;
			if (t == JitTokens::minus)	return false;
			if (t == JitTokens::plus)	return true;
			else						return false;
		}
	};

	String getName() const override
	{
		return "Inlining";
	}

	Result process(SyntaxTree* tree) override;

	using ExprPtr = Operations::Expression::Ptr;

	static void swapBinaryOpIfPossible(ExprPtr binOp);

	static void createSelfAssignmentFromBinaryOp(ExprPtr assignment);

	static bool containsVariableReference(ExprPtr p, BaseScope::RefPtr refToCheck);

};

}
}