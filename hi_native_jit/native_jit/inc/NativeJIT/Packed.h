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


namespace NativeJIT
{
    typedef uint32_t PackedUnderlyingType;

    //*************************************************************************
    // DESIGN NOTE
    //
    // The Packed class design makes some accomodations in order to simplify
    // the NativeJIT code. In VS 2012 and VS 2013, the compiler uses a more
    // complex calling convention when invoking a function whose return value
    // is not a POD type as determined by Microsoft's implementation of
    // std::is_pod.
    //
    // When the return type is not a POD, the first parameter of the function
    // will be a pointer to caller-allocated storage for the return value, and
    // the caller will assume that the function will return this pointer in RAX.
    //
    // When the return type is POD, its value is returned directly in RAX.
    //
    // The Microsoft implementation of std::is_pod is more restrictive than
    // the C++11 definition. The Packed class design is impacted in, particular,
    // by Microsoft's restriction on the use of private data members, base
    // classes, and user defined constructors in POD type definitions.
    //
    // A cleaner design of Packed would make m_bits private, and probably
    // move it to a base class common to all of the template specializations.
    // The FromBits()/FromComponents() methods would be replaced by constructors
    // with the same parameter list.
    //
    // Down the road, we may consider implementing support for non-POD
    // return values in NativeJIT, but even if we do this, we may still want
    // to keep Packed as POD for performance reasons.
    //
    //*************************************************************************

    // This class and PackedMinMaxNode should probably move from NativeJIT to BitFunnel.Library.
    // See bug#30.


    // Retrieves the number of bits in the rightmost (last) component of the
    // Packed<> type.
    template <unsigned LEFT, unsigned... RIGHT>
    struct PackedRightmost
    {
        // Defer the answer to the right portion of the parameter pack.
        static const unsigned c_value = PackedRightmost<RIGHT...>::c_value;
    };


    // Base case.
    template <unsigned LEFT>
    struct PackedRightmost<LEFT>
    {
        static const unsigned c_value = LEFT;
    };


    // Packed<> with two or more components.
    template <unsigned LEFT, unsigned... RIGHT>
    struct Packed
    {
        typedef Packed<RIGHT...> Right;

        static const unsigned c_componentCount = 1 + sizeof...(RIGHT);
        static const unsigned c_totalBitCount = LEFT + Right::c_totalBitCount;

        static const unsigned c_leftmostBitCount = LEFT;
        static const unsigned c_rightmostBitCount = PackedRightmost<LEFT, RIGHT...>::c_value;

        static_assert(c_totalBitCount <= sizeof(PackedUnderlyingType) * 8, "Too many bits in the packed.");


        static Packed FromBits(PackedUnderlyingType value)
        {
            return Packed{value};
        }


        // Constructs a packed from its components.
        template <typename T1, typename... ARGS>
        static Packed FromComponents(T1 left, ARGS... args)
        {
            static_assert(1 + sizeof...(args) == c_componentCount,
                          "Invalid number of arguments in the constructor");

            const auto right = Right::FromComponents(args...);

            return right.template InsertLeftmost<LEFT>(left);
        }


        // Creates a new packed by adding an extra component at the left side.
        template <unsigned LEFTMOST>
        Packed<LEFTMOST, LEFT, RIGHT...> InsertLeftmost(PackedUnderlyingType value) const
        {
            const PackedUnderlyingType bits = (value << c_totalBitCount) | m_bits;

            return Packed<LEFTMOST, LEFT, RIGHT...>::FromBits(bits);
        }


        // Creates a new packed by adding an extra component at the right side.
        template <unsigned RIGHTMOST>
        Packed<LEFT, RIGHT..., RIGHTMOST> InsertRightmost(PackedUnderlyingType value) const
        {
            const PackedUnderlyingType bits = (m_bits << RIGHTMOST) | value;

            return Packed<LEFT, RIGHT..., RIGHTMOST>::FromBits(bits);
        }


        // Creates a new packed by removing the leftmost component.
        Right WithoutLeftmost() const
        {
            return Right::FromBits(m_bits & ((1 << Right::c_totalBitCount) - 1));
        }


        // Returns the bits of the leftmost component shifted to the zero bit.
        PackedUnderlyingType Leftmost() const
        {
            return m_bits >> Right::c_totalBitCount;
        }


        // Returns the bits of the rightmost component.
        PackedUnderlyingType Rightmost() const
        {
            return m_bits & ((1 << c_rightmostBitCount) - 1);
        }


        operator PackedUnderlyingType() const
        {
            return m_bits;
        }


        PackedUnderlyingType m_bits;
    };


    // Packed<> with one component.
    template <unsigned LEFT>
    struct Packed<LEFT>
    {
        // Useful for clients to have different overrides if the Right type is void.
        typedef void Right;

        static const unsigned c_componentCount = 1;

        static const unsigned c_totalBitCount = LEFT;
        static const unsigned c_leftmostBitCount = LEFT;
        static const unsigned c_rightmostBitCount = LEFT;

        static_assert(c_totalBitCount <= sizeof(PackedUnderlyingType) * 8, "Too many bits in the packed.");


        static Packed FromBits(PackedUnderlyingType value)
        {
            return Packed{value};
        }


        static Packed FromComponents(PackedUnderlyingType value)
        {
            return Packed{value};
        }


        template <unsigned LEFTMOST>
        Packed<LEFTMOST, LEFT> InsertLeftmost(PackedUnderlyingType value) const
        {
            const PackedUnderlyingType bits = (value << c_totalBitCount) | m_bits;

            return Packed<LEFTMOST, LEFT>::FromBits(bits);
        }


        template <unsigned RIGHTMOST>
        Packed<LEFT, RIGHTMOST> InsertRightmost(PackedUnderlyingType value) const
        {
            const PackedUnderlyingType bits = (m_bits << RIGHTMOST) | value;

            return Packed<LEFT, RIGHTMOST>::FromBits(bits);
        }


        PackedUnderlyingType Leftmost() const
        {
            return m_bits;
        }


        PackedUnderlyingType Rightmost() const
        {
            return m_bits;
        }


        operator PackedUnderlyingType() const
        {
            return m_bits;
        }


        PackedUnderlyingType m_bits;
    };
}
