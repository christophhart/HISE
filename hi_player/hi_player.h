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

#ifndef HI_PLAYER_INCLUDED
#define HI_PLAYER_INCLUDED

#include <AppConfig.h>

#define USE_FRONTEND 0



#define DEBUG_AREA_BACKGROUND_COLOUR 0x11FFFFFF
#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x84000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#include "../hi_modules/hi_modules.h"


using namespace juce;

#include "player/PlayerBinaryData.h"
#include "player/PresetProcessor.h"
#include "player/PresetContainerProcessor.h"

#include "player/PresetPlayerComponents.h"
#include "player/PresetProcessorEditor.h"
#include "player/PresetContainerProcessorEditor.h"

#endif   // HI_PLAYER_INCLUDED
