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

struct TemplateParameter
{
	enum VariadicType
	{
		Single,
		Variadic,
		numVariadicTypes
	};

	enum ParameterType
	{
		Empty,
		ConstantInteger,
		Type,
		IntegerTemplateArgument,
		TypeTemplateArgument,
		numTypes
	};

	TemplateParameter() :
		t(Empty),
		type({}),
		variadic(VariadicType::Single),
		constant(0),
		constantDefined(false)
	{};

	TemplateParameter(const NamespacedIdentifier& id, int value, bool defined, VariadicType vType=VariadicType::Single):
		t(IntegerTemplateArgument),
		type({}),
		argumentId(id),
		constant(value),
		constantDefined(defined),
		variadic(vType)
	{
		jassert(isTemplateArgument());
	}

	TemplateParameter(const NamespacedIdentifier& id, const TypeInfo& defaultType = {}, VariadicType vType = VariadicType::Single) :
		t(TypeTemplateArgument),
		type(defaultType),
		argumentId(id),
		constant(0),
		variadic(vType)
	{
		jassert(isTemplateArgument());
	}


	TemplateParameter(int c, VariadicType vType = VariadicType::Single) :
		type(TypeInfo()),
		constant(c),
		variadic(vType),
		t(ParameterType::ConstantInteger),
		constantDefined(true)
	{
		jassert(!isTemplateArgument());
	};

	TemplateParameter(const TypeInfo& t, VariadicType vType = VariadicType::Single) :
		type(t),
		constant(0),
		variadic(vType),
		t(ParameterType::Type)
	{
		
	};

	bool isTemplateArgument() const
	{
		return t == IntegerTemplateArgument || t == TypeTemplateArgument;
	}

	bool operator !=(const TemplateParameter& other) const
	{
		return !(*this == other);
	}

	bool operator==(const TemplateParameter& other) const
	{
		bool tMatch = t == other.t;
		bool typeMatch = type == other.type;
		bool cMatch = constant == other.constant;
		bool cdMatch = constantDefined == other.constantDefined;
		return tMatch && typeMatch && cMatch && cdMatch;
	}

	bool matchesTemplateType(const TypeInfo& t) const
	{
		jassert(argumentId.isValid());
		return t.getTemplateId() == argumentId;
	}

	TemplateParameter withId(const NamespacedIdentifier& id) const
	{
		// only valid with parameters...
		jassert(!isTemplateArgument());

		auto c = *this;
		c.argumentId = id;
		return c;
	}

	bool isVariadic() const
	{
		return variadic == VariadicType::Variadic;
	}

	bool isResolved() const
	{
		jassert(!isTemplateArgument());

		if (t == Type)
			return type.isValid();
		else
			return constantDefined;
	}

	using List = Array<TemplateParameter>;

	

	struct ListOps
	{
		static juce::String toString(const List& l, bool includeParameterNames = true);

		static List filter(const List& l, const NamespacedIdentifier& id);

		static List merge(const List& arguments, const List& parameters, juce::Result& r);

		static List sort(const List& arguments, const List& parameters, juce::Result& r);

		static List mergeWithCallParameters(const List& argumentList, const List& existing, const TypeInfo::List& originalFunctionArguments, const TypeInfo::List& callParameterTypes, Result& r);

		static Result expandIfVariadicParameters(List& parameterList, const List& parentParameters);

		static bool isVariadicList(const List& l);

		static bool matchesParameterAmount(const List& parameters, int expected);

		static bool isParameter(const List& l);

		static bool isArgument(const List& l);

		static bool isArgumentOrEmpty(const List& l);

		static bool match(const List& first, const List& second);

		static bool isNamed(const List& l);

		static bool isSubset(const List& all, const List& possibleSubSet);

		static bool readyToResolve(const List& l);

		static bool isValidTemplateAmount(const List& argList, int numProvided);
	};

	TypeInfo type;
	int constant;
	bool constantDefined = false;
	VariadicType variadic = VariadicType::Single;
	ParameterType t;
	NamespacedIdentifier argumentId;
};

/** This should be used whenever a class instance is being identified instead of a normal NamespacedIdentifier. 

	It provides additional template parameters that have been used to instantiate the actual template object
	(eg. class template parameters for a templated function). 
*/
struct TemplateInstance
{
	TemplateInstance(const NamespacedIdentifier& id_, const TemplateParameter::List& tp_) :
		id(id_),
		tp(tp_)
	{
		jassert(tp.isEmpty() || TemplateParameter::ListOps::isParameter(tp));

#if JUCE_DEBUG
		String s;
		s << id.toString() << TemplateParameter::ListOps::toString(tp, false);
		debugName = s.toStdString();
#endif
	}

	bool operator==(const TemplateInstance& other) const
	{
		return id == other.id && TemplateParameter::ListOps::match(tp, other.tp);
	}

	bool isValid() const
	{
		return id.isValid();
	}

	bool isParentOf(const TemplateInstance& other) const
	{
		return id.isParentOf(other.id) && TemplateParameter::ListOps::isSubset(other.tp, tp);
	}

	TemplateInstance getChildIdWithSameTemplateParameters(const Identifier& childId)
	{
		auto c = *this;
		c.id = id.getChildId(childId);
		return c;
	}

	String toString() const
	{
		String s;
		s << id.toString();
		s << TemplateParameter::ListOps::toString(tp, false);
		return s;
	}

	NamespacedIdentifier id;
	TemplateParameter::List tp;

#if JUCE_DEBUG
	std::string debugName;
#endif
};

