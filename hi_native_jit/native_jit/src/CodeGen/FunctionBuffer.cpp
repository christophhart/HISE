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

#include "../../inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "../../inc/NativeJIT/CodeGen/FunctionSpecification.h"
#include "UnwindCode.h"


// linux x64 stack unwinding
// http://blog.reverberate.org/2013/05/deep-wizardry-stack-unwinding.html
// http://www.hexblog.com/wp-content/uploads/2012/06/Recon-2012-Skochinsky-Compiler-Internals.pdf
// https://llvm.org/bugs/show_bug.cgi?id=24233
// https://bugzilla.mozilla.org/show_bug.cgi?id=844196
// https://sourceware.org/gdb/current/onlinedocs/gdb/JIT-Interface.html#JIT-Interface

// linux x64 ABI
// http://x86-64.org/documentation/abi.pdf


namespace NativeJIT
{
    //*************************************************************************
    //
    // FunctionBuffer
    //
    //*************************************************************************

#ifdef NATIVEJIT_PLATFORM_WINDOWS
    RUNTIME_FUNCTION*
    FunctionBuffer::WindowsGetRuntimeFunctionCallback(DWORD64 controlPc, void* context)
    {
        auto const pc = reinterpret_cast<uint8_t const *>(controlPc);
        auto const fb = reinterpret_cast<FunctionBuffer /* const */ *>(context);
        auto const runtime = &fb->m_runtimeFunction;

        // Check whether program counter is inside function's code.
        return ((fb->BufferStart() + runtime->BeginAddress <= pc)
                && (pc < fb->BufferStart() + runtime->EndAddress))
                ? runtime
                : nullptr;
    }
#endif


    FunctionBuffer::FunctionBuffer(Allocators::IAllocator& codeAllocator,
                                   unsigned capacity)
        : X64CodeGenerator(codeAllocator, capacity),
          m_runtimeFunction(),
          m_unwindInfoStartOffset(0),
          m_unwindInfoByteLength(0),
          m_prologStartOffset(0),
          m_prologLength(0),
          m_isCodeGenerationCompleted(false)
    {
        LogThrowAssert(reinterpret_cast<size_t>(&m_runtimeFunction) % sizeof(DWORD) == 0,
                       "RUNTIME_FUNCTION must be DWORD aligned");

#ifdef NATIVEJIT_PLATFORM_WINDOWS

        // Register a callback to return the RUNTIME_FUNCTION when Windows
        // asks for it during exception handling.
        if (!RtlInstallFunctionTableCallback(UnwindUtils::MakeFunctionTableIdentifier(this),
                                             reinterpret_cast<DWORD64>(BufferStart()),
                                             GetCapacity(),
                                             &FunctionBuffer::WindowsGetRuntimeFunctionCallback,
                                             this,
                                             nullptr))
        {
            throw std::runtime_error("Couldn't install function table callback");
        }
#endif
    }


    FunctionBuffer::~FunctionBuffer()
    {
#ifdef NATIVEJIT_PLATFORM_WINDOWS
        // From MSDN about the argument to RtlDeleteFunctionTable: "A pointer to
        // ... or an identifier previously passed to RtlInstallFunctionTableCallback."
        // TODO: return code not checked as there's nothing else to do on error but
        // log, however no logging facility is available right now.
        auto entry = reinterpret_cast<RUNTIME_FUNCTION*>(
            UnwindUtils::MakeFunctionTableIdentifier(this));
        RtlDeleteFunctionTable(entry);
#endif
    }


    void const * FunctionBuffer::GetEntryPoint() const
    {
        LogThrowAssert(m_isCodeGenerationCompleted,
                       "Cannot get entry point until code generation is finalized");

        return BufferStart() + m_runtimeFunction.BeginAddress;
    }

    unsigned FunctionBuffer::GetFunctionCodeStartOffset() const
    {
        LogThrowAssert(m_isCodeGenerationCompleted,
                       "Cannot get start offset until code generation is finalized");

        return m_runtimeFunction.BeginAddress;
    }


    unsigned FunctionBuffer::GetFunctionCodeEndOffset() const
    {
        LogThrowAssert(m_isCodeGenerationCompleted,
                       "Cannot get end offset until code generation is finalized");

        return m_runtimeFunction.EndAddress;
    }


