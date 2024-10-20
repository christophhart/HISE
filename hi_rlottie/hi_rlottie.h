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

  ID:               hi_rlottie
  vendor:           Samsung
  version:          1.6.0
  name:             HISE Tools module
  description:      A JUCE module that wraps the RLottie library by Samsung
  website:          http://hise.audio
  license:          MIT

  dependencies:     juce_audio_basics, juce_audio_formats, juce_core, juce_graphics,  juce_data_structures, juce_events
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once


#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_graphics/juce_graphics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../hi_zstd/hi_zstd.h"

#if HISE_INCLUDE_LORIS
#include "../hi_loris/hi_loris.h"
#endif

/** Config: HISE_INCLUDE_RLOTTIE

    Includes the Rlottie framework so you can load Lottie Animations
*/
#ifndef HISE_INCLUDE_RLOTTIE
#define HISE_INCLUDE_RLOTTIE 1
#endif

/** Config: HISE_RLOTTIE_DYNAMIC_LIBRARY
    We can now include the rlottie library in the codebase, so
    we don't have to drag around the dynamic library. If you want
    to use the dynamic library, set this to one...
 */
#ifndef HISE_RLOTTIE_DYNAMIC_LIBRARY
#define HISE_RLOTTIE_DYNAMIC_LIBRARY 0
#endif

#if HISE_INCLUDE_RLOTTIE
#include "include/rlottie_capi.h"
#include "wrapper/RLottieManager.h"
#include "wrapper/RLottieAnimation.h"
#include "wrapper/RLottieComponent.h"

#endif
