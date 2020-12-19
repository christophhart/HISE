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

class FunctionClass;



class NamespaceHandler;

struct AssemblyMemory;

struct SubTypeConstructData;
struct FunctionData;

struct ComplexType : public ReferenceCountedObject
{
	static int numInstances;

	struct InitData
	{
		AssemblyMemory* asmPtr = nullptr;
		void* dataPointer = nullptr;
		InitialiserList::Ptr initValues;
		bool callConstructor = false;
	};

	ComplexType()
	{
		numInstances++;
	};

	virtual ~ComplexType() { numInstances--; };

	static void* getPointerWithOffset(void* data, size_t byteOffset)
	{
		return reinterpret_cast<void*>((uint8*)data + byteOffset);
	}

	static void writeNativeMemberTypeToAsmStack(const ComplexType::InitData& d, int initIndex, int offsetInBytes, int size);

	static void writeNativeMemberType(void* dataPointer, int byteOffset, const VariableStorage& initValue);

	using Ptr = ReferenceCountedObjectPtr<ComplexType>;
	using WeakPtr = WeakReference<ComplexType>;

	using TypeFunction = std::function<bool(Ptr, void* dataPointer)>;

	/** Override this and return the size of the object. It will be used by the allocator to create the memory. */
	virtual size_t getRequiredByteSize() const = 0;

	virtual size_t getRequiredAlignment() const = 0;

	/** Override this and optimise the alignment. After this call the data structure must not be changed. */
	virtual void finaliseAlignment() { finalised = true; };

	virtual void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const = 0;

	virtual Result initialise(InitData d) = 0;

	virtual InitialiserList::Ptr makeDefaultInitialiserList() const = 0;

	virtual Result callConstructor(void* data, InitialiserList::Ptr initList);

	virtual bool hasConstructor();

	virtual void registerExternalAtNamespaceHandler(NamespaceHandler* handler, const juce::String& description);

	virtual Types::ID getRegisterType(bool allowSmallObjectOptimisation) const 
	{ 
		ignoreUnused(allowSmallObjectOptimisation);
		return Types::ID::Pointer; 
	}

	/** Override this, check if the type matches and call the function for itself and each member recursively and abort if t returns true. */
	virtual bool forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer) = 0;

	/** Override this and return a function class object containing methods that are performed on this type. The object returned by this function must be owned by the caller (because keeping a member object will most likely create a cyclic reference).
	*/
	virtual FunctionClass* getFunctionClass() { return nullptr; };

	bool isFinalised() const { return finalised; }

	bool operator ==(const ComplexType& other) const
	{
		return matchesOtherType(other);
	}

	virtual bool isValidCastSource(Types::ID nativeSourceType, ComplexType::Ptr complexSourceType) const;

	virtual bool isValidCastTarget(Types::ID nativeTargetType, ComplexType::Ptr complexTargetType) const;

	virtual ComplexType::Ptr createSubType(SubTypeConstructData* ) { return nullptr; }

	

	int hash() const
	{
		return (int)toString().hash();
	}

	void setAlias(const NamespacedIdentifier& newAlias)
	{
		usingAlias = newAlias;
	}

	NamespacedIdentifier getAlias() const
	{
		jassert(hasAlias());
		return usingAlias;
	}

	juce::String toString() const
	{
		if (hasAlias())
			return usingAlias.toString();
		else
			return toStringInternal();
	}

	bool hasAlias() const 
	{ 
		return usingAlias.isValid(); 
	}

	virtual bool matchesOtherType(const ComplexType& other) const
	{
		if (usingAlias.isValid() && other.usingAlias == usingAlias)
			return true;

		if (toStringInternal() == other.toStringInternal())
			return true;

		return false;
	}

	bool matchesId(const NamespacedIdentifier& id) const
	{
		if (id == usingAlias)
			return true;

		if (toStringInternal() == id.toString())
			return true;

		return false;
	}

	juce::String getActualTypeString() const
	{
		return toStringInternal();
	}

