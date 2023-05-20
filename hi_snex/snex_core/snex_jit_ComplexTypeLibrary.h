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


struct ArrayTypeBase : public ComplexType,
					   public ComplexTypeWithTemplateParameters
{
	virtual ~ArrayTypeBase() {}

	virtual TypeInfo getElementType() const = 0;

	
};

struct SpanType : public ArrayTypeBase
{
	/** Creates a simple one-dimensional span. */
	SpanType(const TypeInfo& dataType, int size_);
	~SpanType();

	void finaliseAlignment() override;
	size_t getRequiredByteSize() const override;
	size_t getRequiredAlignment() const override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override;
	juce::String toStringInternal() const override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override;

	Result callDestructor(InitData& data) override;

    ValueTree createDataLayout() const override
    {
        ValueTree d("DataLayout");
        d.setProperty("ID",  toStringInternal(), nullptr);
        d.setProperty("type", "void*", nullptr);
        d.setProperty("size", (int)getRequiredByteSize(), nullptr);
        
        auto numElements = getNumElements();
        
        if(numElements > 16)
            d.setProperty("NumElements", numElements, nullptr);
        else
        {
            auto elementType = getElementType();
            
            auto byteSize = (int)elementType.getRequiredByteSizeNonZero();
            
            ValueTree item("Item");
            item.setProperty("type", elementType.toStringWithoutAlias(), nullptr);
            item.setProperty("size", byteSize, nullptr);
            
            
            if(elementType.isComplexType())
            {
                item.addChild(elementType.getComplexType()->createDataLayout(), -1, nullptr);
            }
            
            for(int i = 0; i < numElements; i++)
            {
                auto idx = item.createCopy();
                idx.setProperty("index", i, nullptr);
                idx.setProperty("offset", i * byteSize, nullptr);
                d.addChild(idx, -1, nullptr);
            }
        }
        
        return d;
        
    }
    
	bool hasDestructor() override
	{
		if (auto typePtr = getElementType().getTypedIfComplexType<ComplexType>())
		{
			return typePtr->hasDestructor();
		}

		return false;
	}

	bool hasConstructor() override
	{
		if (auto typePtr = getElementType().getTypedIfComplexType<ComplexType>())
		{
			return typePtr->hasConstructor();
		}

		return false;
	}

	bool hasDefaultConstructor() override
	{
		if (auto typePtr = getElementType().getTypedIfComplexType<ComplexType>())
		{
			auto ok = typePtr->hasDefaultConstructor();

			// span elements always need a default constructor
			jassert(ok);
			return ok;
		}

		return true;
	}

	bool matchesOtherType(const ComplexType& other) const override
	{
		if (auto otherSpan = dynamic_cast<const SpanType*>(&other))
		{
			if (otherSpan->getElementType() != getElementType())
				return false;

			if (otherSpan->getNumElements() != getNumElements())
				return false;

			return true;
		}

		return false;
	}

	Types::ID getRegisterType(bool allowSmallObjectOptimisation) const override 
	{ 
		return (allowSmallObjectOptimisation && size == 1) ? elementType.getRegisterType(allowSmallObjectOptimisation) : Types::ID::Pointer; 
	};

	TemplateParameter::List getTemplateInstanceParameters() const override
	{
		NamespacedIdentifier sId("span");
		TemplateParameter::List l;

		l.add(TemplateParameter(elementType).withId(sId.getChildId("DataType")));
		l.add(TemplateParameter(getNumElements()).withId(sId.getChildId("NumElements")));

		return l;
	}

	FunctionClass* getFunctionClass() override;
	TypeInfo getElementType() const override;
	int getNumElements() const;
	static bool isSimdType(const TypeInfo& t);

	bool isSimd() const;

	size_t getElementSize() const;

private:

	TypeInfo elementType;
	juce::String typeName;
	int size;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpanType);
};

struct DynType : public ArrayTypeBase
{
	DynType(const TypeInfo& elementType_);

	bool hasDefaultConstructor() override
	{
		return true;
	}

	bool hasDestructor() override
	{
		return false;
	}

