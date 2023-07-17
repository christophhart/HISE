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

#include <type_traits>



namespace snex {
namespace jit {
using namespace juce;



class NamespaceHandler;




namespace FunctionValidators
{




template <typename Arg1>
static constexpr bool isValidParameterType(const Arg1& a)
{
	auto ok = !Types::Helpers::isPointerType<Arg1>() || std::is_pointer<Arg1>();

	return ok;
}

template <typename Arg1, typename Arg2> 
static constexpr bool isValidParameterType(const Arg1& a1, const Arg2& a2)
{
	return isValidParameterType(a1) && isValidParameterType(a2);
}

template <typename Arg1, typename Arg2, typename Arg3> 
static constexpr bool isValidParameterType(const Arg1& a1, const Arg2& a2, const Arg3& a3)
{
	return isValidParameterType(a1) && 
		   isValidParameterType(a2) && 
		   isValidParameterType(a3);
}


template <typename Arg1, typename Arg2, typename Arg3, typename Arg4> 
static constexpr bool isValidParameterType(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4)
{
	return isValidParameterType(a1) && isValidParameterType(a3) &&
		isValidParameterType(a2) && isValidParameterType(a4);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> 
static constexpr bool isValidParameterType(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4, const Arg5& a5)
{
	return isValidParameterType(a1) && isValidParameterType(a2) &&
		isValidParameterType(a3) && isValidParameterType(a4) && isValidParameterType(a5);
}

}

struct StaticFunctionPointer
{
	String signature;
	String label;
	void* function;
};


/** A wrapper around a function. */
struct FunctionData
{
	struct DefaultParameter: public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<DefaultParameter>;
		
		Symbol id;

		// This inliner will be called with a SyntaxTreeInlineData
		// object and just needs to add a StatementPtr to the args list
		Inliner::Func expressionBuilder;
	};

	template <typename T> void addArgs(bool omitObjPtr=false)
	{
		if(!omitObjPtr || !std::is_same<T, void*>())
			args.add(createIndexedSymbol(0, TypeInfo::fromT<T>()));
	}

	template <typename T1, typename T2> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(createIndexedSymbol(0, TypeInfo::fromT<T1>()));

		args.add(createIndexedSymbol(1, TypeInfo::fromT<T2>()));
	}

	template <typename T1, typename T2, typename T3> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(createIndexedSymbol(0, TypeInfo::fromT<T1>()));

		args.add(createIndexedSymbol(1, TypeInfo::fromT<T2>()));
		args.add(createIndexedSymbol(2, TypeInfo::fromT<T3>()));
	}

	void addArgs(const Identifier& argName, const TypeInfo& t)
	{
		args.add(Symbol(id.getChildId(argName), t));
	}

	template <typename ReturnType> static FunctionData createWithoutParameters(const Identifier& id, void* ptr = nullptr)
	{
		FunctionData d;

		d.id = NamespacedIdentifier(id);
		d.returnType = TypeInfo::fromT<ReturnType>();
		d.function = reinterpret_cast<void*>(ptr);

		return d;
	}

	Symbol createIndexedSymbol(int index, TypeInfo t)
	{
		Identifier pId("Param" + juce::String(index));
		return { id.getChildId(pId), t};
	}


	template <typename ReturnType, typename...Parameters> static FunctionData create(const Identifier& id, ReturnType(*ptr)(Parameters...) = nullptr, bool omitObjectPtr=false)
	{
		FunctionData d = createWithoutParameters<ReturnType>(id, reinterpret_cast<void*>(ptr));
		d.addArgs<Parameters...>(omitObjectPtr);
		return d;
	}

	template <typename T> void setFunction(T* typedFunctionPointer)
	{
		function = reinterpret_cast<void*>(typedFunctionPointer);
	}

	void setDefaultParameter(const Identifier& s, const Inliner::Func& expressionBuilder);

	void setDefaultParameter(const Identifier& s, const VariableStorage& immediateValue);

	juce::String getCodeToInsert() const;

	juce::String getSignature(const Array<Identifier>& parameterIds = {}, bool useFullParameterIds=true) const;

	operator bool() const noexcept { return function != nullptr; };

