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


struct NamespacedIdentifier
{
	explicit NamespacedIdentifier(const Identifier& id)
	{
		add(id);
	}

	static NamespacedIdentifier fromString(const juce::String& s)
	{
		auto sa = StringArray::fromTokens(s, "::", "");

		NamespacedIdentifier c;
		for (auto s : sa)
			c.add(Identifier(s));

		return c;
	}

	static NamespacedIdentifier null()
	{
		static const NamespacedIdentifier n("null");
		return n;
	}

	bool isNull() const
	{
		return null() == *this;
	}

	NamespacedIdentifier()
	{}

	Array<Identifier> getIdList() const
	{
		Array<Identifier> l;
		l.addArray(namespaces);
		l.add(id);
		return l;
	}

	std::string debugName;
	Array<Identifier> namespaces;
	Identifier id;

	bool isValid() const
	{
		return id.isValid();
	}

	bool isInGlobalNamespace() const
	{
		return namespaces.isEmpty();
	}

	bool isExplicit() const
	{
		return isInGlobalNamespace();
	}

	Identifier getIdentifier() const
	{
		return id;
	}

	bool isParentOf(const NamespacedIdentifier& other) const
	{
		auto otherName = other.toString();
		auto thisName = toString();
		return otherName.startsWith(thisName);
	}

	void relocateSelf(const NamespacedIdentifier& oldParent, const NamespacedIdentifier& newParent)
	{
		jassert(oldParent.isParentOf(*this));

		NamespacedIdentifier s(newParent);

		auto opl = oldParent.getIdList();
		auto ids = getIdList();

		for (int i = 0; i < ids.size(); i++)
		{
			if (opl[i] != ids[i])
			{
				s = s.getChildId(ids[i]);
			}
		}

		jassert(newParent.isParentOf(s));

		debugName = s.debugName;
		namespaces = s.namespaces;
		id = s.id;
	}

	NamespacedIdentifier relocate(const NamespacedIdentifier& oldParent, const NamespacedIdentifier& newParent) const
	{
		NamespacedIdentifier s(*this);

		s.relocateSelf(oldParent, newParent);
		return s;
	}

	NamespacedIdentifier getChildId(const Identifier& id) const
	{
		auto c = *this;
		c.add(id);
		return c;
	}

	NamespacedIdentifier getParent() const
	{
		if (namespaces.isEmpty())
			return {};

		auto c = *this;
		c.pop();
		return c;
	}

	bool operator !=(const NamespacedIdentifier& other) const
	{
		return !(*this == other);
	}

	bool operator==(const NamespacedIdentifier& other) const
	{
		if (id != other.id)
			return false;

		if (namespaces.size() == other.namespaces.size())
		{
			for (int i = 0; i < namespaces.size(); i++)
			{
				if (namespaces[i] != other.namespaces[i])
					return false;
			}

			return true;
		}

		return false;
	}

	NamespacedIdentifier withId(const Identifier& id) const
	{
		auto c = *this;
		c.add(id);
		return c;
	}

	juce::String toString() const
	{
		juce::String s;

		for (auto n : namespaces)
			s << n << "::";

		s << id;

		return s;
	}

	void add(const Identifier& newId)
	{
		if (id.isValid())
			namespaces.add(id);

		id = newId;

		debugName = toString().toStdString();
	}

	Result pop()
	{
		if (id.isNull())
			return Result::fail("Can't pop namespace");

		id = namespaces.getLast();
		namespaces.removeLast();

		debugName = toString().toStdString();

		return Result::ok();
	}
};

class NamespaceHandler;

struct AssemblyMemory;

struct ComplexType : public ReferenceCountedObject
{
	static int numInstances;

	struct InitData
	{
		AssemblyMemory* asmPtr = nullptr;
		void* dataPointer = nullptr;
		InitialiserList::Ptr initValues;
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

	static void writeNativeMemberType(void* dataPointer, int byteOffset, const VariableStorage& initValue)
	{
		auto dp_raw = getPointerWithOffset(dataPointer, byteOffset);
		auto copy = initValue;

		switch (copy.getType())
		{
		case Types::ID::Integer: *reinterpret_cast<int*>(dp_raw) = (int)initValue; break;
		case Types::ID::Double:  *reinterpret_cast<double*>(dp_raw) = (double)initValue; break;
		case Types::ID::Float:	 *reinterpret_cast<float*>(dp_raw) = (float)initValue; break;
		case Types::ID::Pointer: *((void**)dp_raw) = copy.getDataPointer(); break;
		case Types::ID::Block:	 *reinterpret_cast<block*>(dp_raw) = initValue.toBlock(); break;
		default:				 jassertfalse;
		}

		auto x = (int*)dataPointer;
		int y = 1;

	}

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