struct SubTypeConstructData
{
	NamespaceHandler* handler;
	NamespacedIdentifier id;
	TemplateParameter::List l;
	Result r = Result::ok();
};




struct SyntaxTreeInlineData;
struct AsmInlineData;

struct InlineData
{
	virtual ~InlineData() {};

	virtual bool isHighlevel() const = 0;

	SyntaxTreeInlineData* toSyntaxTreeData() const;
	AsmInlineData* toAsmInlineData() const;

	Array<TemplateParameter> templateParameters;
};


struct Inliner : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Inliner>;
	using Func = std::function<Result(InlineData* d)>;

	enum InlineType
	{
		HighLevel,
		Assembly,
		AutoReturnType,
		numInlineTypes
	};

	Inliner(const NamespacedIdentifier& id, const Func& asm_, const Func& highLevel_) :
		asmFunc(asm_),
		highLevelFunc(highLevel_)
	{
		if (hasHighLevelInliner())
			inlineType = HighLevel;

		if (hasAsmInliner())
			inlineType = Assembly;
	};

	static Inliner* createFromType(const NamespacedIdentifier& id, InlineType type, const Func& f)
	{
		if (type == Assembly)
			return createAsmInliner(id, f);
		else
			return createHighLevelInliner(id, f);
	}

	static Inliner* createHighLevelInliner(const NamespacedIdentifier& id, const Func& highLevelFunc)
	{
		return new Inliner(id, [](InlineData* b)
		{
			jassert(!b->isHighlevel());
			return Result::fail("must be inlined on higher level");
		}, highLevelFunc);
	}

	static Inliner* createAsmInliner(const NamespacedIdentifier& id, const Func& asmFunc)
	{
		return new Inliner(id, asmFunc, {});
	}

	bool hasHighLevelInliner() const
	{
		return (bool)highLevelFunc;
	}

	bool hasAsmInliner() const
	{
		return (bool)asmFunc;
	}

	Result process(InlineData* d) const;

	InlineType inlineType = numInlineTypes;

	//const NamespacedIdentifier functionId;
	const Func asmFunc;
	const Func highLevelFunc;

	// Optional: returns a TypeInfo
	Func returnTypeFunction;
};

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
			args.add(createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T>()));
	}

	template <typename T1, typename T2> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T1>()));

		args.add(createIndexedSymbol(1, Types::Helpers::getTypeFromTypeId<T2>()));
	}

	template <typename T1, typename T2, typename T3> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T1>()));

		args.add(createIndexedSymbol(1, Types::Helpers::getTypeFromTypeId<T2>()));
		args.add(createIndexedSymbol(2, Types::Helpers::getTypeFromTypeId<T3>()));
	}

	void addArgs(const Identifier& argName, const TypeInfo& t)
	{
		args.add(Symbol(id.getChildId(argName), t));
	}

	template <typename ReturnType> static FunctionData createWithoutParameters(const Identifier& id, void* ptr = nullptr)
	{
		FunctionData d;

		d.id = NamespacedIdentifier(id);
		d.returnType = TypeInfo(Types::Helpers::getTypeFromTypeId<ReturnType>());
		d.function = reinterpret_cast<void*>(ptr);

		return d;
	}

	Symbol createIndexedSymbol(int index, Types::ID t)
	{
		Identifier pId("Param" + juce::String(index));
		return { id.getChildId(pId), TypeInfo(t) };
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

	FunctionData withParent(const NamespacedIdentifier& newParent) const;

	TypeInfo getOrResolveReturnType(ComplexType::Ptr p);

	bool matchIdArgs(const FunctionData& other) const;

	bool matchIdArgsAndTemplate(const FunctionData& other) const;

	bool matchesArgumentTypes(TypeInfo r, const Array<TypeInfo>& argsList) const;

	bool matchesArgumentTypes(const Array<TypeInfo>& typeList) const;

	bool matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType = true) const;

	bool matchesNativeArgumentTypes(Types::ID r, const Array<Types::ID>& nativeArgList) const;

	bool matchesArgumentTypesWithDefault(const Array<TypeInfo>& typeList) const;

	bool hasDefaultParameter(const Symbol& arg) const;

	Inliner::Func getDefaultExpression(const Symbol& s) const;

	bool matchesTemplateArguments(const TemplateParameter::List& l) const;

	/** Checks if the id matches the constructor syntax (parent name == function name). */
	bool isConstructor() const { return id.getIdentifier() == id.getParent().getIdentifier(); }

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

	/** The return type. */
	TypeInfo returnType;

	/** whether the function has any side effects. */
	bool const_;

	Array<TemplateParameter> templateParameters;

	using Argument = Symbol;

	/** The argument list. */
	Array<Argument> args;

	/** A pretty formatted function name for debugging purposes. */
	juce::String functionName;

	DefaultParameter::List defaultParameters;

	/** A wrapped lambda containing the assembly generation code for that function. */
	Inliner::Ptr inliner;

	bool canBeInlined(bool highLevelInlining) const
	{
		if (inliner == nullptr)
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

	template <typename... Parameters> forcedinline void callVoidUnchecked(Parameters... ps) const
	{
		using signature = void(*)(Parameters...);

		auto f_ = (signature)function;
		f_(ps...);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUncheckedWithObject(void* d, Parameters... ps) const
	{
		using signature = ReturnType(*)(void*, Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(d, ps...));
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUnchecked(Parameters... ps) const
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

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callInternal(Parameters... ps) const
	{
		if (function != nullptr)
			return callUnchecked<ReturnType, Parameters...>(ps...);
		else
			return ReturnType();
	}
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
		juce::Result* r;
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

