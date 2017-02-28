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


#include <stdexcept>

#ifdef NATIVEJIT_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "../../inc/NativeJIT/CodeGen/ExecutionBuffer.h"
#include "../../inc/Temporary/Assert.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // ExecutionBuffer
    //
    //*************************************************************************

    // Rounds x up to the nearest size, where size is a power of 2.
    size_t RoundUp(size_t  x, size_t powerOfTwo)
    {
        return (x + powerOfTwo - 1) & (~(powerOfTwo - 1));
    }


    // http://stackoverflow.com/questions/570257/jit-compilation-and-dep
#ifdef NATIVEJIT_PLATFORM_WINDOWS
    ExecutionBuffer::ExecutionBuffer(size_t bufferSize)
        : m_buffer(nullptr),
          m_bytesAllocated(0)
    {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);

        m_bufferSize = RoundUp(bufferSize, systemInfo.dwPageSize);

        // Allocate m_bufferSize bytes plus one extra page that will act as
        // a write-guard to detect buffer overruns.
        m_buffer = (unsigned char*)VirtualAlloc(NULL, m_bufferSize + systemInfo.dwPageSize,
                                                MEM_COMMIT, PAGE_EXECUTE_READWRITE);

        if (m_buffer == NULL)
        {
            throw std::runtime_error("CodeBuffer: out of memory.");
        }

        // Set protection on the guard page.
        DWORD oldProtection;
        if (!VirtualProtect(m_buffer + m_bufferSize, systemInfo.dwPageSize,
                            PAGE_NOACCESS, &oldProtection))
        {
            // TODO: Fix memory leaks by f. ex. using unique_ptr with custom deleter
            // for m_buffer. See bug#13
            throw std::runtime_error("CodeBuffer: failed to set protection on guard page.");
        }

        DebugInitialize();
    }
#else
    ExecutionBuffer::ExecutionBuffer(size_t bufferSize)
        : m_bytesAllocated(0),
          m_buffer(nullptr)
    {
        m_bufferSize = RoundUp(bufferSize, getpagesize());
        m_buffer = static_cast<unsigned char*>(
                       mmap(nullptr,
                            m_bufferSize,
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_PRIVATE | MAP_ANON,
                            -1,
                            0));
        if (m_buffer == MAP_FAILED) {
            // TODO: Fix memory leaks by f. ex. using unique_ptr with custom deleter
            // for m_buffer. See bug#13
            throw std::runtime_error("CodeBuffer: failed to set protection on guard page.");
        }

    }
#endif


#ifdef NATIVEJIT_PLATFORM_WINDOWS
    ExecutionBuffer::~ExecutionBuffer()
    {
        if (m_buffer != nullptr)
        {
            if (VirtualFree(m_buffer, 0, MEM_RELEASE) == 0)
            {
                // TODO: Fix this See bug#13
                // throw std::runtime_error("CodeBuffer: VirtualFree failed.");
            }
        }
    }
#else
    ExecutionBuffer::~ExecutionBuffer()
    {
        if (m_buffer != nullptr)
        {
            if (munmap(m_buffer, m_bufferSize) != 0)
            {
                // TODO: Fix this. See bug#13
                // throw std::runtime_error("CodeBuffer: munmap failed.");
            }
        }
    }

#endif


    //
    // IAllocator methods
    //

    // Allocates a block of a specified byte size.
    void* ExecutionBuffer::Allocate(size_t size)
    {
        LogThrowAssert(m_bytesAllocated + size <= m_bufferSize, "Out of memory");

        void* result = static_cast<void*>(m_buffer + m_bytesAllocated);
        m_bytesAllocated += size;

        return result;
    }


    // Frees a block.
    void ExecutionBuffer::Deallocate(void* /*block*/)
    {
        // Intentional NOP
    }


    // Returns the maximum legal allocation size in bytes.
    size_t ExecutionBuffer::MaxSize() const
    {
        return m_bufferSize;
    }


    // Frees all blocks that have been allocated since construction or the
    // last call to Reset().
    void ExecutionBuffer::Reset()
    {
        m_bytesAllocated = 0;
        DebugInitialize();
    }


    void ExecutionBuffer::DebugInitialize()
    {
#ifdef _DEBUG
        // Fill the buffer with break code (i.e. INT 3 software breakpoint).
        memset(m_buffer, 0xcc, m_bufferSize);
#endif
    }
}