	virtual void registerExternalAtNamespaceHandler(NamespaceHandler* handler);

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

	virtual ComplexType::Ptr createSubType(const NamespacedIdentifier& id) { return nullptr; }

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
	{}

	explicit TypeInfo(Types::ID type_, bool isConst_=false, bool isRef_=false):
		type(type_),
		const_(isConst_),
		ref_(isRef_)
	{
		jassert(type != Types::ID::Pointer || isConst_);
	}

	explicit TypeInfo(ComplexType::Ptr p, bool isConst_=false, bool isRef_=true) :
		typePtr(p),
		const_(isConst_),
		ref_(isRef_)
	{
		jassert(p != nullptr);
		type = Types::ID::Pointer;
	}

	explicit TypeInfo(const NamespacedIdentifier& templateTypeId_, bool isConst_ = false, bool isRef_ = false):
		templateTypeId(templateTypeId_),
		const_(isConst_),
		ref_(isRef_)
	{

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

	TypeInfo withModifiers(bool isConst_, bool isRef_) const
	{
		auto c = *this;
		c.const_ = isConst_;
		c.ref_ = isRef_;
		return c;
	}

	juce::String toString() const
	{
		juce::String s;

		if (isConst())
			s << "const ";

		if (isComplexType())
			s << typePtr->toString();
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
		return t;
	}

	TypeInfo asNonConst()
	{
		auto t = *this;
		t.const_ = false;
		return t;
	}

private:

	bool const_ = false;
	bool ref_ = false;
	Types::ID type = Types::ID::Dynamic;
	ComplexType::Ptr typePtr;
	NamespacedIdentifier templateTypeId;
};

class NamespaceHandler;

struct TemplateParameter
{
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
		constant(0),
		constantDefined(false)
	{};

	TemplateParameter(const NamespacedIdentifier& id, int value, bool defined):
		t(IntegerTemplateArgument),
		type({}),
		argumentId(id),
		constant(value),
		constantDefined(defined)
	{
		jassert(isTemplateArgument());
	}

	TemplateParameter(const NamespacedIdentifier& id, const TypeInfo& defaultType = {}) :
		t(TypeTemplateArgument),
		type(defaultType),
		argumentId(id),
		constant(0)
	{
		jassert(isTemplateArgument());
	}


	TemplateParameter(int c) :
		type(TypeInfo()),
		constant(c),
		t(ParameterType::ConstantInteger),
		constantDefined(true)
	{
		jassert(!isTemplateArgument());
	};

	TemplateParameter(const TypeInfo& t) :
		type(t),
		constant(0),
		t(ParameterType::Type)
	{
		jassert(!isTemplateArgument());
	};

	bool isTemplateArgument() const
	{
		return t == IntegerTemplateArgument || t == TypeTemplateArgument;
	}

	bool matchesTemplateType(const TypeInfo& t) const
	{
		jassert(argumentId.isValid());
		return t.getTemplateId() == argumentId;
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

	static juce::String createParameterListString(const List& l);

	TypeInfo type;
	int constant;
	bool constantDefined = false;
	ParameterType t;
	NamespacedIdentifier argumentId;
};

struct TemplateClass
{
	struct ConstructData
	{
		bool expectTemplateParameterAmount(int expectedSize) const
		{
			if (tp.size() != expectedSize)
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
			auto& t = tp[argIndex];

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
			auto& t = tp[argIndex];

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
			auto& t = tp[argIndex];

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

			auto& t = tp[argIndex];

			if (t.constant == illegalNumber)
			{
				*r = Result::fail("Illegal template argument: " + juce::String(illegalNumber));
				return false;
			}

			return true;
		}

		NamespaceHandler* handler;
		NamespacedIdentifier id;
		Array<TemplateParameter> tp;
		juce::Result* r;
	};

	using Constructor = std::function<ComplexType::Ptr(const ConstructData&)>;

	bool operator==(const TemplateClass& other) const
	{
		return id == other.id;
	}

	

	NamespacedIdentifier id;
	Constructor f;
};



struct TemplatedComplexType : public ComplexType
{
	TemplatedComplexType(const TemplateClass& c_, const TemplateClass::ConstructData& d_) :
		c(c_),
		d(d_)
	{

	}

