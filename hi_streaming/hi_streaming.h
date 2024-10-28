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

  ID:               hi_streaming
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE Streaming module
  description:      The streaming sampler classes for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:     hi_lac
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_STREAMING_INCLUDED
#define HI_STREAMING_INCLUDED

#include <atomic>

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_data_structures/juce_data_structures.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../hi_lac/hi_lac.h"

// Note: this is only required by the faust compiler with libfaust version <2.52.6
#if JUCE_MAC && USE_BACKEND
#define HISE_DEFAULT_STACK_SIZE 1024 * 1024 * 8
#else
#define HISE_DEFAULT_STACK_SIZE 0
#endif


#if USE_IPP
#include "ipp.h"
#endif


//=============================================================================
/** Config: STANDALONE_STREAMING

Set this to true if you add this module to your existing C++ project and don't embed it into a HISE Project.
*/
#ifndef STANDALONE_STREAMING
#define STANDALONE_STREAMING 1
#endif

/** Config: HISE_SAMPLER_CUBIC_INTERPOLATION

Set this to true in order to use cubic interpolation for the sample playback.

*/
#ifndef HISE_SAMPLER_CUBIC_INTERPOLATION
#define HISE_SAMPLER_CUBIC_INTERPOLATION 0
#endif

/** Config: HISE_SAMPLER_ALLOW_RELEASE_START

Set this to false to disable the release start feature.

*/
#ifndef HISE_SAMPLER_ALLOW_RELEASE_START
#define HISE_SAMPLER_ALLOW_RELEASE_START 1
#endif


#include "hi_streaming/lockfree_fifo/readerwriterqueue.h"
#include "hi_streaming/lockfree_fifo/concurrentqueue.h"

#include "timestretch/time_stretcher.h"

#include "hi_streaming/SampleThreadPool.h"
#include "hi_streaming/MonolithAudioFormat.h"
#include "hi_streaming/StreamingSampler.h"
#include "hi_streaming/StreamingSamplerSound.h"
#include "hi_streaming/StreamingSamplerVoice.h"


#endif   // HI_STREAMING_INCLUDED
