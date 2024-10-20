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


#include "hi_backend.h"
#include <BinaryData.h>

#if HISE_INCLUDE_FAUST_JIT
#include "../hi_faust_jit/hi_faust_jit.h"
#endif

#include "backend/WinInstallerTemplate.cpp"

#include "backend/BackendCommandIcons.cpp"

#include "backend/debug_components/SamplePoolTable.h"
#include "backend/debug_components/MacroEditTable.h"
#include "backend/debug_components/ScriptComponentEditPanel.h"
#include "backend/debug_components/ScriptComponentPropertyPanels.h"
#include "backend/debug_components/ProcessorCollection.h"
#include "backend/debug_components/ApiBrowser.h"
#include "backend/debug_components/ScriptComponentList.h"
#include "backend/debug_components/ModuleBrowser.h"
#include "backend/debug_components/PatchBrowser.h"
#include "backend/debug_components/FileBrowser.h"

#include "backend/debug_components/SamplePoolTable.cpp"
#include "backend/debug_components/MacroEditTable.cpp"
#include "backend/debug_components/ScriptComponentEditPanel.cpp"
#include "backend/debug_components/ScriptComponentPropertyPanels.cpp"
#include "backend/debug_components/ProcessorCollection.cpp"
#include "backend/debug_components/ApiBrowser.cpp"
#include "backend/debug_components/ExtendedApiDocumentation.cpp"
#include "backend/debug_components/ScriptComponentList.cpp"
#include "backend/debug_components/ModuleBrowser.cpp"
#include "backend/debug_components/PatchBrowser.cpp"
#include "backend/debug_components/FileBrowser.cpp"

#include "backend/PostDebugComponents.cpp"

#include "backend/BackendProcessor.cpp"
#include "backend/BackendComponents.cpp"
#include "backend/BackendToolbar.cpp"
#include "backend/BackendApplicationCommandWindows.cpp"
#include "backend/dialog_library/dialog_library.cpp"
#include "backend/BackendApplicationCommands.cpp"
#include "backend/BackendVisualizer.h"
#include "backend/BackendVisualizer.cpp"
#include "backend/BackendEditor.cpp"
#include "backend/BackendRootWindow.cpp"

#include "backend/ProjectTemplate.cpp"
#include "backend/ProjectDllTemplate.cpp"
#include "backend/StandaloneProjectTemplate.cpp"

#include "backend/CompileExporter.cpp"

#include "backend/doc_generators/ApiMarkdownGenerator.h"
#include "backend/doc_generators/ModuleDocGenerator.h"
#include "backend/doc_generators/UiComponentDocGenerator.h"
#include "backend/doc_generators/MenuReferenceGenerator.h"

#include "backend/doc_generators/ApiMarkdownGenerator.cpp"
#include "backend/doc_generators/ModuleDocGenerator.cpp"
#include "backend/doc_generators/MenuReferenceGenerator.cpp"
#include "backend/doc_generators/UiComponentDocGenerator.cpp"

#include "backend/PostDocGenerators.cpp"



#if HISE_INCLUDE_SNEX
#include "snex_workbench/DspNetworkWorkbench.cpp"
#endif
#include "snex_workbench/WorkbenchProcessor.cpp"
