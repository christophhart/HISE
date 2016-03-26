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

#ifndef HI_BACKEND_INCLUDED
#define HI_BACKEND_INCLUDED

#include "../hi_modules/hi_modules.h"

using namespace juce;


#define DEBUG_AREA_BACKGROUND_COLOUR 0x11FFFFFF
#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x84000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#define BACKEND_ICON_COLOUR_ON 0xCCFFFFFF
#define BACKEND_ICON_COLOUR_OFF 0xFF333333

#define DEBUG_BG_COLOUR 0xff636363

#include "backend/BackendBinaryData.h"

#include "backend/debug_components/SamplePoolTable.h"
#include "backend/debug_components/MacroEditTable.h"
#include "backend/debug_components/ScriptWatchTable.h"
#include "backend/debug_components/ProcessorCollection.h"
#include "backend/debug_components/ApiBrowser.h"
#include "backend/debug_components/ModuleBrowser.h"
#include "backend/debug_components/PatchBrowser.h"
#include "backend/debug_components/FileBrowser.h"
#include "backend/debug_components/DebugArea.h"

#include "backend/BackendProcessor.h"
#include "backend/BackendComponents.h"
#include "backend/BackendToolbar.h"
#include "backend/ProcessorPopupList.h"
#include "backend/BackendApplicationCommands.h"
#include "backend/BackendEditor.h"



#endif   // HI_BACKEND_INCLUDED
