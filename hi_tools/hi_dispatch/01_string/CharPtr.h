/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise {
namespace dispatch {	
using namespace juce;


#ifndef HISE_CHAR_PTR_WARNING_LEVEL
#define HISE_CHAR_PTR_WARNING_LEVEL 0
#endif

#if HISE_CHAR_PTR_WARNING_LEVEL == 0
#define CHAR_PTR_FORCED_HASH_ACTION
#elif HISE_CHAR_PTR_WARNING_LEVEL == 1
#define CHAR_PTR_FORCED_HASH_ACTION DBG("Warning: forced hash comparison");
#elif HISE_CHAR_PTR_WARNING_LEVEL >= 2
#define CHAR_PTR_FORCED_HASH_ACTION jassertfalse;
#endif


/** a char pointer to any type of preallocated string data. */
struct CharPtr
{
    enum class Type
    {
        Empty = 0,
        Identifier,
        RawString,
        ByteArray,  // lifetime not guaranteed
        JuceString, // lifetime not guaranteed
        Wildcard = '*',
        numTypes
    };
    
    constexpr CharPtr() noexcept;;
    
    constexpr CharPtr(const CharPtr& other) = default;
    constexpr CharPtr(CharPtr&& other) = default;
    
    /** Creates a CharPtr from a juce::Identifier. This will take the address of the pooled string. */
    explicit CharPtr(const Identifier& id) noexcept;
    
    /** Creates a CharPtr from a raw literal. */
    explicit CharPtr(const char* rawText, size_t numToLimit=0) noexcept;
    
    /** Creates a Wildcard CharPtr. */
    explicit CharPtr(Type t);;
    
    /** Creates a CharPtr from a dynamic byte array. */
    CharPtr(const uint8* byteData, size_t numBytes) noexcept;
    
    /** Creates a CharPtr from a String. */
    explicit CharPtr(const String& s);
    
    explicit operator bool() const noexcept
    {
        return type != Type::Empty && numCharacters > 0 && *ptr != 0;
    }

    explicit operator StringRef () const noexcept;
    
    /** The equality for CharPtrs is defined with these conditions:
        - none of them is an empty pointer
        - one of them is a wildcard
        - they have the same type and point to the same address
     */
    template <typename OtherType> bool operator==(const OtherType& other) const noexcept
    {
        if(type == Type::Empty || other.getType() == Type::Empty)
            return false;
        
        if(isWildcard() || other.isWildcard())
            return true;
        
        if(OtherType::isHashed())
        {
            return sameAddressAndLength(other) ||
                   hash() == other.hash();
        }
        
        if(type != other.getType())
            return false;
        
        return sameAddressAndLength(other);
    }
    
    template <typename OtherType> bool operator!=(const OtherType& other) const noexcept
    {
        return !(*this == other);
        
    }
    
    const char* get() const noexcept { return ptr; }
    size_t length()   const noexcept { return numCharacters; }
    int hash()        const noexcept { return calculateHash(); }
    Type getType()    const noexcept { return type; }
    bool isDynamic()  const noexcept { return type == Type::ByteArray || type == Type::JuceString; }
    bool isWildcard() const noexcept { return type == Type::Wildcard; }
    static constexpr bool isHashed() { return false; }
    
private:
    
    int calculateHash() const noexcept;
    
    template <typename OtherType> bool sameAddressAndLength(const OtherType& other) const noexcept
    {
	    const auto thisAddress = reinterpret_cast<const void*>(get());
	    const auto otherAddress = reinterpret_cast<const void*>(other.get());
        
        return thisAddress == otherAddress &&
               numCharacters == other.length();
    }
    
    const char* ptr;
    size_t numCharacters;
    const Type type;
    static char nothing;
};


/** A extension of the CharPtr subtype with a hashed integer value. */
struct HashedCharPtr
{
public:
    
    using Type = CharPtr::Type;
    
    HashedCharPtr();
    explicit HashedCharPtr(Type);
	HashedCharPtr(const HashedCharPtr& other) = default;
    explicit HashedCharPtr(const CharPtr& cpl_) noexcept;
    explicit HashedCharPtr(uint8* values, size_t numBytes) noexcept;
	HashedCharPtr(const char* rawText) noexcept;
    HashedCharPtr(const Identifier& id) noexcept;
    explicit HashedCharPtr(const String& s) noexcept;
    
    explicit operator bool() const { return static_cast<bool>(cpl); }
    explicit operator StringRef () const noexcept { return StringRef(cpl.get()); }
    
    template <typename OtherType> bool operator==(const OtherType& other) const noexcept
    {
        if(getType() == Type::Empty || other.getType() == Type::Empty)
            return false;
        
        if(isWildcard() || other.isWildcard())
            return true;
        
        return hashed == other.hash();
    }
    
    template <typename OtherType> bool operator!=(const OtherType& other) const noexcept
    {
        return !(*this == other);
    }
    
    const char*  get()      const noexcept { return cpl.get(); }
    size_t length()         const noexcept { return cpl.length(); }
    int hash()              const noexcept { return hashed; };
    CharPtr::Type getType() const noexcept { return cpl.getType(); }
    bool isDynamic()        const noexcept { return cpl.isDynamic(); }
    bool isWildcard()       const noexcept { return cpl.isWildcard(); }
    static constexpr bool isHashed() { return true; }

    String toString() const noexcept { return String::createStringFromData(cpl.get(), cpl.length()); }

private:
    
    CharPtr cpl;
    const int hashed = 0;
};

/** Three hashed pointers that will be used to match the Root::Child objects in the array.
 *
 *  modules.*.attributes
 *  modules.myId.bypassed
 *  "content.[mySlider, mySlider2].value"
 *  content..range
 *  content.*.value
 *
 */
struct HashedPath
{
    using Type = CharPtr::Type;

    HashedPath() = default;

    HashedPath(const HashedCharPtr& sourceManager_, const HashedCharPtr& source_, const HashedCharPtr& slot_):
	  sourceManager(sourceManager_),
	  source(source_),
	  slot(slot_)
    {
        jassert(!sourceManager.isDynamic());
        jassert(!slot.isDynamic());
    }

    bool operator==(const HashedPath& other) const noexcept
    {
	    return sourceManager == other.sourceManager &&
               source == other.source &&
               slot == other.slot;
    }

    bool operator!=(const HashedPath& other) const noexcept
    {
	    return !(*this == other);
    }

    static HashedPath parse(const HashedCharPtr& path);
    
    static constexpr bool isHashed() { return true; }

    /** Only checks whether the source should be dynamic. */
    bool isDynamic() const noexcept { return source.isDynamic(); }
    
    bool isAllWildcard() const noexcept { return sourceManager.isWildcard(); }
    bool isSourceWildcard() const noexcept { return source.isWildcard(); }
    bool isSlotWildcard() const noexcept { return slot.isWildcard(); }
    bool isAnyWildcard() const noexcept { return isAllWildcard() || isSourceWildcard() || isSlotWildcard(); }
    Type getType() const noexcept { return source.getType(); }

private:

    const HashedCharPtr sourceManager;
    const HashedCharPtr source;
    const HashedCharPtr slot;
};


} // dispatch
} // hise