    ValueTree createDataLayout() const override
    {
        ValueTree t("DataLayout");
        t.setProperty("ID", toString(), nullptr);
        t.setProperty("type", "void*", nullptr);
        t.setProperty("size", 16, nullptr);
        t.setProperty("ElementSize", elementType.getRequiredByteSizeNonZero(), nullptr);
        
        auto et = TemplateParameter(elementType).withId(NamespacedIdentifier("DataType"));
        
        t.addChild(et.createDataLayout(), -1, nullptr);
        
        ValueTree c2("Member");
        c2.setProperty("ID", "size", nullptr);
        c2.setProperty("type", "int", nullptr);
        c2.setProperty("offset", 4, nullptr);
        c2.setProperty("default", 0, nullptr);
        
        t.addChild(c2, -1, nullptr);
        
        ValueTree c1("Member");
        c1.setProperty("ID", "data", nullptr);
        c1.setProperty("type", "void*", nullptr);
        c1.setProperty("offset", 8, nullptr);
        c1.setProperty("default", 0, nullptr);
        
        t.addChild(c1, -1, nullptr);
        
        FunctionClass::Ptr fc = const_cast<DynType*>(this)->getFunctionClass();
        
        Array<NamespacedIdentifier> functions;
        fc->getAllFunctionNames(functions);
        
        for(auto& s: functions)
        {
            Array<FunctionData> flist;
            
            fc->addMatchingFunctions(flist, s);
            
            for(const auto& f: flist)
            {
                t.addChild(f.createDataLayout(true), -1, nullptr);
            }
        }
        
        return t;
    }
    
	TypeInfo getElementType() const override { return elementType; }

	size_t getRequiredByteSize() const override;
	virtual size_t getRequiredAlignment() const override;
	void dumpTable(juce::String& s, int& intentLevel, void* dataStart, void* complexTypeStartPointer) const;
	FunctionClass* getFunctionClass() override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction&, Ptr, void*) override { return false; }
	juce::String toStringInternal() const override;

	bool matchesOtherType(const ComplexType& other) const override
	{
		if (auto otherSpan = dynamic_cast<const DynType*>(&other))
		{
			return otherSpan->getElementType() == getElementType();
		}

		return false;
	}

	TemplateParameter::List getTemplateInstanceParameters() const override
	{
		TemplateParameter::List l;
		auto dId = NamespacedIdentifier("dyn");
		l.add(TemplateParameter(elementType).withId(dId.getChildId("DataType")));
		return l;
	}

	TypeInfo elementType;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynType);
};

struct StructType : public ComplexType,
					public ComplexTypeWithTemplateParameters
{
	StructType(const NamespacedIdentifier& s, const Array<TemplateParameter>& templateParameters = {});;
	~StructType();

	size_t getRequiredByteSize() const override;
	size_t getRequiredAlignment() const override;

	Types::ID getRegisterType(bool allowSmallObjectOptimisation) const override
	{
		if (allowSmallObjectOptimisation && memberData.size() == 1)
			return memberData.getFirst()->typeInfo.getRegisterType(allowSmallObjectOptimisation);

		return Types::ID::Pointer;
	}

	bool hasDestructor() override;

	Result callDestructor(InitData& d) override;

	bool hasConstructor() override;

	bool hasDefaultConstructor() override;

	Identifier getConstructorId();

	void finaliseAlignment() override;
	juce::String toStringInternal() const override;
	FunctionClass* getFunctionClass() override;
	Result initialise(InitData data) override;
	bool forEach(const TypeFunction& t, ComplexType::Ptr typePtr, void* dataPointer) override;
	void dumpTable(juce::String& s, int& intendLevel, void* dataStart, void* complexTypeStartPointer) const override;
	InitialiserList::Ptr makeDefaultInitialiserList() const override;

	/** If this struct type has not a default constructor, it will create one. */
	bool createDefaultConstructor();

	bool matchesOtherType(const ComplexType& other) const override
	{
		if (auto st = dynamic_cast<const StructType*>(&other))
		{
			if (id == st->id)
				return TemplateParameter::ListOps::match(templateParameters, st->templateParameters);
				
			return false;

		}

		return false;
	}

    ValueTree createDataLayout() const override
    {
        ValueTree t("DataLayout");
        t.setProperty("ID", toStringInternal(), nullptr);
        t.setProperty("NumBytes", (int)getRequiredByteSize(), nullptr);
        
        for(const auto& tp: templateParameters)
        {
            t.addChild(tp.createDataLayout(), -1, nullptr);
        }
        
        for(auto m: memberData)
        {
            t.addChild(m->createDataLayout(), -1, nullptr);
        }
        
        for(const auto& m: memberFunctions)
        {
            t.addChild(m.createDataLayout(true), -1, nullptr);
        }
        
        return t;
    }
    
	void registerExternalAtNamespaceHandler(NamespaceHandler* handler, const String& description) override;

	bool setDefaultValue(const Identifier& id, InitialiserList::Ptr defaultList);

	bool setTypeForDynamicReturnFunction(FunctionData& functionDataWithReturnType);

	bool hasMemberAtOffset(int offset, const TypeInfo& type) const;

	ComplexType::Ptr createSubType(SubTypeConstructData* sd) override;

	bool hasMember(int index) const;
	bool hasMember(const Identifier& id) const;
	TypeInfo getMemberTypeInfo(const Identifier& id) const;
	Types::ID getMemberDataType(const Identifier& id) const;
	bool isNativeMember(const Identifier& id) const;
	ComplexType::Ptr getMemberComplexType(const Identifier& id) const;
	size_t getMemberOffset(const Identifier& id) const;

	size_t getMemberOffset(int index) const; 

	int getNumMembers() const { return memberData.size(); };

	NamespaceHandler::Visibility getMemberVisibility(const Identifier& id) const;

	Identifier getMemberName(int index) const;

	void addJitCompiledMemberFunction(const FunctionData& f);

	bool injectInliner(const FunctionData& f);

	MemoryBlock createByteBlock() const;

	int injectInliner(const Identifier& functionId, Inliner::InlineType type, const Inliner::Func& func, const TemplateParameter::List& functionTemplateParameters = {});

	Symbol getMemberSymbol(const Identifier& id) const;

	TemplateInstance getTemplateInstanceId() const;

	bool injectMemberFunctionPointer(const FunctionData& f, void* fPointer);

	void finaliseExternalDefinition()
	{
		isExternalDefinition = true;

		for (auto m : memberData)
		{
			if (m->typeInfo.isComplexType())
			{
				if (!m->typeInfo.getComplexType()->isFinalised())
					return;
			}
		}

		finaliseAlignment();
	}

	void addWrappedMemberMethod(const Identifier& memberId, FunctionData wrapperFunction);

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

	template <class ObjectType, typename ArgumentType> void addExternalMember(const Identifier& id, ObjectType& obj, ArgumentType& defaultValue, NamespaceHandler::Visibility v = NamespaceHandler::Visibility::Public)
	{
		auto type = Types::Helpers::getTypeFromTypeId<ArgumentType>();
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = TypeInfo(type, type == Types::ID::Pointer);
		nm->offset = reinterpret_cast<uint64>(&defaultValue) - reinterpret_cast<uint64>(&obj);

		if constexpr (!std::is_same<ArgumentType, void*>())
			nm->defaultList = InitialiserList::makeSingleList(VariableStorage(type, var(defaultValue)));
		else
			nm->defaultList = InitialiserList::makeSingleList(VariableStorage(nullptr));

		nm->visibility = v;

		memberData.add(nm);
		isExternalDefinition = true;
	}

	void setVisibility(const Identifier& id, NamespaceHandler::Visibility v)
	{
		for (auto m : memberData)
		{
			if (m->id == id)
				m->visibility = v;
		}
	}

	void addMember(const Identifier& id, const TypeInfo& typeInfo, const String& comment = {})
	{
		jassert(!isFinalised());

		TypeInfo toUse = typeInfo;

		if (toUse.isTemplateType())
		{
			for (int i = 0; i < templateParameters.size(); i++)
			{
				if (templateParameters[i].matchesTemplateType(typeInfo))
				{
					toUse = templateParameters[i].type;
					break;
				}
			}
		}
		
		auto nm = new Member();
		nm->id = id;
		nm->typeInfo = toUse;
		nm->offset = 0;
		nm->comment = comment;
		nm->visibility = NamespaceHandler::Visibility::Public;
		memberData.add(nm);
	}

	void addExternalMemberFunction(const FunctionData& data)
	{
		jassert(data.function != nullptr);
		memberFunctions.add(data);
	}

	void setExternalMemberParameterNames(const StringArray& names)
	{
		jassert(memberFunctions.size() != 0);

		auto& f = memberFunctions.getReference(memberFunctions.size() - 1);

		jassert(names.size() == f.args.size());

		for (int i = 0; i < f.args.size(); i++)
			f.args.getReference(i).id = NamespacedIdentifier(names[i]);
	}

	template <typename ReturnType, typename... Parameters>void addExternalMemberFunction(const Identifier& id, ReturnType(*ptr)(Parameters...))
	{
		FunctionData f = FunctionData::create(id, ptr, true);
		f.function = reinterpret_cast<void*>(ptr);

		memberFunctions.add(f);
	}

	TemplateParameter::List getTemplateInstanceParametersForFunction(NamespacedIdentifier& id);

	TemplateParameter::List getTemplateInstanceParameters() const 
	{
		return templateParameters;
	}

	// Returns the member index of the base class if the function matches a base class method.
	int getBaseClassIndexForMethod(const FunctionData& f) const;

	/** Returns a list of special functions for all base classes. */
	Array<FunctionData> getBaseSpecialFunctions(FunctionClass::SpecialSymbols s, TypeInfo returnType = {}, const Array<TypeInfo>& args = {});

	void findMatchesFromBaseClasses(Array<FunctionData>& possibleMatches, const NamespacedIdentifier& idWithDerivedParent, int& baseOffset, ComplexType::Ptr& baseClass);

	NamespacedIdentifier id;

	/** Use this in order to overwrite the actual member structure. 
	
		You can use it to create an opaque data structure from an existing C++ class. 	
	*/
	template <typename T> void setSizeFromObject(const T& obj)
	{
		externalyDefinedSize = sizeof(T);

		// Setup a constructor before this;
		jassert(hasConstructor());

		memberData.clear();
	}

	void setCustomDumpFunction(const std::function<String(void*)>& f)
	{
		customDumpFunction = f;
	}

	void setInternalProperty(const Identifier& propId, const var& newValue)
	{
		internalProperties.set(propId, newValue);
	}

	var getInternalProperty(const Identifier& propId, const var& defaultValue) override
	{
		return internalProperties.getWithDefault(propId, defaultValue);
	}

	bool hasInternalProperty(const Identifier& propId) const
	{
		return internalProperties.getVarPointer(propId) != nullptr;
	}

	/** Call this if you want to redirect all methods with the same id to the one
	    with the supplied argument list.
	*/
	Result redirectAllOverloadedMembers(const Identifier& id, TypeInfo::List mainArgs);

	
	void addBaseClass(StructType* b);

	/** Checks whether the given id can be a member. It also checks all base classes. */
	bool canBeMember(const NamespacedIdentifier& possibleMemberId) const;

	void setCompiler(Compiler& c)
	{
		registeredCompiler = &c;
	}

	Compiler* getCompiler() { return registeredCompiler.get(); }

private:

	NamedValueSet internalProperties;
	std::function<String(void*)> customDumpFunction;
	size_t externalyDefinedSize = 0;
	TemplateParameter::List templateParameters;
	Array<FunctionData> memberFunctions;

	struct Member
	{
        ValueTree createDataLayout() const
        {
            ValueTree c1("Member");
            c1.setProperty("ID", id.toString(), nullptr);
            
            auto t = Types::Helpers::getCppTypeName(typeInfo.getType());
                           
            if(t == "pointer") t = "void*";
            
            c1.setProperty("type", t, nullptr);
            c1.setProperty("offset", (int)(offset + padding), nullptr);
            c1.setProperty("size", (int)typeInfo.getRequiredByteSizeNonZero(), nullptr);
            c1.setProperty("default", defaultList != nullptr ? defaultList->toString() : "", nullptr);
            
            if(typeInfo.isComplexType())
            {
                c1.addChild(typeInfo.getComplexType()->createDataLayout(), -1, nullptr);
            }
            
            return c1;
        }
        
		String comment;
		size_t offset = 0;
		size_t padding = 0;
		Identifier id;
		TypeInfo typeInfo;
		NamespaceHandler::Visibility visibility = NamespaceHandler::Visibility::numVisibilities;
		InitialiserList::Ptr defaultList;
	};

	struct BaseClass
	{
		BaseClass(StructType* s) :
			baseClass(s),
			strongType(s)
		{};

		Member** begin() const
		{
			return baseClass->memberData.begin();
		}

		Member** end() const
		{
			return baseClass->memberData.end();
		}


		// the index of the first member
		int memberOffset = -1;

		
		WeakReference<StructType> baseClass;

	private:

		// Just keep a ref-counted reference around or the destructor might be dangling...
		ComplexType::Ptr strongType;
	};

	static void* getMemberPointer(Member* m, void* dataPointer);

	static size_t getRequiredAlignment(Member* m);

	OwnedArray<Member> memberData;
	OwnedArray<BaseClass> baseClasses;

	bool isExternalDefinition = false;

	WeakReference<Compiler> registeredCompiler = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(StructType);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StructType);
};

