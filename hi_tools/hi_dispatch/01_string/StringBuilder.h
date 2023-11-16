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
    
    StringBuilder& operator<<(const char* rawLiteral);
    StringBuilder& operator<<(const CharPtr& p);
    StringBuilder& operator<<(const HashedCharPtr& p);
    StringBuilder& operator<<(const String& s);
    StringBuilder& operator<<(int number);
    StringBuilder& operator<<(const StringBuilder& other);
	StringBuilder& operator<<(const Queue::FlushArgument& f);
    StringBuilder& operator<<(EventType eventType);

    StringBuilder& operator<<(DispatchType notificationType);

    StringBuilder& appendEventValues(EventType eventType, const uint8* values, size_t numBytes);

    StringBuilder& appendRawByteArray(const uint8* values, size_t numBytes);

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
    int position = 0;
};

} // dispatch
} // hise