/*
  ==============================================================================

    NewFunkyParser.h
    Created: 31 Jan 2019 4:53:19pm
    Author:  Christoph

  ==============================================================================
*/

#pragma once

namespace hnode {
namespace jit
{
using namespace juce;


#define PROCESS_IF_NOT_NULL(expr) if (expr != nullptr) expr->process(compiler, scope);
#define COMPILER_PASS(x) if (compiler->getCurrentPass() == x)
#define CREATE_ASM_COMPILER(type) AsmCodeGenerator(getFunctionCompiler(compiler), type);
#define SKIP_IF_CONSTEXPR if(isConstExpr()) return;

class FunctionScope;

/** This class has a variable pool that will not exceed the lifetime of the compilation. */
class Operations
{
public:

	using FunctionCompiler = asmjit::X86Compiler;

	static FunctionCompiler& getFunctionCompiler(BaseCompiler* c);

	static BaseScope* findClassScope(BaseScope* scope);

	static BaseScope* findFunctionScope(BaseScope* scope);

	using RegPtr = AssemblyRegister::Ptr;

	static asmjit::Runtime* getRuntime(BaseCompiler* c);

	using Symbol = BaseScope::Symbol;
	using Location = ParserHelpers::CodeLocation;
	using TokenType = ParserHelpers::TokenType;

	struct Statement : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Statement>;

		Statement(Location l) :
			location(l)
		{};

		void setParent(Statement* parent_)
		{
			parent = parent_;
		}

		template <class T> bool is() const
		{
			return dynamic_cast<const T*>(this) != nullptr;
		}

		virtual ~Statement() {};

		virtual Types::ID getType() const = 0;
		virtual void process(BaseCompiler* compiler, BaseScope* scope)
		{
			currentCompiler = compiler;
			currentScope = scope;
			currentPass = compiler->getCurrentPass();
		}

		virtual bool hasSideEffect() const { return false; }

		void throwError(const String& errorMessage)
		{
			ParserHelpers::CodeLocation::Error e(location.program, location.location);
			e.errorMessage = errorMessage;
			throw e;
		}

		void logOptimisationMessage(const String& m)
		{
			logMessage(currentCompiler, BaseCompiler::VerboseProcessMessage, m);
		}

		void logWarning(const String& m)
		{
			logMessage(currentCompiler, BaseCompiler::Warning, m);
		}

		bool isConstExpr() const;

		void logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const String& message)
		{
			String m;

			m << "Line " << location.getLineNumber(location.program, location.location) << ": ";
			m << message;

			DBG(m);

			compiler->logMessage(type, m);
		}

		Location location;

		BaseCompiler* currentCompiler = nullptr;
		BaseScope* currentScope = nullptr;
		BaseCompiler::Pass currentPass;
		JUCE_DECLARE_WEAK_REFERENCEABLE(Statement);

		WeakReference<Statement> parent;
	};
	
	/** A high level node in the syntax tree that is used by the optimization passes
		to simplify the code.
	*/
	struct Expression : public Statement
	{
		using Ptr = ReferenceCountedObjectPtr<Expression>;

		Expression(Location loc) :
			Statement(loc)
		{};

		virtual ~Expression() {};

		Types::ID getType() const override 
		{ 
#if 0
			if (auto firstExpr = subExpr.getFirst())
				return firstExpr->getType();
#endif

			return type; 
		}

		void attachAsmComment(const String& message)
		{
			asmComment = message;
		}

		void checkAndSetType(int offset = 0, Types::ID expectedType=Types::ID::Dynamic);

		/** Processes all sub expressions. Call this from your base class. */
		void process(BaseCompiler* compiler, BaseScope* scope) override;

		

		bool isAnonymousStatement() const
		{
			return isStatementType<StatementBlock>(parent) ||
				   isStatementType<SyntaxTree>(parent);
		}

		VariableStorage getConstExprValue() const;

		Ptr replaceInParent(Expression::Ptr newExpression)
		{
			if (auto expr = dynamic_cast<Expression*>(parent.get()))
			{
				for (int i = 0; i < expr->subExpr.size(); i++)
				{
					if (expr->getSubExpr(i).get() == this)
					{
						Ptr f(this);
						expr->subExpr.set(i, newExpression.get());
						newExpression->parent = expr;
						return f;
					}
				}
			}

			return nullptr;
		}

		

		int getNumSubExpressions() const
		{
			return subExpr.size();
		}

		void addSubExpression(Ptr expr, int index=-1) 
		{ 
			if (index == -1)
				subExpr.add(expr.get());
			else
				subExpr.insert(index, expr.get());

			expr->setParent(this);
		}

