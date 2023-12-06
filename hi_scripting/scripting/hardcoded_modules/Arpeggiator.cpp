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

#define LOG_ARP(x)

Arpeggiator::Arpeggiator(MainController *mc, const String &id, ModulatorSynth *ms) :
	HardcodedScriptProcessor(mc, id, ms),
	hise::Processor::BypassListener(mc->getRootDispatcher())
{
	addBypassListener(this, dispatch::sendNotificationSync);

	ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());

	onInit();

	mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
}

Arpeggiator::~Arpeggiator()
{
	removeBypassListener(this);

	mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
}

void Arpeggiator::mpeModeChanged(bool isEnabled)
{
	try
	{
		mpeMode = isEnabled; reset(true, true);
	}
	catch (String& m)
	{
#if USE_BACKEND
		debugError(this, m);
#endif
	}
}

void Arpeggiator::onInit()
{
	Content.setWidth(800);
	Content.setHeight(550);
	
	userHeldKeysArray.ensureStorageAllocated(128);
	userHeldKeysArraySorted.ensureStorageAllocated(128);
	MidiSequenceArray.ensureStorageAllocated(128);
	MidiSequenceArraySorted.ensureStorageAllocated(128);
	currentlyPlayingEventIds.ensureStorageAllocated(128);

	minNoteLenSamples = (int)(Engine.getSampleRate() / 80.0);

	bypassButton = Content.addButton("BypassButton", 0, 10);
	bypassButton->set("width", 74);
	bypassButton->set("text", "Bypass");
	
	parameterNames.add("Bypass");

	resetButton = Content.addButton("ResetButton", 74, 10);

	resetButton->set("isMomentary", true);
	resetButton->set("width", 64);
	resetButton->set("text", "Reset");

	parameterNames.add("Reset");


	numStepSlider = Content.addKnob("NumStepSlider", 10, 110);

	numStepSlider->set("text", "Num Steps");
	numStepSlider->set("min", 1);
	numStepSlider->set("max", 128);
	numStepSlider->set("stepSize", "1");
	numStepSlider->set("defaultValue", "4");

	parameterNames.add("NumSteps");

	stepReset = Content.addKnob("StepReset", 10, 170);

	stepReset->set("text", "Step Reset");
	stepReset->set("max", 128);
	stepReset->set("stepSize", "1");

	parameterNames.add("StepReset");
	


	stepSkipSlider = Content.addKnob("StepSkipSlider", 10, 230);

	stepSkipSlider->set("text", "Stride");
	stepSkipSlider->set("min", -12);
	stepSkipSlider->set("max", 12);
	stepSkipSlider->set("stepSize", "1");
	stepSkipSlider->set("middlePosition", 0);

	parameterNames.add("Stride");


	sortKeysButton = Content.addButton("SortKeysButton", 10, 300);
	sortKeysButton->set("text", "Sort Keys");

	parameterNames.add("SortKeys");

	speedKnob = Content.addKnob("SpeedKnob", 10, 340);

	speedKnob->set("mode", "TempoSync");

	speedKnob->set("text", "Speed");
	

	parameterNames.add("Tempo");

	sequenceComboBox = Content.addComboBox("SequenceComboBox", 10, 410);

	sequenceComboBox->set("text", "Direction");
	sequenceComboBox->set("items", "Up\nDown\nUp-Down\nDown-Up\nRandom\nChords");

	parameterNames.add("Direction");

	octaveSlider = Content.addKnob("OctaveRange", 10, 445);

	octaveSlider->set("min", -2);
	octaveSlider->set("max", 4);
	octaveSlider->set("stepSize", "1");

	parameterNames.add("OctaveRange");

	shuffleSlider = Content.addKnob("Shuffle", 150, 445);
	shuffleSlider->set("mode", "NormalizedPercentage");

	parameterNames.add("Shuffle");

	currentStepSlider = Content.addKnob("CurrentValue", 160, 400);

	currentStepSlider->set("width", 512);
	currentStepSlider->set("height", 16);
	currentStepSlider->set("max", 4);
	currentStepSlider->set("bgColour", 335544319);
	currentStepSlider->set("itemColour", 687865855);
	currentStepSlider->set("itemColour2", 0);
	currentStepSlider->set("saveInPreset", false);
	currentStepSlider->set("style", "Horizontal");
	currentStepSlider->set("enabled", false);
	currentStepSlider->set("stepSize", "1");

	parameterNames.add("CurrentStep");

	enableTieNotes = Content.addButton("EnableTie", 0, 70);
	enableTieNotes->set("text", "Tie 100%");
	parameterNames.add("EnableTieNotes");

	
	auto bg = Content.addPanel("packBg", 150, 5);

	bg->set("width", 532);
	bg->set("height", 422);

	semiToneSliderPack = Content.addSliderPack("SemiToneSliderPack", 160, 30);
	semiToneSliderPack->registerComplexDataObjectAtParent(0);

	semiToneSliderPack->getSliderPackData()->setDefaultValue(0.0);

	semiToneSliderPack->set("width", 512);
	semiToneSliderPack->set("min", -24);
	semiToneSliderPack->set("max", 24);
	semiToneSliderPack->set("sliderAmount", 4);
	semiToneSliderPack->set("stepSize", 1);
	velocitySliderPack = Content.addSliderPack("VelocitySliderPack", 160, 160);
	velocitySliderPack->getSliderPackData()->setDefaultValue(127.0);
	velocitySliderPack->registerComplexDataObjectAtParent(1);

	velocitySliderPack->set("width", 512);
	velocitySliderPack->set("min", 1);
	velocitySliderPack->set("max", 127);
	velocitySliderPack->set("sliderAmount", 4);
	velocitySliderPack->set("stepSize", "1");
	
	
	lengthSliderPack = Content.addSliderPack("LengthSliderPack", 160, 290);
	lengthSliderPack->getSliderPackData()->setDefaultValue(75.0);
	lengthSliderPack->registerComplexDataObjectAtParent(2);

	lengthSliderPack->set("width", 512);
	lengthSliderPack->set("max", 100);
	lengthSliderPack->set("sliderAmount", 4);
	lengthSliderPack->set("stepSize", "1");
	
	inputMidiChannel = Content.addComboBox("ChannelSelector", 300, 460);
	inputMidiChannel->set("items", "");
	parameterNames.add("InputChannel");
	inputMidiChannel->addItem("All Channels");
	inputMidiChannel->addItem("Channel 1");
	inputMidiChannel->addItem("Channel 2");
	inputMidiChannel->addItem("Channel 3");
	inputMidiChannel->addItem("Channel 4");
	inputMidiChannel->addItem("Channel 5"); 
	inputMidiChannel->addItem("Channel 6");
	inputMidiChannel->addItem("Channel 7");
	inputMidiChannel->addItem("Channel 8");
	inputMidiChannel->addItem("Channel 9");
	inputMidiChannel->addItem("Channel 10");
	inputMidiChannel->addItem("Channel 11");
	inputMidiChannel->addItem("Channel 12");
	inputMidiChannel->addItem("Channel 13");
	inputMidiChannel->addItem("Channel 14");
	inputMidiChannel->addItem("Channel 15");
	inputMidiChannel->addItem("Channel 16");

	outputMidiChannel = Content.addComboBox("OutputChannelSelector", 300, 460 + 55);
	parameterNames.add("OutputChannel");
	outputMidiChannel->set("items", "");
	outputMidiChannel->addItem("Use input channel");
	outputMidiChannel->addItem("Channel 1");
	outputMidiChannel->addItem("Channel 2");
	outputMidiChannel->addItem("Channel 3");
	outputMidiChannel->addItem("Channel 4");
	outputMidiChannel->addItem("Channel 5");
	outputMidiChannel->addItem("Channel 6");
	outputMidiChannel->addItem("Channel 7");
	outputMidiChannel->addItem("Channel 8");
	outputMidiChannel->addItem("Channel 9");
	outputMidiChannel->addItem("Channel 10");
	outputMidiChannel->addItem("Channel 11");
	outputMidiChannel->addItem("Channel 12");
	outputMidiChannel->addItem("Channel 13");
	outputMidiChannel->addItem("Channel 14");
	outputMidiChannel->addItem("Channel 15");
	outputMidiChannel->addItem("Channel 16");

	mpeStartChannel = Content.addComboBox("MPEStartChannel", 450, 460);
	parameterNames.add("MPEStartChannel");
	mpeStartChannel->set("items", "");
	mpeStartChannel->addItem("Inactive");
	mpeStartChannel->addItem("Channel 2");
	mpeStartChannel->addItem("Channel 3");
	mpeStartChannel->addItem("Channel 4");
	mpeStartChannel->addItem("Channel 5");
	mpeStartChannel->addItem("Channel 6");
	mpeStartChannel->addItem("Channel 7");
	mpeStartChannel->addItem("Channel 8");
	mpeStartChannel->addItem("Channel 9");
	mpeStartChannel->addItem("Channel 10");
	mpeStartChannel->addItem("Channel 11");
	mpeStartChannel->addItem("Channel 12");
	mpeStartChannel->addItem("Channel 13");
	mpeStartChannel->addItem("Channel 14");
	mpeStartChannel->addItem("Channel 15");
	mpeStartChannel->addItem("Channel 16");

	mpeEndChannel = Content.addComboBox("MPEEndChannel", 450, 460 + 55);
	mpeEndChannel->set("text", "MPE End Channel");
	mpeEndChannel->set("items", "");
	mpeEndChannel->addItem("Inactive");
	mpeEndChannel->addItem("Channel 2");
	mpeEndChannel->addItem("Channel 3");
	mpeEndChannel->addItem("Channel 4");
	mpeEndChannel->addItem("Channel 5");
	mpeEndChannel->addItem("Channel 6");
	mpeEndChannel->addItem("Channel 7");
	mpeEndChannel->addItem("Channel 8");
	mpeEndChannel->addItem("Channel 9");
	mpeEndChannel->addItem("Channel 10");
	mpeEndChannel->addItem("Channel 11");
	mpeEndChannel->addItem("Channel 12");
	mpeEndChannel->addItem("Channel 13");
	mpeEndChannel->addItem("Channel 14");
	mpeEndChannel->addItem("Channel 15");
	mpeEndChannel->addItem("Channel 16");

	sustainHold = Content.addButton("Hold", 85, 70);
	
	parameterNames.add("Hold");
	enableTieNotes->set("width", 85);
	sustainHold->set("width", 60);

	createLabel("NoteLabel", "Note Numbers", semiToneSliderPack);
	createLabel("VeloLabel", "Velocity", velocitySliderPack);
	createLabel("LengthLabel", "Note Length", lengthSliderPack);
	createLabel("MIDILabel1", "Input Channel", inputMidiChannel);
	createLabel("MIDILabel2", "Output Channel", outputMidiChannel);
	createLabel("MIDILabel3", "MPE Start Channel", mpeStartChannel);
	createLabel("MIDILabel4", "MPE End Channel", mpeEndChannel);

	stepSkipSlider->setValue(1);
	sequenceComboBox->setValue(1);
	speedKnob->setValue((int)TempoSyncer::Tempo::Sixteenth); 
	resetButton->setValue(0);
	numStepSlider->setValue(4);
	currentStepSlider->setValue(0);
	octaveSlider->setValue(0);
	inputMidiChannel->setValue(1);
	outputMidiChannel->setValue(1);
	mpeStartChannel->setValue(2);
	mpeEndChannel->setValue(16);
	enableTieNotes->setValue(1);

	velocitySliderPack->setAllValues(127);
	lengthSliderPack->setAllValues(75);
	semiToneSliderPack->setAllValues(0);

	resetButton->set("tooltip", "Stops the current sequence if pressed");

	enableTieNotes->set("tooltip", "Ties steps with 100% length to the next note");
	sustainHold->set("tooltip", "Holds the sequence if the sustain pedal is pressed");
	inputMidiChannel->set("tooltip", "The MIDI channel that is fed into the arpeggiator.");
	outputMidiChannel->set("tooltip", "The MIDI channel that is used for the arpeggiated notes");

	updateParameterSlots();
}

