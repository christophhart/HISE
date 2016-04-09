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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#if (defined (_WIN32) || defined (_WIN64))
#define JUCE_WINDOWS 1
#endif

#if HI_USE_CONSOLE 
#define debugToConsole(p, x) (p->getMainController()->writeToConsole(x, 0, p))
#define debugError(p, x) (p->getMainController()->writeToConsole(x, 1, p))
#else
#define debugToConsole(p, x)
#define debugError(p, x)
#endif


#if USE_COPY_PROTECTION
#define CHECK_KEY(mainController){ if(FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(mainController)) fp->checkKey();}
#else
#define CHECK_KEY(mainController) {mainController;}
#endif


#define SCALE_FACTOR() ((float)Desktop::getInstance().getDisplays().getMainDisplay().scale)



#ifdef JUCE_WINDOWS
#define GLOBAL_FONT() (Font("Tahoma", 13.0f, Font::plain))
#define GLOBAL_BOLD_FONT() (Font("Tahoma", 13.0f, Font::bold))
#define GLOBAL_MONOSPACE_FONT() (Font("Consolas", 14.0f, Font::plain))
#else
#define GLOBAL_FONT() (Font("Helvetica", 10.5f, Font::plain))
#define GLOBAL_BOLD_FONT() (Font("Helvetica", 10.5f, Font::bold))
#define GLOBAL_MONOSPACE_FONT() (Font("Menlo", 13.0f, Font::plain))
#endif


#define loadTable(tableVariableName, nameAsString) { const var savedData = v.getProperty(nameAsString, var::null); tableVariableName->restoreData(savedData); }
#define saveTable(tableVariableName, nameAsString) ( v.setProperty(nameAsString, tableVariableName->exportData(), nullptr) )

#if JUCE_DEBUG
#define START_TIMER() (startTimer(150))
#else
#define START_TIMER() (startTimer(30))
#endif

#if JUCE_DEBUG
#define IGNORE_UNUSED_IN_RELEASE(x) 
#else
#define IGNORE_UNUSED_IN_RELEASE(x) (ignoreUnused(x))
#endif

#define CONSTRAIN_TO_0_1(x)(jlimit<float>(0.0f, 1.0f, x))

#define DEBUG_AREA_BACKGROUND_COLOUR 0x11FFFFFF
#define DEBUG_AREA_BACKGROUND_COLOUR_DARK 0x84000000
#define BACKEND_BG_COLOUR 0xFF888888//0xff4d4d4d
#define BACKEND_BG_COLOUR_BRIGHT 0xFF646464

#define BACKEND_ICON_COLOUR_ON 0xCCFFFFFF
#define BACKEND_ICON_COLOUR_OFF 0xFF333333

#define DEBUG_BG_COLOUR 0xff636363

#include "copyProtectionMacros.h"


#endif  // MACROS_H_INCLUDED
