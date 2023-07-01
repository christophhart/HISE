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


struct Operations::Function : public Statement,
	public AsmJitErrorHandler,
	public Operations::FunctionDefinitionBase
{
	static constexpr int InlineScoreThreshhold = 70;

	SET_EXPRESSION_ID(Function);

	Function(Location l, const Symbol& id_) :
		Statement(l),
		FunctionDefinitionBase(id_)
	{

	};

	~Function()
	{
		data = {};
		functionScope = nullptr;
		statements = nullptr;
		parameters.clear();
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		jassert(functionScope == nullptr);
		jassert(statements == nullptr);
		jassert(objectPtr == nullptr);

		auto c = new Function(l, { data.id, data.returnType });
		c->data = data;
		c->code = code;
		c->codeLength = codeLength;
		c->parameters = parameters;
		return c;
	}

	ValueTree toValueTree() const override
	{
		auto t = Statement::toValueTree();
		t.setProperty("Signature", data.getSignature(parameters), nullptr);

		if (classData != nullptr && classData->function != nullptr)
			t.setProperty("FuncPointer", reinterpret_cast<int64>(classData->function), nullptr);

		if (getTypeInfo().isComplexType() && !getTypeInfo().isRef())
		{
			t.setProperty("ReturnBlockSize", getTypeInfo().getRequiredByteSizeNonZero(), nullptr);
		}

		if (statements != nullptr)
			t.addChild(statements->toValueTree(), -1, nullptr);

		return t;
	}

#if SNEX_ASMJIT_BACKEND
	void handleError(asmjit::Error, const char* message, asmjit::BaseEmitter* emitter) override
	{
		throwError(juce::String(message));
	}
#endif

	TypeInfo getTypeInfo() const override { return TypeInfo(data.returnType); }

	void process(BaseCompiler* compiler, BaseScope* scope) override;

	ScopedPointer<FunctionScope> functionScope;

	struct FunctionCompileData
	{
		using InnerFunction = std::function<void(FunctionCompileData&)>;

		FunctionCompileData(FunctionData& ref, BaseCompiler* compiler_, BaseScope* scope_) :
			data(ref),
			compiler(compiler_),
			scope(scope_)
		{

		};

		FunctionScope* functionScopeToUse = nullptr;
		FunctionData& data;
		BaseCompiler* compiler;
		BaseScope* scope;
		ScopedPointer<AsmJitX86Compiler> cc;
		ScopedPointer<AsmJitStringLogger> assemblyLogger;
		AsmJitErrorHandler* errorHandler = nullptr;
		Statement::Ptr statementToCompile = nullptr;
	};

	static void* compileFunction(FunctionCompileData& f, const FunctionCompileData::InnerFunction& func);

	RegPtr objectPtr;
	bool hasObjectPtr;
	FunctionData* classData = nullptr;

	bool isHardcodedFunction = false;
	ComplexType::Ptr hardcodedObjectType;

private:

	/** Compiles the very own syntax tree. */
	void compileSyntaxTree(FunctionCompileData& f);;

	/** Compiles a function call with a asm inliner. */
	void compileAsmInlinerBeforeCodegen(FunctionCompileData& f);;

	// member functions will not be owned by the StructType
	ScopedPointer<FunctionData> ownedMemberFunction;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Function);
};



struct Operations::FunctionCall : public Expression
{
	SET_EXPRESSION_ID(FunctionCall);

	enum CallType
	{
		Unresolved,
		InbuiltFunction,
		MemberFunction,
		StaticFunction,
		ExternalObjectFunction,
		RootFunction,
		GlobalFunction,
		ApiFunction,
		NativeTypeCall, // either block or event
		BaseMemberFunction,
		numCallTypes
	};

	FunctionCall(Location l, Ptr f, const Symbol& id, const Array<TemplateParameter>& tp);;

	void setObjectExpression(Ptr e);

	Ptr getObjectExpression() const
	{
		if (hasObjectExpression)
			return getSubExpr(0);

		return nullptr;
	}

	Statement::Ptr clone(ParserHelpers::CodeLocation l) const override
	{
		auto newFC = new FunctionCall(l, nullptr, Symbol(function.id, function.returnType), function.templateParameters);

		if (getObjectExpression())
		{
			auto clonedObject = getObjectExpression()->clone(l);
			newFC->setObjectExpression(clonedObject);
		}

		for (int i = 0; i < getNumArguments(); i++)
			newFC->addArgument(getArgument(i)->clone(l));

		if (function.isResolved())
			newFC->function = function;

		return newFC;
	}