	/** Returns false if the function is not const or has a non-const reference argument. */
	bool isConstOrHasConstArgs() const;

	bool isConst() const noexcept
	{
		return const_;
	}

	void setConst(bool isConst_)
	{
		const_ = isConst_;
	}

	bool isResolved() const
	{
		return function != nullptr || inliner != nullptr;
	}

	bool hasTemplatedArgumentOrReturnType() const;

	bool hasUnresolvedTemplateParameters() const;

	FunctionData withParent(const NamespacedIdentifier& newParent) const;

	TypeInfo getOrResolveReturnType(ComplexType::Ptr p);

	Symbol toSymbol() const { return { id, returnType }; }

	String createAssembly() const;

	bool matchIdArgs(const FunctionData& other) const;

	bool matchIdArgsAndTemplate(const FunctionData& other) const;

	bool matchesArgumentTypes(TypeInfo r, const Array<TypeInfo>& argsList, bool checkIfEmpty=false) const;

	bool matchesArgumentTypes(const Array<TypeInfo>& typeList, bool checkIfEmpty=false) const;

	bool matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType = true) const;

	bool matchesNativeArgumentTypes(Types::ID r, const Array<Types::ID>& nativeArgList) const;

	bool matchesArgumentTypesWithDefault(const Array<TypeInfo>& typeList) const;

	bool hasDefaultParameter(const Symbol& arg) const;

	bool isValid() const;

	Result validateWithArgs(Types::ID r, const Array<Types::ID>& nativeArgList) const;

	Result validateWithArgs(String returnString, const StringArray& argStrings) const;

	Inliner::Func getDefaultExpression(const Symbol& s) const;

	bool matchesTemplateArguments(const TemplateParameter::List& l) const;

	/** Checks if the id matches the constructor syntax (parent name == function name). */
	bool isConstructor() const { return id.getIdentifier() == id.getParent().getIdentifier(); }

	int getSpecialFunctionType() const;

    template <typename T> bool setInterpretedArgs(int index, T value)
    {
        if(isPositiveAndBelow(index, args.size()))
        {
            if constexpr (std::is_same<T, VariableStorage>())
            {
                args.getReference(index).constExprValue = value;
            }
            else
            {
                auto expectedType = args[0].typeInfo.getType();
                VariableStorage v(value);
                
                if(Types::Helpers::getTypeFromTypeId<T>() != expectedType)
                    v.setWithType(expectedType, (double)value);
                
                args.getReference(index).constExprValue = v;
            }
            
            return true;
        }
            
        return false;
    }
    
    ValueTree createDataLayout(bool addThisPointer) const
    {
        ValueTree c1("Method");
        c1.setProperty("ID", id.getIdentifier().toString(), nullptr);
        c1.setProperty("ReturnType", returnType.toString(false), nullptr);
        
        c1.setProperty("IsResolved", function != nullptr, nullptr);
        
        if(addThisPointer)
        {
            ValueTree t("Arg");
            t.setProperty("ID", "_this_", nullptr);
            t.setProperty("Type", "pointer", nullptr);
            c1.addChild(t, -1, nullptr);
        }
        
        
        
        for(const auto& a: args)
        {
            ValueTree a1("Arg");
            a1.setProperty("ID", a.id.getIdentifier().toString(), nullptr);
            a1.setProperty("Type", Types::Helpers::getCppTypeName(a.typeInfo.getType()), nullptr);
            c1.addChild(a1, -1, nullptr);
        }
        
        return c1;
    }
    
	void setDescription(const juce::String& d, const StringArray& parameterNames = StringArray())
	{
		description = d;

		for (int i = 0; i < args.size(); i++)
		{
			if(parameterNames[i].isNotEmpty())
				args.getReference(i).id = NamespacedIdentifier(parameterNames[i]);
		}
	}

	juce::String description;

	/** the function ID. */
	NamespacedIdentifier id;

	/** If this is not null, the function will be a member function for the given object. */
	void* object = nullptr;

	/** the function pointer. Use call<ReturnType, Args...>() for type checking during debugging. */
	void* function = nullptr;

	size_t numBytes = 0;

	/** The return type. */
	TypeInfo returnType;

	/** whether the function has any side effects. */
	bool const_ = false;

	Array<TemplateParameter> templateParameters;

	using Argument = Symbol;

	/** The argument list. */
	Array<Argument> args;

	/** A pretty formatted function name for debugging purposes. */
	juce::String functionName;

	DefaultParameter::List defaultParameters;

	/** A wrapped lambda containing the assembly generation code for that function. */
	Inliner::Ptr inliner;

	// Set this to true to always avoid inlining
	bool neverInline = false;

	bool canBeInlined(bool highLevelInlining) const
	{
		if (inliner == nullptr)
			return false;

		if (neverInline)
			return false;

		if (!highLevelInlining && inliner->hasAsmInliner())
			return true;

		if (highLevelInlining && inliner->hasHighLevelInliner())
			return true;

		return false;
	}

	Result inlineFunction(InlineData* d) const
	{
		jassert(canBeInlined(d->isHighlevel()));

		if (inliner != nullptr)
			return inliner->process(d);

		return Result::fail("Can't inline");
	}

	void callVoidDynamic(VariableStorage* args, int numArgs) const;

	VariableStorage callDynamic(VariableStorage* args, int numArgs) const;

    VariableStorage callInterpreted(VariableStorage* args, int numArgs) const;
    
	template <typename... Parameters> void callVoid(Parameters... ps) const
	{
		if (function != nullptr)
		{
			if (object != nullptr)
				callVoidUnchecked(object, ps...);
			else
				callVoidUnchecked(ps...);
		}
	}

	template <typename... Parameters> void callVoidUnchecked(Parameters... ps) const
	{
		using signature = void(*)(Parameters...);

		auto f_ = (signature)function;
		f_(ps...);
	}

	template <typename... Parameters> void callVoidUncheckedWithObject(Parameters... ps) const
	{
		jassert(object != nullptr);
		jassert(function != nullptr);
		using signature = void(*)(void*, Parameters...);

		auto f_ = (signature)function;
		f_(object, ps...);
	}

	template <typename ReturnType, typename... Parameters> ReturnType callUncheckedWithObj5ect(Parameters... ps) const
	{
		jassert(object != nullptr);
		jassert(function != nullptr);

		using signature = ReturnType(*)(void*, Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(object, ps...));
	}

	template <typename ReturnType, typename... Parameters> ReturnType callUnchecked(Parameters... ps) const
	{
		using signature = ReturnType(*)(Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(ps...));
	}

	template <typename ReturnType, typename... Parameters> ReturnType call(Parameters... ps) const
	{
		// return type must be pointer for complex objects
		jassert(FunctionValidators::isValidParameterType(ReturnType()));

		// arguments must be pointer for complex objects...
		jassert(FunctionValidators::isValidParameterType(ReturnType(), ps...));

		if(object != nullptr)
			return callInternal<ReturnType>(object, ps...);
		else
			return callInternal<ReturnType>(ps...);
	}

