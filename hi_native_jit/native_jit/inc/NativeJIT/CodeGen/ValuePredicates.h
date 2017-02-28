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
#include <type_traits>

#include "../TypeConverter.h"


namespace NativeJIT
{
    // Function overloads returns the byte sizes of various types.
    // Used by X64CodeGenerator::Group1() template definition.
    unsigned Size(uint8_t);
    unsigned Size(uint16_t value);
    unsigned Size(uint32_t value);
    unsigned Size(uint64_t value);
    unsigned Size(int8_t);
    unsigned Size(int16_t value);
    unsigned Size(int32_t value);
    unsigned Size(int64_t value);


    // Cast using a static_cast for convertible immediates.
    template <typename TO, typename FROM>
    TO ForcedCast(FROM from,
                  typename std::enable_if<std::is_convertible<FROM, TO>::value>::type* = nullptr)
    {
        return static_cast<TO>(from);
    }


    // Cast using a reinterpret_cast for non-convertible immediates of the
    // same size.
    template <typename TO, typename FROM>
    TO ForcedCast(FROM from,
                  typename std::enable_if<!std::is_convertible<FROM, TO>::value>::type* = nullptr)
    {
        static_assert(sizeof(FROM) == sizeof(TO),
                      "Cannot do a forced cast between incompatible types of different sizes");

        return convertType<FROM, TO>(from);
    }
}