private:

	/** Override this and create a string representation. This must be "unique" in a sense that pointers to types with the same string can be interchanged. */
	virtual juce::String toStringInternal() const = 0;

	bool finalised = false;
	NamespacedIdentifier usingAlias;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComplexType);
	JUCE_DECLARE_WEAK_REFERENCEABLE(ComplexType);
};



struct TypeInfo
{
	using List = Array<TypeInfo>;

	TypeInfo():
		type(Types::ID::Dynamic)
	{
		updateDebugName();
	}

	explicit TypeInfo(Types::ID type_, bool isConst_=false, bool isRef_=false, bool isStatic_=false):
		type(type_),
		const_(isConst_),
		ref_(isRef_),
		static_(isStatic_)
	{
		jassert(!(type == Types::ID::Void && isRef()));

		jassert(type != Types::ID::Pointer || isConst_);
		updateDebugName();
	}

	explicit TypeInfo(ComplexType::Ptr p, bool isConst_=false, bool isRef_=false) :
		typePtr(p),
		const_(isConst_),
		ref_(isRef_),
		static_(false)
	{
		jassert(p != nullptr);
		type = Types::ID::Pointer;
		updateDebugName();
	}

	explicit TypeInfo(const NamespacedIdentifier& templateTypeId_, bool isConst_ = false, bool isRef_ = false):
		templateTypeId(templateTypeId_),
		const_(isConst_),
		ref_(isRef_),
		static_(false)
	{
		updateDebugName();
	}

	

	

	bool isDynamic() const noexcept
	{
		return !isTemplateType() && type == Types::ID::Dynamic;
	}

	bool isValid() const noexcept
	{
		return !isInvalid();
	}

	bool isTemplateType() const noexcept
	{
		return templateTypeId.isValid();
	}
	
	bool isInvalid() const noexcept
	{
		return type == Types::ID::Void || type == Types::ID::Dynamic;
	}

	bool operator!=(const TypeInfo& other) const
	{
		return !(*this == other);
	}

	bool operator==(const TypeInfo& other) const
	{
		if (typePtr != nullptr)
		{
			if (other.typePtr != nullptr)
				return *typePtr == *other.typePtr;

			return false;
		}

		return getType() == other.type;
	}

	bool operator==(const Types::ID other) const
	{
		return getType() == other;
	}

	bool operator!=(const Types::ID other) const
	{
		return getType() != other;
	}

	size_t getRequiredByteSize() const
	{
		if (isComplexType())
			return typePtr->getRequiredByteSize();
		else
			return Types::Helpers::getSizeForType(type);
	}

	size_t getRequiredAlignment() const
	{
		if (isComplexType())
			return typePtr->getRequiredAlignment();
		else
			return Types::Helpers::getSizeForType(type);
	}

	NamespacedIdentifier getTemplateId() const { return templateTypeId; }

	TypeInfo withModifiers(bool isConst_, bool isRef_, bool isStatic_=false) const
	{
		auto c = *this;
		c.const_ = isConst_;
		c.ref_ = isRef_;
		c.static_ = isStatic_;

		

		c.updateDebugName();
		return c;
	}

	juce::String toString() const
	{
		juce::String s;

		if (isStatic())
			s << "static ";

		if (isConst())
			s << "const ";

		if (isComplexType())
		{
			s << typePtr->toString();

			if (isRef())
				s << "&";
		}
			
		else
		{
			s << Types::Helpers::getTypeName(type);

			if (isRef())
				s << "&";
		}

		return s;
	}

	InitialiserList::Ptr makeDefaultInitialiserList() const
	{
		if (isComplexType())
			return getComplexType()->makeDefaultInitialiserList();
		else
		{
			jassert(getType() != Types::ID::Pointer);
			InitialiserList::Ptr p = new InitialiserList();
			p->addImmediateValue(VariableStorage(getType(), 0.0));

			return p;
		}
	}

	void setType(Types::ID newType)
	{
		jassert(newType != Types::ID::Pointer);
		type = newType;
		updateDebugName();
	}

	bool isNativePointer() const
	{
		return !isComplexType() && type == Types::ID::Pointer;
	}

