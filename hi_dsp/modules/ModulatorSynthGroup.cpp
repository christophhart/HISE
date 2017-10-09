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


#define CREATE_ITERATOR() ModulatorSynthGroup::ChildSynthIterator iterator(static_cast<ModulatorSynthGroup*>(ownerSynth), ModulatorSynthGroup::ChildSynthIterator::IterateAllSynths); ModulatorSynth *childSynth;


ModulatorSynthGroupVoice::ModulatorSynthGroupVoice(ModulatorSynth *ownerSynth) :
	ModulatorSynthVoice(ownerSynth)
{
	enablePitchModulation(true);
}


bool ModulatorSynthGroupVoice::canPlaySound(SynthesiserSound *)
{
	return true;
}


void ModulatorSynthGroupVoice::addChildSynth(ModulatorSynth *childSynth)
{
	ScopedLock sl(ownerSynth->getSynthLock());

	childSynths.add(childSynth);
}


void ModulatorSynthGroupVoice::removeChildSynth(ModulatorSynth *childSynth)
{
	ScopedLock sl(ownerSynth->getSynthLock());

	jassert(childSynth != nullptr);
	jassert(childSynths.indexOf(childSynth) != -1);

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

	numUnisonoVoices = (int)getOwnerSynth()->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount);
	int numVoices = (int)getOwnerSynth()->getAttribute(ModulatorSynth::Parameters::VoiceLimit);

	const float detune = getOwnerSynth()->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoDetune);

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

		if (unisonoVoiceIndex >= numVoices) // don't start more voices than you have allocated
			break;

		CREATE_ITERATOR();

		while (iterator.getNextAllowedChild(childSynth))
		{
			startNoteInternal(childSynth, unisonoVoiceIndex, midiNoteNumber, velocity);

		}
			
	}
};


ModulatorSynthVoice* ModulatorSynthGroupVoice::startNoteInternal(ModulatorSynth* childSynth, int childVoiceIndex, int midiNoteNumber, float velocity)
{
	ModulatorSynthVoice* childVoice = static_cast<ModulatorSynthVoice*>(childSynth->getVoice(childVoiceIndex));

	ModulatorSynthSound *soundToPlay = nullptr;

	for (int j = 0; j < childSynth->getNumSounds(); j++)
	{
		ModulatorSynthSound *s = static_cast<ModulatorSynthSound*>(childSynth->getSound(j));

		if (s->appliesToMessage(1, midiNoteNumber, (int)(velocity * 127)))
		{
			soundToPlay = s;
			break; // only one sound at a time (can be changed later)
		}
	}

	if (soundToPlay == nullptr) return nullptr;

	childVoice->setCurrentHiseEvent(getCurrentHiseEvent());

	if (numUnisonoVoices != 1)
	{
		childVoice->addToStartOffset(startOffsetRandomizer.nextInt(441));
	}

	childSynth->preStartVoice(childVoiceIndex, midiNoteNumber);
	childVoice->startNote(midiNoteNumber, velocity, soundToPlay, -1);
}


void ModulatorSynthGroupVoice::calculateBlock(int startSample, int numSamples)
{
	ScopedLock sl(ownerSynth->getSynthLock());

	// Clear the buffer, since all child voices are added to this so it must be empty.
	voiceBuffer.clear();

	ModulatorSynthGroup *group = static_cast<ModulatorSynthGroup*>(getOwnerSynth());

	if (group->fmIsCorrectlySetup())
	{
		calculateFMBlock(group, startSample, numSamples);
	}
	else
	{
		calculateNoFMBlock(startSample, numSamples);

	}

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startSample, numSamples);

	const float *modValues = getVoiceGainValues(startSample, numSamples);

	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), modValues + startSample, numSamples);
	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startSample), modValues + startSample, numSamples);
};



void ModulatorSynthGroupVoice::calculateNoFMBlock(int startSample, int numSamples)
{
	const float *voicePitchValues = getVoicePitchValues();


	if (numUnisonoVoices == 1)
	{
		CREATE_ITERATOR();

		while (iterator.getNextAllowedChild(childSynth))
			calculateNoFMVoiceInternal(childSynth, voiceIndex, startSample, numSamples, voicePitchValues);
	}
	else
	{
		int numVoices = (int)getOwnerSynth()->getAttribute(ModulatorSynth::Parameters::VoiceLimit);

		for (int i = 0; i < numUnisonoVoices; i++)
		{
			const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

			CREATE_ITERATOR();

			while (iterator.getNextAllowedChild(childSynth))
				calculateNoFMVoiceInternal(childSynth, unisonoVoiceIndex, startSample, numSamples, voicePitchValues);
		}
	}


	

	CREATE_ITERATOR();

	
}