void Arpeggiator::onNoteOn()
{
	if (bypassButton->getValue())
		return;

	int channel = Message.getChannel();

	if (shouldFilterMessage(channel))
		return;

	if (mpeMode)
	{
		mpeValues.glideValues[channel] = 8192;
		mpeValues.pressValues[channel] = 0;
		mpeValues.slideValues[channel] = 64;
	}

	if(killIncomingNotes || mpeMode)
		Message.ignoreEvent(true);

	minNoteLenSamples = (int)(Engine.getSampleRate() / 80.0);

	NoteWithChannel newNote = { (int8)Message.getNoteNumber(), (int8)Message.getChannel() };

	addUserHeldKey(newNote);

	if (is_playing && currentDirection == Direction::Chord && (Engine.getUptime() - chordStartUptime < 0.02))
	{
		// Here we land if the chord mode has just been started but the
		// arp is already playing, so we need to manually play the note.
		newNote += (int8)semiToneSliderPack->getSliderValueAt(currentStep);
		int thisId = sendNoteOnInternal(newNote);
		Synth.noteOffDelayedByEventId(thisId, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));

		additionalChordStartKeys.add(thisId);
	}

	// do not call playNote() if timer is already running
	if (!is_playing)
	{
		if (currentDirection == Direction::Chord)
			chordStartUptime = Engine.getUptime();

		playNote();
	}
		
}

