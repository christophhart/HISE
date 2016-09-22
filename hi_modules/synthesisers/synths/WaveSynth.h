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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
#ifndef WAVESYNTH_H_INCLUDED
#define WAVESYNTH_H_INCLUDED

class WaveSynth;

#define NAIVE 0

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

	WaveSynthVoice(ModulatorSynth *ownerSynth):
		ModulatorSynthVoice(ownerSynth),
		octaveTransposeFactor1(1.0),
		octaveTransposeFactor2(1.0)
	{
		setWaveForm(WaveformComponent::Saw, true);
		setWaveForm(WaveformComponent::Saw, false);

		initSinTable(sinTable);
		
	};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote (int midiNoteNumber, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override
	{
        ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);
        
		midiNoteNumber += getTransposeAmount();

        const double cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        
		const double cyclesPerSample = cyclesPerSecond / getSampleRate() * getOwnerSynth()->getMainController()->getGlobalPitchFactor() * eventPitchFactor;
		
		voiceUptime2 = 0.0;

		uptimeDelta = cyclesPerSample;
		uptimeDelta2 = cyclesPerSample;
    }

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *voicePitchValues = getVoicePitchValues();

		const float *modValues = getVoiceGainValues(startSample, numSamples);

		float *outL = voiceBuffer.getWritePointer(0, startSample);
		float *outR = voiceBuffer.getWritePointer(1, startSample);

		if (voicePitchValues != nullptr)
		{
			voicePitchValues += startSample;

			while (numSamples > 0)
			{
				for (int i = 0; i < 4; i++)
				{
					const float currentSample = getLeftSample(voiceUptime, uptimeDelta);
					const float currentSample2 = getRightSample(voiceUptime2, uptimeDelta2);

					*outL++ = currentSample;
					*outR++ = currentSample2;

					voiceUptime += (uptimeDelta * *voicePitchValues * octaveTransposeFactor1);
					voiceUptime2 += (uptimeDelta2 * *voicePitchValues++ * octaveTransposeFactor2);

					
				}

				numSamples -= 4;
			}
		}
		else
		{
			while (numSamples > 0)
			{
				for (int i = 0; i < 4; i++)
				{
					const float currentSample = getLeftSample(voiceUptime, uptimeDelta);
					const float currentSample2 = getRightSample(voiceUptime2, uptimeDelta2);

					*outL++ = currentSample;
					*outR++ = currentSample2;

					voiceUptime += (uptimeDelta * octaveTransposeFactor1);
					voiceUptime2 += (uptimeDelta2 * octaveTransposeFactor2);

					
				}

				numSamples -= 4;
			}
		}
	
			

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	};

	void setOctaveTransposeFactor(double newFactor, bool leftFactor)
	{
		ScopedLock sl(getOwnerSynth()->getSynthLock());

		if(leftFactor) octaveTransposeFactor1 = newFactor;
		else octaveTransposeFactor2 = newFactor;
	}

	void setWaveForm(WaveformComponent::WaveformType type, bool left)
	{
		switch(type)
		{
		case WaveformComponent::Saw:	left ? (getLeftSample = &getSaw) :
											   (getRightSample = &getSaw); break;
		case WaveformComponent::Sine:	left ? (getLeftSample = &getSine) :
											   (getRightSample = &getSine); break;
		case WaveformComponent::Triangle:	left ? (getLeftSample = &getTriangle) :
												   (getRightSample = &getTriangle); break;
		case WaveformComponent::Noise:	left ? (getLeftSample = &getNoise):
											   (getRightSample = &getNoise); break;
		default:						left ? (getLeftSample = &getPulse) :
											   (getRightSample = &getPulse); break;


		}
	}

private:

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

	float (*getLeftSample)(double, double);
	float (*getRightSample)(double, double);

	double octaveTransposeFactor1, octaveTransposeFactor2;

	double uptimeDelta2;

	double voiceUptime2;

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

class WaveSynth: public ModulatorSynth
{
public:

	SET_PROCESSOR_NAME("WaveSynth", "Waveform Generator")

	enum EditorStates
	{
		MixChainShown = ModulatorSynth::numEditorStates
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
		numWaveSynthParameters
	};

	enum InternalChains
	{
		MixModulation = ModulatorSynth::numInternalChains,
		numInternalChains
	};

