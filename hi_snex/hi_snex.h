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

  ID:               hi_snex
  vendor:           HISE
  version:          1.0
  name:             Snex JIT compiler
  description:      A (more or less) C compiler based on ASMJit
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:     juce_core
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/


/* TODO Roadmap:

- use JitCompiledNode with the boilerplate code for scriptnode library nodes et al... b
- add scriptnode node classes to SNEX

*/

#pragma once


#define REMOVE_REUSABLE_REG 0


/** As soon as we jump to C++14, we can use proper if constexpr. */
#define IF_CONSTEXPR if

#include "AppConfig.h"




/** Config: HISE_INCLUDE_SNEX
 
Set to 0 to disable SNEX compilation (default on iOS).
 */
#ifndef HISE_INCLUDE_SNEX
#if HISE_IOS
#define HISE_INCLUDE_SNEX 0
#else
#define HISE_INCLUDE_SNEX 1
#endif
#endif




#include "../hi_tools/hi_tools.h"
#include "../hi_lac/hi_lac.h"
#include "../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"

#if HISE_INCLUDE_SNEX
#include "snex_core/snex_Types.h"

#include "snex_core/snex_ArrayTypes.h"
#include "snex_core/snex_ArrayTypes.cpp"
#include "snex_core/snex_Math.h"
#include "snex_core/snex_DynamicType.h"
#include "snex_core/snex_TypeHelpers.h"
#include "snex_jit/snex_jit_public.h"
#include "api/SnexApi.h"

#include "snex_core/snex_CallbackCollection.h"
#include "snex_core/snex_FrameProcessor.h"
#include "snex_core/snex_FrameProcessor.cpp"
#include "snex_core/snex_ProcessDataTypes.h"
#include "snex_core/snex_ProcessDataTypes.cpp"
#include "snex_core/snex_ExternalObjects.h"
#include "snex_jit/snex_jit_NativeDspFunctions.h"



#include "snex_components/snex_WorkbenchData.h"
#include "snex_components/snex_ExtraComponents.h"

#include "snex_components/snex_JitPlayground.h"
#include "snex_components/snex_DebugTools.h"
#endif

#if HISE_INCLUDE_SNEX
using SnexDebugHandler = snex::DebugHandler;
using SnexExpressionPtr = snex::JitExpression::Ptr;
#else
struct SnexDebugHandler
{
    virtual ~SnexDebugHandler() {}
    virtual void logMessage(int level, const juce::String& s) {};
};

using SnexExpressionPtr = void*;
#endif
