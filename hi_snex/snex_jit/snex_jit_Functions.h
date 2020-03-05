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

struct ComplexType : public ReferenceCountedObject
{
	static void* getPointerWithOffset(void* data, size_t byteOffset)
	{
		return reinterpret_cast<void*>((uint8*)data + byteOffset);
	}

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
		case Types::ID::Event:	 *reinterpret_cast<HiseEvent*>(dp_raw) = initValue.toEvent(); break;
		case Types::ID::Block:	 *reinterpret_cast<block*>(dp_raw) = initValue.toBlock(); break;
		default:				 jassertfalse;
		}

		auto x = (int*)dataPointer;
		int y = 1;

	}

	using Ptr = ReferenceCountedObjectPtr<ComplexType>;

	using TypeFunction = std::function<bool(Ptr, void* dataPointer)>;

	/** Override this and return the size of the object. It will be used by the allocator to create the memory. */
	virtual size_t getRequiredByteSize() const = 0;

	

	/** Override this and return the actual data type that this type operates on. */
	virtual Types::ID getDataType() const = 0;

	virtual size_t getRequiredAlignment() const = 0;

	/** Override this and optimise the alignment. After this call the data structure must not be changed. */
	virtual void finaliseAlignment() { finalised = true; };

	virtual void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const = 0;

	virtual Result initialise(void* dataPointer, InitialiserList::Ptr initValues) = 0;

	virtual InitialiserList::Ptr makeDefaultInitialiserList() const = 0;

	/** Override this, check if the type matches and call the function for itself and each member recursively and abort if t returns true. */
	virtual bool forEach(const TypeFunction& t, Ptr typePtr, void* dataPointer) = 0;

	/** Override this and return a function class object containing methods that are performed on this type. */
	virtual FunctionClass* getFunctionClass() { return nullptr; };

	bool isFinalised() const { return finalised; }

	bool operator ==(const ComplexType& other) const
	{
		return hash() == other.hash();
	}

	int hash() const
	{
		return (int)toString().hash();
	}

	void setAlias(const Identifier& newAlias)
	{
		usingAlias = newAlias;
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

	bool matchesId(const Identifier& id) const
	{
		if (id == usingAlias)
			return true;

		if (Identifier(toStringInternal()) == id)
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
	Identifier usingAlias;
};



struct WrapType : public ComplexType
{
	enum OpType
	{
		Inc,
		Dec,
		Set,
		numOpTypes
	};

	WrapType(int size_) :
		size(size_)
	{
		finaliseAlignment();
	};

	size_t getRequiredByteSize() const override { return 4; }

	/** Override this and return the actual data type that this type operates on. */
	virtual Types::ID getDataType() const override { return Types::ID::Integer; }

	virtual size_t getRequiredAlignment() const override { return 4; }

	/** Override this and optimise the alignment. After this call the data structure must not be changed. */
	virtual void finaliseAlignment() { ComplexType::finaliseAlignment(); };

	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
	{
		auto v = juce::String(*reinterpret_cast<int*>(complexTypeStartPointer));

		s << v << "\n";
	}

	InitialiserList::Ptr makeDefaultInitialiserList() const override
	{
		return InitialiserList::makeSingleList(VariableStorage(0));
	}

	Result initialise(void* dataPointer, InitialiserList::Ptr initValues) override
	{
		if (initValues->size() != 1)
			return Result::fail("Can't initialise with more than one value");

		VariableStorage v;
		initValues->getValue(0, v);

		*reinterpret_cast<int*>(dataPointer) = v.toInt() % size;

		return Result::ok();
	}

	/** Override this, check if the type matches and call the function for itself and each member recursively and abort if t returns true. */
	bool forEach(const TypeFunction& , Ptr , void* ) override
	{
		return false;
	}

	juce::String toStringInternal() const override
	{
		juce::String w;
		w << "wrap<" << juce::String(size) << ">";
		return w;
	}

	const int size;
};


struct TypeInfo
{
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

	explicit TypeInfo(ComplexType::Ptr p, bool isConst_ = false) :
		typePtr(p),
		const_(isConst_),
		ref_(true)
	{
		jassert(p != nullptr);
		type = Types::ID::Pointer;
	}

	bool isValid() const noexcept
	{
		return !isInvalid();
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

	Types::ID getType() const noexcept
	{
		if (isComplexType())
			return getComplexType()->getDataType();

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
		jassert(isRef());

		return typePtr;
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
};


/** A Symbol is used to identifiy the data slot. */
struct Symbol
{
	static Symbol createRootSymbol(const Identifier& id);

	static Symbol createIndexedSymbol(int index, Types::ID type);

	Symbol();

	Symbol(const Array<Identifier>& ids, Types::ID t_, bool isConst_, bool isRef_);

	Symbol(const Array<Identifier>& ids, const TypeInfo& info);

	bool operator==(Types::ID t) const
	{
		jassertfalse;
		return false;
	}

	bool operator==(const Symbol& other) const;

	bool matchesIdAndType(const Symbol& other) const;

	Symbol getParentSymbol() const;

	Symbol getChildSymbol(const Identifier& id, const TypeInfo& t = {}) const;

	Symbol withParent(const Symbol& parent) const;

	Symbol withType(const Types::ID type) const;

	Symbol withComplexType(ComplexType::Ptr typePtr) const;

	Symbol relocate(const Symbol& newParent) const;

	void setTypeInfo(const TypeInfo& other)
	{
		typeInfo = other;
		debugName = toString().toStdString();
	}

	Array<Identifier> getPath() const { return fullIdList; };

	bool isExplicit() const { return fullIdList.size() > 1; }

	bool isConst() const { return typeInfo.isConst(); }

	bool isParentOf(const Symbol& otherSymbol) const;

	int getNumParents() const { return fullIdList.size() - 1; };

	Types::ID getRegisterType() const
	{
		return typeInfo.isRef() ? Types::Pointer : typeInfo.getType();
	}

	Identifier getId() const { return id; }

	bool isReference() const { return typeInfo.isRef(); };

	juce::String toString() const;

	operator bool() const;

	std::string debugName;
	// a list of identifiers...
	Array<Identifier> fullIdList;
	Identifier id;

	VariableStorage constExprValue = {};

	TypeInfo typeInfo;

#if 0
	bool const_ = false;
	bool ref_ = false;
	Types::ID type = Types::ID::Dynamic;
	ComplexType::Ptr typePtr;
#endif
};


/** A wrapper around a function. */
struct FunctionData
{
	template <typename T> void addArgs(bool omitObjPtr=false)
	{
		if(!omitObjPtr || !std::is_same<T, void*>())
			args.add(Symbol::createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T>()));
	}

	template <typename T1, typename T2> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(Symbol::createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T1>()));

		args.add(Symbol::createIndexedSymbol(1, Types::Helpers::getTypeFromTypeId<T2>()));
	}

	template <typename T1, typename T2, typename T3> void addArgs(bool omitObjPtr = false)
	{
		if (!omitObjPtr || !std::is_same<T1, void*>())
			args.add(Symbol::createIndexedSymbol(0, Types::Helpers::getTypeFromTypeId<T1>()));

		args.add(Symbol::createIndexedSymbol(1, Types::Helpers::getTypeFromTypeId<T2>()));
		args.add(Symbol::createIndexedSymbol(2, Types::Helpers::getTypeFromTypeId<T3>()));
	}

	template <typename ReturnType> static FunctionData createWithoutParameters(const Identifier& id, void* ptr = nullptr)
	{
		FunctionData d;

		d.id = id;
		d.returnType = TypeInfo(Types::Helpers::getTypeFromTypeId<ReturnType>());
		d.function = reinterpret_cast<void*>(ptr);

		return d;
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
				args.getReference(i).id = parameterNames[i];
		}
	}

	juce::String description;

	/** the function ID. */
	Identifier id;

	/** If this is not null, the function will be a member function for the given object. */
	void* object = nullptr;

	/** the function pointer. Use call<ReturnType, Args...>() for type checking during debugging. */
	void* function = nullptr;

	/** The return type. */
	TypeInfo returnType;

	using Argument = Symbol;

