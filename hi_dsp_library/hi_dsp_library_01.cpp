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



// Import the files here when not building a library (see comment in hi_dsp_library.h)
#if HI_EXPORT_DSP_LIBRARY
#else
#include "hi_dsp_library.h"

#include "dywapitchtrack/dywapitchtrack.h"
#include "dywapitchtrack/dywapitchtrack.c"
#include "dywapitchtrack/PitchDetection.cpp"

#include "dsp_library/DspBaseModule.cpp"



#include "snex_basics/snex_Types.cpp"
#include "snex_basics/snex_ArrayTypes.cpp"
#include "snex_basics/snex_Math.cpp"


#include "snex_basics/snex_DynamicType.cpp"
#include "snex_basics/snex_TypeHelpers.cpp"

#include "snex_basics/snex_ExternalData.cpp"
#include "node_api/helpers/Error.cpp"
#include "node_api/helpers/ParameterData.cpp"
#include "node_api/nodes/Base.cpp"
#include "node_api/nodes/OpaqueNode.cpp"
#include "node_api/nodes/prototypes.cpp"
#include "node_api/nodes/duplicate.cpp"

#include "dsp_basics/chunkware_simple_dynamics/chunkware_simple_dynamics.cpp"
#include "dsp_basics/AllpassDelay.cpp"
#include "dsp_basics/Oscillators.cpp"
#include "dsp_basics/MultiChannelFilters.cpp"

#include "fft_convolver/Utilities.cpp"
#include "fft_convolver/AudioFFT.cpp"
#include "fft_convolver/FFTConvolver.cpp"
#include "fft_convolver/TwoStageFFTConvolver.cpp"


#include "dsp_basics/ConvolutionBase.cpp"

#if HI_RUN_UNIT_TESTS
#include "unit_test/wrapper_tests.cpp"
#include "unit_test/node_tests.cpp"
#include "unit_test/container_tests.cpp"
#endif

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
#include "dsp_nodes/DynamicsNode.cpp"

namespace hise
{
	using namespace juce;

	size_t HelperFunctions::writeString(char* location, const char* content)
	{
		strcpy(location, content);
		return strlen(content);
	}

	juce::String HelperFunctions::createStringFromChar(const char* charFromOtherHeap, size_t length)
	{
		return juce::String(charFromOtherHeap, length);
	}

#define CREATE_PROPERTY_OBJECT(T) if (propertyIndex == T::PropertyIndex) return new T(b);

	// Add all property objects here
	SimpleRingBuffer::PropertyObject* SimpleRingBuffer::createPropertyObject(int propertyIndex, WriterBase* b)
	{
		JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

		CREATE_PROPERTY_OBJECT(OscillatorDisplayProvider::OscillatorDisplayObject);
		CREATE_PROPERTY_OBJECT(ModPlotter::ModPlotterPropertyObject);
		CREATE_PROPERTY_OBJECT(scriptnode::envelope::pimpl::simple_ar_base::PropertyObject);
		CREATE_PROPERTY_OBJECT(scriptnode::envelope::pimpl::ahdsr_base::AhdsrRingBufferProperties);
		CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::Oscilloscope);
		CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::FFT);
		CREATE_PROPERTY_OBJECT(scriptnode::analyse::Helpers::GonioMeter);


		jassertfalse;
		return nullptr;
	}

#undef CREATE_PROPERTY_OBJECT
}



#endif


