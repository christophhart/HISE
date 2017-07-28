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

#include "JuceHeader.h"

#include "resizable_height_component/ResizableHeightComponent.cpp"

#include "vu_meter/Plotter.cpp"
#include "drag_plot/SliderPack.cpp"
#include "drag_plot/TableEditor.cpp"
#include "keyboard/CustomKeyboard.cpp"
#include "plugin_components/VoiceCpuBpmComponent.cpp"
#include "plugin_components/PresetBrowser.cpp"
#include "plugin_components/PresetComponents.cpp"
#include "plugin_components/StandalonePopupComponents.cpp"
#include "plugin_components/FrontendBar.cpp"

#if USE_BACKEND
#include "plugin_components/PluginPreviewWindow.cpp"
#endif

#include "wave_components/SampleDisplayComponent.cpp"
#include "wave_components/WavetableComponents.cpp"

#include "vu_meter/VuMeter.cpp"

#include "eq_plot/FilterInfo.cpp"
#include "eq_plot/FilterGraph.cpp"
#include "eq_plot/EqComponent.cpp"


#include "floating_layout/FloatingLayout.cpp"
