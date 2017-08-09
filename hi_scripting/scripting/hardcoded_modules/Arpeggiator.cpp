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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


Arpeggiator::Arpeggiator(MainController *mc, const String &id, ModulatorSynth *ms) :
	HardcodedScriptProcessor(mc, id, ms)
{
	onInit();
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
	
	MidiSequenceArray.ensureStorageAllocated(128);
	MidiSequenceArraySorted.ensureStorageAllocated(128);
	userHeldKeysArray.ensureStorageAllocated(128);
	userHeldKeysArraySorted.ensureStorageAllocated(128);

	static const String tempoList = "1/1\n1/2D\n1/2\n1/2T\n1/4D\n1/4\n1/4T\n1/8D\n1/8\n1/8T\n1/16D\n1/16\n1/16T\n1/32D\n1/32\n1/32D\n1/64D\n1/64\n1/64T";

	timeSigValArray = createTempoDivisionValueArrayViaStringArray(tempoList);
	minNoteLenSamples = (int)(Engine.getSampleRate() / 80.0);

	bypassButton = Content.addButton("BypassButton", 10, 10);
	bypassButton->set("text", "Bypass");
	
	parameterNames.add("Bypass");

	syncHostButton = Content.addButton("SyncHostButton", 10, 50);
	syncHostButton->set("text", "Sync to Host");

	parameterNames.add("SyncToHost");

	internalBPMSlider = Content.addKnob("InternalBPMSlider", 10, 90);
	
	internalBPMSlider->set("text", "Internal BPM");
	internalBPMSlider->set("width", 130);
	internalBPMSlider->set("height", 50);
	internalBPMSlider->set("min", 40);
	internalBPMSlider->set("max", 180);
	internalBPMSlider->set("stepSize", "1");
	internalBPMSlider->set("defaultValue", "120");
	internalBPMSlider->set("suffix", " BPM");
	
	parameterNames.add("InternalBPM");



	numStepSlider = Content.addKnob("NumStepSlider", 10, 140);

	numStepSlider->set("text", "Num Steps");
	numStepSlider->set("min", 1);
	numStepSlider->set("max", 32);
	numStepSlider->set("stepSize", "1");
	numStepSlider->set("defaultValue", "4");

	parameterNames.add("NumSteps");

	stepReset = Content.addKnob("StepReset", 10, 200);

	stepReset->set("text", "Step Reset");
	stepReset->set("max", 32);
	stepReset->set("stepSize", "1");

	parameterNames.add("StepReset");
	


	stepSkipSlider = Content.addKnob("StepSkipSlider", 10, 260);

	stepSkipSlider->set("text", "Stride");
	stepSkipSlider->set("min", -12);
	stepSkipSlider->set("max", 12);
	stepSkipSlider->set("stepSize", "1");
	stepSkipSlider->set("middlePosition", 0);

	parameterNames.add("Stride");


	sortKeysButton = Content.addButton("SortKeysButton", 10, 330);
	sortKeysButton->set("text", "Sort Keys");

	parameterNames.add("SortKeys");

	speedComboBox = Content.addComboBox("SpeedComboBox", 10, 370);

	speedComboBox->set("text", "Speed");
	speedComboBox->set("items", tempoList);

	parameterNames.add("Tempo");

	sequenceComboBox = Content.addComboBox("SequenceComboBox", 10, 410);

	sequenceComboBox->set("text", "Direction");
	sequenceComboBox->set("items", "Up\nDown\nUp-Down\nDown-Up");

	parameterNames.add("Direction");

	octaveSlider = Content.addKnob("OctaveRange", 10, 445);

	octaveSlider->set("min", 0);
	octaveSlider->set("max", 4);
	octaveSlider->set("stepSize", "1");

	parameterNames.add("OctaveRange");

	shuffleSlider = Content.addKnob("Shuffle", 150, 445);
	

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

	

	auto bg = Content.addPanel("packBg", 150, 5);

	bg->set("width", 532);
	bg->set("height", 422);

	semiToneSliderPack = Content.addSliderPack("SemiToneSliderPack", 160, 30);
	
	semiToneSliderPack->set("width", 512);
	semiToneSliderPack->set("min", -24);
	semiToneSliderPack->set("max", 24);
	semiToneSliderPack->set("sliderAmount", 4);
	semiToneSliderPack->set("stepSize", 1);
	
	velocitySliderPack = Content.addSliderPack("VelocitySliderPack", 160, 160);
	
	velocitySliderPack->set("width", 512);
	velocitySliderPack->set("min", 1);
	velocitySliderPack->set("max", 127);
	velocitySliderPack->set("sliderAmount", 4);
	velocitySliderPack->set("stepSize", "1");
	
	lengthSliderPack = Content.addSliderPack("LengthSliderPack", 160, 290);
	
	lengthSliderPack->set("width", 512);
	lengthSliderPack->set("max", 100);
	lengthSliderPack->set("sliderAmount", 4);
	lengthSliderPack->set("stepSize", "1");
	
	
	

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

	

	internalBPMSlider->setValue(120);
	syncHostButton->setValue(false);
	stepSkipSlider->setValue(1);
	sequenceComboBox->setValue(1);
	speedComboBox->setValue(3); 
	numStepSlider->setValue(4);
	currentStepSlider->setValue(0);
	octaveSlider->setValue(0);

	velocitySliderPack->setAllValues(127);
	lengthSliderPack->setAllValues(75);
	semiToneSliderPack->setAllValues(0);
	

}

