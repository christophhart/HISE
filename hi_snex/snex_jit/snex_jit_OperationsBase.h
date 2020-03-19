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

class FunctionScope;

#define SET_EXPRESSION_ID(x) Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER(#x); }

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

	using Location = ParserHelpers::CodeLocation;
	using TokenType = ParserHelpers::TokenType;

	static juce::String toSyntaxTreeString(const ValueTree& v, int intLevel)
	{
		juce::String s;

		auto lineNumber = (int)v["Line"];
		if (lineNumber < 10)
			s << "0";
		s << juce::String(lineNumber) << " ";

		for (int i = 0; i < intLevel; i++)
		{
			s << "-";
		}



		s << v.getType() << ": ";

		for (int i = 0; i < v.getNumProperties(); i++)
		{
			auto id = v.getPropertyName(i);

			if (id == Identifier("Line"))
				continue;

			s << id << "=" << v[id].toString();

			if (i != v.getNumProperties() - 1)
				s << ", ";
		}

		s << "\n";

		return s;
	}

	struct Statement : public ReferenceCountedObject
	{
		enum class TextFormat
		{
			SyntaxTree,
			CppCode,
			numTextFormat
		};

		using Ptr = ReferenceCountedObjectPtr<Statement>;

		Statement(Location l);;

		void setParent(Statement* parent_)
		{
			parent = parent_;
		}

		template <class T> bool is() const
		{
			return dynamic_cast<const T*>(this) != nullptr;
		}

		virtual juce::String toString(TextFormat t) const
		{
			jassertfalse;
			return {};
		}

		virtual ~Statement() {};

		virtual TypeInfo getTypeInfo() const = 0;
		virtual void process(BaseCompiler* compiler, BaseScope* scope);

		virtual bool hasSideEffect() const { return false; }

		virtual size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const { return 0; }

		void cloneChildren(Ptr newClone) const
		{
			for (auto s : *this)
				newClone->addStatement(s->clone(newClone->location));
		}

		virtual Ptr clone(Location l) const = 0;

		Ptr getChildStatement(int index) const
		{
			return childStatements[index];
		}

		int getNumChildStatements() const
		{
			return childStatements.size();
		}

		void swapSubExpressions(int first, int second)
		{
			childStatements.swap(first, second);
		}

		void flushChildStatements()
		{
			jassert(currentPass >= BaseCompiler::CodeGeneration);

			childStatements.clear();
		}

		virtual Identifier getStatementId() const = 0;

		virtual ValueTree toValueTree() const
		{
			ValueTree v(getStatementId());

			v.setProperty("Line", location.getLine(), nullptr);

			for (auto s : *this)
				v.addChild(s->toValueTree(), -1, nullptr);

			return v;
		}

		void throwError(const juce::String& errorMessage);
		void logOptimisationMessage(const juce::String& m);
		void logWarning(const juce::String& m);
		void logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const juce::String& message);

		Statement** begin() const { return childStatements.begin(); }

		Statement** end() const { return childStatements.end(); }

		Statement::Ptr getLastStatement() const
		{
			return childStatements.getLast();
		}

		void processAllChildren(BaseCompiler* compiler, BaseScope* scope)
		{
			for (auto s : *this)
				s->process(compiler, scope);
		}

		bool forEachRecursive(const std::function<bool(Ptr)>& f)
		{
			if (f(this))
				return true;

			for (int i = 0; i < getNumChildStatements(); i++)
			{
				auto c = getChildStatement(i);

				if (c->forEachRecursive(f))
					return true;
			}

			return false;
		}

		virtual bool isConstExpr() const;

		Location location;

		BaseCompiler* currentCompiler = nullptr;
		BaseScope* currentScope = nullptr;
		BaseCompiler::Pass currentPass;

		WeakReference<Statement> parent;

		void addStatement(Statement* b, bool addFirst=false);

		Ptr replaceInParent(Ptr newExpression);
		Ptr replaceChildStatement(int index, Ptr newExpr);

	private:

		ReferenceCountedArray<Statement> childStatements;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Statement);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement);
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

		void attachAsmComment(const juce::String& message);

		TypeInfo checkAndSetType(int offset, TypeInfo expectedType);

		TypeInfo setTypeForChild(int childIndex, TypeInfo expectedType);

		/** Processes all sub expressions. Call this from your base class. */
		void process(BaseCompiler* compiler, BaseScope* scope) override;

		void processChildrenIfNotCodeGen(BaseCompiler* compiler, BaseScope* scope);

		bool isCodeGenPass(BaseCompiler* compiler) const;

		bool preprocessCodeGenForChildStatements(BaseCompiler* compiler, BaseScope* scope, const std::function<bool()>& abortFunction);

		void replaceMemoryWithExistingReference(BaseCompiler* compiler);

		bool isAnonymousStatement() const;

		Types::ID getType() const
		{
			return getTypeInfo().getType();
		}

		virtual VariableStorage getConstExprValue() const;

		bool hasSubExpr(int index) const;

		virtual VariableStorage getPointerValue() const;

		Ptr getSubExpr(int index) const;

		/** Returns a pointer to the register of this expression.

			This can be called after the RegisterAllocation pass
		*/
		RegPtr getSubRegister(int index) const;

		juce::String asmComment;

		RegPtr reg;

		void releaseRegister()
		{
			reg = nullptr;
		}

		void setTargetRegister(RegPtr targetToUse, bool clearRegister=true)
		{
			if (reg != nullptr)
				return;

			if (clearRegister)
			{
				jassert(targetToUse->canBeReused());
				targetToUse->clearForReuse();
			}

			reg = targetToUse;
		}

	private:

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Expression);
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


	struct Assignment;		struct Immediate;				struct Noop;
	struct FunctionCall;	struct ReturnStatement;			struct StatementBlock;
	struct Function;		struct BinaryOp;				struct VariableReference;
	struct TernaryOp;		struct LogicalNot;				struct Cast;
	struct Negation;		struct Compare;					struct UnaryOp;
	struct Increment;		struct DotOperator;				struct Loop;		
	struct IfStatement;		struct ClassStatement;			struct UsingStatement;
	struct CastedSimd;      struct Subscript;				struct InlinedParameter;
	struct ComplexTypeDefinition;						    struct ControlFlowStatement;
	struct InlinedArgument; struct MemoryReference;

	struct ScopeStatementBase
	{
		ScopeStatementBase(const NamespacedIdentifier& id) :
			returnType(Types::ID::Dynamic),
			path(id)
		{};

		virtual ~ScopeStatementBase() {};

		Statement::Ptr createChildBlock(Location l) const;

		void cloneScopeProperties(ScopeStatementBase* newClone) const
		{
			newClone->returnType = returnType;
		}

		void allocateReturnRegister(BaseCompiler* c, BaseScope* s)
		{
			jassert(hasReturnType());
			returnRegister = c->registerPool.getNextFreeRegister(s, getReturnType());
		}

		void setParentScopeStatement(ScopeStatementBase* parent)
		{
			parentScopeStatement = parent;
		}

		void setReturnType(TypeInfo t)
		{
			returnType = t;
		}

		TypeInfo getReturnType() const
		{
			return returnType;
		}

		static ScopeStatementBase* getStatementListWithReturnType(Statement* s)
		{
			if (s == nullptr)
				return nullptr;

			if (auto ss = dynamic_cast<ScopeStatementBase*>(s))
			{
				if (ss->hasReturnType())
					return ss;
			}

			return getStatementListWithReturnType(s->parent);
		}

		RegPtr getReturnRegister()
		{
			jassert(hasReturnType());
			return returnRegister;
		}

		bool hasReturnType() const
		{
			return returnType.getType() != Types::ID::Dynamic;
		}

		NamespacedIdentifier getPath() const { return path; }

	protected:

		RegPtr returnRegister;
		TypeInfo returnType;

	private:

		NamespacedIdentifier path;
		WeakReference<ScopeStatementBase> parentScopeStatement;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScopeStatementBase);
	};

	/** Just a empty base class that checks whether the global variables will be loaded
		before the branch.
	*/
	struct ConditionalBranch
	{
		void allocateDirtyGlobalVariables(Statement::Ptr stament, BaseCompiler* c, BaseScope* s);

		

		virtual ~ConditionalBranch() {}
	};

	struct TypeDefinitionBase
	{
		virtual ~TypeDefinitionBase() {};
		virtual Array<NamespacedIdentifier> getInstanceIds() const = 0;
	};

	struct SymbolStatement
	{
		virtual ~SymbolStatement() {}

		bool isApiClass(BaseScope* s) const
		{
			if (auto fc = getFunctionClassForSymbol(s))
			{
				return dynamic_cast<ComplexType*>(fc) == nullptr;
			}

			return false;
		}

		FunctionClass* getFunctionClassForSymbol(BaseScope* scope) const
		{
			if (auto gfc = scope->getGlobalScope()->getGlobalFunctionClass(getSymbol().id))
				return gfc;

			if (scope->getScopeType() == BaseScope::Global)
				return nullptr;

			return scope->getRootData()->getSubFunctionClass(getSymbol().id);
		}

		virtual Symbol getSymbol() const = 0;
	};

	struct BranchingStatement
	{
		virtual ~BranchingStatement() {};

		Statement::Ptr getCondition()
		{
			return asStatement()->getChildStatement(0);
		}

		Statement::Ptr getTrueBranch()
		{
			return asStatement()->getChildStatement(1);
		}

		Statement::Ptr getFalseBranch()
		{
			if (asStatement()->getNumChildStatements() > 2)
				return asStatement()->getChildStatement(2);

			return nullptr;
		}

		Statement* asStatement()
		{
			return dynamic_cast<Statement*>(this);
		}
	};

	static Expression* findAssignmentForVariable(Expression::Ptr variable, BaseScope* scope);
};