#if 0
	struct Argument
	{
		Argument() {};



		Argument(Types::ID type_, bool isAlias_ = false) :
			type(type_),
			isAlias(isAlias_)
		{};

		bool operator==(const Types::ID& t) const
		{
			return t == type;
		}

		bool operator==(const Argument& other) const
		{
			return type == other.type && isAlias == other.isAlias;
		}

		Types::ID type = Types::ID::Dynamic;
		bool isAlias = false;
		String parameterName;
		ComplexType::Ptr typePtr;
	};
#endif

	/** The argument list. */
	Array<Argument> args;

	/** A pretty formatted function name for debugging purposes. */
	String functionName;

	template <typename... Parameters> void callVoid(Parameters... ps) const
	{
		if (function != nullptr)
			callVoidUnchecked(ps...);
	}

	template <typename... Parameters> forcedinline void callVoidUnchecked(Parameters... ps) const
	{
		using signature = void(*)(Parameters...);

		auto f_ = (signature)function;
		f_(ps...);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUnchecked(Parameters... ps) const
	{
		using signature = ReturnType & (*)(Parameters...);
		auto f_ = (signature)function;
		auto& r = f_(ps...);
		return ReturnType(r);
	}

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callUncheckedWithCopy(Parameters... ps) const
	{
		using signature = ReturnType(*)(Parameters...);
		auto f_ = (signature)function;
		return static_cast<ReturnType>(f_(ps...));
	}

	template <typename ReturnType, typename... Parameters> ReturnType call(Parameters... ps) const
	{
		if(object != nullptr)
			return callInternal<ReturnType>(object, ps...);
		else
			return callInternal<ReturnType>(ps...);
	}

private:

	template <typename ReturnType, typename... Parameters> forcedinline ReturnType callInternal(Parameters... ps) const
	{
		// You must not call this method if you return an event or a block.
		// Use callWithReturnCopy instead...

		if (Types::Helpers::getTypeFromTypeId<ReturnType>() == Types::ID::Event ||
			Types::Helpers::getTypeFromTypeId<ReturnType>() == Types::ID::Block)
		{
			if (function != nullptr)
				return callUnchecked<ReturnType, Parameters...>(ps...);
			else
				return ReturnType();
		}
		else
		{
			if (function != nullptr)
				return callUncheckedWithCopy<ReturnType, Parameters...>(ps...);
			else
				return ReturnType();
		}
	}


};









