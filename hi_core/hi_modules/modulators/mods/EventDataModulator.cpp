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


#include "EventDataModulator.h"

namespace hise { using namespace juce;



EventDataModulator::EventDataModulator(MainController* mc, const String& id, int numVoices, Modulation::Mode m):
	VoiceStartModulator(mc, id, numVoices, m),
	Modulation(m) 
{
	auto rm = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(mc);
	additionalEventStorage = &rm->additionalEventStorage;

	parameterNames.add(Identifier("SlotIndex"));
	parameterNames.add(Identifier("DefaultValue")); 
	
	updateParameterSlots();
}

#if USE_BACKEND
struct EventDataEditor: public ProcessorEditorBody
{
	EventDataEditor(ProcessorEditor* pe):
	  ProcessorEditorBody(pe),
	  defaultValue("DefaultValue"),
	  dataSlot("SlotIndex")
	{
		auto isEnvelope = dynamic_cast<EventDataEnvelope*>(getProcessor()) != nullptr;

		addAndMakeVisible(defaultValue);
		defaultValue.setup(getProcessor(), isEnvelope ?
			(int)EventDataEnvelope::Parameter::DefaultValue :
			(int)EventDataModulator::Parameter::DefaultValue, "DefaultValue");
		defaultValue.setMode(HiSlider::NormalizedPercentage);
		defaultValue.setTooltip (TRANS("The value if the event data hasn't been written"));
	    defaultValue.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
	    defaultValue.setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
	    defaultValue.setColour (Slider::thumbColourId, Colour (0x80666666));
	    defaultValue.setColour (Slider::textBoxTextColourId, Colours::white);


		addAndMakeVisible(dataSlot);
		dataSlot.setup(getProcessor(), isEnvelope ? 
			(int)EventDataEnvelope::Parameter::SlotIndex :
			(int)EventDataModulator::Parameter::SlotIndex, "SlotIndex");
		dataSlot.setMode(HiSlider::Discrete, 0.0, (double)AdditionalEventStorage::NumDataSlots, DBL_MAX,  1.0);

		dataSlot.setTooltip (TRANS("Set the Slot index for the event data"));
	    dataSlot.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
	    dataSlot.setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
	    dataSlot.setColour (Slider::thumbColourId, Colour (0x80666666));
	    dataSlot.setColour (Slider::textBoxTextColourId, Colours::white);

		if(isEnvelope)
		{
			addAndMakeVisible(smoothingSlider = new HiSlider("SmoothingTime"));
			smoothingSlider->setup(getProcessor(), EventDataEnvelope::Parameter::SmoothingTime, "SmoothingTime");
			smoothingSlider->setMode(HiSlider::Time, 0.0, 2000.0, 100.0);
			smoothingSlider->setTooltip (TRANS("The value if the event data hasn't been written"));
		    smoothingSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
		    smoothingSlider->setTextBoxStyle (Slider::TextBoxRight, true, 80, 20);
		    smoothingSlider->setColour (Slider::thumbColourId, Colour (0x80666666));
		    smoothingSlider->setColour (Slider::textBoxTextColourId, Colours::white);
		}

	}

	void paint(Graphics& g) override
	{
		ProcessorEditorLookAndFeel::fillEditorBackgroundRect(g, this);
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(20);

		dataSlot.setBounds(b.removeFromLeft(128));
		b.removeFromLeft(20);
		defaultValue.setBounds(b.removeFromLeft(128));

		if(smoothingSlider != nullptr)
		{
			b.removeFromLeft(20);
			smoothingSlider->setBounds(b.removeFromLeft(128));
		}
	}

	void updateGui() override
	{
		defaultValue.updateValue();
		dataSlot.updateValue();

		if(smoothingSlider != nullptr)
			smoothingSlider->updateValue();
	}

	int getBodyHeight() const override { return 48 + 40; }

	HiSlider defaultValue, dataSlot;

	ScopedPointer<HiSlider> smoothingSlider;
};
#endif

ProcessorEditorBody* EventDataModulator::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND
	return new EventDataEditor(parentEditor); 
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

float EventDataModulator::calculateVoiceStartValue(const HiseEvent& e)
{
	if(auto rm = getMainController()->getGlobalRoutingManager())
	{
		if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(rm))
		{
			auto nv = m->additionalEventStorage.getValue(e.getEventId(), dataSlot);

			if(nv.first)
				return var(jlimit(0.0f, 1.0f, (float)nv.second));
		}
	}
	else
	{
		debugError(this, "You need to create a global routing manager before using this modulator");
	}

