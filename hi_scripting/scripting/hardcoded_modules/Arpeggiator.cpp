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
	HardcodedScriptProcessor(mc, id, ms)
{
	ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());

	onInit();

	mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
}

Arpeggiator::~Arpeggiator()
{
	mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);
}

int Arpeggiator::getNumSliderPacks()
{
	return 3;
}

SliderPackData * Arpeggiator::getSliderPackData(int index)
{
	switch (index)
	{
	case 0: return semiToneSliderPack->getSliderPackData();
	case 1: return velocitySliderPack->getSliderPackData();
	case 2: return lengthSliderPack->getSliderPackData();
	default: jassertfalse;
		return semiToneSliderPack->getSliderPackData();
	}
}

const SliderPackData * Arpeggiator::getSliderPackData(int index) const
{
	switch (index)
	{
	case 0: return semiToneSliderPack->getSliderPackData();
	case 1: return velocitySliderPack->getSliderPackData();
	case 2: return lengthSliderPack->getSliderPackData();
	default: jassertfalse;
		return semiToneSliderPack->getSliderPackData();
	}
}

void Arpeggiator::onInit()
{
	Content.setWidth(800);
	Content.setHeight(500);
	
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
	sequenceComboBox->set("items", "Up\nDown\nUp-Down\nDown-Up\nRandom");

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

	enableTieNotes = Content.addButton("EnableTie", 10, 70);
	enableTieNotes->set("text", "Enable Tie Notes");
	parameterNames.add("EnableTieNotes");

	auto bg = Content.addPanel("packBg", 150, 5);

	bg->set("width", 532);
	bg->set("height", 422);

	semiToneSliderPack = Content.addSliderPack("SemiToneSliderPack", 160, 30);
	semiToneSliderPack->getSliderPackData()->setDefaultValue(0.0);

	semiToneSliderPack->set("width", 512);
	semiToneSliderPack->set("min", -24);
	semiToneSliderPack->set("max", 24);
	semiToneSliderPack->set("sliderAmount", 4);
	semiToneSliderPack->set("stepSize", 1);
	velocitySliderPack = Content.addSliderPack("VelocitySliderPack", 160, 160);
	velocitySliderPack->getSliderPackData()->setDefaultValue(127.0);

	velocitySliderPack->set("width", 512);
	velocitySliderPack->set("min", 1);
	velocitySliderPack->set("max", 127);
	velocitySliderPack->set("sliderAmount", 4);
	velocitySliderPack->set("stepSize", "1");
	
	
	lengthSliderPack = Content.addSliderPack("LengthSliderPack", 160, 290);
	lengthSliderPack->getSliderPackData()->setDefaultValue(75.0);

	lengthSliderPack->set("width", 512);
	lengthSliderPack->set("max", 100);
	lengthSliderPack->set("sliderAmount", 4);
	lengthSliderPack->set("stepSize", "1");
	
	
	
	inputMidiChannel = Content.addComboBox("ChannelSelector", 300, 440);
	inputMidiChannel->set("text", "MIDI Channel");
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

	outputMidiChannel = Content.addComboBox("OutputChannelSelector", 300, 440 + 35);
	outputMidiChannel->set("text", "MIDI Channel");
	outputMidiChannel->addItem("All Channels");
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

	mpeStartChannel = Content.addComboBox("MPEStartChannel", 450, 440);
	mpeStartChannel->set("text", "MPE Start Channel");
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

	mpeEndChannel = Content.addComboBox("MPEEndChannel", 450, 440 + 35);
	mpeEndChannel->set("text", "MPE End Channel");
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

	auto noteLabel = Content.addLabel("NoteLabel", 160, 11);
	
	noteLabel->set("text", "Note Numbers");
	noteLabel->set("saveInPreset", false);
	noteLabel->set("width", 110);
	noteLabel->set("height", 20);
	noteLabel->set("fontName", "Oxygen");
	noteLabel->set("fontStyle", "Bold");
	noteLabel->set("editable", false);
	noteLabel->set("multiline", false);
	
	auto velocityLabel = Content.addLabel("VelocityLabel", 160, 140);

	velocityLabel->set("text", "Velocity");
	velocityLabel->set("saveInPreset", false);
	velocityLabel->set("width", 110);
	velocityLabel->set("height", 20);
	velocityLabel->set("fontName", "Oxygen");
	velocityLabel->set("fontStyle", "Bold");
	velocityLabel->set("editable", false);
	velocityLabel->set("multiline", false);

	auto lengthLabel = Content.addLabel("LengthLabel", 160, 270);

	lengthLabel->set("text", "Note Length");
	lengthLabel->set("saveInPreset", false);
	lengthLabel->set("width", 110);
	lengthLabel->set("height", 20);
	lengthLabel->set("fontName", "Oxygen");
	lengthLabel->set("fontStyle", "Bold");
	lengthLabel->set("editable", false);
	lengthLabel->set("multiline", false);
	
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

	addUserHeldKey({(int8)Message.getNoteNumber(), (int8)Message.getChannel()});

	// do not call playNote() if timer is already running
	if (!is_playing)
		playNote();
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
		if ((double)value > 0.5)
		{
			clearUserHeldKeys();
			reset(true, true);
		}
	}
	else if (c == resetButton)
	{
		clearUserHeldKeys();
		reset(true, true);
	}
	else if (c == sequenceComboBox)
	{
		changeDirection();
	}
	else if (c == inputMidiChannel)
	{
		reset(true, false);

		channelFilter = (int)value - 1;

		killIncomingNotes = channelFilter == 0 || midiChannel == channelFilter;

	}
	else if (c == outputMidiChannel)
	{
		reset(true, false);

		midiChannel = jmax<int>(1, (int)value);

		killIncomingNotes = midiChannel == 0 || midiChannel == channelFilter;
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
}

