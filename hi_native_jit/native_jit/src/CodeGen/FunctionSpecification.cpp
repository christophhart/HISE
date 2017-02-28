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


#include <algorithm>    // For std::min.
#include <stdexcept>

#include "../../inc/NativeJIT/BitOperations.h"
#include "../../inc/NativeJIT/CodeGen/CallingConvention.h"
#include "../../inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "../../inc/NativeJIT/CodeGen/FunctionSpecification.h"
#include "../../inc/Temporary/IAllocator.h"
#include "UnwindCode.h"


namespace NativeJIT
{
    //
    // FunctionSpecificationBase
    //

    // Note: defining in .cpp file to avoid the need to include UnwindCode.h
    // in FunctionSpecification.h.
    const unsigned FunctionSpecification::c_maxUnwindInfoBufferSize
        = sizeof(UnwindInfo)
          - sizeof(UnwindCode) // Included in UnwindInfo.
          + c_maxUnwindCodes * sizeof(UnwindCode);



    FunctionSpecification::FunctionSpecification(Allocators::IAllocator& allocator,
                                                 int maxFunctionCallParameters,
                                                 unsigned localStackSlotCount,
                                                 unsigned savedRxxNonVolatilesMask,
                                                 unsigned savedXmmNonVolatilesMask,
                                                 BaseRegisterType baseRegisterType,
                                                 std::ostream* diagnosticsStream)
        : m_stlAllocator(allocator),
          m_unwindInfoBuffer(m_stlAllocator),
          m_prologCode(m_stlAllocator),
          m_epilogCode(m_stlAllocator)
    {
        // The code in this buffer will not be executed directly, so the general
        // allocator can be used for code buffer allocation.
        X64CodeGenerator code(allocator, c_maxPrologOrEpilogSize);

        if (diagnosticsStream != nullptr)
        {
            code.EnableDiagnostics(*diagnosticsStream);
        }

        BuildUnwindInfoAndProlog(maxFunctionCallParameters,
                                 localStackSlotCount,
                                 savedRxxNonVolatilesMask,
                                 savedXmmNonVolatilesMask,
                                 baseRegisterType,
                                 code,
                                 m_unwindInfoBuffer,
                                 m_offsetToOriginalRsp);

        m_prologCode.assign(code.BufferStart(),
                            code.BufferStart() + code.CurrentPosition());

        code.Reset();
        BuildEpilog(*reinterpret_cast<UnwindInfo*>(m_unwindInfoBuffer.data()),
                    code);

        m_epilogCode.assign(code.BufferStart(),
                            code.BufferStart() + code.CurrentPosition());
    }


    namespace
    {
        // Populates the current UnwindCode with the provided values and updates
        // the pointer to the predecessor unwind code (to facilitate reverse,
        // epilog-like order of unwind codes). Verifies that the current code does
        // not come before the starting boundary.
        void AddCodeAndBackDown(UnwindCode const * unwindCodesStart,
                                UnwindCode*& currUnwindCode,
                                unsigned codeOffset,
                                UnwindCodeOp op,
                                uint8_t info)
        {
            LogThrowAssert(currUnwindCode >= unwindCodesStart, "Unwind codes overflow");
            LogThrowAssert(codeOffset <= (std::numeric_limits<uint8_t>::max)(),
                           "Code offset overflow: %u",
                           codeOffset);
            *currUnwindCode-- = UnwindCode(static_cast<uint8_t>(codeOffset),
                                           op,
                                           info);
        }


        // The flavor which populates unwind data that requires two unwind codes:
        // one with core information and the other one with 16-bit value.
        void AddCodeAndBackDown(UnwindCode const * unwindCodesStart,
                                UnwindCode*& currUnwindCode,
                                unsigned codeOffset,
                                UnwindCodeOp op,
                                uint8_t info,
                                uint16_t frameOffset)
        {
            // Since the codes are filled in reverse (epilog) order, place the
            // second operand first.
            LogThrowAssert(currUnwindCode >= unwindCodesStart, "Unwind codes overflow");
            *currUnwindCode-- = UnwindCode(frameOffset);

            AddCodeAndBackDown(unwindCodesStart, currUnwindCode, codeOffset, op, info);
        }
    }


