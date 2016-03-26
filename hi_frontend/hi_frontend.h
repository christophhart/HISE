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

#ifndef HI_FRONTEND_INCLUDED
#define HI_FRONENT_INCLUDED

#include <AppConfig.h>

#include "LibConfig.h"
#include "Macros.h"
#include "JuceHeader.h"

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/stk_core/stk_core.h"
#include "../JUCE/modules/stk_effects/stk_effects.h"
#include "../JUCE/modules/stk_filters/stk_filters.h"
#include "../JUCE/modules/stk_generators/stk_generators.h"
#include "../JUCE/modules/juce_tracktion_marketplace/juce_tracktion_marketplace.h"



using namespace juce;

#include "frontend/FrontEndProcessor.h"
#include "frontend/FrontendBar.h"
#include "frontend/FrontendProcessorEditor.h"


#endif   // HI_FRONTEND_INCLUDED
