/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_lac
  vendor:           Christoph Hart
  version:          1.1
  name:             HISE Lossless Audio Codec
  description:      A fast, lossless audio codec suitable for disk streaming.
  website:          http://hise.audio
  license:          GPLv3 / commercial

  dependencies:     juce_audio_basics, juce_audio_formats, juce_core
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/


/* TODO 24bit rewrite:

- write unit tests for all new functions
- make the temporary voice buffer a floating point buffer and normalize when copying from the two read buffers there.
- ensure 100% backwards compatibility
- check that there's no performance overhead if not used
- skip this when the sample material is already 16bit (maybe even add a warning then)
- support seeking and odd sample offsets
- make the .hr1 files 24bit FLACs
- add some nice end user options


SampleMap bugs:

- saving without saving the samplemap discard changes...

*/


#pragma once





#include "AppConfig.h"
#include "../JUCE/modules/juce_audio_formats/juce_audio_formats.h"

// This is the current HLAC version. HLAC has full backward compatibility.
#define HLAC_VERSION 3

// This is the compression block size used by HLAC. Don't change that value unless you know what you're doing...
#define COMPRESSION_BLOCK_SIZE 4096

/** Config: HI_ENABLE_LEGACY_CPU_SUPPORT

If enabled, then all SSE instructions are replaced by their native implementation. This can be used to compile a
version that runs on legacy CPU models. 
*/
#ifndef HI_ENABLE_LEGACY_CPU_SUPPORT
#define HI_ENABLE_LEGACY_CPU_SUPPORT 0
#endif

//=============================================================================
/** Config: HLAC_MEASURE_DECODING_PERFORMANCE

If enabled, then the decoding time is measured and printed to the console
*/
#ifndef HLAC_MEASURE_DECODING_PERFORMANCE
#define HLAC_MEASURE_DECODING_PERFORMANCE 0
#endif

//=============================================================================
/** Config: HLAC_DEBUG_LOG

If enabled, then the decoder / encoder prints debug information to the output stream.
*/
#ifndef HLAC_DEBUG_LOG
#define HLAC_DEBUG_LOG 0
#endif

//=============================================================================
/** Config: HLAC_INCLUDE_TEST_SUITE

If enabled, then the unit test suite will be compiled and added to all unit tests.
*/
#ifndef HLAC_INCLUDE_TEST_SUITE
#define HLAC_INCLUDE_TEST_SUITE 0
#endif


#include "hlac/BitCompressors.h"
#include "hlac/CompressionHelpers.h"
#include "hlac/SampleBuffer.h"
#include "hlac/HlacEncoder.h"
#include "hlac/HlacDecoder.h"
#include "hlac/HlacAudioFormatWriter.h"
#include "hlac/HlacAudioFormatReader.h"
#include "hlac/HiseLosslessAudioFormat.h"



/** Perfetto integration copied from https://github.com/sudara/melatonin_perfetto. */

#ifndef PERFETTO
    #define PERFETTO 0
#endif

#if PERFETTO

#include <chrono>
#include <fstream>
#include <perfetto.h>
#include <thread>

PERFETTO_DEFINE_CATEGORIES(
	perfetto::Category("component")
	.SetDescription("Component"),
	perfetto::Category("dsp")
	.SetDescription("dsp"));

class MelatoninPerfetto
{
public:
    MelatoninPerfetto (const MelatoninPerfetto&) = delete;

    static MelatoninPerfetto& get()
    {
        static MelatoninPerfetto instance;
        return instance;
    }

    void beginSession (uint32_t buffer_size_kb = 80000);

    // Returns the file where the dump was written to (or a null file if an error occurred)
    // the return value can be ignored if you don't need this information
    juce::File endSession();

    static juce::File getDumpFileDirectory();

private:
    MelatoninPerfetto();

    juce::File writeFile();

    std::unique_ptr<perfetto::TracingSession> session;
};

/*

   There be dragons here. Serious C++ constexpr dragons.

   Deriving the function name produces an ugly string:
        auto AudioProcessor::processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &)::(anonymous class)::operator()()::(anonymous class)::operator()(uint32_t) const

   Without dragons, trying to trim the string results in it happening at runtime.

   With dragons, we get a nice compile time string:
        AudioProcessor::processBlock

*/

namespace melatonin
{
    // This wraps our compile time strings (coming from PRETTY_FUNCTION, etc) in lambdas
    // so that they can be used as template parameters
    // This lets us trim the strings while still using them at compile time
    // https://accu.org/journals/overload/30/172/wu/#_idTextAnchor004
    #define WRAP_COMPILE_TIME_STRING(x) [] { return (x); }
    #define UNWRAP_COMPILE_TIME_STRING(x) (x)()

