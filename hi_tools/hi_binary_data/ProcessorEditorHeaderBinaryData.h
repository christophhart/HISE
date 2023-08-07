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

#ifndef PROCESSOREDITORHEADERBINARYDATA_H_INCLUDED
#define PROCESSOREDITORHEADERBINARYDATA_H_INCLUDED

namespace hise { namespace HiBinaryData {

namespace ProcessorIcons
{
	DECLARE_DATA(gainModulation, 115);
	DECLARE_DATA(pitchModulation, 532);
	DECLARE_DATA(effectChain, 224);
	DECLARE_DATA(sampleStartModulation, 69);
	
};


namespace ProcessorEditorHeaderIcons
{
	DECLARE_DATA(addIcon, 2445);
	DECLARE_DATA(closeIcon, 2481);
	DECLARE_DATA(foldedIcon, 1330);
	DECLARE_DATA(bypassShape, 1466);
	DECLARE_DATA(bipolarIcon, 95);
	DECLARE_DATA(unipolarIcon, 77);
	DECLARE_DATA(popupShape, 2391);
	DECLARE_DATA(retriggerOffPath, 50);
	DECLARE_DATA(retriggerOnPath, 77);
	DECLARE_DATA(polyphonicPath, 11110);
	DECLARE_DATA(monophonicPath, 1458);

};

namespace SpecialSymbols
{
	DECLARE_DATA(midiData, 774);
	DECLARE_DATA(masterEffect, 444);
	DECLARE_DATA(macros, 355);
	DECLARE_DATA(globalCableIcon, 1046);
	DECLARE_DATA(scriptProcessor, 1325);
	DECLARE_DATA(routingIcon, 241);
};

}} // namespace hise::HiBinaryData

#endif  // PROCESSOREDITORHEADERBINARYDATA_H_INCLUDED
