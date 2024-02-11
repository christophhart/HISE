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

  ID:               hi_loris
  vendor:           CERL Soundgroup
  version:          1.8.0
  name:             HISE Loris module
  description:      A JUCE module that wraps the Loris library
  website:          https://www.cerlsoundgroup.org/Loris/
  license:          GPL 2.0 (must not be included in compiled pluings!)

  dependencies:     juce_audio_basics, juce_audio_formats, juce_core, juce_graphics,  juce_data_structures, juce_events
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once


#include "../JUCE/modules/juce_audio_formats/juce_audio_formats.h"
#include "../JUCE/modules/juce_data_structures/juce_data_structures.h"
#include "../JUCE/modules/juce_events/juce_events.h"
#include "../JUCE/modules/juce_graphics/juce_graphics.h"
#include "../hi_zstd/hi_zstd.h"

/** Config: HISE_INCLUDE_LORIS

    Includes the Loris framework. This can be deactivated when buiding the HISE app in order to speed up building times a bit.
*/
#ifndef HISE_INCLUDE_LORIS
#error "this must be defined as non default so that the other translation units pickup the value"
#define HISE_INCLUDE_LORIS 1
#endif

/** Config: HISE_USE_LORIS_DLL
 
	If enabled, this uses the (old) dynamic library for the loris library. If disabled, it uses the statically compiled functions. */
#ifndef HISE_USE_LORIS_DLL
#define HISE_USE_LORIS_DLL 0
#endif

#if HISE_INCLUDE_LORIS
#include "wrapper/public.h"
#include "wrapper/ThreadController.h"
#endif