void ModulatorSynthGroupVoice::calculateNoFMVoiceInternal(ModulatorSynth * childSynth, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues)
{
	ModulatorSynthVoice *childVoice = static_cast<ModulatorSynthVoice*>(childSynth->getVoice(childVoiceIndex));

	if (childSynth->isBypassed() || childVoice->isInactive()) return;

	const float gain = childSynth->getGain();

	childVoice->calculateVoicePitchValues(startSample, numSamples);

	float *childPitchValues = childVoice->getVoicePitchValues();

	float detuneMultiplier = 1.0f;
	float detuneGainFactor = 1.0f;
	

	float detuneBalanceLeft = 1.0f;
	float detuneBalanceRight = 1.0f;

	if (numUnisonoVoices != 1)
	{
		// 0 ... voiceAmount -> -detune ... detune

		const float detune = ownerSynth->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoDetune);
		const float balance = ownerSynth->getAttribute(ModulatorSynthGroup::SpecialParameters::UnisonoSpread);

		const int unisonoIndex = childVoiceIndex % numUnisonoVoices;

		detuneGainFactor = 1.0f / sqrtf((float)numUnisonoVoices);

		const float normalizedVoiceIndex = (float)unisonoIndex / (float)(numUnisonoVoices - 1);
		const float normalizedDetuneAmount = normalizedVoiceIndex * 2.0f - 1.0f;
		const float detuneOctaveAmount = detune * normalizedDetuneAmount;
		detuneMultiplier = Modulation::PitchConverters::octaveRangeToPitchFactor(detuneOctaveAmount);

		const float detuneBalanceAmount = normalizedDetuneAmount * 100.0f * balance;

		

		detuneBalanceLeft = BalanceCalculator::getGainFactorForBalance(detuneBalanceAmount, true);
		detuneBalanceRight = BalanceCalculator::getGainFactorForBalance(detuneBalanceAmount, false);

	}

	if (childPitchValues != nullptr && voicePitchValues != nullptr)
	{
		FloatVectorOperations::multiply(childPitchValues + startSample, voicePitchValues + startSample, detuneMultiplier, numSamples);
	}

	childVoice->calculateBlock(startSample, numSamples);
	childSynth->setPeakValues(gain, gain);

	const float g_left = detuneBalanceLeft * detuneGainFactor * gain * childSynth->getBalance(false);
	const float g_right = detuneBalanceRight * detuneGainFactor * gain * childSynth->getBalance(true);

	voiceBuffer.addFrom(0, startSample, childVoice->getVoiceValues(0, startSample), numSamples, g_left);
	voiceBuffer.addFrom(1, startSample, childVoice->getVoiceValues(1, startSample), numSamples, g_right);
}

void ModulatorSynthGroupVoice::calculateFMBlock(ModulatorSynthGroup * group, int startSample, int numSamples)
{
	// Calculate the modulator

	const float *voicePitchValues = getVoicePitchValues();

	auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	ModulatorSynth *modSynth = static_cast<ModulatorSynth*>(group->getChildProcessor(group->modIndex - 1 + offset));
	jassert(modSynth != nullptr);

	ModulatorSynthVoice *modVoice = static_cast<ModulatorSynthVoice*>(modSynth->getVoice(voiceIndex));

	if (modSynth->isBypassed() || modVoice->isInactive()) return;

	modVoice->calculateVoicePitchValues(startSample, numSamples);

	const float modGain = modSynth->getGain();

	float *modPitchValues = modVoice->getVoicePitchValues();

	if (voicePitchValues != nullptr && modPitchValues != nullptr)
	{
		FloatVectorOperations::multiply(modPitchValues + startSample, voicePitchValues + startSample, numSamples);
	}

	modVoice->calculateBlock(startSample, numSamples);

	const float *modValues = modVoice->getVoiceValues(0, startSample); // Channel is the same;

	FloatVectorOperations::copy(fmModBuffer, modValues, numSamples);
	FloatVectorOperations::multiply(fmModBuffer, group->modSynthGainValues.getReadPointer(0, startSample), numSamples);
	FloatVectorOperations::multiply(fmModBuffer, modGain, numSamples);

	const float peak = FloatVectorOperations::findMaximum(fmModBuffer, numSamples);

	FloatVectorOperations::add(fmModBuffer, 1.0f, numSamples);

	modSynth->setPeakValues(peak, peak);

	// Calculate the carrier voice

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;
		calculateFMCarrierInternal(group, unisonoVoiceIndex, startSample, numSamples, voicePitchValues);
	}

	
}


