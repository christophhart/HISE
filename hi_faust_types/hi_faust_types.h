
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

  ID:               hi_faust_types
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE Faust type wrapper
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL

  dependencies:

END_JUCE_MODULE_DECLARATION

******************************************************************************/

/** Config: FAUST_NO_WARNING_MESSAGES
 
 Set this to 1 if you're linking against a Faust version older than 2.54.0 in order
 to fix the getWarningMessages() compile error
*/
#ifndef FAUST_NO_WARNING_MESSAGES
#define FAUST_NO_WARNING_MESSAGES 0
#endif

#if HISE_INCLUDE_FAUST
#include "faust_wrap/dsp/dsp.h"
#include "faust_wrap/gui/UI.h"
#include "faust_wrap/gui/meta.h"
#endif

// On Windows we'll use libfaust's C interface instead of C++ (Enabled by default on Windows)
#ifndef HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#if (defined (_WIN32) || defined (_WIN64))
#define HISE_FAUST_USE_LIBFAUST_C_INTERFACE 1
#else
#define HISE_FAUST_USE_LIBFAUST_C_INTERFACE 0
#endif
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
