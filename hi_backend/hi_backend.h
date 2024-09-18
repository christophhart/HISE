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

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_backend
  vendor:           Hart Instruments
  version:          4.0.0
  name:             HISE Backend Module
  description:      The backend application classes for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_scripting, hi_faust_jit

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_BACKEND_INCLUDED
#define HI_BACKEND_INCLUDED

#include "AppConfig.h"
#include "../hi_core/hi_core.h"
#include "../hi_scripting/hi_scripting.h"


/** Config: USE_WORKBENCH_EDITOR

If true, the backend processor will create a workbench editor instead of the HISE app. */
#ifndef USE_WORKBENCH_EDITOR
#define USE_WORKBENCH_EDITOR 0
#endif


#include "backend/BackendProcessor.h"
#include "backend/BackendComponents.h"
#include "backend/BackendToolbar.h"
#include "backend/dialog_library/dialog_library.h"
#include "backend/BackendApplicationCommands.h"
#include "backend/BackendEditor.h"
#include "backend/BackendRootWindow.h"
#include "backend/CompileExporter.h"





#if HISE_INCLUDE_SNEX
#include "snex_workbench/DspNetworkWorkbench.h"
#endif
#include "snex_workbench/WorkbenchProcessor.h"



#endif   // HI_BACKEND_INCLUDED
