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

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef INT_MIN
#define INT_MIN -2147483647
#endif





/** @defgroup processor_interfaces Processor Interface Classes
	@ingroup dsp
*	Interface classes that enhance the functionality of a processor.
	You can add functionality to a Processor by subclassing it from one of these
	pure virtual base classes.
* */




/** @defgroup core Core Classes
*	A collection of basic classes.
*/
#include "UtilityClasses.h"

#include "HiseEventBuffer.h"
#include "DebugLogger.h"
#include "ThreadWithQuasiModalProgressWindow.h"
#include "Popup.h"
#include "UpdateMerger.h"
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


#include "MacroControlledComponents.h"
#include "MacroControlBroadcaster.h"
#include "MiscComponents.h"
#include "StandaloneProcessor.h"

#endif  // HI_CORE_H_INCLUDED
