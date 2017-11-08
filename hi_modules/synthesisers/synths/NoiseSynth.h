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
    }

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *modValues = getVoiceGainValues(startSample, numSamples);

		while (--numSamples >= 0)
        {
			const float currentSample = getNextValue();

			// Stereo mode assumed
			voiceBuffer.setSample (0, startSample, currentSample);
			voiceBuffer.setSample (1, startSample, currentSample);

			voiceUptime++;

            ++startSample;    
        }

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	};

private:

	inline float getNextValue() const
	{
		return 2.0f * ((float)(rand()) / (float)(RAND_MAX)) - 1.0f;
	};

};

class NoiseSynth: public ModulatorSynth
{
public:

	SET_PROCESSOR_NAME("Noise", "Noise Generator")

	NoiseSynth(MainController *mc, const String &id, int numVoices):
		ModulatorSynth(mc, id, numVoices)
	{
		for(int i = 0; i < numVoices; i++) addVoice(new NoiseVoice(this));
		addSound (new NoiseSound());	
	};

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;
};

} // namespace hise