void Arpeggiator::onNoteOff()
{
	auto channel = Message.getChannel();

	if (shouldFilterMessage(channel))
		return;

	if (bypassButton->getValue())
		return;

	if(killIncomingNotes || mpeMode)
		Message.ignoreEvent(true);

	remUserHeldKey({ (int8)Message.getNoteNumber(), (int8)channel });

	if (!keys_are_held())
	{
		// reset arpeggiator if no midi input / user held keys
		reset(false, true);
	}

	
}

void Arpeggiator::onControl(ScriptingApi::Content::ScriptComponent *c, var value)
{
	if (c == numStepSlider)
	{
		int newNumber = jlimit<int>(1, 128, (int)value);

		lengthSliderPack->set("sliderAmount", newNumber);
		velocitySliderPack->set("sliderAmount", newNumber);
		semiToneSliderPack->set("sliderAmount", newNumber);
		currentStepSlider->set("max", newNumber);
	}
	else if (c == bypassButton)
	{
		clearUserHeldKeys();
		reset(true, true);
	}
	else if (c == resetButton)
	{
		clearUserHeldKeys();
		reset(true, true);
	}
	else if (c == sequenceComboBox)
	{
		currentDirection = (Direction)(int)sequenceComboBox->getValue();
		changeDirection();
	}
	else if (c == inputMidiChannel)
	{
		reset(true, false);

		channelFilter = (int)value - 1;

		killIncomingNotes = midiChannel == 0 || midiChannel == channelFilter;

	}
	else if (c == outputMidiChannel)
	{
		reset(true, false);

		//midiChannel = jmax<int>(1, (int)value);
		midiChannel = (int)value - 1;

		killIncomingNotes = midiChannel == 0 || midiChannel == channelFilter;
	}
	else if (c == sustainHold)
	{
		auto newActive = (bool)value;

		if (newActive != sustainHoldActive)
		{
			if (sustainHoldActive)
			{
				// the pedal is released here,
				// remove all notes from the playing queue

				for (auto& s : sustainHoldKeys)
				{
					userHeldKeysArray.removeFirstMatchingValue(s);
					userHeldKeysArraySorted.removeFirstMatchingValue(s);
				}

				sustainHoldKeys.clearQuick();

				if (!keys_are_held())
					reset(false, true);
			}

			sustainHoldActive = newActive;
		}
	}
	else if (c == mpeStartChannel || c == mpeEndChannel)
	{
		mpeStart = (int)mpeStartChannel->getValue();
		mpeEnd = (int)mpeEndChannel->getValue();

		if (mpeStart == 1 || mpeEnd == 1)
		{
			mpeStart = 2;
			mpeEnd = 16;
		}
	}
	
	else if (c == currentStepSlider)
	{
		currentStep = jlimit<int>(0, velocitySliderPack->getNumSliders()-1, (int)value);
		curMasterStep = currentStep;
	}
}

