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

  ID:               hi_faust_jit
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE Faust Integration (JIT Compiler)
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL

  dependencies:     hi_scripting hi_dsp_library hi_faust


END_JUCE_MODULE_DECLARATION

******************************************************************************/
#pragma once

#include "../hi_faust/hi_faust.h"

#if HISE_INCLUDE_FAUST_JIT
#include <optional>
#include "../hi_faust_lib/hi_faust_lib.h"
#include "../hi_core/hi_core.h" // FileHandlerBase
#include "../hi_scripting/hi_scripting.h" // DspNetwork, NodeBase, WrapperNode


#include "FaustWrapper.h"
#include "FaustJitNode.h"

#endif // HISE_INCLUDE_FAUST_JIT


