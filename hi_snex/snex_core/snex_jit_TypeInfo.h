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

struct TypeInfo
{
	using List = Array<TypeInfo>;

	TypeInfo();
	TypeInfo(Types::ID type_, bool isConst_ = false, bool isRef_ = false, bool isStatic_ = false);
	explicit TypeInfo(ComplexType::Ptr p, bool isConst_ = false, bool isRef_ = false);
	explicit TypeInfo(const NamespacedIdentifier& templateTypeId_, bool isConst_ = false, bool isRef_ = false);

	bool isDynamic() const noexcept;
	bool isValid() const noexcept;
	bool isTemplateType() const noexcept;
	bool isInvalid() const noexcept;

	bool operator!=(const TypeInfo& other) const;
	bool operator==(const TypeInfo& other) const;
	bool operator==(const Types::ID other) const;
	bool operator!=(const Types::ID other) const;

	size_t getRequiredByteSize() const;
	size_t getRequiredAlignment() const;

	int getRequiredByteSizeNonZero() const;
	int getRequiredAlignmentNonZero() const;

	NamespacedIdentifier getTemplateId() const { return templateTypeId; }
	TypeInfo withModifiers(bool isConst_, bool isRef_, bool isStatic_ = false) const;
	juce::String toString(bool useAlias=true) const;

	juce::String toStringWithoutAlias() const;

	InitialiserList::Ptr makeDefaultInitialiserList() const;
	void setType(Types::ID newType);

	template <class CppType> static TypeInfo fromT()
	{
		auto type = Types::Helpers::getTypeFromTypeId<CppType>();
		
		return TypeInfo(type, std::is_const<CppType>(), std::is_pointer<CppType>::value || std::is_reference<CppType>::value);
	}

	bool isNativePointer() const;
	TypeInfo toNativePointer() const;
	TypeInfo toPointerIfNativeRef() const;
	bool replaceBlockWithDynType(ComplexType::Ptr blockPtr);

	Types::ID getRegisterType(bool allowSmallObjectOptimisation) const noexcept;
	Types::ID getType() const noexcept;

	template <class CType> CType* getTypedComplexType() const
	{
		static_assert(std::is_base_of<ComplexType, CType>(), "Not a base class");

		return dynamic_cast<CType*>(getComplexType().get());
	}

	template <class CType> CType* getTypedIfComplexType() const
	{
		if (isComplexType())
			return dynamic_cast<CType*>(getComplexType().get());

		return nullptr;
	}

	bool isConst() const noexcept;
	bool isRef() const noexcept;
	bool isStatic() const noexcept;
	ComplexType::Ptr getComplexType() const;
	bool isComplexType() const;

	TypeInfo asConst();
	TypeInfo asNonConst();

	void setRefCounted(bool shouldBeRefcounted);

	bool isRefCounted() const;

	static TypeInfo makeNonRefCountedReferenceType(ComplexType* t);

private:

	ComplexType* getRawComplexTypePtr() const;

	void updateDebugName();

#if SNEX_ENABLE_DEBUG_TYPENAMES
	std::string debugName;
#endif

	bool static_ = false;
	bool const_ = false;
	bool ref_ = false;
	Types::ID type = Types::ID::Dynamic;
	ComplexType::Ptr typePtr;
	WeakReference<ComplexType> weakPtr;
	NamespacedIdentifier templateTypeId;
};

struct ExternalTypeParser
{
	ExternalTypeParser(String::CharPointerType location, String::CharPointerType wholeProgram);

	String::CharPointerType getEndOfTypeName();
	Result getResult() const { return parseResult; }
	TypeInfo getType() const { return type; }

private:

	TypeInfo type;
	String::CharPointerType l;
	Result parseResult;
};


/** A Symbol is used to identify the data slot. */
struct Symbol
{
	Symbol(const Identifier& singleId);
	Symbol();
	Symbol(const NamespacedIdentifier& id_, const TypeInfo& t);;

	bool operator==(const Symbol& other) const;
	bool matchesIdAndType(const Symbol& other) const;

	Symbol getParentSymbol(NamespaceHandler* handler) const;
	Symbol getChildSymbol(const Identifier& childName, NamespaceHandler* handler) const;

	Identifier getName() const;

	bool isParentOf(const Symbol& otherSymbol) const;
	bool isConst() const { return typeInfo.isConst(); }
	bool isConstExpr() const { return !constExprValue.isVoid(); }
	Types::ID getRegisterType() const;

	NamespacedIdentifier getId() const { return id; }
	bool isReference() const { return typeInfo.isRef(); };
	juce::String toString(bool allowAlias=true) const;

	operator bool() const;

	NamespacedIdentifier id;
	bool resolved = false;
	VariableStorage constExprValue = {};
	TypeInfo typeInfo;

private:

};

}
}