    void FunctionSpecification::BuildUnwindInfoAndProlog(int maxFunctionCallParameters,
                                                         unsigned localStackSlotCount,
                                                         unsigned savedRxxNonVolatilesMask,
                                                         unsigned savedXmmNonVolatilesMask,
                                                         BaseRegisterType baseRegisterType,
                                                         X64CodeGenerator& prologCode,
                                                         AllocatorVector<uint8_t>& unwindInfoBuffer,
                                                         int32_t& offsetToOriginalRsp)
    {
        LogThrowAssert((savedRxxNonVolatilesMask & ~CallingConvention::c_rxxWritableRegistersMask) == 0,
                       "Saving/restoring of non-writable RXX registers is not allowed: 0x%Ix",
                       savedRxxNonVolatilesMask & ~CallingConvention::c_rxxWritableRegistersMask);

        LogThrowAssert((savedXmmNonVolatilesMask & ~CallingConvention::c_xmmWritableRegistersMask) == 0,
                       "Saving/restoring of non-writable XMM registers is not allowed: 0x%Ix",
                       savedXmmNonVolatilesMask & ~CallingConvention::c_xmmWritableRegistersMask);

        // Stack pointer is always saved/restored. However, unlike for the other
        // registers, it's done by subtracting/adding a value in the prolog/epilog.
        savedRxxNonVolatilesMask &= ~rsp.GetMask();

        // Ensure that the frame register is saved if used.
        if (baseRegisterType == BaseRegisterType::SetRbpToOriginalRsp)
        {
            savedRxxNonVolatilesMask |= rbp.GetMask();
        }

        const unsigned codeStartPos = prologCode.CurrentPosition();

        // If there are any function calls, at least 4 parameter slots need to
        // be allocated regardless of the actual parameter count.
        const unsigned functionParamsSlotCount
            = maxFunctionCallParameters >= 0
              ? (std::max)(maxFunctionCallParameters, 4)
              : 0;

        const unsigned rxxSavesCount = BitOp::GetNonZeroBitCount(savedRxxNonVolatilesMask);
        const unsigned xmmSavesCount = BitOp::GetNonZeroBitCount(savedXmmNonVolatilesMask);

        // All 128 bits of XMM registers need to saved in the prolog, so each
        // XMM register needs two slots. Also, XMM slots need to be 16-byte aligned,
        // so reserve one additional slot which may be needed for alignment.
        const unsigned regSavesSlotCount = rxxSavesCount
                                           + 2 * xmmSavesCount
                                           + (xmmSavesCount > 0 ? 1 : 0);

        // Calculate the total number of allocated stack slots. Ensure it's odd
        // since the stack pointer needs to be 16-byte aligned but it already
        // has one slot used for the return address.
        //
        // Note: there are some cases when it's not required to align the stack.
        // However, for simplicity and because the documentation is contradictory
        // (some sources say alignment is unnecessary only when there are no
        // function calls, some only when there is no stack allocation of any
        // type) the stack is always aligned here.
        //
        // Stack layout after setup:
        // [address 0] ---> [...]
        //     ---> [beginning of stack, 16-byte aligned; RSP adjusted by prolog points here]
        //         ---> [home space, empty or max(4, maxParametersInCallsByFunction); must be placed here]
        //         ---> [registers saved by prolog]
        //         ---> [local stack for temporaries etc]
        //     ---> [end of stack; original RSP pointed here; RBP points here if SetRbpToOriginalRsp]
        //     ---> [return address and parameters to the function]
        const unsigned totalStackSlotCount
            = (functionParamsSlotCount
               + regSavesSlotCount
               + localStackSlotCount)
              | 1;
        const unsigned totalStackBytes = totalStackSlotCount * sizeof(void*);
        offsetToOriginalRsp = totalStackBytes;

        LogThrowAssert(totalStackBytes > 0 && totalStackBytes <= c_maxStackSize,
                       "Invalid request for %u stack slots",
                       totalStackSlotCount);

        // Need to use UWOP_ALLOC_SMALL for stack sizes from 8 to 128 bytes and
        // UWOP_ALLOC_LARGE otherwise. If using UWOP_ALLOC_LARGE, currently only
        // the version which uses two unwind codes is supported. That version
        // can allocate almost 512 kB, which is far more than the 4 kB limit
        // which would require a chkstk call.
        const bool isSmallStackAlloc = totalStackBytes <= 128;

        // Compute number of unwind codes needed. Each RXX/XMM save takes two
        // codes and stack allocation takes 1 for UWOP_ALLOC_SMALL and 2 for
        // UWOP_ALLOC_LARGE (also see the previous comment).
        const unsigned actualUnwindCodeCount
            = (rxxSavesCount + xmmSavesCount) * 2
              + (isSmallStackAlloc ? 1 : 2);

        LogThrowAssert(actualUnwindCodeCount > 0
                       && actualUnwindCodeCount <= c_maxUnwindCodes,
                       "Invalid number of unwind codes: %u",
                       actualUnwindCodeCount);

        // From MSDN UNWIND_INFO documentation for unwind codes array:
        // "For alignment purposes, this array will always have an even number
        // of entries, with the final entry potentially unused (in which case
        // the array will be one longer than indicated by the count of unwind
        // codes field)."
        const unsigned alignedCodeCount = (actualUnwindCodeCount + 1) & ~1u;

        // Allocate the memory. Account for the fact that one unwind code is
        // already included in UnwindInfo. Due to the check above that unwind
        // count is positive, the calculation will not overflow.
        unwindInfoBuffer.resize(sizeof(UnwindInfo)
                                + (alignedCodeCount - 1)
                                  * sizeof(UnwindCode));
        UnwindInfo* unwindInfo = reinterpret_cast<UnwindInfo*>(unwindInfoBuffer.data());

        // Initialize UnwindInfo.
        unwindInfo->m_countOfCodes = static_cast<unsigned char>(actualUnwindCodeCount);
        unwindInfo->m_version = 1;
        unwindInfo->m_flags = 0;
        unwindInfo->m_frameRegister = 0;
        unwindInfo->m_frameOffset = 0;

        // Unwind codes are placed to the array of codes in order that will be
        // used in epilog (i.e. reverse order of steps), so locate the end first.
        UnwindCode* unwindCodes = &unwindInfo->m_firstUnwindCode;
        UnwindCode* currUnwindCode = &unwindCodes[actualUnwindCodeCount - 1];

        // Start emitting the unwind codes and the opcodes for prolog. First,
        // adjust the stack pointer.
        prologCode.EmitImmediate<OpCode::Sub>(rsp, offsetToOriginalRsp);

        // Emit the matching unwind codes.
        if (isSmallStackAlloc)
        {
            LogThrowAssert(totalStackSlotCount >= 1 && totalStackSlotCount <= 16,
                           "Logic error, alloc small slot count %u",
                           totalStackSlotCount);

            // The slot count values 1-16 are encoded as 0-15, so subtract one.
            AddCodeAndBackDown(unwindCodes,
                               currUnwindCode,
                               prologCode.CurrentPosition() - codeStartPos,
                               UnwindCodeOp::UWOP_ALLOC_SMALL,
                               static_cast<uint8_t>(totalStackSlotCount - 1));
        }
        else
        {
            LogThrowAssert(totalStackSlotCount >= 17
                           && totalStackSlotCount <= (std::numeric_limits<uint16_t>::max)(),
                           "Logic error, alloc large slot count %u",
                           totalStackSlotCount);

            // Value of 0 for info argument signifies two-code version of
            // UWOP_ALLOC_LARGE which is used for allocations from 136 to
            // 512 kB - 8 bytes (i.e. 17 to 65535 slots).
            AddCodeAndBackDown(unwindCodes,
                               currUnwindCode,
                               prologCode.CurrentPosition() - codeStartPos,
                               UnwindCodeOp::UWOP_ALLOC_LARGE,
                               static_cast<uint8_t>(0),
                               static_cast<uint16_t>(totalStackSlotCount));
        }

        // Save registers into the reserved area. The area comes right after
        // the initial slots reserved for parameters for function calls.
        unsigned currStackSlotOffset = functionParamsSlotCount;

        unsigned regId = 0;
        unsigned registersMask = savedRxxNonVolatilesMask;

        // Save the RXX registers.
        while (BitOp::GetLowestBitSet(registersMask, &regId))
        {
            prologCode.Emit<OpCode::Mov>(rsp,
                                         currStackSlotOffset * sizeof(void*),
                                         Register<8, false>(regId));

            AddCodeAndBackDown(unwindCodes,
                               currUnwindCode,
                               prologCode.CurrentPosition() - codeStartPos,
                               UnwindCodeOp::UWOP_SAVE_NONVOL,
                               static_cast<uint8_t>(regId),
                               static_cast<uint16_t>(currStackSlotOffset));

            BitOp::ClearBit(&registersMask, regId);
            currStackSlotOffset++;
        }

        // Save XMM registers.
        if (xmmSavesCount > 0)
        {
            // Ensure that slot offset is even (16-byte aligned). The additional
            // slot was already reserved for this case.
            if ((currStackSlotOffset & 1) != 0)
            {
                currStackSlotOffset++;
            }

            registersMask = savedXmmNonVolatilesMask;

            while (BitOp::GetLowestBitSet(registersMask, &regId))
            {
                // Both movaps and movapd can be used here, but according to
                // external tests, movaps is more performant.
                prologCode.Emit<OpCode::MovAP>(
                    rsp,
                    currStackSlotOffset * sizeof(void*),
                    Register<4, true>(regId));

                // The offset specifies 16-byte slots, thus the divide by two.
                // The offset was previously verified to be even.
                AddCodeAndBackDown(unwindCodes,
                                   currUnwindCode,
                                   prologCode.CurrentPosition() - codeStartPos,
                                   UnwindCodeOp::UWOP_SAVE_XMM128,
                                   static_cast<uint8_t>(regId),
                                   static_cast<uint16_t>(currStackSlotOffset / 2));

                BitOp::ClearBit(&registersMask, regId);
                currStackSlotOffset += 2;
            }
        }

        // Ensure that the unwind codes were filled exactly as planned by
        // comparing the last filled code with the beginning of the array. Since
        // currUnwindCode points to the place where the *next* code would have
        // been placed, currUnwindCode + 1 points to the last placed code.
        LogThrowAssert(currUnwindCode + 1 == unwindCodes,
                       "Mismatched count of unwind codes: %Id",
                       currUnwindCode + 1 - unwindCodes);

        // Point the RBP to the original RSP value. Note: not using UWOP_SET_FPREG
        // since 1) it's not necessary on x64 as setting the base pointer is
        // only an optional convenience 2) The offset is limited to [0, 240] range,
        // which may not be enough and 3) some documentation sources state that
        // if used, UWOP_SET_FPREG must occur before any register saves that
        // specify offset, which complicates this function needlessly.
        if (baseRegisterType == BaseRegisterType::SetRbpToOriginalRsp)
        {
            // It's necessary to extend the last unwind code that recorded an
            // instruction offset into prolog to account for the instruction
            // to be added to set up RBP.
            LogThrowAssert(unwindCodes[0].m_operation.m_codeOffset
                           == prologCode.CurrentPosition() - codeStartPos,
                           "Logical error in RBP adjustment, instruction offset %u vs %u",
                           unwindCodes[0].m_operation.m_codeOffset,
                           prologCode.CurrentPosition() - codeStartPos);

            prologCode.Emit<OpCode::Lea>(rbp, rsp, offsetToOriginalRsp);

            // Note: the cast is safe, the code buffer is length limited to
            // max size for prolog/epilog.
            unwindCodes[0].m_operation.m_codeOffset
                = static_cast<uint8_t>(prologCode.CurrentPosition() - codeStartPos);
        }

        // Code offset points to the next instruction after the end of prolog,
        // so (given that prolog starts from offset 0) it also specifies the
        // size of prolog.
        unwindInfo->m_sizeOfProlog = unwindCodes[0].m_operation.m_codeOffset;
    }


