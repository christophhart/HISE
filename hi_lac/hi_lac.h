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


#ifndef HI_LAC_INCLUDED
#define HI_LAC_INCLUDED

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



#endif   // HI_LAC_INCLUDED