private:

	template <typename ReturnType, typename... Parameters> ReturnType callInternal(Parameters... ps) const
	{
		if (function != nullptr)
			return callUnchecked<ReturnType, Parameters...>(ps...);
		else
			return ReturnType();
	}
};


struct FunctionCollectionBase : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<FunctionCollectionBase>;

	virtual ~FunctionCollectionBase() {};

	virtual FunctionData getFunction(const NamespacedIdentifier& functionId) = 0;

	virtual VariableStorage getVariable(const Identifier& id) const = 0;

	void* getMainObjectPtr();

	virtual size_t getMainObjectSize() const = 0;

	virtual void* getVariablePtr(const Identifier& id) const = 0;

	virtual juce::String dumpTable() = 0;

	virtual Array<NamespacedIdentifier> getFunctionIds() const = 0;

	virtual Array<Symbol> getAllVariables() const = 0;

	virtual ValueTree getDataLayout(int dataIndex) const { return {}; }

protected:

	static NamespacedIdentifier getMainId();
};


struct TemplateObject
{
	struct ConstructData
	{
		bool expectTemplateParameterAmount(int expectedSize) const
		{
			if (!TemplateParameter::ListOps::matchesParameterAmount(tp, expectedSize))
			{
				juce::String s;

				s << "template amount mismatch: ";
				s << juce::String(tp.size());
				s << ", expected: " << juce::String(expectedSize);

				*r = Result::fail(s);
				return false;
			}

			return true;
		}