void Arpeggiator::onController()
{
	if (bypassButton->getValue())
		return;

	const auto& m = *getCurrentHiseEvent();

	

	if (mpeMode)
	{
		

		auto c = m.getChannel();

		if (m.isNoteOn())					mpeValues.strokeValues[c] = (int8)m.getVelocity();
		else if (m.isChannelPressure())		mpeValues.pressValues[c] = (int8)m.getNoteNumber();
		else if (m.isControllerOfType(74))	mpeValues.slideValues[c] = (int8)m.getControllerValue();
		else if (m.isPitchWheel())			mpeValues.glideValues[c] = (int16)m.getPitchWheelValue();
		else if (m.isNoteOff())				mpeValues.liftValues[c] = (int8)m.getVelocity();
	}
}

void Arpeggiator::onAllNotesOff()
{
	if (bypassButton->getValue())
		return;

	clearUserHeldKeys();
	reset(false, true);
}

void Arpeggiator::onTimer(int /*offsetInBuffer*/)
{
	if (bypassButton->getValue())
		return;

	if (keys_are_held())
	{
		// do work if user is holding notes
		playNote();
	}
}


void Arpeggiator::playNote()
{
	calcTimeInterval();
	start();

	additionalChordStartKeys.clearQuick();
	MidiSequenceArray.clearQuick();
	MidiSequenceArraySorted.clearQuick();

	const int octaveRaw = (int)octaveSlider->getValue();
	const int octaveSign = octaveRaw >= 0 ? 1 : -1;
	auto octaveAmount = abs(octaveRaw) + 1;

	for (int i = 0; i < octaveAmount; i++)
	{
		for (int j = 0; j < userHeldKeysArray.size(); j++)
		{
			MidiSequenceArray.addIfNotAlreadyThere(userHeldKeysArray[j] + (int8)(octaveSign * i * 12));
			MidiSequenceArraySorted.addIfNotAlreadyThere(userHeldKeysArraySorted[j] + (int8)(octaveSign * i * 12));
		}
	}

	if (randomOrder)
	{
		int newIndex = MidiSequenceArraySorted.size() == 0 ? 0 : r.nextInt(MidiSequenceArraySorted.size());

		while (curHeldNoteIdx == newIndex && MidiSequenceArraySorted.size() > 2)
			newIndex = r.nextInt(MidiSequenceArraySorted.size());

		curHeldNoteIdx = newIndex;
	}
	else
	{
		// this ensure that if a new note is added to the sorted list, we update where we are supposed to be along the sequence
		// not a good implementation of the idea, but it works for now
		if (sortKeysButton->getValue() && curHeldNoteIdx > 1)
			curHeldNoteIdx = MidiSequenceArraySorted.indexOf(currentNote) + arpDirMod;

		// we must increment and wrap separately otherwise we will go from 0 to 1 and back to 0 instantly unless more than one note is held at exactly the same time
		curHeldNoteIdx = WrapValueFromZeroToMax(curHeldNoteIdx, MidiSequenceArray.size());

		curSeqPatternEnum = sequenceComboBox->getValue();

		// check if we need to reverse sequence based on up/dn mode
		if ((int)sequenceComboBox->getValue() >= 3 && MidiSequenceArray.size() > 1)
		{
			if (arpDirMod > 0 && curHeldNoteIdx == MidiSequenceArray.size() - 1)
				arpDirMod = -abs(arpDirMod);
			else if (arpDirMod < 0 && curHeldNoteIdx == 0)
				arpDirMod = abs(arpDirMod);
		}
	}

	const int stepResetAmount = (int)stepReset->getValue();

	// if master reset num steps is 0 ignore this block, otherwise do a full arp reset when num steps have passed
	if (stepResetAmount > 0)
	{
		if (curMasterStep >= stepResetAmount)
			reset(false, false);
	}
	else
		curMasterStep = 0; // otherwise just keep curMasterStep at 0

	currentNote = sortKeysButton->getValue() ? MidiSequenceArraySorted[curHeldNoteIdx] : MidiSequenceArray[curHeldNoteIdx];
	currentNote += (int8)semiToneSliderPack->getSliderValueAt(currentStep);
	currentVelocity = (int)velocitySliderPack->getSliderValueAt(currentStep);
	currentNoteLengthInSamples = (int)(Engine.getSamplesForMilliSeconds(timeInterval * 1000.0) * (double)lengthSliderPack->getSliderValueAt(currentStep) / 100.0);

	/* --- PLAY NOTE LOGIC --- */

	// use this PLAY NOTE logic only if there's more than 1 note in the sequence
	if (MidiSequenceArray.size() > 1 && !curr_step_is_skip())
	{
		// this means we are coming from the 1 note logic and we need to end the last note
		if (!currentlyPlayingEventIds.isEmpty())
		{
			for (auto& id : currentlyPlayingEventIds)
				sendNoteOff(id);

			currentlyPlayingEventIds.clearQuick();
		}

		last_step_was_tied = false;
		lastEventIdRange = sendNoteOn();

		// add a little bit of time to the note off for a tied step to engage the synth's legato features
		// only do it if next step is not going to be skipped
		if (curr_step_is_tied() && !next_step_will_be_skipped())
		{
			currentNoteLengthInSamples += minNoteLenSamples;
		}

		for(int i = lastEventIdRange.getStart(); i < lastEventIdRange.getEnd(); i++)
			Synth.noteOffDelayedByEventId(i, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));

		//Synth.addNoteOff(midiChannel, currentNote, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
	}
	// use this PLAY NOTE only if there's 1 note in the sequence because we need to do a "hold note" kind of logic
	else if (MidiSequenceArray.size() == 1)
	{
		// if last step was not tied, add new note
		if (!last_step_was_tied && !curr_step_is_skip())
		{
			lastEventIdRange = sendNoteOn();

			for(int i = lastEventIdRange.getStart(); i < lastEventIdRange.getEnd(); i++)
				currentlyPlayingEventIds.add(i);
		}

		last_step_was_tied = false;

		// if current step is not tied, schedule an end note time
		if (curr_step_is_tied())
		{
			last_step_was_tied = true;
		}
		else
		{
			stopCurrentNote();

		}

		if (next_step_will_be_skipped())
		{
			last_step_was_tied = false;

			for(auto& id: currentlyPlayingEventIds)
				Synth.noteOffDelayedByEventId(id, currentNoteLengthInSamples);

			currentlyPlayingEventIds.clearQuick();
		}
	}

	/* --- INCREMENTORS --- */

	if (!randomOrder)
	{
		// increment and wraparound user held note index
		curHeldNoteIdx += arpDirMod;
	}

	currentStepSlider->setValue(currentStep + 1);

	// increment and wrap around current step index
	currentStep = incAndWrapValueFromZeroToMax((int)stepSkipSlider->getValue(), currentStep, numStepSlider->getValue());

	// increment master step to know when to do a full sequence reset, don't do this is the feature is turned off i.e. numStepsBeforeReset > 0
	if ((int)stepReset->getValue() > 0) ++curMasterStep;
}

