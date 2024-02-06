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

SET_DOCUMENTATION(ModulatorSynthGroup)
{
	SET_DOC_NAME(ModulatorSynthGroup);

	ADD_PARAMETER_DOC_WITH_NAME(EnableFM, "Enable FM", "Enables FM synthesis for this group.");
	ADD_PARAMETER_DOC_WITH_NAME(CarrierIndex, "Carrier Index", "the index for the FM carrier.");
	ADD_PARAMETER_DOC_WITH_NAME(ModulatorIndex, "Modulator Index", "the index for the FM Modulator");
	ADD_PARAMETER_DOC_WITH_NAME(UnisonoVoiceAmount, "Unisono Voices", "the number of unisono voices");
	ADD_PARAMETER_DOC_WITH_NAME(UnisonoDetune, "Unisono Detune", "The detune amount for the unisono voices");
	ADD_PARAMETER_DOC_WITH_NAME(UnisonoSpread, "Unisono Spread", "the spread amount for the unisono voices");
	ADD_PARAMETER_DOC_WITH_NAME(ForceMono, "Force Mono", "if enabled, the voices will be rendered as mono voice");
	ADD_PARAMETER_DOC_WITH_NAME(KillSecondVoices, "Kill second voices", "kills the second voices");

	ADD_CHAIN_DOC(DetuneModulation, "Detune Mod",
		"Modulates the unisono detune amount.");

	ADD_CHAIN_DOC(SpreadModulation, "Spread mod",
		"Modulates the unisono stereo spread amount.");
}

ModulatorSynthGroupVoice::ModulatorSynthGroupVoice(ModulatorSynth *ownerSynth) :
	ModulatorSynthVoice(ownerSynth)
{
}


bool ModulatorSynthGroupVoice::canPlaySound(SynthesiserSound *)
{
	return true;
}


void ModulatorSynthGroupVoice::addChildSynth(ModulatorSynth *childSynth)
{
	LockHelpers::SafeLock sl(ownerSynth->getMainController(), LockHelpers::Type::AudioLock);

	childSynths.add(ChildSynth(childSynth));
}


void ModulatorSynthGroupVoice::removeChildSynth(ModulatorSynth *childSynth)
{
	LOCK_PROCESSING_CHAIN(ownerSynth);

	//LockHelpers::SafeLock sl(ownerSynth->getMainController(), LockHelpers::Type::AudioLock, getOwnerSynth()->isOnAir());

	jassert(childSynth != nullptr);
	jassert(childSynths.indexOf(childSynth) != -1);

	for (int i = 0; i < NUM_MAX_UNISONO_VOICES; i++)
	{
		resetInternal(childSynth, i);
	}

	if (childSynth != nullptr)
	{
		childSynths.removeAllInstancesOf(childSynth);
	}
}

/** Calls the base class startNote() for the group itself and all child synths.  */
void ModulatorSynthGroupVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int)
{
	ModulatorSynthVoice::startNote(midiNoteNumber, velocity, nullptr, -1);

	// The uptime is not used, but it must be > 0, or the voice is not rendered.
	uptimeDelta = 1.0;

	auto group = static_cast<ModulatorSynthGroup*>(getOwnerSynth());

	useFMForVoice = group->fmIsCorrectlySetup();

	handleActiveStateForChildSynths();

	numUnisonoVoices = (int)getOwnerSynth()->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount);
	
#if JUCE_DEBUG

	Iterator iter(this);

	auto numVoicesInGroup = group->getNumVoices();

	while (auto s = iter.getNextActiveChildSynth())
	{
		jassert(numVoicesInGroup == s->getNumVoices());
	}

#endif

	unisonoStates.clear();

	auto mod = getFMModulator();

	if (mod != nullptr)
		startNoteInternal(mod, voiceIndex, getCurrentHiseEvent());

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

		if (unisonoVoiceIndex >= NUM_POLYPHONIC_VOICES) // don't start more voices than you have allocated
			break;

		Iterator iter2(this);

		while (auto childSynth = iter2.getNextActiveChildSynth())
		{
			if (childSynth == mod)
				continue;

			startNoteInternal(childSynth, unisonoVoiceIndex, getCurrentHiseEvent());
		}
	}
};


ModulatorSynthVoice* ModulatorSynthGroupVoice::startNoteInternal(ModulatorSynth* childSynth, int childVoiceIndex, const HiseEvent& e)
{
	if (childVoiceIndex >= NUM_POLYPHONIC_VOICES)
		return nullptr;


	int midiNoteNumber = e.getNoteNumber() + e.getTransposeAmount();

	//midiNoteNumber += transposeAmount;

	auto group = static_cast<ModulatorSynthGroup*>(getOwnerSynth());


	for (auto s : childSynth->soundsToBeStarted)
	{
		//if (auto childVoice = static_cast<ModulatorSynthVoice*>(childSynth->getVoice(childVoiceIndex)))

		if (auto childVoice = childSynth->getFreeVoice(s, 1, midiNoteNumber))
		{
			jassert(!unisonoStates.isBitSet(childVoice->getVoiceIndex()));
			jassert(childVoice->isInactive());

			// Do not set the unisono state for the FM modulator
			if (childSynth != getFMModulator())
				unisonoStates.setBit(childVoice->getVoiceIndex());

			childVoice->setStartUptime(childSynth->getMainController()->getUptime());
			childVoice->setCurrentHiseEvent(getCurrentHiseEvent());

			if (numUnisonoVoices != 1)
			{
				childVoice->addToStartOffset((uint16)startOffsetRandomizer.nextInt(441));
			}

			childSynth->preStartVoice(childVoice->getVoiceIndex(), getCurrentHiseEvent());
			childSynth->startVoiceWithHiseEvent(childVoice, s, getCurrentHiseEvent());

			getChildContainer(childVoiceIndex).addVoice(childVoice);
		}
		else
		{
			// this shouldn't happen, the synth group must kill enough voices for all child synths
			jassertfalse;
			group->resetAllVoices();
		}
	}

#if 0
	for (int j = 0; j < childSynth->getNumSounds(); j++)
	{
		ModulatorSynthSound *s = static_cast<ModulatorSynthSound*>(childSynth->getSound(j));

		//if (s->appliesToMessage(1, midiNoteNumber, (int)(velocity * 127)))
		if(childSynth->soundCanBePlayed(s, 1, midiNoteNumber, velocity))
		{
			soundToPlay = s;
			
			if (soundToPlay == nullptr) return nullptr;

			auto childVoice = childSynth->getFreeVoice(soundToPlay, 1, midiNoteNumber);

			if (childVoice != nullptr)
			{
				childVoice->setStartUptime(childSynth->getMainController()->getUptime());
				childVoice->setCurrentHiseEvent(getCurrentHiseEvent());

				if (numUnisonoVoices != 1)
				{
					childVoice->addToStartOffset((uint16)startOffsetRandomizer.nextInt(441));
				}

				childSynth->preStartVoice(childVoice->getVoiceIndex(), midiNoteNumber);
				childSynth->startVoiceWithHiseEvent(childVoice, soundToPlay, getCurrentHiseEvent());
				
				getChildContainer(childVoiceIndex).addVoice(childVoice);
			}
			else
			{
				// this shouldn't happen, the synth group must kill enough voices for all child synths
				jassertfalse;
				getOwnerSynth()->resetAllVoices();
			}
		}
	}
#endif

	
	return nullptr;
}

