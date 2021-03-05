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


#ifndef FLOATINGLAYOUT_H_INCLUDED
#define FLOATINGLAYOUT_H_INCLUDED

namespace hise
{
class FloatingTile;
}

#ifndef HISE_FLOATING_TILE_ALLOW_RESIZE_ANIMATION
#define HISE_FLOATING_TILE_ALLOW_RESIZE_ANIMATION 1
#endif



#include "FloatingIcons.cpp"

#include "FloatingTileContent.h"

#include "PanelWithProcessorConnection.h"

#if HI_ENABLE_EXPANSION_EDITING
#include "SamplerPanelTypes.h"
#endif

#include "BackendPanelTypes.h"

#include "MiscFloatingPanelTypes.h"
#include "FrontendPanelTypes.h"
#include "FloatingTileContainer.h"
#include "FloatingTile.h"

#if HISE_INCLUDE_SNEX_FLOATING_TILES
#include "SnexFloatingTiles.h"
#endif

#include "FloatingInterfaceBuilder.h"


#endif  // FLOATINGLAYOUT_H_INCLUDED