    unsigned FunctionBuffer::GetUnwindInfoStartOffset() const
    {
        LogThrowAssert(m_isCodeGenerationCompleted,
                       "Cannot get unwind info offset until code generation is finalized");

        return m_runtimeFunction.UnwindData;
    }


    void FunctionBuffer::BeginFunctionBodyGeneration(FunctionSpecification const & spec)
    {
        BeginFunctionBodyGeneration(spec.GetUnwindInfoByteLength(),
                                    spec.GetPrologLength());
    }


    void FunctionBuffer::BeginFunctionBodyGeneration()
    {
        // Function specification is unknown at this point. Reserve enough
        // space for unwind info and prolog to be filled in after it's known.
        BeginFunctionBodyGeneration(FunctionSpecification::c_maxUnwindInfoBufferSize,
                                    FunctionSpecification::c_maxPrologOrEpilogSize);
    }


    void FunctionBuffer::BeginFunctionBodyGeneration(unsigned reservedUnwindInfoLength,
                                                     unsigned reservedPrologLength)
    {
        LogThrowAssert(!m_isCodeGenerationCompleted, "Code generation has already been completed");

        LogThrowAssert(reservedUnwindInfoLength % sizeof(DWORD) == 0,
                       "Unaligned reserved UnwindInfo length of %u bytes",
                       reservedUnwindInfoLength);

        // Begining of UnwindInfo must be DWORD-aligned.
        AdvanceToAlignment<DWORD>();
        m_unwindInfoStartOffset = CurrentPosition();
        m_unwindInfoByteLength = reservedUnwindInfoLength;
        Advance(m_unwindInfoByteLength);

        m_prologStartOffset = CurrentPosition();
        m_prologLength = reservedPrologLength;
        Advance(m_prologLength);
    }


    void FunctionBuffer::EndFunctionBodyGeneration(FunctionSpecification const & spec)
    {
        LogThrowAssert(spec.GetUnwindInfoByteLength() <= m_unwindInfoByteLength,
                       "Unwind info length of %u bytes is larger than the reserved %u bytes",
                       spec.GetUnwindInfoByteLength(),
                       m_unwindInfoByteLength);

        LogThrowAssert(spec.GetPrologLength() <= m_prologLength,
                       "Prolog length of %u bytes is larger than the reserved %u bytes",
                       spec.GetPrologLength(),
                       m_prologLength);

        // Write the unwind info to the buffer.
        // Note: alignment for start and max size of the target has already been
        // verified in BeginFunctionBodyGeneration().
        ReplaceBytes(m_unwindInfoStartOffset,
                     spec.GetUnwindInfoBuffer(),
                     spec.GetUnwindInfoByteLength());
        m_unwindInfoByteLength = spec.GetUnwindInfoByteLength();

        // If more bytes was reserved than was needed for the prolog, adjust
        // the offset and length so that the end of the prolog is adjascent to
        // the beginning of the body of the function.
        if (spec.GetPrologLength() < m_prologLength)
        {
            const unsigned delta = m_prologLength - spec.GetPrologLength();

            m_prologStartOffset += delta;
            m_prologLength -= delta;
        }

        ReplaceBytes(m_prologStartOffset,
                     spec.GetProlog(),
                     spec.GetPrologLength());

        // Emit the epilog at the current position.
        EmitBytes(spec.GetEpilog(), spec.GetEpilogLength());

        // Patch any references to labels.
        PatchCallSites();

        // Fill in information about the function.
        m_runtimeFunction.BeginAddress = m_prologStartOffset;
        m_runtimeFunction.EndAddress = CurrentPosition();
        m_runtimeFunction.UnwindData = m_unwindInfoStartOffset;

        m_isCodeGenerationCompleted = true;
    }


    void FunctionBuffer::Reset()
    {
        X64CodeGenerator::Reset();

        m_unwindInfoStartOffset
            = m_unwindInfoByteLength
            = m_prologStartOffset
            = m_prologLength
            = 0;
        m_isCodeGenerationCompleted = false;
        m_runtimeFunction = {0, 0, 0};
    }
}
