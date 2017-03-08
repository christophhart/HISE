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


#include <iomanip>
#include <iostream>

#include "../../inc/NativeJIT/CodeGen/X64CodeGenerator.h"
#include "../../inc/Temporary/Assert.h"


inline unsigned __int64 popcnt64x86(unsigned __int64 value)
{
	const unsigned int* v_int = reinterpret_cast<unsigned int*>(&value);

	return (unsigned __int64)(__popcnt(v_int[0]) + __popcnt(v_int[1]));
}

namespace NativeJIT
{
    //*************************************************************************
    //
    // Label
    //
    //*************************************************************************
    Label::Label()
        : m_id(0)
    {
    }


    Label::Label(size_t id)
        : m_id(id)
    {
    }


    size_t Label::GetId() const
    {
        return m_id;
    }


    //*************************************************************************
    //
    // X64CodeGenerator public methods
    //
    //*************************************************************************
    X64CodeGenerator::X64CodeGenerator(Allocators::IAllocator& codeAllocator,
                                       unsigned capacity)
        : CodeBuffer(codeAllocator, capacity),
          m_diagnosticsStream(nullptr)
    {
    }


    void X64CodeGenerator::EnableDiagnostics(std::ostream& out)
    {
        m_diagnosticsStream = &out;
    }


    void X64CodeGenerator::DisableDiagnostics()
    {
        m_diagnosticsStream = nullptr;
    }


    void X64CodeGenerator::PlaceLabel(Label l)
    {
        CodePrinter printer(*this);

        CodeBuffer::PlaceLabel(l);
        printer.PlaceLabel(l);
    }


    void X64CodeGenerator::Call(Label label)
    {
        CodePrinter printer(*this);

        Emit8(0xe8);
        EmitCallSite(label, 4);

        printer.PrintJump(label);
    }


    void X64CodeGenerator::Jmp(Label label)
    {
        CodePrinter printer(*this);

        Emit8(0xe9);
        EmitCallSite(label, 4);

        printer.PrintJump(label);
    }


    void X64CodeGenerator::Jmp(void* functionPtr)
    {
        CodePrinter printer(*this);

        Emit8(0xe9);
        Emit64(reinterpret_cast<uint64_t>(functionPtr));

        printer.PrintJump(functionPtr);
    }


    char const * X64CodeGenerator::OpCodeName(OpCode op)
    {
        static char const * names[] = {
            "add",
            "and",
            "bsf",
            "bsr",
            "bt",
            "btc",
            "btr",
            "bts",
            "call",
            "cmp",
            "cvtfp2fp",
            "cvtfp2si",
            "cvtsi2fp",
            "dec",
            "imul",
            "inc",
            "lea",
            "mov",
            "movsx",
            "movzx",
            "movap",
            "neg",
            "nop",
            "not",
            "or",
            "pop",
            "push",
            "rep",
            "ret",
            "rol",
            "shl",
            "shld",
            "shr",
            "stosq",
            "sub",
            "xor",
        };

        static_assert(static_cast<unsigned>(OpCode::OpCodeCount) == std::extent<decltype(names)>::value,
                      "Mismatched number of opcode names.");
        LogThrowAssert(static_cast<unsigned>(op)  < std::extent<decltype(names)>::value, "Invalid OpCode");

        return names[static_cast<unsigned>(op)];
    }


    char const * X64CodeGenerator::JccName(JccType jcc)
    {
        static char const * names[] = {
            "jo",
            "jno",
            "jb",
            "jae",
            "je",
            "jne",
            "jbe",
            "ja",
            "js",
            "jns",
            "jp",
            "jnp",
            "jl",
            "jnl",
            "jle",
            "jnle"
        };

        static_assert(static_cast<unsigned>(JccType::JccCount) == std::extent<decltype(names)>::value,
                      "Mismatched number of JCC names.");
        LogThrowAssert(static_cast<unsigned>(jcc)  < std::extent<decltype(names)>::value, "Invalid JCC");

        return names[static_cast<unsigned>(jcc)];
    }


    bool X64CodeGenerator::IsDiagnosticsStreamAvailable() const
    {
        return m_diagnosticsStream != nullptr;
    }


    std::ostream& X64CodeGenerator::GetDiagnosticsStream() const
    {
        LogThrowAssert(m_diagnosticsStream != nullptr, "Diagnostics are disabled");

        return *m_diagnosticsStream;
    }


    void X64CodeGenerator::Call(Register<8, false> r)
    {
        // EmitRex() would set REX.W, but this instruction defaults to
        // 64-bit operands in 64-bit mode and doesn't need it.
        if (r.IsExtended())
        {
            Emit8(0x41);
        }
        Emit8(0xff);
        Emit8(0xD0 | r.GetId8());
    }


    void X64CodeGenerator::Pop(Register<8, false> r)
    {
        // EmitRex() would set REX.W, but this instruction defaults to
        // 64-bit operands in 64-bit mode and doesn't need it.
        if (r.IsExtended())
        {
            Emit8(0x41);
        }
        Emit8(0x58 + r.GetId8());
    }