void ModulatorSynthGroupVoice::calculateFMCarrierInternal(ModulatorSynthGroup * group, int childVoiceIndex, int startSample, int numSamples, const float * voicePitchValues)
{
	auto indexOffset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	ModulatorSynth *carrierSynth = static_cast<ModulatorSynth*>(group->getChildProcessor(group->carrierIndex - 1 + indexOffset));
	jassert(carrierSynth != nullptr);

	ModulatorSynthVoice *carrierVoice = static_cast<ModulatorSynthVoice*>(carrierSynth->getVoice(childVoiceIndex));


	if (carrierSynth->isBypassed() || carrierVoice->isInactive()) return;

	carrierVoice->calculateVoicePitchValues(startSample, numSamples);

	float *carrierPitchValues = carrierVoice->getVoicePitchValues();

	FloatVectorOperations::multiply(carrierPitchValues + startSample, voicePitchValues + startSample, numSamples);

	// This is the magic FM command
	FloatVectorOperations::multiply(carrierPitchValues + startSample, fmModBuffer, numSamples);

#if JUCE_WINDOWS
	FloatVectorOperations::clip(carrierPitchValues + startSample, carrierPitchValues + startSample, 0.00000001f, 1000.0f, numSamples);
#endif
	carrierVoice->calculateBlock(startSample, numSamples);

	const float carrierGain = carrierSynth->getGain();

	voiceBuffer.addFrom(0, startSample, carrierVoice->getVoiceValues(0, startSample), numSamples, carrierGain * carrierSynth->getBalance(false));
	voiceBuffer.addFrom(1, startSample, carrierVoice->getVoiceValues(1, startSample), numSamples, carrierGain * carrierSynth->getBalance(true));

	const float peak2 = FloatVectorOperations::findMaximum(carrierVoice->getVoiceValues(0, startSample), numSamples);
	carrierSynth->setPeakValues(peak2, peak2);
}

void ModulatorSynthGroupVoice::stopNote(float, bool)
{
	DBG("Stop Voice " + String(voiceIndex));

	for (int i = 0; i < numUnisonoVoices; i++)
	{
		const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

		DBG(unisonoVoiceIndex);

		CREATE_ITERATOR();

		while (iterator.getNextAllowedChild(childSynth))
			stopNoteInternal(childSynth, unisonoVoiceIndex);
	}

	

	ModulatorSynthVoice::stopNote(0.0, false);
};


void ModulatorSynthGroupVoice::stopNoteInternal(ModulatorSynth * childSynth, int childVoiceIndex)
{
	ModulatorChain *g = static_cast<ModulatorChain*>(childSynth->getChildProcessor(ModulatorSynth::GainModulation));
	ModulatorChain *p = static_cast<ModulatorChain*>(childSynth->getChildProcessor(ModulatorSynth::PitchModulation));

	g->stopVoice(voiceIndex);
	p->stopVoice(voiceIndex);
}

void ModulatorSynthGroupVoice::checkRelease()
{
	ModulatorChain *ownerGainChain = static_cast<ModulatorChain*>(ownerSynth->getChildProcessor(ModulatorSynth::GainModulation));
	//ModulatorChain *ownerPitchChain = static_cast<ModulatorChain*>(ownerSynth->getChildProcessor(ModulatorSynth::PitchModulation));

	if (killThisVoice && (killFadeLevel < 0.001f))
	{
		resetVoice();

		for (int i = 0; i < numUnisonoVoices; i++)
		{
			const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

			CREATE_ITERATOR();

			while (iterator.getNextAllowedChild(childSynth))
				resetInternal(childSynth, voiceIndex);
		}

		return;
	}

	if (!ownerGainChain->isPlaying(voiceIndex))
	{
		resetVoice();

		for (int i = 0; i < numUnisonoVoices; i++)
		{
			const int unisonoVoiceIndex = voiceIndex*numUnisonoVoices + i;

			CREATE_ITERATOR();

			while (iterator.getNextAllowedChild(childSynth))
				resetInternal(childSynth, voiceIndex);
		}
	}
}




