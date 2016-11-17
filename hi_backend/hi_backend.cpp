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


#include "hi_backend.h"

#include "backend/BackendBinaryData.cpp"


#include "backend/debug_components/SamplePoolTable.cpp"
#include "backend/debug_components/MacroEditTable.cpp"
#include "backend/debug_components/ScriptWatchTable.cpp"
#include "backend/debug_components/ProcessorCollection.cpp"
#include "backend/debug_components/ApiBrowser.cpp"
#include "backend/debug_components/ModuleBrowser.cpp"
#include "backend/debug_components/PatchBrowser.cpp"
#include "backend/debug_components/FileBrowser.cpp"
#include "backend/debug_components/DebugArea.cpp"

#include "backend/BackendProcessor.cpp"
#include "backend/BackendComponents.cpp"
#include "backend/BackendToolbar.cpp"
#include "backend/ProcessorPopupList.cpp"
#include "backend/MainMenuComponent.cpp"
#include "backend/BackendApplicationCommandWindows.cpp"
#include "backend/BackendApplicationCommands.cpp"
#include "backend/BackendEditor.cpp"

#include "backend/ProjectTemplate.cpp"
#include "backend/StandaloneProjectTemplate.cpp"
#include "backend/CompileExporter.cpp"