	WaveSynth(MainController *mc, const String &id, int numVoices):
		ModulatorSynth(mc, id, numVoices),
		octaveTranspose1((int)getDefaultValue(OctaveTranspose1)),
		octaveTranspose2((int)getDefaultValue(OctaveTranspose2)),
		detune1(getDefaultValue(Detune1)),
		detune2(getDefaultValue(Detune2)),
		pan1(getDefaultValue(Pan1)),
		pan2(getDefaultValue(Pan2)),
		mix(getDefaultValue(Mix)),
		waveForm1(WaveformComponent::Saw),
		waveForm2(WaveformComponent::Saw),
		mixChain(new ModulatorChain(mc, "Mix Modulation", 1, Modulation::GainMode, this))
	{
		parameterNames.add("OctaveTranspose1");
		parameterNames.add("WaveForm1");
		parameterNames.add("Detune1");
		parameterNames.add("Pan1");
		parameterNames.add("OctaveTranspose2");
		parameterNames.add("WaveForm2");
		parameterNames.add("Detune2");
		parameterNames.add("Pan2");
		parameterNames.add("Mix");

		editorStateIdentifiers.add("MixChainShown");

		mixChain->setColour(Colour(0xff4D54B3));

		for(int i = 0; i < numVoices; i++) addVoice(new WaveSynthVoice(this));
		addSound (new WaveSound());	
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		ModulatorSynth::restoreFromValueTree(v);

		loadAttribute(OctaveTranspose1, "OctaveTranspose1");
		loadAttribute(OctaveTranspose2, "OctaveTranspose2");
		loadAttribute(Detune1, "Detune1");
		loadAttribute(Detune2, "Detune2");
		loadAttribute(WaveForm1, "WaveForm1");
		loadAttribute(WaveForm2, "WaveForm2");
		loadAttribute(Pan1, "Pan1");
		loadAttribute(Pan2, "Pan2");
		loadAttribute(Mix, "Mix");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = ModulatorSynth::exportAsValueTree();

		saveAttribute(OctaveTranspose1, "OctaveTranspose1");
		saveAttribute(OctaveTranspose2, "OctaveTranspose2");
		saveAttribute(Detune1, "Detune1");
		saveAttribute(Detune2, "Detune2");
		saveAttribute(WaveForm1, "WaveForm1");
		saveAttribute(WaveForm2, "WaveForm2");
		saveAttribute(Pan1, "Pan1");
		saveAttribute(Pan2, "Pan2");
		saveAttribute(Mix, "Mix");
		
		return v;
	}

	int getNumChildProcessors() const override { return numInternalChains;	};

	int getNumInternalChains() const override {return numInternalChains; };

	virtual Processor *getChildProcessor(int processorIndex) override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case MixModulation:		return mixChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	virtual const Processor *getChildProcessor(int processorIndex) const override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case MixModulation:		return mixChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	
	float getDefaultValue(int parameterIndex) const override
	{
		if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch (parameterIndex)
		{
		case OctaveTranspose1:		return 0.0f;
		case WaveForm1:				return (float)WaveformComponent::WaveformType::Saw;
		case Detune1:				return 0.0f;
		case Pan1:					return 0.0f;
		case OctaveTranspose2:		return 0.0f;
		case WaveForm2:				return (float)WaveformComponent::WaveformType::Saw;
		case Detune2:				return 0.0f;
		case Pan2:					return 0.0f;
		case Mix:					return 0.5f;
		default:					jassertfalse; return -1.0f;
		}
	};


	float getAttribute(int parameterIndex) const override 
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

