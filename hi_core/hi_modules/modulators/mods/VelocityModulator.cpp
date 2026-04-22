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

namespace hise { using namespace juce;

hise::ProcessorMetadata VelocityModulator::createMetadata()
{
	return ProcessorMetadata(getClassType())
		.withPrettyName("Velocity Modulator")
		.withDescription("Creates a modulation value from the MIDI velocity of incoming note messages, with optional table mapping and decibel conversion.")
		.withType<hise::VoiceStartModulator>()
		.withComplexDataInterface(ExternalData::DataType::Table)
		.withParameter(ProcessorMetadata::ParameterMetadata(Inverted)
			.withId("Inverted")
			.withDescription("Inverts the velocity response so high velocities produce low values")
			.asToggle()
			.withDefault(0.0f))
		.withParameter(ProcessorMetadata::ParameterMetadata(UseTable)
			.withId("UseTable")
			.withDescription("Enables a lookup table for custom velocity response curves")
			.asToggle()
			.withDefault(0.0f))
		.withParameter(ProcessorMetadata::ParameterMetadata(DecibelMode)
			.withId("DecibelMode")
			.withDescription("Converts the output to a decibel scale (-100dB to 0dB) then to linear gain")
			.asToggle()
			.withDefault(0.0f));
}

ProcessorEditorBody *VelocityModulator::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new VelocityEditorBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif

};

} // namespace hise