void Arpeggiator::onController()
{
	if (bypassButton->getValue())
		return;

	if (mpeMode)
	{
		const auto& m = *getCurrentHiseEvent();

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
	// get time scale for timer
	calcTimeInterval();

	// start synth timer
	start();

	// transfer user held keys to midi sequence
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
	// otherwise just keep curMasterStep at 0
	else
	{
		curMasterStep = 0;
	}

	/* --- GET CURRENT SETP VARIABLES --- */

	// get note either from the sorted sequence array or the non-sorted
	currentNote = sortKeysButton->getValue() ? MidiSequenceArraySorted[curHeldNoteIdx] : MidiSequenceArray[curHeldNoteIdx];
	

	

	// add semitone offset if enabled
	if (do_use_step_semitone_offsets) currentNote += (int8)semiToneSliderPack->getSliderValueAt(currentStep);

	// get step velocity
	currentVelocity = (int)velocitySliderPack->getSliderValueAt(currentStep);

	// get step gate length		
	currentNoteLengthInSamples = (int)(Engine.getSamplesForMilliSeconds(timeInterval * 1000.0) * (double)lengthSliderPack->getSliderValueAt(currentStep) / 100.0);

	

	/* --- PLAY NOTE LOGIC --- */

	// use this PLAY NOTE logic only if there's more than 1 note in the sequence
	if (MidiSequenceArray.size() > 1 && !curr_step_is_skip())
	{
		// this means we are coming from the 1 note logic and we need to end the last note
		if (currentlyPlayingEventIds.size() > 0)
		{
			for (int i = 0; i < currentlyPlayingEventIds.size(); ++i)
			{
				auto eventId = currentlyPlayingEventIds[i];


				sendNoteOff(eventId);

			}

			currentlyPlayingEventIds.clearQuick();
		}

		last_step_was_tied = false;

		const int onId = sendNoteOn();

		// add a little bit of time to the note off for a tied step to engage the synth's legato features
		// only do it if next step is not going to be skipped
		if (curr_step_is_tied() && !next_step_will_be_skipped())
		{
			currentNoteLengthInSamples += minNoteLenSamples;
		}

		Synth.noteOffDelayedByEventId(onId, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));

		//Synth.addNoteOff(midiChannel, currentNote, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
	}
	// use this PLAY NOTE only if there's 1 note in the sequence because we need to do a "hold note" kind of logic
	else if (MidiSequenceArray.size() == 1)
	{
		// if last step was not tied, add new note
		if (!last_step_was_tied && !curr_step_is_skip())
		{
			const int onId = sendNoteOn();

			currentlyPlayingEventIds.add(onId);
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

			for (int i = 0; i < currentlyPlayingEventIds.size(); ++i)
			{
				//Synth.addNoteOff(midiChannel, currentlyPlayingEventIds[i], jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
				Synth.noteOffDelayedByEventId(currentlyPlayingEventIds[i], currentNoteLengthInSamples);
			}

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

	for (int i = 0; i < currentlyPlayingEventIds.size(); ++i)
	{
		//Synth.addNoteOff(midiChannel, currentlyPlayingEventIds[i], );
		Synth.noteOffDelayedByEventId(currentlyPlayingEventIds[i], l);
	}

	currentlyPlayingEventIds.clearQuick();
}

void Arpeggiator::sendNoteOff(int eventId)
{
	Synth.noteOffDelayedByEventId(eventId, minNoteLenSamples);
}

int Arpeggiator::sendNoteOn()
{
	//const int shuffleTimeStamp = (currentStep % 2 != 0) ? (int)(0.8 * (double)currentNoteLengthInSamples * (double)shuffleSlider->getValue()) : 0;

	const int eventId = Synth.addNoteOn(mpeMode ? currentNote.channel : midiChannel, currentNote.noteNumber, currentVelocity, 0);

	if (mpeMode)
	{
		auto& ce = *getCurrentHiseEvent();

		auto timestamp = (int)ce.getTimeStamp();// +shuffleTimeStamp;

		auto c = currentNote.channel;

		HiseEvent pressValue(HiseEvent::Type::Aftertouch, mpeValues.pressValues[c], 0, c);
		HiseEvent slideValue(HiseEvent::Type::Controller, 74, mpeValues.slideValues[c], c);
		HiseEvent glideValue(HiseEvent::Type::PitchBend, 0, 0, c);
		glideValue.setPitchWheelValue(mpeValues.glideValues[c]);

		
		slideValue.setTimeStamp(timestamp);
		glideValue.setTimeStamp(timestamp);
		pressValue.setTimeStamp(timestamp);

		addHiseEventToBuffer(slideValue);
		addHiseEventToBuffer(glideValue);
		addHiseEventToBuffer(pressValue);
	}

	return eventId;
}





void Arpeggiator::changeDirection()
{
	switch ((int)sequenceComboBox->getValue())
	{
	case enumSeqUP:
		arpDirMod = 1;
		randomOrder = false;
		break;
	case enumSeqDN:
		arpDirMod = -1;
		randomOrder = false;
		break;
	case enumSeqDNUP:
	case enumSeqUPDN:
		randomOrder = false;
		break;
	case enumSeqRND:
		randomOrder = true;
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

	userHeldKeysArray.add(note);
	userHeldKeysArraySorted.add(note);
	userHeldKeysArraySorted.sort();
}

void Arpeggiator::remUserHeldKey(const NoteWithChannel& note)
{
	userHeldKeysArray.removeFirstMatchingValue(note);
	userHeldKeysArraySorted.removeFirstMatchingValue(note);
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
	
	switch ((int)sequenceComboBox->getValue())
	{
	case enumSeqUP:
	case enumSeqUPDN:
		arpDirMod = 1;
		curHeldNoteIdx = 0;
		break;
	case enumSeqDN:
	case enumSeqDNUP:
		arpDirMod = -1;
		curHeldNoteIdx = MidiSequenceArray.size() - 1;
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
	stopCurrentNote();

	Synth.stopTimer();
	is_playing = false;
	shuffleNextNote = false;
}

} // namespace hise
