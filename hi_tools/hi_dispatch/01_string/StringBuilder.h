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



struct StringBuilder
{
    static constexpr int SmallBufferSize = 64;

    explicit StringBuilder(size_t numToPreallocate=0);

    StringBuilder(const StringBuilder& other):
      position(other.position)
    {
	    data.setSize(other.position);
        memcpy(data.getObjectPtr(), other.data.getObjectPtr(), position);
    }

    StringBuilder& operator=(const StringBuilder& other)
    {
        position = other.position;
	    data.setSize(other.position);
        memcpy(data.getObjectPtr(), other.data.getObjectPtr(), position);
        return *this;
    }

    StringBuilder& operator==(StringBuilder&& other)
    {
        position = other.position;
	    std::swap(other.data, data);
        return *this;
    }

    StringBuilder& operator<<(const char* rawLiteral);
    StringBuilder& operator<<(const CharPtr& p);
    StringBuilder& operator<<(const HashedCharPtr& p);
    StringBuilder& operator<<(const String& s);
    StringBuilder& operator<<(int number);
    StringBuilder& operator<<(uint8 number);
    StringBuilder& operator<<(size_t number);
    StringBuilder& operator<<(const StringBuilder& other);

#if ENABLE_QUEUE_AND_LOGGER
	StringBuilder& operator<<(const Queue::FlushArgument& f);
#endif

    StringBuilder& operator<<(EventType eventType);

    StringBuilder& operator<<(DispatchType notificationType);

    StringBuilder& appendEventValues(EventType eventType, const uint8* values, size_t numBytes);

    StringBuilder& appendRawByteArray(const uint8* values, size_t numBytes);
    
    CharPtr toCharPtr() const noexcept { return CharPtr(reinterpret_cast<const uint8*>(data.getObjectPtr()), length()); }
    String toString() const noexcept;
    const char* get() const noexcept;
    const char* end()   const noexcept;
    size_t length() const noexcept;

    void clearQuick() { position = 0; }

    void ensureAllocated(size_t num);

private:
    
    char* getWriteHead() const;
    char* getWriteHeadAndAdvance(size_t numToWrite);
    
    ObjectStorage<SmallBufferSize, 0> data;
    size_t position = 0;
};

struct HashedPath
{
    HashedPath();;

    HashedPath(CharPtr::Type t):
      handler(HashedCharPtr(t)),
      source(HashedCharPtr(t)),
      slot(HashedCharPtr(t)),
      dispatchType(HashedCharPtr(t))
    {}

    HashedPath(const HashedCharPtr& fullPath_):
      handler(CharPtr::Type::Wildcard),
      source(CharPtr::Type::Wildcard),
      slot(CharPtr::Type::Wildcard),
      dispatchType(CharPtr::Type::Wildcard)
    {
        fullPath << fullPath_;
        parse();
    }

    bool operator==(const HashedPath& otherPath) const;

    bool operator!=(const HashedPath& otherPath) const
    {
        return !(*this == otherPath);
    }



    explicit operator String() const noexcept;

    bool isWildcard()  const noexcept
    {
        return handler.isWildcard() &&
               source.isWildcard() &&
               slot.isWildcard() &&
               dispatchType.isWildcard();
    }

    static constexpr bool isHashed() { return true; }

    HashedCharPtr handler;
    HashedCharPtr source;
    HashedCharPtr slot;
    HashedCharPtr dispatchType;
    
private:
    
    void parse();
    
    StringBuilder fullPath;
};

} // dispatch
} // hise