    // A helper function to return the number of unwind codes for an opcode.
    static unsigned GetUnwindOpCodeCount(UnwindCode code)
    {
        switch (static_cast<UnwindCodeOp>(code.m_operation.m_unwindOp))
        {
            case UnwindCodeOp::UWOP_ALLOC_SMALL:
            case UnwindCodeOp::UWOP_PUSH_MACHFRAME:
            case UnwindCodeOp::UWOP_PUSH_NONVOL:
            case UnwindCodeOp::UWOP_SET_FPREG:
                return 1;

            case UnwindCodeOp::UWOP_SAVE_NONVOL:
            case UnwindCodeOp::UWOP_SAVE_XMM128:
                return 2;

            case UnwindCodeOp::UWOP_SAVE_NONVOL_FAR:
            case UnwindCodeOp::UWOP_SAVE_XMM128_FAR:
                return 3;

            case UnwindCodeOp::UWOP_ALLOC_LARGE:
                LogThrowAssert(code.m_operation.m_opInfo <= 1,
                               "Invalid OpInfo for UWOP_ALLOC_LARGE: %u",
                               code.m_operation.m_opInfo);

                return code.m_operation.m_opInfo == 0 ? 2 : 3;

            default:
                LogThrowAbort("Unknown unwind operation %u", code.m_operation.m_unwindOp);
                // Silence the "not all paths return a value" warning.
                return 0;
        }
    }


