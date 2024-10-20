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

CharPtr::CharPtr(const char* rawText, size_t limit) noexcept :
    ptr(rawText),
    numCharacters(limit != 0 ? jmin(limit, strlen(ptr)) : strlen(ptr)),
    type((numCharacters == 1 && *rawText == '*') ? Type::Wildcard : Type::RawString)
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
{
	if(numCharacters == 1 && *byteData == '*')
        type = Type::Wildcard;
}

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

HashedPath::HashedPath():
	handler(CharPtr::Type::Wildcard),
	source(CharPtr::Type::Wildcard),
	slot(CharPtr::Type::Wildcard),
	dispatchType(CharPtr::Type::Wildcard)
{}

bool HashedPath::operator==(const HashedPath& otherPath) const
{
	return handler == otherPath.handler &&
		source == otherPath.source &&
		slot == otherPath.slot &&
		dispatchType == otherPath.dispatchType;
}

void HashedPath::parse()
{
	auto start = const_cast<char*>(fullPath.get());
	auto end = start + fullPath.length();
	auto pos = start;
	auto tokenStart = start;

	auto delimiter = '.';

	int tokenIndex = 0;
    
    auto setTokenAndAdvance = [&](HashedCharPtr&& p)
    {
        switch(tokenIndex)
        {
            case 0: std::swap(handler, p); break;
            case 1: source = std::move(p); break;
            case 2: slot = std::move(p); break;
            case 3: dispatchType = std::move(p); break;
        }
        
        tokenIndex++;
    };
    

	if(*pos == delimiter)
		throw Result::fail("expected token");

	while(pos < end && tokenIndex < 4)
	{
		if(*pos == delimiter)
		{
            HashedCharPtr newToken(reinterpret_cast<uint8*>(tokenStart), (pos - tokenStart));
            setTokenAndAdvance(std::move(newToken));
			tokenStart = ++pos;
		}

		while(pos < end && *pos != delimiter)
		{
			if(*pos == '*')
			{
				setTokenAndAdvance(HashedCharPtr(CharPtr::Type::Wildcard));
				++pos;

				if(pos < end && *pos != delimiter)
				{
					throw Result::fail("expected '.'");
				}
                    
				tokenStart = ++pos;

				break;
			}

			pos++;
		}
	}

	if(tokenStart < end && tokenIndex < 4)
	{
        HashedCharPtr newToken(reinterpret_cast<uint8*>(tokenStart), (end - tokenStart));
        setTokenAndAdvance(std::move(newToken));
	}
}

HashedPath::operator String() const noexcept
{
    return fullPath.toString();
}
} // dispatch
} // hise