		void swapSubExpressions(int first, int second)
		{
			subExpr.swap(first, second);
		}

		bool hasSubExpr(int index) const 
		{ 
			return isPositiveAndBelow(index, subExpr.size()); 
		}

		Ptr getSubExpr(int index) const 
		{ 
			return subExpr[index];
		}

		/** Returns a pointer to the register of this expression.
		
			This can be called after the RegisterAllocation pass
		*/
		RegPtr getSubRegister(int index) const
		{
			// If you hit this, you have either forget to call the 
			// Statement::process() or you try to access an register 
			// way too early...
			jassert(currentPass >= BaseCompiler::Pass::RegisterAllocation);

			if (auto e = getSubExpr(index))
				return e->reg;

			// Can't find the subexpression you want
			jassertfalse;
		}

		String asmComment;

		RegPtr reg;
        
    protected:
        
        Types::ID type;
        
    private:
        
		ReferenceCountedArray<Expression> subExpr;
	};

	template <class T> static bool isStatementType(const Statement* t)
	{
		return dynamic_cast<const T*>(t) != nullptr;
	}

	template <class T> static T* findParentStatementOfType(Operations::Statement* e)
	{
		if (auto p = dynamic_cast<T*>(e))
			return p;

		if (e->parent != nullptr)
			return findParentStatementOfType<T>(e->parent.get());

		return nullptr;
	}

	template <class T> static const T* findParentStatementOfType(const Operations::Statement* e)
	{
		if (auto p = dynamic_cast<const T*>(e))
			return p;

		if (e->parent != nullptr)
			return findParentStatementOfType<T>(e->parent.get());

		return nullptr;
	}

	static bool isOpAssignment(Expression::Ptr p);


	struct Assignment;		struct Immediate;
	struct FunctionCall;	struct ReturnStatement;			struct StatementBlock;
	struct Function;		struct BinaryOp;				struct VariableReference;
	struct TernaryOp;		struct LogicalNot;				struct Cast;
	struct Negation;		struct Compare;					struct UnaryOp;
	struct Increment;		struct BlockAccess;				struct BlockAssignment;
	struct BlockLoop;

	static Expression* findAssignmentForVariable(Expression::Ptr variable, BaseScope* scope);
};

class SyntaxTree: public Operations::Statement
{
public:

	SyntaxTree(ParserHelpers::CodeLocation l):
		Statement(l)
	{};

	Types::ID getType() const { return Types::ID::Void; }

	~SyntaxTree()
	{
		list.clear();
	}

	void add(Operations::Statement::Ptr newStatement)
	{
		newStatement->setParent(this);
		list.add(newStatement);
	}

	ReferenceCountedArray<Operations::Statement> list;

	void addVariableReference(Operations::Statement* s)
	{
		variableReferences.add(s);
	}


	bool isFirstReference(Operations::Statement* v) const;
	
	Operations::Statement* getLastVariableForReference(BaseScope::RefPtr ref) const;

	Operations::Statement* getLastAssignmentForReference(BaseScope::RefPtr ref) const;

	Operations::Statement** begin() const
	{
		return list.begin();
	}

	Operations::Statement** end() const
	{
		return list.end();
	}

private:

	Array<WeakReference<Statement>> variableReferences;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyntaxTree);
};

class OptimizationPass : public BaseCompiler::OptimizationPassBase
{
public:

	using ExprPtr = Operations::Expression::Ptr;

	template <class T> T* as(Operations::Statement* obj)
	{
		return dynamic_cast<T*>(obj);
	}
};

class BlockParser : public ParserHelpers::TokenIterator
{
public:

	using ExprPtr = Operations::Expression::Ptr;
	using StatementPtr = Operations::Statement::Ptr;

	BlockParser(BaseCompiler* c, const String::CharPointerType& code, const String::CharPointerType& wholeProgram, int length) :
		TokenIterator(code, wholeProgram, length),
		compiler(c)
	{};

	SyntaxTree* parseStatementList();

	virtual StatementPtr parseStatement() = 0;

	virtual void finaliseSyntaxTree(SyntaxTree* tree) 
	{
		ignoreUnused(tree);
	}

	VariableStorage parseVariableStorageLiteral();

	WeakReference<BaseCompiler> compiler;
};



class NewClassParser : public BlockParser
{
public:

	NewClassParser(BaseCompiler* c, const String& code):
		BlockParser(c, code.getCharPointer(), code.getCharPointer(), code.length())
	{}

	StatementPtr parseStatement() override;
	StatementPtr parseVariableDefinition(bool isConst);
	StatementPtr parseFunction();
};



}
}