/** A syntax tree is a list of statements without a parent (so that the SyntaxTreeWalker will look deeper than that. 

It's used by either class definitions or function definition (so that each function has its own syntax tree).
*/
class SyntaxTree : public Operations::Statement,
				   public Operations::ScopeStatementBase
{
public:

	SyntaxTree(ParserHelpers::CodeLocation l, const NamespacedIdentifier& ns);
	TypeInfo getTypeInfo() const { return returnType; }

	

	~SyntaxTree()
	{
		int x = 5;
	}

	size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const
	{
		size_t s = 0;

		for (int i = 0; i < getNumChildStatements(); i++)
			s += getChildStatement(i)->getRequiredByteSize(compiler, scope);

		return s;
	}

	juce::String toString(TextFormat t) const override
	{
		switch (t)
		{
		case Operations::Statement::TextFormat::SyntaxTree: return dump();
		case Operations::Statement::TextFormat::CppCode:
		{
			juce::String s;
			juce::String nl;
			s << "{";

			for (auto c : *this)
			{
				s << c->toString(t) << ";\n";
			}

			s << "}";

			return s;
		}
		default:
			return {};
		}
	}

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("SyntaxTree"); }

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		Statement::process(compiler, scope);

		for (int i = 0; i < getNumChildStatements(); i++)
			getChildStatement(i)->process(compiler, scope);

		if(compiler->getCurrentPass() == BaseCompiler::RegisterAllocation && hasReturnType())
		{
			allocateReturnRegister(compiler, scope);
		}
	}

	bool isFirstReference(Operations::Statement* v) const;

	void addAlias(const Identifier& id, const juce::String& typeString)
	{
		registeredAliases.add({ id, typeString });
	}

	juce::String dump() const
	{
		auto v = toValueTree();
		juce::String s;
		dumpInternal(0, s, v);
		return s;
	}

	

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override;

private:

	

	static void dumpInternal(int intLevel, juce::String& s, const ValueTree& v)
	{
		intLevel++;

		s << Operations::toSyntaxTreeString(v, intLevel);

		for (const auto& c : v)
			dumpInternal(intLevel, s, c);

	};

	struct UsingAliases
	{
		Identifier id;
		juce::String aliasContent;
	};

	Array<UsingAliases> registeredAliases;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyntaxTree);
};

}
}