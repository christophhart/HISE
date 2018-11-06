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

  ID:               hi_components
  vendor:           Hart Instruments
  version:          1.6.0
  name:             HISE Components Module
  description:      The UI componets for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp, hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/



#ifndef HI_COMPONENTS_H_INCLUDED
#define HI_COMPONENTS_H_INCLUDED


#include "../hi_dsp/hi_dsp.h"
#include "../hi_dsp_library/hi_dsp_library.h"

/** @defgroup ui UI

	All class related to the interface design using C++	
*/

/** @defgroup event_handling Event Handling
	@ingroup ui
	
	Classes and helpers for communication between audio rendering
	and the user interface.
/*

/** @defgroup hise_ui HISE Components
*	@ingroup ui
*
*	All HISE UI Components that can be used in compiled plugins.
*/


#include "resizable_height_component/ResizableHeightComponent.h"


#include "keyboard/CustomKeyboard.h"
#include "plugin_components/VoiceCpuBpmComponent.h"
#include "plugin_components/PresetBrowser.h"
#include "plugin_components/StandalonePopupComponents.h"

#include "plugin_components/FrontendBar.h"


#if USE_BACKEND
#include "plugin_components/PluginPreviewWindow.h"
#endif

#include "wave_components/SampleComponents.h"


#include "eq_plot/FilterInfo.h"
#include "eq_plot/FilterGraph.h"
#include "eq_plot/EqComponent.h"

#include "floating_layout/FloatingLayout.h"
#include "plugin_components/PanelTypes.h"

#include "hi_expansion/ExpansionFloatingTiles.h"

namespace hise {
using namespace juce;

}



#endif  // HI_COMPONENTS_H_INCLUDED
