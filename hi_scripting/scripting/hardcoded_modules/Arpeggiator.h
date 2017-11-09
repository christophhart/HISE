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

/** A hardcoded midi script arpeggiator.
*
*	This code is based on a script by Elan Hickler
*/
class Arpeggiator : public HardcodedScriptProcessor,
	public SliderPackProcessor
{
public:

	Arpeggiator(MainController *mc, const String &id, ModulatorSynth *ms);;

	SET_PROCESSOR_NAME("Arpeggiator", "Arpeggiator");

	int getNumSliderPacks();

	SliderPackData *getSliderPackData(int index) override;

	const SliderPackData *getSliderPackData(int index) const override;


	void onInit() override;

	
	void onNoteOn() override;;

	void onNoteOff() override;

	void onControl(ScriptingApi::Content::ScriptComponent *c, var value) override;

	void onTimer(int /*offsetInBuffer*/);

	void playNote();;

private:

	double timeInterval = 1.0;
	
	Array<double> timeSigValArray;
	Array<int> userHeldKeysArray;
	Array<int> userHeldKeysArraySorted;
	Array<int> currentlyPlayingKeys;
	Array<int> sequence;

	double internalBPM = 150.0;
	double BPM = 60.0;
	double BPS = 1.0;
	int minNoteLenSamples; // LATER: = Engine.getSampleRate() / 80;
	const double minTimerTime = 0.04;
	
	int arpDirMod = 1; // increment value for midi sequence.

	// gui objects and sequence arrays
	
	Array<int> MidiSequenceArray;
	Array<int> MidiSequenceArraySorted;
	
	// direction stuff
	bool do_use_step_semitone_offsets = true;
	
	enum Direction
	{
		enumSeqUP = 1,
		enumSeqDN = 2,
		enumSeqUPDN = 3,
		enumSeqDNUP = 4
	};

	int curSeqPatternEnum = 1;
	
	/* ON CONTROL FUNCTIONS */

	int curIndex = 0;
	int lastIndex = 0;

	/* EXECUTION FUNCTIONS: just use these for simplest implementation */
	
	// private //
	int lastEventId = 0;
	int curHeldNoteIdx = 0;
	int curMasterStep = 0;
	int currentNote = -1;
	int currentVelocity = 0;
	int currentStep = 0;
	int currentNoteLengthInSamples = 0;
	int midiChannel = 1;
	
	bool is_playing = false;

	bool do_tie_note = false;
	bool last_step_was_tied = false;
	int last_tied_note = -1;
	bool dir_needs_change = false;
	int curTiedNote = -1;

	static Array<double> createTempoDivisionValueArrayViaStringArray(const String& tempoValues);

	void changeDirection();;

	void calcTimeInterval();;

	void addUserHeldKey(int notenumber);;

	void remUserHeldKey(int notenumber);;

	void reset(bool do_all_notes_off, bool do_stop);;

	void start()
	{
		Synth.startTimer(timeInterval);
		is_playing = true;
	};

	void stop()
	{
		Synth.stopTimer();
		is_playing = false;
	};

	bool keys_are_held()
	{
		return Synth.getNumPressedKeys() != 0;
	};


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
		return getSliderValueWithoutDisplay(lengthSliderPack, currentStep) == 100.0f;
	};

	bool next_step_will_be_tied()
	{
		int nextStep = incAndWrapValueFromZeroToMax(currentStep, stepSkipSlider->getValue(), lengthSliderPack->getNumSliders());

		return getSliderValueWithoutDisplay(lengthSliderPack, nextStep) == 100.0f;

	};

	bool curr_step_is_skip()
	{
		return getSliderValueWithoutDisplay(lengthSliderPack, currentStep) == 0.0f;
	};

	bool next_step_will_be_skipped()
	{
		int nextStep = incAndWrapValueFromZeroToMax(currentStep, stepSkipSlider->getValue(), lengthSliderPack->getNumSliders());

		return getSliderValueWithoutDisplay(lengthSliderPack, nextStep) == 0.0f;
	};

	static float getSliderValueWithoutDisplay(ScriptingApi::Content::ScriptSliderPack* sp, int index)
	{
		auto array = sp->getSliderPackData()->getDataArray();

		if (index < array.size())
			return (float)array[index];
		else
		{
			jassertfalse;
			return 0.0f;
		}
			
	}

	ScriptingApi::Content::ScriptSliderPack *semiToneSliderPack;
	ScriptingApi::Content::ScriptSliderPack *velocitySliderPack;
	ScriptingApi::Content::ScriptSliderPack *lengthSliderPack;
	ScriptingApi::Content::ScriptButton *bypassButton;
	ScriptingApi::Content::ScriptSlider *numStepSlider;
	ScriptingApi::Content::ScriptButton* sortKeysButton;
	ScriptingApi::Content::ScriptComboBox* speedComboBox;
	ScriptingApi::Content::ScriptComboBox* sequenceComboBox;
	ScriptingApi::Content::ScriptSlider *stepReset;
	ScriptingApi::Content::ScriptSlider *stepSkipSlider;
	ScriptingApi::Content::ScriptButton *resetButton;
	ScriptingApi::Content::ScriptSlider *currentStepSlider;
	ScriptingApi::Content::ScriptSlider *octaveSlider;
	ScriptingApi::Content::ScriptSlider *shuffleSlider;
};




} // namespace hise
#endif  // ARPEGGIATOR_H_INCLUDED