void ModulatorSynthGroupVoice::resetInternal(ModulatorSynth * childSynth, int childVoiceIndex)
{
	ModulatorSynthVoice *childVoice = static_cast<ModulatorSynthVoice*>(childSynth->getVoice(voiceIndex));

	childSynth->setPeakValues(0.0f, 0.0f);
	childVoice->resetVoice();
}

ModulatorSynthGroup::ModulatorSynthGroup(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	numVoices(numVoices),
	handler(this),
	vuValue(0.0f),
	fmEnabled(getDefaultValue(ModulatorSynthGroup::SpecialParameters::EnableFM) > 0.5f),
	carrierIndex((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::CarrierIndex)),
	modIndex((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::ModulatorIndex)),
	fmState("FM disabled"),
	fmCorrectlySetup(false),
	unisonoVoiceAmount((int)getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoVoiceAmount)),
	unisonoDetuneAmount((double)getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoDetune)),
	unisonoSpreadAmount(getDefaultValue(ModulatorSynthGroup::SpecialParameters::UnisonoSpread)),
	sampleStartChain(new ModulatorChain(mc, "Sample Start Modulation", numVoices, Modulation::GainMode, this))
{
	setFactoryType(new ModulatorSynthChainFactoryType(numVoices, this));
	getFactoryType()->setConstrainer(new SynthGroupConstrainer());

	setGain(1.0);

	parameterNames.add("EnableFM");
	parameterNames.add("CarrierIndex");
	parameterNames.add("ModulatorIndex");
	parameterNames.add("UnisonoVoiceAmount");
	parameterNames.add("UnisonoDetune");
	parameterNames.add("UnisonoSpread");

	allowStates.clear();

	for (int i = 0; i < numVoices; i++) addVoice(new ModulatorSynthGroupVoice(this));
	addSound(new ModulatorSynthGroupSound());
};


ModulatorSynthGroup::~ModulatorSynthGroup()
{
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
	case EnableFM:			 fmEnabled = (newValue > 0.5f); checkFmState(); break;

	case ModulatorIndex:	 modIndex = (int)newValue; checkFmState(); break;
	case CarrierIndex:		 carrierIndex = (int)newValue; checkFmState(); break;
	case UnisonoVoiceAmount: setUnisonoVoiceAmount((int)newValue); break;
	case UnisonoDetune:		 setUnisonoDetuneAmount(newValue); break;
	case UnisonoSpread:		 setUnisonoSpreadAmount(newValue); break;
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
	case ModulatorIndex: return (float)0;
	case CarrierIndex:	 return (float)0;
	case UnisonoVoiceAmount: return 1.0f;
	case UnisonoDetune:		 return 0.0f;
	case UnisonoSpread:		 return 0.0f;
	default:			 jassertfalse; return -1.0f;
	}
}

Processor * ModulatorSynthGroup::getChildProcessor(int processorIndex)
{
	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else if (processorIndex == SampleStartModulation)		return sampleStartChain;
	else													return handler.getProcessor(processorIndex - numInternalChains);
}


const Processor * ModulatorSynthGroup::getChildProcessor(int processorIndex) const
{
	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else if (processorIndex == SampleStartModulation)		return sampleStartChain;
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
	unisonoVoiceAmount = newVoiceAmount;
}


void ModulatorSynthGroup::setUnisonoDetuneAmount(float newDetuneAmount)
{
	unisonoDetuneAmount = newDetuneAmount;
}


void ModulatorSynthGroup::setUnisonoSpreadAmount(float newSpreadAmount)
{
	unisonoSpreadAmount = newSpreadAmount;
}

void ModulatorSynthGroup::preHiseEventCallback(const HiseEvent &m)
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
		modSynthGainValues = AudioSampleBuffer(1, samplesPerBlock);

		ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

		sampleStartChain->prepareToPlay(newSampleRate, samplesPerBlock);

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


void ModulatorSynthGroup::preVoiceRendering(int startSample, int numThisTime)
{
	ModulatorSynth::preVoiceRendering(startSample, numThisTime);

	ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
	ModulatorSynth *childSynth;

	if (fmCorrectlySetup)
	{
		auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

		ModulatorSynth *modSynth = static_cast<ModulatorSynth*>(getChildProcessor(modIndex - 1 + offset));
		ModulatorChain *gainChainOfModSynth = static_cast<ModulatorChain*>(modSynth->getChildProcessor(ModulatorSynth::GainModulation));

		gainChainOfModSynth->renderNextBlock(modSynthGainValues, startSample, numThisTime);
	}

	while (iterator.getNextAllowedChild(childSynth))
	{
		childSynth->preVoiceRendering(startSample, numThisTime);
	}
}


