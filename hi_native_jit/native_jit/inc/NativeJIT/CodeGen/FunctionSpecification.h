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
#include <iosfwd>   // Diagnostic stream parameter.

#include "../AllocatorVector.h"      // Embedded member.


namespace NativeJIT
{
    struct UnwindInfo;
    class X64CodeGenerator;


    // A class that builds function specification in the form of unwind info,
    // prolog and epilog code. These are built from description of function's
    // behavior (see constructor parameters for more information).
    class FunctionSpecification
    {
    public:
        // Specifies whether/how to set-up the base register. Unused means
        // that no base register will be set up. SetRbpToOriginalRsp will make
        // RBP reserved and set its value to the value that RSP had before the
        // first instruction of the prolog started executing.
        enum class BaseRegisterType { Unused, SetRbpToOriginalRsp };

        // The maximum size for the unwind buffer that needs to be reserved
        // if the number of unwind codes is not known in advance.
        // DESIGN NOTE: not defined inline to avoid inclusion of UnwindCode.h.
        static const unsigned c_maxUnwindInfoBufferSize;

        // The maximum size needed to reserve for prolog or epilog code buffer.
        // Larger values allow for more extreme prolog sizes, but lead to more
        // wasted space in common case (in the scenario when prolog space is
        // reserved and then filled in). Close to 144 bytes is needed when all
        // 8 RXX (plus RSP separately) and 10 XMM non-volatiles need to be saved.
        static const size_t c_maxPrologOrEpilogSize = 144;
        static_assert(c_maxPrologOrEpilogSize <= 256,
                      "Prolog/epilog cannot be larger than 256 bytes");

        // A value larger than 4096 would require the generated code to call _chkstk.
        static const unsigned c_maxStackSize = 4096;
        static_assert(c_maxStackSize <= 4096, "Cannot have stack larger than 4096 bytes");

        // Builds unwind info, prolog and epilog code from the information about
        // function's behavior: maximum number of parameters for the functions
        // that it calls (negative for none), the number of stack slots to
        // reserve for function variables, the set of RXX and XMM registers to
        // save/resetore in prolog/epilog and whether to set-up the base register
        // and how.
        //
        // If BaseRegisterType is SetRbpToOriginalRsp, [rbp] holds the return
        // address, [rbp + 8] home for function's first argument etc, whereas
        // [rbp - 8] holds the first stacj variable etc. This setup allows well
        // defined access to parameters and stack variables during function
        // body generation time, even if the number of required stack slots is
        // not known yet.
        //
        // If diagnosticStream is non-null, it will be used to print x64
        // instructions used for prolog and epilog.
        FunctionSpecification(Allocators::IAllocator& allocator,
                              int maxFunctionCallParameters,
                              unsigned localStackSlotCount,
                              unsigned savedRxxNonVolatilesMask,
                              unsigned savedXmmNonVolatilesMask,
                              BaseRegisterType baseRegisterType,
                              std::ostream* diagnosticStream);

        // Returns the offset that can be added to the current RSP to get the
        // value of RSP that was effective before the prolog started executing.
        // This offset can then be used to access the return address as
        // [rsp + offset], function parameters as [rsp + offset + 8] etc. or
        // stack variables as [rsp + offset - 8] etc.
        int32_t GetOffsetToOriginalRsp() const;

        // Returns a pointer to the unwind info data and its length.
        uint8_t const * GetUnwindInfoBuffer() const;
        unsigned GetUnwindInfoByteLength() const;

        // Returns a pointer to the prolog code and its length.
        uint8_t const * GetProlog() const;
        unsigned GetPrologLength() const;

        // Returns a pointer to the epilog code and its length.
        uint8_t const * GetEpilog() const;
        unsigned GetEpilogLength() const;

    private:
        // Builds unwind info and prolog code from the information about
        // function's behavior. The prolog code and unwind info are built into
        // the two provided buffers.
        // DESIGN NOTE: Conceptually, it makes most sense to first build UnwindInfo
        // and then build prolog from it. However, unwind info and prolog code
        // are tightly coupled since offsets to prolog instructions need to be
        // specified in unwind codes. Also, some instructions not directly
        // corresponding to unwind codes may be included in prolog (e.g. setting
        // up of RBP).
        static void BuildUnwindInfoAndProlog(int maxFunctionCallParameters,
                                             unsigned localStackSlotCount,
                                             unsigned savedRxxNonVolatilesMask,
                                             unsigned savedXmmNonVolatilesMask,
                                             BaseRegisterType baseRegisterType,
                                             // Out parameters:
                                             X64CodeGenerator& prologCode,
                                             AllocatorVector<uint8_t>& unwindInfoBuffer,
                                             int32_t& m_offsetToOriginalRsp);

        // Uses the unwind information to generate epilog code into the current
        // position of the provided code generator.
        static void BuildEpilog(UnwindInfo const & unwindInfo,
                                X64CodeGenerator& epilogCode);

        // Need to save at most 8 RXX and 10 XMM non-volatiles. Each save takes
        // 2 codes for a total of 36 codes. Additionally, at most 2 codes to
        // allocate stack space.
        static const unsigned c_maxUnwindCodes = 38;
        static_assert((c_maxUnwindCodes % 2) == 0, "Must have an even number of max unwind codes for alignment");
        static_assert(c_maxUnwindCodes <= 256, "Cannot have more than 256 unwind codes");

        // Offset to add to RSP after it's modified in prolog to get the value
        // of RSP before prolog started executing.
        int32_t m_offsetToOriginalRsp;

        Allocators::StlAllocator<uint8_t> m_stlAllocator;
        AllocatorVector<uint8_t> m_unwindInfoBuffer;
        AllocatorVector<uint8_t> m_prologCode;
        AllocatorVector<uint8_t> m_epilogCode;
    };
}

