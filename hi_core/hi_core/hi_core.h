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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HI_CORE_H_INCLUDED
#define HI_CORE_H_INCLUDED


template <class A, class B>
struct AreSame { enum { VALUE = -1 }; };
template <class A>
struct AreSame<A, A> { enum { VALUE = 1 }; };

struct TestGlobalNamespace
{
	int test_namespace[AreSame<TestGlobalNamespace, ::TestGlobalNamespace>::VALUE];
};


#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef INT_MIN
#define INT_MIN -2147483647
#endif

#include "VariantBuffer.h"


#if USE_IPP
#include "IppFFT.h"
#endif


#include "CustomDataContainers.h"


/** @defgroup processor_interfaces Processor Interface Classes
*	Interface classes that enhance the functionality of a processor.
* */

/** @defgroup core Core Classes
*	A collection of basic classes.
*/
#include "UtilityClasses.h"
#include "HI_LookAndFeels.h"
#include "HiseEventBuffer.h"
#include "DebugLogger.h"


#include "ThreadWithQuasiModalProgressWindow.h"
#include "Popup.h"
#include "Tables.h"
#include "UpdateMerger.h"
#include "Markdown.h"
#include "BackgroundThreads.h"
#include "HiseSettings.h"
#include "SettingsWindows.h"

#include "PresetHandler.h"

#include "ExternalFilePool.h"


#include "ExpansionHandler.h"
#include "GlobalScriptCompileBroadcaster.h"
#include "MainControllerHelpers.h"
#include "LockHelpers.h"
#include "MainController.h"
#include "SampleExporter.h"
#include "Console.h"



#include "JavascriptTokeniser.h"
#include "JavascriptTokeniserFunctions.h"
#include "MacroControlledComponents.h"
#include "MacroControlBroadcaster.h"
#include "MiscComponents.h"
#include "StandaloneProcessor.h"

#endif  // HI_CORE_H_INCLUDED