void ModulatorSynthGroup::postVoiceRendering(int startSample, int numThisTime)
{
	ChildSynthIterator iterator(this, ChildSynthIterator::IterateAllSynths);
	ModulatorSynth *childSynth;

	while (iterator.getNextAllowedChild(childSynth))
	{
		childSynth->postVoiceRendering(startSample, numThisTime);

	}

	// Apply the gain after the rendering of the child synths...
	ModulatorSynth::postVoiceRendering(startSample, numThisTime);
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

	return v;
}

void ModulatorSynthGroup::checkFmState()
{
	auto offset = (int)ModulatorSynthGroup::InternalChains::numInternalChains;

	if (fmEnabled)
	{
		if (carrierIndex == 0 || getChildProcessor(carrierIndex - 1 + offset) == nullptr)
		{
			fmState = "The carrier syntesizer is not valid.";
			fmCorrectlySetup = false;
		}
		else if (modIndex == 0 || getChildProcessor(modIndex - 1 + offset) == nullptr)
		{
			fmState = "The modulation synthesizer is not valid.";
			fmCorrectlySetup = false;
		}
		else if (modIndex == carrierIndex)
		{
			fmState = "You can't use the same synthesiser as carrier and modulator.";
			fmCorrectlySetup = false;
		}
		else
		{
			fmState = "FM is working.";
			fmCorrectlySetup = true;
		}
	}
	else
	{
		fmState = "FM is deactivated";
		fmCorrectlySetup = false;
	}

	if (fmCorrectlySetup)
	{
		enablePitchModulation(true);

		static_cast<ModulatorSynth*>(getChildProcessor(carrierIndex - 1 + offset))->enablePitchModulation(true);
	}



	sendChangeMessage();
}



void ModulatorSynthGroup::ModulatorSynthGroupHandler::add(Processor *newProcessor, Processor * /*siblingToInsertBefore*/)
{


	ModulatorSynth *m = dynamic_cast<ModulatorSynth*>(newProcessor);

	// Check incompatibilites with SynthGroups

	if (m->getChildProcessor(ModulatorSynth::EffectChain)->getNumChildProcessors() != 0)
	{
		if (AlertWindow::showOkCancelBox(AlertWindow::AlertIconType::WarningIcon, "Effects detected", "Synths that are added to a SynthGroup are not allowed to have effects.\n Press OK to create the synth with all effects removed"))
		{
			dynamic_cast<EffectProcessorChain*>(m->getChildProcessor(ModulatorSynth::EffectChain))->getHandler()->clear();
			m->setEditorState(ModulatorSynth::EffectChainShown, false);
		}
		else
		{
			return;
		}
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

	m->enablePitchModulation(true);
	m->setGroup(group);
	m->prepareToPlay(group->getSampleRate(), group->getBlockSize());


	{
		MainController::ScopedSuspender ss(group->getMainController());

		m->setIsOnAir(true);

		jassert(m != nullptr);
		group->synths.add(m);

		group->allowStates.setBit(group->synths.indexOf(m), true);

		for (int i = 0; i < group->getNumVoices(); i++)
		{
			static_cast<ModulatorSynthGroupVoice*>(group->getVoice(i))->addChildSynth(m);
		}



		group->checkFmState();

	}


	group->sendChangeMessage();

	sendChangeMessage();
}


void ModulatorSynthGroup::ModulatorSynthGroupHandler::remove(Processor *processorToBeRemoved)
{
	{
		MainController::ScopedSuspender ss(group->getMainController(), MainController::ScopedSuspender::LockType::Lock);

		ModulatorSynth *m = dynamic_cast<ModulatorSynth*>(processorToBeRemoved);

		for (int i = 0; i < group->getNumVoices(); i++)
		{
			static_cast<ModulatorSynthGroupVoice*>(group->getVoice(i))->removeChildSynth(m);
		}

		group->synths.removeObject(m);

		group->checkFmState();
	}

	sendChangeMessage();
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
	group->synths.clear();

	sendChangeMessage();
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

#undef CREATE_ITERATOR