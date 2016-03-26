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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef LIB_CONFIG_H
#define LIB_CONFIG_H

#define BUILD_SUB_VERSION 615

#if USE_BACKEND || USE_FRONTEND == 0

#ifndef WARN_MISSING_FILE
#define WARN_MISSING_FILE 1
#endif

#if JUCE_WINDOWS

#ifndef USE_IPP
#define USE_IPP 1
#endif

#else

#define USE_VDSP_FOR_CONVOLUTION 1

#endif

#define HI_USE_BACKWARD_COMPATIBILITY 1

#if USE_IPP
#include "ipp.h"
#endif

#ifndef USE_COPY_PROTECTION
// Deactivate the copy protection
#define USE_COPY_PROTECTION 0
#endif

#else

#ifndef USE_COPY_PROTECTION
// Activate the copy protection
#define USE_COPY_PROTECTION 1
#endif

#endif

#ifdef STANDALONE_CONVOLUTION

#define SKIP_IF_STANDALONE_CONVOLUTION() (return;)

#else

#define SKIP_IF_STANDALONE_CONVOLUTION() ()

#endif


#ifndef IS_STANDALONE_APP
#define IS_STANDALONE_APP 0
#endif

#define NUM_POLYPHONIC_VOICES 128
#define NUM_GLOBAL_VARIABLES 128
#define NUM_MIC_POSITIONS 6
#define NUM_MAX_CHANNELS 16

#if USE_BACKEND

#define USE_FRONTEND 0
#define HI_USE_CONSOLE 1

#else

#define HI_USE_CONSOLE 0

#endif


#if JUCE_MAC_OSX
#ifndef HI_WINDOWS
#define HI_WINDOWS 0
#endif
#else
#ifndef HI_WINDOWS
#define HI_WINDOWS 1
#endif
#endif

#ifndef INCLUDE_PROTOPLUG
#define INCLUDE_PROTOPLUG 1
#endif
#define USE_OLD_FILE_FORMAT 0


#endif