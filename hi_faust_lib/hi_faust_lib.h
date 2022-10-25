
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

  ID:               hi_faust_lib
  vendor:           Hart Instruments
  version:          0.0.1
  name:             HISE Faust library wrapper
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL

  dependencies: hi_faust

  

END_JUCE_MODULE_DECLARATION

******************************************************************************/


#if HISE_INCLUDE_FAUST
#include "../hi_faust_types/hi_faust_types.h"
#endif

#if HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT
#if HISE_FAUST_USE_LLVM_JIT
#include "faust_wrap/dsp/llvm-dsp.h"
#else
#include "faust_wrap/dsp/interpreter-dsp.h"
#endif // HISE_FAUST_USE_LLVM_JIT
#include "faust_wrap/dsp/libfaust.h"
#endif // HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT
