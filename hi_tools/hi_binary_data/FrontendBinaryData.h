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

#ifndef FRONTENDBINARYDATA_H_INCLUDED
#define FRONTENDBINARYDATA_H_INCLUDED

namespace hise { namespace HiBinaryData {

namespace FrontendBinaryData
{
	DECLARE_DATA(hiLogo, 2261);
	DECLARE_DATA(infoButtonShape, 606);
	DECLARE_DATA(panicButtonShape, 334);
	

#if USE_LATO_AS_DEFAULT

	extern const char*   LatoBold_ttf;
	const int            LatoBold_ttfSize = 73332;

	extern const char*   LatoRegular_ttf;
	const int            LatoRegular_ttfSize = 75152;

#else

    extern const char*   oxygen_bold_ttf;
    const int            oxygen_bold_ttfSize = 48812;
    
    extern const char*   oxygen_regular_ttf;
    const int            oxygen_regular_ttfSize = 48092;

#endif

    extern const char*   SourceCodeProBold_otf;
    const int            SourceCodeProBold_otfSize = 143932;

    extern const char*   SourceCodeProRegular_otf;
    const int            SourceCodeProRegular_otfSize = 140088;
    
	

}

}} // hise::HiBinaryData

#endif  // FRONTENDBINARYDATA_H_INCLUDED
