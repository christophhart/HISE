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

#include "../hi_dsp_library/hi_dsp_library.h"
#include "hi_tools.h"

#if !HISE_NO_GUI_TOOLS
#include "hi_dev/CodeEditorApiBase.cpp"
#endif

#if HISE_INCLUDE_PROFILING_TOOLKIT
#include "hi_dev/DebugProfileTools.cpp"
#include "hi_dev/DebugSessionViewer.cpp"
#include "hi_dev/DebugSessionMultiViewer.cpp"
#include "hi_dev/DebugSessionViewItem.cpp"
#include "hi_dev/DebugSessionComponents.cpp"
#include "hi_dev/DebugSessionManager.cpp"
#include "hi_dev/DebugSession.cpp"
#endif

#if !HISE_NO_GUI_TOOLS
#include "hi_dev/AdvancedCodeEditor.cpp"
#include "hi_dev/ScriptWatchTable.cpp"
#include "hi_dev/JavascriptTokeniserFunctions.h"
#include "hi_dev/JavascriptTokeniser.cpp"

#if USE_BACKEND // Only include this file in the GPL build configuration
#include "hi_dev/FaustTokeniser.cpp"
#endif

#include "hi_dev/ZoomableViewport.cpp"
#if USE_BACKEND
#include "hi_standalone_components/PerfettoWebViewer.cpp"
#endif

#if HISE_INCLUDE_RLOTTIE
#include "hi_dev/RLottieDevComponent.cpp"
#endif
#endif
