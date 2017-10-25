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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if USE_VDSP_FFT
#define Point DummyPoint
#define Component DummyComponent
#define MemoryBlock DummyMB
#include <Accelerate/Accelerate.h>
#undef Point
#undef Component
#undef MemoryBlock
#endif


#include "hi_core.h"

#include <regex>
#include <atomic>
#include <float.h>
#include <limits.h>

#if JUCE_IOS
#else
#include "xmmintrin.h"
#endif


namespace juce
{
#include "VariantBuffer.cpp"
}

namespace hise
{
using namespace juce;

#include "CustomDataContainers.cpp"

#if USE_IPP
#include "IppFFT.cpp"
#endif

#include "UtilityClasses.cpp"
#include "DebugLogger.cpp"
#include "ThreadWithQuasiModalProgressWindow.cpp"
#include "HI_LookAndFeels.cpp"
#include "Tables.cpp"
#include "ExternalFilePool.cpp"
#include "GlobalScriptCompileBroadcaster.cpp"
#include "MainControllerHelpers.cpp"
#include "MainController.cpp"
#include "MainControllerSubClasses.cpp"
#include "PresetHandler.cpp"
#include "SampleExporter.cpp"
#include "Popup.cpp"
#include "Console.cpp"
#include "BackgroundThreads.cpp"
#include "SettingsWindows.cpp"
#include "MiscComponents.cpp"
#include "JavascriptTokeniser.cpp"
#include "MacroControlledComponents.cpp"
#include "MacroControlBroadcaster.cpp"
#include "HiseEventBuffer.cpp"
#include "StandaloneProcessor.cpp"

#if HI_RUN_UNIT_TESTS
//#include "HiseEventBufferUnitTests.cpp"
#endif

}
