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

#include <type_traits>


namespace NativeJIT
{
    template <typename PACKED>
    class Model
    {
    public:
        typedef PACKED PackedType;

        Model();

        float Apply(PACKED packed) const;

        float& operator[](unsigned index);
        float const & operator[](unsigned index) const;

        float& operator[](PACKED packed);
        float const & operator[](PACKED packed) const;

        // m_data must be public or friend of NativeJIT::ExpressionNodeFactory.
        // Otherwise the JIT compiler cannot access m_data.

        static const unsigned c_size = 1 << PACKED::c_totalBitCount;
        float m_data[c_size];
    };


    //*************************************************************************
    //
    // Template definitions for Model<PACKED>
    //
    //*************************************************************************
    template <typename PACKED>
    Model<PACKED>::Model()
        : m_data()
    {
    }


    template <typename PACKED>
    float Model<PACKED>::Apply(PACKED packed) const
    {
        return m_data[packed.m_bits];
    }


    template <typename PACKED>
    float& Model<PACKED>::operator[](unsigned index)
    {
        return m_data[index];
    }


    template <typename PACKED>
    float const & Model<PACKED>::operator[](unsigned index) const
    {
        return m_data[index];
    }


    template <typename PACKED>
    float& Model<PACKED>::operator[](PACKED packed)
    {
        return m_data[packed.m_bits];
    }


    template <typename PACKED>
    float const & Model<PACKED>::operator[](PACKED packed) const
    {
        return m_data[packed.m_bits];
    }
}