void ModulatorSynthGroupVoice::calculateBlock(int startSample, int numSamples)
{
	ScopedLock sl(ownerSynth->getMainController()->getLock());

	// Clear the buffer, since all child voices are added to this so it must be empty.
	voiceBuffer.clear();

	ModulatorSynthGroup *group = static_cast<ModulatorSynthGroup*>(getOwnerSynth());

	detuneValues.detuneModValue = group->getDetuneModValue(startSample);
	detuneValues.spreadModValue = group->getSpreadModValue(startSample);
	
	if (useFMForVoice)
	{
		calculateFMBlock(group, startSample, numSamples);
	}
	else
	{
		calculateNoFMBlock(startSample, numSamples);
	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startSample, numSamples);

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), modValues + startSample, numSamples);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startSample), modValues + startSample, numSamples);
	}
	else
	{
		float constantGain = getOwnerSynth()->getConstantGainModValue();

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), constantGain, numSamples);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startSample), constantGain, numSamples);
	}
};



void ModulatorSynthGroupVoice::calculateNoFMBlock(int startSample, int numSamples)
{
	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	bool isFirst = true;

#if LOG_SYNTH_EVENTS
	String s;

	s << "V" << voiceIndex << ":\t\t";
	s << (unisonoStates.isBitSet(0) ? "1 " : "0 ");
	s << (unisonoStates.isBitSet(1) ? "1 " : "0 ");
	s << (unisonoStates.isBitSet(2) ? "1 " : "0 ");
	s << (unisonoStates.isBitSet(3) ? "1 " : "0 ");

	LOG_SYNTH_EVENT(s);
#endif


	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

		Iterator iter(this);

		while (auto childSynth = iter.getNextActiveChildSynth())
			calculateNoFMVoiceInternal(childSynth, unisonoVoiceIndex, startSample, numSamples, voicePitchValues, isFirst);
	}

	if (!unisonoStates.anyActive())
	{
		resetVoice();
	}
		
}


void ModulatorSynthGroupVoice::calculateNoFMVoiceInternal(ModulatorSynth* childSynth, int unisonoIndex, int startSample, int numSamples, const float * voicePitchValues, bool& isFirst)
{
	if (childSynth->isSoftBypassed())
		return;

	if (unisonoIndex >= NUM_POLYPHONIC_VOICES)
		return;

	calculateDetuneMultipliers(unisonoIndex);

	auto& childContainer = getChildContainer(unisonoIndex);

	const float gain = childSynth->getGain();
	const float g_left = detuneValues.getGainFactor(false) * gain * childSynth->getBalance(false);
	const float g_right = detuneValues.getGainFactor(true) * gain * childSynth->getBalance(true);

	const bool forceMono = getOwnerSynth()->getAttribute(ModulatorSynthGroup::SpecialParameters::ForceMono) > 0.5f;

	for (int i = 0; i < childContainer.size(); i++)
	{
		ModulatorSynthVoice *childVoice = childContainer.getVoice(i); //static_cast<ModulatorSynthVoice*>(childSynth->getVoice(childVoiceIndex));

		// You have to remove them from the childvoice list
		jassert(unisonoStates.isBitSet(childVoice->getVoiceIndex()));

		if (childVoice->isInactive() || childVoice->getOwnerSynth() != childSynth)
		{
			LOG_SYNTH_EVENT("V" + String(voiceIndex) + ": Skipping inactive voice " + String(childVoice->getVoiceIndex()));
			continue;
		}

		LOG_SYNTH_EVENT("V" + String(voiceIndex) + ": Rendering child voice " + String(childVoice->getVoiceIndex()));

		calculatePitchValuesForChildVoice(childSynth, childVoice, startSample, numSamples, voicePitchValues);

		childVoice->calculateBlock(startSample, numSamples);
		
		if (childVoice->shouldBeKilled())
		{
			childVoice->applyKillFadeout(startSample, numSamples);
		}

		if (forceMono)
		{
			float* scratch = (float*)alloca(sizeof(float)*numSamples);

			FloatVectorOperations::copy(scratch, childVoice->getVoiceValues(0, startSample), numSamples);
			FloatVectorOperations::add(scratch, childVoice->getVoiceValues(1, startSample), numSamples);
			FloatVectorOperations::multiply(scratch, 0.5f, numSamples);

			if (isFirst)
			{
				voiceBuffer.copyFrom(0, startSample, scratch, numSamples, g_left);
				voiceBuffer.copyFrom(1, startSample, scratch, numSamples, g_right);
				isFirst = false;
			}
			else
			{
				voiceBuffer.addFrom(0, startSample, scratch, numSamples, g_left);
				voiceBuffer.addFrom(1, startSample, scratch, numSamples, g_right);
			}
			
		}
		else
		{
			if (isFirst)
			{
				voiceBuffer.copyFrom(0, startSample, childVoice->getVoiceValues(0, startSample), numSamples, g_left);
				voiceBuffer.copyFrom(1, startSample, childVoice->getVoiceValues(1, startSample), numSamples, g_right);
				isFirst = false;
			}
			else
			{
				voiceBuffer.addFrom(0, startSample, childVoice->getVoiceValues(0, startSample), numSamples, g_left);
				voiceBuffer.addFrom(1, startSample, childVoice->getVoiceValues(1, startSample), numSamples, g_right);
			}
		}

		if (childVoice->getCurrentlyPlayingSound() == nullptr)
		{
			LOG_SYNTH_EVENT("V" + String(voiceIndex) + ": Suspending unisono voice with index " + String(childVoice->getVoiceIndex()));
			unisonoStates.clearBit(childVoice->getVoiceIndex());
			childContainer.removeVoice(childVoice);
		}			
	}

	childSynth->clearPendingRemoveVoices();

	childSynth->setPeakValues(gain, gain);
}


void ModulatorSynthGroupVoice::calculatePitchValuesForChildVoice(ModulatorSynth* childSynth, ModulatorSynthVoice * childVoice, int startSample, int numSamples, const float * voicePitchValues, bool applyDetune/*=true*/)
{
	if (isInactive())
		return;

	childSynth->calculateModulationValuesForVoice(childVoice, startSample, numSamples);

	auto childPitchValues = childSynth->getPitchValuesForVoice();

	if (childPitchValues != nullptr && voicePitchValues != nullptr)
	{
		FloatVectorOperations::multiply(childPitchValues + startSample, voicePitchValues + startSample, numSamples);
	}
	else if (voicePitchValues != nullptr)
	{
		childSynth->overwritePitchValues(voicePitchValues, startSample, numSamples);
		jassert(childSynth->getPitchValuesForVoice() != nullptr);
	}

	// Hack: isPitchFadeActive() doesn't work in groups because it's calculated before the rendering
	// However, we can use the uptimeDelta value for this
	childVoice->applyConstantPitchFactor(uptimeDelta);

	if(applyDetune)
		childVoice->applyConstantPitchFactor(detuneValues.multiplier);
}

