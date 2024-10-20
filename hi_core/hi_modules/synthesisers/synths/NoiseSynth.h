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

	NoiseVoice(ModulatorSynth *ownerSynth): ModulatorSynthVoice(ownerSynth) {};

	bool canPlaySound(SynthesiserSound *) override { return true; };

	void startNote (int /*midiNoteNumber*/, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(0, 0.0f, nullptr, -1);

        voiceUptime = 0.0;
        uptimeDelta = 1.0;
    }

	void calculateBlock(int startSample, int numSamples) override;;

private:

	inline float getNextValue() const { return 2.0f * ((float)(rand()) / (float)(RAND_MAX)) - 1.0f; };
};

/** A simple noise generator.
	@ingroup synthTypes.
*/
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

	SET_PROCESSOR_NAME("Noise", "Noise Generator", "A simple noise generator.");

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


class SilentSound : public ModulatorSynthSound
{
public:
	SilentSound() {} // Sound of silence

	bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
	bool appliesToChannel(int /*midiChannel*/) override { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override { return true; }
};

class SilentVoice : public ModulatorSynthVoice
{
public:

	SilentVoice(ModulatorSynth *ownerSynth) : ModulatorSynthVoice(ownerSynth) {};
	bool canPlaySound(SynthesiserSound *) override { return true; };
	void calculateBlock(int startSample, int numSamples) override 
	{
		// Sound of silence
		voiceBuffer.clear(startSample, numSamples);
		
		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startSample, numSamples);
	};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		auto numSourceChannels = getOwnerSynth()->getMatrix().getNumSourceChannels();

		if (voiceBuffer.getNumChannels() != numSourceChannels)
		{
			voiceBuffer.setSize(numSourceChannels, samplesPerBlock);
			voiceBuffer.clear();
		}

		ModulatorSynthVoice::prepareToPlay(sampleRate, samplesPerBlock);
	}

	void checkRelease() override
	{
		if (killThisVoice && FloatSanitizers::isSilence(killFadeLevel))
		{
			resetVoice();
			return;
		}

		if (!getOwnerSynth()->effectChain->hasTailingPolyEffects())
			resetVoice();
	}
};

class SilentSynth : public ModulatorSynth
{
public:

	SET_PROCESSOR_NAME("SilentSynth", "Silent Synth", "A sound generator that produces silence.");

	SilentSynth(MainController *mc, const String &id, int numVoices);

	void addProcessorsWhenEmpty() override
	{

	}

	bool synthNeedsEnvelope() const override { return false; }

	bool soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity)
	{
		return true;
	}

	void numSourceChannelsChanged() 
	{
		auto sr = getSampleRate();

		if (sr > 0.0)
		{
			for (auto v : voices)
				dynamic_cast<ModulatorSynthVoice*>(v)->prepareToPlay(getSampleRate(), getLargestBlockSize());
		}

		if (internalBuffer.getNumSamples() != 0)
		{
			jassert(getLargestBlockSize() > 0);
			internalBuffer.setSize(getMatrix().getNumSourceChannels(), internalBuffer.getNumSamples());
		}

		for (int i = 0; i < effectChain->getNumChildProcessors(); i++)
		{
			RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(effectChain->getChildProcessor(i));

			if (rp != nullptr)
			{
				rp->getMatrix().setNumSourceChannels(getMatrix().getNumSourceChannels());
				rp->getMatrix().setNumDestinationChannels(getMatrix().getNumSourceChannels());
			}
		}
	}

	void numDestinationChannelsChanged() {}
	void connectionChanged() {};

	void preVoiceRendering(int startSample, int numThisTime) override
	{
		effectChain->preRenderCallback(startSample, numThisTime);
	}

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;
};

} // namespace hise
