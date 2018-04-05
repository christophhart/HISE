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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



namespace hise {
using namespace juce;

// Define Macros =================================================================================================================================

#define CLASS(Name) c = addClass(#Name);
#define METHOD(Name) m = c->addMethod(#Name);
#define DESCRIPTION(descriptionString) m->addDescriptionLine(descriptionString); 
#define PARAMETER(type, id, description) m->addParameter<type>(id, description);
#define CODE(codeLine) m->addCodeLine(codeLine);
#define RETURN(type, description) m->addReturnType<type>(description);

// ==============================================================================================================================================

/** This file contains the extended documentation for the Scripting API.
*
*	You can use the preprocessor macros above to add documentation to a method. See below for an example how to use it.
*
*/
void ExtendedApiDocumentation::init()
{
	inititalised = true;

	ClassDocumentation* c = nullptr;
	MethodDocumentation* m = nullptr;

	// Engine Class =============================================================================================================================

	CLASS(Engine);

	// ==========================================================================================================================================

	METHOD(getSamplesForMilliSeconds);
	DESCRIPTION("Converts milli seconds to samples.");
	DESCRIPTION("This uses the current sample rate so the result may vary depending on your audio settings.");
	PARAMETER(double, "milliSeconds", "The time in **millisconds**");

	CODE("// returns 44100.0");
	CODE("const var time = Engine.getSamplesForMilliSeconds(1000.0);");

	RETURN(double, "The time in samples");

	// ==========================================================================================================================================
}

// Undefine macros ==============================================================================================================================

#undef CLASS
#undef METHOD
#undef DESCRIPTION
#undef PARAMETER
#undef CODE
#undef RETURN

// ==============================================================================================================================================

}