void ModulatorSynthGroupVoice::calculateDetuneMultipliers(int childVoiceIndex)
{
	if (numUnisonoVoices != 1)
	{
		// 0 ... voiceAmount -> -detune ... detune

		const float detune = ownerSynth->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoDetune);
		const float balance = ownerSynth->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoSpread);

		const int unisonoIndex = childVoiceIndex % numUnisonoVoices;

		detuneValues.gainFactor = 1.0f / sqrt((float)numUnisonoVoices);

		const float normalizedVoiceIndex = (float)unisonoIndex / (float)(numUnisonoVoices - 1);
		const float normalizedDetuneAmount = normalizedVoiceIndex * 2.0f - 1.0f;
		const float detuneOctaveAmount = detune * normalizedDetuneAmount * detuneValues.detuneModValue;
		detuneValues.multiplier = Modulation::PitchConverters::octaveRangeToPitchFactor(detuneOctaveAmount);

		const float detuneBalanceAmount = normalizedDetuneAmount * 100.0f * balance * detuneValues.spreadModValue;

		detuneValues.balanceLeft = BalanceCalculator::getGainFactorForBalance(detuneBalanceAmount, true);
		detuneValues.balanceRight = BalanceCalculator::getGainFactorForBalance(detuneBalanceAmount, false);
	}
	else
	{
		// reset them...
		detuneValues = DetuneValues();
	}

}

void ModulatorSynthGroupVoice::calculateFMBlock(ModulatorSynthGroup * group, int startSample, int numSamples)
{
	// Calculate the modulator

	const float *voicePitchValues = getOwnerSynth()->getPitchValuesForVoice();

	ModulatorSynth *modSynth = getFMModulator();

	if (modSynth == nullptr)
		return;

	ModulatorSynthVoice *modVoice = static_cast<ModulatorSynthVoice*>(modSynth->getVoice(voiceIndex));

	if (modSynth->isBypassed() || modVoice->isInactive()) return;

	const float modGain = modSynth->getGain();

	// Do not apply the detune to the FM modulator
	calculatePitchValuesForChildVoice(modSynth, modVoice, startSample, numSamples, voicePitchValues, false);

	modVoice->calculateBlock(startSample, numSamples);

	if (modVoice->shouldBeKilled())
	{
		modVoice->applyKillFadeout(startSample, numSamples);
	}

	const float *modValues = modVoice->getVoiceValues(0, startSample); // Channel is the same;

	FloatVectorOperations::copy(fmModBuffer, modValues, numSamples);

	FloatVectorOperations::multiply(fmModBuffer, modGain, numSamples);

	const float peak = FloatVectorOperations::findMaximum(fmModBuffer, numSamples);

	FloatVectorOperations::add(fmModBuffer, 1.0f, numSamples);

	modSynth->setPeakValues(peak, peak);

	modSynth->clearPendingRemoveVoices();

	// Calculate the carrier voice

	bool isFirst = true;

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;
		calculateFMCarrierInternal(group, unisonoVoiceIndex, startSample, numSamples, voicePitchValues, isFirst);
	}

	if (!unisonoStates.anyActive())
		resetVoice();
}


void ModulatorSynthGroupVoice::calculateFMCarrierInternal(ModulatorSynthGroup * group, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues, bool& isFirst)
{
	if (childVoiceIndex >= NUM_POLYPHONIC_VOICES)
		return;

	auto indexOffset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	ModulatorSynth *carrierSynth = static_cast<ModulatorSynth*>(group->getChildProcessor(group->carrierIndex - 1 + indexOffset));
	jassert(carrierSynth != nullptr);

	ModulatorSynth *modSynth = static_cast<ModulatorSynth*>(group->getChildProcessor(group->modIndex - 1 + indexOffset));
	jassert(modSynth != nullptr);

	calculateDetuneMultipliers(childVoiceIndex);

	const float carrierGain = carrierSynth->getGain();
	const float g_left = detuneValues.getGainFactor(false) * carrierGain * carrierSynth->getBalance(false);
	const float g_right = detuneValues.getGainFactor(true) * carrierGain * carrierSynth->getBalance(true);

	auto& childContainer = getChildContainer(childVoiceIndex);

	const bool forceMono = getOwnerSynth()->getAttribute(ModulatorSynthGroup::SpecialParameters::ForceMono) > 0.5f;

	for (int i = 0; i < childContainer.size(); i++)
	{
		ModulatorSynthVoice *carrierVoice = childContainer.getVoice(i); // static_cast<ModulatorSynthVoice*>(carrierSynth->getVoice(childVoiceIndex));

		

		if (carrierVoice->getOwnerSynth() == modSynth)
			continue; // The modulator will be listed in the child voices but shouldn't be rendered here...

		// You have to clear the unisono flag when resetting the voice;
		jassert(unisonoStates.isBitSet(carrierVoice->getVoiceIndex()));

		if (carrierVoice == nullptr)
			return;

		if (carrierSynth->isSoftBypassed())
			return;
		
		if (carrierVoice->isInactive())
			continue;

		calculatePitchValuesForChildVoice(carrierSynth, carrierVoice, startSample, numSamples, voicePitchValues);

		//carrierSynth->calculateModulationValuesForVoice(carrierVoice, startSample, numSamples);
		//carrierVoice->applyConstantPitchFactor(getOwnerSynth()->getConstantPitchModValue());

		float *carrierPitchValues = carrierSynth->getPitchValuesForVoice();

		if (carrierPitchValues == nullptr)
		{
			carrierPitchValues = (float*)alloca(sizeof(float)*(numSamples + startSample));
			FloatVectorOperations::fill(carrierPitchValues + startSample, 1.0f, numSamples);
		}
		
		if(voicePitchValues != nullptr)
			FloatVectorOperations::multiply(carrierPitchValues + startSample, voicePitchValues + startSample, 1.0f, numSamples);

		

		// This is the magic FM command
		FloatVectorOperations::multiply(carrierPitchValues + startSample, fmModBuffer, numSamples);

#if JUCE_WINDOWS
		FloatVectorOperations::clip(carrierPitchValues + startSample, carrierPitchValues + startSample, 0.00000001f, 1000.0f, numSamples);
#endif

		carrierSynth->overwritePitchValues(carrierPitchValues, startSample, numSamples);

		carrierVoice->calculateBlock(startSample, numSamples);

		if (carrierVoice->shouldBeKilled())
		{
			carrierVoice->applyKillFadeout(startSample, numSamples);
		}

		if (forceMono)
		{
			float* scratch = (float*)alloca(sizeof(float)*numSamples);

			FloatVectorOperations::copy(scratch, carrierVoice->getVoiceValues(0, startSample), numSamples);
			FloatVectorOperations::add(scratch, carrierVoice->getVoiceValues(1, startSample), numSamples);
			FloatVectorOperations::multiply(scratch, 0.5f, numSamples);

			if (isFirst)
			{
				voiceBuffer.copyFrom(0, startSample, scratch, numSamples, g_left);
				voiceBuffer.copyFrom(1, startSample, scratch, numSamples, g_right);

				isFirst = false;
			}
			else
			{
				voiceBuffer.addFrom(0, startSample, scratch, numSamples, g_left);
				voiceBuffer.addFrom(1, startSample, scratch, numSamples, g_right);
			}
			
		}
		else
		{
			if (isFirst)
			{
				voiceBuffer.copyFrom(0, startSample, carrierVoice->getVoiceValues(0, startSample), numSamples, g_left);
				voiceBuffer.copyFrom(1, startSample, carrierVoice->getVoiceValues(1, startSample), numSamples, g_right);

				isFirst = false;
			}
			else
			{
				voiceBuffer.addFrom(0, startSample, carrierVoice->getVoiceValues(0, startSample), numSamples, g_left);
				voiceBuffer.addFrom(1, startSample, carrierVoice->getVoiceValues(1, startSample), numSamples, g_right);
			}

			
		}

#if ENABLE_ALL_PEAK_METERS
		const float peak2 = FloatVectorOperations::findMaximum(carrierVoice->getVoiceValues(0, startSample), numSamples);
		carrierSynth->setPeakValues(peak2, peak2);
#endif

		if (carrierVoice->getCurrentlyPlayingSound() == nullptr)
		{
			LOG_SYNTH_EVENT("Suspending FM voice " + String(carrierVoice->getVoiceIndex()));
			unisonoStates.clearBit(carrierVoice->getVoiceIndex());
			childContainer.removeVoice(carrierVoice);
		}
	}

	carrierSynth->clearPendingRemoveVoices();
}