void Arpeggiator::onNoteOn()
{
	if (bypassButton->getValue())
		return;

	Message.ignoreEvent(true);

	addUserHeldKey(Message.getNoteNumber());

	// do not call playNote() if timer is already running
	if (!is_playing)
		playNote();
}

void Arpeggiator::onNoteOff()
{
	if (bypassButton->getValue())
		return;

	remUserHeldKey(Message.getNoteNumber());
}

void Arpeggiator::onControl(ScriptingApi::Content::ScriptComponent *c, var value)
{
	if (c == numStepSlider)
	{
		int newNumber = jmax<int>(1, (int)value);

		lengthSliderPack->set("sliderAmount", newNumber);
		velocitySliderPack->set("sliderAmount", newNumber);
		semiToneSliderPack->set("sliderAmount", newNumber);
		currentStepSlider->set("max", newNumber);
	}
	else if (c == bypassButton)
	{
		userHeldKeysArray.clearQuick();
		userHeldKeysArraySorted.clearQuick();

		reset(true, true);
	}
	else if (c == syncHostButton)
	{
		internalBPMSlider->set("enabled", 1 - (int)value);
	}
	else if (c == sequenceComboBox)
	{
		changeDirection();
	}
}

void Arpeggiator::onTimer(int /*offsetInBuffer*/)
{
	if (bypassButton->getValue())
		return;

	if (!keys_are_held())
	{
		// reset arpeggiator if no midi input / user held keys
		reset(true, true);
	}
	else
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

	auto octaveAmount = (int)octaveSlider->getValue() + 1;

	for (int i = 0; i < octaveAmount; i++)
	{
		for (int j = 0; j < userHeldKeysArray.size(); j++)
		{
			MidiSequenceArray.addIfNotAlreadyThere(userHeldKeysArray[j] + i * 12);
			MidiSequenceArraySorted.addIfNotAlreadyThere(userHeldKeysArraySorted[j] + i * 12);
		}
	}

	// this ensure that if a new note is added to the sorted list, we update where we are supposed to be along the sequence
	// not a good implementation of the idea, but it works for now
	if (sortKeysButton->getValue() && curHeldNoteIdx > 1) curHeldNoteIdx = MidiSequenceArraySorted.indexOf(currentNote) + arpDirMod;

	// we must increment and wrap separately otherwise we will go from 0 to 1 and back to 0 instantly unless more than one note is held at exactly the same time
	curHeldNoteIdx = WrapValueFromZeroToMax(curHeldNoteIdx, MidiSequenceArray.size());

	curSeqPatternEnum = sequenceComboBox->getValue();

	// check if we need to reverse sequence based on up/dn mode
	if ((int)sequenceComboBox->getValue() >= Direction::enumSeqUPDN && MidiSequenceArray.size() > 1)
	{
		if (arpDirMod > 0 && curHeldNoteIdx == MidiSequenceArray.size() - 1)
			arpDirMod = -abs(arpDirMod);
		else if (arpDirMod < 0 && curHeldNoteIdx == 0)
			arpDirMod = abs(arpDirMod);
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
	if (do_use_step_semitone_offsets) currentNote += (int)semiToneSliderPack->getSliderValueAt(currentStep);

	// get step velocity
	currentVelocity = (int)velocitySliderPack->getSliderValueAt(currentStep);

	// get step gate length		
	currentNoteLengthInSamples = (int)(Engine.getSamplesForMilliSeconds(timeInterval * 1000.0) * (double)lengthSliderPack->getSliderValueAt(currentStep) / 100.0);

	/* --- PLAY NOTE LOGIC --- */

	// use this PLAY NOTE logic only if there's more than 1 note in the sequence
	if (MidiSequenceArray.size() > 1 && !curr_step_is_skip())
	{
		// this means we are coming from the 1 note logic and we need to end the last note
		if (currentlyPlayingKeys.size() > 0)
		{
			for (int i = 0; i < currentlyPlayingKeys.size(); ++i)
			{
				Synth.addNoteOff(midiChannel, currentlyPlayingKeys[i], minNoteLenSamples);
			}

			currentlyPlayingKeys.clear();
		}

		last_step_was_tied = false;

		int shuffleTimeStamp = (currentStep % 2 != 0) ? (int)(0.8 * (double)currentNoteLengthInSamples * (double)shuffleSlider->getValue()) : 0;

		Synth.addNoteOn(midiChannel, currentNote, currentVelocity, shuffleTimeStamp);

		// add a little bit of time to the note off for a tied step to engage the synth's legato features
		// only do it if next step is not going to be skipped
		if (curr_step_is_tied() && !next_step_will_be_skipped())
		{
			currentNoteLengthInSamples += minNoteLenSamples;
		}

		Synth.addNoteOff(midiChannel, currentNote, jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
	}
	// use this PLAY NOTE only if there's 1 note in the sequence because we need to do a "hold note" kind of logic
	else if (MidiSequenceArray.size() == 1)
	{
		// if last step was not tied, add new note
		if (!last_step_was_tied && !curr_step_is_skip())
		{
			int shuffleTimeStamp = (currentStep % 2 != 0) ? (int)(0.8 * (double)currentNoteLengthInSamples * (double)shuffleSlider->getValue()) : 0;

			Synth.addNoteOn(midiChannel, currentNote, currentVelocity, shuffleTimeStamp);
			currentlyPlayingKeys.add(currentNote);
		}

		last_step_was_tied = false;

		// if current step is not tied, schedule an end note time
		if (curr_step_is_tied())
		{
			last_step_was_tied = true;
		}
		else
		{
			for (int i = 0; i < currentlyPlayingKeys.size(); ++i)
			{
				Synth.addNoteOff(midiChannel, currentlyPlayingKeys[i], jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
			}

			currentlyPlayingKeys.clear();
		}

		if (next_step_will_be_skipped())
		{
			last_step_was_tied = false;

			for (int i = 0; i < currentlyPlayingKeys.size(); ++i)
			{
				Synth.addNoteOff(midiChannel, currentlyPlayingKeys[i], jmax<int>(minNoteLenSamples, currentNoteLengthInSamples));
			}

			currentlyPlayingKeys.clear();
		}
	}

	/* --- INCREMENTORS --- */

	// increment and wraparound user held note index
	curHeldNoteIdx += arpDirMod;

	currentStepSlider->setValue(currentStep + 1);

	// increment and wrap around current step index
	currentStep = incAndWrapValueFromZeroToMax((int)stepSkipSlider->getValue(), currentStep, numStepSlider->getValue());



	// increment master step to know when to do a full sequence reset, don't do this is the feature is turned off i.e. numStepsBeforeReset > 0
	if ((int)stepReset->getValue() > 0) ++curMasterStep;
}




Array<double> Arpeggiator::createTempoDivisionValueArrayViaStringArray(const String& tempoValues)
{
	StringArray timeSignatureStrArray = StringArray::fromLines(tempoValues);

	DBG("====");
	DBG("createTempoDivisionValueArrayViaStringArray(timeSignatureStrArray)");
	DBG("----");

	int numerator;
	int denominator;
	double modifier;
	String modifier_str;
	double result;
	StringArray regexArray;
	Array<double> collection;

	for (int i = 0; i < timeSignatureStrArray.size(); ++i)
	{
		regexArray = RegexFunctions::getFirstMatch("(\\d+)/(\\d+)(.?)", timeSignatureStrArray[i]);

		DBG("Matched " + String(regexArray.size() - 1) + " regex groups after parsing time signatre.");

		if (regexArray.isEmpty())
			continue;

		numerator = regexArray[1].getIntValue();
		denominator = regexArray[2].getIntValue();

		if (denominator == 0)
		{
			// Hello, my name is C++ and I'll crash at trying to divide by zero...
			continue;
		}

		modifier_str = regexArray[3];

		if (RegexFunctions::matchesWildcard("[\\.dD]", modifier_str))
			modifier = 1.5;
		else if (RegexFunctions::matchesWildcard("[tT]", modifier_str))
			modifier = 1.0 / 1.5;
		else
			modifier = 1.0;

		result = (double)numerator / (double)denominator * modifier;
		result = (double)result * 4.0; // because music theory

		collection.add(result);

		DBG(timeSignatureStrArray[i] + " > is parsed as " + regexArray[1] + "/" + regexArray[2] + regexArray[3]);
		//DBG(result + " = " + String(numerator + " / " + denominator + " * " + modifier);
		DBG("----");
	}
	DBG("Input had " + String(timeSignatureStrArray.size()) + " elements");
	DBG("====");

	return collection;
}

void Arpeggiator::changeDirection()
{
	switch ((int)sequenceComboBox->getValue())
	{
	case enumSeqUP:
		arpDirMod = 1;
		break;
	case enumSeqDN:
		arpDirMod = -1;
		break;
	}
}

void Arpeggiator::calcTimeInterval()
{
	//DBG("timeSigValArray[enum("+this.timeSigEnum+")] = " + this.timeSigValArray[this.timeSigEnum] + " / " + this.timeSigStrArray[this.timeSigEnum]);
	BPM = ((int)syncHostButton->getValue() > 0) ? (double)Engine.getHostBpm() : (double)internalBPMSlider->getValue();
	BPS = BPM / 60.0;

	timeInterval = jmax<double>(minTimerTime, (double)timeSigValArray[speedComboBox->getValue()] * 1.0 / BPS);
}

void Arpeggiator::addUserHeldKey(int notenumber)
{
	if (userHeldKeysArray.contains(notenumber))
		return;

	userHeldKeysArray.add(notenumber);
	userHeldKeysArraySorted.add(notenumber);
	userHeldKeysArraySorted.sort();
}

void Arpeggiator::remUserHeldKey(int notenumber)
{
	userHeldKeysArray.removeFirstMatchingValue(notenumber);
	userHeldKeysArraySorted.removeFirstMatchingValue(notenumber);
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
