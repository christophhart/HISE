
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

float WaveSynthVoice::sinTable[2048];

Random WaveSynthVoice::noiseGenerator = Random();

WaveSynth::WaveSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	octaveTranspose1((int)getDefaultValue(OctaveTranspose1)),
	octaveTranspose2((int)getDefaultValue(OctaveTranspose2)),
	detune1(getDefaultValue(Detune1)),
	detune2(getDefaultValue(Detune2)),
	pan1(getDefaultValue(Pan1)),
	pan2(getDefaultValue(Pan2)),
	mix(getDefaultValue(Mix)),
	pulseWidth1(getDefaultValue(PulseWidth1)),
	pulseWidth2(getDefaultValue(PulseWidth2)),
	waveForm1(WaveformComponent::Saw),
	waveForm2(WaveformComponent::Saw),
	mixChain(new ModulatorChain(mc, "Mix Modulation", 1, Modulation::GainMode, this))
{
	tempBuffer = AudioSampleBuffer(2, 0);
	mixBuffer = AudioSampleBuffer(1, 0);

	parameterNames.add("OctaveTranspose1");
	parameterNames.add("WaveForm1");
	parameterNames.add("Detune1");
	parameterNames.add("Pan1");
	parameterNames.add("OctaveTranspose2");
	parameterNames.add("WaveForm2");
	parameterNames.add("Detune2");
	parameterNames.add("Pan2");
	parameterNames.add("Mix");
	parameterNames.add("EnableSecondOscillator");
	parameterNames.add("PulseWidth1");
	parameterNames.add("PulseWidth2");

	editorStateIdentifiers.add("MixChainShown");

	mixChain->setColour(Colour(0xff4D54B3));

	for (int i = 0; i < numVoices; i++)
		addVoice(new WaveSynthVoice(this));
	
	addSound(new WaveSound());
}

void WaveSynth::restoreFromValueTree(const ValueTree &v)
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
	loadAttribute(EnableSecondOscillator, "EnableSecondOscillator");
	loadAttribute(PulseWidth1, "PulseWidth1");
	loadAttribute(PulseWidth2, "PulseWidth2");
}

ValueTree WaveSynth::exportAsValueTree() const
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
	saveAttribute(EnableSecondOscillator, "EnableSecondOscillator");
	saveAttribute(PulseWidth1, "PulseWidth1");
	saveAttribute(PulseWidth2, "PulseWidth2");

	return v;
}

Processor * WaveSynth::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case MixModulation:		return mixChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}

const Processor * WaveSynth::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case MixModulation:		return mixChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}

float WaveSynth::getDefaultValue(int parameterIndex) const
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
	case EnableSecondOscillator: return 1.0f;
	case PulseWidth1:			return 0.5f;
	case PulseWidth2:			return 0.5f;
	default:					jassertfalse; return -1.0f;
	}
}

float WaveSynth::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	switch (parameterIndex)
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
	case EnableSecondOscillator: return enableSecondOscillator ? 1.0f : 0.0f;
	case PulseWidth1:			return (float)pulseWidth1;
	case PulseWidth2:			return (float)pulseWidth2;
	default:					jassertfalse; return -1.0f;
	}
}

void WaveSynth::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
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
	case EnableSecondOscillator: enableSecondOscillator = newValue > 0.5f; break;
	case PulseWidth1:			pulseWidth1 = jlimit<float>(0.0f, 1.0f, newValue); refreshPulseWidth(true); break;
	case PulseWidth2:			pulseWidth2 = jlimit<float>(0.0f, 1.0f, newValue); refreshPulseWidth(false); break;
	default:					jassertfalse;
		break;
	}
}

void WaveSynth::postVoiceRendering(int startSample, int numSamples)
{
	if (enableSecondOscillator)
	{
		const float *leftSamples = internalBuffer.getReadPointer(0, startSample);
		const float *rightSamples = internalBuffer.getReadPointer(1, startSample);

		// Copy all samples to the temporary buffer which contains the separated generators
		FloatVectorOperations::copy(tempBuffer.getWritePointer(0, startSample), leftSamples, numSamples);
		FloatVectorOperations::copy(tempBuffer.getWritePointer(1, startSample), rightSamples, numSamples);

		const bool useMixBuffer = mixChain->getNumChildProcessors() != 0;

		if (useMixBuffer) mixChain->renderAllModulatorsAsMonophonic(mixBuffer, startSample, numSamples);

		if (useMixBuffer)
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
			FloatVectorOperations::multiply(tempBuffer.getWritePointer(0, startSample), 1.0f - mix, numSamples);
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
	}
	else
	{
		// Just copy the first channel to the second
		FloatVectorOperations::copy(internalBuffer.getWritePointer(1, startSample), internalBuffer.getReadPointer(0, startSample), numSamples);
	}

	

	ModulatorSynth::postVoiceRendering(startSample, numSamples);
}

