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

hise::ProcessorMetadata DelayEffect::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<DelayEffect>()
		.withDescription("Stereo delay with independent left and right times, feedback, and filtering, with optional tempo sync for rhythmic echo effects")
		.withParameter(Par(DelayTimeLeft)
			.withId("DelayTimeLeft")
			.withDescription("Left channel delay time in milliseconds or tempo-synced note value when sync is enabled")
			.withSliderMode(HiSlider::Time, Range(0.0, 3000.0, 1.0))
			.withDynamicDefault([](const Processor* p){
				auto d = dynamic_cast<const DelayEffect*>(p);
				return (float)(int)(d->tempoSync ? d->syncTimeLeft : d->delayTimeLeft);
				})
			.withTempoSyncMode(TempoSync))
		.withParameter(Par(DelayTimeRight)
			.withId("DelayTimeRight")
			.withDescription("Right channel delay time in milliseconds or tempo-synced note value when sync is enabled")
			.withSliderMode(HiSlider::Time, Range(0.0, 3000.0, 1.0))
			.withDynamicDefault([](const Processor* p) {
				auto d = dynamic_cast<const DelayEffect*>(p);
				return (float)(int)(d->tempoSync ? d->syncTimeRight : d->delayTimeRight);
				})
			.withTempoSyncMode(TempoSync))
		.withParameter(Par(FeedbackLeft)
			.withId("FeedbackLeft")
			.withDescription("Left channel feedback amount where 0.0 is no repeats and 1.0 is infinite")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.3f))
		.withParameter(Par(FeedbackRight)
			.withId("FeedbackRight")
			.withDescription("Right channel feedback amount where 0.0 is no repeats and 1.0 is infinite")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.3f))
		.withParameter(Par(LowPassFreq)
			.withId("LowPassFreq")
			.withDescription("Low-pass cutoff frequency applied to the delay feedback")
			.withSliderMode(HiSlider::Frequency, Range(20.0, 20000.0, 0.0))
			.withDefault(20000.0f))
		.withParameter(Par(HiPassFreq)
			.withId("HiPassFreq")
			.withDescription("High-pass cutoff frequency applied to the delay feedback")
			.withSliderMode(HiSlider::Frequency, Range(20.0, 20000.0, 0.0))
			.withDefault(40.0f))
		.withParameter(Par(Mix)
			.withId("Mix")
			.withDescription("Dry and wet balance where 0.0 is fully dry and 1.0 is fully wet")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.5f))
		.withParameter(Par(TempoSync)
			.withId("TempoSync")
			.withDescription("Enables tempo-synced delay times instead of milliseconds")
			.asToggle()
			.withDefault(1.0f));
}

DelayEffect::DelayEffect(MainController *mc, const String &id) :
	MasterEffectProcessor(mc, id),
	delayTimeLeft(300.0f),
	delayTimeRight(250.0f),
	feedbackLeft(0.3f),
	feedbackRight(0.3f),
	lowPassFreq(20000.0f),
	hiPassFreq(40.0f),
	mix(0.5f),
	tempoSync(true),
	syncTimeLeft(TempoSyncer::QuarterTriplet),
	syncTimeRight(TempoSyncer::Quarter),
	skipFirstBuffer(true)
{
	finaliseModChains();
	updateParameterSlots();
	mc->addTempoListener(this);
	enableConsoleOutput(true);
}

float DelayEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DelayTimeLeft:		return tempoSync ? (float)syncTimeLeft : delayTimeLeft;
	case DelayTimeRight:	return tempoSync ? (float)syncTimeRight : delayTimeRight;
	case FeedbackLeft:		return feedbackLeft;
	case FeedbackRight:		return feedbackRight;
	case LowPassFreq:		return lowPassFreq;
	case HiPassFreq:		return hiPassFreq;
	case Mix:				return mix;
	case TempoSync:			return tempoSync ? 1.0f : 0.0f;
	default:				jassertfalse; return 0.0f;
	}
}

void DelayEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case DelayTimeLeft:		if (tempoSync)
								syncTimeLeft = (TempoSyncer::Tempo)(int)newValue;
							
							else
								delayTimeLeft = newValue;

							calcDelayTimes();
							break;

	case DelayTimeRight:	if (tempoSync)
								syncTimeRight = (TempoSyncer::Tempo)(int)newValue;
	
							else
								delayTimeRight = newValue;

							calcDelayTimes();
							break;
	case FeedbackLeft:		feedbackLeft = newValue; break;
	case FeedbackRight:		feedbackRight = newValue; break;
	case LowPassFreq:		lowPassFreq = newValue; break;
	case HiPassFreq:		hiPassFreq = newValue; break;
	case Mix:				mix = newValue; break;
	case TempoSync:			tempoSync = (newValue == 1.0f); 
							calcDelayTimes(); break;
	default:				jassertfalse;
	}
}

void DelayEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	if (skipFirstBuffer)
	{
		skipFirstBuffer = false;
		return;
	}

	const auto dryMix = scriptnode::faders::overlap().getFadeValue<0>(2, mix);
	const auto wetMix = scriptnode::faders::overlap().getFadeValue<1>(2, mix);

	for(auto& s: snex::block(buffer.getWritePointer(0, startSample), numSamples))
		s = dryMix * s + wetMix * leftDelay.getDelayedValue(s + leftDelay.getLastValue() * feedbackLeft);

	for(auto& s: snex::block(buffer.getWritePointer(1, startSample), numSamples))
		s = dryMix * s + wetMix * rightDelay.getDelayedValue(s + rightDelay.getLastValue() * feedbackRight);
}

ProcessorEditorBody *DelayEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new DelayEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}
} // namespace hise