	ComplexType::Ptr createTemplatedInstance(const TemplateParameter::List& suppliedTemplateParameters, juce::Result& r)
	{
		TemplateParameter::List instanceParameters;

		for (const auto& p : d.tp)
		{
			if (p.type.isTemplateType())
			{
				for (const auto& sp : suppliedTemplateParameters)
				{
					if (sp.argumentId == p.type.getTemplateId())
					{
						if (sp.t == TemplateParameter::ConstantInteger)
						{
							TemplateParameter ip(sp.constant);
							ip.argumentId = sp.argumentId;
							instanceParameters.add(ip);
						}
						else
						{
							TemplateParameter ip(sp.type);
							ip.argumentId = sp.argumentId;
							instanceParameters.add(ip);
						}
					}
				}
			}
			else if (p.isTemplateArgument())
			{
				for (const auto& sp : suppliedTemplateParameters)
				{
					if (sp.argumentId == p.argumentId)
					{
						jassert(sp.isResolved());
						TemplateParameter ip = sp;
						instanceParameters.add(ip);
					}
				}
			}
			else
			{
				jassert(p.isResolved());
				instanceParameters.add(p);
			}
		}

		for (auto& p : instanceParameters)
		{
			jassert(p.isResolved());
		}
		
		TemplateClass::ConstructData instanceData = d;
		instanceData.tp = instanceParameters;

		instanceData.r = &r;

		ComplexType::Ptr p = c.f(instanceData);

		

		return p;
	}

	size_t getRequiredByteSize() const override { return 0; }

	size_t getRequiredAlignment() const override { return 0; }

	void dumpTable(juce::String&, int&, void*, void*) const override {}

	Result initialise(InitData d) { return Result::ok(); };

	InitialiserList::Ptr makeDefaultInitialiserList() const { return nullptr; }

	ComplexType::Ptr createSubType(const NamespacedIdentifier& id) override;

	void registerExternalAtNamespaceHandler(NamespaceHandler* handler)
	{
		
	}

	bool forEach(const TypeFunction&, Ptr, void*) { return false; }

	juce::String toStringInternal() const override
	{
		return "template " + c.id.toString();
	}

private:

	TemplateClass c;
	TemplateClass::ConstructData d;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TemplatedComplexType);
	JUCE_DECLARE_WEAK_REFERENCEABLE(TemplatedComplexType);
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

	Inliner(const NamespacedIdentifier& id, const Func& asm_, const Func& highLevel_) :
		functionId(id),
		asmFunc(asm_),
		highLevelFunc(highLevel_)
	{};

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
	jassert(ok);
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

	juce::String getSignature(const Array<Identifier>& parameterIds = {}) const;

	operator bool() const noexcept { return function != nullptr; };

	bool isResolved() const
	{
		return function != nullptr || inliner != nullptr;
	}

	bool matchesArgumentTypes(TypeInfo r, const Array<TypeInfo>& argsList) const;

	bool matchesArgumentTypes(const Array<TypeInfo>& typeList) const;

	bool matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType = true) const;

	bool matchesNativeArgumentTypes(Types::ID r, const Array<Types::ID>& nativeArgList) const;

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



class BaseScope;
class NamespaceHandler;
 
struct InlineData;

struct VariadicSubType : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<VariadicSubType>;

	ComplexType::WeakPtr variadicType;
	NamespacedIdentifier variadicId;
	Array<FunctionData> functions;
	bool isTemplateClass = false;
};


/** A function class is a collection of functions. */
struct FunctionClass: public DebugableObjectBase,
					  public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<FunctionClass>;

	enum SpecialSymbols
	{
		AssignOverload = 0,
		IncOverload,
		NativeTypeCast,
		Subscript,
		ToSimdOp,
		numOperatorOverloads
	};

	static Identifier getSpecialSymbol(SpecialSymbols s)
	{
		switch (s)
		{
		case AssignOverload: return "operator=";
		case NativeTypeCast: return "type_cast";
		case IncOverload:    return "operator++";
		case Subscript:		 return "operator[]";
		case ToSimdOp:		 return "toSimd";
		}

		return {};
	}

	bool hasSpecialFunction(SpecialSymbols s) const
	{
		auto id = getClassName().getChildId(getSpecialSymbol(s));
		return hasFunction(id);
	}

	void addSpecialFunctions(SpecialSymbols s, Array<FunctionData>& possibleMatches) const
	{
		auto id = getClassName().getChildId(getSpecialSymbol(s));
		addMatchingFunctions(possibleMatches, id);
	}

	FunctionData getSpecialFunction(SpecialSymbols s, TypeInfo returnType, const TypeInfo::List& args) const;

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
		f->id = getClassName().getChildId(getSpecialSymbol(s));
		
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