	TypeInfo toPointerIfNativeRef() const
	{
		if (!isComplexType() && isRef())
			return TypeInfo(Types::ID::Pointer, true);

		return *this;
	}

	Types::ID getRegisterType(bool allowSmallObjectOptimisation) const noexcept
	{
		if (isComplexType())
			return getComplexType()->getRegisterType(allowSmallObjectOptimisation);

		return getType();
	}

	Types::ID getType() const noexcept
	{
		if (isComplexType())
			return Types::ID::Pointer;

		return type;
	}

	template <class CType> CType* getTypedComplexType() const
	{
		static_assert(std::is_base_of<ComplexType, CType>(), "Not a base class");

		return dynamic_cast<CType*>(getComplexType().get());
	}

	template <class CType> CType* getTypedIfComplexType() const
	{
		if(isComplexType())
			return dynamic_cast<CType*>(getComplexType().get());

		return nullptr;
	}

	bool isConst() const noexcept
	{
		return const_;
	}

	bool isRef() const noexcept
	{
		return ref_;
	}

	bool isStatic() const noexcept
	{
		if (static_)
		{
			jassert(!isComplexType());
			jassert(!isRef());
		}

		return static_;
	}

	ComplexType::Ptr getComplexType() const
	{
		jassert(type == Types::ID::Pointer);
		jassert(typePtr != nullptr);

		return typePtr.get();
	}

	bool isComplexType() const
	{
		return typePtr != nullptr;
	}

	TypeInfo asConst()
	{
		auto t = *this;
		t.const_ = true;
		t.updateDebugName();
		return t;
	}

	TypeInfo asNonConst()
	{
		auto t = *this;
		t.const_ = false;
		t.updateDebugName();
		return t;
	}

	

private:

	void updateDebugName()
	{
#if JUCE_DEBUG
		debugName = toString().toStdString();
#endif

		jassert(!(type == Types::ID::Void && isRef()));
	}

#if JUCE_DEBUG
	std::string debugName;
#endif

	bool static_ = false;
	bool const_ = false;
	bool ref_ = false;
	Types::ID type = Types::ID::Dynamic;
	ComplexType::Ptr typePtr;
	NamespacedIdentifier templateTypeId;
};

struct ExternalTypeParser
{
	ExternalTypeParser(String::CharPointerType location, String::CharPointerType wholeProgram);

	String::CharPointerType getEndOfTypeName()
	{
		return l;
	}

	Result getResult() const { return parseResult; }

	TypeInfo getType() const { return type; }

private:

	TypeInfo type;

	String::CharPointerType l;
	Result parseResult;
};

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



/** A Symbol is used to identify the data slot. */
struct Symbol
{
	Symbol(const Identifier& singleId):
		id(NamespacedIdentifier(singleId)),
		resolved(false),
		typeInfo({})
	{}

	Symbol():
		id(NamespacedIdentifier::null()),
		resolved(false),
		typeInfo({})
	{}

	Symbol(const NamespacedIdentifier& id_, const TypeInfo& t) :
		id(id_),
		typeInfo(t),
		resolved(!t.isDynamic())
	{};
	

	bool operator==(const Symbol& other) const
	{
		return id == other.id;
	}

	bool matchesIdAndType(const Symbol& other) const
	{
		return id == other.id && typeInfo == other.typeInfo;
	}

	Symbol getParentSymbol(NamespaceHandler* handler) const;

	Symbol getChildSymbol(const Identifier& childName, NamespaceHandler* handler) const;

	Identifier getName() const
	{
		//jassert(!resolved);
		return id.getIdentifier();
	}

	bool isParentOf(const Symbol& otherSymbol) const
	{
		return otherSymbol.id.getParent() == id;
	}

	bool isConst() const { return typeInfo.isConst(); }

	bool isConstExpr() const { return !constExprValue.isVoid(); }


	Types::ID getRegisterType() const
	{
		return typeInfo.isRef() ? Types::Pointer : typeInfo.getType();
	}

	NamespacedIdentifier getId() const { return id; }

	bool isReference() const { return typeInfo.isRef(); };

