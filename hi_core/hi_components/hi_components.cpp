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

#include "resizable_height_component/ResizableHeightComponent.cpp"


#include "keyboard/CustomKeyboard.cpp"
#include "plugin_components/VoiceCpuBpmComponent.cpp"
#include "plugin_components/PresetBrowserComponents.cpp"
#include "plugin_components/PresetBrowser.cpp"
#include "plugin_components/StandalonePopupComponents.cpp"
#include "plugin_components/PanelTypes.cpp"
#include "plugin_components/FrontendBar.cpp"

#include "markdown_components/MarkdownComponents.cpp"
#include "markdown_components/MarkdownPreview.cpp"

#if USE_BACKEND
#include "plugin_components/PluginPreviewWindow.cpp"
#endif

#include "audio_components/SampleComponents.cpp"
#include "audio_components/EqComponent.cpp"
#include "floating_layout/WrapperWithMenuBar.cpp"
#include "floating_layout/FloatingLayout.cpp"
#include "hi_expansion/ExpansionFloatingTiles.cpp"

#include "midi_overlays/SimpleMidiViewer.cpp"
#include "midi_overlays/MidiDropper.cpp"
#include "midi_overlays/MidiLooper.cpp"
#include "midi_overlays/MidiOverlayFactory.cpp"