int ModulatorSynthGroupVoice::getChildVoiceAmount() const
{
	int s = 0;

	for (int i = 0; i < NUM_MAX_UNISONO_VOICES; i++)
	{
		s += startedChildVoices[i].size();
	}

	return s;
}

ModulatorSynth* ModulatorSynthGroupVoice::getFMModulator()
{
	return static_cast<ModulatorSynthGroup*>(getOwnerSynth())->getFMModulator();
}

ModulatorSynth* ModulatorSynthGroupVoice::getFMCarrier()
{
	return static_cast<ModulatorSynthGroup*>(getOwnerSynth())->getFMCarrier();
}

void ModulatorSynthGroupVoice::handleActiveStateForChildSynths()
{
	if (useFMForVoice)
	{
		LOG_SYNTH_EVENT("Calculating active states for FM");

		auto mod = getFMModulator();
		auto carrier = getFMCarrier();
		
		for (auto& s : childSynths)
		{
			s.isActiveForThisVoice = (s.synth == mod || s.synth == carrier);

			LOG_SYNTH_EVENT(s.synth->getId() + " is " + (s.isActiveForThisVoice ? "active" : "inactive"));
		}
			
	}
	else
	{
		LOG_SYNTH_EVENT("Calculating active states without FM");

		if (auto carrier = getFMCarrier())
		{
			for (auto& s : childSynths)
			{
				s.isActiveForThisVoice = s.synth == carrier;

				LOG_SYNTH_EVENT(s.synth->getId() + " is " + (s.isActiveForThisVoice ? "active" : "inactive"));
			}
		}
		else
		{
			for (auto& s : childSynths)
			{
				s.isActiveForThisVoice = !s.synth->isBypassed();

				LOG_SYNTH_EVENT(s.synth->getId() + " is " + (s.isActiveForThisVoice ? "active" : "inactive"));
			}
		}

		

		
	}
}

void ModulatorSynthGroupVoice::stopNote(float, bool)
{
	if (auto mod = getFMModulator())
		stopNoteInternal(mod, voiceIndex);

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

		Iterator iter(this);

		while (auto childSynth = iter.getNextActiveChildSynth())
		{
			stopNoteInternal(childSynth, unisonoVoiceIndex);
		}	
	}

	ModulatorSynthVoice::stopNote(0.0, false);
};


void ModulatorSynthGroupVoice::stopNoteInternal(ModulatorSynth * childSynth, int childVoiceIndex)
{
	if (childVoiceIndex >= NUM_POLYPHONIC_VOICES)
		return;

	ModulatorChain *g = static_cast<ModulatorChain*>(childSynth->getChildProcessor(ModulatorSynth::GainModulation));
	ModulatorChain *p = static_cast<ModulatorChain*>(childSynth->getChildProcessor(ModulatorSynth::PitchModulation));

	
	if(g->hasVoiceModulators())
		g->stopVoice(voiceIndex);

	if(p->hasVoiceModulators())
		p->stopVoice(voiceIndex);
}

void ModulatorSynthGroupVoice::checkRelease()
{
	ModulatorChain *ownerGainChain = static_cast<ModulatorChain*>(ownerSynth->getChildProcessor(ModulatorSynth::GainModulation));
	//ModulatorChain *ownerPitchChain = static_cast<ModulatorChain*>(ownerSynth->getChildProcessor(ModulatorSynth::PitchModulation));

	if (killThisVoice && FloatSanitizers::isSilence(killFadeLevel))
	{
		resetVoice();
	}

	if (ownerGainChain->hasActivePolyEnvelopes() && !ownerGainChain->isPlaying(voiceIndex))
	{
		resetVoice();
	}
}




void ModulatorSynthGroupVoice::resetVoice()
{
	ModulatorSynthVoice::resetVoice();

	if (auto mod = getFMModulator())
		resetInternal(mod, voiceIndex);

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex * numUnisonoVoices + i;

		Iterator iter(this);

		while (auto childSynth = iter.getNextActiveChildSynth())
			resetInternal(childSynth, unisonoVoiceIndex);
	}

	unisonoStates.clear();
}

void ModulatorSynthGroupVoice::resetInternal(ModulatorSynth * childSynth, int childVoiceIndex)
{
	if (childVoiceIndex >= NUM_POLYPHONIC_VOICES)
		return;

	
	auto& childContainer = getChildContainer(childVoiceIndex);

	for (int i = 0; i < childContainer.size(); i++)
	{
		ModulatorSynthVoice *childVoice =  childContainer.getVoice(i);

		unisonoStates.clearBit(childVoice->getVoiceIndex());
		childSynth->setPeakValues(0.0f, 0.0f);
		childVoice->resetVoice();
	}

	childContainer.clear();
	
}

