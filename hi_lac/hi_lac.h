/*  HISE Lossless Audio Codec
*	�2017 Christoph Hart
*
*	Redistribution and use in source and binary forms, with or without modification, 
*	are permitted provided that the following conditions are met:
*
*	1. Redistributions of source code must retain the above copyright notice, 
*	   this list of conditions and the following disclaimer.
*
*	2. Redistributions in binary form must reproduce the above copyright notice, 
*	   this list of conditions and the following disclaimer in the documentation 
*	   and/or other materials provided with the distribution.
*
*	3. All advertising materials mentioning features or use of this software must 
*	   display the following acknowledgement: 
*	   This product includes software developed by Hart Instruments
*
*	4. Neither the name of the copyright holder nor the names of its contributors may be used 
*	   to endorse or promote products derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY CHRISTOPH HART "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
*	BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
*	DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
*	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
*	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
*	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_lac
  vendor:           Christoph Hart
  version:          1.1
  name:             HISE Lossless Audio Codec
  description:      A fast, lossless audio codec suitable for disk streaming.
  website:          http://hise.audio
  license:          BSD Clause 4

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

#include <juce_audio_formats/juce_audio_formats.h>

#if !JUCE_IOS
#include <nmmintrin.h> 
#endif

// This is the current HLAC version. HLAC has full backward compatibility.
#define HLAC_VERSION 3

// This is the compression block size used by HLAC. Don't change that value unless you know what you're doing...
#define COMPRESSION_BLOCK_SIZE 4096

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
