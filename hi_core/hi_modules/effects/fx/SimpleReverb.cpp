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

hise::ProcessorMetadata SimpleReverbEffect::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<SimpleReverbEffect>()
		.withDescription("Algorithmic reverb based on Freeverb with controls for room size, damping, and stereo width")
		.withParameter(Par(RoomSize)
			.withId("RoomSize")
			.withDescription("Perceived room size from 0 (small) to 1 (large cathedral)")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.8f))
		.withParameter(Par(Damping)
			.withId("Damping")
			.withDescription("High frequency absorption where higher values create a darker reverb tail")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.6f))
		.withParameter(Par(WetLevel)
			.withId("WetLevel")
			.withDescription("Wet/dry mix where 0 is fully dry and 1 is fully wet")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.2f))
		.withParameter(Par(DryLevel)
			.withId("DryLevel")
			.withDescription("Level of the dry signal mixed into the output")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.8f))
		.withParameter(Par(Width)
			.withId("Width")
			.withDescription("Stereo width of the reverb where 0 is mono and 1 is full stereo")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.8f))
		.withParameter(Par(FreezeMode)
			.withId("FreezeMode")
			.withDescription("Freeze mode that sustains the reverb tail indefinitely")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.1f));
}

ProcessorEditorBody *SimpleReverbEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new ReverbEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

} // namespace hise