void Arpeggiator::stopCurrentNote()
{
	auto l = jmax<int>(minNoteLenSamples, currentNoteLengthInSamples);

	for(auto& id: currentlyPlayingEventIds)
		Synth.noteOffDelayedByEventId(id, l);

	currentlyPlayingEventIds.clearQuick();
}

void Arpeggiator::sendNoteOff(int eventId)
{
	Synth.noteOffDelayedByEventId(eventId, minNoteLenSamples);
}

Range<uint16> Arpeggiator::sendNoteOn()
{
	if (currentDirection != Direction::Chord)
	{
		auto id = sendNoteOnInternal(currentNote);

		return { id, (uint16)(id + 1) };
	}
		
	else
	{
		uint16 firstId = 0;
		uint16 lastId = 0;

		for (auto& c : MidiSequenceArraySorted)
		{
			c += (int8)semiToneSliderPack->getSliderValueAt(currentStep);

			auto thisId = sendNoteOnInternal(c);

			if (firstId == 0)
				firstId = thisId;
			else
			{
				// Needs to be consecutive...
				jassert(thisId - lastId == 1);
			}
			
			lastId = thisId;
		}

		return { firstId, (uint16)(lastId + 1) };
	}
}





uint16 Arpeggiator::sendNoteOnInternal(const Arpeggiator::NoteWithChannel& c)
{
	int midiChannelToUse = mpeMode || midiChannel == 0 ? c.channel : midiChannel;

	auto eventId = (uint16)Synth.addNoteOn(midiChannelToUse, c.noteNumber, currentVelocity, 0);

	if (mpeMode)
	{
		auto& ce = *getCurrentHiseEvent();

		auto timestamp = (int)ce.getTimeStamp();// +shuffleTimeStamp;

		auto ch = c.channel;

		HiseEvent pressValue(HiseEvent::Type::Aftertouch, mpeValues.pressValues[ch], 0, ch);
		HiseEvent slideValue(HiseEvent::Type::Controller, 74, mpeValues.slideValues[ch], ch);
		HiseEvent glideValue(HiseEvent::Type::PitchBend, 0, 0, ch);
		glideValue.setPitchWheelValue(mpeValues.glideValues[ch]);

		slideValue.setTimeStamp(timestamp);
		glideValue.setTimeStamp(timestamp);
		pressValue.setTimeStamp(timestamp);

		addHiseEventToBuffer(slideValue);
		addHiseEventToBuffer(glideValue);
		addHiseEventToBuffer(pressValue);
	}

	return eventId;
}