    void FunctionSpecification::BuildEpilog(UnwindInfo const & unwindInfo,
                                            X64CodeGenerator& epilogCode)
    {
        UnwindCode const * codes = &unwindInfo.m_firstUnwindCode;
        unsigned i = 0;

        while (i < unwindInfo.m_countOfCodes)
        {
            const UnwindCode unwindCode = codes[i];

            // Check how many codes the operation needs.
            const unsigned codeCount = GetUnwindOpCodeCount(unwindCode);
            LogThrowAssert(i + codeCount <= unwindInfo.m_countOfCodes,
                           "Not enough unwind codes for op %u",
                           unwindCode.m_operation.m_unwindOp);

            // For 2 or more codes, read the data for the second code (always
            // the m_frameOffset union member).
            const unsigned code2Offset = codeCount >= 2
                                         ? codes[i + 1].m_frameOffset
                                         : 0;

            switch (static_cast<UnwindCodeOp>(unwindCode.m_operation.m_unwindOp))
            {
            case UnwindCodeOp::UWOP_ALLOC_LARGE:
                LogThrowAssert(codeCount == 2, "Unexpected %u-code UWOP_ALLOC_LARGE", codeCount);
                // The second code contains the offset in quadwords.
                epilogCode.EmitImmediate<OpCode::Add>(rsp,
                                                      static_cast<int32_t>(code2Offset
                                                                           * sizeof(void*)));
                break;

            case UnwindCodeOp::UWOP_ALLOC_SMALL:
                // The second code contains the slot count (in quadwords)
                // decreased by one.
                epilogCode.EmitImmediate<OpCode::Add>(
                    rsp,
                    static_cast<int32_t>((unwindCode.m_operation.m_opInfo + 1)
                                         * sizeof(void*)));
                break;

            case UnwindCodeOp::UWOP_SAVE_NONVOL:
                // The second code contains the offset in quadwords.
                epilogCode.Emit<OpCode::Mov>(Register<8, false>(unwindCode.m_operation.m_opInfo),
                                             rsp,
                                             code2Offset * sizeof(void*));
                break;

            case UnwindCodeOp::UWOP_SAVE_XMM128:
                // The second code contains the halved offset in quadwords.
                epilogCode.Emit<OpCode::MovAP>(
                    Register<4, true>(unwindCode.m_operation.m_opInfo),
                    rsp,
                    code2Offset * 2 * sizeof(void*));
                break;

            default:
                LogThrowAbort("Unsupported unwind operation %u", unwindCode.m_operation.m_unwindOp);
                break;
            }

            i += codeCount;
        }

        // Return to caller.
        epilogCode.Emit<OpCode::Ret>();
    }


    int32_t FunctionSpecification::GetOffsetToOriginalRsp() const
    {
        return m_offsetToOriginalRsp;
    }


    uint8_t const * FunctionSpecification::GetUnwindInfoBuffer() const
    {
        return m_unwindInfoBuffer.data();
    }


    unsigned FunctionSpecification::GetUnwindInfoByteLength() const
    {
        return static_cast<unsigned>(m_unwindInfoBuffer.size());
    }


    uint8_t const * FunctionSpecification::GetProlog() const
    {
        return m_prologCode.data();
    }


    unsigned FunctionSpecification::GetPrologLength() const
    {
        return static_cast<unsigned>(m_prologCode.size());
    }


    uint8_t const * FunctionSpecification::GetEpilog() const
    {
        return m_epilogCode.data();
    }


    unsigned FunctionSpecification::GetEpilogLength() const
    {
        return static_cast<unsigned>(m_epilogCode.size());
    }
}