    Ptr extractFirstArgumentFromConstructor(NamespaceHandler& handler)
    {
        if(auto cType = handler.getComplexType(function.id))
        {
            function.returnType = TypeInfo(cType, false, false);
            return getArgument(0)->clone(location);
        }
        
        return nullptr;
    }
    
	bool tryToResolveType(BaseCompiler* compiler) override;

	ValueTree toValueTree() const override
	{
		auto t = Expression::toValueTree();
        
        auto copy = function;
        
#if 1
        for(int i = 0; i < copy.args.size(); i++)
        {
            auto originalType = copy.args[i].typeInfo;
            auto argType = getArgument(i)->getTypeInfo();
            copy.args.getReference(i).typeInfo = argType.withModifiers(originalType.isConst(), originalType.isRef());
            
        }
#endif
        
		t.setProperty("Signature", copy.getSignature({}, false), nullptr);
        
        if(hasObjectExpression)
        {
            t.setProperty("ObjectType", getSubExpr(0)->getTypeInfo().toString(false), nullptr);
        }
		else if (callType == StaticFunction)
		{
			t.setProperty("ObjectType", copy.id.getParent().toString(), nullptr);
		}
        
		if (getTypeInfo().isComplexType() && !getTypeInfo().isRef())
		{
			t.setProperty("ReturnBlockSize", getTypeInfo().getRequiredByteSizeNonZero(), nullptr);
		}

		const StringArray resolveNames = { "Unresolved", "InbuiltFunction", "MemberFunction", "StaticFunction", "ExternalObjectFunction", "RootFunction", "GlobalFunction", "ApiFunction", "NativeTypeCall", "BaseMemberFunction" };
		t.setProperty("CallType", resolveNames[(int)callType], nullptr);

		if(callType == CallType::BaseMemberFunction)
			t.setProperty("BaseOffset", baseOffset, nullptr);

		if(baseClass != nullptr)
			t.setProperty("BaseObjectType", baseClass->toString(), nullptr);

		return t;
	}

	void addArgument(Ptr arg) 
	{
		addStatement(arg);
	}

	Expression* getArgument(int index) const
	{
		return getSubExpr(!hasObjectExpression ? index : (index + 1)).get();
	}

	int getNumArguments() const
	{
		if (!hasObjectExpression)
			return getNumChildStatements();
		else
			return getNumChildStatements() - 1;
	}

	bool hasSideEffect() const override
	{
		return true;
	}

	bool shouldInlineFunctionCall(BaseCompiler* compiler, BaseScope* scope) const;

	void addDefaultParameterExpressions(const FunctionData& f);

	void inlineAndSetType(BaseCompiler* compiler, const FunctionData& f);

	void inlineFunctionCall(AsmCodeGenerator& acg);

	/*	A function call to a base class method might cause slicing if the object pointer is not
		adjusted to the first base class member byte offset. 
	*/
	void adjustBaseClassPointer(BaseCompiler* compiler, BaseScope* scope);

	TypeInfo getTypeInfo() const override;

	void resolveBaseClassMethods();
	void process(BaseCompiler* compiler, BaseScope* scope);

	bool isVectorOpFunction() const;

	bool resolveWithParameters(Array<TypeInfo> t);

	void convertNumericTypes(Array<TypeInfo>& t);

	CallType callType = Unresolved;
	Array<FunctionData> possibleMatches;
	mutable FunctionData function;
	WeakReference<FunctionClass> fc;
	FunctionClass::Ptr ownedFc;

	bool hasObjectExpression = false;

	ReferenceCountedArray<AssemblyRegister> parameterRegs;

	void setAllowInlining(bool canBeInlined)
	{
		allowInlining = canBeInlined;
	}

	void setPreferFunctionPointerOverAsmInliner(bool shouldPrefer)
	{
		preferFunctionPointer = shouldPrefer;
	}

private:

	bool preferFunctionPointer = false;

	bool allowInlining = true;
	int baseOffset = 0;
	ComplexType::Ptr baseClass;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionCall);

};



}
}
