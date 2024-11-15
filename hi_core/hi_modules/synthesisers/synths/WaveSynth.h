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
#ifndef WAVESYNTH_H_INCLUDED
#define WAVESYNTH_H_INCLUDED

namespace hise { using namespace juce;

class WaveSynth;

#define NAIVE 0

#define USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM 1

class WaveSound : public ModulatorSynthSound
{
public:
    WaveSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override   { return true; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }
};

class WaveSynthVoice: public ModulatorSynthVoice
{
public:

	WaveSynthVoice(ModulatorSynth *ownerSynth);;


	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override;

	void calculateBlock(int startSample, int numSamples) override;;

	void setOctaveTransposeFactor(double newFactor, bool leftFactor);

	void setWaveForm(WaveformComponent::WaveformType type, bool left);

	void setPulseWidth(double pulseWidth, bool left);

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

private:

	float(*getLeftSample)(double, double);
	float(*getRightSample)(double, double);

#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	mf::PolyBLEP leftGenerator;
	mf::PolyBLEP rightGenerator;

#else

	static float getSaw(double voiceUptime, double uptimeDelta)
	{
		const double phase = fmod(voiceUptime, 1.0);
		return getBoxFilteredSaw(phase, uptimeDelta);
	};

	static float getTriangle(double voiceUptime, double /*uptimeDelta*/)
	{
		const double phase = fmod(voiceUptime, 1.0) * 4.0;

		return (float)fabs(fmod(phase, 4.0) - 2.0) - 1.0f;
	};

	static float getNoise(double /*voiceUptime*/, double /*uptimeDelta*/)
	{
		return noiseGenerator.nextFloat();
	}
        
	static float getPulse(double voiceUptime, double uptimeDelta)
	{
		const double pulseWidth = 0.5;
		const double phase = fmod(voiceUptime, 1.0);
		return getBoxFilteredSaw(phase, uptimeDelta) - getBoxFilteredSaw(fmod(phase + pulseWidth, 1.0), uptimeDelta);
	}

	static float getSine(double voiceUptime, double /*uptimeDelta*/)
	{
		const double phase = fmod(voiceUptime, 1.0) * 1024.0;

		int index = (int)phase;

		float v1 = sinTable[index & 2047];
		float v2 = sinTable[(index +1) & 2047];

		const float alpha = float(phase) - (float)index;
		const float invAlpha = 1.0f - alpha;

		const float currentSample = invAlpha * v1 + alpha * v2;

		return currentSample;
	}

	static float getBoxFilteredSaw(double phase, double kernelSize)
	{
		double a, b;

		// Remap phase and kernelSize from [0.0, 1.0] to [-1.0, 1.0]
		kernelSize *= 2.0;
		phase = phase * 2.0 - 1.0;

		if (phase + kernelSize > 1.0)
		{
			// Kernel wraps around edge of [-1.0, 1.0]
			a = phase;
			b = phase + kernelSize - 2.0;
		}
		else
		{
			// Kernel fits nicely in [-1.0, 1.0]
			a = phase;
			b = phase + kernelSize;
		}

		// Integrate and divide with kernelSize
		return (float)((b * b - a * a) / (2.0 * kernelSize));
	};


#endif

	double octaveTransposeFactor1, octaveTransposeFactor2;

	double uptimeDelta2;

	double voiceUptime2;

	double cyclesPerSample = 1.0;

	Random voiceStartRandomizer;

	bool enableSecondOsc = true;

	WaveformComponent::WaveformType type1, type2;

	static float sinTable[2048];

	static Random noiseGenerator;