ModulatorSynthGroup::ModulatorSynthGroup(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	numVoices(numVoices),
	handler(this),
	vuValue(0.0f),
	fmEnabled(getDefaultValue(ModulatorSynthGroup::SpecialParameters::EnableFM) > 0.5f),
	carrierIndex((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::CarrierIndex)),
	modIndex((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::ModulatorIndex)),
	fmCorrectlySetup(false),
	unisonoVoiceAmount((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount)),
	unisonoDetuneAmount((double)getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoDetune)),
	unisonoSpreadAmount(getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoSpread)),
	modSynthGainValues(1, 0)
{
	modChains += {this, "Detune Mod"};
	modChains += {this, "Spread Mod"};

	finaliseModChains();

	detuneChain = modChains[ModChains::Detune].getChain();
	spreadChain = modChains[ModChains::Spread].getChain();

	setFactoryType(new ModulatorSynthChainFactoryType(numVoices, this));
	getFactoryType()->setConstrainer(new SynthGroupConstrainer());

	detuneChain->setColour(Colour(0xFF880022));
	spreadChain->setColour(Colour(0xFF22AA88));

	setGain(1.0);

	parameterNames.add("EnableFM");
	parameterNames.add("CarrierIndex");
	parameterNames.add("ModulatorIndex");
	parameterNames.add("UnisonoVoiceAmount");
	parameterNames.add("UnisonoDetune");
	parameterNames.add("UnisonoSpread");
	parameterNames.add("ForceMono");
	parameterNames.add("KillSecondVoices");

	updateParameterSlots();

	allowStates.clear();

	for (int i = 0; i < numVoices; i++) addVoice(new ModulatorSynthGroupVoice(this));
	
	auto sound = new ModulatorSynthGroupSound();

	addSound(sound);

	soundsToBeStarted.insert(sound);
};


ModulatorSynthGroup::~ModulatorSynthGroup()
{
	handler.clearAsync(this);

	// This must be destroyed before the base class destructor because the MidiProcessor destructors may use some of ModulatorSynthGroup methods...
	midiProcessorChain = nullptr;

	

}

ProcessorEditorBody *ModulatorSynthGroup::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GroupBody(parentEditor);


#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

Chain::Handler* ModulatorSynthGroup::getHandler()
{ return &handler; }

const Chain::Handler* ModulatorSynthGroup::getHandler() const
{ return &handler; }

FactoryType* ModulatorSynthGroup::getFactoryType() const
{ return modulatorSynthFactory; }

void ModulatorSynthGroup::setFactoryType(FactoryType* newFactoryType)
{ modulatorSynthFactory = newFactoryType; }

int ModulatorSynthGroup::getNumChildProcessors() const
{ return numInternalChains + handler.getNumProcessors(); }

int ModulatorSynthGroup::getNumInternalChains() const
{ return numInternalChains; }

Processor* ModulatorSynthGroup::getParentProcessor()
{ return nullptr; }

const Processor* ModulatorSynthGroup::getParentProcessor() const
{ return nullptr; }

float ModulatorSynthGroup::getDetuneModValue(int startSample) const
{
	return modChains[ModChains::Detune].getOneModulationValue(startSample);
}

float ModulatorSynthGroup::getSpreadModValue(int startSample) const noexcept
{
	return modChains[ModChains::Spread].getOneModulationValue(startSample);
}

ModulatorSynthGroup::ModulatorSynthGroupHandler::ModulatorSynthGroupHandler(ModulatorSynthGroup* synthGroupToHandle):
	group(synthGroupToHandle)
{

}

String ModulatorSynthGroup::getFMState() const
{ return getFMStateString(); }

bool ModulatorSynthGroup::fmIsCorrectlySetup() const
{ return fmCorrectlySetup; }

bool ModulatorSynthGroup::SynthVoiceAmount::operator==(const SynthVoiceAmount& other) const
{
	return other.s == s;
};

void ModulatorSynthGroup::setInternalAttribute(int index, float newValue)
{
	if (index < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(index, newValue);
		return;
	}

	switch (index)
	{
	case EnableFM:			
	{
		const bool nv = (newValue > 0.5f);

		if (fmEnabled != nv)
		{
			fmEnabled = nv; 
			checkFmState();
		}

		break;
	}
	case ModulatorIndex:	 
	{
		const int nv = (int)newValue;

		if (nv != modIndex)
		{
			modIndex = nv;
			checkFmState();
		}

		break;
	}
	case CarrierIndex:		
	{
		const int nv = (int)newValue;

		if (carrierIndex != nv)
		{
			carrierIndex = nv;

			checkFmState();

			auto c = getFMCarrier();

			carrierIsSampler = dynamic_cast<ModulatorSampler*>(c) != nullptr;
		}

		break;
	}
	case UnisonoVoiceAmount: setUnisonoVoiceAmount((int)newValue); break;
	case UnisonoDetune:		 setUnisonoDetuneAmount(newValue); break;
	case UnisonoSpread:		 setUnisonoSpreadAmount(newValue); break;
	case ForceMono:			 forceMono = newValue > 0.5f; break;
	case KillSecondVoices:	 killSecondVoice = newValue > 0.5f; break;
	default:				 jassertfalse;
	}
}

float ModulatorSynthGroup::getAttribute(int index) const
{
	if (index < ModulatorSynth::numModulatorSynthParameters)
	{
		return ModulatorSynth::getAttribute(index);
	}

	switch (index)
	{
	case EnableFM:			 return fmEnabled ? 1.0f : 0.0f;
	case ModulatorIndex:	 return (float)modIndex;
	case CarrierIndex:		 return (float)carrierIndex;
	case UnisonoVoiceAmount: return (float)unisonoVoiceAmount;
	case UnisonoDetune:		 return (float)unisonoDetuneAmount;
	case UnisonoSpread:		 return unisonoSpreadAmount;
	case ForceMono:			 return forceMono ? 1.0f : 0.0f;
	case KillSecondVoices:	 return killSecondVoice ? 1.0f : 0.0f;
	default:				 jassertfalse; return -1.0f;
	}
}


float ModulatorSynthGroup::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		return ModulatorSynth::getDefaultValue(parameterIndex);
	}

	switch (parameterIndex)
	{
	case EnableFM:		 return 0.0f;
	case ModulatorIndex: return (float)-1;
	case CarrierIndex:	 return (float)-1;
	case UnisonoVoiceAmount: return 1.0f;
	case UnisonoDetune:		 return 0.0f;
	case UnisonoSpread:		 return 1.0f;
	case ForceMono:		 return 0.0f;
	case KillSecondVoices:	return 0.0f;
	default:			 jassertfalse; return -1.0f;
	}
}

ModulatorSynth* ModulatorSynthGroup::getFMModulator()
{
	if (fmIsCorrectlySetup())
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;
		return static_cast<ModulatorSynth*>(getChildProcessor(modIndex - 1 + offset));
	}
	else
		return nullptr;
}

const ModulatorSynth* ModulatorSynthGroup::getFMModulator() const
{
	if (fmIsCorrectlySetup())
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;
		return static_cast<const ModulatorSynth*>(getChildProcessor(modIndex - 1 + offset));
	}
	else
		return nullptr;
}

