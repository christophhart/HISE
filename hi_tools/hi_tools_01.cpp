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




#if USE_VDSP_FFT
#define Point DummyPoint
#define Component DummyComponent
#define MemoryBlock DummyMB
#include <Accelerate/Accelerate.h>
#undef Point
#undef Component
#undef MemoryBlock
#endif

#include <atomic>
#include <float.h>
#include <limits.h>
#include <regex>

#include "hi_tools.h"

#if !HISE_NO_GUI_TOOLS
#include "hi_binary_data/hi_binary_data.cpp"
#endif

#if USE_IPP
#include "hi_tools/IppFFT.cpp"
#endif

#include "hi_tools/CustomDataContainers.cpp"
#include "hi_tools/HiseEventBuffer.cpp"

#include "hi_tools/MiscToolClasses.cpp"
#include "hi_dispatch/hi_dispatch.cpp"

#include "hi_tools/PathFactory.cpp"
#include "hi_tools/HI_LookAndFeels.cpp"

#include "hi_tools/runtime_target.cpp"

#if !HISE_NO_GUI_TOOLS


#include "gin_images/gin_imageutilities.h"
#include "gin_images/gin_imageeffects.cpp"
#include "gin_images/gin_imageeffects_blending.cpp"
#include "gin_images/gin_imageeffects_stackblur.cpp"

#include "gin_images/gin_imageutilities.cpp"
#include "hi_tools/PostGraphicsRenderer.cpp"

#include "hi_standalone_components/ChocWebView.cpp"
#include "hi_standalone_components/CodeEditorApiBase.cpp"
#include "hi_standalone_components/AdvancedCodeEditor.cpp"
#include "hi_standalone_components/ScriptWatchTable.cpp"
#include "hi_standalone_components/ComponentWithPreferredSize.cpp"

#if USE_BACKEND // Only include this file in the GPL build configuration
#include "hi_tools/FaustTokeniser.h"
#include "hi_tools/FaustTokeniser.cpp"
#endif

#include "hi_tools/JavascriptTokeniserFunctions.h"
#include "hi_tools/JavascriptTokeniser.cpp"
#include "hi_markdown/MarkdownLink.cpp"
#include "hi_markdown/MarkdownHeader.cpp"
#include "hi_markdown/MarkdownDatabase.cpp"
#include "hi_markdown/MarkdownLayout.cpp"
#include "hi_markdown/MarkdownElements.cpp"
#include "hi_markdown/Markdown.cpp"
#include "hi_markdown/MarkdownDefaultProviders.cpp"
#include "hi_markdown/MarkdownRenderer.cpp"
#include "hi_markdown/MarkdownHtmlExporter.cpp"
#include "hi_markdown/MarkdownDatabaseCrawler.cpp"
#include "hi_markdown/MarkdownParser.cpp"



#include "mcl_editor/mcl_editor.cpp"
#endif

#include "hi_tools/VariantBuffer.cpp"
#include "hi_tools/Tables.cpp"
#include "hi_tools/ValueTreeHelpers.cpp"

#if HISE_INCLUDE_LORIS
#include "hi_tools/LorisManager.cpp"
#endif

#if !HISE_NO_GUI_TOOLS
#include "hi_standalone_components/ZoomableViewport.cpp"
#if USE_BACKEND
#include "hi_standalone_components/PerfettoWebViewer.cpp"
#endif
#endif

#include "hi_standalone_components/SampleDisplayComponent.cpp"

#include "hi_standalone_components/VuMeter.cpp"
#include "hi_standalone_components/Plotter.cpp"
#include "hi_standalone_components/RingBuffer.cpp"
#include "hi_standalone_components/SliderPack.cpp"
#include "hi_standalone_components/TableEditor.cpp"

#if HISE_INCLUDE_RLOTTIE
#include "hi_standalone_components/RLottieDevComponent.cpp"
#endif


#include "hi_standalone_components/eq_plot/FilterInfo.cpp"
#include "hi_standalone_components/eq_plot/FilterGraph.cpp"

#if HISE_INCLUDE_RT_NEURAL
#include "hi_neural/hi_neural.cpp"
#endif