	juce::String toString() const
	{
		juce::String s;

		if (resolved)
			s << typeInfo.toString() << " ";
		else
			s << "unresolved ";

		s << id.toString();

		return s;
	}

	operator bool() const;

	NamespacedIdentifier id;
	bool resolved = false;
	VariableStorage constExprValue = {};
	TypeInfo typeInfo;

private:

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
		functionId(id),
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
		auto ok = id.getIdentifier() == Identifier("reset4");

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

	const NamespacedIdentifier functionId;
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

class BaseScope;
class NamespaceHandler;
 
struct InlineData;



/** A function class is a collection of functions. */
struct FunctionClass: public DebugableObjectBase,
					  public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<FunctionClass>;

	enum SpecialSymbols
	{
		AssignOverload = 0,
		IncOverload,
		BeginIterator,
		SizeFunction,
		NativeTypeCast,
		Subscript,
		ToSimdOp,
		Constructor,
		numOperatorOverloads
	};

	static Identifier getSpecialSymbol(const NamespacedIdentifier& classId, SpecialSymbols s)
	{
		switch (s)
		{
		case AssignOverload: return "operator=";
		case NativeTypeCast: return "type_cast";
		case IncOverload:    return "operator++";
		case Subscript:		 return "operator[]";
		case ToSimdOp:		 return "toSimd";
		case BeginIterator:  return "begin";
		case SizeFunction:	 return "size";
		case Constructor:    return classId.getIdentifier();
		}

		return {};
	}

