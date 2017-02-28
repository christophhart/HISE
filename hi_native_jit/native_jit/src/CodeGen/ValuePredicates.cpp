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


#include "../../inc/NativeJIT/CodeGen/ValuePredicates.h"


namespace NativeJIT
{
    unsigned Size(uint8_t value)
    {
        return (value == 0) ? 0 : 1;
    }


    unsigned Size(uint16_t value)
    {
        return (value == 0) ? 0 : ((value <= 0xff) ? 1 : 2);
    }


    unsigned Size(uint32_t value)
    {
        return (value == 0) ? 0 : ((value <= 0xff) ? 1 : ((value <= 0xffff) ? 2 : 4));
    }


    unsigned Size(uint64_t value)
    {
        return (value == 0) ? 0 : ((value <= 0xff)? 1 : ((value <= 0xffff) ? 2 : ((value <= 0xffffffff) ? 4 : 8)));
    }


    unsigned Size(int8_t value)
    {
        return (value == 0) ? 0 : 1;
    }


    unsigned Size(int16_t value)
    {
        return (value == 0) ? 0 : ((value >= -0x80 && value <= 0x7f)? 1 : 2);
    }


    unsigned Size(int32_t value)
    {
        return (value == 0)? 0 : ((value >= -0x80 && value <= 0x7f)? 1 : ((value >= -0x8000 && value <= 0x7fff) ? 2 : 4));
    }


    unsigned Size(int64_t value)
    {
        return (value == 0) ? 0 : ((value >= -0x80LL && value <= 0x7fLL)? 1 : ((value >= -0x8000LL && value <= 0x7fffLL) ? 2 : ((value >= -0x80000000LL && value <= 0x7fffffffLL) ? 4 : 8)));
    }
}