	static void initSinTable(float *sin)
	{
		for(int i = 0; i < 2048; i++)
		{
			const float deltaX = (float)i * (2.0f * float_Pi) / 1024.0f;
			sin[i] = sinf(deltaX);

		}
	};
};

/** A waveform generator based on BLIP synthesis of common synthesiser waveforms.
	@ingroup synthTypes.
*/
class WaveSynth: public ModulatorSynth,
				 public WaveformComponent::Broadcaster
{
public:

	SET_PROCESSOR_NAME("WaveSynth", "Waveform Generator", "A waveform generator based on BLIP synthesis of common synthesiser waveforms.");

	enum EditorStates
	{
		MixChainShown = ModulatorSynth::numEditorStates
	};

	enum ChainIndex
	{
		GainChain=0,
		PitchChain,
		MixChain,
		Osc2PitchIndex
	};

	enum AdditionalWaveformTypes
	{
		Triangle2 = WaveformComponent::WaveformType::numWaveformTypes,
		Square2,
		Trapezoid1,
		Trapezoid2,
		numAdditionalWaveformTypes
	};

	enum SpecialParameters
	{
		OctaveTranspose1 = ModulatorSynth::numModulatorSynthParameters, ///< -5 ... **0** ... 5 | The octave transpose factor for the first Oscillator.
		WaveForm1, ///< Sine, Triangle, **Saw**, Square, Noise | the waveform type
		Detune1, ///< -100ct ... **0.0ct** ... 100ct | The pitch detune of the first oscillator in cent (100 cent = 1 semitone).
		Pan1, ///< -100 ... **0** ... 100 | the stereo panning of the first oscillator
		OctaveTranspose2, ///< -5 ... **0** ... 5 | the octave transpose factor for the first oscillator
		WaveForm2, ///< Sine, Triangle, **Saw**, Square, Noise | the waveform type
		Detune2, ///< -100ct ... **0ct** ... 100ct | the pitch detune of the first oscillator in cent (100 cent = 1 semitone)
		Pan2, ///< -100 ... **0** ... 100 | the stereo panning of the first oscillator
		Mix, ///< 0 ... **50%** ... 100% | the balance between the two oscillators (0% is only the left oscillator, while 100% is the right oscillator). This can be modulated using the Mix Modulation chain (if there are some Modulators, this control will be disabled.
		EnableSecondOscillator, ///< **On** ... Off | Can be used to mute the second oscillator to save CPU cycles
		PulseWidth1, ///< 0 ... **1** | Determines the first pulse width for waveforms that support this (eg. square)
		PulseWidth2, ///< 0 ... **1** | Determines the second pulse width for waveforms that support this (eg. square)
		HardSync, ///< **Off** ... On | Syncs the second oscillator to the first
		SemiTones1, ///< -12 ... **0** ... 12 | The semitone transpose amount for the first Oscillator.
		SemiTones2, ///< -12 ... **0** ... 12 | The semitone transpose amount for the second Oscillator.
		numWaveSynthParameters
	};

	enum InternalChains
	{
		MixModulation = ModulatorSynth::numInternalChains,
		Osc2PitchChain,
		numInternalChains
	};

	WaveSynth(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	int getNumChildProcessors() const override { return numInternalChains;	};

	int getNumInternalChains() const override {return numInternalChains; };

	Processor *getChildProcessor(int processorIndex) override;

	const Processor *getChildProcessor(int processorIndex) const override;


	float getDefaultValue(int parameterIndex) const override;;

	void getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue) override;

	int getNumWaveformDisplays() const override { return 2; }

	float getAttribute(int parameterIndex) const override;;

	void setInternalAttribute(int parameterIndex, float newValue) override;;


	void prepareToPlay(double newSampleRate, int samplesPerBlock) override;

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	const float* getPitch2ModValues(int startSample)
	{
		return modChains[ChainIndex::Osc2PitchIndex].getReadPointerForVoiceValues(startSample);
	}

	float getConstantPitch2ModValue() const
	{
		auto& mb = modChains[ChainIndex::Osc2PitchIndex];
		return mb.getConstantModulationValue();
	}

	bool isHardSyncEnabled() const { return hardSync; }

	float* getMixModulationValues(int startSample)
	{
		return modChains[ChainIndex::MixChain].getWritePointerForVoiceValues(startSample);
	}

	float getConstantMixValue() const noexcept
	{
		auto& mb = modChains[ChainIndex::MixChain];

		if (mb.getChain()->shouldBeProcessedAtAll())
		{
			return mb.getConstantModulationValue();
		}

		return mix;
	}

	AudioSampleBuffer& getTempBufferForMixCalculation()
	{
		return tempBuffer;
	}

	float getBalanceValue(bool usePan1, bool isLeft) const noexcept;

private:

	bool enableSecondOscillator = true;

	void refreshPitchValues(bool left);

	void refreshWaveForm(bool left);

	void refreshPulseWidth(bool left);

	double getPitchValue(bool getLeftValue);

	ModulatorChain* mixChain;
	ModulatorChain* osc2pitchChain;

	AudioSampleBuffer tempBuffer;

	int octaveTranspose1, octaveTranspose2;
	int semiTones1, semiTones2;

	float mix;

	float pan1, pan2;

	float detune1, detune2;

	double pulseWidth1, pulseWidth2;

	bool hardSync = false;

	WaveformComponent::WaveformType waveForm1, waveForm2;

};

} // namespace hise

#endif  // WAVESYNTH_H_INCLUDED
