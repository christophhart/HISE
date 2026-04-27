
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

float WaveSynthVoice::sinTable[2048];

Random WaveSynthVoice::noiseGenerator = Random();

hise::ProcessorMetadata WaveSynth::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ModulatorSynth::createBaseMetadata()
		.withId(getClassType())
		.withPrettyName("Waveform Generator")
		.withDescription("A waveform generator based on BLIP synthesis of common synthesiser waveforms.")
		.withType<hise::ModulatorSynth>()
		.withParameter(Par(OctaveTranspose1)
			.withId("OctaveTranspose1")
			.withDescription("The octave transpose factor for the first Oscillator")
			.withSliderMode(HiSlider::Discrete, Range(-5.0, 5.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(WaveForm1)
			.withId("WaveForm1")
			.withDescription("The waveform type for oscillator 1")
			.withValueList({ "Sine", "Triangle", "Saw", "Square", "Noise", "Triangle 2", "Square 2", "Trapezoid 1", "Trapezoid 2" })
			.withDefault((float)WaveformComponent::WaveformType::Saw))
		.withParameter(Par(Detune1)
			.withId("Detune1")
			.withDescription("The pitch detune of the first oscillator in cent")
			.withSliderMode(HiSlider::Linear, Range(-100.0, 100.0))
			.withDefault(0.0f))
		.withParameter(Par(Pan1)
			.withId("Pan1")
			.withDescription("The stereo panning of the first oscillator")
			.withSliderMode(HiSlider::Pan, { -1.0, 1.0 })
			.withDefault(0.0f))
		.withParameter(Par(OctaveTranspose2)
			.withId("OctaveTranspose2")
			.withDescription("The octave transpose factor for the second Oscillator")
			.withSliderMode(HiSlider::Discrete, Range(-5.0, 5.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(WaveForm2)
			.withId("WaveForm2")
			.withDescription("The waveform type for oscillator 2")
			.withValueList({ "Sine", "Triangle", "Saw", "Square", "Noise", "Triangle 2", "Square 2", "Trapezoid 1", "Trapezoid 2" })
			.withDefault((float)WaveformComponent::WaveformType::Saw))
		.withParameter(Par(Detune2)
			.withId("Detune2")
			.withDescription("The pitch detune of the second oscillator in cent")
			.withSliderMode(HiSlider::Linear, Range(-100.0, 100.0))
			.withDefault(0.0f))
		.withParameter(Par(Pan2)
			.withId("Pan2")
			.withDescription("The stereo panning of the second oscillator")
			.withSliderMode(HiSlider::Pan, { -1.0, 1.0 })
			.withDefault(0.0f))
		.withParameter(Par(Mix)
			.withId("Mix")
			.withDescription("The balance between the two oscillators")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.5f))
		.withParameter(Par(EnableSecondOscillator)
			.withId("EnableSecondOscillator")
			.withDescription("Can be used to mute the second oscillator to save CPU cycles")
			.asToggle()
			.withDefault(1.0f))
	.withParameter(Par(PulseWidth1)
			.withId("PulseWidth1")
			.withDescription("The pulse width of oscillator 1 for waveforms that support this (eg. square)")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.5f))
		.withParameter(Par(PulseWidth2)
			.withId("PulseWidth2")
			.withDescription("The pulse width of oscillator 2 for waveforms that support this (eg. square)")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.5f))
		.withParameter(Par(HardSync)
			.withId("HardSync")
			.withDescription("Syncs the second oscillator to the first")
			.asToggle()
			.withDefault(0.0f))
		.withParameter(Par(SemiTones1)
			.withId("SemiTones1")
			.withDescription("The semitone transpose amount for the first Oscillator")
			.withSliderMode(HiSlider::Discrete, Range(-12.0, 12.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(SemiTones2)
			.withId("SemiTones2")
			.withDescription("The semitone transpose amount for the second Oscillator")
			.withSliderMode(HiSlider::Discrete, Range(-12.0, 12.0, 1.0))
			.withDefault(0.0f))
		.withModulation(Mod(MixModulation)
			.withId("Mix Modulation")
			.withDescription("Modulates the balance between oscillator 1 and 2")
			.withMode(scriptnode::modulation::ParameterMode::ScaleAdd)
			.withModulatedParameter(Mix))
		.withModulation(Mod(Osc2PitchChain)
			.withId("Osc2 Pitch Modulation")
			.withDescription("Modulates the pitch of the second oscillator independently")
			.withMode(scriptnode::modulation::ParameterMode::Pitch));
}

WaveSynth::WaveSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	metadataInitialised(updateParameterSlots()),
	octaveTranspose1((int)getDefaultValue(OctaveTranspose1)),
	octaveTranspose2((int)getDefaultValue(OctaveTranspose2)),
	semiTones1((int)getDefaultValue(SemiTones1)),
	semiTones2((int)getDefaultValue(SemiTones2)),
	detune1(getDefaultValue(Detune1)),
	detune2(getDefaultValue(Detune2)),
	pan1(getDefaultValue(Pan1)),
	pan2(getDefaultValue(Pan2)),
	mix(getDefaultValue(Mix)),
	pulseWidth1(getDefaultValue(PulseWidth1)),
	pulseWidth2(getDefaultValue(PulseWidth2)),
	waveForm1(WaveformComponent::Saw),
	waveForm2(WaveformComponent::Saw),
    tempBuffer(2, 0)
{
	modChains += { this, "Mix Modulation", ModulatorChain::ModulationType::Normal, Modulation::Mode::CombinedMode };
	modChains += { this, "Osc2 Pitch Modulation", ModulatorChain::ModulationType::Normal, Modulation::PitchMode};

	finaliseModChains();

	modChains[ChainIndex::MixChain].setAllowModificationOfVoiceValues(true);
	modChains[ChainIndex::MixChain].setExpandToAudioRate(true);
	modChains[ChainIndex::MixChain].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[ChainIndex::MixChain].setClampTo0To1(true);

	modChains[ChainIndex::Osc2PitchIndex].setExpandToAudioRate(true);

	mixChain = modChains[ChainIndex::MixChain].getChain();
	osc2pitchChain = modChains[ChainIndex::Osc2PitchIndex].getChain();

	scaleFunction = [](float input) { return input * 2.0f - 1.0f; };

	WaveformLookupTables::init();

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
	loadAttribute(SemiTones1, "SemiTones1");
	loadAttribute(OctaveTranspose2, "OctaveTranspose2");
	loadAttribute(SemiTones2, "SemiTones2");
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
	loadAttribute(HardSync, "HardSync");
}

ValueTree WaveSynth::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	saveAttribute(OctaveTranspose1, "OctaveTranspose1");
	saveAttribute(SemiTones1, "SemiTones1");
	saveAttribute(OctaveTranspose2, "OctaveTranspose2");
	saveAttribute(SemiTones2, "SemiTones2");
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
	saveAttribute(HardSync, "HardSync");

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
	case Osc2PitchChain:    return osc2pitchChain;
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
	case Osc2PitchChain:    return osc2pitchChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}




void WaveSynth::getWaveformTableValues(int displayIndex, float const** tableValues, int& numValues, float& normalizeValue)
{
	auto type = (int)(displayIndex == 1 ? waveForm2 : waveForm1);

	switch (type)
	{
	case hise::WaveformComponent::Sine:
		*tableValues = WaveformLookupTables::sineTable;
		break;
	case hise::WaveformComponent::Triangle:
	case AdditionalWaveformTypes::Triangle2:
		*tableValues = WaveformLookupTables::triangleTable;
		break;
	case hise::WaveformComponent::Saw:
	case AdditionalWaveformTypes::Trapezoid1:
	case AdditionalWaveformTypes::Trapezoid2:
		*tableValues = WaveformLookupTables::sawTable;
		break;
	case hise::WaveformComponent::Square:
	case WaveSynth::AdditionalWaveformTypes::Square2:
		*tableValues = WaveformLookupTables::squareTable;
		break;
	case hise::WaveformComponent::Noise:
		*tableValues = WaveformLookupTables::randomTable;
		break;
	default:
		break;
	}

	numValues = SAMPLE_LOOKUP_TABLE_SIZE;
	normalizeValue = 1.0f;
}

float WaveSynth::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	switch (parameterIndex)
	{
	case OctaveTranspose1:		return (float)octaveTranspose1;
	case SemiTones1:			return (float)semiTones1;
	case WaveForm1:				return (float)waveForm1;
	case Detune1:				return detune1;
	case Pan1:					return pan1;
	case OctaveTranspose2:		return (float)octaveTranspose2;
	case SemiTones2:			return (float)semiTones2;
	case WaveForm2:				return (float)waveForm2;
	case Detune2:				return detune2;
	case Pan2:					return pan2;
	case Mix:					return mix;
	case EnableSecondOscillator: return enableSecondOscillator ? 1.0f : 0.0f;
	case PulseWidth1:			return (float)pulseWidth1;
	case PulseWidth2:			return (float)pulseWidth2;
	case HardSync:				return hardSync ? 1.0f : 0.0f;
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
	case SemiTones1:			semiTones1 = (int)newValue;
		refreshPitchValues(true);
		break;
	case OctaveTranspose2:		octaveTranspose2 = (int)newValue;
		refreshPitchValues(false);
		break;
	case SemiTones2:			semiTones2 = (int)newValue;
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
	case Mix:					
		mix = newValue;
		modChains[MixChain].getChain()->setInitialValue(newValue);
		break;
	case EnableSecondOscillator: enableSecondOscillator = newValue > 0.5f; break;
	case PulseWidth1:			pulseWidth1 = jlimit<float>(0.0f, 1.0f, newValue); refreshPulseWidth(true); break;
	case PulseWidth2:			pulseWidth2 = jlimit<float>(0.0f, 1.0f, newValue); refreshPulseWidth(false); break;
	case HardSync:				hardSync = newValue > 0.5f; break;
	default:					jassertfalse;
		break;
	}
}