		switch(parameterIndex)
		{
		case OctaveTranspose1:		return (float)octaveTranspose1;
		case WaveForm1:				return (float)waveForm1;
		case Detune1:				return detune1;
		case Pan1:					return pan1;
		case OctaveTranspose2:		return (float)octaveTranspose2;
		case WaveForm2:				return (float)waveForm2;
		case Detune2:				return detune2;
		case Pan2:					return pan2;
		case Mix:					return mix;
		default:					jassertfalse; return -1.0f;
		}
	};

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		if(parameterIndex < ModulatorSynth::numModulatorSynthParameters)
		{
			ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
			return;
		}

		switch(parameterIndex)
		{
		case OctaveTranspose1:		octaveTranspose1 = (int)newValue;
									refreshPitchValues(true);
									break;
		case OctaveTranspose2:		octaveTranspose2 = (int)newValue;
									refreshPitchValues(false);
									break;
		case Detune1:				detune1 = newValue;
									refreshPitchValues(true);
									break;
		case Detune2:				detune2 = newValue;
									refreshPitchValues(false);
									break;
		case WaveForm1:				waveForm1 = (WaveformComponent::WaveformType)(int)newValue;
									refreshWaveForm(true);
									break;
		case WaveForm2:				waveForm2 = (WaveformComponent::WaveformType)(int)newValue; 
									refreshWaveForm(false);
									break;
		case Pan1:					pan1 = newValue; break;
		case Pan2:					pan2 = newValue; break;
		case Mix:					mix = newValue; break;
		default:					jassertfalse;
									break;
		}
	};

	void postVoiceRendering(int startSample, int numSamples) override
	{
		const float *leftSamples = internalBuffer.getReadPointer(0, startSample);
		const float *rightSamples = internalBuffer.getReadPointer(1, startSample);

		// Copy all samples to the temporary buffer which contains the separated generators
		FloatVectorOperations::copy(tempBuffer.getWritePointer(0, startSample), leftSamples, numSamples);
		FloatVectorOperations::copy(tempBuffer.getWritePointer(1, startSample), rightSamples, numSamples);

		const bool useMixBuffer = mixChain->getNumChildProcessors() != 0;

		if(useMixBuffer) mixChain->renderAllModulatorsAsMonophonic(mixBuffer, startSample, numSamples);

		if(useMixBuffer)
		{

			// Multiply the left generator with the mix modulation values
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(0, startSample), mixBuffer.getReadPointer(0, startSample), numSamples);
		
			// Invert the mix modulation values
			FloatVectorOperations::multiply(mixBuffer.getWritePointer(0, startSample), -1.0f, numSamples);
			FloatVectorOperations::add(mixBuffer.getWritePointer(0, startSample), 1.0f, numSamples);

			// Multiply the right generator with the inverted mix values
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(1, startSample), mixBuffer.getReadPointer(0, startSample), numSamples);
		}
		else
		{
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(0, startSample), 1.0f-mix, numSamples);
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(1, startSample), mix, numSamples);
		}

		const float balance1Left = BalanceCalculator::getGainFactorForBalance(pan1, true);
		const float balance1Right = BalanceCalculator::getGainFactorForBalance(pan1, false);
		
		FloatVectorOperations::copyWithMultiply(internalBuffer.getWritePointer(0, startSample), tempBuffer.getReadPointer(0, startSample), balance1Left, numSamples);
		FloatVectorOperations::copyWithMultiply(internalBuffer.getWritePointer(1, startSample), tempBuffer.getReadPointer(0, startSample), balance1Right, numSamples);

		const float balance2Left = BalanceCalculator::getGainFactorForBalance(pan2, true);
		const float balance2Right = BalanceCalculator::getGainFactorForBalance(pan2, false);

		FloatVectorOperations::addWithMultiply(internalBuffer.getWritePointer(0, startSample), tempBuffer.getReadPointer(1, startSample), balance2Left, numSamples);
		FloatVectorOperations::addWithMultiply(internalBuffer.getWritePointer(1, startSample), tempBuffer.getReadPointer(1, startSample), balance2Right, numSamples);

		ModulatorSynth::postVoiceRendering(startSample, numSamples);
	};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);

		if(sampleRate != -1.0)
		{
			tempBuffer = AudioSampleBuffer(2, samplesPerBlock);
			mixBuffer = AudioSampleBuffer(1, samplesPerBlock);

			mixChain->prepareToPlay(sampleRate, samplesPerBlock);
		}
	}

	void preHiseEventCallback(const HiseEvent &m) override
	{
		mixChain->handleHiseEvent(m);

		ModulatorSynth::preHiseEventCallback(m);
	}

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

private:

	void refreshPitchValues(bool left)
	{
		for(int i = 0; i < getNumVoices(); i++) 
		{
			static_cast<WaveSynthVoice*>(getVoice(i))->setOctaveTransposeFactor(getPitchValue(left), left);
		}
	}

	void refreshWaveForm(bool left)
	{
		for(int i = 0; i < getNumVoices(); i++) 
		{
			static_cast<WaveSynthVoice*>(getVoice(i))->setWaveForm(left ? waveForm1 : waveForm2, left);
		}
	}

	double getPitchValue(bool getLeftValue)
	{
		const double octaveValue = pow(2.0, (double) getLeftValue ? octaveTranspose1 : octaveTranspose2);

		const double detuneValue = pow(2.0, (getLeftValue ? detune1 : detune2) / 1200.0);

		return octaveValue * detuneValue;
	}

	ScopedPointer<ModulatorChain> mixChain;

	AudioSampleBuffer tempBuffer;
	AudioSampleBuffer mixBuffer;

	int octaveTranspose1, octaveTranspose2;

	float mix;

	float pan1, pan2;

	float detune1, detune2;

	WaveformComponent::WaveformType waveForm1, waveForm2;

};


#endif  // WAVESYNTH_H_INCLUDED
