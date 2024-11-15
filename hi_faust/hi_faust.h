/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2022 Roman Sommer
*   Copyright 2022 Christoph Hart
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

  ID:               hi_faust
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE Faust Integration
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL

  dependencies:      hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/
#pragma once

/** Config: HISE_INCLUDE_FAUST

Enables static Faust wrapper code. Needed for anything related to Faust, including compilation of previously exported code.
*/
#ifndef HISE_INCLUDE_FAUST
#define HISE_INCLUDE_FAUST 0
#endif // HISE_INCLUDE_FAUST

/** Config: HISE_FAUST_USE_LLVM_JIT

Use the LLVM JIT backend for runtime Faust compilation (Enabled by default)
If disabled the significantly slower interpreter backend will be used as a fallback.
Disable if you have issues with the LLVM backend.
Does not affect exported code.
*/
#ifndef HISE_FAUST_USE_LLVM_JIT
#define HISE_FAUST_USE_LLVM_JIT 1
#endif // HISE_FAUST_USE_LLVM_JIT

/** Config: HISE_INCLUDE_FAUST_JIT

Enables the "core.faust" node for dynamic compilation/interpretation of Faust
code and the static code export mechanism.
Enable if you want to develop Faust code in HISE.
Not needed if you just want to build already exported code.
*/
#ifndef HISE_INCLUDE_FAUST_JIT
#define HISE_INCLUDE_FAUST_JIT 0
#endif // HISE_INCLUDE_FAUST_JIT

// HISE_INCLUDE_FAUST depends on HISE_INCLUDE_FAUST
#if HISE_INCLUDE_FAUST_JIT && !HISE_INCLUDE_FAUST
#error "HISE_INCLUDE_FAUST_JIT was enabled but depends on HISE_INCLUDE_FAUST, which is disabled."
#endif

#if HISE_INCLUDE_FAUST
#include <optional>
#include "../hi_faust_types/hi_faust_types.h"
#include "../hi_dsp_library/hi_dsp_library.h"


// We'll link the faust library using a #pragma so that we can use different Projucer Export configurations
// to enable / disable faust
#if JUCE_WINDOWS && HISE_INCLUDE_FAUST_JIT
#pragma comment(lib, "faust.lib")
#endif

#include "FaustUI.h"
#include "FaustWrapper.h"
#include "FaustStaticWrapper.h"
#endif // HISE_INCLUDE_FAUST