#define CREATE_SNEX_STRUCT(x) new StructType(NamespacedIdentifier(#x));
#define ADD_SNEX_STRUCT_MEMBER(structType, object, member) structType->addExternalMember(#member, object, object.member);
#define ADD_SNEX_PRIVATE_MEMBER(structType, object, member) structType->addExternalMember(#member, object, object.member, NamespaceHandler::Visibility::Private);
#define ADD_SNEX_STRUCT_COMPLEX(structType, typePtr, object, member) structType->addExternalComplexMember(#member, typePtr, object, object.member);

#define ADD_SNEX_STRUCT_METHOD(structType, obj, name) structType->addExternalMemberFunction(#name, obj::Wrapper::name);

#define SET_SNEX_PARAMETER_IDS(obj, ...) obj->setExternalMemberParameterNames({ __VA_ARGS__ });

#define ADD_INLINER(x, f) fc->addInliner(#x, [obj](InlineData* d_)f);

#define SETUP_INLINER(X) auto d = d_->toAsmInlineData(); \
						 auto& cc = d->gen.cc; \
						 auto base = x86::ptr(PTR_REG_R(d->object)); \
					     auto type = Types::Helpers::getTypeFromTypeId<X>(); \
						 ignoreUnused(type, base, cc, d);

}
}
