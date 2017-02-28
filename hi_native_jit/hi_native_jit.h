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

  ID:               hi_native_jit
  vendor:           Hart Instruments
  version:          0.999
  name:             HISE NativeJit C Compiler
  description:      A C compiler based on NativeJIT
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:     juce_core
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_CORE_INCLUDED
#define HI_CORE_INCLUDED

#include "AppConfig.h"



#include "native_jit/inc/Temporary/Allocator.h"
#include "native_jit/inc/Temporary/AllocatorOperations.h"
#include "native_jit/inc/NativeJIT/AllocatorVector.h"
#include "native_jit/inc/Temporary/Assert.h"
#include "native_jit/inc/NativeJIT/BitOperations.h"
#include "native_jit/inc/NativeJIT/CodeGen/CallingConvention.h"
#include "native_jit/inc/NativeJIT/CodeGen/CodeBuffer.h"
#include "native_jit/inc/NativeJIT/CodeGen/ExecutionBuffer.h"
#include "native_jit/inc/NativeJIT/Function.h"
#include "native_jit/inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "native_jit/inc/NativeJIT/CodeGen/FunctionSpecification.h"
#include "native_jit/inc/Temporary/IAllocator.h"
#include "native_jit/inc/NativeJIT/CodeGen/JumpTable.h"
#include "native_jit/inc/NativeJIT/CodeGen/Register.h"
#include "native_jit/inc/Temporary/StlAllocator.h"
#include "native_jit/inc/NativeJIT/CodeGen/ValuePredicates.h"
#include "native_jit/inc/NativeJIT/CodeGen/X64CodeGenerator.h"

#include "../JUCE/modules/juce_core/juce_core.h"


#endif   // HI_CORE_INCLUDED