void Arpeggiator::applySliderPackData(NoteWithChannel& c)
{

}

void Arpeggiator::changeDirection()
{
	switch (currentDirection)
	{
	case Direction::Up:
		arpDirMod = 1;
		randomOrder = false;
		break;
	case Direction::Down:
		arpDirMod = -1;
		randomOrder = false;
		break;
	case Direction::UpDown:
	case Direction::DownUp:
		randomOrder = false;
		break;
	case Direction::Random:
		randomOrder = true;
		break;
    case Direction::Chord:
        arpDirMod = 1;
        randomOrder = false;
        break;
    default:
        jassertfalse;
        break;
	}
}

void Arpeggiator::calcTimeInterval()
{
	//LOG_ARP("timeSigValArray[enum("+this.timeSigEnum+")] = " + this.timeSigValArray[this.timeSigEnum] + " / " + this.timeSigStrArray[this.timeSigEnum]);
	BPM = Engine.getHostBpm();
	BPS = BPM / 60.0;

	auto t = TempoSyncer::getTempoInMilliSeconds(BPM, (TempoSyncer::Tempo)(int)speedKnob->getValue()) * 0.001;
	
	timeInterval = jmax<double>(minTimerTime, t);
}

void Arpeggiator::addUserHeldKey(const NoteWithChannel& note)
{
	if (userHeldKeysArray.contains(note))
		return;

	// allow replaying the note to keep it ringing after the pedal was released
	if(sustainHoldActive)
		sustainHoldKeys.removeFirstMatchingValue(note);

	userHeldKeysArray.add(note);
	userHeldKeysArraySorted.add(note);
	userHeldKeysArraySorted.sort();
}