		bool expectIsComplexType(int argIndex) const
		{
			auto t = tp[argIndex];

			if (!t.type.isComplexType())
			{
				juce::String s;
				s << "template parameter mismatch: ";
				s << t.type.toString();
				s << " expected: complex type";
				*r = Result::fail(s);
				return false;
			}

			return true;
		}

		bool expectType(int argIndex) const
		{
			auto t = tp[argIndex];

			if (t.type.isInvalid())
			{
				juce::String s;
				s << "template parameter mismatch: expected type";
				*r = Result::fail(s);
				return false;
			}

			return true;
		}

		bool expectIsNumber(int argIndex) const
		{
			auto t = tp[argIndex];

			if (t.type.isValid())
			{
				juce::String s;
				s << "template parameter mismatch: ";
				s << t.type.toString();
				s << " expected: integer literal";
				*r = Result::fail(s);
				return false;
			}

			return true;
		}

		bool expectNotIntegerValue(int argIndex, int illegalNumber) const
		{
			if (!expectIsNumber(argIndex))
				return false;

			auto t = tp[argIndex];

			if (t.constant == illegalNumber)
			{
				*r = Result::fail("Illegal template argument: " + juce::String(illegalNumber));
				return false;
			}

			return true;
		}

		ConstructData(const TemplateInstance& id_) :
			id(id_)
		{};

		NamespaceHandler* handler;
		TemplateInstance id;
		TemplateParameter::List tp;
		mutable juce::Result* r;
	};

	using ClassConstructor = std::function<ComplexType::Ptr(const ConstructData&)>;
	using FunctionConstructor = std::function<void(const ConstructData&)>;
	using FunctionArgumentCollector = std::function<TypeInfo::List(void)>;

	bool operator==(const TemplateObject& other) const
	{
		return id == other.id && argList.size() == other.argList.size();
	}

	TemplateObject() :
		id({}, {})
	{};

	TemplateObject(const TemplateInstance& id_) :
		id(id_)
	{};

	TemplateInstance id;
	String description;
	ClassConstructor makeClassType;
	FunctionConstructor makeFunction;
	FunctionArgumentCollector functionArgs;
	TemplateParameter::List argList;
};

struct ComplexTypeWithTemplateParameters
{
	virtual ~ComplexTypeWithTemplateParameters() {};

	virtual TemplateParameter::List getTemplateInstanceParameters() const = 0;
};

struct TemplatedComplexType : public ComplexType,
						      public ComplexTypeWithTemplateParameters
{
	TemplatedComplexType(const TemplateObject& c_, const TemplateObject::ConstructData& d_) :
		c(c_),
		d(d_)
	{

	}

	ComplexType::Ptr createTemplatedInstance(const TemplateParameter::List& suppliedTemplateParameters, juce::Result& r);

	size_t getRequiredByteSize() const override { return 0; }

	size_t getRequiredAlignment() const override { return 0; }

    ValueTree createDataLayout() const override
    {
        
        return {};
    }
    
	void dumpTable(juce::String&, int&, void*, void*) const override {}

	Result initialise(InitData d) { return Result::ok(); };

	InitialiserList::Ptr makeDefaultInitialiserList() const { return nullptr; }

	ComplexType::Ptr createSubType(SubTypeConstructData* sd) override;

	void registerExternalAtNamespaceHandler(NamespaceHandler* handler)
	{

	}

	bool forEach(const TypeFunction&, Ptr, void*) { return false; }

	juce::String toStringInternal() const override
	{
		return "template " + c.id.toString();
	}

	TemplateParameter::List getTemplateInstanceParameters() const override
	{
		return d.tp;
	}

private:

	TemplateObject c;
	TemplateObject::ConstructData d;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TemplatedComplexType);
	JUCE_DECLARE_WEAK_REFERENCEABLE(TemplatedComplexType);
};



} // end namespace jit
} // end namespace snex

