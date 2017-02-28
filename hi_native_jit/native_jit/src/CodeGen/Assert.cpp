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


#include <cstdarg>
#include <stdexcept>
#include <sstream>
#include <vector>

#include "../../inc/Temporary/Assert.h"


namespace NativeJIT
{
    void LogThrowImpl(char const * filename,
                      char const * function,
                      unsigned lineNumber,
                      char const * title,
                      char const * format,
                      ...)
    {
        const unsigned messageBufferSize = 1024;
        char message[messageBufferSize];

        va_list arguments;
        va_start(arguments, format);

#ifdef _MSC_VER
        vsnprintf_s(message, _TRUNCATE, format, arguments);
#else
        vsnprintf(&message[0], messageBufferSize, format, arguments);
#endif

        va_end(arguments);

        std::stringstream sstream;
        sstream << "Assertion " << title << " failed in " << function
                << " at " << filename << ":" << lineNumber << ": " << message;

        throw std::runtime_error(sstream.str().c_str());
    }
}
