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

#include "CodeGen/Register.h"

namespace NativeJIT
{
    // A trait used to determine whether a type has any decay other than
    // const/volatile.
    template <typename T>
    struct HasMeaningfulDecay
        : std::integral_constant<
            bool,
            !std::is_same<typename std::remove_cv<T>::type,
                          typename std::decay<T>::type>
                        ::value>
    {
    };


    // Declares the properties of the type used for register storage.
    template <typename T>
    struct RegisterStorage
    {
        // The underlying type stored inside the register when passing the
        // storage as an argument to a function. F. ex. references and large
        // structures are passed as pointers. Any const/volatile qualifiers are
        // removed.
        // WARNING: Representation of a reference is implementation-specific.
        // However, (almost?) all compilers implement it as a pointer.
        typedef typename std::conditional<
            std::is_reference<T>::value
                || (sizeof(T) > RegisterBase::c_maxSize),
            typename std::add_pointer<typename std::remove_cv<T>::type>::type,
            typename std::conditional<
                HasMeaningfulDecay<T>::value,
                typename std::decay<T>::type,
                typename std::remove_cv<T>::type
            >::type
        >::type UnderlyingType;

        static const unsigned c_size = sizeof(UnderlyingType);
        static const bool c_isFloat = std::is_floating_point<UnderlyingType>::value;

        typedef Register<c_size, c_isFloat> RegisterType;
    };

    // In the context of immediates that are emitted as parts of x64 instructions,
    // there are three categories of types:
    //
    // 1. NonImmediate types cannot be immediates at all (void).
    // 2. RIPRelativeImmediate types must be represented as RIP-relative indirects
    //    because there are no x64 instructions that support them as immediates
    //    (floating point values) or most x64 instructions don't support them as
    //    immediates (quadwords, including function pointers, things that decay
    //    to pointers such as arrays etc).
    // 3. InlineImmediate types can be used directly as immediates in x64 instructions.
    enum class ImmediateCategory { NonImmediate, RIPRelativeImmediate, InlineImmediate };

    // Classifies types that belong to the RIPRelativeImmediate category.
    template <typename T>
    struct MustBeEncodedAsRIPRelative
        : std::integral_constant<bool,
                                 RegisterStorage<T>::c_isFloat
                                 || RegisterStorage<T>::c_size == 8
          >
    {
    };

    template <>
    struct MustBeEncodedAsRIPRelative<void> : std::false_type {};


    // Classifies types that belong to the InlineImmediate category in the list above.
    template <typename T>
    struct CanBeInImmediateStorage
        : std::integral_constant<
            bool,
            std::is_same<typename RegisterStorage<T>::UnderlyingType,
                         typename std::remove_cv<T>::type>::value
                && !MustBeEncodedAsRIPRelative<T>::value
          >
    {
    };

    template <>
    struct CanBeInImmediateStorage<void> : std::false_type {};


    // Classifies types by ImmediateCategory.
    template <typename T>
    struct ImmediateCategoryOf
        : std::integral_constant<ImmediateCategory,
                                 CanBeInImmediateStorage<T>::value
                                    ? ImmediateCategory::InlineImmediate
                                    : (MustBeEncodedAsRIPRelative<T>::value
                                        ? ImmediateCategory::RIPRelativeImmediate
                                        : ImmediateCategory::NonImmediate)
          >
    {
    };


    // Determines the canonical C++ type that maps to a register type.
    template <typename T>
    struct CanonicalRegisterType;

    template <> struct CanonicalRegisterType<Register<1, false>> { typedef uint8_t Type; };
    template <> struct CanonicalRegisterType<Register<2, false>> { typedef uint16_t Type; };
    template <> struct CanonicalRegisterType<Register<4, false>> { typedef uint32_t Type; };
    template <> struct CanonicalRegisterType<Register<8, false>> { typedef uint64_t Type; };

    template <> struct CanonicalRegisterType<Register<4, true>> { typedef float Type; };
    template <> struct CanonicalRegisterType<Register<8, true>> { typedef double Type; };


    // Determines the canonical C++ type that maps to a register storage needed
    // for a type. F. ex. void*, a function pointer etc. map to uint64_t.
    template <typename T>
    struct CanonicalRegisterStorageType
    {
        typedef typename CanonicalRegisterType<
            typename RegisterStorage<T>::RegisterType
        >::Type Type;
    };


    // Specifies whether a type is a valid parameter/return type for a NativeJIT function.
    template <typename T>
    struct IsValidParameter
    {
        static const bool c_value
            = std::is_arithmetic<T>::value
              || std::is_pointer<T>::value
              || std::is_reference<T>::value
              || (std::is_pod<T>::value
                  && sizeof(T) <= RegisterBase::c_maxSize);
    };
}
