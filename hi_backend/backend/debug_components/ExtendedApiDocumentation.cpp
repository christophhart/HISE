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
#define SUBCLASS(Name) c->addSubClass(#Name);
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

	CLASS(Content);
	METHOD(setValuePopupData);
	PARAMETER(DynamicObject, "JSON Object", "a JSON containing the properties");
	DESCRIPTION("Customizes the appearance of the Value popups used in Sliders, Tables & SliderPacks.");
	CODE(R"({)");
	CODE(R"(  "fontName": "Comic Sans MS",)");
	CODE(R"(  "fontSize": 18,)");
	CODE(R"(  "borderSize":)");
	CODE(R"(  "borderRadius":)");
	CODE(R"(  "margin": 10,)");
	CODE(R"(  "bgColour": 0xFFFF0000,)");
	CODE(R"(  "itemColour": 0xFF00FF00,)");
	CODE(R"(  "itemColour2": 0xFF0000FF,)");
	CODE(R"(  "textColour": 0xFF00FFFF)");
	CODE(R"(};)");

	CLASS(ScriptComponent);
	SUBCLASS(ScriptSlider);
	SUBCLASS(ScriptButton);
	SUBCLASS(ScriptComboBox);
	SUBCLASS(ScriptPanel);
	SUBCLASS(ScriptLabel);
	SUBCLASS(ScriptFloatingTile);
	SUBCLASS(ScriptedViewport);
	SUBCLASS(ScriptImage);
	SUBCLASS(ScriptTable);
	SUBCLASS(ScriptAudioWaveform);
	SUBCLASS(ScriptSliderPack);
	
	METHOD(changed);
	DESCRIPTION("This method triggers the control callback of the control.");
	DESCRIPTION("The execution will be asynchronous, so it's safe to call it within performance critical callbacks.");
	DESCRIPTION("> There is an inbuild recursion loop protection which prevents recursive calls to this method.");
	CODE("// this only changes the internal value.");
	CODE("Knob.setValue(12);");
	CODE("// the control callback will be called with 12");
	CODE("Knob.changed();")
	CODE("")
	
	CLASS(ScriptSliderPack);
	METHOD(setWidthArray);
	PARAMETER(Array<var>, "number array", "the normalized widths for each slider");
	DESCRIPTION("Makes the slider pack use non-uniform widths for the sliders.");
	DESCRIPTION("The parameter must be an array starting with `0.0` and ending with `1.0`.");
	DESCRIPTION("It will use these values for calculating the width");
	DESCRIPTION("> The size of the array must be one element bigger than the number of sliders in the SliderPack.");
	CODE("// This array sets a slider pack with 3 sliders");
	CODE("   to use the widths 25%, 50%, 25%:");
	CODE("const var a = [0, 0.25, 0.75, 1.0]");
	CODE("SliderPack.setSliderWidths(a);");
	
	
	CLASS(ScriptTable);
	METHOD(setSnapValues);
	PARAMETER(Array<var>, "number array", "the normalized widths for the snap positions");
	DESCRIPTION("Makes the table snap to the given x positions (from 0.0 to 1.0).");
	DESCRIPTION("The parameter must be an array starting with `0.0` and ending with `1.0`.");
	DESCRIPTION("When dragging / adding points, it will snap the x-position to these values (using a +-10px snap range).");
	DESCRIPTION("The array must start with `0.0` and end with `1.0`, however it won't snap to these values.");
	DESCRIPTION("> The size of the array must be one element bigger than the number of sliders in the SliderPack.");
	CODE("// This array snaps the points to 25% and 75%:");
	CODE("const var a = [0, 0.25, 0.75, 1.0]");
	CODE("SliderPack.setSliderWidths(a);");
	
    
    CLASS(Synth)
    METHOD(playNoteWithStartOffset)
    PARAMETER(int, "channel", "The MIDI channel (starting with 1)");
    PARAMETER(int, "number", "The MIDI note number from 0 to 127");
    PARAMETER(int, "velocity", "The MIDI velocity from 0 to 127");
    PARAMETER(int, "offset", "The offset in samples");
    DESCRIPTION("Plays a note with a given offset. In order to make this work, your sound generator needs to support");
    DESCRIPTION("the offset, so if you try to play a sample which has no sample start modulation, it won't have any effect");
    DESCRIPTION("> Due to the internal event data system, the value is limited to 65536, which is a little bit more than one second. This might become a problem for some use cases");
    
    
    
	/**  */
	
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