void WaveSynth::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	if (newSampleRate != -1.0)
	{
		ProcessorHelpers::increaseBufferIfNeeded(tempBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(mixBuffer, samplesPerBlock);

		mixChain->prepareToPlay(newSampleRate, samplesPerBlock);
	}
}

void WaveSynth::preHiseEventCallback(const HiseEvent &m)
{
	mixChain->handleHiseEvent(m);

	ModulatorSynth::preHiseEventCallback(m);
}

ProcessorEditorBody* WaveSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new WaveSynthBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


void WaveSynth::refreshWaveForm(bool left)
{
	for (int i = 0; i < getNumVoices(); i++)
	{
		static_cast<WaveSynthVoice*>(getVoice(i))->setWaveForm(left ? waveForm1 : waveForm2, left);
	}
}

void WaveSynth::refreshPulseWidth(bool left)
{
	for (int i = 0; i < getNumVoices(); i++)
	{
		static_cast<WaveSynthVoice*>(getVoice(i))->setPulseWidth(left ? pulseWidth1 : pulseWidth2, left);
	}
}

double WaveSynth::getPitchValue(bool getLeftValue)
{
	const double octaveValue = pow(2.0, (double)getLeftValue ? octaveTranspose1 : octaveTranspose2);

	const double detuneValue = pow(2.0, (getLeftValue ? detune1 : detune2) / 1200.0);

	return octaveValue * detuneValue;
}


void WaveSynth::refreshPitchValues(bool left)
{
	for (int i = 0; i < getNumVoices(); i++)
	{
		static_cast<WaveSynthVoice*>(getVoice(i))->setOctaveTransposeFactor(getPitchValue(left), left);
	}
}


WaveSynthVoice::WaveSynthVoice(ModulatorSynth *ownerSynth) :
	ModulatorSynthVoice(ownerSynth),
	octaveTransposeFactor1(1.0),
#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM
	octaveTransposeFactor2(1.0),
	leftGenerator(44100.0),
	rightGenerator(44100.0)
#else
	octaveTransposeFactor2(1.0)
#endif
	
{
	setWaveForm(WaveformComponent::Saw, true);
	setWaveForm(WaveformComponent::Saw, false);

	initSinTable(sinTable);
}



void WaveSynthVoice::startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, 0.0f, nullptr, -1);

	midiNoteNumber += getTransposeAmount();

	enableSecondOsc = getOwnerSynth()->getAttribute(WaveSynth::SpecialParameters::EnableSecondOscillator) > 0.5f;

	const double cyclesPerSecond = MidiMessage::getMidiNoteInHertz(midiNoteNumber) * getOwnerSynth()->getMainController()->getGlobalPitchFactor();

#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	leftGenerator.setFrequency(cyclesPerSecond * octaveTransposeFactor1);

	if(enableSecondOsc)
		rightGenerator.setFrequency(cyclesPerSecond * octaveTransposeFactor2);

	leftGenerator.setStartOffset((double)getCurrentHiseEvent().getStartOffset());
	
	if(enableSecondOsc)
		rightGenerator.setStartOffset((double)getCurrentHiseEvent().getStartOffset());

#else

	cyclesPerSample = cyclesPerSecond / getSampleRate();

	voiceUptime = (double)getCurrentHiseEvent().getStartOffset() * cyclesPerSample;
	voiceUptime2 = voiceUptime;

	uptimeDelta = cyclesPerSample;
	uptimeDelta2 = cyclesPerSample;

#endif
}

void WaveSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getVoicePitchValues();

	const float *modValues = getVoiceGainValues(startSample, numSamples);

	float *outL = voiceBuffer.getWritePointer(0, startSample);
	float *outR = voiceBuffer.getWritePointer(1, startSample);

#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	if (voicePitchValues == nullptr)
	{
		while (--numSamples >= 0)
		{
			*outL++ = leftGenerator.getAndInc();

			if(enableSecondOsc)
				*outR++ = rightGenerator.getAndInc();
		}
	}
	else
	{
		voicePitchValues += startSample;

		while (--numSamples >= 0)
		{
			
			leftGenerator.setFreqModulationValue(*voicePitchValues);
			*outL++ = leftGenerator.getAndInc();

			if (enableSecondOsc)
			{
				*outR++ = rightGenerator.getAndInc();
				rightGenerator.setFreqModulationValue(*voicePitchValues);
			}
			
			voicePitchValues++;
		}
	}

	

	