ModulatorSynth* ModulatorSynthGroup::getFMCarrier()
{
	if (carrierIndex <= 0)
		return nullptr;

	else
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;
		return static_cast<ModulatorSynth*>(getChildProcessor(carrierIndex - 1 + offset));
	}
}

const ModulatorSynth* ModulatorSynthGroup::getFMCarrier() const
{
	if (carrierIndex <= 0)
		return nullptr;

	else
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;
		return static_cast<const ModulatorSynth*>(getChildProcessor(carrierIndex - 1 + offset));
	}
}

Processor * ModulatorSynthGroup::getChildProcessor(int processorIndex)
{
	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else if (processorIndex == DetuneModulation)			return detuneChain;
	else if (processorIndex == SpreadModulation)			return spreadChain;
	else													return handler.getProcessor(processorIndex - numInternalChains);
}


const Processor * ModulatorSynthGroup::getChildProcessor(int processorIndex) const
{
	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else if (processorIndex == DetuneModulation)			return detuneChain;
	else if (processorIndex == SpreadModulation)			return spreadChain;
	else													return handler.getProcessor(processorIndex - numInternalChains);
}


void ModulatorSynthGroup::allowChildSynth(int childSynthIndex, bool shouldBeAllowed)
{
	allowStates.setBit(childSynthIndex, shouldBeAllowed);
}


void ModulatorSynthGroup::setAllowStateForAllChildSynths(bool shouldBeEnabled)
{
	allowStates.setRange(0, numVoices, shouldBeEnabled);
}


void ModulatorSynthGroup::setUnisonoVoiceAmount(int newVoiceAmount)
{
	unisonoVoiceAmount = jmax<int>(1, newVoiceAmount);

	detuneChain->setBypassed(unisonoVoiceAmount == 1);
	spreadChain->setBypassed(unisonoVoiceAmount == 1);

	const int unisonoVoiceLimit = NUM_POLYPHONIC_VOICES / unisonoVoiceAmount;

	setVoiceLimit(unisonoVoiceLimit);
}


void ModulatorSynthGroup::setUnisonoDetuneAmount(float newDetuneAmount)
{
	unisonoDetuneAmount = newDetuneAmount;
}


void ModulatorSynthGroup::setUnisonoSpreadAmount(float newSpreadAmount)
{
	unisonoSpreadAmount = newSpreadAmount;
}

int ModulatorSynthGroup::getNumActiveVoices() const
{
	int thisNumActiveVoices = 0;

	for (int i = 0; i < voices.size(); i++)
	{
		if (!voices[i]->isVoiceActive())
			continue;

		thisNumActiveVoices += static_cast<ModulatorSynthGroupVoice*>(voices[i])->getChildVoiceAmount();
	}

	return thisNumActiveVoices;
}

void ModulatorSynthGroup::preHiseEventCallback(HiseEvent &m)
{
	ModulatorSynth::preHiseEventCallback(m);

	ModulatorSynth *child;
	ChildSynthIterator iterator(this, ChildSynthIterator::SkipUnallowedSynths);

	while (iterator.getNextAllowedChild(child))
	{
		child->preHiseEventCallback(m);
	}
};


void ModulatorSynthGroup::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	if (newSampleRate != -1.0)
	{
		ProcessorHelpers::increaseBufferIfNeeded(modSynthGainValues, samplesPerBlock);

		ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

		ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
		ModulatorSynth *childSynth;

		while (iterator.getNextAllowedChild(childSynth))
		{
			childSynth->prepareToPlay(newSampleRate, samplesPerBlock);
		}
	}
}


void ModulatorSynthGroup::initRenderCallback()
{
	ModulatorSynth::initRenderCallback();

	ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
	ModulatorSynth *childSynth;

	while (iterator.getNextAllowedChild(childSynth))
	{
		childSynth->initRenderCallback();
	}
}


int ModulatorSynthGroup::collectSoundsToBeStarted(const HiseEvent& m)
{
	ChildSynthIterator iter(this);
	ModulatorSynth* child = nullptr;

	synthVoiceAmounts.clearQuick();

#if JUCE_DEBUG
	eventForSoundCollection = m;
#endif

	while (iter.getNextAllowedChild(child))
	{
		int numVoicesForChild = child->collectSoundsToBeStarted(m);
		synthVoiceAmounts.insertWithoutSearch({ child, numVoicesForChild });
	}

	return 1;
}

void ModulatorSynthGroup::preVoiceRendering(int startSample, int numThisTime)
{
	ModulatorSynth::preVoiceRendering(startSample, numThisTime);

	ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
	ModulatorSynth *childSynth;

#if 0
	if (fmCorrectlySetup)
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

		ModulatorSynth *modSynth = static_cast<ModulatorSynth*>(getChildProcessor(modIndex - 1 + offset));
		ModulatorChain *gainChainOfModSynth = static_cast<ModulatorChain*>(modSynth->getChildProcessor(ModulatorSynth::GainModulation));

		gainChainOfModSynth->renderNextBlock(modSynthGainValues, startSample, numThisTime);
	}
#endif

	while (iterator.getNextAllowedChild(childSynth))
	{
		childSynth->preVoiceRendering(startSample, numThisTime);
	}
}


void ModulatorSynthGroup::handleRetriggeredNote(ModulatorSynthVoice *voice)
{
	if (killSecondVoice)
	{
		int noteNumber = voice->getCurrentlyPlayingNote();
		auto uptime = voice->getVoiceUptime();

		for (auto v: activeVoices)
		{
			auto thisNumber = v->getCurrentlyPlayingNote();
			auto thisUptime = v->getVoiceUptime();

			if (noteNumber == thisNumber && thisUptime < uptime)
			{
				v->killVoice();
			}
		}
	}
	else
	{
		if (carrierIsSampler)
			getFMCarrier()->handleRetriggeredNote(voice);
		else
			ModulatorSynth::handleRetriggeredNote(voice);
	}
}

bool ModulatorSynthGroup::handleVoiceLimit(int numVoicesToClear)
{
	jassert(numVoicesToClear == 1);

	bool killedSomeVoices = ModulatorSynth::handleVoiceLimit(numVoicesToClear);
	
	if (killedSomeVoices)
	{
		// If this happens, the group already has killed a few voices and should be ready to roll...
		return true;
	}
	else
	{
		// We still need to make room in each child synth for the new voice starts...
		for (auto cva : synthVoiceAmounts)
		{
			const int numVoicesNeededInChild = cva.numVoicesNeeded * unisonoVoiceAmount;

			int numFreeChildVoices = cva.s->getNumFreeVoices();


			while (numFreeChildVoices <= numVoicesNeededInChild)
			{
				const bool forceKill = numFreeChildVoices == 0;
				const auto killedThisTime = killLastVoice(!forceKill);

				if (killedThisTime != 0)
				{
					killedSomeVoices = true;
				}
				else
				{
					jassertfalse;
					break;
				}

				numFreeChildVoices += killedThisTime;
			}
		}
	}

	

	return killedSomeVoices;
}

