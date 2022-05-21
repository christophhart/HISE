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

/* HI Module */

#include "AppConfig.h"

// Import the files here when not building a library (see comment in hi_dsp_library.h)
#if HI_EXPORT_DSP_LIBRARY
#else
#include "hi_dsp_library.h"



#include "dsp_nodes/CoreNodes.cpp"
#include "dsp_nodes/DelayNode.cpp"
#include "dsp_nodes/MathNodes.cpp"
#include "dsp_nodes/FXNodes.cpp"
#include "dsp_nodes/JuceNodes.cpp"
#include "dsp_nodes/RoutingNodes.cpp"
#include "dsp_nodes/FilterNode.cpp"
#include "dsp_nodes/CableNodeBaseClasses.cpp"
#include "dsp_nodes/CableNodes.cpp"
#include "dsp_nodes/EnvelopeNodes.cpp"
#include "dsp_nodes/AnalyserNodes.cpp"
#include "dsp_nodes/ConvolutionNode.cpp"

#endif