	return defaultValue;;
}


EventDataEnvelope::EventDataEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m):
		EnvelopeModulator(mc, id, voiceAmount, m),
		Modulation(m)
{
	auto rm = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(mc);
	additionalEventStorage = &rm->additionalEventStorage;

	parameterNames.add("SlotIndex");
	parameterNames.add("DefaultValue");
	parameterNames.add("SmoothingTime");

	updateParameterSlots();

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	monophonicState = createSubclassedState(-1);
	state = dynamic_cast<EventDataEnvelopeState*>(monophonicState.get());
}

void EventDataEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	FloatSanitizers::sanitizeFloatNumber(newValue);

	switch(parameterIndex)
	{
	case Parameter::DefaultValue:
		defaultValue = jlimit(0.0f, 1.0f, newValue);
		break;
	case Parameter::SlotIndex:
		dataSlot = jlimit<uint8>(0, AdditionalEventStorage::NumDataSlots, (uint8)(int)newValue);
		break;
	case Parameter::SmoothingTime:
		smoothingTime = newValue;
		updateSmoothing();
		break;
	}
}

float EventDataEnvelope::getAttribute(int parameterIndex) const
{
	switch(parameterIndex)
	{
	case Parameter::SlotIndex:		  return (float)dataSlot;
	case Parameter::DefaultValue:	  return defaultValue;
	case Parameter::SmoothingTime:    return smoothingTime;
	}

	return 0.0f;
}

void EventDataEnvelope::restoreFromValueTree(const ValueTree& v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(SlotIndex, "SlotIndex");
	loadAttribute(DefaultValue, "DefaultValue");
	loadAttribute(SmoothingTime, "SmoothingTime");
}

ValueTree EventDataEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(SlotIndex, "SlotIndex");
	saveAttribute(DefaultValue, "DefaultValue");
	saveAttribute(SmoothingTime, "SmoothingTime");

	return v;
}

float EventDataEnvelope::startVoice(int voiceIndex)
{
	auto ms = static_cast<ModulatorSynth*>(getParentProcessor(true));
	auto v = static_cast<ModulatorSynthVoice*>(ms->getVoice(voiceIndex));
	auto s = static_cast<EventDataEnvelopeState*>(states[voiceIndex]);
	s->e = v->getCurrentHiseEvent();
	auto nv = additionalEventStorage->getValue(s->e.getEventId(), dataSlot);
	return nv.first ? (float)nv.second : defaultValue;
}

void EventDataEnvelope::stopVoice(int voiceIndex)
{
	EnvelopeModulator::stopVoice(voiceIndex);
}

void EventDataEnvelope::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);
}

bool EventDataEnvelope::isPlaying(int voiceIndex) const
{
	return true; // never kill
}

void EventDataEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	
	updateSmoothing();
}

void EventDataEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();
	jassert(voiceIndex < states.size());

	if(isMonophonic)
		state = static_cast<EventDataEnvelopeState*>(monophonicState.get());
	else
		state = static_cast<EventDataEnvelopeState*>(states[voiceIndex]);

	auto nv = additionalEventStorage->getValue(state->e.getEventId(), dataSlot);

	auto v = nv.first ? (float)nv.second : defaultValue;
	
	if(state->rampValue.targetValue != v)
		state->rampValue.set(v);

	if (!state->rampValue.isActive())
	{
		auto v = state->rampValue.get();
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), v, numSamples);
	}
	else
	{
		float *out = internalBuffer.getWritePointer(0, startSample);

		while (--numSamples >= 0)
				*out++ = state->rampValue.advance();
	}
}

ProcessorEditorBody* EventDataEnvelope::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND
	return new EventDataEditor(parentEditor); 
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void EventDataEnvelope::updateSmoothing()
{
	auto sr = getSampleRate();

	if(sr > 0.0)
	{
		for(int i = 0; i < states.size(); i++)
			static_cast<EventDataEnvelopeState*>(states[i])->update(sr, smoothingTime);

		state->update(sr, smoothingTime);
	}
};

} // namespace hise
