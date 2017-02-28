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

// http://wiki.osdev.org/X86-64_Instruction_Encoding
// http://ref.x86asm.net/coder64.html
// http://felixcloutier.com/x86/

#include <ostream>                              // Debugging output.

#include "../BitOperations.h"
#include "CodeBuffer.h"       // Inherits from CodeBuffer.
#include "ValuePredicates.h"  // Called by template code.
#include "Register.h"         // Register parameter.
#include "../../Temporary/NonCopyable.h"              // Inherits from NonCopyable.


#ifdef _MSC_VER
// Supress warning about constant expression involving template parameters.
#pragma warning(push)
#pragma warning(disable:4127)
#endif


namespace Allocators
{
    class IAllocator;
}


namespace NativeJIT
{
    // WARNING: When modifying JccType, be sure to also modify the function JccName().
    enum class JccType : unsigned
    {
        JO = 0,
        JNO = 1,
        JB = 2,
        JAE = 3, JNB = 3, JNC = 3,
        JE = 4, JZ = 4,
        JNE = 5, JNZ = 5,
        JBE = 6, JNA = 6,
        JA = 7, JNBE = 7,
        JS = 8,
        JNS = 9,
        JP = 0xa, JPE = 0xa,
        JNP = 0xb, JPO = 0xb,
        JL = 0xc, JNGE = 0xc,
        JNL = 0xd, JGE = 0xd,
        JLE = 0xe, JNG = 0xe,
        JNLE = 0xf, JG = 0xf,
        // The following value must be the last one.
        JccCount
    };


    // WARNING: When modifying OpCode, be sure to also modify the function OpCodeName().
    enum class OpCode : unsigned
    {
        Add,
        And,
        Bsf,
        Bsr,
        Bt,
        Btc,
        Btr,
        Bts,
        Call,
        Cmp,
        CvtFP2FP,
        CvtFP2SI,
        CvtSI2FP,
        Dec,
        IMul,
        Inc,
        Lea,
        Mov,
        MovSX,
        MovZX,
        MovAP,      // Aligned 128-bit SSE move.
        Neg,
        Nop,
        Not,
        Or,
        Pop,
        Push,
        Rep,
        Ret,
        Rol,
        Shl,        // Note: Shl and Sal are aliases, unlike Shr and Sar.
        Shld,
        Shr,
        Stosq,
        Sub,
        Xor,
        // The following value must be the last one.
        OpCodeCount
    };


    enum class SIB : uint8_t
    {
        Scale1 = 0,
        Scale2 = 1,
        Scale4 = 2,
        Scale8 = 3
    };


    class X64CodeGenerator : public CodeBuffer
    {
    public:
        // Sets up a code buffer with specified capacity. See the CodeBuffer
        // constructor for more details on the allocator.
        X64CodeGenerator(Allocators::IAllocator& codeAllocator, unsigned capacity);

        void EnableDiagnostics(std::ostream& out);
        void DisableDiagnostics();

        // Returns the diagnostic stream. GetDiagnosticsStream() throws if it is not available.
        bool IsDiagnosticsStreamAvailable() const;
        std::ostream& GetDiagnosticsStream() const;

        // This override allows for printing of debugging information.
        virtual void PlaceLabel(Label l) override;

        void Call(Label l);
        void Jmp(Label l);
        void Jmp(void* functionPtr);

        // These two methods are public in order to allow access for BinaryNode debugging text.
        static char const * OpCodeName(OpCode op);
        static char const * JccName(JccType jcc);

        //
        // X64 opcode emit methods.
        //

        // Note: the name of this method was changed from Emit() to work around
        // an internal compiler error in VisualStudio 2013 (VC12). The ICE was
        // fixed in VisualStudio 2015.
        template <JccType JCC>
        void EmitConditionalJump(Label l);

        // No operand (e.g nop, ret)
        template <OpCode OP>
        void Emit();

        // One register operand (e.g. call, neg, not, push, pop)
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<SIZE, ISFLOAT> dest);