class BaseScope;

struct InlineData;

/** A function class is a collection of functions. */
struct FunctionClass: public DebugableObjectBase,
					  public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<FunctionClass>;

	struct Inliner
	{
		using Func = std::function<Result(InlineData* d)>;

		Inliner(const Identifier& id, const Func& f_) :
			functionId(id),
			f(f_)
		{};

		Result inlineFunction(InlineData* d)
		{
			if (f)
				return f(d);
		}

		const Identifier functionId;
		const Func f;
	};

	struct Constant
	{
		Identifier id;
		VariableStorage value;
	};

	FunctionClass(const Symbol& id) :
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

				if (arg.typeInfo.getType() == Types::ID::Event && r->getObjectName().toString() == "Message")
					continue;

				arguments << Types::Helpers::getTypeName(arg.typeInfo.getType());

				if (arg.id.isValid())
					arguments << " " << arg.id;

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

	juce::String getCategory() const override { return "API call"; };

	Identifier getObjectName() const override { return classSymbol.getId(); }

	juce::String getDebugValue() const override { return classSymbol.toString(); };

	juce::String getDebugDataType() const override { return "Class"; };

	void getAllFunctionNames(Array<Identifier>& functions) const 
	{
		functions.addArray(getFunctionIds());
	};

	void setDescription(const String& s, const StringArray& names)
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

	VariableStorage getConstantValue(const Symbol& s) const;

	// =====================================================================================

	virtual bool hasFunction(const Symbol& s) const;

	bool hasConstant(const Symbol& s) const;

	void addFunctionConstant(const Identifier& constantId, VariableStorage value);

	virtual void addMatchingFunctions(Array<FunctionData>& matches, const Symbol& symbol) const;

	void addFunctionClass(FunctionClass* newRegisteredClass);

	void addFunction(FunctionData* newData);

	Array<Identifier> getFunctionIds() const;

	const Symbol& getClassName() const { return classSymbol; }

	bool fillJitFunctionPointer(FunctionData& dataWithoutPointer);

	bool injectFunctionPointer(FunctionData& dataToInject);

	FunctionClass* getSubFunctionClass(const Symbol& id)
	{
		for (auto f : registeredClasses)
		{
			if (f->getClassName() == id)
				return f;
		}

		return nullptr;
	}

	bool isInlineable(const Identifier& id) const
	{
		for (auto i : inliners)
			if (i->functionId == id)
				return true;

		return false;
	}

	Result inlineFunctionCall(const Identifier& id, InlineData* d)
	{
		jassert(isInlineable(id));

		for (auto i : inliners)
		{
			if (i->functionId == id)
			{
				return i->inlineFunction(d);
			}
		}

		return Result::fail("Can't inline function " + id.toString());
	}

	void addInliner(const Identifier& id, const Inliner::Func& s)
	{
		if (isInlineable(id))
			return;

		inliners.add(new Inliner(id, s));
	}