void ModulatorSynthGroup::killAllVoices()
{
	for (auto v : activeVoices)
	{
		v->killVoice();

		for (auto c : static_cast<ModulatorSynthGroupVoice*>(v)->childSynths)
		{
			if (c.isActiveForThisVoice)
			{
				auto childVoice = c.synth->getVoice(v->getVoiceIndex());

				if (childVoice != nullptr)
				{
					static_cast<ModulatorSynthVoice*>(childVoice)->killVoice();
				}
			}
		}
	}

	effectChain->killMasterEffects();


}

void ModulatorSynthGroup::resetAllVoices()
{
	ModulatorSynth::resetAllVoices();

	for (auto s : synths)
		s->resetAllVoices();
}

String ModulatorSynthGroup::getFMStateString() const
{
	auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	if (fmEnabled)
	{
		if (carrierIndex == -1 || getChildProcessor(carrierIndex - 1 + offset) == nullptr)
		{
			return "The carrier syntesizer is not valid.";
		}
		else if (modIndex == -1 || getChildProcessor(modIndex - 1 + offset) == nullptr)
		{
			return "The modulation synthesizer is not valid.";
		}
		else if (modIndex == carrierIndex)
		{
			return "You can't use the same synthesiser as carrier and modulator.";
		}
		else
		{
			return "FM is working.";
		}
	}
	else
	{
		if (auto c = getFMCarrier())
		{
			return c->getId() + " is soloed (no FM)";
		}
		else
		{
			return "FM is deactivated";
		}
	}
}

void ModulatorSynthGroup::restoreFromValueTree(const ValueTree &v)
{
	ModulatorSynth::restoreFromValueTree(v);

	loadAttribute(EnableFM, "EnableFM");
	loadAttribute(CarrierIndex, "CarrierIndex");
	loadAttribute(ModulatorIndex, "ModulatorIndex");
	loadAttribute(UnisonoVoiceAmount, "UnisonoVoiceAmount");
	loadAttribute(UnisonoDetune, "UnisonoDetune");
	loadAttribute(UnisonoSpread, "UnisonoSpread");
	loadAttribute(KillSecondVoices, "KillSecondVoices");

}

ValueTree ModulatorSynthGroup::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	saveAttribute(EnableFM, "EnableFM");
	saveAttribute(CarrierIndex, "CarrierIndex");
	saveAttribute(ModulatorIndex, "ModulatorIndex");
	saveAttribute(UnisonoVoiceAmount, "UnisonoVoiceAmount");
	saveAttribute(UnisonoDetune, "UnisonoDetune");
	saveAttribute(UnisonoSpread, "UnisonoSpread");
	saveAttribute(KillSecondVoices, "KillSecondVoices");

	return v;
}

void ModulatorSynthGroup::checkFmState()
{
	getMainController()->getKillStateHandler().killVoicesAndCall(this, 
		[](Processor* p) {dynamic_cast<ModulatorSynthGroup*>(p)->checkFMStateInternally(); return SafeFunctionCall::OK; },
		MainController::KillStateHandler::TargetThread::SampleLoadingThread);


	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
}



void ModulatorSynthGroup::checkFMStateInternally()
{
	LockHelpers::freeToGo(getMainController());
	LockHelpers::SafeLock l(getMainController(), LockHelpers::Type::AudioLock, isOnAir());

	auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	if (fmEnabled)
	{
		if (carrierIndex == -1 || getChildProcessor(carrierIndex - 1 + offset) == nullptr)
		{
			fmCorrectlySetup = false;
		}
		else if (modIndex == -1 || getChildProcessor(modIndex - 1 + offset) == nullptr)
		{
			fmCorrectlySetup = false;
		}
		else if (modIndex == carrierIndex)
		{
			fmCorrectlySetup = false;
		}
		else
		{
			fmCorrectlySetup = true;
		}
	}
	else
	{
		if (getFMCarrier() != nullptr)
		{
			fmCorrectlySetup = false;
		}
		else
		{
			fmCorrectlySetup = false;
		}
	}
}

void ModulatorSynthGroup::ModulatorSynthGroupHandler::add(Processor *newProcessor, Processor * /*siblingToInsertBefore*/)
{
	ModulatorSynth *m = dynamic_cast<ModulatorSynth*>(newProcessor);

	if(getNumProcessors() >= 8)
	{
		debugError(m, "Can't add sound generator to synth group - exceeded max number of child synths (8).");

		// usually this will be owned by the group but since this is an error
		// we'll live with the memory leak of the processor...
		return;
	}

	// Check incompatibilites with SynthGroups

	if (m->getChildProcessor(ModulatorSynth::EffectChain)->getNumChildProcessors() != 0)
	{
        auto fxChain = m->getChildProcessor(ModulatorSynth::EffectChain);
        
        bool didSomething = false;
        
        for(int i = 0; i < fxChain->getNumChildProcessors(); i++)
        {
            auto fx = fxChain->getChildProcessor(i);
            
            if(dynamic_cast<VoiceEffectProcessor*>(fx) == nullptr)
            {
                dynamic_cast<Chain*>(fxChain)->getHandler()->remove(fx);
                i--;
                didSomething = true;
            }
        }
        
        if(didSomething)
            PresetHandler::showMessageWindow("Removed non-polyphonic FX", "A child of a synth group can only render polyphonic effects");
	}
	else if (dynamic_cast<ModulatorSampler*>(m) != nullptr && m->getAttribute(ModulatorSampler::VoiceAmount) != group->getNumVoices())
	{
		if (AlertWindow::showOkCancelBox(AlertWindow::AlertIconType::WarningIcon, "Different Voice Amount detected", "StreamingSamplers that are added to a SynthGroup must have the same voice number as the SynthGroup\n Press OK to resize the voice amount."))
		{
			dynamic_cast<ModulatorSampler*>(m)->setAttribute(ModulatorSampler::VoiceAmount, (float)group->getNumVoices(), sendNotification);
		}
		else
		{
			return;
		}
	}

	m->setGroup(group);
	m->prepareToPlay(group->getSampleRate(), group->getLargestBlockSize());

	m->setParentProcessor(group);

	{
		LOCK_PROCESSING_CHAIN(group);

		m->setIsOnAir(group->isOnAir());

		jassert(m != nullptr);
		group->synths.add(m);
		group->allowStates.setBit(group->synths.indexOf(m), true);

		for (int i = 0; i < group->getNumVoices(); i++)
		{
			static_cast<ModulatorSynthGroupVoice*>(group->getVoice(i))->addChildSynth(m);
		}

		group->checkFmState();
	}

	group->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);

	notifyListeners(Listener::ProcessorAdded, newProcessor);
}


