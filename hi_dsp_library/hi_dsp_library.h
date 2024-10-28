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

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_dsp_library
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE DSP Library module
  description:      The module for building DSP modules
  website:          http://hise.audio
  license:          MIT

  dependencies:     juce_core

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once



#include "../hi_tools/hi_tools.h"
#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_dsp/juce_dsp.h"


/** Config: HI_EXPORT_AS_PROJECT_DLL

	Set this to 1 if you compile the project's networks as dll.
*/
#ifndef HI_EXPORT_AS_PROJECT_DLL
#define HI_EXPORT_AS_PROJECT_DLL 0
#endif

/** Config: HI_EXPORT_DSP_LIBRARY

Set this to 0 if you want to load libraries created with this module.
*/
#ifndef HI_EXPORT_DSP_LIBRARY
#define HI_EXPORT_DSP_LIBRARY 1
#endif

/** Config: IS_STATIC_DSP_LIBRARY

Set this to 1 if you want to embed the libraries created with this module into your binary plugin.
*/
#ifndef IS_STATIC_DSP_LIBRARY
#define IS_STATIC_DSP_LIBRARY 1
#endif

/** Config: HISE_LOG_FILTER_FREQMOD

	If enabled, it will use a logarithmic scale to apply the filter modulation. It's disabled
	by default for old projects in order to keep the sound persistent, but you can enable it to
	get a more natural modulation curve.
*/
#ifndef HISE_LOG_FILTER_FREQMOD
#define HISE_LOG_FILTER_FREQMOD 0
#endif

/** Set the max delay time for the hise delay line class in samples. It must be a power of two. 

	By default this means that the max delay time at 44kHz is ~1.5 seconds, so if you have long delay times
	from a tempo synced delay at 1/1, the delay time will get capped and the delay looses its synchronisation.
	
	If that happens on your project, just raise that to a bigger power of two value (131072, 262144, 524288, 1048576)
	in the ExtraDefinitions field of your project settings.
*/
#ifndef HISE_MAX_DELAY_TIME_SAMPLES
#define HISE_MAX_DELAY_TIME_SAMPLES 65536
#endif






// Include the basic structures from SNEX


#include "node_api/helpers/node_macros.h"


#include "snex_basics/snex_Types.h"

#include "snex_basics/snex_TypeHelpers.h"

#include "snex_basics/snex_IndexTypes.h"
#include "snex_basics/snex_ArrayTypes.h"
#include "snex_basics/snex_Math.h"
#include "snex_basics/snex_IndexLogic.h"
#include "snex_basics/snex_DynamicType.h"







#include "snex_basics/snex_ExternalData.h"



#include "snex_basics/snex_FrameProcessor.h"
#include "snex_basics/snex_FrameProcessor.cpp"
#include "snex_basics/snex_ProcessDataTypes.h"
#include "snex_basics/snex_ProcessDataTypes.cpp"

#include "dsp_library/DspBaseModule.h"
#include "dsp_library/BaseFactory.h"
#include "dsp_library/DspFactory.h"


#include "dsp_basics/chunkware_simple_dynamics/chunkware_simple_dynamics.h"
#include "dsp_basics/AllpassDelay.h"


#include "dsp_basics/logic_classes.h"
#include "dsp_basics/DelayLine.h"
#include "dsp_basics/DelayLine.cpp"
#include "dsp_basics/Oscillators.h"
#include "dsp_basics/MultiChannelFilters.h"


#include "fft_convolver/Utilities.h"
#include "fft_convolver/AudioFFT.h"
#include "fft_convolver/FFTConvolver.h"
#include "fft_convolver/TwoStageFFTConvolver.h"
#include "dsp_basics/ConvolutionBase.h"

#include "node_api/helpers/Error.h"
#include "node_api/helpers/node_ids.h"
#include "node_api/helpers/ParameterData.h"

#include "node_api/helpers/range.h"
#include "node_api/helpers/range_impl.h"


#include "node_api/helpers/parameter.h"
#include "node_api/helpers/parameter_impl.h"



#include "node_api/nodes/prototypes.h"
#include "node_api/nodes/duplicate.h"

#include "node_api/nodes/Base.h"
#include "node_api/nodes/Bypass.h"
#include "node_api/nodes/container_base.h"
#include "node_api/nodes/container_base_impl.h"
#include "node_api/nodes/Containers.h"
#include "node_api/nodes/Container_Chain.h"
#include "node_api/nodes/Container_Split.h"
#include "node_api/nodes/Container_Multi.h"

#include "node_api/nodes/OpaqueNode.h"
#include "node_api/nodes/processors.h"

#include "dsp_nodes/CoreNodes.h"


#include "dsp_nodes/CableNodeBaseClasses.h"
#include "dsp_nodes/CableNodes.h"
#include "dsp_nodes/RoutingNodes.h"
#include "dsp_nodes/JuceNodes.h"
#include "dsp_nodes/DelayNode.h"
#include "dsp_nodes/MathNodes.h"
#include "dsp_nodes/FXNodes.h"

#include "dsp_nodes/ConvolutionNode.h"
#include "dsp_nodes/FilterNode.h"
#include "dsp_nodes/EventNodes.h"
#include "dsp_nodes/EnvelopeNodes.h"
#include "dsp_nodes/DynamicsNode.h"
#include "dsp_nodes/AnalyserNodes.h"


#include "dsp_nodes/StretchNode.h"

#include "dsp_nodes/FXNodes_impl.h"


// Include these files in the header because the external functions won't get linked when in another object file...
#if HI_EXPORT_DSP_LIBRARY
#include "dsp_library/DspBaseModule.cpp"

#include "dsp_library/HiseLibraryHeader.h"
#include "dsp_library/HiseLibraryHeader.cpp"
#else

#if HI_EXPORT_AS_PROJECT_DLL
#include "dsp_library/HiseLibraryHeader.h"
#endif

namespace hise {
	namespace HelperFunctions
	{
		size_t writeString(char* location, const char* content);

		juce::String createStringFromChar(const char* charFromOtherHeap, size_t length);
	};
}
#endif

