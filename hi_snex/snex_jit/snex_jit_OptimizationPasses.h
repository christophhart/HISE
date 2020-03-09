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

/** An OptimizationPass is a class that performs optimization on a given
    Statement.
 
    In order to use it, subclass from this class, register it to the
    OptimizationFactory and then call GlobalScope::addOptimization with the
    given ID.
 
    There are three Optimization contexts:
 
    - directly after parsing, before the symbols are resolved
    - after the type check
    - before the code generation
 
 
*/
class OptimizationPass : public BaseCompiler::OptimizationPassBase
{
public:
        
    using StatementPtr = Operations::Statement::Ptr;
    using ExprPtr = Operations::Expression::Ptr;
    
    /** Overwrite this method and perform the operation.
        The signature is similar to Statement::process(), so you can perform
        similar operations.
     
        Be aware that by default this will be called in the same order as the
        Statement::process() method, which is children first, then the parent node.
     
        Example:
     
        float x = 1.0f * y + 8.0f;
     
        The order in which the nodes will be evaluated is:
     
        - multiplication
        - addition
        - assignment
     
        However, you can force another order by explicitely calling process()
        on a child node (in this case, use Statement::process() or it might not
        have the correct state.
     
        It will also be called only during optimization passes, so you can't
        perform anything during regular passes (eg. ResolvingSymbols).

		If some optimizations were performed, return true so that this pass will be repeated.
		This might catch subsequent optimizations.
    */
    virtual bool processStatementInternal(BaseCompiler* c, BaseScope* s, StatementPtr statement) = 0;
    
    /** Call this if you want to replace a statement with a noop node.
        If the statement is not referenced anywhere else, it might be
        deleted, so be aware of that when you need to use the object afterwards.
    */
    void replaceWithNoop(StatementPtr s);
    
    /** Call this if you want to replace the given statement with a new expression
        as a result of a optimization pass. */

	void replaceExpression(StatementPtr old, StatementPtr newExpression)
	{
		old->replaceInParent(newExpression);
	}

    /** Short helper tool to check Types. */
    template <class T> T* as(StatementPtr obj)
    {
        return dynamic_cast<T*>(obj.get());
    }
};

    
    
#define OPTIMIZATION_FACTORY(id, x) static Identifier getStaticId() { return id; } \
static BaseCompiler::OptimizationPassBase* create() { return new x(); } \
juce::String getName() const override { return getStaticId().toString(); };

    /** TODO: Optimizations:
     
     1 Expression simplifications:
       - x / constant => x * (1 / constant)
       - x * 2 => x + x
       - 125 - x => -x + 125 ( => x *= -1; x += 125 )
       - x % pow2 => x & log2(pow2)-1;
     
     */
    
class ConstExprEvaluator : public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::ConstantFolding, ConstExprEvaluator);

	using TokenType = const char*;


	using OpType = const char*;

	void reset() override
	{
		constantVariables.clear();
	}

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement);

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

	static ExprPtr evalConstMathFunction(Operations::FunctionCall* functionCall);

	static ExprPtr evalDotOperator(BaseScope* s, Operations::DotOperator* dot);

private:

	Array<Symbol> constantVariables;
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

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;

private:

	bool swapIfBetter(ExprPtr bOp, const char* op, BaseCompiler* compiler, BaseScope* s);

	bool simplifyOp(ExprPtr l, ExprPtr r, const char* op, BaseCompiler* compiler, BaseScope* s);

	bool isAssignedVariable(ExprPtr e) const;

	static Operations::BinaryOp* getFirstOp(ExprPtr e);

	SymbolWithScope currentlyAssignedId;

	static bool containsVariableReference(ExprPtr p, const Symbol& refToCheck);

	void swapBinaryOpIfPossible(ExprPtr binaryOp);

	void createSelfAssignmentFromBinaryOp(ExprPtr assignment);
};

class DeadcodeEliminator : public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::DeadCodeElimination, DeadcodeEliminator);

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;
};

class FunctionInliner: public OptimizationPass
{
public:

	OPTIMIZATION_FACTORY(OptimizationIds::Inlining, FunctionInliner);

	using TokenType = ParserHelpers::TokenType;

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;

	bool inlineRootFunction(BaseCompiler* compiler, BaseScope* scope, Operations::Function* f, Operations::FunctionCall* fc);

	using ExprPtr = Operations::Expression::Ptr;

};

struct OptimizationFactory
{
	using CreateFunction = std::function<BaseCompiler::OptimizationPassBase*(void)>;
	using IdFunction = std::function<juce::Identifier(void)>;

	OptimizationFactory()
	{
		registerOptimization<FunctionInliner>();
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
