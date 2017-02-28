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

#include <cstdint>
#include <ostream>
#include <type_traits>

// _M_X64 is defined by VC, as well as icc on Windows.
// __amd64__ is defined by gcc/clang.
// __x86_64__ is defined by icc.
#if !defined(_M_X64) && !defined(__amd64__) && !defined(__x86_64__)
#error This code must be compiled for _AMD64_
#endif

#ifdef NATIVEJIT_PLATFORM_WINDOWS
#include <windows.h>
#else
typedef uint64_t DWORD64;
#endif

namespace NativeJIT
{
    //*************************************************************************
    //
    // Windows unwind structure definitions.
    // From MSDN: http://msdn.microsoft.com/en-us/library/ssa62fwe.aspx
    // Page title is "Unwind Data Definitions in C"
    //
    // Unwind information explanation: http://uninformed.org/index.cgi?v=4&a=1&p=13
    // Stack frame annotation explanation: http://uninformed.org/index.cgi?v=4&a=1&p=17
    //
    //*************************************************************************
    enum class UnwindCodeOp
    {
        UWOP_PUSH_NONVOL = 0, // info == register number
        UWOP_ALLOC_LARGE,     // no info, alloc size in next 2 slots
        UWOP_ALLOC_SMALL,     // info == size of allocation / 8 - 1
        UWOP_SET_FPREG,       // no info, FP = RSP + UNWIND_INFO.FPRegOffset*16
        UWOP_SAVE_NONVOL,     // info == register number, offset in next slot
        UWOP_SAVE_NONVOL_FAR, // info == register number, offset in next 2 slots
        UWOP_SAVE_XMM128,     // info == XMM reg number, offset in next slot
        UWOP_SAVE_XMM128_FAR, // info == XMM reg number, offset in next 2 slots
        UWOP_PUSH_MACHFRAME   // info == 0: no error-code, 1: error-code
    };


    // MSDN struct UNWIND_CODE: https://msdn.microsoft.com/en-us/library/ck9asaa9.aspx
    struct UnwindCode
    {
        UnwindCode();
        UnwindCode(uint8_t codeOffset, UnwindCodeOp op, uint8_t info);
        UnwindCode(uint16_t frameOffset);

        union
        {
            // Note: C++ doesn't allow nameless structures, so naming this one
            // m_operation.
            struct
            {
                // MSDN: Offset from the beginning of the prolog of the end of
                // the instruction that performs this operation, plus 1 (that is,
                // the offset of the start of the next instruction).
                uint8_t m_codeOffset;

                uint8_t m_unwindOp : 4;
                uint8_t m_opInfo   : 4;
            } m_operation;

            uint16_t m_frameOffset;
        };
    };

    static_assert(std::is_standard_layout<UnwindCode>::value, "UnwindCode must be standard layout.");
    static_assert(sizeof(UnwindCode) == 2, "UnwindCode must be exactly two bytes");


    // MSDN struct UNWIND_INFO: https://msdn.microsoft.com/en-us/library/ddssxxy8.aspx
    // Notable points: "The UNWIND_INFO structure must be DWORD aligned in memory."
    // "For alignment purposes, [unwind codes] array will always have an even number
    // of entries, with the final entry potentially unused (in which case the array
    // will be one longer than indicated by the count of unwind codes field)."
    struct UnwindInfo
    {
        UnwindInfo();

        uint8_t m_version       : 3;
        uint8_t m_flags         : 5;
        uint8_t m_sizeOfProlog;
        uint8_t m_countOfCodes;
        uint8_t m_frameRegister : 4;
        uint8_t m_frameOffset   : 4;

        // Additional codes follow after the end of the structure.
        UnwindCode m_firstUnwindCode;
    };

    static_assert(std::is_standard_layout<UnwindInfo>::value,
                  "UnwindInfo must be standard layout.");
    static_assert(sizeof(UnwindInfo) == 4 + sizeof(UnwindCode),
                  "Invalid UnwindInfo size, unexpected padding or extra members present");

    namespace UnwindUtils
    {
        // Creates a function table identifier for a dynamically generated function.
        // The identifier is used for RtlInstallFunctionTableCallback and
        // RtlDeleteFunctionTable calls. Per their requirements, the two low-order
        // bits of the identifier will be set.
        //
        // The provided address must uniquely identify the buffer containing the
        // function code. Some examples include the address of the buffer itself
        // or the address of a class instance that owns the buffer.
        DWORD64 MakeFunctionTableIdentifier(void* objectAddress);
    }
}