#else

	if (voicePitchValues != nullptr)
	{
		voicePitchValues += startSample;

		while (--numSamples >= 0)
		{
			const float currentSample = getLeftSample(voiceUptime, uptimeDelta);
			const float currentSample2 = getRightSample(voiceUptime2, uptimeDelta2);

			*outL++ = currentSample;
			*outR++ = currentSample2;

			voiceUptime += (uptimeDelta * *voicePitchValues * octaveTransposeFactor1);
			voiceUptime2 += (uptimeDelta2 * *voicePitchValues++ * octaveTransposeFactor2);
		}
	}
	else
	{
		while (--numSamples >= 0)
		{
			const float currentSample = getLeftSample(voiceUptime, uptimeDelta);
			const float currentSample2 = getRightSample(voiceUptime2, uptimeDelta2);

			*outL++ = currentSample;
			*outR++ = currentSample2;

			voiceUptime += (uptimeDelta * octaveTransposeFactor1);
			voiceUptime2 += (uptimeDelta2 * octaveTransposeFactor2);
		}
	}

#endif

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
}

void WaveSynthVoice::setOctaveTransposeFactor(double newFactor, bool leftFactor)
{
	ScopedLock sl(getOwnerSynth()->getSynthLock());

	if (leftFactor) octaveTransposeFactor1 = newFactor;
	else octaveTransposeFactor2 = newFactor;
}

void WaveSynthVoice::setWaveForm(WaveformComponent::WaveformType type, bool left)
{
#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	switch (type)
	{
	case hise::WaveformComponent::Sine: 
		left ? leftGenerator.setWaveform(mf::PolyBLEP::SINE) : rightGenerator.setWaveform(mf::PolyBLEP::SINE); break;
	case hise::WaveformComponent::Triangle:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::TRIANGLE) : rightGenerator.setWaveform(mf::PolyBLEP::TRIANGLE); break;
	case hise::WaveformComponent::Saw:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::SAWTOOTH) : rightGenerator.setWaveform(mf::PolyBLEP::SAWTOOTH); break;
	case hise::WaveformComponent::Square:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::RECTANGLE) : rightGenerator.setWaveform(mf::PolyBLEP::RECTANGLE); break;
	case hise::WaveformComponent::Noise:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::NOISE) : rightGenerator.setWaveform(mf::PolyBLEP::NOISE); break;
	case WaveSynth::AdditionalWaveformTypes::Triangle2:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::TRIANGULAR_PULSE) : rightGenerator.setWaveform(mf::PolyBLEP::TRIANGULAR_PULSE); break;
	case WaveSynth::AdditionalWaveformTypes::Square2:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::MODIFIED_SQUARE) : rightGenerator.setWaveform(mf::PolyBLEP::MODIFIED_SQUARE); break;
	case WaveSynth::AdditionalWaveformTypes::Trapezoid1:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::TRAPEZOID_FIXED) : rightGenerator.setWaveform(mf::PolyBLEP::TRAPEZOID_FIXED); break;
	case WaveSynth::AdditionalWaveformTypes::Trapezoid2:
		left ? leftGenerator.setWaveform(mf::PolyBLEP::TRAPEZOID_VARIABLE) : rightGenerator.setWaveform(mf::PolyBLEP::TRAPEZOID_VARIABLE); break;
	default:
		break;
	}

#else
	switch (type)
	{
	case WaveformComponent::Saw:	left ? (getLeftSample = &getSaw) :
		(getRightSample = &getSaw); break;
	case WaveformComponent::Sine:	left ? (getLeftSample = &getSine) :
		(getRightSample = &getSine); break;
	case WaveformComponent::Triangle:	left ? (getLeftSample = &getTriangle) :
		(getRightSample = &getTriangle); break;
	case WaveformComponent::Noise:	left ? (getLeftSample = &getNoise) :
		(getRightSample = &getNoise); break;
	default:						left ? (getLeftSample = &getPulse) :
		(getRightSample = &getPulse); break;
	}
#endif
}

void WaveSynthVoice::setPulseWidth(double pulseWidth, bool left)
{
#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	if (left)
		leftGenerator.setPulseWidth(pulseWidth);
	else
		rightGenerator.setPulseWidth(pulseWidth);

#endif
}

void WaveSynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	leftGenerator.setSampleRate(sampleRate);
	rightGenerator.setSampleRate(sampleRate);

#endif

	ModulatorSynthVoice::prepareToPlay(sampleRate, samplesPerBlock);
}

