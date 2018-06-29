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

class NoiseSound : public ModulatorSynthSound
{
public:
    NoiseSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override   { return true; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }
};


class NoiseVoice: public ModulatorSynthVoice
{
public:

	NoiseVoice(ModulatorSynth *ownerSynth):
		ModulatorSynthVoice(ownerSynth)
	{
	};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote (int /*midiNoteNumber*/, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(0, 0.0f, nullptr, -1);

        voiceUptime = 0.0;
        
        uptimeDelta = 1.0;
#if HI_RUN_UNIT_TEST
		lastUptime = -1.0;
#endif
    }

	void calculateBlock(int startSample, int numSamples) override;;

private:

#if HI_RUN_UNIT_TESTS
	double lastUptime = 0.0;
#endif

	inline float getNextValue() const
	{
		return 2.0f * ((float)(rand()) / (float)(RAND_MAX)) - 1.0f;
	};

	

};

class NoiseSynth: public ModulatorSynth
{
public:

	enum TestSignal
	{
		Normal,
		DC,
		Ramp,
		DiracTrain,
		Square,
		numTestSignals
	};

	SET_PROCESSOR_NAME("Noise", "Noise Generator")

	NoiseSynth(MainController *mc, const String &id, int numVoices);

	void setTestSignal(TestSignal newSignalType)
	{
#if !HI_RUN_UNIT_TESTS
		jassertfalse;
#endif

		signalType = newSignalType;

	}

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	TestSignal getTestSignal() const noexcept
	{
		return signalType;
	}

private:

	TestSignal signalType = Normal;;
};

} // namespace hise
