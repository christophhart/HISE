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


namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


bool TypeInfo::operator==(const Types::ID other) const
{
	return getType() == other;
}

bool TypeInfo::operator==(const TypeInfo& other) const
{
	if (isComplexType())
	{
		if (other.isComplexType())
			return *getRawComplexTypePtr() == *other.getRawComplexTypePtr();

		return false;
	}

	return getType() == other.type;
}

bool TypeInfo::operator!=(const TypeInfo& other) const
{
	return !(*this == other);
}

bool TypeInfo::operator!=(const Types::ID other) const
{
	return getType() != other;
}

TypeInfo::TypeInfo(const NamespacedIdentifier& templateTypeId_, bool isConst_ /*= false*/, bool isRef_ /*= false*/) :
	templateTypeId(templateTypeId_),
	const_(isConst_),
	ref_(isRef_),
	static_(false)
{
	updateDebugName();
}

TypeInfo::TypeInfo(ComplexType::Ptr p, bool isConst_ /*= false*/, bool isRef_ /*= false*/) :
	typePtr(p),
	const_(isConst_),
	ref_(isRef_),
	static_(false)
{
	jassert(p != nullptr);
	type = Types::ID::Pointer;
	updateDebugName();
}

TypeInfo::TypeInfo(Types::ID type_, bool isConst_ /*= false*/, bool isRef_ /*= false*/, bool isStatic_ /*= false*/) :
	type(type_),
	const_(isConst_),
	ref_(isRef_),
	static_(isStatic_)
{
	jassert(!(type == Types::ID::Void && isRef()));

	
	updateDebugName();
}

TypeInfo::TypeInfo() :
	type(Types::ID::Dynamic)
{
	updateDebugName();
}

bool TypeInfo::isDynamic() const noexcept
{
	return !isTemplateType() && type == Types::ID::Dynamic;
}

bool TypeInfo::isValid() const noexcept
{
	return !isInvalid();
}

bool TypeInfo::isTemplateType() const noexcept
{
	return templateTypeId.isValid();
}

bool TypeInfo::isInvalid() const noexcept
{
	return type == Types::ID::Void || type == Types::ID::Dynamic;
}

size_t TypeInfo::getRequiredByteSize() const
{
	if (isComplexType())
		return getRawComplexTypePtr()->getRequiredByteSize();
	else
		return Types::Helpers::getSizeForType(type);
}

size_t TypeInfo::getRequiredAlignment() const
{
	if (isComplexType())
		return getRawComplexTypePtr()->getRequiredAlignment();
	else
		return Types::Helpers::getSizeForType(type);
}

snex::jit::TypeInfo TypeInfo::withModifiers(bool isConst_, bool isRef_, bool isStatic_ /*= false*/) const
{
	auto c = *this;
	c.const_ = isConst_;
	c.ref_ = isRef_;
	c.static_ = isStatic_;

	c.updateDebugName();
	return c;
}