protected:

	OwnedArray<Inliner> inliners;

	ReferenceCountedArray<FunctionClass> registeredClasses;

	Symbol classSymbol;
	OwnedArray<FunctionData> functions;

	Array<Constant> constants;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionClass);
	JUCE_DECLARE_WEAK_REFERENCEABLE(FunctionClass)
};


struct SpanType : public ComplexType
{
	/** Creates a simple one-dimensional span. */
	SpanType(Types::ID dataType, int size_) :
		elementType(TypeInfo(dataType)),
		size(size_)
	{
		finaliseAlignment();
	}

	/** Creates a SpanType with another ComplexType as element type. */
	SpanType(ComplexType::Ptr childToBeOwned, int size_) :
		elementType(TypeInfo(childToBeOwned)),
		size(size_)
	{
	}

	Types::ID getDataType() const
	{
		return Types::ID::Pointer;
	}

	static bool isSimdType(const TypeInfo& t)
	{
		if(auto st = t.getTypedIfComplexType<SpanType>())
		{
			if (st->getElementType() == Types::ID::Float && st->getNumElements() == 4)
			{
				jassert(st->hasAlias());
				jassert(st->toString() == "float4");
				return true;
			}
		}

		return false;
	}

	void finaliseAlignment() override
	{
		if (elementType.isComplexType())
			elementType.getComplexType()->finaliseAlignment();

		ComplexType::finaliseAlignment();
	}

	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override
	{
		if (elementType.isComplexType())
		{
			// must be aligned already...
			jassert((uint64_t)dataPointer % getRequiredAlignment() == 0);

			for (size_t i = 0; i < size; i++)
			{
				auto mPtr = ComplexType::getPointerWithOffset(dataPointer, getElementSize() * i);

				if (elementType.getComplexType()->forEach(t, typePtr, mPtr))
					return true;
			}
		}

		return false;
	}

	TypeInfo getElementType() const
	{
		return elementType;
	}

	int getNumElements() const
	{
		return size;
	}

	size_t getRequiredByteSize() const override
	{
		return getElementSize() * size;
	}

	size_t getRequiredAlignment() const override
	{
		return elementType.getRequiredAlignment();
	}

	InitialiserList::Ptr makeDefaultInitialiserList() const override
	{
		if (elementType.isComplexType())
		{
			auto c = elementType.getComplexType()->makeDefaultInitialiserList();

			InitialiserList::Ptr n = new InitialiserList();
			n->addChildList(c);
			return n;
		}
		else
			return InitialiserList::makeSingleList(VariableStorage(getElementType().getType(), var(0.0)));
	}

	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override
	{
		intendLevel++;

		for (int i = 0; i < size; i++)
		{
			juce::String symbol;

			auto address = ComplexType::getPointerWithOffset(complexTypeStartPointer, i * getElementSize());

			if (elementType.isComplexType())
			{
				symbol << Types::Helpers::getIntendation(intendLevel);

				if (hasAlias())
					symbol << toString();
				else
					symbol << "Span";

				symbol << "[" << juce::String(i) << "]: \n";

				s << symbol;
				elementType.getComplexType()->dumpTable(s, intendLevel, dataStart, address);
			}
			else
			{
				symbol << "[" << juce::String(i) << "]";

				Types::Helpers::dumpNativeData(s, intendLevel, symbol, dataStart, address, getElementSize(), elementType.getType());
			}
		}

		intendLevel--;
	}