        // Two register operands with the same type and size (e.g. and, mov, or, sub).
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src);

        // Base register and offset.
        template <OpCode OP, unsigned SIZE>
        void Emit(Register<8u, false> dest, int32_t offset);

        // Two register operands with different type and/or size (f. ex. movzx).
        template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void Emit(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        // Two operands - register destination and indirect source with the same
        // type and size.
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<SIZE, ISFLOAT> dest, Register<8, false> src, int32_t srcOffset);

        // Two operands - register destination and indirect source. This flavor
        // of the method is used for instructions which can read values of a
        // varying size from the source address. F. ex. movzx with 32-bit
        // target destination can read either a byte or a word from the indirect
        // source.
        template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void Emit(Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset);

        // Scale-index-base (SIB) + offset.
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<SIZE, ISFLOAT> dest,
                  Register<8, false> base,
                  Register<8, false> index,
                  SIB scale,
                  int32_t offset);

        // Two operands - indirect destination and register source with the same type and size.
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<8, false> dest, int32_t destOffset, Register<SIZE, ISFLOAT> src);

        // Scale-index-base (SIB) + offset.
        template <OpCode OP, unsigned SIZE, bool ISFLOAT>
        void Emit(Register<8, false> base,
                  Register<8, false> index,
                  SIB scale,
                  int32_t offset,
                  Register<SIZE, ISFLOAT> src);

        // Two operands - indirect destination and register source. This flavor
        // of the method is used for instructions which can write values of a
        // varying size to the destination address.
        template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void Emit(Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src);

        // Two operands - register destination and immediate source.
        // Note: Method is named EmitImmediate() to avoid clashes with other
        // Emit() methods in case when T gets resolved to f. ex. Register.
        template <OpCode OP, unsigned SIZE, bool ISFLOAT, typename T>
        void EmitImmediate(Register<SIZE, ISFLOAT> dest, T value);

        // Three operands: two register operands of the same size and type and
        // an immediate (f. ex. shld eax, ebx, 4).
        template <OpCode OP, unsigned SIZE, bool ISFLOAT, typename T>
        void EmitImmediate(Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src, T value);

    private:
        void Call(Register<8, false> r);

        template <unsigned SIZE>
        void IMul(Register<SIZE, false> dest,
                  Register<SIZE, false> src);

        template <unsigned SIZE>
        void IMul(Register<SIZE, false> dest,
                  Register<8, false> src,
                  int32_t srcOffset);

        template <unsigned SIZE, typename T>
        void IMulImmediate(Register<SIZE, false> dest,
                           T value);

        template <unsigned SIZE>
        void Lea(Register<SIZE, false> dest,
                 Register<8, false> src,
                 int32_t srcOffset);

        template <unsigned SIZE, typename T>
        void MovImmediate(Register<SIZE, false> dest,
                          T value);

        template <typename T>
        void MovImmediate(Register<8, false> dest,
                          T* value);

        void Pop(Register<8, false> r);
        void Push(Register<8, false> r);

        void Ret();

        template <unsigned SIZE>
        void MovD(Register<SIZE, true> dest, Register<SIZE, false> src);

        template <unsigned SIZE1, unsigned SIZE2>
        void MovSX(Register<SIZE1, false> dest, Register<SIZE2, false> src);

        template <unsigned SIZE1, unsigned SIZE2>
        void MovSX(Register<SIZE1, false> dest, Register<8, false> src, int32_t srcOffset);

        template <unsigned SIZE1, unsigned SIZE2>
        void MovZX(Register<SIZE1, false> dest, Register<SIZE2, false> src);

        template <unsigned SIZE1, unsigned SIZE2>
        void MovZX(Register<SIZE1, false> dest, Register<8, false> src, int32_t srcOffset);

        template <unsigned SIZE>
        void Shld(Register<SIZE, false> dest, Register<SIZE, false> src, uint8_t bitCount);

        template <unsigned SIZE>
        void Shld(Register<SIZE, false> dest, Register<SIZE, false> src);

        // Scalar SSE instructions are encoded as XX 0F OPCODE, where XX is
        // either 0xF2 or 0xF3 depending on the register size. Used for
        // instructions operating on scalars (f. ex. MovSS/SD, AddSS/SD) rather
        // than packed (f. ex. MOVUPS/MOVUPD).

        // Generic method for emitting 0xF2 or 0xF3 prefix depending on the
        // register type and size.
        template <unsigned RMSIZE, bool RMISFLOAT,
                  unsigned REGSIZE, bool REGISFLOAT,
                  unsigned RMREGSIZE, bool RMREGISFLOAT>
        void EmitScalarSSEPrefix(Register<REGSIZE, REGISFLOAT> reg, Register<RMREGSIZE, RMREGISFLOAT> rm);

        // Two direct registers.
        template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void EmitScalarSSEPrefixDirect(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        // Two registers, the one corresponding to R/M is indirect.
        template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
        void EmitScalarSSEPrefixIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm);

        // Variants for emitting scalar SSE instructions.

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void ScalarSSE(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void ScalarSSE(Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset);

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void ScalarSSE(Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src);

        // SSEx66 methods emit SSE instructions that are encoded as
        // [0x66] 0F OPCODE, where 0x66 prefix is present only if the source is
        // a double. Used mostly but not exclusively for instructions operating
        // on packed (f. ex. MovAPS/PD, XorPS/PD). Note that this is not the
        // exclusive use of this prefix as there are instructions which use the
        // 0x66 prefix to specify whether the arguments are MMX or XMM registers.

        // Generic method for emitting the 0x66 prefix depending on the register
        // type and size.
        template <unsigned RMSIZE, bool RMISFLOAT,
                  unsigned REGSIZE, bool REGISFLOAT,
                  unsigned RMREGSIZE, bool RMREGISFLOAT>
        void EmitSSEx66Prefix(Register<REGSIZE, REGISFLOAT> reg, Register<RMREGSIZE, RMREGISFLOAT> rm);

        // Two direct registers.
        template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void EmitSSEx66PrefixDirect(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        // Two registers, the one corresponding to R/M is indirect.
        template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
        void EmitSSEx66PrefixIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm);

        // Variants for emitting the SSEx66 instructions.

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void SSEx66(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void SSEx66(Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset);

        template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void SSEx66(Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src);

        // Group 1/2 instructions.

        template <unsigned SIZE>
        void Group1(uint8_t baseOpCode,
                    Register<SIZE, false> dest,
                    Register<SIZE, false> src);

        template <unsigned SIZE>
        void Group1(uint8_t baseOpCode,
                    Register<SIZE, false> dest,
                    Register<8, false> src,
                    int32_t srcOffset);

        template <unsigned SIZE>
        void Group1(uint8_t baseOpCode,
                    Register<SIZE, false> dest,
                    Register<8, false> base,
                    Register<8, false> index,
                    SIB scale,
                    int32_t srcOffset);

        template <unsigned SIZE>
        void Group1(uint8_t baseOpCode,
                    Register<8, false> dest,
                    int32_t destOffset,
                    Register<SIZE, false> src);

        template <unsigned SIZE>
        void Group1(uint8_t baseOpCode,
                    Register<8, false> base,
                    Register<8, false> index,
                    SIB scale,
                    int32_t offset,
                    Register<SIZE, false> src);


        template <unsigned SIZE, typename T>
        void Group1(uint8_t baseOpCode,
                    uint8_t extensionOpCode,
                    Register<SIZE, false> dest,
                    T value);

        template <unsigned SIZE>
        void Group2(uint8_t extensionOpCode,
                    Register<SIZE, false> dest);

        template <unsigned SIZE>
        void Group2(uint8_t extensionOpCode,
                    uint8_t shift,
                    Register<SIZE, false> dest);

        template <unsigned SIZE>
        void Group3And5(uint8_t opcode,
                        uint8_t extensionOpCode,
                        Register<SIZE, false> dest);

        template <unsigned SIZE>
        void Group3And5(uint8_t opcode,
                        uint8_t extensionOpCode,
                        Register<8u, false> base,
                        int32_t offset);

        template <unsigned SIZE>
        void GroupBitOps(uint8_t opcode,
                         Register<SIZE, false> dest,
                         Register<SIZE, false> bit);


        // Methods for emitting the 0x66 operand size override prefix if
        // size of either operand is 16-bit. Note: for indirect addressing, the
        // size of the operand is the size of the memory being accessed, not the
        // size of the indirect base register.
        // Reference: http://wiki.osdev.org/X86-64_Instruction_Encoding#Operand-size_and_address-size_override_prefix

        // Generic version. When used with indirect addressing, the first two
        // template parameters describe the target memory size and type whereas
        // the last two describe the base register. For regular addressing,
        // these two pairs of template parameters have to be the same.
        template <unsigned RMSIZE, bool RMISFLOAT,
                  unsigned REGSIZE, bool REGISFLOAT,
                  unsigned RMREGSIZE, bool RMREGISFLOAT>
        void EmitOpSizeOverride(Register<REGSIZE, REGISFLOAT> reg,
                                Register<RMREGSIZE, RMREGISFLOAT> rm);

        // Single direct register.
        template <unsigned SIZE, bool ISFLOAT>
        void EmitOpSizeOverride(Register<SIZE, ISFLOAT> reg);

        // Two direct registers.
        template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void EmitOpSizeOverrideDirect(Register<SIZE1, ISFLOAT1> dest,
                                      Register<SIZE2, ISFLOAT2> src);

        // Single indirect register.
        template <unsigned RMSIZE, bool RMISFLOAT>
        void EmitOpSizeOverrideIndirect(Register<8, false> rm);

        // Two registers, the one corresponding to R/M is indirect.
        template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
        void EmitOpSizeOverrideIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm);

        // Methods for emitting the REX byte. For indirect addressing, the
        // size of the operand is the size of the memory being accessed, not the
        // size of the indirect base register.
        // Reference: http://wiki.osdev.org/X86-64_Instruction_Encoding#REX_prefix

        // Generic version. See the comment for EmitOpSizeOverride() for more information.
        template <unsigned RMSIZE, bool RMISFLOAT,
                  unsigned REGSIZE, bool REGISFLOAT,
                  unsigned RMREGSIZE, bool RMREGISFLOAT>
        void EmitRex(Register<REGSIZE, REGISFLOAT> reg, Register<RMREGSIZE, RMREGISFLOAT> rm);

        // Single direct register.
        template <unsigned SIZE, bool ISFLOAT>
        void EmitRex(Register<SIZE, ISFLOAT> reg);

        // Two direct registers.
        template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void EmitRexDirect(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        // Single indirect register.
        template <unsigned RMSIZE, bool RMISFLOAT>
        void EmitRexIndirect(Register<8, false> rm);

        // Two registers, the one corresponding to R/M is indirect.
        template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
        void EmitRexIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm);

        // Methods for emitting the ModR/M byte.
        // Reference: http://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM

        template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
        void EmitModRM(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

        template <unsigned SIZE>
        void EmitModRM(uint8_t extensionOpCode, Register<SIZE, false> dest);

        template <unsigned SIZE, bool ISFLOAT>
        void EmitModRMOffset(Register<SIZE, ISFLOAT> dest, Register<8, false> src, int32_t srcOffset);

        // Helper class used to provide partial specializations by OpCode,
        // ISFLOAT and SIZE for the Emit() methods.
        template <OpCode OP>
        struct Helper
        {
            static void Emit(X64CodeGenerator& code);

            struct ArgTypes0
            {
                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code,
                                 Register<SIZE, false> dest);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code,
                                 Register<8u, false> dest,
                                 int32_t offset);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code,
                                 Register<SIZE, false> dest,
                                 Register<SIZE, false> bit);
            };

            // Emit methods where operands have the same type and size. This is
            // another layer used to provide partial specialization for the
            // Emit() methods.
            template <bool ISFLOAT>
            struct ArgTypes1
            {
                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code, Register<SIZE, ISFLOAT> dest);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code, Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code, Register<SIZE, ISFLOAT> dest, Register<8, false> src, int32_t srcOffset);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code,
                                 Register<SIZE, ISFLOAT> dest,
                                 Register<8, false> base,
                                 Register<8, false> index,
                                 SIB scale,
                                 int32_t srcOffset);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code, Register<8, false> dest, int32_t destOffset, Register<SIZE, ISFLOAT> src);

                template <unsigned SIZE>
                static void Emit(X64CodeGenerator& code,
                                 Register<8, false> base,
                                 Register<8, false> index,
                                 SIB scale,
                                 int32_t srcOffset,
                                 Register<SIZE, ISFLOAT> src);

                template <unsigned SIZE, typename T>
                static void EmitImmediate(X64CodeGenerator& code, Register<SIZE, ISFLOAT> dest, T value);

                template <unsigned SIZE, typename T>
                static void EmitImmediate(X64CodeGenerator& code, Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src, T value);
            };


            // Emit methods where operands don't have the same type and size.
            template <bool ISFLOAT1, bool ISFLOAT2>
            struct ArgTypes2
            {
                template <unsigned SIZE1, unsigned SIZE2>
                static void Emit(X64CodeGenerator& code, Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

                template <unsigned SIZE1, unsigned SIZE2>
                static void Emit(X64CodeGenerator& code, Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset);

                template <unsigned SIZE1, unsigned SIZE2>
                static void Emit(X64CodeGenerator& code, Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src);
            };
        };


        // CodePrinter is a helper class for formatting X64CodeGenerator opcodes
        // and operands along with the resulting assembly encoding of the
        // instruction into a diagnostics stream, if diagnostics output is enabled.
        //
        // The client is expected to 1. call NoteStartPosition() before emitting
        // the instructions into the X64CodeGenerator and 2. call the relevant
        // Print() method after emitting them.
        //
        class CodePrinter : public NonCopyable
        {
        public:
            // Calls NoteStartPosition() and stores the output diagnostics stream.
            CodePrinter(X64CodeGenerator& code);

            void NoteStartPosition();

            void PlaceLabel(Label label);

            // The Print() methods will display the last remembered starting
            // point of the code buffer, all the encoded bytes from
            // the starting point to the end of the buffer followed by the
            // X64CodeGenerator opcodes and operands.

            void PrintJump(void *function);
            void PrintJump(Label label);

            template <JccType JCC>
            void Print(Label l);

            void Print(OpCode op);

            template <unsigned SIZE>
            void Print(OpCode op, Register<8u, false> base, int32_t offset);

            template <unsigned SIZE, bool ISFLOAT>
            void Print(OpCode op, Register<SIZE, ISFLOAT> dest);

            template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
            void Print(OpCode op, Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src);

            template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
            void Print(OpCode op, Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset);

            template <unsigned SIZE1, bool ISFLOAT1>
            void Print(OpCode op,
                       Register<SIZE1, ISFLOAT1> dest,
                       Register<8u, false> base,
                       Register<8u, false> index,
                       SIB scale,
                       int32_t offset);

            template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
            void Print(OpCode op, Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src);

            template <unsigned SIZE1, bool ISFLOAT1>
            void Print(OpCode op,
                       Register<8u, false> base,
                       Register<8u, false> index,
                       SIB scale,
                       int32_t offset,
                       Register<SIZE1, ISFLOAT1> src);

            template <unsigned SIZE, bool ISFLOAT, typename T>
            void PrintImmediate(OpCode op, Register<SIZE, ISFLOAT> dest, T value);

            template <unsigned SIZE, bool ISFLOAT, typename T>
            void PrintImmediate(OpCode op, Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src, T value);

        private:
            X64CodeGenerator& m_code;
            unsigned m_startPosition;
            std::ostream* m_out;

            // Returns "byte" for 1, "word" for 2 etc.
            static char const * GetPointerName(unsigned pointerSize);

            template <typename T>
            static void PrintImmediate(std::ostream& out, T value);

            void PrintBytes(unsigned startPosition, unsigned endPosition);

            // A functor which implements the abs() operation for integral types.
            // CodePrinter prints immediates in hex so the negative values need
            // to have the sign printed explicitly in front of the absolute value.
            template <typename T, bool ISSIGNED = std::is_signed<T>::value>
            struct IntegralAbs
            {
                T operator()(T value);
            };


            // Specialization for non-signed types.
            template <typename T>
            struct IntegralAbs<T, false>
            {
                T operator()(T value);
            };
        };

        std::ostream* m_diagnosticsStream;
    };


    // TODO: Move this class to a separate header
    //
    // Stores the current value of stream's flags, width and fill character in
    // the constructor and restores them back in the destructor. The saving/restoring
    // of these properties is a close subset of what std::ios::copyfmt does, but
    // is more lightweight (no locale, invocation of callbacks etc).
    class IosMiniStateRestorer final : private NonCopyable
    {
    public:
        IosMiniStateRestorer(std::ios& stream);
        ~IosMiniStateRestorer();

    private:
        std::ios& m_stream;
        std::ios::fmtflags m_flags;
        std::streamsize m_width;
        char m_fillChar;
    };


    //*************************************************************************
    //
    // Helper code printing methods.
    //
    //*************************************************************************

    template <JccType JCC>
    void X64CodeGenerator::CodePrinter::Print(Label label)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << JccName(JCC) << " L" << label.GetId() << std::endl;
        }
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::CodePrinter::Print(OpCode op, Register<SIZE, ISFLOAT> dest)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op) << ' ' << dest.GetName() << std::endl;
        }
    }


    template <unsigned SIZE>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<8u, false> base,
                                              int32_t offset)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out
                << OpCodeName(op)
                << ' '
                << GetPointerName(SIZE)
                << " ptr ["
                << base.GetName()
                << std::uppercase
                << std::hex;

            if (offset > 0)
            {
                *m_out << " + " << offset << "h";
            }
            else if (offset < 0)
            {
                *m_out << " - " << -static_cast<int64_t>(offset) << "h";
            }

            *m_out << "]" << std::endl;
        }
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<SIZE1, ISFLOAT1> dest,
                                              Register<SIZE2, ISFLOAT2> src)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out
                << OpCodeName(op)
                << ' ' << dest.GetName()
                << ", " << src.GetName()
                << std::endl;
        }
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<SIZE1, ISFLOAT1> dest,
                                              Register<8, false> src,
                                              int32_t srcOffset)
    {
        if (m_out != nullptr)
        {
            IosMiniStateRestorer state(*m_out);

            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op)
                   << ' ' << dest.GetName()
                   << ", "
                   << GetPointerName(SIZE2)
                   << " ptr ["
                   << src.GetName()
                   << std::uppercase
                   << std::hex;

            if (srcOffset > 0)
            {
                *m_out << " + " << srcOffset << "h";
            }
            else if (srcOffset < 0)
            {
                *m_out << " - " << -static_cast<int64_t>(srcOffset) << "h";
            }

            *m_out << "]"  << std::endl;
        }
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<SIZE, ISFLOAT> dest,
                                              Register<8u, false> base,
                                              Register<8u, false> index,
                                              SIB scale,
                                              int32_t offset)
    {
        if (m_out != nullptr)
        {
            IosMiniStateRestorer state(*m_out);

            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op)
                << ' ' << dest.GetName()
                << ", "
                << GetPointerName(SIZE)
                << " ptr ["
                << base.GetName()
                << " + "
                << index.GetName()
                << " * "
                << static_cast<unsigned>(scale)
                << std::uppercase
                << std::hex;

            if (offset > 0)
            {
                *m_out << " + " << offset << "h";
            }
            else if (offset < 0)
            {
                *m_out << " - " << -static_cast<int64_t>(offset) << "h";
            }

            *m_out << "]" << std::endl;
        }
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<8, false> dest,
                                              int32_t destOffset,
                                              Register<SIZE2, ISFLOAT2> src)
    {
        if (m_out != nullptr)
        {
            IosMiniStateRestorer state(*m_out);

            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op)
                   << ' '
                   << GetPointerName(SIZE1)
                   << " ptr ["
                   << dest.GetName()
                   << std::uppercase
                   << std::hex;

            if (destOffset > 0)
            {
                *m_out << " + " << destOffset << "h";
            }
            else if (destOffset < 0)
            {
                *m_out << " - " << -static_cast<int64_t>(destOffset) << "h";
            }

            *m_out << "], " << src.GetName() << std::endl;
        }
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::CodePrinter::Print(OpCode op,
                                              Register<8u, false> base,
                                              Register<8u, false> index,
                                              SIB scale,
                                              int32_t offset,
                                              Register<SIZE, ISFLOAT> src)
    {
        if (m_out != nullptr)
        {
            IosMiniStateRestorer state(*m_out);

            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op)
                << GetPointerName(SIZE)
                << " ptr ["
                << base.GetName()
                << " + "
                << index.GetName()
                << " * "
                << static_cast<unsigned>(scale)
                << std::uppercase
                << std::hex;

            if (offset > 0)
            {
                *m_out << " + " << offset << "h";
            }
            else if (offset < 0)
            {
                *m_out << " - " << -static_cast<int64_t>(offset) << "h";
            }

            *m_out
                << "], "
                << ' ' << src.GetName()
                << std::endl;
        }
    }


    template <typename T, bool ISSIGNED>
    T X64CodeGenerator::CodePrinter::IntegralAbs<T, ISSIGNED>::operator()(T value)
    {
        return value < 0 ? -value : value;
    }


    // Specialization for non-signed types.
    template <typename T>
    T X64CodeGenerator::CodePrinter::IntegralAbs<T, false>::operator()(T value)
    {
        return value;
    }


    template <typename T>
    void X64CodeGenerator::CodePrinter::PrintImmediate(std::ostream& out, T value)
    {
        static_assert(!std::is_floating_point<T>::value,
                      "Floating point values cannot be used as immediates.");

        const T nonNegativeValue = IntegralAbs<T>()(value);
        const auto prefix = (value != nonNegativeValue) ? "-" : "";

        IosMiniStateRestorer state(out);

        // Unary + causes integral promotion so char would be printed as integer,
        // as expected.
        out
            << std::uppercase
            << std::hex
            << prefix
            << +nonNegativeValue
            << 'h';
    }


    template <unsigned SIZE, bool ISFLOAT, typename T>
    void X64CodeGenerator::CodePrinter::PrintImmediate(OpCode op,
                                                       Register<SIZE, ISFLOAT> dest,
                                                       T value)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op) << ' ' << dest.GetName() << ", ";

            PrintImmediate(*m_out, value);

            *m_out << std::endl;
        }
    }


    template <unsigned SIZE, bool ISFLOAT, typename T>
    void X64CodeGenerator::CodePrinter::PrintImmediate(OpCode op,
                                                       Register<SIZE, ISFLOAT> dest,
                                                       Register<SIZE, ISFLOAT> src,
                                                       T value)
    {
        if (m_out != nullptr)
        {
            PrintBytes(m_startPosition, m_code.CurrentPosition());

            *m_out << OpCodeName(op)
                   << ' ' << dest.GetName()
                   << ", " << src.GetName()
                   << ", ";

            PrintImmediate(*m_out, value);

            *m_out << std::endl;
        }
    }


    //*************************************************************************
    //
    // Template definitions for X64CodeGenerator - public methods.
    //
    //*************************************************************************
    template <JccType JCC>
    void X64CodeGenerator::EmitConditionalJump(Label label)
    {
        CodePrinter printer(*this);

        Emit8(0xf);
        Emit8(0x80 + static_cast<uint8_t>(JCC));
        EmitCallSite(label, 4);

        printer.Print<JCC>(label);
    }


    template <OpCode OP>
    void X64CodeGenerator::Emit()
    {
        CodePrinter printer(*this);

        Helper<OP>::Emit(*this);

        printer.Print(OP);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<SIZE, ISFLOAT> dest)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, dest);

        printer.Print(OP, dest);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, dest, src);

        printer.Print(OP, dest, src);
    }


    template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::Emit(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes2<ISFLOAT1, ISFLOAT2>::template Emit<SIZE1, SIZE2>(*this, dest, src);

        printer.Print(OP, dest, src);
    }


    // Base register and offset.
    template <OpCode OP, unsigned SIZE>
    void X64CodeGenerator::Emit(Register<8u, false> dest, int32_t offset)
    {
        CodePrinter printer(*this);

        Helper<OP>::ArgTypes0::template Emit<SIZE>(*this, dest, offset);

        printer.Print<SIZE>(OP, dest, offset);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<SIZE, ISFLOAT> dest, Register<8, false> src, int32_t srcOffset)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, dest, src, srcOffset);

        printer.Print<SIZE, ISFLOAT, SIZE, ISFLOAT>(OP, dest, src, srcOffset);
    }


    template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::Emit(Register<SIZE1, ISFLOAT1> dest, Register<8, false> src, int32_t srcOffset)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes2<ISFLOAT1, ISFLOAT2>::template Emit<SIZE1, SIZE2>(*this, dest, src, srcOffset);

        printer.Print<SIZE1, ISFLOAT1, SIZE2, ISFLOAT2>(OP, dest, src, srcOffset);
    }


    // Scale-index-base (SIB) + offset.
    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<SIZE, ISFLOAT> dest,
                                Register<8, false> base,
                                Register<8, false> index,
                                SIB scale,
                                int32_t offset)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, dest, base, index, scale, offset);

        printer.Print<SIZE, ISFLOAT>(OP, dest, base, index, scale, offset);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<8, false> dest, int32_t destOffset, Register<SIZE, ISFLOAT> src)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, dest, destOffset, src);

        printer.Print<SIZE, ISFLOAT, SIZE, ISFLOAT>(OP, dest, destOffset, src);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::Emit(Register<8, false> base,
                                Register<8, false> index,
                                SIB scale,
                                int32_t offset,
                                Register<SIZE, ISFLOAT> src)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template Emit<SIZE>(*this, base, index, scale, offset, src);

        printer.Print<SIZE, ISFLOAT>(OP, base, index, scale, offset, src);
    }

    template <OpCode OP, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::Emit(Register<8, false> dest, int32_t destOffset, Register<SIZE2, ISFLOAT2> src)
    {
        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes2<ISFLOAT1, ISFLOAT2>::template Emit<SIZE1, SIZE2>(*this, dest, destOffset, src);

        printer.Print<SIZE1, ISFLOAT1, SIZE2, ISFLOAT2>(OP, dest, destOffset, src);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT, typename T>
    void X64CodeGenerator::EmitImmediate(Register<SIZE, ISFLOAT> dest, T value)
    {
        static_assert(!std::is_floating_point<T>::value, "Floating point values cannot be used as immediates.");

        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template EmitImmediate<SIZE, T>(*this, dest, value);

        printer.PrintImmediate(OP, dest, value);
    }


    template <OpCode OP, unsigned SIZE, bool ISFLOAT, typename T>
    void X64CodeGenerator::EmitImmediate(Register<SIZE, ISFLOAT> dest, Register<SIZE, ISFLOAT> src, T value)
    {
        static_assert(!std::is_floating_point<T>::value, "Floating point values cannot be used as immediates.");

        CodePrinter printer(*this);

        Helper<OP>::template ArgTypes1<ISFLOAT>::template EmitImmediate<SIZE, T>(*this, dest, src, value);

        printer.PrintImmediate(OP, dest, src, value);
    }


    //*************************************************************************
    //
    // Template definitions for X64CodeGenerator - private methods.
    //
    //*************************************************************************

    //
    // X64 opcodes
    //
    template <unsigned SIZE>
    void X64CodeGenerator::IMul(Register<SIZE, false> dest,
                                Register<SIZE, false> src)
    {
        EmitOpSizeOverrideDirect(dest, src);
        EmitRexDirect(dest, src);
        Emit8(0x0f);
        Emit8(0xAF);
        EmitModRM(dest, src);
    }


    template <unsigned SIZE>
    void X64CodeGenerator::IMul(Register<SIZE, false> dest,
                                Register<8, false> src,
                                int32_t srcOffset)
    {
        EmitOpSizeOverrideIndirect<SIZE, false>(dest, src);
        EmitRexIndirect<SIZE, false>(dest, src);
        Emit8(0x0f);
        Emit8(0xAF);
        EmitModRMOffset(dest, src, srcOffset);
    }


    template <unsigned SIZE, typename T>
    void X64CodeGenerator::IMulImmediate(Register<SIZE, false> dest,
                                         T value)
    {
        static_assert(SIZE != 1, "8-bit target not supported.");
        static_assert(sizeof(T) <= SIZE, "Invalid size of the immediate.");
        static_assert(sizeof(T) < 8, "64-bit immediates are not supported by IMul.");
        static_assert(std::is_integral<T>::value, "IMul works only with integral values.");

        const unsigned valueSize = Size(value);

        EmitOpSizeOverrideDirect(dest, dest);
        EmitRexDirect(dest, dest);

        if (valueSize <= 1
            && (static_cast<int64_t>(value) < 0
                || !BitOp::TestBit(static_cast<uint64_t>(value), 7)))
        {
            // Use the sign-extend flavor of the opcode only if the value is
            // signed and negative or if the bit #7 is not set otherwise.
            Emit8(0x6b);
            EmitModRM(dest, dest);
            Emit8(static_cast<uint8_t>(value));
        }
        else
        {
            Emit8(0x69);
            EmitModRM(dest, dest);

            if (SIZE == 2)
            {
                Emit16(static_cast<uint16_t>(value));
            }
            else // if (valueSize <= 4)
            {
                Emit32(static_cast<uint32_t>(value));
            }
        }
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Lea(Register<SIZE, false> dest,
                               Register<8, false> src,
                               int32_t srcOffset)
    {
        EmitRexIndirect<SIZE, false>(dest, src);
        Emit8(0x8d);
        EmitModRMOffset(dest, src, srcOffset);
    }


    template <unsigned SIZE, typename T>
    void X64CodeGenerator::MovImmediate(Register<SIZE, false> dest,
                                        T value)
    {
        static_assert(!std::is_floating_point<T>::value, "Invalid type of the immediate.");
        static_assert(sizeof(T) <= SIZE, "Invalid size of the immediate.");

        EmitOpSizeOverride(dest);
        EmitRex(dest);

        if (SIZE == 1)
        {
            Emit8(0xb0 | dest.GetId8());
            Emit8(static_cast<uint8_t>(value));
        }
        else if (SIZE == 8
                 && Size(value) <= 4
                 && ((std::is_signed<T>::value && static_cast<int64_t>(value) < 0)
                     || !BitOp::TestBit(static_cast<uint64_t>(value), 31)))
        {
            // Use the sign-extend flavor of the opcode only if the value is
            // signed and negative or if the bit #31 is not set otherwise.
            Emit8(0xc7);
            EmitModRM(0, dest);
            Emit32(static_cast<uint32_t>(value));
        }
        else
        {
            Emit8(0xb8 + dest.GetId8());
            if (SIZE == 2)
            {
                Emit16(static_cast<uint16_t>(value));
            }
            else if (SIZE == 4)
            {
                Emit32(static_cast<uint32_t>(value));
            }
            else
            {
                Emit64(static_cast<uint64_t>(value));
            }
        }
    }


    template <typename T>
    void X64CodeGenerator::MovImmediate(Register<8, false> dest,
                                        T* value)
    {
        MovImmediate(dest, reinterpret_cast<uint64_t>(value));
    }


    template <unsigned SIZE>
    void X64CodeGenerator::MovD(Register<SIZE, true> dest, Register<SIZE, false> src)
    {
        Emit8(0x66);
        EmitRexDirect(dest, src);
        Emit8(0x0f);
        Emit8(0x6e);
        EmitModRM(dest, src);
    }


    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::MovZX(Register<SIZE1, false> dest, Register<SIZE2, false> src)
    {
        static_assert(SIZE2 == 1 || SIZE2 == 2, "Invalid source size.");
        static_assert(SIZE1 > SIZE2, "Target size must be larger than the source size.");

        const bool twoByteSource = SIZE2 == 2;

        // Size override is not necessary for 16-bit source since an opcode that
        // defaults to 16 bits is used in that scenario.
        //
        if (!twoByteSource)
        {
            EmitOpSizeOverrideDirect(dest, src);
        }
        EmitRexDirect(dest, src);
        Emit8(0x0f);
        Emit8(twoByteSource ? 0xb7 : 0xb6);
        EmitModRM(dest, src);
    }


    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::MovZX(Register<SIZE1, false> dest, Register<8, false> src, int32_t srcOffset)
    {
        static_assert(SIZE2 == 1 || SIZE2 == 2, "Invalid source size.");
        static_assert(SIZE1 > SIZE2, "Target size must be larger than the source size.");

        const bool twoByteSource = SIZE2 == 2;

        if (!twoByteSource)
        {
            EmitOpSizeOverrideIndirect<SIZE2, false>(dest, src);
        }
        EmitRexIndirect<SIZE2, false>(dest, src);
        Emit8(0x0f);
        Emit8(twoByteSource ? 0xb7 : 0xb6);
        EmitModRMOffset(dest, src, srcOffset);
    }


    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::MovSX(Register<SIZE1, false> dest, Register<SIZE2, false> src)
    {
        static_assert(SIZE2 < 8, "Invalid source size.");
        static_assert(SIZE1 > SIZE2, "Target size must be larger than the source size.");

        if (SIZE2 == 1)
        {
            EmitOpSizeOverrideDirect(dest, src);
            EmitRexDirect(dest, src);
            Emit8(0x0f);
            Emit8(0xbe);
        }
        else if (SIZE2 == 2)
        {
            // No size override since 16-bit is default operand size; different opcode.
            EmitRexDirect(dest, src);
            Emit8(0x0f);
            Emit8(0xbf);
        }
        else // if (SIZE2 == 4)
        {
            // No size override since neither operand can be 16-bit.
            // No prefix, different opcode.
            EmitRexDirect(dest, src);
            Emit8(0x63);
        }

        EmitModRM(dest, src);
    }


    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::MovSX(Register<SIZE1, false> dest, Register<8, false> src, int32_t srcOffset)
    {
        static_assert(SIZE2 < 8, "Invalid source size.");
        static_assert(SIZE1 > SIZE2, "Target size must be larger than the source size.");

        if (SIZE2 == 1)
        {
            EmitOpSizeOverrideIndirect<SIZE2, false>(dest, src);
            EmitRexIndirect<SIZE2, false>(dest, src);
            Emit8(0x0f);
            Emit8(0xbe);
        }
        else if (SIZE2 == 2)
        {
            // No size override since 16-bit is default operand size; different opcode.
            EmitRexIndirect<SIZE2, false>(dest, src);
            Emit8(0x0f);
            Emit8(0xbf);
        }
        else // if (SIZE2 == 4)
        {
            // No size override since neither operand can be 16-bit.
            // No prefix, different opcode.
            EmitRexIndirect<SIZE2, false>(dest, src);
            Emit8(0x63);
        }

        EmitModRMOffset(dest, src, srcOffset);
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Shld(Register<SIZE, false> dest, Register<SIZE, false> src, uint8_t bitCount)
    {
        // Note: operand encoding is MRC, so the order of arguments for the
        // Emit*() methods is reversed.
        EmitOpSizeOverrideDirect(src, dest);
        EmitRexDirect(src, dest);
        Emit8(0x0f);
        Emit8(0xa4);
        EmitModRM(src, dest);
        Emit8(bitCount);
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Shld(Register<SIZE, false> dest, Register<SIZE, false> src)
    {
        // Note: operand encoding is MRC, so the order of arguments for the
        // Emit*() methods is reversed.
        EmitOpSizeOverrideDirect(src, dest);
        EmitRexDirect(src, dest);
        Emit8(0x0f);
        Emit8(0xa5);
        EmitModRM(src, dest);
    }


    //
    // Scalar SSE instructions
    //

    template <unsigned RMSIZE, bool RMISFLOAT,
              unsigned REGSIZE, bool REGISFLOAT,
              unsigned RMREGSIZE, bool RMREGISFLOAT>
    void X64CodeGenerator::EmitScalarSSEPrefix(Register<REGSIZE, REGISFLOAT> /* reg */,
                                               Register<RMREGSIZE, RMREGISFLOAT> /* rm */)
    {
        static_assert(RMISFLOAT || REGISFLOAT, "Invalid ScalarSSE usage.");
        static_assert(RMSIZE >= 4 && REGSIZE >= 4, "Invalid ScalarSSE integer register."); // Floats would error out before this point.

        // Use the source register to decide on the prefix, but only if it's a float.
        if (RMISFLOAT)
        {
            Emit8(RMSIZE == 8 ? 0xf2 : 0xf3);
        }
        else
        {
            Emit8(REGSIZE == 8 ? 0xf2 : 0xf3);
        }
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::EmitScalarSSEPrefixDirect(Register<SIZE1, ISFLOAT1> dest,
                                                     Register<SIZE2, ISFLOAT2> src)
    {
        EmitScalarSSEPrefix<SIZE2, ISFLOAT2>(dest, src);
    }


    template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
    void X64CodeGenerator::EmitScalarSSEPrefixIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm)
    {
        EmitScalarSSEPrefix<RMSIZE, RMISFLOAT>(reg, rm);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::ScalarSSE(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src)
    {
        EmitScalarSSEPrefixDirect(dest, src);
        EmitRexDirect(dest, src);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRM(dest, src);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::ScalarSSE(Register<SIZE1, ISFLOAT1> dest,
                                     Register<8, false> src,
                                     int32_t srcOffset)
    {
        EmitScalarSSEPrefixIndirect<SIZE2, ISFLOAT2>(dest, src);
        EmitRexIndirect<SIZE2, ISFLOAT2>(dest, src);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRMOffset(dest, src, srcOffset);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::ScalarSSE(Register<8, false> dest,
                                     int32_t destOffset,
                                     Register<SIZE2, ISFLOAT2> src)
    {
        // Note: operand encoding is MR, so the order of arguments for the
        // Emit*() methods is reversed.
        EmitScalarSSEPrefixIndirect<SIZE1, ISFLOAT1>(src, dest);
        EmitRexIndirect<SIZE1, ISFLOAT1>(src, dest);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRMOffset(src, dest, destOffset);
    }


    //
    // SSEx66 instructions
    //

    template <unsigned RMSIZE, bool RMISFLOAT,
              unsigned REGSIZE, bool REGISFLOAT,
              unsigned RMREGSIZE, bool RMREGISFLOAT>
    void X64CodeGenerator::EmitSSEx66Prefix(Register<REGSIZE, REGISFLOAT> /* reg */,
                                            Register<RMREGSIZE, RMREGISFLOAT> /* rm */)
    {
        static_assert(RMISFLOAT || REGISFLOAT, "Invalid SSEx66 usage.");
        static_assert(RMSIZE >= 4 && REGSIZE >= 4, "Invalid ScalarSSE integer register."); // Floats would error out before this point.

        // Use the source size to decide on the prefix if it's a float.
        // Otherwise, the target is a float so check its size.
        if ((RMISFLOAT && RMSIZE == 8)
            || (!RMISFLOAT && REGSIZE == 8))
        {
            Emit8(0x66);
        }
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::EmitSSEx66PrefixDirect(Register<SIZE1, ISFLOAT1> dest,
                                                  Register<SIZE2, ISFLOAT2> src)
    {
        EmitSSEx66Prefix<SIZE2, ISFLOAT2>(dest, src);
    }


    template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
    void X64CodeGenerator::EmitSSEx66PrefixIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm)
    {
        EmitSSEx66Prefix<RMSIZE, RMISFLOAT>(reg, rm);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::SSEx66(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src)
    {
        EmitSSEx66PrefixDirect(dest, src);
        EmitRexDirect(dest, src);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRM(dest, src);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::SSEx66(Register<SIZE1, ISFLOAT1> dest,
                                  Register<8, false> src,
                                  int32_t srcOffset)
    {
        EmitSSEx66PrefixIndirect<SIZE2, ISFLOAT2>(dest, src);
        EmitRexIndirect<SIZE2, ISFLOAT2>(dest, src);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRMOffset(dest, src, srcOffset);
    }


    template <uint8_t OPCODE, unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::SSEx66(Register<8, false> dest,
                                  int32_t destOffset,
                                  Register<SIZE2, ISFLOAT2> src)
    {
        // Note: operand encoding is MR, so the order of arguments for the
        // Emit*() methods is reversed.
        EmitSSEx66PrefixIndirect<SIZE1, ISFLOAT1>(src, dest);
        EmitRexIndirect<SIZE1, ISFLOAT1>(src, dest);
        Emit8(0x0f);
        Emit8(OPCODE);
        EmitModRMOffset(src, dest, destOffset);
    }


    //
    // X64 group1 opcodes
    //

    template <unsigned SIZE>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  Register<SIZE, false> dest,
                                  Register<SIZE, false> src)
    {
        EmitOpSizeOverrideDirect(dest, src);
        EmitRexDirect(dest, src);
        if (SIZE == 1)
        {
            Emit8(baseOpCode + 0x2);
        }
        else
        {
            Emit8(baseOpCode + 0x3);
        }
        EmitModRM(dest, src);
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  Register<SIZE, false> dest,
                                  Register<8, false> src,
                                  int32_t srcOffset)
    {
        EmitOpSizeOverrideIndirect<SIZE, false>(dest, src);
        EmitRexIndirect<SIZE, false>(dest, src);
        if (SIZE == 1)
        {
            Emit8(baseOpCode + 0x2);
        }
        else
        {
            Emit8(baseOpCode + 0x3);
        }
        EmitModRMOffset(dest, src, srcOffset);
    }


    inline uint8_t Mod(int32_t offset)
    {
        if (offset == 0)
        {
            return 0;
        }
        else if (offset >= -0x80 && offset <= 0x7f)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  Register<SIZE, false> dest,
                                  Register<8, false> base,
                                  Register<8, false> index,
                                  SIB scale,
                                  int32_t offset)
    {
        EmitOpSizeOverrideIndirect<SIZE, false>(dest, base);

        //
        // Emit rex
        //
        const bool w = (SIZE == 8);
        Emit8(0x40
              | (w ? 8 : 0)
              | (dest.IsExtended() ? 4 : 0)
              | (index.IsExtended() ? 2 : 0)
              | (base.IsExtended() ? 1 : 0));

        //
        // Emit opcode
        //

        if (SIZE == 1)
        {
            Emit8(baseOpCode + 0x2);
        }
        else
        {
            Emit8(baseOpCode + 0x3);
        }

        //
        // Emit mod/rm
        //

        uint8_t mod = Mod(offset);
        const uint8_t baseField = base.GetId8();
        const uint8_t destField = dest.GetId8();
        const uint8_t indexField = index.GetId8();

        Emit8((mod << 6) | (destField << 3) | 4);

        //
        // Emit SIB
        //
        const uint8_t s = static_cast<uint8_t>(scale);
        Emit8((s << 6) | (indexField << 3) | baseField);

        //
        // Emit offset
        //

        if (mod == 1)
        {
            Emit8(static_cast<uint8_t>(offset));
        }
        else if (mod == 2)
        {
            Emit32(offset);
        }
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  Register<8, false> dest,
                                  int32_t destOffset,
                                  Register<SIZE, false> src)
    {
        // Note: operand encoding is MR, so the order of arguments for the
        // Emit*() methods is reversed.
        EmitOpSizeOverrideIndirect<SIZE, false>(src, dest);
        EmitRexIndirect<SIZE, false>(src, dest);
        if (SIZE == 1)
        {
            Emit8(baseOpCode);
        }
        else
        {
            Emit8(baseOpCode + 0x1);
        }
        EmitModRMOffset(src, dest, destOffset);
    }


    template <unsigned SIZE>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  Register<8, false> base,
                                  Register<8, false> index,
                                  SIB scale,
                                  int32_t offset,
                                  Register<SIZE, false> src)
    {
        EmitOpSizeOverrideIndirect<SIZE, false>(src, base);

        //
        // Emit rex
        //
        const bool w = (SIZE == 8);
        Emit8(0x40
              | (w ? 8 : 0)
              | (src.IsExtended() ? 4 : 0)
              | (index.IsExtended() ? 2 : 0)
              | (base.IsExtended() ? 1 : 0));

        //
        // Emit opcode
        //

        if (SIZE == 1)
        {
            Emit8(baseOpCode);
        }
        else
        {
            Emit8(baseOpCode + 0x1);
        }

        //
        // Emit mod/rm
        //

        uint8_t mod = Mod(offset);
        const uint8_t baseField = base.GetId8();
        const uint8_t destField = src.GetId8();
        const uint8_t indexField = index.GetId8();

        Emit8((mod << 6) | (destField << 3) | 4);

        //
        // Emit SIB
        //
        const uint8_t s = static_cast<uint8_t>(scale);
        Emit8((s << 6) | (indexField << 3) | baseField);

        //
        // Emit offset
        //

        if (mod == 1)
        {
            Emit8(static_cast<uint8_t>(offset));
        }
        else if (mod == 2)
        {
            Emit32(offset);
        }
    }


    template <unsigned SIZE, typename T>
    void X64CodeGenerator::Group1(uint8_t baseOpCode,
                                  uint8_t extensionOpCode,
                                  Register<SIZE, false> dest,
                                  T value)
    {
        static_assert(std::is_integral<T>::value, "Group1 opcodes work only with integral values.");
        static_assert(sizeof(T) <= SIZE, "Invalid size of the immediate.");
        static_assert(sizeof(T) < 8, "Group1 instructions don't support 64-bit immediates.");
        static_assert(!(SIZE == 8 && sizeof(T) == 4 && std::is_unsigned<T>::value),
                      "Cannot safely use 32-bit unsigned immediate with 64-bit register, "
                      "sign extension would have been used.");

        unsigned valueSize = Size(value);

        EmitOpSizeOverride(dest);

        if (dest.GetId() == 0 && SIZE == 1)
        {
            // Special case for AL.
            Emit8(baseOpCode + 0x04);
            Emit8(static_cast<uint8_t>(value));
        }
        else if (dest.GetId() == 0 && valueSize != 1)
        {
            EmitRex(dest);
            Emit8(baseOpCode + 0x05);

            if (SIZE == 2)
            {
                Emit16(static_cast<uint16_t>(value));
            }
            else
            {
                // Note: in case of 64-bit register (RAX) and 32-bit immediate,
                // the opcode used above will sign-extend the immediate. This is
                // OK since the earlier static assert verifies that if a 32-bit
                // immediate is used in such case, its type must be signed.
                // Any sign-extension will thus be intended in this case.
                Emit32(static_cast<uint32_t>(value));
            }
        }
        else
        {
            EmitRex(dest);

            // For an immediate fitting in 1-byte, use the sign-extend flavor of
            // the opcode only if it's irrelevant (target is also 1 byte) or if
            // it is OK to do so (the value is signed and negative or the bit
            // #7 is clear).
            if (valueSize <= 1
                && (SIZE == 1
                    || (static_cast<int64_t>(value) < 0
                        || !BitOp::TestBit(static_cast<uint64_t>(value), 7))))
            {
                if (SIZE == 1)
                {
                    Emit8(0x80);
                }
                else
                {
                    Emit8(0x83);
                }
                EmitModRM(extensionOpCode, dest);
                Emit8(static_cast<uint8_t>(value));
            }
            else // if (valueSize <= 4)
            {
                Emit8(0x81);
                EmitModRM(extensionOpCode, dest);

                if (SIZE == 2)
                {
                    Emit16(static_cast<uint16_t>(value));
                }
                else
                {
                    // This will sign-extend the immediate if 64-bit register
                    // is used. See the comment inside the topmost else branch
                    // for RAX for details on why is that OK.
                    Emit32(static_cast<uint32_t>(value));
                }
            }
        }
    }


    //
    // X64 group2 opcodes
    //

    template <unsigned SIZE>
    void X64CodeGenerator::Group2(uint8_t extensionOpCode,
                                  Register<SIZE, false> dest)
    {
        EmitOpSizeOverride(dest);
        EmitRex(dest);
        if (SIZE == 1)
        {
            Emit8(0xd2);
        }
        else
        {
            Emit8(0xd3);
        }
        EmitModRM(extensionOpCode, dest);
    }


    // Coalesce with previous by calling previous before emitting shift.
    template <unsigned SIZE>
    void X64CodeGenerator::Group2(uint8_t extensionOpCode,
                                  uint8_t shift,
                                  Register<SIZE, false> dest)
    {
        EmitOpSizeOverride(dest);
        EmitRex(dest);
        if (SIZE == 1)
        {
            Emit8(0xc0);
        }
        else
        {
            Emit8(0xc1);
        }
        EmitModRM(extensionOpCode, dest);
        Emit8(shift);
    }


    //
    // X64 group 5 opcodes.
    //

    template <unsigned SIZE>
    void X64CodeGenerator::Group3And5(uint8_t opcode,
                                      uint8_t extensionOpCode,
                                      Register<SIZE, false> dest)
    {
        EmitOpSizeOverride(dest);
        EmitRex(dest);
        if (SIZE == 1)
        {
            Emit8(opcode);
        }
        else
        {
            Emit8(opcode + 1u);
        }
        EmitModRM(extensionOpCode, dest);
    }

    template <unsigned SIZE>
    void X64CodeGenerator::Group3And5(uint8_t opcode,
                                      uint8_t extensionOpCode,
                                    Register<8u, false> dest,
                                    int32_t offset)
    {
        EmitOpSizeOverride(Register<SIZE, false>(0));
        EmitRex<SIZE, false>(Register<SIZE, false>(0), dest);
        if (SIZE == 1)
        {
            Emit8(opcode);
        }
        else
        {
            Emit8(opcode + 1u);
        }
        EmitModRMOffset(Register<8u, false>(extensionOpCode), dest, offset);
    }


    //
    // Bit operation opcodes.
    //
    template <unsigned SIZE>
    void X64CodeGenerator::GroupBitOps(uint8_t opcode,
                                       Register<SIZE, false> dest,
                                       Register<SIZE, false> bit)
    {
        EmitOpSizeOverride(Register<SIZE, false>(0));
        if (opcode == 0xbc || opcode == 0xbd)
        {
            EmitRex<SIZE, false>(dest, bit);
            Emit8(0x0f);
            Emit8(opcode);
            EmitModRM(dest, bit);
        }
        else
        {
            EmitRex<SIZE, false>(bit, dest);
            Emit8(0x0f);
            Emit8(opcode);
            EmitModRM(bit, dest);
        }
    }


    //
    // X64 opcode encoding - operand size override.
    //

    template <unsigned RMSIZE, bool RMISFLOAT,
              unsigned REGSIZE, bool REGISFLOAT,
              unsigned RMREGSIZE, bool RMREGISFLOAT>
    void X64CodeGenerator::EmitOpSizeOverride(Register<REGSIZE, REGISFLOAT> /* reg */,
                                              Register<RMREGSIZE, RMREGISFLOAT> /* rm */)
    {
        static_assert((RMSIZE == RMREGSIZE && RMISFLOAT == RMREGISFLOAT)
                      || (RMREGSIZE == 8 && !RMREGISFLOAT),
                      "Only direct addressing or indirect addresing with 64-bit "
                      "general purpose base register can be used.");

        // Emit operand size override prefix if necessary.
        // Note: address size override prefix (0x67) is not considered since
        // we don't support 32-bit indirects.
        if (RMSIZE == 2 || REGSIZE == 2)
        {
            Emit8(0x66);
        }
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::EmitOpSizeOverride(Register<SIZE, ISFLOAT> reg)
    {
        EmitOpSizeOverride<SIZE, ISFLOAT>(reg, reg);
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::EmitOpSizeOverrideDirect(Register<SIZE1, ISFLOAT1> dest,
                                                    Register<SIZE2, ISFLOAT2> src)
    {
        EmitOpSizeOverride<SIZE2, ISFLOAT2>(dest, src);
    }


    template <unsigned RMSIZE, bool RMISFLOAT>
    void X64CodeGenerator::EmitOpSizeOverrideIndirect(Register<8, false> rm)
    {
        EmitOpSizeOverride<RMSIZE, RMISFLOAT>(Register<RMSIZE, RMISFLOAT>(), rm);
    }


    template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
    void X64CodeGenerator::EmitOpSizeOverrideIndirect(Register<REGSIZE, REGISFLOAT> reg,
                                                      Register<8, false> rm)
    {
        EmitOpSizeOverride<RMSIZE, RMISFLOAT>(reg, rm);
    }


    //
    // X64 opcode encoding - REX.
    //

    template <unsigned RMSIZE, bool RMISFLOAT,
              unsigned REGSIZE, bool REGISFLOAT,
              unsigned RMREGSIZE, bool RMREGISFLOAT>
    void X64CodeGenerator::EmitRex(Register<REGSIZE, REGISFLOAT> reg,
                                   Register<RMREGSIZE, RMREGISFLOAT> rm)
    {
        static_assert((RMSIZE == RMREGSIZE && RMISFLOAT == RMREGISFLOAT)
                      || (RMREGSIZE == 8 && !RMREGISFLOAT),
                      "Only direct addressing or indirect addresing with 64-bit "
                      "general purpose base register can be used.");

        // This code currently doesn't support the REX.X bit.
        // REX.X is an extra bit for the SIB index field. SIB is scale index
        // base (think `lea`). We don't currently support this addressing
        // mode. The old assembler might support SIB?
        //
        // Note that the REX.W bit is never set when two floating point operands
        // are used.
        //
        // In the indirect mode, the size and type of the R/M operand are
        // determined by the size and type of the target memory area (given
        // that the base register is always a general purpose register and size
        // in our case always 8).
        const bool w = (REGSIZE == 8 && !REGISFLOAT) || (RMSIZE == 8 && !RMISFLOAT);

        // When spl, bpl, sil or dil are used, at least an empty REX byte must
        // be emitted. Otherwise, these registers would be treated as ah, ch, dh
        // and bh respectively.
        const bool forceRex = (reg == spl || reg == bpl || reg == sil || reg == dil)
                              || (rm == spl || rm == bpl || rm == sil || rm == dil);

        if (forceRex || w || reg.IsExtended() || rm.IsExtended())
        {
            // WRXB
            Emit8(0x40
                  | (w ? 8 : 0)
                  | (reg.IsExtended() ? 4 : 0)
                  | (rm.IsExtended() ? 1 : 0));
        }
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::EmitRex(Register<SIZE, ISFLOAT> reg)
    {
        // Use eax for the other register since it will not affect any of the REX flags.
        EmitRex<SIZE, ISFLOAT>(eax, reg);
    }


    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::EmitRexDirect(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src)
    {
        EmitRex<SIZE2, ISFLOAT2>(dest, src);
    }


    template <unsigned RMSIZE, bool RMISFLOAT>
    void X64CodeGenerator::EmitRexIndirect(Register<8, false> rm)
    {
        // Use eax for the other register since it will not affect any of the REX flags.
        EmitRex<RMSIZE, RMISFLOAT>(eax, rm);
    }


    template <unsigned RMSIZE, bool RMISFLOAT, unsigned REGSIZE, bool REGISFLOAT>
    void X64CodeGenerator::EmitRexIndirect(Register<REGSIZE, REGISFLOAT> reg, Register<8, false> rm)
    {
        EmitRex<RMSIZE, RMISFLOAT>(reg, rm);
    }


    //
    // X64 opcode encoding - ModR/M.
    //

    template <unsigned SIZE1, bool ISFLOAT1, unsigned SIZE2, bool ISFLOAT2>
    void X64CodeGenerator::EmitModRM(Register<SIZE1, ISFLOAT1> dest, Register<SIZE2, ISFLOAT2> src)
    {
        Emit8(0xc0 | (dest.GetId8() << 3) | src.GetId8());
        // BUGBUG: check special cases for RSP, R12. Shouldn't be necessary here if
        // this function is only used for Register-Register encoding. Problem will
        // crop up if caller passes the base register from an X64Indirect.
    }


    template <unsigned SIZE>
    void X64CodeGenerator::EmitModRM(uint8_t extensionOpCode, Register<SIZE, false> dest)
    {
        Emit8(0xc0 | (extensionOpCode << 3) | dest.GetId8());
        // BUGBUG: check special cases for RSP, R12. Shouldn't be necessary here if
        // this function is only used for Register-Register encoding. Problem will
        // crop up if caller passes the base register from an X64Indirect.
    }


    template <unsigned SIZE, bool ISFLOAT>
    void X64CodeGenerator::EmitModRMOffset(Register<SIZE, ISFLOAT> reg,
                                           Register<8, false> rm,
                                           int32_t offset)
    {
        if (rm.IsRIP())
        {
            // RIP-relative addressing. Hard-code mod = 0 and rmField = 5.
            const uint8_t mod = 0;
            const uint8_t rmField = 5;
            const uint8_t regField = reg.GetId8();

            Emit8((mod << 6) | (regField << 3) | rmField);

            Emit32(offset - CurrentPosition() - 4);
        }
        else
        {
            // Normal, GPR-indirect addressing.
            uint8_t mod = Mod(offset);
            const uint8_t rmField = rm.GetId8();
            const uint8_t regField = reg.GetId8();

            if (rmField == 5 && mod == 0)
            {
                // The combination of rmField == 5 && mod == 0 is a special case
                // which is used for RIP-relative addressing. Convert to mod 01
                // and emit an 8-bit displacement of 0.
                mod = 1;
            }

            Emit8((mod << 6) | (regField << 3) | rmField);

            if (rmField == 4)
            {
                // When rm is RSP or R12 or XMM4 or XMM12, rmField == 4, which
                // is a special case used for SIB addressing. Emit an SIB byte
                // which encodes the same register with no scaled index.
                // Want SS = 00, Index = 100 (none), and Base = 100 (4).
                Emit8(0x24);
            }

            if (mod == 1)
            {
                Emit8(static_cast<uint8_t>(offset));
            }
            else if (mod == 2)
            {
                Emit32(offset);
            }
        }
    }


    //*************************************************************************
    //
    // X64CodeGenerator::Helper definitions for each opcode and addressing mode.
    //
    //*************************************************************************

    //
    // Bit operations
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Bsf>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xbc, dest, bit);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Bsr>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xbd, dest, bit);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Bt>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xa3, dest, bit);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Btc>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xbb, dest, bit);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Btr>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xb3, dest, bit);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Bts>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> bit)
    {
        code.GroupBitOps(0xab, dest, bit);
    }


    //
    // Dec
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Dec>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest)
    {
        code.Group3And5(0xfe, 1, dest);
    }


    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Dec>::ArgTypes0::Emit(
        X64CodeGenerator& code,
        Register<8u, false> base,
        int32_t offset)
    {
        code.Group3And5<SIZE>(0xfe, 1, base, offset);
    }


    //
    // Inc
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Inc>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest)
    {
        code.Group3And5(0xfe, 0, dest);
    }


    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Inc>::ArgTypes0::Emit(
        X64CodeGenerator& code,
        Register<8u, false> base,
        int32_t offset)
    {
        code.Group3And5<SIZE>(0xfe, 0, base, offset);
    }


    //
    // Neg
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Neg>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest)
    {
        code.Group3And5(0xf6, 3, dest);
    }


    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Neg>::ArgTypes0::Emit(
        X64CodeGenerator& code,
        Register<8u, false> base,
        int32_t offset)
    {
        code.Group3And5<SIZE>(0xf6, 3, base, offset);
    }


    //
    // Not
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Not>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest)
    {
        code.Group3And5(0xf6, 2, dest);
    }


    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Not>::ArgTypes0::Emit(
        X64CodeGenerator& code,
        Register<8u, false> base,
        int32_t offset)
    {
        code.Group3And5<SIZE>(0xf6, 2, base, offset);
    }


    //
    // Lea
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Lea>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        code.Lea(dest, src, srcOffset);
    }


    //
    // Mov
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> src)
    {
        code.Group1(0x88, dest, src);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        code.Group1(0x88, dest, src, srcOffset);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<8, false> base,
        Register<8, false> index,
        SIB scale,
        int32_t srcOffset)
    {
        code.Group1(0x88, dest, base, index, scale, srcOffset);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<8, false> dest,
        int32_t destOffset,
        Register<SIZE, false> src)
    {
        code.Group1(0x88, dest, destOffset, src);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<8, false> base,
        Register<8, false> index,
        SIB scale,
        int32_t offset,
        Register<SIZE, false> src)
    {
        code.Group1(0x88, base, index, scale, offset, src);
    }


    template <>
    template <>
    template <unsigned SIZE, typename T>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<false>::EmitImmediate(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        T value)
    {
        code.MovImmediate(dest, value);
    }


    template <>
    template <>
    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes2<true, false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE1, true> dest,
        Register<SIZE2, false> src)
    {
        code.MovD(dest, src);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Mov>::ArgTypes1<true>::Emit(
        X64CodeGenerator& code,
        Register<8, false> dest,
        int32_t destOffset,
        Register<SIZE, true> src)
    {
        // MovSS/SD.
        code.ScalarSSE<0x11, SIZE, true, SIZE, true>(dest, destOffset, src);
    }


    //
    // Mul
    //

    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::IMul>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> src)
    {
        code.IMul(dest, src);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::IMul>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        code.IMul(dest, src, srcOffset);
    }


    template <>
    template <>
    template <unsigned SIZE, typename T>
    void X64CodeGenerator::Helper<OpCode::IMul>::ArgTypes1<false>::EmitImmediate(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        T value)
    {
        code.IMulImmediate(dest, value);
    }


    //
    // MovSX
    //

    template <>
    template <>
    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::Helper<OpCode::MovSX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE1, false> dest,
        Register<SIZE2, false> src)
    {
        code.MovSX(dest, src);
    }


    template <>
    template <>
    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::Helper<OpCode::MovSX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE1, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        code.MovSX<SIZE1, SIZE2>(dest, src, srcOffset);
    }


    //
    // MovZX
    //

    template <>
    template <>
    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE1, false> dest,
        Register<SIZE2, false> src)
    {
        code.MovZX(dest, src);
    }

    // There is no "movzx dest64, src32" instruction, MovZX opcode for this
    // combination is implemented in terms of "mov dest32, src32" using full
    // template specialization.
    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit<8, 4>(
        X64CodeGenerator& code,
        Register<8, false> dest,
        Register<4, false> src);


    template <>
    template <>
    template <unsigned SIZE1, unsigned SIZE2>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE1, false> dest,
        Register<8, false> src,
        int32_t srcOffset)
    {
        code.MovZX<SIZE1, SIZE2>(dest, src, srcOffset);
    }


    template <>
    template <>
    template <>
    void X64CodeGenerator::Helper<OpCode::MovZX>::ArgTypes2<false, false>::Emit<8, 4>(
        X64CodeGenerator& code,
        Register<8, false> dest,
        Register<8, false> src,
        int32_t srcOffset);


    //
    // Shld
    //

    template <>
    template <>
    template <unsigned SIZE, typename T>
    void X64CodeGenerator::Helper<OpCode::Shld>::ArgTypes1<false>::EmitImmediate(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> src,
        T bitCount)
    {
        code.Shld(dest, src, bitCount);
    }


    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::Shld>::ArgTypes1<false>::Emit(
        X64CodeGenerator& code,
        Register<SIZE, false> dest,
        Register<SIZE, false> src)
    {
        code.Shld(dest, src);
    }


