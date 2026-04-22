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

hise::ProcessorMetadata SendEffect::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<SendEffect>()
		.withDescription("Routes audio to a send container with adjustable gain, channel offset, and optional smoothing for consistent send automation")
		.withParameter(Par(Gain)
			.withId("Gain")
			.withDescription("Send gain in decibels applied before routing to the send container")
			.withSliderMode(HiSlider::Decibel, Range(-100.0, 0.0, 0.1))
			.withDefault(-100.0f))
		.withParameter(Par(ChannelOffset)
			.withId("ChannelOffset")
			.withDescription("Channel offset applied when routing into the send container")
			.withSliderMode(HiSlider::Discrete, Range(0.0, (double)NUM_MAX_CHANNELS, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(SendIndex)
			.withId("SendIndex")
			.withDescription("Index of the send container to receive the routed signal")
			.withSliderMode(HiSlider::Discrete, Range(0.0, 128.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(Smoothing)
			.withId("Smoothing")
			.withDescription("Enables smoothing of gain changes to reduce clicks")
			.asToggle()
			.withDefault(1.0f))
		.withModulation(Mod((int)InternalChains::SendLevel)
			.withId("Send Modulation")
			.withDescription("Modulates the send gain")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly));
}

hise::ProcessorMetadata RouteEffect::createMetadata()
{
	return ProcessorMetadata()
		.withStandardMetadata<RouteEffect>()
		.withDescription("Routing matrix for duplicating and distributing audio across channels, useful for building aux-style signal paths and complex channel layouts");
}

RouteEffect::RouteEffect(MainController *mc, const String &uid) :
MasterEffectProcessor(mc, uid)
{
	finaliseModChains();

	getMatrix().setOnlyEnablingAllowed(false);
}

ProcessorEditorBody *RouteEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new RouteFXEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void RouteEffect::renderWholeBuffer(AudioSampleBuffer &b)
{
	

	const int numSamples = b.getNumSamples();

	if (getMatrix().anyChannelActive())
	{
		float gainValues[NUM_MAX_CHANNELS];

		jassert(getMatrix().getNumSourceChannels() == b.getNumChannels());

		for (int i = 0; i < b.getNumChannels(); i++)
		{
			if (getMatrix().isEditorShown(i))
				gainValues[i] = b.getMagnitude(i, 0, b.getNumSamples());
			else
				gainValues[i] = 0.0f;
		}

		getMatrix().setGainValues(gainValues, true);
	}

	for (int i = 0; i < b.getNumChannels(); i++)
	{
		const int j = getMatrix().getSendForSourceChannel(i);

		if (j != -1)
		{
			FloatVectorOperations::add(b.getWritePointer(j), b.getReadPointer(i), numSamples);
		}
	}

	if (getMatrix().anyChannelActive())
	{
		float gainValues[NUM_MAX_CHANNELS];

		jassert(getMatrix().getNumDestinationChannels() == b.getNumChannels());

		for (int i = 0; i < b.getNumChannels(); i++)
		{
			if (getMatrix().isEditorShown(i))
				gainValues[i] = b.getMagnitude(i, 0, b.getNumSamples());
			else
				gainValues[i] = 0.0f;
		}

		getMatrix().setGainValues(gainValues, false);
	}
	

}

void RouteEffect::applyEffect(AudioSampleBuffer &, int, int /*numSamples*/)
{
	
}

} // namespace hise
