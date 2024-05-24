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

#pragma once

namespace hise { using namespace juce;


/** A constant Modulator which calculates a random value at the voice start.
*	@ingroup modulatorTypes
*
*	It can use a look up table to "massage" the outcome in order to raise the probability of some values etc.
*	In this case, the values are limited to 7bit for MIDI feeling...
*/
class EventDataModulator: public VoiceStartModulator
{
public:

	enum Parameter
	{
		SlotIndex,
		DefaultValue,
		numParameters
	};

	SET_PROCESSOR_NAME("EventDataModulator", "Event Data Modulator", "Creates a modulation value based on the event data written through the global routing manager.")

	EventDataModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);

		loadAttribute(SlotIndex, "SlotIndex");
		loadAttribute(DefaultValue, "DefaultValue");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();

		saveAttribute(SlotIndex, "SlotIndex");
		saveAttribute(DefaultValue, "DefaultValue");

		return v;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override
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
		}
	};

	float getAttribute(int parameterIndex) const override
	{
		switch(parameterIndex)
		{
		case Parameter::SlotIndex:    return (float)dataSlot;
		case Parameter::DefaultValue: return defaultValue;
		}

		return 0.0f;
	};

	AdditionalEventStorage* additionalEventStorage = nullptr;

	float calculateVoiceStartValue(const HiseEvent &m) override;;

	uint8 dataSlot = 0;
	float defaultValue = 0.0f;
};

class EventDataEnvelope: public EnvelopeModulator	
{
public:

	SET_PROCESSOR_NAME("EventDataEnvelope", "EventData Envelope", "An envelope modulator for time-varying event data slots")

	/// @brief special parameters for EventDataEnvelope
	enum Parameter
	{
		SlotIndex = EnvelopeModulator::Parameters::numParameters,
		DefaultValue, 
		SmoothingTime, 
		numTotalParameters
	};
	
	EventDataEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);

	void setInternalAttribute(int parameterIndex, float newValue) override;;

	float getAttribute(int parameterIndex) const override;;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;
	
	int getNumInternalChains() const override { return getNumChildProcessors(); };
	int getNumChildProcessors() const override { return 0; };
	Processor *getChildProcessor(int ) override { return nullptr; };
	const Processor *getChildProcessor(int ) const override  { return nullptr; };

	float startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

    /** @internal The container for the envelope state. */
    struct EventDataEnvelopeState: public EnvelopeModulator::ModulatorState
	{
	public:

		EventDataEnvelopeState(int voiceIndex): 
			ModulatorState(voiceIndex)
		{};

		void update(double sampleRate, double smoothingTimeMs)
		{
			sampleRate /= (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
			rampValue.prepare(sampleRate, smoothingTimeMs);
		}

		HiseEvent e;
		sfloat rampValue;
	};

	ModulatorState *createSubclassedState(int voiceIndex) const override {return new EventDataEnvelopeState(voiceIndex); };

private:

	AdditionalEventStorage* additionalEventStorage = nullptr;

	void updateSmoothing();
	
	uint8 dataSlot = 0;
	float defaultValue = 0.0f;
	float smoothingTime = 0.0f;

	float calculateNewValue(int voiceIndex);

	EventDataEnvelopeState *state;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventDataEnvelope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(EventDataEnvelope);
};


} // namespace hise