#define DEFINE_GROUP1(name, baseOpCode, extensionOpCode) \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest,                                                             \
        Register<SIZE, false> src)                                                              \
    {                                                                                           \
        code.Group1(baseOpCode, dest, src);                                                     \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest,                                                             \
        Register<8, false> src,                                                                 \
        int32_t srcOffset)                                                                      \
    {                                                                                           \
        code.Group1(baseOpCode, dest, src, srcOffset);                                          \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest,                                                             \
        Register<8, false> base,                                                                \
        Register<8, false> index,                                                               \
        SIB scale,                                                                              \
        int32_t srcOffset)                                                                      \
    {                                                                                           \
        code.Group1(baseOpCode, dest, base, index, scale, srcOffset);                           \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<8, false> dest,                                                                \
        int32_t destOffset,                                                                     \
        Register<SIZE, false> src)                                                              \
    {                                                                                           \
        code.Group1(baseOpCode, dest, destOffset, src);                                         \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<8, false> base,                                                                \
        Register<8, false> index,                                                               \
        SIB scale,                                                                              \
        int32_t offset,                                                                         \
        Register<SIZE, false> src)                                                              \
    {                                                                                           \
        code.Group1(baseOpCode, base, index, scale, offset, src);                               \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE, typename T>                                                        \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::EmitImmediate(               \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest,                                                             \
        T value)                                                                                \
    {                                                                                           \
        code.Group1(baseOpCode, extensionOpCode, dest, value);                                  \
    }

    DEFINE_GROUP1(Add, 0, 0);
    DEFINE_GROUP1(And, 0x20, 4);
    DEFINE_GROUP1(Cmp, 0x38, 7);
    DEFINE_GROUP1(Or, 8, 1);
    DEFINE_GROUP1(Sub, 0x28, 5);
    DEFINE_GROUP1(Xor, 0x30, 6);

#undef DEFINE_GROUP1


#define DEFINE_GROUP2(name, extensionOpCode)                                                    \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE>                                                                    \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::Emit(                        \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest)                                                             \
    {                                                                                           \
        code.Group2(extensionOpCode, dest);                                                     \
    }                                                                                           \
                                                                                                \
                                                                                                \
    template <>                                                                                 \
    template <>                                                                                 \
    template <unsigned SIZE, typename T>                                                        \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<false>::EmitImmediate(               \
        X64CodeGenerator& code,                                                                 \
        Register<SIZE, false> dest,                                                             \
        T value)                                                                                \
    {                                                                                           \
        code.Group2(extensionOpCode, value, dest);                                              \
    }

    DEFINE_GROUP2(Rol, 0);
    DEFINE_GROUP2(Shl, 4);
    DEFINE_GROUP2(Shr, 5);

#undef DEFINE_GROUP2


// SSE instruction, both arguments of the same type and size.
#define DEFINE_SSE_ARGS1(name, emitMethod, opcode) \
    template <>                                                                         \
    template <>                                                                         \
    template <unsigned SIZE>                                                            \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<true>::Emit(                 \
        X64CodeGenerator& code,                                                         \
        Register<SIZE, true> dest,                                                      \
        Register<SIZE, true> src)                                                       \
    {                                                                                   \
        code.emitMethod<opcode>(dest, src);                                             \
    }                                                                                   \
                                                                                        \
                                                                                        \
    template <>                                                                         \
    template <>                                                                         \
    template <unsigned SIZE>                                                            \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes1<true>::Emit(                 \
        X64CodeGenerator& code,                                                         \
        Register<SIZE, true> dest,                                                      \
        Register<8, false> src,                                                         \
        int32_t srcOffset)                                                              \
    {                                                                                   \
        code.emitMethod<opcode, SIZE, true, SIZE, true>(dest, src, srcOffset);          \
    }                                                                                   \

    DEFINE_SSE_ARGS1(Add,            ScalarSSE, 0x58);  // AddSS/AddSD.
    DEFINE_SSE_ARGS1(Cmp,            SSEx66,    0x2f);  // ComISS/ComISD.
    DEFINE_SSE_ARGS1(IMul,           ScalarSSE, 0x59);  // MulSS/MulSD.
    DEFINE_SSE_ARGS1(Mov,            ScalarSSE, 0x10);  // MovSS/MovSD.
    DEFINE_SSE_ARGS1(MovAP,          SSEx66,    0x28);  // MovAPS/MovAPD.
    DEFINE_SSE_ARGS1(Sub,            ScalarSSE, 0x5c);  // SubSS/SubSD.

#undef DEFINE_SSE_ARGS1

    // Unlike others, MovAPS/MovAPD also have the "mov [rxx + 16], xmm" form
    // of the instruction with a different opcode.
    template <>
    template <>
    template <unsigned SIZE>
    void X64CodeGenerator::Helper<OpCode::MovAP>::ArgTypes1<true>::Emit(
        X64CodeGenerator& code,
        Register<8, false> dest,
        int32_t destOffset,
        Register<SIZE, true> src)
    {
        code.SSEx66<0x29, SIZE, true, SIZE, true>(dest, destOffset, src);
    }


// SSE instruction, arguments of different type or size.
#define DEFINE_SSE_ARGS2(name, emitMethod, opcode, type1, type2, validityCondition)     \
    template <>                                                                         \
    template <>                                                                         \
    template <unsigned SIZE1, unsigned SIZE2>                                           \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes2<type1, type2>::Emit(         \
        X64CodeGenerator& code,                                                         \
        Register<SIZE1, type1> dest,                                                    \
        Register<SIZE2, type2> src)                                                     \
    {                                                                                   \
        static_assert(validityCondition,                                                \
                      "Invalid " #name " instruction, must be " #validityCondition);    \
        code.emitMethod<opcode, SIZE1, type1, SIZE2, type2>(dest, src);                 \
    }                                                                                   \
                                                                                        \
                                                                                        \
    template <>                                                                         \
    template <>                                                                         \
    template <unsigned SIZE1, unsigned SIZE2>                                           \
    void X64CodeGenerator::Helper<OpCode::name>::ArgTypes2<type1, type2>::Emit(         \
        X64CodeGenerator& code,                                                         \
        Register<SIZE1, type1> dest,                                                    \
        Register<8, false> src,                                                         \
        int32_t srcOffset)                                                              \
    {                                                                                   \
        static_assert(validityCondition,                                                \
                      "Invalid " #name " instruction, must be " #validityCondition);    \
        code.emitMethod<opcode, SIZE1, type1, SIZE2, type2>(dest, src, srcOffset);      \
    }                                                                                   \

    DEFINE_SSE_ARGS2(CvtSI2FP, ScalarSSE, 0x2A, true,  false, SIZE2 >= 4);       // CvtSI2SD/CvtSI2SS (convert signed int to floating point).
    DEFINE_SSE_ARGS2(CvtFP2SI, ScalarSSE, 0x2C, false, true,  SIZE1 >= 4);       // CvtTSD2SI/CvtTSS2SI (convert floating point to signed int with truncation).
    DEFINE_SSE_ARGS2(CvtFP2FP, ScalarSSE, 0x5A, true,  true,  SIZE1 != SIZE2);   // CvtSS2SD/CvtSD2SS (convert float to double and vice versa).

#undef DEFINE_SCALAR_SSE2
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
