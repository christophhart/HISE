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


class FunctionScope;

#define SET_EXPRESSION_ID(x) Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER(#x); }


#define PROCESS_IF_NOT_NULL(expr) if (expr != nullptr) expr->process(compiler, scope);
#define COMPILER_PASS(x) if (compiler->getCurrentPass() == x)
#define CREATE_ASM_COMPILER(type) AsmCodeGenerator(getFunctionCompiler(compiler), &compiler->registerPool, type, location, compiler->getOptimizations());
#define SKIP_IF_CONSTEXPR if(isConstExpr()) return;

/** This class has a variable pool that will not exceed the lifetime of the compilation. */
namespace Operations
{
	using FunctionCompiler = AsmJitX86Compiler;

	static FunctionCompiler& getFunctionCompiler(BaseCompiler* c);
	static BaseScope* findClassScope(BaseScope* scope);
	static BaseScope* findFunctionScope(BaseScope* scope);

    static bool callRecursive(ValueTree& root, const std::function<bool(ValueTree&)>& f)
    {
        if(f(root))
            return true;
        
        for(auto c: root)
        {
            if(callRecursive(c, f))
                return true;
        }
        
        return false;
    }

	enum class IterationType
	{
		AllChildStatements,			 //< all child nodes will be iterated
		NoChildInlineFunctionBlocks, //< the iterator will not go inside inlined functions (but if the root is an inline function it will process it)
		NoInlineFunctionBlocks,		 //< the iterator will skip all inline functions (even if it's the root node)
		numIterationTypes
	};

	using RegPtr = AssemblyRegister::Ptr;

#if SNEX_ASMJIT_BACKEND
	static AsmJitRuntime* getRuntime(BaseCompiler* c);
#endif

	using Location = ParserHelpers::CodeLocation;
	using TokenType = ParserHelpers::TokenType;

	

	static juce::String toSyntaxTreeString(const ValueTree& v, int intLevel)
	{
		juce::String s;

		auto lineNumber = (int)v["Line"];



		if (lineNumber < 10)
			s << "0";
		s << juce::String(intLevel) << " ";

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
			SimpleTree,
			numTextFormat
		};

		
		

		using Ptr = ReferenceCountedObjectPtr<Statement>;
		using List = ReferenceCountedArray<Statement>;

		Statement(Location l);;

		void setParent(Statement* parent_)
		{
			parent = parent_;
		}

		template <class T> bool is() const
		{
			return dynamic_cast<const T*>(this) != nullptr;
		}

		String toSimpleTree() const;

		virtual juce::String toString(TextFormat t) const
		{
			
			return {};
		}

		virtual ~Statement() {};

		virtual TypeInfo getTypeInfo() const = 0;
		virtual void process(BaseCompiler* compiler, BaseScope* scope) = 0;

		virtual Ptr clone(Location l) const = 0;

		virtual Identifier getStatementId() const = 0;

		virtual bool hasSideEffect() const 
		{ 
			for (auto s : *this)
			{
				if (s->hasSideEffect())
					return true;
			}

			return false; 
		}

		virtual size_t getRequiredByteSize(BaseCompiler* compiler, BaseScope* scope) const { return 0; }

		Types::ID getType() const
		{
			return getTypeInfo().getType();
		}

		void cloneChildren(Ptr newClone) const
		{
			for (auto s : *this)
				newClone->addStatement(s->clone(newClone->location));
		}

		

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

		virtual bool tryToResolveType(BaseCompiler* compiler)
		{
			for (auto s : *this)
				s->tryToResolveType(compiler);

			if (getTypeInfo().isValid())
				return true;

			return false;
		};

		virtual ValueTree toValueTree() const
		{
			ValueTree v(getStatementId());

			v.setProperty("Line", location.getLine(), nullptr);

			for (auto s : *this)
				v.addChild(s->toValueTree(), -1, nullptr);

			return v;
		}

		void processAllPassesUpTo(BaseCompiler::Pass p, BaseScope* s)
		{
			jassert(currentCompiler != nullptr);

			for (int i = 0; i <= (int)p; i++)
			{
				auto p = (BaseCompiler::Pass)i;
				BaseCompiler::ScopedPassSwitcher sps(currentCompiler, p);
				currentCompiler->executePass(p, s, this);
			}

			jassert(currentPass == p);
		}

