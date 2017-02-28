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


namespace NativeJIT
{
    namespace CallingConvention
    {
#ifdef NATIVEJIT_PLATFORM_WINDOWS
        // Register masks of volatile integer and floating point registers.
        // TODO: This could be replaced with constexpr: add operator| to Register and use it to define the masks.
        // Note: RIP is included in volatiles.
        static const unsigned c_rxxVolatileRegistersMask = 0xf07;     // 1 0000 1111 0000 0111 (RAX | RCX | RDX | R8 | R9 | R10 | R11 | RIP)
        static const unsigned c_xmmVolatileRegistersMask = 0x3f;      //   0000 0000 0011 1111 (XMM0-XMM5)

        // Register masks of non-volatile integer and floating point registers.
        // Note: RIP not included.
        static const unsigned c_rxxNonVolatileRegistersMask = 0xf0f8; // 0 1111 0000 1111 1000 (RBX | RSP | RBP | RSI | RDI | R12-R15)
        static const unsigned c_xmmNonVolatileRegistersMask = 0xffc0; //   1111 1111 1100 0000 (XMM6-XMM15)

        // Register masks of registers that can be written to.
        static const unsigned c_rxxWritableRegistersMask = 0xFFFF;    // Everything except RIP.
        static const unsigned c_xmmWritableRegistersMask = 0xFFFF;    // All XMM registers.
#else
        // Register masks of volatile integer and floating point registers.
        // TODO: This could be replaced with constexpr: add operator| to Register and use it to define the masks.
        // Note: RIP is included in volatiles.
        static const unsigned c_rxxVolatileRegistersMask = 0xfc7;     // 1 0000 1111 1100 0111 (RAX | RCX | RDX | RSI | RDI | R8 | R9 | R10 | R11 | RIP)
        static const unsigned c_xmmVolatileRegistersMask = 0xffff;    //   1111 1111 1111 1111 (XMM0-XMM15)

        // Register masks of non-volatile integer and floating point registers.
        // Note: RIP not included.
        static const unsigned c_rxxNonVolatileRegistersMask = 0xf038; // 0 1111 0000 0011 1000 (RBX | RSP | RBP | R12-R15)
        static const unsigned c_xmmNonVolatileRegistersMask = 0x0000; //   0000 0000 0000 0000 (none)

        // Register masks of registers that can be written to.
        static const unsigned c_rxxWritableRegistersMask = 0xFFFF;    // Everything except RIP.
        static const unsigned c_xmmWritableRegistersMask = 0xFFFF;    // All XMM registers.
#endif

    static_assert((c_rxxNonVolatileRegistersMask ^ c_rxxVolatileRegistersMask) == 0xffff,
                  "Each register should appear exactly once in calling convention mask");
    static_assert((c_xmmNonVolatileRegistersMask ^ c_xmmVolatileRegistersMask) == 0xffff,
                  "Each register should appear exactly once in calling convention mask");
    }
}