void ModulatorSynthGroup::ModulatorSynthGroupHandler::remove(Processor *processorToBeRemoved, bool removeSynth)
{
	notifyListeners(Listener::ProcessorDeleted, processorToBeRemoved);

	ScopedPointer<ModulatorSynth> m = dynamic_cast<ModulatorSynth*>(processorToBeRemoved);

	{
		LOCK_PROCESSING_CHAIN(group);

		for (int i = 0; i < group->getNumVoices(); i++)
		{
			static_cast<ModulatorSynthGroupVoice*>(group->getVoice(i))->removeChildSynth(m);
		}

		m->setIsOnAir(false);
		group->synths.removeObject(m, false);
		group->checkFmState();
	}

	if (removeSynth)
		m = nullptr;
	else
		m.release();
}


Processor * ModulatorSynthGroup::ModulatorSynthGroupHandler::getProcessor(int processorIndex)
{
	return (group->synths[processorIndex]);
}


const Processor * ModulatorSynthGroup::ModulatorSynthGroupHandler::getProcessor(int processorIndex) const
{
	return (group->synths[processorIndex]);
}


int ModulatorSynthGroup::ModulatorSynthGroupHandler::getNumProcessors() const
{
	return group->synths.size();
}


void ModulatorSynthGroup::ModulatorSynthGroupHandler::clear()
{
	notifyListeners(Listener::Cleared, nullptr);

	group->synths.clear();
}

ModulatorSynthGroup::ChildSynthIterator::ChildSynthIterator(ModulatorSynthGroup *groupToBeIterated, Mode iteratorMode /*= SkipUnallowedSynths*/) :
	limit(groupToBeIterated->getHandler()->getNumProcessors()),
	counter(0),
	group(*groupToBeIterated),
	mode(iteratorMode)
{

}

bool ModulatorSynthGroup::ChildSynthIterator::getNextAllowedChild(ModulatorSynth *&child)
{
	if (mode == GetFMCarrierOnly && group.fmIsCorrectlySetup())
	{
		if (carrierWasReturned)
			return false;

		auto indexOffset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

		child = static_cast<ModulatorSynth*>(group.getChildProcessor(group.carrierIndex - 1 + indexOffset));

		carrierWasReturned = true;
		return true;
	}

	if (mode == SkipUnallowedSynths)
	{
		counter = group.allowStates.findNextSetBit(counter);
		if (counter == -1) return false;
	}

	child = group.synths[counter++];

	if (child == nullptr) return false;

	// This should not happen
	jassert(child != nullptr);

	return child != nullptr && counter <= limit;
}

float ModulatorSynthGroupVoice::DetuneValues::getGainFactor(bool getRightChannel)
{
	return gainFactor * (getRightChannel ? balanceRight : balanceLeft);
}

ModulatorSynthGroupVoice::Iterator::Iterator(ModulatorSynthGroupVoice* v_):
	v(v_)
{
	numSize = v->childSynths.size();
	mod = v->getFMModulator();
}

ModulatorSynthGroupVoice::ChildVoiceContainer::ChildVoiceContainer()
{
	clear();
}

void ModulatorSynthGroupVoice::ChildVoiceContainer::addVoice(ModulatorSynthVoice* v)
{
	jassert(numVoices < 8);
	voices[numVoices++] = v;
}

bool ModulatorSynthGroupVoice::ChildVoiceContainer::removeVoice(ModulatorSynthVoice* v)
{
	for (int i = 0; i < numVoices; i++)
	{
		if (voices[i] == v)
		{
			for (int j = i; j < numVoices-1; j++)
			{
				voices[j] = voices[j + 1];
			}

			voices[numVoices--] = nullptr;
			return true;
		}
	}

	return false;
}

ModulatorSynthVoice* ModulatorSynthGroupVoice::ChildVoiceContainer::getVoice(int index)
{
	if (index < numVoices)
	{
		return voices[index];
	}
	else
	{
		jassertfalse;
		return nullptr;
	}
}

int ModulatorSynthGroupVoice::ChildVoiceContainer::size() const
{
	return numVoices;
}

void ModulatorSynthGroupVoice::ChildVoiceContainer::clear()
{
	memset(voices, 0, sizeof(ModulatorSynthGroupVoice*) * 8);
	numVoices = 0;
}

ModulatorSynthGroupVoice::ChildVoiceContainer& ModulatorSynthGroupVoice::getChildContainer(int childVoiceIndex)
{
	return startedChildVoices[childVoiceIndex % NUM_MAX_UNISONO_VOICES];
}

uint64 ModulatorSynthGroupVoice::UnisonoState::getIndex(int index) const
{
	return static_cast<uint64>(index % bitsPerNumber);
}

uint64 ModulatorSynthGroupVoice::UnisonoState::getOffset(int index) const
{
	return static_cast<uint64>(index / bitsPerNumber);
}

ModulatorSynthGroupVoice::UnisonoState::UnisonoState()
{
	clear();
}

bool ModulatorSynthGroupVoice::UnisonoState::anyActive() const noexcept
{
	bool active = false;

	for (int i = 0; i < getNumInts(); i++)
	{
		active |= state[i] != 0;
	}

	return active;
}

void ModulatorSynthGroupVoice::UnisonoState::clearBit(int index)
{
	auto i = getIndex(index);
	auto offset = getOffset(index);
	auto v = ~(uint64(1) << i);

	state[offset] &= v;
}

bool ModulatorSynthGroupVoice::UnisonoState::isBitSet(int index) const
{
	auto i = getIndex(index);
	auto offset = getOffset(index);
	auto v = uint64(1) << i;

	return (state[offset] & v) != 0;
}

void ModulatorSynthGroupVoice::UnisonoState::setBit(int index)
{
	auto i = getIndex(index);
	auto offset = getOffset(index);

	auto v = uint64(1) << i;

	state[offset] |= v;
}

void ModulatorSynthGroupVoice::UnisonoState::clear()
{
	memset(state, 0, sizeof(uint64) * getNumInts());
}

ModulatorSynthGroupVoice::ChildSynth::ChildSynth():
	synth(nullptr),
	isActiveForThisVoice(false)
{}

ModulatorSynthGroupVoice::ChildSynth::ChildSynth(ModulatorSynth* synth_):
	synth(synth_),
	isActiveForThisVoice(false)
{}

ModulatorSynthGroupVoice::ChildSynth::ChildSynth(const ChildSynth& other):
	synth(other.synth),
	isActiveForThisVoice(other.isActiveForThisVoice)
{}

bool ModulatorSynthGroupVoice::ChildSynth::operator==(const ChildSynth& other) const
{
	return synth == other.synth;
}

ModulatorSynth* ModulatorSynthGroupVoice::Iterator::getNextActiveChildSynth()
{
	if (v->useFMForVoice)
	{
		if (i == 0)
		{
			i++;
			return v->getFMCarrier();
		}
		else
			return nullptr;
	}
	else
	{
		while (i < numSize)
		{
			auto c = v->childSynths[i].synth;

			if (v->childSynths[i].isActiveForThisVoice)
			{
				i++;
				return c;
			}
			else
			{
				i++;
			}
		}

		return nullptr;
	}
}

} // namespace hise
