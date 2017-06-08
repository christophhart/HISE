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

#ifndef AUDIOLOOPER_H_INCLUDED
#define AUDIOLOOPER_H_INCLUDED


class AudioLooper;

class AudioLooperSound : public ModulatorSynthSound
{
public:
	AudioLooperSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class AudioLooperVoice : public ModulatorSynthVoice
{
public:

	AudioLooperVoice(ModulatorSynth *ownerSynth);;

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;

	void calculateBlock(int startSample, int numSamples) override;;

private:

	friend class AudioLooper;

	float syncFactor;


};

class AudioLooper : public ModulatorSynth,
					public AudioSampleProcessor,
					public TempoListener
{
public:

	SET_PROCESSOR_NAME("AudioLooper", "Audio Loop Player");

	enum SpecialParameters
	{
		SyncMode = ModulatorSynth::numModulatorSynthParameters,
		LoopEnabled,
		PitchTracking,
		RootNote,
		numLooperParameters
	};

	AudioLooper(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	void tempoChanged(double /*newTempo*/) override
	{
		setSyncMode(syncMode);
	}

	float getAttribute(int parameterIndex) const override;;

	float getDefaultValue(int parameterIndex) const override;

	void setInternalAttribute(int parameterIndex, float newValue) override;

	

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;
	void setSyncMode(int newSyncMode);

	const CriticalSection& getFileLock() const override { return lock; };

private:

	UpdateMerger inputMerger;

	bool loopEnabled;
	bool pitchTrackingEnabled;
	int rootNote;

	friend class AudioLooperVoice;

	AudioSampleProcessor::SyncToHostMode syncMode;

};







#endif  // AUDIOLOOPER_H_INCLUDED
