// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include <cstring>

#include "../../inc/Temporary/Allocator.h"
#include "../../inc/Temporary/Assert.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // Allocator
    //
    //*************************************************************************
    Allocator::Allocator(size_t bufferSize)
        : m_bufferSize(bufferSize),
          m_bytesAllocated(0),
          m_buffer(new char[bufferSize])
    {
        DebugInitialize();
    }


    Allocator::~Allocator()
    {
    }


    void* Allocator::Allocate(size_t size)
    {
        LogThrowAssert(m_bytesAllocated + size <= m_bufferSize, "Out of memory");

        void* result = static_cast<void*>(m_buffer.get() + m_bytesAllocated);
        m_bytesAllocated += size;

        return result;
    }


    void Allocator::Deallocate(void* block)
    {
        LogThrowAssert(static_cast<char*>(block) >= m_buffer.get() &&
                       static_cast<char*>(block) < m_buffer.get() + m_bytesAllocated,
                       "Attempting to deallocate memory not owned by this allocator.");

        // Intentional NOP
    }


    size_t Allocator::MaxSize() const
    {
        return m_bufferSize;
    }


    void Allocator::Reset()
    {
        m_bytesAllocated = 0;
        DebugInitialize();
    }


    void Allocator::DebugInitialize()
    {
        memset(m_buffer.get(), 0xcc, m_bufferSize);
    }
}