void WaveSynth::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	if (newSampleRate != -1.0)
	{
		ProcessorHelpers::increaseBufferIfNeeded(tempBuffer, samplesPerBlock);
	}
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

	triggerWaveformUpdate();
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
	const double octaveValue = pow(2.0, (double)(getLeftValue ? octaveTranspose1 : octaveTranspose2));
	const double semiToneValue = pow(2.0, (double)(getLeftValue ? semiTones1 : semiTones2) / 12.0);
	const double detuneValue = pow(2.0, (getLeftValue ? detune1 : detune2) / 1200.0);

	return octaveValue * semiToneValue * detuneValue;
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

	uptimeDelta = 1.0;

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
	auto wavesynth = static_cast<WaveSynth*>(getOwnerSynth());

	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();
	const float* secondPitchValues = wavesynth->getPitch2ModValues(startSample);

	float *outL = voiceBuffer.getWritePointer(0, startSample);
	float *outR = voiceBuffer.getWritePointer(1, startSample);

#if USE_MARTIN_FINKE_POLY_BLEP_ALGORITHM

	if (voicePitchValues == nullptr && secondPitchValues == nullptr)
	{
		if (enableSecondOsc)
		{
			leftGenerator.setFreqModulationValue((float)uptimeDelta);
			rightGenerator.setFreqModulationValue((float)uptimeDelta * wavesynth->getConstantPitch2ModValue());

			while (--numSamples >= 0)
			{
				*outL++ = leftGenerator.get();
				*outR++ = rightGenerator.get();

				rightGenerator.inc();

				if (wavesynth->isHardSyncEnabled())
				{
					if (leftGenerator.inc())
						rightGenerator.sync(0.0);
				}
				else
					leftGenerator.inc();
			}
		}
		else
		{
			leftGenerator.setFreqModulationValue((float)uptimeDelta);

			while (--numSamples >= 0)
			{
				*outL = leftGenerator.getAndInc();
				*outR++ = *outL++;
			}
		}
	}
	else
	{
		if(voicePitchValues != nullptr)
			voicePitchValues += startSample;

		if (enableSecondOsc)
		{
			while (--numSamples >= 0)
			{
				auto leftDelta = (float)uptimeDelta;
                                
				if (voicePitchValues != nullptr)
					leftDelta *= *voicePitchValues;

				leftGenerator.setFreqModulationValue(leftDelta);
                                

				auto rightDelta = (float)uptimeDelta;
                                
				if (voicePitchValues != nullptr)
					rightDelta *= *voicePitchValues;
                                
				if (secondPitchValues != nullptr)
					rightDelta *= *secondPitchValues;

				rightGenerator.setFreqModulationValue(rightDelta);

				*outL++ = leftGenerator.get();
				*outR++ = rightGenerator.get();

				rightGenerator.inc();

				if (wavesynth->isHardSyncEnabled())
				{
					if (leftGenerator.inc())
						rightGenerator.sync(0.0);
				}
				else
					leftGenerator.inc();

				if(voicePitchValues != nullptr)
					voicePitchValues++;
				if (secondPitchValues != nullptr)
					secondPitchValues++;
			}
		}
		else
		{
			while (--numSamples >= 0)
			{
				auto leftDelta = (float)uptimeDelta;

				if (voicePitchValues != nullptr)
					leftDelta *= *voicePitchValues;

				leftGenerator.setFreqModulationValue(leftDelta);

                                
				voicePitchValues++;

				*outL = leftGenerator.getAndInc();
				*outR++ = *outL++;
			}
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

        

#if HISE_USE_WRONG_VOICE_RENDERING_ORDER
	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);
#endif

	applyGainModulation(startIndex, samplesToCopy, false);

	if (enableSecondOsc)
	{
		auto leftSamples = voiceBuffer.getWritePointer(0, startIndex);
		auto rightSamples = voiceBuffer.getWritePointer(1, startIndex);

                

		auto& tBuffer = wavesynth->getTempBufferForMixCalculation();

		// Copy all samples to the temporary buffer which contains the separated generators
		FloatVectorOperations::copy(tBuffer.getWritePointer(0, startIndex), leftSamples, samplesToCopy);
		FloatVectorOperations::copy(tBuffer.getWritePointer(1, startIndex), rightSamples, samplesToCopy);

		if (auto modValues = wavesynth->getMixModulationValues(startIndex))
		{
			// Multiply the left generator with the mix modulation values
			FloatVectorOperations::multiply(tBuffer.getWritePointer(1, startIndex), modValues, samplesToCopy);

			// Invert the mix modulation values
			FloatVectorOperations::multiply(modValues, -1.0f, samplesToCopy);
			FloatVectorOperations::add(modValues, 1.0f, samplesToCopy);

			// Multiply the right generator with the inverted mix values
			FloatVectorOperations::multiply(tBuffer.getWritePointer(0, startIndex), modValues, samplesToCopy);
		}
		else
		{
			float modValue = wavesynth->getConstantMixValue();

			// Multiply the left generator with the mix modulation values
			FloatVectorOperations::multiply(tBuffer.getWritePointer(1, startIndex), modValue, samplesToCopy);

			// Multiply the right generator with the inverted mix values
			FloatVectorOperations::multiply(tBuffer.getWritePointer(0, startIndex), 1.0f - modValue, samplesToCopy);
		}

		const float balance1Left = wavesynth->getBalanceValue(true, true);
		const float balance1Right = wavesynth->getBalanceValue(true, false);

		FloatVectorOperations::copyWithMultiply(leftSamples, tBuffer.getReadPointer(0, startIndex), balance1Left, samplesToCopy);
		FloatVectorOperations::copyWithMultiply(rightSamples, tBuffer.getReadPointer(0, startIndex), balance1Right, samplesToCopy);

		const float balance2Left = wavesynth->getBalanceValue(false, true);
		const float balance2Right = wavesynth->getBalanceValue(false, false);

		FloatVectorOperations::addWithMultiply(leftSamples, tBuffer.getReadPointer(1, startIndex), balance2Left, samplesToCopy);
		FloatVectorOperations::addWithMultiply(rightSamples, tBuffer.getReadPointer(1, startIndex), balance2Right, samplesToCopy);
	}

#if !HISE_USE_WRONG_VOICE_RENDERING_ORDER
	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);
#endif
}


float WaveSynth::getBalanceValue(bool usePan1, bool isLeft) const noexcept
{
	return BalanceCalculator::getGainFactorForBalance(usePan1 ? pan1 : pan2, isLeft);
}



void WaveSynthVoice::setOctaveTransposeFactor(double newFactor, bool leftFactor)
{
	ScopedLock sl(getOwnerSynth()->getMainController()->getLock());

	if (leftFactor) octaveTransposeFactor1 = newFactor;
	else octaveTransposeFactor2 = newFactor;
}

void WaveSynthVoice::setWaveForm(WaveformComponent::WaveformType type, bool left)
{
	switch ((int)type)
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

} // namespace hise
