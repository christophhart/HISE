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

// MSVC has __pragma directive for use in macros since #pragma would apply to the macro itself.
#ifdef _MSC_VER
#define PREPROCESSOR_PRAGMA_WARNING(x) __pragma(warning(x))
#else
#define PREPROCESSOR_PRAGMA_WARNING(x)
#endif


#define LogThrowAssert(condition, ...)        \
do                                            \
{                                             \
    if (!(condition))                         \
    {                                         \
        NativeJIT::LogThrowImpl(__FILE__,     \
                                __FUNCTION__, \
                                __LINE__,     \
                                #condition,   \
                                __VA_ARGS__); \
    }                                         \
PREPROCESSOR_PRAGMA_WARNING(push)             \
PREPROCESSOR_PRAGMA_WARNING(disable:4127)     \
} while (false)                               \
PREPROCESSOR_PRAGMA_WARNING(pop)


#define LogThrowAbort(...)                    \
    NativeJIT::LogThrowImpl(__FILE__,         \
                            __FUNCTION__,     \
                            __LINE__,         \
                            "Forced failure", \
                            __VA_ARGS__)


namespace NativeJIT
{
    void LogThrowImpl(char const * filename,
                      char const * function,
                      unsigned lineNumber,
                      char const * title,
                      char const * format,
                      ...);
}
