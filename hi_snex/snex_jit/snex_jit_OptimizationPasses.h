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

#define OPTIMIZATION_FACTORY(id, x) static Identifier getStaticId() { return id; } \
static BaseCompiler::OptimizationPassBase* create() { return new x(); } \
String getName() const override { return getStaticId().toString(); };

class ConstExprEvaluator : public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::ConstantFolding, ConstExprEvaluator);

	using TokenType = const char*;

	static VariableStorage binaryOp(TokenType t, VariableStorage left, VariableStorage right);

	using OpType = const char*;

	void reset() override
	{
		constantVariables.clear();
	}

	void process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement);

	void addConstKeywordToSingleWriteVariables(Operations::VariableReference* v, BaseScope* s, BaseCompiler* compiler);

	void replaceWithImmediate(ExprPtr e, const VariableStorage& value);

	/** Checks if the two expressions are constant.

		Returns nullptr if it can't be optimized, otherwise an
		Immediate expression with the result.
	*/
	static ExprPtr evalBinaryOp(ExprPtr left, ExprPtr right, OpType op);

	static ExprPtr evalNegation(ExprPtr expr);

	static ExprPtr evalCast(ExprPtr expression, Types::ID targetType);

	static ExprPtr createInvertImmediate(ExprPtr immediate, OpType op);

private:

	ReferenceCountedArray<BaseScope::Reference> constantVariables;
};

class BinaryOpOptimizer : public OptimizationPass
{
	using TokenType = ParserHelpers::TokenType;

	struct Helpers
	{
		static int getPrecedenceLevel(TokenType t)
		{
			if (t == JitTokens::divide)	return 4;
			if (t == JitTokens::times)	return 3;
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

public:

	OPTIMIZATION_FACTORY(OptimizationIds::BinaryOpOptimisation, BinaryOpOptimizer);

	void process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;

private:

	bool isAssignedVariable(ExprPtr e) const;

	static Operations::BinaryOp* getFirstOp(ExprPtr e);

	BaseScope::Symbol currentlyAssignedId;

	static bool containsVariableReference(ExprPtr p, BaseScope::RefPtr refToCheck);

	void swapBinaryOpIfPossible(ExprPtr binaryOp);

	void createSelfAssignmentFromBinaryOp(ExprPtr assignment);
};

class DeadcodeEliminator : public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::DeadCodeElimination, DeadcodeEliminator);

	void process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;
};

class Inliner: public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::Inlining, Inliner);

	using TokenType = ParserHelpers::TokenType;

	

	void process(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;

	using ExprPtr = Operations::Expression::Ptr;

	

};

struct OptimizationFactory
{
	using CreateFunction = std::function<BaseCompiler::OptimizationPassBase*(void)>;
	using IdFunction = std::function<juce::Identifier(void)>;

	OptimizationFactory()
	{
		registerOptimization<Inliner>();
		registerOptimization<DeadcodeEliminator>();
		registerOptimization<BinaryOpOptimizer>();
		registerOptimization<ConstExprEvaluator>();
	}

	struct Entry
	{
		Identifier id;
		CreateFunction f;
	};

	template <class T> void registerOptimization()
	{
		Entry e;
		e.id = T::getStaticId();
		e.f = T::create;

		entries.add(std::move(e));
	}

	BaseCompiler::OptimizationPassBase* createOptimization(const Identifier& id)
	{
		for (auto& e : entries)
		{
			if (id == e.id)
			{
				return e.f();
			}
		}

		return nullptr;
	}

	Array<Entry> entries;
};

}
}