void Arpeggiator::remUserHeldKey(const NoteWithChannel& note)
{
	if (sustainHoldActive)
	{
		// keep it playing, but save it for later...
		sustainHoldKeys.addIfNotAlreadyThere(note);
	}
	else
	{
		userHeldKeysArray.removeFirstMatchingValue(note);
		userHeldKeysArraySorted.removeFirstMatchingValue(note);
	}
}

void Arpeggiator::clearUserHeldKeys()
{
	userHeldKeysArray.clearQuick();
	userHeldKeysArraySorted.clearQuick();
}

void Arpeggiator::reset(bool do_all_notes_off, bool do_stop)
{
	if (do_stop) stop();

	currentStep = 0;
	curMasterStep = 0;
	currentStepSlider->setValue(0);
	
	switch (currentDirection)
	{
	case Direction::Up:
	case Direction::UpDown:
    case Direction::Chord:
		arpDirMod = 1;
		curHeldNoteIdx = 0;
		break;
	case Direction::Down:
	case Direction::DownUp:
		arpDirMod = -1;
		curHeldNoteIdx = MidiSequenceArray.size() - 1;
		break;
	case Direction::Random:
		arpDirMod = 1;
		curHeldNoteIdx = 0;
		break;
    default:
        jassertfalse;
        break;
	}

	if (do_all_notes_off)
		Engine.allNotesOff();

	last_step_was_tied = false;

}

void Arpeggiator::start()
{
	auto shuffleAmount = 0.5 * (double)shuffleSlider->getValue();

	auto shuffleFactor = shuffleNextNote ? (1.0 - shuffleAmount) : (1.0 + shuffleAmount);

	shuffleNextNote = !shuffleNextNote;

	Synth.startTimer(timeInterval * shuffleFactor);
	is_playing = true;
}

void Arpeggiator::stop()
{
	for (int i = lastEventIdRange.getStart(); i < lastEventIdRange.getEnd(); i++)
		Synth.noteOffByEventId(i);

	for (auto& ac : additionalChordStartKeys)
		Synth.noteOffByEventId(ac);

	stopCurrentNote();

	Synth.stopTimer();
	is_playing = false;
	shuffleNextNote = false;
}

} // namespace hise
