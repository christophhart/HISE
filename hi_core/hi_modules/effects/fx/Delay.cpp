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

	parameterNames.add("DelayTimeLeft");
	parameterNames.add("DelayTimeRight");
	parameterNames.add("FeedbackLeft");
	parameterNames.add("FeedbackRight");
	parameterNames.add("LowPassFreq");
	parameterNames.add("HiPassFreq");
	parameterNames.add("Mix");
	parameterNames.add("TempoSync");

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

float DelayEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case DelayTimeLeft:		return tempoSync ? (float)syncTimeLeft : delayTimeLeft;
	case DelayTimeRight:	return tempoSync ? (float)syncTimeRight : delayTimeRight;
	case FeedbackLeft:		return feedbackLeft;
	case FeedbackRight:		return feedbackRight;
	case LowPassFreq:		return lowPassFreq;
	case HiPassFreq:		return hiPassFreq;
	case Mix:				return 0.5f;
	case TempoSync:			return true;
	default:				jassertfalse; return 0.0f;
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
