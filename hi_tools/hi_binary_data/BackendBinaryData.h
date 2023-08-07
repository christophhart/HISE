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

#ifndef BACKENDBINARYDATA_H_INCLUDED
#define BACKENDBINARYDATA_H_INCLUDED

namespace hise { using namespace juce;

namespace BackendBinaryData
{
	namespace PopupSymbols
	{
		DECLARE_DATA(hiddenPattern, 87);
		DECLARE_DATA(soloShape, 750);
		DECLARE_DATA(octaveUpIcon, 2927);
		DECLARE_DATA(octaveDownIcon, 2886);
		DECLARE_DATA(sustainIcon, 1355);
		DECLARE_DATA(rootShape, 3183);
	};


	namespace ToolbarIcons
	{
		DECLARE_DATA(hiLogo, 1944);
		DECLARE_DATA(apiList, 1231);
		DECLARE_DATA(hamburgerIcon, 388);
		DECLARE_DATA(refreshIcon, 655);
		DECLARE_DATA(modulatorList, 573);
		DECLARE_DATA(viewPanel, 4061);
		DECLARE_DATA(mixer, 3940);
		DECLARE_DATA(keyboard, 495);
		DECLARE_DATA(debugPanel, 2337);
		DECLARE_DATA(settings, 964);
		DECLARE_DATA(macros, 355);
		DECLARE_DATA(plotter, 432);
		DECLARE_DATA(sampleTable, 883);
		DECLARE_DATA(fileTable, 496);
		DECLARE_DATA(imageTable, 442);
		DECLARE_DATA(fileBrowser, 451);
		DECLARE_DATA(menuIcon, 68);
		DECLARE_DATA(addIcon, 2445);
	};
}

} // namespace hise

#endif  // BACKENDBINARYDATA_H_INCLUDED