		void throwError(const juce::String& errorMessage) const;
		void logOptimisationMessage(const juce::String& m);
		void logWarning(const juce::String& m);
		void logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const juce::String& message);

		Statement** begin() { return childStatements.begin(); }
		Statement** end() { return childStatements.end(); }

		Statement* const* begin() const { return childStatements.begin(); }
		Statement* const* end() const { return childStatements.end(); }

		Statement::Ptr getLastStatement() const
		{
			return childStatements.getLast();
		}

		void processAllChildren(BaseCompiler* compiler, BaseScope* scope)
		{
			for (auto s : *this)
				s->process(compiler, scope);
		}

		bool forEachRecursive(const std::function<bool(Ptr)>& f, IterationType it);

		bool replaceIfOverloaded(Ptr objPtr, List args, FunctionClass::SpecialSymbols overloadType);

		virtual bool isConstExpr() const;

		Location location;

		WeakReference<BaseCompiler> currentCompiler = nullptr;
		BaseScope* currentScope = nullptr;
		BaseCompiler::Pass currentPass;

		WeakReference<Statement> parent;

		void addStatement(Ptr b, bool addFirst=false);

		Ptr replaceInParent(Ptr newExpression);
		Ptr replaceChildStatement(int index, Ptr newExpr);

		void removeNoops();

		void attachAsmComment(const juce::String& message);

		TypeInfo checkAndSetType(int offset, TypeInfo expectedType);

		TypeInfo setTypeForChild(int childIndex, TypeInfo expectedType);

		/** Processes all sub expressions. Call this from your base class. */
		
		void processBaseWithoutChildren(BaseCompiler* compiler, BaseScope* scope)
		{
			jassert(scope != nullptr);

			currentCompiler = compiler;
			currentScope = scope;
			currentPass = compiler->getCurrentPass();

			if (parent == nullptr)
				return;

			

			if (BaseCompiler::isOptimizationPass(currentPass))
			{
				bool found = false;

				for (int i = 0; i < parent->getNumChildStatements(); i++)
					found |= parent->getChildStatement(i).get() == this;

				if (found)
					compiler->executeOptimization(this, scope);
			}

			if (currentPass == BaseCompiler::CodeGeneration && asmComment.isNotEmpty())
			{
				ASMJIT_ONLY(getFunctionCompiler(compiler).setInlineComment(asmComment.getCharPointer().getAddress()));
			}
		}

		void processBaseWithChildren(BaseCompiler* compiler, BaseScope* scope)
		{
			processBaseWithoutChildren(compiler, scope);
			processAllChildren(compiler, scope);
		}

		void replaceMemoryWithExistingReference(BaseCompiler* compiler);

		bool isAnonymousStatement() const;

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

		void setTargetRegister(RegPtr targetToUse, bool clearRegister = true)
		{
			if (reg != nullptr)
				return;

#if REMOVE_REUSABLE_REG
			if (clearRegister)
			{
				jassert(targetToUse->canBeReused());
				targetToUse->clearForReuse();
			}
#endif

			reg = targetToUse;
		}

		String comment;


	private:

		ReferenceCountedArray<Statement> childStatements;


		JUCE_DECLARE_WEAK_REFERENCEABLE(Statement);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Statement);
	};

	using Expression = Statement;

	template <class T> static bool isStatementType(const Statement* t)
	{
		return dynamic_cast<const T*>(t) != nullptr;
	}

	TemplateParameter::List collectParametersFromParentClass(Statement::Ptr p, const TemplateParameter::List& instanceParameters);

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


	static void dumpSyntaxTreeRecursive(Statement::Ptr p, juce::String& s, int& intLevel)
	{
		s << Operations::toSyntaxTreeString(p->toValueTree(), intLevel);

		intLevel++;

		for (const auto& c : *p)
			dumpSyntaxTreeRecursive(c, s, intLevel);

		intLevel--;
	}

	static bool isOpAssignment(Expression::Ptr p);

	template <class T> static T* as(Statement::Ptr p)
	{
		return dynamic_cast<T*>(p.get());
	}

	static bool canBeReferenced(Expression::Ptr p);

	static Expression::Ptr evalConstExpr(Expression::Ptr expr);

	struct Assignment;		struct Immediate;				struct Noop;
	struct FunctionCall;	struct ReturnStatement;			struct StatementBlock;
	struct Function;		struct BinaryOp;				struct VariableReference;
	struct TernaryOp;		struct LogicalNot;				struct Cast;
	struct Negation;		struct Compare;					struct UnaryOp;
	struct Increment;		struct DotOperator;				struct Loop;	
	struct VectorOp;
	struct IfStatement;		struct ClassStatement;			struct Subscript;				
	struct InlinedParameter;struct ComplexTypeDefinition;	struct ControlFlowStatement;
	struct InlinedArgument; struct MemoryReference;			struct TemplateDefinition; 
	struct TemplatedTypeDef; struct TemplatedFunction;		struct ThisPointer;
	struct WhileLoop;		struct PointerAccess;			struct AnonymousBlock;
	struct InternalProperty;

	
	

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

			returnRegister = c->registerPool.getNextFreeRegister(s, getReturnType().toPointerIfNativeRef());
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

		void addDestructors(BaseScope* scope);

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

		void removeStatementsAfterReturn();

		NamespacedIdentifier getPath() const { return path; }

		void setNewPath(BaseCompiler* c, const NamespacedIdentifier& newPath);

	protected:

		RegPtr returnRegister;
		TypeInfo returnType;

	private:

		NamespacedIdentifier path;
		WeakReference<ScopeStatementBase> parentScopeStatement;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScopeStatementBase);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeStatementBase);
	};

	struct StatementWithControlFlowEffectBase
	{
		virtual ~StatementWithControlFlowEffectBase() {};

		/** Override this function and return the statement block that is affected by this statement. 
			For return statements it's most likely the root syntax tree (or the inlined function block) and
			for break / continue statements it'll be the loop body. 
		*/
		virtual ScopeStatementBase* findRoot() const = 0;

		virtual bool shouldAddDestructor(ScopeStatementBase* b, const Symbol& id) const = 0;

		static void addDestructorToAllChildStatements(Statement::Ptr p, const Symbol& id);
	};

	/** Just a empty base class that checks whether the global variables will be loaded
		before the branch.
	*/
	struct ConditionalBranch
	{
		static void preallocateVariableRegistersBeforeBranching(Statement::Ptr stament, BaseCompiler* c, BaseScope* s);

		virtual AsmJitLabel getJumpTargetForEnd(bool getContinue) = 0;

		virtual ~ConditionalBranch() {}

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(ConditionalBranch);
	};

	struct FunctionDefinitionBase
	{
		FunctionDefinitionBase(const Symbol& s) :
			code(nullptr)
		{
			data.id = s.id;
			data.returnType = s.typeInfo;
		}

		virtual ~FunctionDefinitionBase()
		{
			data = {};
		}

		FunctionData data;
		juce::String::CharPointerType code;
		int codeLength = 0;
		Statement::Ptr statements;

		Array<Identifier> parameters;
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

	struct ArrayStatementBase
	{
		enum ArrayType
		{
			Undefined,
			Span,
			Dyn,
			CustomObject,
			numSubscriptTypes
		};

		virtual ~ArrayStatementBase() {};

		virtual ArrayType getArrayType() const = 0;
	};


	struct ClassDefinitionBase
	{
		virtual ~ClassDefinitionBase() {};

		virtual bool isTemplate() const = 0;

		void addMembersFromStatementBlock(StructType* t, Statement::Ptr bl);
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

	struct TemplateParameterResolver
	{
		TemplateParameterResolver(const TemplateParameter::List& tp_);;

		Result process(FunctionData& f) const;
		Result process(Statement::Ptr p);
		Result processType(TypeInfo& t) const;

		Result resolveIds(FunctionData& d) const;
		Result resolveIdForType(TypeInfo& t) const;

		TemplateParameter::List tp;
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

	int getInlinerScore();

	Identifier getStatementId() const override { RETURN_STATIC_IDENTIFIER("SyntaxTree"); }

	void process(BaseCompiler* compiler, BaseScope* scope) override
	{
		processBaseWithChildren(compiler, scope);
		
		COMPILER_PASS(BaseCompiler::DataAllocation)
		{
			removeStatementsAfterReturn();
			addDestructors(scope);
		}

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

struct InitialiserList::MemberPointer: public InitialiserList::ChildBase
{
	MemberPointer(StructType* st_, const Identifier& id) :
		st(st_),
		variableId(id)
	{};

	juce::String toString() const override
	{
		return variableId.toString();
	}

	bool getValue(VariableStorage& v) const override
	{
		v = value;
		return !v.isVoid();
	}

	InitialiserList::Ptr createChildList() const override
	{
		InitialiserList::Ptr n = new InitialiserList();
		n->addChild(new MemberPointer(st, variableId));
		return n;
	}

	StructType* st;
	Identifier variableId;
	VariableStorage value;
};

struct InitialiserList::ExpressionChild : public InitialiserList::ChildBase
{
	ExpressionChild(Operations::Expression::Ptr e) :
		expression(e)
	{};

	juce::String toString() const override
	{
		if(expressionIndex == -1)
			return expression->toString(Operations::Statement::TextFormat::CppCode);
		else
		{
            if(expression->isConstExpr())
            {
                auto v = expression->getConstExprValue();
                return Types::Helpers::getCppValueString(v);
            }
            else
            {
                String s;
                s << "$";
                s << Types::Helpers::getTypeName(expression->getType())[0];
                s << String(expressionIndex);
                return s;
            }
            
			
		}
	}

	

	InitialiserList::Ptr createChildList() const override
	{
		auto p = new InitialiserList();

		auto e = new ExpressionChild(expression);
		p->addChild(e);
		return p;
	}

	bool getValue(VariableStorage& v) const override
	{
		if (!value.isVoid())
		{
			v = value;
			return true;
		}

		if (expression->currentScope != nullptr &&
			expression->currentCompiler != nullptr &&
			expression->currentScope->getParent() != nullptr)
		{
			auto cExpression = Operations::evalConstExpr(expression);

			if (cExpression->isConstExpr())
			{
                expression = cExpression;
                
				v = cExpression->getConstExprValue();
                return true;
			}
		}

		
		
		return false;
	}

	mutable Operations::Expression::Ptr expression;
	mutable VariableStorage value;
	mutable int expressionIndex = -1;
};

juce::ReferenceCountedObject* InitialiserList::getExpression(int index)
{
	if (auto child = root[index])
	{
		if (auto ec = dynamic_cast<ExpressionChild*>(child.get()))
		{
			return ec->expression.get();
		}
	}

	return nullptr;
}



}