    template <typename CompileTimeLambdaWrappedString>
    constexpr auto compileTimePrettierFunction (CompileTimeLambdaWrappedString wrappedSrc)
    {
    // if we're C++20 or higher, ensure we're compile-time
    #if __cplusplus >= 202002L
        // This should never assert, but if so,  report it on this issue:
        // https://github.com/sudara/melatonin_perfetto/issues/13#issue-1558171132
        if (!std::is_constant_evaluated())
            jassertfalse;
    #endif

        constexpr auto src = UNWRAP_COMPILE_TIME_STRING (wrappedSrc);
        constexpr auto size = std::string_view (src).size(); // -1 to ignore the /0
        std::array<char, size> result {};

        // loop through the source, building a new truncated array
        // see: https://stackoverflow.com/a/72627251
        for (size_t i = 0; i < size; ++i)
        {
            // wait until after the return type (first space in the string)
            if (src[i] == ' ')
            {
                ++i; // skip the space

                // MSVC has an additional identifier after the return type: __cdecl
                if (src[i + 1] == '_')
                    i += 8; // skip __cdecl and the space afterwards

                size_t j = 0;

                // build result, stop when we hit the arguments
                // clang and gcc use (, MSVC uses <
                while ((src[i] != '(' && src[i] != '<') && i < size && j < size)
                {
                    result[j] = src[i];
                    ++i; // increment character in source
                    ++j; // increment character in result
                }

                // really ugly clean up after msvc, remove the extra :: before <lambda_1>
                if (src[i] == '<')
                {
                    result[j-2] = '\0';
                }
                return result;
            }
        }
        return result;
    }

    namespace test
    {
        // a lil test helper so we don't go crazy
        template <size_t sizeResult, size_t sizeTest>
        constexpr bool strings_equal (const std::array<char, sizeResult>& result, const char (&test)[sizeTest])
        {
            // sanity check
            static_assert (sizeTest > 1);
            static_assert (sizeResult + 1 >= sizeTest); // +1 for the /0

            std::string_view resultView (result.data(), sizeTest);
            std::string_view testView (test, sizeTest);
            return testView == resultView;
        }

        // testing at compile time isn't fun (no debugging) so dumb things like this help:
        constexpr std::array<char, 10> main { "main" };
        static_assert (strings_equal (main, "main"));

        // ensure the return type is removed
        static_assert (strings_equal (compileTimePrettierFunction (WRAP_COMPILE_TIME_STRING ("int main")), "main"));

        // clang example
        static_assert (strings_equal (compileTimePrettierFunction (WRAP_COMPILE_TIME_STRING ("void AudioProcessor::processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &)::(anonymous class)::operator()()::(anonymous class)::operator()(uint32_t) const")), "AudioProcessor::processBlock"));

        // msvc example
        static_assert (strings_equal (compileTimePrettierFunction (WRAP_COMPILE_TIME_STRING ("void __cdecl AudioProcessor::processBlock::<lambda_1>::operator")), "AudioProcessor::processBlock"));
    }
}

    // Et voilà! Our nicer macros.
    // This took > 20 hours, hope the DX is worth it...
    // The separate constexpr calls are required for `compileTimePrettierFunction` to remain constexpr
    // in other words, they can't be inline with perfetto::StaticString, otherwise it will go runtime
    #define TRACE_DSP(...)                                                                                                            \
        constexpr auto pf = melatonin::compileTimePrettierFunction (WRAP_COMPILE_TIME_STRING (PERFETTO_DEBUG_FUNCTION_IDENTIFIER())); \
        TRACE_EVENT ("dsp", perfetto::StaticString (pf.data()), ##__VA_ARGS__)
    #define TRACE_COMPONENT(...)                                                                                                      \
        constexpr auto pf = melatonin::compileTimePrettierFunction (WRAP_COMPILE_TIME_STRING (PERFETTO_DEBUG_FUNCTION_IDENTIFIER())); \
        TRACE_EVENT ("component", perfetto::StaticString (pf.data()), ##__VA_ARGS__)

#else // if PERFETTO
    #define TRACE_EVENT_BEGIN(category, ...)
    #define TRACE_EVENT_END(category)
    #define TRACE_EVENT(category, ...)
    #define TRACE_DSP(...)
    #define TRACE_COMPONENT(...)
#endif




