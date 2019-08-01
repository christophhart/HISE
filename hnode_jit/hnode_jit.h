/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hnode_jit
  vendor:           HISE
  version:          1.0
  name:             hnode JIT compiler
  description:      A C compiler based on ASMJit
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:     juce_core
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/


/* TODO Rewrite:

- wrap into namespace hnode::jit OK
- rename everything to hnodeJit OK
- fix unit tests OK

- move classes where they belong OK
- add one liner documentation

- change typeinfo to hnode::Types::ID
- remove juce:: from public header OK
- change template implementation files to _impl.h OK
- remove DSPModule OK

- remove C++ class bullshit
- make oneline mode for expr node

*/

#pragma once


#define IF_CONSTEXPR if

#include "AppConfig.h"

#ifndef HNODE_BOOL_IS_NOT_INT
#define HNODE_BOOL_IS_NOT_INT 0
#endif


#include "../hi_tools/hi_tools.h"
#include "../hi_lac/hi_lac.h"
#include "../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"


#include "hnode_core/hnode_Types.h"
#include "hnode_core/hnode_DynamicType.h"
#include "hnode_core/hnode_TypeHelpers.h"


#include "hnode_jit/hnode_jit_public.h"



#include "jit_components/hnode_JitPlayground.h"


