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



namespace hise {
namespace dispatch {	
using namespace juce;

char CharPtr::nothing = 0;



/** Creates a CharPtr from a juce::Identifier. This will take the address of the pooled string. */

constexpr CharPtr::CharPtr() noexcept :
    ptr(&nothing),
    numCharacters(0),
    type(Type::Empty)
{}

CharPtr::CharPtr(const Identifier& id) noexcept :
    ptr(id.getCharPointer().getAddress()),
    numCharacters(id.toString().length()),
    type(Type::Identifier)
{}

/** Creates a CharPtr from a raw literal. */

CharPtr::CharPtr(const char* rawText) noexcept :
    ptr(rawText),
    numCharacters(strlen(ptr)),
    type(Type::RawString)
{}

CharPtr::CharPtr(Type t) :
    ptr(reinterpret_cast<const char*>(&type)),
    numCharacters(1),
    type(Type::Wildcard)
{
    jassert(t == Type::Wildcard);
}

/** Creates a CharPtr from a dynamic byte array*/

CharPtr::CharPtr(const uint8* byteData, size_t numBytes) noexcept :
    ptr(reinterpret_cast<const char*>(byteData)),
    numCharacters(numBytes),
    type(Type::ByteArray)
{}

CharPtr::CharPtr(const String & s) :
    ptr(s.getCharPointer().getAddress()),
    numCharacters(s.length()),
    type(Type::JuceString)
{}

CharPtr::operator StringRef() const noexcept { return StringRef(ptr); }

int CharPtr::calculateHash() const noexcept
{
    int hash = 0;
    enum { multiplier = sizeof(int) > 4 ? 101 : 31 };
    
    for (int i = 0; i < length(); i++)
        hash = multiplier * hash + static_cast<int>(get()[i]);
    return hash;
}

HashedCharPtr::HashedCharPtr() :
    cpl(),
    hashed(0)
{}

HashedCharPtr::HashedCharPtr(Type) :
    cpl(Type::Wildcard),
    hashed(static_cast<int>(Type::Wildcard))
{}

HashedCharPtr::HashedCharPtr(const CharPtr & cpl_) noexcept :
    cpl(cpl_),
    hashed(cpl.hash())
{}

HashedCharPtr::HashedCharPtr(uint8* values, size_t numBytes) noexcept :
    cpl(values, numBytes),
    hashed(cpl.hash())
{}

HashedCharPtr::HashedCharPtr(const char* rawText) noexcept :
    cpl(rawText),
    hashed(cpl.hash())
{}

HashedCharPtr::HashedCharPtr(const Identifier& id) noexcept:
    cpl(id),
    hashed(cpl.hash())
{}

HashedCharPtr::HashedCharPtr(const String & s) noexcept :
    cpl(s),
    hashed(cpl.hash())
{}

StringBuilder::StringBuilder(size_t numToPreallocate)
{
    if (numToPreallocate != 0)
        data.setSize(numToPreallocate);
}

StringBuilder& StringBuilder::operator<<(const char* rawLiteral)
{
	const auto num = strlen(rawLiteral);
    memcpy(getWriteHeadAndAdvance(num), rawLiteral, num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const CharPtr& p)
{
	const auto num = p.length();
    memcpy(getWriteHeadAndAdvance(num), p.get(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const HashedCharPtr& p)
{
	const auto num = p.length();
    memcpy(getWriteHeadAndAdvance(num), p.get(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const String& s)
{
	const auto num = s.length();
    memcpy(getWriteHeadAndAdvance(num), s.getCharPointer().getAddress(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(int number)
{
    ensureAllocated(8); // the limit is 99 999 999 digits yo...
    const auto ptr = get();
    getWriteHeadAndAdvance(snprintf(ptr, 8, "%d", number));
    return *this;
}

StringBuilder& StringBuilder::operator<<(const StringBuilder& other)
{
	const auto num = other.length();
    memcpy(getWriteHeadAndAdvance(num), other.get(), num);
    return *this;
}

void StringBuilder::ensureAllocated(size_t num)
{
    data.ensureAllocated(position + num, true);
}

char* StringBuilder::getWriteHead() const
{
    return static_cast<char*>(data.getObjectPtr()) + position;
}

char* StringBuilder::getWriteHeadAndAdvance(size_t numToWrite)
{
    ensureAllocated(numToWrite);
    const auto ptr = getWriteHead();
    position += numToWrite;
    *(ptr + position) = 0;
    return ptr;
}



} // dispatch
} // hise