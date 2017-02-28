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
#include <stdexcept>

#include "../../inc/NativeJIT/CodeGen/CodeBuffer.h"
#include "../../inc/NativeJIT/CodeGen/JumpTable.h"
#include "../../inc/Temporary/Assert.h"
#include "../../inc/Temporary/IAllocator.h"

namespace NativeJIT
{
    CodeBuffer::CodeBuffer(Allocators::IAllocator& codeAllocator, unsigned capacity)
        : m_codeAllocator(codeAllocator),
          m_capacity(capacity),
          m_bufferStart(nullptr)
    {
        m_bufferStart = static_cast<uint8_t*>(codeAllocator.Allocate(capacity));
        m_bufferEnd = m_bufferStart + capacity;
        m_current = m_bufferStart;
    }


    CodeBuffer::~CodeBuffer()
    {
        m_codeAllocator.Deallocate(m_bufferStart);
    }


    Label CodeBuffer::AllocateLabel()
    {
        return m_localJumpTable.AllocateLabel();
    }


    void CodeBuffer::PlaceLabel(Label label)
    {
        m_localJumpTable.PlaceLabel(label, m_current);
    }


    void CodeBuffer::EmitBytes(uint8_t const *data, unsigned length)
    {
        VerifyNoBufferOverflow(length);

        memcpy(m_current, data, length);
        m_current += length;
    }


    void CodeBuffer::Emit8(uint8_t x)
    {
        EmitBytes(x);
    }


    void CodeBuffer::Emit16(uint16_t x)
    {
        EmitBytes(x);
    }


    void CodeBuffer::Emit32(uint32_t x)
    {
        EmitBytes(x);
    }


    void CodeBuffer::Emit64(uint64_t x)
    {
        EmitBytes(x);
    }


    void CodeBuffer::ReplaceBytes(unsigned startPosition,
                                  uint8_t const *data,
                                  unsigned length)
    {
        LogThrowAssert(startPosition + length <= CurrentPosition(),
                       "Cannot replace parts of the buffer that have not been populated "
                       "(populated: [0, %u), wanted [%u, %u))",
                       CurrentPosition(),
                       startPosition,
                       startPosition + length);

        memmove(&m_bufferStart[startPosition], data, length);
    }


    unsigned CodeBuffer::GetCapacity() const
    {
        return m_capacity;
    }


    uint8_t* CodeBuffer::BufferStart() const
    {
        return m_bufferStart;
    }


    unsigned CodeBuffer::CurrentPosition() const
    {
        return static_cast<unsigned>(m_current - m_bufferStart);
    }


    void CodeBuffer::Reset()
    {
        m_current = m_bufferStart;
        m_localJumpTable.Clear();
    }


    uint8_t* CodeBuffer::Advance(int byteCount)
    {
        VerifyNoBufferOverflow(byteCount);

        uint8_t* start = m_current;
        m_current += byteCount;
        return start;
    }


    void CodeBuffer::Fill(unsigned start, unsigned length, uint8_t value)
    {
        VerifyNoBufferOverflow(length);
        memset(m_bufferStart + start, value, length);
    }


    void CodeBuffer::PatchCallSites()
    {
        m_localJumpTable.PatchCallSites();
    }


    void CodeBuffer::EmitCallSite(Label label, unsigned size)
    {
        m_localJumpTable.AddCallSite(label, m_current, size);

#ifdef _DEBUG
        while (size-- > 0)
        {
            Emit8(0);
        }
#else
        Advance(size);
#endif
    }
}
