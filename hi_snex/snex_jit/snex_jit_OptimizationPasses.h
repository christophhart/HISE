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

	void processPreviousPasses(BaseCompiler* c, BaseScope* s, StatementPtr st);

	template <class StatementType, class... StatementTypes> static bool is(StatementPtr obj)
	{
        return Operations::as<StatementType>(obj) != nullptr && is<StatementTypes...>(obj);
	}

	template <class StatementType> static bool is(StatementPtr obj)
	{
		return Operations::as<StatementType>(obj) != nullptr;
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

	Symbol currentlyAssignedId;

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

class LoopOptimiser : public OptimizationPass
{
	using Ptr = Operations::Statement::Ptr;

public:

	OPTIMIZATION_FACTORY(OptimizationIds::LoopOptimisation, LoopOptimiser);

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, Ptr statement) override;

	static bool replaceWithVectorLoop(BaseCompiler* compiler, BaseScope* s, Ptr binaryOpToReplace);

private:

	struct Helpers
	{
		static int getCompileTimeAmount(Ptr target)
		{
			auto t = target->getTypeInfo();

			if (auto sp = t.getTypedIfComplexType<SpanType>())
				return sp->getNumElements();

			if (auto st = t.getTypedIfComplexType<StructType>())
			{
				if (st->id == NamespacedIdentifier("FrameProcessor"))
					return st->getTemplateInstanceParameters()[0].constant;
			}

			return 0;
		}
	};

	Operations::Loop* currentLoop = nullptr;
	BaseScope* currentScope = nullptr;
	BaseCompiler* currentCompiler = nullptr;

	bool unroll(BaseCompiler* c, BaseScope* s, Operations::Loop* l);

	bool combineLoops(BaseCompiler* c, BaseScope* s, Operations::Loop* l);

	bool sameTarget(Operations::Loop* l1, Operations::Loop* l2);

	bool isBlockWithSingleStatement(Ptr s);

	bool combineInternal(Operations::Loop* l, Operations::Loop* nl);

	Operations::Loop* getLoopStatement(Ptr s);

	StatementPtr getRealParent(Ptr s);

	static Symbol getRealSymbol(Ptr s);
	
};

class LoopVectoriser : public OptimizationPass
{
	using Ptr = Operations::Statement::Ptr;

public:

	OPTIMIZATION_FACTORY(OptimizationIds::AutoVectorisation, LoopVectoriser);

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override;

private:

	bool convertToSimd(BaseCompiler* c, Operations::Loop* l);

	Result changeIteratorTargetToSimd(Operations::Loop* l);

	static bool isUnSimdableOperation(Ptr s);
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


#if SNEX_ASMJIT_BACKEND

/** This pass tries to optimize on the lowest level just before the actual machine code is being generated.

	There are a few redundant instructions that might have been created during code generation, eg. mov calls
	to the same target or math operations with immediate values that don't have an effect. 
	
	Since this pass has no information about high-level concepts, it's scope is limited to clean-up tasks rather
	than sophisticated optimization algorithms.

	Note: this pass is not being executed like the other passes, however it's subclassed from the snex::OptimisationPass
	base class for consistency.
*/
struct AsmCleanupPass : public OptimizationPass,
						public asmjit::FuncPass
{
	OPTIMIZATION_FACTORY(OptimizationIds::AsmOptimisation, AsmCleanupPass);

	AsmCleanupPass();

	virtual ~AsmCleanupPass() {}

	/** A default filter for the iterator that does nothing. */
	struct Base
	{
		using ReturnType = BaseNode;
		static bool matches(BaseNode* node) noexcept { return true; }
	};

	/** A node iterator that will iterate over the linked list. 
	
		You can give it a filter class that needs:

		- a type alias called ReturnType for the desired node subclass
		- a function `static bool matches(BaseNode*)` that performs a
		  check on each node whether it should be returned by the iterator.

		Take a look at the Base filter above for an example.
	*/
	template <class FilterType = Base> struct Iterator
	{
		using RetType = typename FilterType::ReturnType;

		Iterator(FuncPass* parent, BaseNode* start) :
			currentNode(start),
			cc(parent->cc())
		{}

		/** Returns the next matching node. */
		RetType* next()
		{
			if (currentNode == nullptr)
				return nullptr;

			auto retNode = currentNode;

			currentNode = currentNode->next();

			using ReturnType = typename FilterType::ReturnType;

			if (FilterType::matches(retNode))
				return retNode->template as<ReturnType>();
			else
				return next();
		}

		/** Checks whether the direct successor to this node matches the filter. */
		RetType* peekNextMatch()
		{
			if (FilterType::matches(currentNode))
				return currentNode->as<typename FilterType::ReturnType>();

			return nullptr;
		}

		/** Removes the given node. */
		void removeNode(BaseNode* n)
		{
			if (n->hasInlineComment())
				n->next()->setInlineComment(n->inlineComment());

			cc->removeNode(n);
		}

		/** Seeks to the next match and returns it or nullptr if there is no match left. */
		RetType* seekToNextMatch()
		{
			BaseNode* n = currentNode;

			while (n != nullptr)
			{
				if (FilterType::matches(n))
					return currentNode->as<FilterType::ReturnType>();

				n = n->next();
			}

			return nullptr;
		}

	private:

		asmjit::BaseCompiler* cc;
		BaseNode* currentNode;
	};

	bool processStatementInternal(BaseCompiler* compiler, BaseScope* s, StatementPtr statement) override
	{
		// This method is never being called because this pass will not be executed
		// from the SNEX compiler, but directly by the asmjit::Compiler
		jassertfalse;

		return false;
	}

private:

	struct SubPassBase
	{
		SubPassBase(FuncPass* p)
		{};

        virtual ~SubPassBase() {};
        
		/** Override this method and return true if the optimisation was successful.
		
			In this case it will repeat the optimisation until it returns false. */
		virtual bool run(BaseNode* n) = 0;
	};

public:

	/** Subclass any subpass from this, then override the run() method to perform the optimisation. */
	template <typename FilterType> struct SubPass : public SubPassBase
	{
		SubPass(FuncPass* p):
			SubPassBase(p),
			parent(p)
		{}

		/** Creates a iterator with the given filter type. */
		Iterator<FilterType> createIterator(BaseNode* n)
		{
			return Iterator<FilterType>(parent, n);
		}

		FuncPass* parent;
	};

	template <class T> void addSubPass()
	{
		auto newPass = new T(this);
		passes.add(newPass);
	}

	Error runOnFunction(asmjit::Zone* zone, asmjit::Logger* logger, FuncNode* func) noexcept
	{
		for (int i = 0; i < passes.size(); i++)
		{
			auto p = passes[i];

			if (p->run(func))
			{
				numOptimisations++;
				i = 0;
			}
		}
		

		return 0;
	}

private:

	int numOptimisations = 0;

	OwnedArray<SubPassBase> passes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsmCleanupPass);
};

#endif

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
		registerOptimization<LoopOptimiser>();
		registerOptimization<LoopVectoriser>();
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
			if (id.toString() == e.id.toString())
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