    void X64CodeGenerator::Push(Register<8, false> r)
    {
        // EmitRex() would set REX.W, but this instruction defaults to
        // 64-bit operands in 64-bit mode and doesn't need it.
        if (r.IsExtended())
        {
            Emit8(0x41);
        }
        Emit8(0x50 + r.GetId8());
    }


    void X64CodeGenerator::Ret()
    {
        Emit8(0xc3);
    }


    //*************************************************************************
    //
    // X64CodeGenerator::Helper<Op> methods.
    //
    //*************************************************************************
    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::Call>::ArgTypes1<false>::Emit(X64CodeGenerator& code, Register<8, false> dest)
    {
        code.Call(dest);
    }


    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::Pop>::ArgTypes1<false>::Emit(X64CodeGenerator& code, Register<8, false> dest)
    {
        code.Pop(dest);
    }


    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::Push>::ArgTypes1<false>::Emit(X64CodeGenerator& code, Register<8, false> dest)
    {
        code.Push(dest);
    }


    template <> void X64CodeGenerator::Helper<OpCode::Rep>::Emit(X64CodeGenerator& code)
    {
        code.Emit8(0xf3);
    }


    template <> void X64CodeGenerator::Helper<OpCode::Ret>::Emit(X64CodeGenerator& code)
    {
        code.Ret();
    }


    template <> void X64CodeGenerator::Helper<OpCode::Stosq>::Emit(X64CodeGenerator& code)
    {
        code.Emit8(0x48);
        code.Emit8(0xab);
    }


    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<4, false> dest,
        Register<4, false> src)
    {
        Helper<OpCode::Mov>::ArgTypes1<false>::Emit<4>(code, dest, src);
    }

    // Full specialization of MovZX opcode for "movzx dest64, src32" which is
    // implemented in terms of "mov dest32, src32".

    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<8, false> dest,
        Register<4, false> src)
    {
        const Register<4, false> dest4(dest);

        Helper<OpCode::Mov>::ArgTypes1<false>::Emit<4>(code, dest4, src);
    }


    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit<8, 4>(
        X64CodeGenerator& code,
        Register<8, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        const Register<4, false> dest4(dest);

        Helper<OpCode::Mov>::ArgTypes1<false>::Emit<4>(code, dest4, src, srcOffset);
    }


    //
    // IosMiniStateRestorer
    //

    IosMiniStateRestorer::IosMiniStateRestorer(std::ios& stream)
        : m_stream(stream),
          m_flags(stream.flags()),
          m_width(stream.width()),
          m_fillChar(stream.fill())
    {
    }


    IosMiniStateRestorer::~IosMiniStateRestorer()
    {
        m_stream.flags(m_flags);
        m_stream.width(m_width);
        m_stream.fill(m_fillChar);
    }



    //*************************************************************************
    //
    // Helper code printing methods.
    //
    //*************************************************************************

    X64CodeGenerator::CodePrinter::CodePrinter(X64CodeGenerator& code)
        : m_code(code),
          m_out(code.IsDiagnosticsStreamAvailable()
                ? &code.GetDiagnosticsStream()
                : nullptr)
    {
        NoteStartPosition();
    }


    void X64CodeGenerator::CodePrinter::NoteStartPosition()
    {
        m_startPosition = m_code.CurrentPosition();
    }


    void X64CodeGenerator::CodePrinter::PlaceLabel(Label label)
    {
        if (m_out != nullptr)
        {
            *m_out << "L" << label.GetId() << ":" << std::endl;
        }
    }


    void X64CodeGenerator::CodePrinter::PrintJump(Label label)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << "jmp L" << label.GetId() << std::endl;
        }
    }


    void X64CodeGenerator::CodePrinter::PrintJump(void* function)
    {
        if (m_out != nullptr)
        {
            IosMiniStateRestorer state(*m_out);

            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << "jmp " << std::uppercase << std::hex << function << 'h' << std::endl;
        }
    }


    void X64CodeGenerator::CodePrinter::Print(OpCode op)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op) << std::endl;
        }
    }


    char const * X64CodeGenerator::CodePrinter::GetPointerName(unsigned pointerSize)
    {
        switch (pointerSize)
        {
        case 1:     return "byte";
        case 2:     return "word";
        case 4:     return "dword";
        case 8:     return "qword";
        default:    return "*** UNKNOWN ***";
        }
    }


    const unsigned c_asmDataWidth = 36;

    void X64CodeGenerator::CodePrinter::PrintBytes(unsigned start, unsigned end)
    {
        IosMiniStateRestorer state(*m_out);

        // TODO: Support for printing '/' after REX prefixes?
        // Alternative is unit test function can strip out white space and '/'.
        uint8_t* startPtr = m_code.BufferStart() + start;
        uint8_t* endPtr = m_code.BufferStart() + end;

        m_out->fill('0');

        *m_out << " ";
        *m_out << std::uppercase << std::hex << std::setw(8) << start << "  ";

        unsigned column = 11;
        while (startPtr < endPtr)
        {
            *m_out << std::setw(2) << static_cast<unsigned>(*startPtr++);
            *m_out << " ";
            column += 3;
        }

        while (column < c_asmDataWidth)
        {
            *m_out << ' ';
            column++;
        }
    }
}