	juce::String toStringInternal() const
	{
		juce::String s = "span<";

		s << elementType.toString();
		s << ", " << size << ">";

		return s;
	}

	Result initialise(void* dataPointer, InitialiserList::Ptr initValues)
	{
		juce::String e;

		if (initValues->size() != size && initValues->size() != 1)
		{

			e << "initialiser list size mismatch. Expected: " << juce::String(size);
			e << ", Actual: " << juce::String(initValues->size());

			return Result::fail(e);
		}

		for (int i = 0; i < size; i++)
		{
			auto indexToUse = initValues->size() == 1 ? 0 : i;

			if (elementType.isComplexType())
			{
				auto childList = initValues->createChildList(indexToUse);

				auto ok = elementType.getComplexType()->initialise(getPointerWithOffset(dataPointer, getElementSize() * i), childList);

				if (!ok.wasOk())
					return ok;
			}
			else
			{
				VariableStorage valueToUse;
				auto ok = initValues->getValue(indexToUse, valueToUse);

				if (!ok.wasOk())
					return ok;

				if (valueToUse.getType() != elementType.getType())
				{
					e << "type mismatch at index " + juce::String(i) << ": " << Types::Helpers::getTypeName(valueToUse.getType());
					return Result::fail(e);
				}

				ComplexType::writeNativeMemberType(dataPointer, getElementSize() * i, valueToUse);
			}
		}

		return Result::ok();
	}

	size_t getElementSize() const
	{
		if (elementType.isComplexType())
		{
			jassert(elementType.getComplexType()->isFinalised());

			auto alignment = elementType.getRequiredAlignment();
			int childSize = elementType.getRequiredByteSize();
			size_t elementPadding = 0;

			if (childSize % alignment != 0)
			{
				elementPadding = alignment - childSize % alignment;
			}

			return childSize + elementPadding;
		}
		else
			return elementType.getRequiredByteSize();
	}

private:


	TypeInfo elementType;

	juce::String typeName;
	int size;
};

struct DynType : public ComplexType
{
	DynType(const TypeInfo& elementType_):
		elementType(elementType_)
	{

	}

	Types::ID getDataType() const override
	{
		return Types::ID::Pointer;
	}

	size_t getRequiredByteSize() const override { return sizeof(VariableStorage); }

	virtual size_t getRequiredAlignment() const override { return 8; }

	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const
	{
		jassertfalse;
	}

	InitialiserList::Ptr makeDefaultInitialiserList() const override
	{
		InitialiserList::Ptr n = new InitialiserList();

		n->addImmediateValue(VariableStorage(nullptr, 0));
		n->addImmediateValue(VariableStorage(0));

		return n;
	}

	Result initialise(void* dataPointer, InitialiserList::Ptr initValues) override
	{
		VariableStorage ptr;
		initValues->getValue(0, ptr);

		jassert(ptr.getType() == Types::ID::Pointer);
		auto data = ptr.getDataPointer();

		VariableStorage s;
		initValues->getValue(1, s);

		memset(dataPointer, 0, 4);

		ComplexType::writeNativeMemberType(dataPointer, 8, ptr);
		ComplexType::writeNativeMemberType(dataPointer, 4, s);

		return Result::ok();
	}

	bool forEach(const TypeFunction&, Ptr, void*) override
	{
		return false;
	}

	juce::String toStringInternal() const override
	{
		juce::String w;
		w << "dyn<" << elementType.toString() << ">";
		return w;
	}

	TypeInfo elementType;
};

struct StructType : public ComplexType
{
	StructType(const Symbol& s) :
		id(s)
	{};

