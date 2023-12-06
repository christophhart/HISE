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

#ifndef ARPEGGIATOR_H_INCLUDED
#define ARPEGGIATOR_H_INCLUDED

namespace hise { using namespace juce;

/** A MIDI arpeggiator.
	@ingroup midiTypes
*
*	A general purpose arpeggiator that can be used to build sequenced patches.
*	This code is based on a script by Elan Hickler.
*/
class Arpeggiator : public HardcodedScriptProcessor,
          public MidiControllerAutomationHandler::MPEData::Listener,
		  public hise::Processor::BypassListener
{
public:

	enum Parameters
	{
		Bypass = 0,
		Reset,
		NumSteps,
		StepReset,
		Stride,
		SortKeys,
		Tempo,
		DirectionType,
		OctaveRange,
		Shuffle,
		CurrentStep
	};

	enum class Direction
	{
		Up = 1,
		Down = 2,
		UpDown = 3,
		DownUp = 4,
		Random,
		Chord,
		numDirections
	};

	Arpeggiator(MainController *mc, const String &id, ModulatorSynth *ms);;

	~Arpeggiator();

	void mpeDataReloaded() override {};
	void mpeModeChanged(bool isEnabled) override;
	void mpeModulatorAssigned(MPEModulator* /*m*/, bool /*wasAssigned*/) override {};

	SET_PROCESSOR_NAME("Arpeggiator", "Arpeggiator", "A arpeggiator module");

	void bypassStateChanged(Processor*, bool isBypassed) override
	{
		if (!isBypassed)
		{
			clearUserHeldKeys();
			reset(true, true);
		}
	}


	void onInit() override;
	void onNoteOn() override;;
	void onNoteOff() override;
	void onControl(ScriptingApi::Content::ScriptComponent *c, var value) override;
	void onController() override;
	void onAllNotesOff() override;
	void onTimer(int /*offsetInBuffer*/);

	void playNote();;

	void stopCurrentNote();

private:

	bool killIncomingNotes = true;

	void sendNoteOff(int eventId);

	Range<uint16> sendNoteOn();

	bool mpeMode = false;

	int channelFilter = 0;

	double timeInterval = 1.0;
	
	struct NoteWithChannel
	{
		int8 noteNumber;
		int8 channel;

		bool operator<(const NoteWithChannel& other) const noexcept { return noteNumber < other.noteNumber; }
		bool operator<=(const NoteWithChannel& other) const noexcept { return noteNumber <= other.noteNumber; }
		bool operator>=(const NoteWithChannel& other) const noexcept { return noteNumber >= other.noteNumber; }
		bool operator>(const NoteWithChannel& other) const noexcept { return noteNumber > other.noteNumber; }
		bool operator==(const NoteWithChannel& other) const noexcept { return noteNumber == other.noteNumber; }
		bool operator!=(const NoteWithChannel& other) const noexcept { return noteNumber != other.noteNumber; }

		NoteWithChannel operator+(int8 delta) const noexcept { return { static_cast<int8>(noteNumber + delta), channel }; };
		NoteWithChannel operator+=(int8 delta) noexcept { noteNumber += delta; return *this; };
	};

	uint16 sendNoteOnInternal(const NoteWithChannel& c);
	
	Array<NoteWithChannel, DummyCriticalSection, 256> sustainHoldKeys;
	Array<NoteWithChannel, DummyCriticalSection, 256> userHeldKeysArray;
	Array<NoteWithChannel, DummyCriticalSection, 256> userHeldKeysArraySorted;
	Array<NoteWithChannel, DummyCriticalSection, 256> MidiSequenceArray;
	Array<NoteWithChannel, DummyCriticalSection, 256> MidiSequenceArraySorted;

	Array<int, DummyCriticalSection, 256> currentlyPlayingEventIds;
	
	struct MPEValues
	{
		MPEValues()
		{
			for (int i = 0; i < 16; i++)
			{
				pressValues[i] = 0;
				strokeValues[i] = 0;
				slideValues[i] = 64;
				glideValues[i] = 8192;
				liftValues[i] = 0;
			}
		}

		int8 pressValues[16];
		int8 strokeValues[16];
		int8 slideValues[16];
		int16 glideValues[16];
		int8 liftValues[16];
	};

	MPEValues mpeValues;

	double internalBPM = 150.0;
	double BPM = 60.0;
	double BPS = 1.0;
	int minNoteLenSamples; // LATER: = Engine.getSampleRate() / 80;
	const double minTimerTime = 0.04;
	
	int arpDirMod = 1; // increment value for midi sequence.

	// gui objects and sequence arrays
	
	void applySliderPackData(NoteWithChannel& c);

	int curSeqPatternEnum = 1;
	
	/* ON CONTROL FUNCTIONS */

	int curIndex = 0;
	int lastIndex = 0;

	/* EXECUTION FUNCTIONS: just use these for simplest implementation */
	
	// private //
	int lastEventId = 0;
	int curHeldNoteIdx = 0;
	int curMasterStep = 0;
	NoteWithChannel currentNote = { -1, -1 };
	int currentVelocity = 0;
	int currentStep = 0;
	int currentNoteLengthInSamples = 0;
	int midiChannel = 0;
	
	bool randomOrder = false;

	bool is_playing = false;

	bool do_tie_note = false;
	bool last_step_was_tied = false;
	int last_tied_note = -1;
	bool dir_needs_change = false;
	int curTiedNote = -1;

	bool shuffleNextNote = false;

	Random r;

	bool shouldFilterMessage(int channel)
	{
		if (mpeMode)
		{
			if (channel == 1)
				return false;

			return channel < mpeStart || channel > mpeEnd;
		}
		else
		{
			return channelFilter > 0 && channel != channelFilter;
		}
	}

	void changeDirection();;

	void calcTimeInterval();;

	void addUserHeldKey(const NoteWithChannel& note);

	void remUserHeldKey(const NoteWithChannel& note);

	void clearUserHeldKeys();

	void reset(bool do_all_notes_off, bool do_stop);;

	void start();;

	void stop();;

	bool keys_are_held()
	{
		return !userHeldKeysArray.isEmpty();

		//return Synth.getNumPressedKeys() != 0;
	};

	void stopIfNoKeysActive()
	{

	}

	int incAndWrapValueFromZeroToMax(int increment, int value, int max)
	{
		if (max == 0)
			return 0;

		int m = max;

		return (((value + increment) % m) + m) % m;
	}

	int WrapValueFromZeroToMax(int value, int m)
	{
		if (m == 0)
			return 0;

		return ((value % m) + m) % m;
	}

	bool curr_step_is_tied()
	{
		return enableTieNotes->getValue() && getSliderValueWithoutDisplay(lengthSliderPack, currentStep) == 100.0f;
	};

	bool next_step_will_be_tied()
	{
		int nextStep = incAndWrapValueFromZeroToMax(currentStep, stepSkipSlider->getValue(), lengthSliderPack->getNumSliders());

		return enableTieNotes->getValue() && getSliderValueWithoutDisplay(lengthSliderPack, nextStep) == 100.0f;

	};

	bool curr_step_is_skip()
	{
		return getSliderValueWithoutDisplay(lengthSliderPack, currentStep) == 0.0f;
	};

	void createLabel(const String& lName, const String& lContent, ScriptComponent* attachedComponent)
	{
		auto pos = attachedComponent->getPosition();

		auto lengthLabel = Content.addLabel(lName, pos.getX(), pos.getY() - 20);

		lengthLabel->set("text", lContent);
		lengthLabel->set("alignment", "left");
		lengthLabel->set("saveInPreset", false);
		lengthLabel->set("width", pos.getWidth());
		lengthLabel->set("height", 20);
		lengthLabel->set("fontName", "Oxygen");
		lengthLabel->set("fontStyle", "Bold");
		lengthLabel->set("editable", false);
		lengthLabel->set("multiline", false);
	}

	bool next_step_will_be_skipped()
	{
		int nextStep = incAndWrapValueFromZeroToMax(currentStep, stepSkipSlider->getValue(), lengthSliderPack->getNumSliders());

		return getSliderValueWithoutDisplay(lengthSliderPack, nextStep) == 0.0f;
	};

	static float getSliderValueWithoutDisplay(ScriptingApi::Content::ScriptSliderPack* sp, int index)
	{
		auto bf = static_cast<SliderPackData*>(sp->getCachedDataObject())->getDataArray().getBuffer();

		if (bf != nullptr && index < bf->size)
			return bf->getSample(index);
		else
		{
			//jassertfalse;
			return 0.0f;
		}
	}

	ScriptSliderPack semiToneSliderPack;
	ScriptSliderPack velocitySliderPack;
	ScriptSliderPack lengthSliderPack;
	ScriptButton bypassButton;
	ScriptSlider numStepSlider;
	ScriptButton sortKeysButton;
	ScriptSlider speedKnob;
	ScriptComboBox sequenceComboBox;
	ScriptSlider stepReset;
	ScriptSlider stepSkipSlider;
	ScriptButton resetButton;
	ScriptSlider currentStepSlider;
	ScriptSlider octaveSlider;
	ScriptSlider shuffleSlider;
	ScriptComboBox inputMidiChannel;
	ScriptComboBox outputMidiChannel;
	ScriptComboBox mpeStartChannel;
	ScriptComboBox mpeEndChannel;
	ScriptButton enableTieNotes;
	ScriptButton sustainHold;

	Range<uint16> lastEventIdRange;
	Array<uint16, DummyCriticalSection, 32> additionalChordStartKeys;
	double chordStartUptime = 0.0;

	Direction currentDirection = Direction::Up;

	

	bool sustainHoldActive = false;

	int mpeStart = 2;
	int mpeEnd = 16;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Arpeggiator);
};




} // namespace hise
#endif  // ARPEGGIATOR_H_INCLUDED