	bool hasSpecialFunction(SpecialSymbols s) const
	{
		

		auto id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));
		return hasFunction(id);
	}


	void addSpecialFunctions(SpecialSymbols s, Array<FunctionData>& possibleMatches) const
	{
		auto id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));
		addMatchingFunctions(possibleMatches, id);
	}

	FunctionData getConstructor(const Array<TypeInfo>& args);

	FunctionData getConstructor(InitialiserList::Ptr initList);

	static bool isConstructor(const NamespacedIdentifier& id)
	{
		return id.isValid() && id.getParent().getIdentifier() == id.getIdentifier();
	}

	FunctionData getSpecialFunction(SpecialSymbols s, TypeInfo returnType = {}, const TypeInfo::List& args = {}) const;

	FunctionData getNonOverloadedFunctionRaw(NamespacedIdentifier id) const
	{
		for (auto f : functions)
		{
			if (f->id == id)
				return *f;
		}

		return {};
	}
	

	FunctionData getNonOverloadedFunction(NamespacedIdentifier id) const
	{
		if (id.getParent() != getClassName())
		{
			id = getClassName().getChildId(id.getIdentifier());
		}

		Array<FunctionData> matches;

		addMatchingFunctions(matches, id);

		if (matches.size() == 1)
			return matches.getFirst();

		return {};
	}

	struct Constant
	{
		Identifier id;
		VariableStorage value;
	};

	FunctionClass(const NamespacedIdentifier& id) :
		classSymbol(id)
	{};

	virtual ~FunctionClass()
	{
		registeredClasses.clear();
		functions.clear();
	};

	// =========================================================== DebugableObject overloads

	static ValueTree createApiTree(FunctionClass* r)
	{
		ValueTree p(r->getObjectName());

		for (auto f : r->functions)
		{
			ValueTree m("method");
			m.setProperty("name", f->id.toString(), nullptr);
			m.setProperty("description", f->getSignature(), nullptr);

			juce::String arguments = "(";

			int index = 0;

			for (auto arg : f->args)
			{
				index++;

				if (arg.typeInfo.getType() == Types::ID::Block && r->getObjectName().toString() == "Block")
					continue;

				arguments << Types::Helpers::getTypeName(arg.typeInfo.getType());

				if (arg.id.isValid())
					arguments << " " << arg.id.toString();

				if (index < (f->args.size()))
					arguments << ", ";
			}

			arguments << ")";

			m.setProperty("arguments", arguments, nullptr);
			m.setProperty("returnType", f->returnType.toString(), nullptr);
			m.setProperty("description", f->description, nullptr);
			m.setProperty("typenumber", (int)f->returnType.getType(), nullptr);

			p.addChild(m, -1, nullptr);
		}

		return p;
	}

	ValueTree getApiValueTree() const
	{
		ValueTree v("Api");

		for (auto r : registeredClasses)
		{
			v.addChild(createApiTree(r), -1, nullptr);
		}

		return v;
	}

	FunctionData& createSpecialFunction(SpecialSymbols s)
	{
		auto f = new FunctionData();
		f->id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));
		
		addFunction(f);
		return *f;
	}

	juce::String getCategory() const override { return "API call"; };

	Identifier getObjectName() const override { return classSymbol.toString(); }

	juce::String getDebugValue() const override { return classSymbol.toString(); };

	juce::String getDebugDataType() const override { return "Class"; };

	void getAllFunctionNames(Array<NamespacedIdentifier>& functions) const 
	{
		functions.addArray(getFunctionIds());
	};

	void setDescription(const juce::String& s, const StringArray& names)
	{
		if (auto last = functions.getLast())
			last->setDescription(s, names);
	}

	virtual void getAllConstants(Array<Identifier>& ids) const 
	{
		for (auto c : constants)
			ids.add(c.id);
	};

	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override
	{
		for (auto& c : constants)
		{
			if (c.id == id)
			{
				auto s = new SettableDebugInfo();
				s->codeToInsert << getInstanceName().toString() << "." << id.toString();
				s->dataType = "Constant";
				s->type = Types::Helpers::getTypeName(c.value.getType());
				s->typeValue = ApiHelpers::DebugObjectTypes::Constants;
				s->value = Types::Helpers::getCppValueString(c.value);
				s->name = s->codeToInsert;
				s->category = "Constant";

				return s;
			}
		}

		return nullptr;
	}

	const var getConstantValue(int index) const 
	{ 
		return var(constants[index].value.toDouble());
	};

	VariableStorage getConstantValue(const NamespacedIdentifier& s) const;

	// =====================================================================================

	virtual bool hasFunction(const NamespacedIdentifier& s) const;

	bool hasConstant(const NamespacedIdentifier& s) const;

	void addFunctionConstant(const Identifier& constantId, VariableStorage value);

	virtual void addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const;

	void addFunctionClass(FunctionClass* newRegisteredClass);

	void removeFunctionClass(const NamespacedIdentifier& id)
	{
		for (auto c : registeredClasses)
		{
			if (c->getClassName() == id)
			{
				registeredClasses.removeObject(c);
				return;
			}
		}
	}

	void addFunction(FunctionData* newData);

	Array<NamespacedIdentifier> getFunctionIds() const;

	const NamespacedIdentifier& getClassName() const { return classSymbol; }

	bool fillJitFunctionPointer(FunctionData& dataWithoutPointer);

	bool injectFunctionPointer(FunctionData& dataToInject);

	FunctionClass* getSubFunctionClass(const NamespacedIdentifier& id)
	{
		for (auto f : registeredClasses)
		{
			if (f->getClassName() == id)
				return f;
		}

		return nullptr;
	}

	bool isInlineable(const NamespacedIdentifier& id) const
	{
		for (auto& f : functions)
			if (f->id == id)
				return f->inliner != nullptr;

		return false;
	}

	Inliner::Ptr getInliner(const NamespacedIdentifier& id) const
	{
		for (auto f : functions)
		{
			if (f->id == id)
				return f->inliner;
		}

		return nullptr;
	}

	void addInliner(const Identifier& id, const Inliner::Func& asmFunc)
	{
		auto nId = getClassName().getChildId(id);

		if (isInlineable(nId))
			return;

		for (auto& f : functions)
		{
			if (f->id == nId)
				f->inliner = new Inliner(nId, asmFunc, {});
		}
	}

protected:

	ReferenceCountedArray<FunctionClass> registeredClasses;

	NamespacedIdentifier classSymbol;
	OwnedArray<FunctionData> functions;

	Array<Constant> constants;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionClass);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionClass)
};




} // end namespace jit
} // end namespace snex