	size_t getRequiredByteSize() const override
	{
		size_t s = 0;

		for (auto m : memberData)
			s += (m->typeInfo.getRequiredByteSize() + m->padding);

		return s;
	}

	virtual juce::String toStringInternal() const override
	{
		return id.toString();
	}

	FunctionClass* getFunctionClass() override
	{
		if (memberFunctions == nullptr)
			memberFunctions = new FunctionClass(id);

		return memberFunctions;
	}

	virtual Result initialise(void* dataPointer, InitialiserList::Ptr initValues) override
	{
		int index = 0;

		for (auto m : memberData)
		{
			if (isPositiveAndBelow(index, initValues->size()))
			{
				auto mPtr = getMemberPointer(m, dataPointer);

				if (m->typeInfo.isComplexType())
				{
					auto childList = initValues->createChildList(index);
					auto ok = m->typeInfo.getComplexType()->initialise(mPtr, childList);

					if (!ok.wasOk())
						return ok;
				}
				else
				{
					VariableStorage childValue;
					initValues->getValue(index, childValue);

					if (m->typeInfo.getType() != childValue.getType())
						return Result::fail("type mismatch at index " + juce::String(index));

					ComplexType::writeNativeMemberType(mPtr, 0, childValue);
				}
			}

			index++;
		}

		return Result::ok();
	}

	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override
	{
		if (typePtr.get() == this)
		{
			return t(this, dataPointer);
		}

		for (auto m : memberData)
		{
			if (m->typeInfo.isComplexType())
			{
				auto mPtr = getMemberPointer(m, dataPointer);

				if (m->typeInfo.getComplexType()->forEach(t, typePtr, mPtr))
					return true;
			}
		}

		return false;
	}

	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override
	{
		size_t offset = 0;
		intendLevel++;

		for (auto m : memberData)
		{
			if (m->typeInfo.isComplexType())
			{
				s << Types::Helpers::getIntendation(intendLevel) << id.toString() << " " << m->id << "\n";
				m->typeInfo.getComplexType()->dumpTable(s, intendLevel, dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id));
			}
			else
			{
				Types::Helpers::dumpNativeData(s, intendLevel, id.getChildSymbol(m->id).toString(), dataStart, (uint8*)complexTypeStartPointer + getMemberOffset(m->id), m->typeInfo.getRequiredByteSize(), m->typeInfo.getType());
			}
		}
		intendLevel--;
	}

	size_t getRequiredAlignment() const override
	{
		if (auto f = memberData.getFirst())
		{
			return f->typeInfo.getRequiredAlignment();
		}

		return 0;
	}

	void finaliseAlignment() override
	{
		if (isFinalised())
			return;

		size_t offset = 0;

		for (auto m : memberData)
		{
			if (m->typeInfo.isComplexType())
				m->typeInfo.getComplexType()->finaliseAlignment();

			m->offset = offset;

			auto alignment = getRequiredAlignment(m);
			m->padding = offset % alignment;
			offset += m->padding + m->typeInfo.getRequiredByteSize();
		}

		jassert(offset == getRequiredByteSize());

		ComplexType::finaliseAlignment();
	}

	bool setDefaultValue(const Identifier& id, InitialiserList::Ptr defaultList)
	{
		jassert(hasMember(id));

		for (auto& m : memberData)
		{
			if (m->id == id)
			{
				m->defaultList = defaultList;
				return true;
			}
		}

		return false;
	}

	bool updateSymbol(Symbol& s) const
	{
		for (auto m : memberData)
		{
			if (m->id == s.id)
			{
				s.typeInfo = m->typeInfo;
				return true;
			}
		}

		return false;
	}

	InitialiserList::Ptr makeDefaultInitialiserList() const override
	{
		InitialiserList::Ptr n = new InitialiserList();

		for (auto m : memberData)
		{
			if (m->typeInfo.isComplexType() && m->defaultList == nullptr)
				n->addChildList(m->typeInfo.getComplexType()->makeDefaultInitialiserList());
			else
				n->addChildList(m->defaultList);
		}

		return n;
	}

	bool hasMember(const Identifier& id) const
	{
		for (auto m : memberData)
			if (m->id == id)
				return true;

		return false;
	}

	TypeInfo getMemberTypeInfo(const Identifier& id) const
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				return m->typeInfo;
		}

		jassertfalse;
		return {};
	}

	Types::ID getMemberDataType(const Identifier& id) const
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				m->typeInfo.getType();
		}
        
        jassertfalse;
        return Types::ID::Void;
	}

	bool isNativeMember(const Identifier& id) const
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				return !m->typeInfo.isComplexType();
		}

		return false;
	}

	ComplexType::Ptr getMemberComplexType(const Identifier& id) const
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				return m->typeInfo.getComplexType();
		}
        
        jassertfalse;
        return nullptr;
	}

	size_t getMemberOffset(const Identifier& id) const
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				return m->padding + m->offset;
		}

		jassertfalse;
		return 0;
	}

	virtual Types::ID getDataType() const override
	{
		return Types::ID::Pointer;
	}

	template <class ObjectType, typename ArgumentType> void addExternalComplexMember(const Identifier& id, ComplexType::Ptr p, ObjectType& obj, ArgumentType& defaultValue)
	{
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = TypeInfo(p);
		nm->offset = reinterpret_cast<uint64>(&defaultValue) - reinterpret_cast<uint64>(&obj);
		nm->defaultList = p->makeDefaultInitialiserList();

		memberData.add(nm);
		isExternalDefinition = true;
	}

	template <class ObjectType, typename ArgumentType> void addExternalMember(const Identifier& id, ObjectType& obj, ArgumentType& defaultValue)
	{
		auto type = Types::Helpers::getTypeFromTypeId<ArgumentType>();
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = TypeInfo(type);
		nm->offset = reinterpret_cast<uint64>(&defaultValue) - reinterpret_cast<uint64>(&obj);
		nm->defaultList = InitialiserList::makeSingleList(VariableStorage(type, var(defaultValue)));

		memberData.add(nm);
		isExternalDefinition = true;
	}

	void addMember(const Identifier& id, const TypeInfo& typeInfo, size_t offset=0)
	{
		jassert(!isFinalised());

		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = typeInfo;
		nm->offset = 0;

		memberData.add(nm);
	}

	Symbol id;

	template <typename ReturnType, typename... Parameters>void addExternalMemberFunction(const Identifier& id, ReturnType(*ptr)(Parameters...))
	{
		FunctionData f = FunctionData::create(id, ptr, true);
		f.id = id;
		f.function = ptr;

		getFunctionClass()->addFunction(new FunctionData(f));
	}

