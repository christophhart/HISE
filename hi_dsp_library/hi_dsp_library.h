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
*   Commercial licences for using HISE in an closed source project are
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
  version:          0.999
  name:             HISE DSP Library module
  description:      The module for building DSP modules
  website:          http://hise.audio
  license:          MIT

  dependencies:     juce_core

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_DSP_LIBRARY_H_INCLUDED
#define HI_DSP_LIBRARY_H_INCLUDED

#include "AppConfig.h"
#include "../JUCE/modules/juce_core/juce_core.h"

using namespace juce;

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



#include "dsp_library/DspBaseModule.h"
#include "dsp_library/BaseFactory.h"
#include "dsp_library/DspFactory.h"

// Include these files in the header because the external functions won't get linked when in another object file...
#if HI_EXPORT_DSP_LIBRARY
#include "dsp_library/DspBaseModule.cpp"
#include "dsp_library/HiseLibraryHeader.h"
#include "dsp_library/HiseLibraryHeader.cpp"
#else


#if HI_EXPORT_DSP_LIBRARY

#else


// Write this here and clean up later...
namespace HelperFunctions
{
	size_t writeString(char* location, const char* content);

	String createStringFromChar(const char* charFromOtherHeap, size_t length);
};
#endif




#endif
 
#endif
