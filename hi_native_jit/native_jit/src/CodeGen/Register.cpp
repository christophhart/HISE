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


namespace NativeJIT
{
    unsigned RegisterBase::c_sizes[c_maxSize + 1] = {
        0,
        0,  // 1 bytes
        1,  // 2 bytes
        0,
        2,  // 4 bytes
        0,
        0,
        0,
        3   // 8 bytes
    };


    // TODO: These names are not in the same order as the declarations below.
    char const * RegisterBase::c_names[c_typesCount][c_validSizesCount][c_maxRegisterID + 1] =
    {
        {
            { "al", "cl", "dl", "bl", "spl", "bpl", "dil", "sil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" },
            { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" },
            { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" },
            { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rip" },
        },
        {
            {},
            {},
            { "xmm0s", "xmm1s", "xmm2s", "xmm3s", "xmm4s", "xmm5s", "xmm6s", "xmm7s",
              "xmm8s", "xmm9s", "xmm10s", "xmm11s", "xmm12s", "xmm13s", "xmm14s", "xmm15s"},
            { "xmm0d", "xmm1d", "xmm2d", "xmm3d", "xmm4d", "xmm5d", "xmm6d", "xmm7d",
              "xmm8d", "xmm9d", "xmm10d", "xmm11d", "xmm12d", "xmm13d", "xmm14d", "xmm15d"}
        }
    };


    Register<1, false> al(0);
    Register<1, false> cl(1);
    Register<1, false> dl(2);
    Register<1, false> bl(3);
    Register<1, false> spl(4);
    Register<1, false> bpl(5);
    Register<1, false> sil(6);
    Register<1, false> dil(7);
    Register<1, false> r8b(8);
    Register<1, false> r9b(9);
    Register<1, false> r10b(10);
    Register<1, false> r11b(11);
    Register<1, false> r12b(12);
    Register<1, false> r13b(13);
    Register<1, false> r14b(14);
    Register<1, false> r15b(15);


    Register<2, false> ax(0);
    Register<2, false> cx(1);
    Register<2, false> dx(2);
    Register<2, false> bx(3);
    Register<2, false> sp(4);
    Register<2, false> bp(5);
    Register<2, false> si(6);
    Register<2, false> di(7);
    Register<2, false> r8w(8);
    Register<2, false> r9w(9);
    Register<2, false> r10w(10);
    Register<2, false> r11w(11);
    Register<2, false> r12w(12);
    Register<2, false> r13w(13);
    Register<2, false> r14w(14);
    Register<2, false> r15w(15);


    Register<4, false> eax(0);
    Register<4, false> ecx(1);
    Register<4, false> edx(2);
    Register<4, false> ebx(3);
    Register<4, false> esp(4);
    Register<4, false> ebp(5);
    Register<4, false> esi(6);
    Register<4, false> edi(7);
    Register<4, false> r8d(8);
    Register<4, false> r9d(9);
    Register<4, false> r10d(10);
    Register<4, false> r11d(11);
    Register<4, false> r12d(12);
    Register<4, false> r13d(13);
    Register<4, false> r14d(14);
    Register<4, false> r15d(15);


    Register<8, false> rax(0);
    Register<8, false> rcx(1);
    Register<8, false> rdx(2);
    Register<8, false> rbx(3);
    Register<8, false> rsp(4);
    Register<8, false> rbp(5);
    Register<8, false> rsi(6);
    Register<8, false> rdi(7);
    Register<8, false> r8(8);
    Register<8, false> r9(9);
    Register<8, false> r10(10);
    Register<8, false> r11(11);
    Register<8, false> r12(12);
    Register<8, false> r13(13);
    Register<8, false> r14(14);
    Register<8, false> r15(15);
    Register<8, false> rip(16);


    Register<4, true> xmm0s(0);
    Register<4, true> xmm1s(1);
    Register<4, true> xmm2s(2);
    Register<4, true> xmm3s(3);
    Register<4, true> xmm4s(4);
    Register<4, true> xmm5s(5);
    Register<4, true> xmm6s(6);
    Register<4, true> xmm7s(7);
    Register<4, true> xmm8s(8);
    Register<4, true> xmm9s(9);
    Register<4, true> xmm10s(10);
    Register<4, true> xmm11s(11);
    Register<4, true> xmm12s(12);
    Register<4, true> xmm13s(13);
    Register<4, true> xmm14s(14);
    Register<4, true> xmm15s(15);


    Register<8, true> xmm0(0);
    Register<8, true> xmm1(1);
    Register<8, true> xmm2(2);
    Register<8, true> xmm3(3);
    Register<8, true> xmm4(4);
    Register<8, true> xmm5(5);
    Register<8, true> xmm6(6);
    Register<8, true> xmm7(7);
    Register<8, true> xmm8(8);
    Register<8, true> xmm9(9);
    Register<8, true> xmm10(10);
    Register<8, true> xmm11(11);
    Register<8, true> xmm12(12);
    Register<8, true> xmm13(13);
    Register<8, true> xmm14(14);
    Register<8, true> xmm15(15);
}