juce::String TypeInfo::toString(bool useAlias) const
{
	juce::String s;

	if (isStatic())
		s << "static ";

	if (isConst())
		s << "const ";

	if (isComplexType())
	{
		if (useAlias)
			s << getRawComplexTypePtr()->toString();
		else
			s << getRawComplexTypePtr()->getActualTypeString();

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

juce::String TypeInfo::toStringWithoutAlias() const
{
	return toString(false);
}

snex::InitialiserList::Ptr TypeInfo::makeDefaultInitialiserList() const
{
	if (isComplexType())
		return getRawComplexTypePtr()->makeDefaultInitialiserList();
	else
	{
		jassert(getType() != Types::ID::Pointer);
		InitialiserList::Ptr p = new InitialiserList();
		p->addImmediateValue(VariableStorage(getType(), 0.0));

		return p;
	}
}

void TypeInfo::setType(Types::ID newType)
{
	jassert(newType != Types::ID::Pointer);
	type = newType;
	updateDebugName();
}

bool TypeInfo::isNativePointer() const
{
	return !isComplexType() && type == Types::ID::Pointer;
}

snex::jit::TypeInfo TypeInfo::toNativePointer() const
{
	if (isComplexType())
		return TypeInfo(Types::ID::Pointer, true);

	return *this;
}

snex::jit::TypeInfo TypeInfo::toPointerIfNativeRef() const
{
	if (!isComplexType() && isRef())
		return TypeInfo(Types::ID::Pointer, true);

	return *this;
}

bool TypeInfo::replaceBlockWithDynType(ComplexType::Ptr blockPtr)
{
	if (getType() == Types::ID::Block)
	{
		auto copy = TypeInfo(blockPtr).withModifiers(isConst(), isRef(), isStatic());
		*this = copy;
		return true;
	}

	return false;
}

snex::Types::ID TypeInfo::getRegisterType(bool allowSmallObjectOptimisation) const noexcept
{
	if (isComplexType())
		return getRawComplexTypePtr()->getRegisterType(allowSmallObjectOptimisation);

	return getType();
}

snex::Types::ID TypeInfo::getType() const noexcept
{
	if (isComplexType())
		return Types::ID::Pointer;

	return type;
}

bool TypeInfo::isConst() const noexcept
{
	return const_;
}

bool TypeInfo::isRef() const noexcept
{
	return ref_;
}

bool TypeInfo::isStatic() const noexcept
{
	if (static_)
	{
		jassert(!isComplexType());
		jassert(!isRef());
	}

	return static_;
}

snex::jit::ComplexType::Ptr TypeInfo::getComplexType() const
{
	jassert(type == Types::ID::Pointer);
	jassert(typePtr != nullptr || weakPtr != nullptr);
	jassert(!(typePtr != nullptr && weakPtr != nullptr));

	if (typePtr != nullptr)
		return typePtr;
	else
		return weakPtr.get();
}

bool TypeInfo::isComplexType() const
{
	return typePtr != nullptr || weakPtr != nullptr;
}

int TypeInfo::getRequiredByteSizeNonZero() const
{
	auto s = getRequiredByteSize();

	if (s == 0)
		return 1;

	return (int)s;
}

int TypeInfo::getRequiredAlignmentNonZero() const
{
	auto s = getRequiredAlignment();

	// Make sure that the movaps calls are working
	if (getRequiredByteSize() >= 16)
		return 16;

	if (s == 0)
		return 1;

	return (int)s;
}

snex::jit::TypeInfo TypeInfo::asConst()
{
	auto t = *this;
	t.const_ = true;
	t.updateDebugName();
	return t;
}

snex::jit::TypeInfo TypeInfo::asNonConst()
{
	auto t = *this;
	t.const_ = false;
	t.updateDebugName();
	return t;
}

void TypeInfo::setRefCounted(bool shouldBeRefcounted)
{
	if (!isComplexType())
		return;

	if (shouldBeRefcounted)
	{
		if (weakPtr != nullptr)
		{
			typePtr = weakPtr.get();
			weakPtr = nullptr;
		}
	}
	else
	{
		if (typePtr != nullptr)
		{
			// You're going to delete it with this operation!
			// Use makeNonRefCountedReference() instead!
			jassert(typePtr->getReferenceCount() > 1);

			weakPtr = typePtr.get();
			typePtr = nullptr;
		}
	}
}

bool TypeInfo::isRefCounted() const
{
	return isComplexType() && weakPtr == nullptr;
}

TypeInfo TypeInfo::makeNonRefCountedReferenceType(ComplexType* ptr)
{
	TypeInfo t;

	t.type = Types::ID::Pointer;
	t.weakPtr = ptr;
	t.ref_ = true;
	t.const_ = false;
	t.static_ = false;

	t.updateDebugName();

	return t;
}

snex::jit::ComplexType* TypeInfo::getRawComplexTypePtr() const
{
	jassert(isComplexType());

	if (typePtr != nullptr)
		return typePtr.get();
	else
		return weakPtr.get();
}

void TypeInfo::updateDebugName()
{
	SNEX_TYPEDEBUG(debugName = toString().toStdString());

	jassert(!(type == Types::ID::Void && isRef()));
}

snex::jit::String::CharPointerType ExternalTypeParser::getEndOfTypeName()
{
	return l;
}

Symbol::Symbol(const Identifier& singleId) :
	id(NamespacedIdentifier(singleId)),
	resolved(false),
	typeInfo({})
{

}

Symbol::Symbol() :
	id(NamespacedIdentifier::null()),
	resolved(false),
	typeInfo({})
{

}

Symbol::Symbol(const NamespacedIdentifier& id_, const TypeInfo& t) :
	id(id_),
	typeInfo(t),
	resolved(!t.isDynamic())
{

}

bool Symbol::operator==(const Symbol& other) const
{
	return id == other.id;
}


snex::jit::Symbol Symbol::getParentSymbol(NamespaceHandler* handler) const
{
	auto p = id.getParent();

	if (p.isValid())
	{
		auto t = handler->getVariableType(id);
		return Symbol(p, t);
	}
	else
		return Symbol(Identifier());
}

snex::jit::Symbol Symbol::getChildSymbol(const Identifier& childName, NamespaceHandler* handler) const
{
	auto cId = id.getChildId(childName);
	auto t = handler->getVariableType(cId);
	return Symbol(cId, t);
}

Symbol::operator bool() const
{
	return !id.isNull() && id.isValid();
}



bool Symbol::matchesIdAndType(const Symbol& other) const
{
	return id == other.id && typeInfo == other.typeInfo;
}

juce::Identifier Symbol::getName() const
{
	//jassert(!resolved);
	return id.getIdentifier();
}

bool Symbol::isParentOf(const Symbol& otherSymbol) const
{
	return otherSymbol.id.getParent() == id;
}

snex::Types::ID Symbol::getRegisterType() const
{
	return typeInfo.isRef() ? Types::Pointer : typeInfo.getType();
}

juce::String Symbol::toString(bool allowAlias) const
{
	juce::String s;

	if (resolved)
	{
		if (allowAlias)
			s << typeInfo.toString();
		else
			s << typeInfo.toStringWithoutAlias();
		
		s << " ";
	}
		
	else
		s << "unresolved ";

	s << id.toString();

	return s;
}

}
}
