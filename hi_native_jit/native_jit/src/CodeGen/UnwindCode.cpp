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


#include "../../inc/NativeJIT/CodeGen/Register.h"
#include "UnwindCode.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // UnwindCode
    //
    //*************************************************************************
    UnwindCode::UnwindCode()
    {
    }


    UnwindCode::UnwindCode(uint16_t frameOffset)
        : m_frameOffset(frameOffset)
    {
    }


    UnwindCode::UnwindCode(uint8_t codeOffset,
                           UnwindCodeOp op,
                           uint8_t info)
    {
        // Note: cannot use initializer list as VC++ doesn't support it (error
        // C2797 "List initialization inside member initializer list or
        // non-static data member initializer is not implemented."
        m_operation.m_codeOffset = codeOffset;
        m_operation.m_unwindOp = static_cast<uint8_t>(op);
        m_operation.m_opInfo = info;
    }

    namespace UnwindUtils
    {
        DWORD64 MakeFunctionTableIdentifier(void* objectAddress)
        {
            // Per MSDN RtlInstallFunctionTableCallback() documentation:
            // "The two low-order bits must be set. For example, BaseAddress|0x3."
            return reinterpret_cast<DWORD64>(objectAddress) | 3;
        }
    }
}
