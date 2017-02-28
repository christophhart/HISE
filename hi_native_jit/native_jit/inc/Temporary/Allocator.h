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


#pragma once

#include <memory>

#include "IAllocator.h"
#include "NonCopyable.h"


namespace NativeJIT
{
    // TODO: This should be a private header.
    class Allocator : public Allocators::IAllocator
    {
    public:
        Allocator(size_t bufferSize);

        virtual ~Allocator() override;

        // Allocates a block of a specified byte size.
        virtual void* Allocate(size_t size) override;

        // Frees a block.
        virtual void Deallocate(void* block) override;

        // Returns the maximum legal allocation size in bytes.
        virtual size_t MaxSize() const override;

        // Frees all blocks that have been allocated since construction or the
        // last call to Reset().
        virtual void Reset() override;

    private:
        void DebugInitialize();

        size_t m_bufferSize;
        size_t m_bytesAllocated;
        std::unique_ptr<char[]> m_buffer;
    };
}
