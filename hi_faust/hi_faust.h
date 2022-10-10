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
  version:          0.0.1
  name:             HISE Faust Integration
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL

  dependencies:      hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/
#pragma once

/** Config: HISE_INCLUDE_FAUST

Enables the Faust Compiler
*/
#ifndef HISE_INCLUDE_FAUST
#define HISE_INCLUDE_FAUST 0
#endif // HISE_INCLUDE_FAUST

/** Config: HISE_FAUST_USE_LLVM_JIT

Use the Faust interpreter instead of the LLVM JIT. This is deactivated by default
and only activated for the HISE backend application.
*/
#ifndef HISE_FAUST_USE_LLVM_JIT
#define HISE_FAUST_USE_LLVM_JIT 0
#endif // HISE_FAUST_USE_LLVM_JIT

// On Windows we'll use libfaust's C interface instead of C++ (Enabled by default on Windows)
#ifndef HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#if (defined (_WIN32) || defined (_WIN64))
#define HISE_FAUST_USE_LIBFAUST_C_INTERFACE 1
#else
#define HISE_FAUST_USE_LIBFAUST_C_INTERFACE 0
#endif
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE

#if HISE_INCLUDE_FAUST
#include <optional>
#include "faust_wrap/gui/UI.h"
#include "faust_wrap/gui/meta.h"
#include "faust_wrap/dsp/dsp.h"
#include "../hi_dsp_library/hi_dsp_library.h"
#include "FaustUI.h"
#include "FaustWrapper.h"
#include "FaustStaticWrapper.h"
#endif // HISE_INCLUDE_FAUST