private:

	FunctionClass::Ptr memberFunctions;

	struct Member
	{
		size_t offset = 0;
		size_t padding = 0;
		Identifier id;
		TypeInfo typeInfo;
		InitialiserList::Ptr defaultList;
	};

	static void* getMemberPointer(Member* m, void* dataPointer)
	{
		jassert(dataPointer != nullptr);
		return ComplexType::getPointerWithOffset(dataPointer, m->offset + m->padding);
	}

	static size_t getRequiredAlignment(Member* m)
	{
		return m->typeInfo.getRequiredAlignment();
	}

	OwnedArray<Member> memberData;
	bool isExternalDefinition = false;
};

#define CREATE_SNEX_STRUCT(x) new StructType(Symbol::createRootSymbol(Identifier(#x)));
#define ADD_SNEX_STRUCT_MEMBER(structType, object, member) structType->addExternalMember(#member, object, object.member);
#define ADD_SNEX_STRUCT_COMPLEX(structType, typePtr, object, member) structType->addExternalComplexMember(#member, typePtr, object, object.member);

#define ADD_SNEX_STRUCT_METHOD(structType, obj, name) structType->addExternalMemberFunction(#name, obj::Wrapper::name);

#define ADD_INLINER(x, f) obj->getFunctionClass()->addInliner(#x, [obj](InlineData* d)f);



#define SETUP_INLINER(X) auto& cc = d->gen.cc; auto base = x86::ptr(PTR_REG_R(d->object)); auto type = Types::Helpers::getTypeFromTypeId<X>();

} // end namespace jit
} // end namespace snex

