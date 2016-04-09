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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef HI_CORE_INCLUDED
#define HI_CORE_INCLUDED

#include <AppConfig.h>

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/juce_tracktion_marketplace/juce_tracktion_marketplace.h"

#include "LibConfig.h"
#include "Macros.h"

//=============================================================================
/** Config: USE_BACKEND

If true, then the plugin uses the backend system including IDE editor & stuff.
*/
#ifndef USE_BACKEND
#define USE_BACKEND 0
#endif

/** Config: USE_FRONTEND

If true, this project uses the frontend module and some special file reference handling.
*/
#ifndef USE_FRONTEND
#define USE_FRONTEND 1
#endif

/** Config: IS_STANDALONE_APP

If true, then this will use some additional features for the standalone app (popup out windows, audio device settings etc.)
*/
#ifndef IS_STANDALONE_APP
#define IS_STANDALONE_APP 0
#endif

/** Config: USE_COPY_PROTECTION

If true, then the copy protection will be used
*/
#ifndef USE_COPY_PROTECTION
#define USE_COPY_PROTECTION 0
#endif

/** Config: USE_IPP

Use the Intel Performance Primitives Library for the convolution reverb.
*/
#ifndef USE_IPP
#define USE_IPP 1
#endif


using namespace juce;

/**Appconfig file

Use this file to enable the modules that are needed

For all defined variables:

- 1 if the module is used
- 0 if the module should not be used

*/

// Comment this out if you don't want to use the debug tools
#define USE_HI_DEBUG_TOOLS 1


/** Add new subgroups here and in hi_module.cpp
*
*	New files must be added in the specific subfolder header / .cpp file.
*/

#include "hi_binary_data/hi_binary_data.h"
#include "hi_core/hi_core.h"
#include "hi_components/hi_components.h"
#include "hi_dsp/hi_dsp.h"
#include "hi_scripting/hi_scripting.h"
#include "hi_sampler/hi_sampler.h"


#endif   // HI_CORE_INCLUDED
