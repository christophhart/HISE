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


void ModulatorSamplerSound::clipRangeProperties(const Identifier& id, int value, bool useUndo)
{
	if (id == SampleIds::SampleStart || id == SampleIds::SampleEnd)
	{
		auto loopStart = getPropertyValueWithDefault(SampleIds::LoopStart);
		auto startMod = getPropertyValueWithDefault(SampleIds::SampleStartMod);
		auto sampleEnd = getPropertyValueWithDefault(SampleIds::SampleEnd);
		auto loopEnd = getPropertyValueWithDefault(SampleIds::LoopEnd);
		auto xfade = getPropertyValueWithDefault(SampleIds::LoopXFade);

		if (id == SampleIds::SampleStart)
		{
			// trim the xfade first
			if (value > loopStart - xfade)
				setSampleProperty(SampleIds::LoopXFade, jmax(loopStart - value, 0), useUndo);

			// now trim the loop start if the xfade wasn't enough
			if (value > loopStart)
				setSampleProperty(SampleIds::LoopStart, value, useUndo);

			if (startMod > sampleEnd - value)
				setSampleProperty(SampleIds::SampleStartMod, sampleEnd - value, useUndo);

			if (value > loopEnd - xfade)
				setSampleProperty(SampleIds::LoopXFade, jmax(0, loopEnd - value), useUndo);

			if (value > loopEnd)
				setSampleProperty(SampleIds::LoopEnd, value, useUndo);
		}

		if (id == SampleIds::SampleEnd)
		{
			if (loopEnd > value)
				setSampleProperty(SampleIds::LoopEnd, value, useUndo);

			if (loopStart > value)
				setSampleProperty(SampleIds::LoopStart, value, useUndo);
		}
	}
}

void ModulatorSamplerSound::loadSampleFromValueTree(const ValueTree& sampleData, HlacMonolithInfo* hmaf)
{
	auto pool = parentMap->getCurrentSamplePool();

	auto filename = sampleData.getProperty(SampleIds::FileName).toString();

	if (FileHandlerBase::isAbsolutePathCrossPlatform(filename) && hmaf != nullptr)
	{
		// Just create a dummy reference string, it won't be used for anything else than identification...
		filename = "{PROJECT_FOLDER}" + FileHandlerBase::getFileNameCrossPlatform(filename, true);
	}

	PoolReference ref(getMainController(), filename, ProjectHandler::SubDirectories::Samples);

	auto existingSample = pool->getSampleFromPool(ref);

	if (existingSample != nullptr && existingSample->isMonolithic() != (hmaf != nullptr))
	{
		pool->removeFromPool(ref);
		existingSample = nullptr;
	}

	if (existingSample != nullptr)
	{
		soundArray.add(existingSample);
		data.setProperty("Duplicate", true, nullptr);
	}
	else
	{
		data.setProperty("Duplicate", false, nullptr);

		if (hmaf != nullptr)
		{
			int multimicIndex = isMultiMicSound ? sampleData.getParent().indexOf(sampleData) : 0;

			soundArray.add(new StreamingSamplerSound(hmaf, multimicIndex, getId()));
		}
		else
		{
			soundArray.add(new StreamingSamplerSound(ref.getFile().getFullPathName(), pool));
		}

		pool->addSound({ ref, soundArray.getLast().get() });
	}
}

int ModulatorSamplerSound::getPropertyValueWithDefault(const Identifier& id) const
{
	if (auto s = getReferenceToSound(0))
	{
		auto fullLength = (int)s->getLengthInSamples();

		if (id == SampleIds::SampleEnd)
			return (int)data.getProperty(SampleIds::SampleEnd, fullLength);

		if (id == SampleIds::LoopEnd)
			return (int)data.getProperty(SampleIds::LoopEnd, getPropertyValueWithDefault(SampleIds::SampleEnd));
	}

	return (int)data.getProperty(id, 0);
}

ModulatorSamplerSound::ModulatorSamplerSound(SampleMap* parent, const ValueTree& d, HlacMonolithInfo* hmaf) :
	ControlledObject(parent->getSampler()->getMainController()),
	parentMap(parent),
	data(d),
	undoManager(getMainController()->getControlUndoManager()),
	isMultiMicSound(data.getNumChildren() != 0),
	gain(1.0f),
	maxRRGroup(parent->getNumRRGroups()),
	normalizedPeak(-1.0f),
	isNormalized(false),
	purgeChannels(0),
	pitchFactor(1.0)
{
	if (isMultiMicSound)
	{
		for (const auto& child: data)
			loadSampleFromValueTree(child, hmaf);
	}
	else
	{
		loadSampleFromValueTree(data, hmaf);	
	}

	firstSound = soundArray.getFirst().get();

	auto gv = parent->getCrossfadeGammaValue();

	for (auto s : soundArray)
	{
		s->setDelayPreloadInitialisation(true);
		s->setCrossfadeGammaValue(gv);
	}

	ScopedValueSetter<bool> svs(enableAsyncPropertyChange, false);

	for (int i = 0; i < data.getNumProperties(); i++)
	{
		const Identifier id_ = data.getPropertyName(i);

		updateInternalData(id_, data.getProperty(id_));
	}
    
    for(auto s: soundArray)
        s->setDelayPreloadInitialisation(false);
}

ModulatorSamplerSound::~ModulatorSamplerSound()
{   
	if(parentMap != nullptr)
		parentMap->getCurrentSamplePool()->clearUnreferencedSamples();

	firstSound = nullptr;
	soundArray.clear();
}

bool ModulatorSamplerSound::isAsyncProperty(const Identifier& id)
{
	return id == SampleIds::SampleStart || id == SampleIds::SampleEnd || id == SampleIds::LoopStart || id == SampleIds::LoopEnabled ||
		id == SampleIds::SampleStartMod || id == SampleIds::LoopEnd || id == SampleIds::LoopXFade || id == SampleIds::SampleState;
}

juce::Range<int> ModulatorSamplerSound::getPropertyRange(const Identifier& id) const
{
	auto s = soundArray.getFirst();

	if (s == nullptr)
		return {};

	if (id == SampleIds::ID)				return Range<int>(0, INT_MAX);
	else if (id == SampleIds::FileName)		return Range<int>();
	else if (id == SampleIds::Root)		return Range<int>(0, 127);
	else if (id == SampleIds::HiKey)		return Range<int>((int)getSampleProperty(SampleIds::LoKey), 127);
	else if (id == SampleIds::LoKey)		return Range<int>(0, (int)getSampleProperty(SampleIds::HiKey));
	else if( id == SampleIds::LoVel)		
		return Range<int>(0, (int)getSampleProperty(SampleIds::HiVel) -
							 (int)getSampleProperty(SampleIds::LowerVelocityXFade) -
							 (int)getSampleProperty(SampleIds::UpperVelocityXFade));
	else if( id == SampleIds::HiVel)		
		return Range<int>((int)getSampleProperty(SampleIds::LoVel) +
						  (int)getSampleProperty(SampleIds::LowerVelocityXFade) +
						  (int)getSampleProperty(SampleIds::UpperVelocityXFade), 127);
	else if( id == SampleIds::Volume)		return Range<int>(-100, 18);
	else if( id == SampleIds::Pan)			return Range<int>(-100, 100);
	else if( id == SampleIds::Normalized)	return Range<int>(0, 1);
	else if( id == SampleIds::RRGroup)		return Range<int>(1, maxRRGroup);
	else if( id == SampleIds::Pitch)		return Range<int>(-100, 100);
	else if (id == SampleIds::LoopEnabled)			return Range<int>(0, 1);
	else if (id == SampleIds::SampleStart || id == SampleIds::SampleEnd ||
		id == SampleIds::LoopStart || id == SampleIds::LoopEnd ||
		id == SampleIds::SampleStartMod || id == SampleIds::LoopXFade)
	{
		// get all interesting properties with a sensible default
		auto fullLength = (int)firstSound->getLengthInSamples();
		auto sampleStart = getPropertyValueWithDefault(SampleIds::SampleStart);
		auto sampleEnd = getPropertyValueWithDefault(SampleIds::SampleEnd);
		auto loopStart = getPropertyValueWithDefault(SampleIds::LoopStart);
		auto loopEnd = getPropertyValueWithDefault(SampleIds::LoopEnd);
		//auto startMod = getPropertyValueWithDefault(SampleIds::SampleStartMod);
		auto xfade = getPropertyValueWithDefault(SampleIds::LoopXFade);

		int minValue, maxValue;

		minValue = 0;
		maxValue = 0;

		if (id == SampleIds::SampleStart)
		{
			minValue = 0;
			maxValue = sampleEnd;
					   //jmin(sampleEnd - startMod,
					   //loopStart - xfade);
		}
		if (id == SampleIds::SampleEnd)
		{
			minValue = sampleStart;// jmax(sampleStart + startMod, loopEnd);
			maxValue = fullLength;
		}
		if (id == SampleIds::LoopStart)
		{
			minValue = sampleStart + xfade;
			maxValue = loopEnd - xfade;
		}
		if (id == SampleIds::LoopEnd)
		{
			minValue = loopStart + xfade;
			maxValue = sampleEnd;
		}
		if (id == SampleIds::SampleStartMod)
		{
			minValue = 0;
			maxValue = sampleEnd - sampleStart;
		}
		if (id == SampleIds::LoopXFade)
		{
			minValue = 0;
			maxValue = jmin(loopEnd - loopStart,
				loopStart - sampleStart);
		}

		return { minValue, maxValue };
	}
	else if( id == SampleIds::UpperVelocityXFade)	return Range < int >(0, (int)getSampleProperty(SampleIds::HiVel) - ((int)getSampleProperty(SampleIds::LoVel) + lowerVeloXFadeValue));
	else if( id == SampleIds::LowerVelocityXFade)	return Range < int >(0, (int)getSampleProperty(SampleIds::HiVel) - upperVeloXFadeValue - (int)getSampleProperty(SampleIds::LoVel));
	else if( id == SampleIds::SampleState)			return Range<int>(0, (int)StreamingSamplerSound::numSampleStates - 1);
    else if (id == SampleIds::NumQuarters) return Range<int>(0, 128);
    
	jassertfalse;
	return {};
}

juce::String ModulatorSamplerSound::getPropertyAsString(const Identifier& id) const
{
	auto s = soundArray.getFirst();

	if (s == nullptr)
		return {};

	auto v = getSampleProperty(id);

	if( id == SampleIds::Root)			return MidiMessage::getMidiNoteName((int)v, true, true, 3);
	else if (id == SampleIds::FileName)		return firstSound->getFileName(true);
	else if( id == SampleIds::HiKey)		return MidiMessage::getMidiNoteName((int)v, true, true, 3);
	else if( id == SampleIds::LoKey)		return MidiMessage::getMidiNoteName((int)v, true, true, 3);
	else if( id == SampleIds::Volume)		return String(Decibels::gainToDecibels(gain.load()), 1) + " dB";
	else if( id == SampleIds::Pan)			return BalanceCalculator::getBalanceAsString((int)v);
	else if( id == SampleIds::Normalized)	return v ? "Enabled" : "Disabled";
	else if( id == SampleIds::Pitch)			return String((int)v) + " ct";
	else if( id == SampleIds::LoopEnabled)	return firstSound->isLoopEnabled() ? "Enabled" : "Disabled";
	else if( id == SampleIds::SampleState)	return firstSound->getSampleStateAsString();

	return v.toString();
}


void ModulatorSamplerSound::toggleBoolProperty(const Identifier& id)
{
	if (id == SampleIds::Normalized)
	{
		isNormalized = !isNormalized;

		data.setProperty(id, isNormalized, undoManager);

		if (isNormalized)
			calculateNormalizedPeak();
	}
	else if (id == SampleIds::LoopEnabled)
	{
		const bool wasEnabled = firstSound->isLoopEnabled();

		data.setProperty(id, !wasEnabled, undoManager);

		FOR_EVERY_SOUND(setLoopEnabled(!wasEnabled));
	}
}

void ModulatorSamplerSound::startPropertyChange(const Identifier& /*id*/, int /*newValue*/)
{
}

void ModulatorSamplerSound::endPropertyChange(const Identifier& /*id*/, int /*startValue*/, int /*endValue*/)
{
}

void ModulatorSamplerSound::endPropertyChange(const String &/*actionName*/)
{
}


void ModulatorSamplerSound::openFileHandle()
{
	FOR_EVERY_SOUND(openFileHandle());
}

void ModulatorSamplerSound::closeFileHandle()
{
	FOR_EVERY_SOUND(closeFileHandle());
}

Range<int> ModulatorSamplerSound::getNoteRange() const			{ return Range<int>((int)getSampleProperty(SampleIds::LoKey), (int)getSampleProperty(SampleIds::HiKey) + 1); }
Range<int> ModulatorSamplerSound::getVelocityRange() const {
	return Range<int>((int)getSampleProperty(SampleIds::LoVel), (int)getSampleProperty(SampleIds::HiVel) + 1);
}
float ModulatorSamplerSound::getPropertyVolume() const noexcept { return gain.load(); }
double ModulatorSamplerSound::getPropertyPitch() const noexcept { return pitchFactor.load(); }

double ModulatorSamplerSound::getMaxPitchRatio() const
{
    if (auto s = getReferenceToSound())
    {
        int hiKey = (int)getSampleProperty(SampleIds::HiKey);
        int root = (int)getSampleProperty(SampleIds::Root);
        return s->getPitchFactor(hiKey, root);
    }
    
    return 1.0;
}
    
bool ModulatorSamplerSound::noteRangeExceedsMaxPitch() const
{
    return getMaxPitchRatio() > (double)MAX_SAMPLER_PITCH;
}

void ModulatorSamplerSound::loadEntireSampleIfMaxPitch()
{
	if(noteRangeExceedsMaxPitch())
	{
		auto safeThis = WeakReference<ModulatorSamplerSound>(this);

		auto f = [this, safeThis](Processor* )
		{
			if (safeThis.get() == nullptr)
				return SafeFunctionCall::OK;

			FOR_EVERY_SOUND(loadEntireSample());
			return SafeFunctionCall::OK;
		};

		auto s = parentMap->getSampler();
		
		s->getMainController()->getKillStateHandler().killVoicesAndCall(s, f, MainController::KillStateHandler::SampleLoadingThread);
	}
}

void ModulatorSamplerSound::setMaxRRGroupIndex(int newGroupLimit)
{
	maxRRGroup = newGroupLimit;
	rrGroup = jmin<int>((int)data.getProperty(SampleIds::RRGroup), newGroupLimit);
}

void ModulatorSamplerSound::setMappingData(MappingData newData)
{
	for (int i = 0; i < newData.data.getNumProperties(); i++)
	{
		const Identifier id_(newData.data.getPropertyName(i));
		setSampleProperty(id_, newData.data[id_]);
	}
}

void ModulatorSamplerSound::calculateNormalizedPeak()
{
	float highestPeak = 0.0f;

	for (auto s : soundArray)
		highestPeak = jmax<float>(highestPeak, s->calculatePeakValue());

	if (highestPeak != 0.0f)
	{
		normalizedPeak = jlimit<float>(1.0f, 1024.0f, 1.0f / highestPeak);
		data.setProperty(SampleIds::NormalizedPeak, normalizedPeak, nullptr);
	}
	else
	{
		normalizedPeak = 0.0f;
		data.setProperty(SampleIds::NormalizedPeak, 0.0f, nullptr);
	}
}

void ModulatorSamplerSound::removeNormalisationInfo(UndoManager* um)
{
	data.setProperty(SampleIds::Normalized, 0, um);
	data.removeProperty(SampleIds::NormalizedPeak, um);
}

float ModulatorSamplerSound::getNormalizedPeak() const
{
	return (isNormalized && normalizedPeak != -1.0f) ? normalizedPeak : 1.0f;
}

float ModulatorSamplerSound::getBalance(bool getRightChannelGain) const
{
	return getRightChannelGain ? rightBalanceGain : leftBalanceGain;
}

void ModulatorSamplerSound::setReversed(bool shouldBeReversed)
{
	if (reversed != shouldBeReversed)
	{
		reversed = shouldBeReversed;

		FOR_EVERY_SOUND(setReversed(reversed));
	}
}

void ModulatorSamplerSound::setVelocityXFade(int crossfadeLength, bool isUpperSound)
{
	if (isUpperSound) lowerVeloXFadeValue = crossfadeLength;
	else upperVeloXFadeValue = crossfadeLength;
}

void ModulatorSamplerSound::setPurged(bool shouldBePurged) 
{
	if (purged != shouldBePurged)
	{
		purged = shouldBePurged;
		FOR_EVERY_SOUND(setPurged(shouldBePurged));
	}
	
}

void ModulatorSamplerSound::checkFileReference()
{
	allFilesExist = true;

	FOR_EVERY_SOUND(checkFileReference())

	for (auto s: soundArray)
	{
		if (s->isMissing())
		{
			allFilesExist = false;
			break;
		}
	}
}

float ModulatorSamplerSound::getGainValueForVelocityXFade(int velocity)
{
	if (upperVeloXFadeValue == 0 && lowerVeloXFadeValue == 0) return 1.0f;

	Range<int> upperRange = Range<int>(velocityRange.getHighestBit() - upperVeloXFadeValue, velocityRange.getHighestBit());
	Range<int> lowerRange = Range<int>(velocityRange.findNextSetBit(0), velocityRange.findNextSetBit(0) + lowerVeloXFadeValue);

	float delta = 1.0f;

	if (upperRange.contains(velocity))
	{
		delta = (float)(velocity - upperRange.getStart()) / (upperRange.getLength());

		return Interpolator::interpolateLinear(1.0f, 0.0f, delta);
	}
	else if (lowerRange.contains(velocity))
	{
		delta = (float)(velocity - lowerRange.getStart()) / (lowerRange.getLength());

		return Interpolator::interpolateLinear(0.0f, 1.0f, delta);
	}
	else
	{
		return 1.0f;
	}
}

int ModulatorSamplerSound::getNumMultiMicSamples() const noexcept { return soundArray.size(); }

bool ModulatorSamplerSound::isChannelPurged(int channelIndex) const { return purgeChannels[channelIndex]; }

void ModulatorSamplerSound::setChannelPurged(int channelIndex, bool shouldBePurged)
{
	if (purged) return;

	purgeChannels.setBit(channelIndex, shouldBePurged);

	if (auto s = soundArray[channelIndex])
	{
		s->setPurged(shouldBePurged);
	}
}

bool ModulatorSamplerSound::preloadBufferIsNonZero() const noexcept
{
	for (auto s: soundArray)
	{
		if (!s->isPurged() && s->getPreloadBuffer().getNumSamples() != 0)
		{
			return true;
		}
	}

	return false;
}

bool ModulatorSamplerSound::hasUnpurgedButUnloadedSounds() const
{
	for (auto s : soundArray)
	{
		if (s->isPurged())
			continue;

		if (s->getPreloadBuffer().getNumSamples() == 0)
			return true;
	}

	return false;
}

int ModulatorSamplerSound::getRRGroup() const {	return rrGroup; }

void ModulatorSamplerSound::selectSoundsBasedOnRegex(const String &regexWildcard, ModulatorSampler *sampler, SelectedItemSet<ModulatorSamplerSound::Ptr> &set)
{
	bool subtractMode = false;

	bool addMode = false;

	String wildcard = regexWildcard;

	if (wildcard.startsWith("sub:"))
	{
		subtractMode = true;
		wildcard = wildcard.fromFirstOccurrenceOf("sub:", false, true);
	}
	else if (wildcard.startsWith("add:"))
	{
		addMode = true;
		wildcard = wildcard.fromFirstOccurrenceOf("add:", false, true);
	}
	else
	{
		set.deselectAll();
	}

	if (wildcard.contains("&"))
	{
		// Regex doesn't have a nice & operator, so we need to implement
		// this manually...

		auto ops = StringArray::fromTokens(regexWildcard, "&", "");

		OwnedArray<SelectedItemSet<Ptr>> andSelections;

		for (auto op : ops)
		{
			auto opSet = new SelectedItemSet<Ptr>();
			selectSoundsBasedOnRegex(op, sampler, *opSet);
			andSelections.add(opSet);
		}

		if (!andSelections.isEmpty())
		{
			SelectedItemSet<Ptr>* smallest = andSelections.getFirst();

			// Find the one with the smallest number, this will make the next loop
			// much faster...
			for (auto s: andSelections)
			{
				if (s->getNumSelected() < smallest->getNumSelected())
					smallest = s;
			}

			for (const auto& thisSound: *smallest)
			{
				bool found = true;

				for (auto s: andSelections)
				{
					if (!s->isSelected(thisSound))
					{
						found = false;
						break;
					}
				}

				if (found)
					set.addToSelection(thisSound);
			}
		}
	}
	else
	{
		try
		{
			std::regex reg(wildcard.toStdString());

			ModulatorSampler::SoundIterator iter(sampler, false);

			while (auto sound = iter.getNextSound())
			{
				const String name = sound->getPropertyAsString(SampleIds::FileName);

				if (std::regex_search(name.toStdString(), reg))
				{
					if (subtractMode)
						set.deselect(sound.get());
					else
						set.addToSelection(sound.get());
				}
			}
		}
		catch (std::regex_error e)
		{
			debugError(sampler, e.what());
		}
	}

#if USE_BACKEND

	SafeAsyncCall::call<ModulatorSampler>(*sampler, [](ModulatorSampler& s)
	{
		s.getSampleEditHandler()->setMainSelectionToLast();
	});

#endif
}

void ModulatorSamplerSound::updateInternalData(const Identifier& id, const var& newValueVar)
{
	int newValue = (int)newValueVar;

	if (!isAsyncProperty(id))
	{
		if (id == SampleIds::Root)
		{
			rootNote = newValue;
		}
		else if (id == SampleIds::HiVel)
		{
			int low = jmin(velocityRange.findNextSetBit(0), newValue, 127);
			velocityRange.clear();
			velocityRange.setRange(low, newValue - low + 1, true);
		}
		else if (id == SampleIds::LoVel)
		{
			int high = jmax(velocityRange.getHighestBit(), newValue, 0);
			velocityRange.clear();
			velocityRange.setRange(newValue, high - newValue + 1, true);
		}
		else if (id == SampleIds::HiKey)
		{
			int low = jmin(midiNotes.findNextSetBit(0), newValue, 127);
			midiNotes.clear();
			midiNotes.setRange(low, newValue - low + 1, true);
		}
		else if (id == SampleIds::LoKey)
		{
			int high = jmax(midiNotes.getHighestBit(), newValue, 0);
			midiNotes.clear();
			midiNotes.setRange(newValue, high - newValue + 1, true);
		}
        else if (id == SampleIds::NumQuarters)
        {
            numQuartersForTimestretch = jlimit<double>(0.0, 128.0, (double)newValue);
        }
		else if (id == SampleIds::Normalized)
		{
			isNormalized = newValue != 0;

			if (isNormalized)
			{
				if (data.hasProperty(SampleIds::NormalizedPeak))
				{
					normalizedPeak = (float)data.getProperty(SampleIds::NormalizedPeak);
					FloatSanitizers::sanitizeFloatNumber(normalizedPeak);
				}
				else
				{
					calculateNormalizedPeak();
				}
			}
			else
			{
				//data.removeProperty(SampleIds::NormalizedPeak, nullptr);
				normalizedPeak = 1.0f;
			}
		}
		else if (id == SampleIds::RRGroup)
		{
			rrGroup = jmin<int>(maxRRGroup, newValue);
		}
		else if (id == SampleIds::Volume)
		{
			gain = Decibels::decibelsToGain((float)newValue);;
		}
		else if (id == SampleIds::Pan)
		{
			leftBalanceGain = BalanceCalculator::getGainFactorForBalance((float)newValue, true);
			rightBalanceGain = BalanceCalculator::getGainFactorForBalance((float)newValue, false);
		}
		else if (id == SampleIds::Pitch)
		{
			pitchFactor.store(powf(2.0f, (float)newValue / 1200.f));
		}
		else if (id == SampleIds::LowerVelocityXFade)
		{
			lowerVeloXFadeValue = newValue;
		}
		else if (id == SampleIds::UpperVelocityXFade)
		{
			upperVeloXFadeValue = newValue;
		}
		else if (id == SampleIds::GainTable || id == SampleIds::PitchTable || id == SampleIds::LowPassTable)
		{
			auto type = SampleIds::Helpers::getEnvelopeType(id);
			auto& t = envelopes[type];
			auto b64 = newValueVar.toString();

			if (b64.isEmpty())
				t = nullptr;
			else if (t == nullptr)
			{
				t = new EnvelopeTable(*this, type, b64);

				if (id == SampleIds::LowPassTable)
					parentMap->getSampler()->setEnableEnvelopeFilter();
			}
		}

		loadEntireSampleIfMaxPitch();
	}
	else
	{
		WeakPtr refPtr = this;
		
		auto f = [refPtr, id, newValue](Processor* )
		{
			auto s = refPtr.get();

			if(s != nullptr)
				s->updateAsyncInternalData(id, newValue);

			return SafeFunctionCall::OK;
		};

		if (enableAsyncPropertyChange)
		{
			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}
		else
		{
			f(getMainController()->getMainSynthChain());
		}
	}
}


void ModulatorSamplerSound::updateAsyncInternalData(const Identifier& id, int newValue)
{
	LockHelpers::freeToGo(getMainController());

	jassert_sample_loading_thread(getMainController());

	if (id == SampleIds::SampleStart)
	{
		FOR_EVERY_SOUND(setSampleStart(newValue));
	}
	else if (id == SampleIds::SampleEnd)
	{
		FOR_EVERY_SOUND(setSampleEnd(newValue));
	}
	else if (id == SampleIds::SampleStartMod)
	{
		FOR_EVERY_SOUND(setSampleStartModulation(newValue));
	}
	else if (id == SampleIds::LoopEnabled)
	{
		FOR_EVERY_SOUND(setLoopEnabled(newValue == 1));
	}
	else if (id == SampleIds::LoopStart)
	{
		FOR_EVERY_SOUND(setLoopStart(newValue));
	}
	else if (id == SampleIds::LoopEnd)
	{
		FOR_EVERY_SOUND(setLoopEnd(newValue));
	}
	else if (id == SampleIds::LoopXFade)
	{
		FOR_EVERY_SOUND(setLoopCrossfade(newValue));
	}
	else if (id == SampleIds::SampleState)
	{
		setPurged(newValue == 1);
	}
	else if (id == SampleIds::Reversed)
	{
		setReversed(newValue == 1);
	}
}

var ModulatorSamplerSound::getDefaultValue(const Identifier& id) const
{
	if (id == SampleIds::HiVel) return var(127);
	else if (id == SampleIds::HiKey) return var(127);
	else if (id == SampleIds::Normalized) return var(false);
	else if (id == SampleIds::SampleEnd) return firstSound != nullptr ? firstSound->getSampleEnd() : 0;
	else if (id == SampleIds::LoopEnabled) return var(false);
	else if (id == SampleIds::LoopEnd) return firstSound != nullptr ? firstSound->getSampleEnd() : 0;
	else if (id == SampleIds::SampleState) return var(false);
    else if (id == SampleIds::NumQuarters) return var(0);
	
	return var(0);
}

void ModulatorSamplerSound::setSampleProperty(const Identifier& id, const var& newValue, bool useUndo)
{
	if (id == SampleIds::GainTable || id == SampleIds::PitchTable || id == SampleIds::LowPassTable)
	{
		data.setProperty(id, newValue.toString(), useUndo ? undoManager : nullptr);
		return;
	}

	clipRangeProperties(id, newValue, useUndo);

	jassert(!newValue.isString());
	auto v = getPropertyRange(id).clipValue((int)newValue);

	data.setProperty(id, v, useUndo ? undoManager : nullptr);
}

var ModulatorSamplerSound::getSampleProperty(const Identifier& id) const
{
	if (id == SampleIds::ID)
		return getId();

	if (id == SampleIds::FileName && data.getNumChildren() != 0)
		return data.getChild(0)[id];

	var rv = data.getProperty(id, getDefaultValue(id));

	if (SampleIds::Helpers::isMapProperty(id))
	{
		return jlimit(0, 127, (int)rv);
	}
	else
		return rv;
}

void ModulatorSamplerSound::addEnvelopeProcessor(HiseAudioThumbnail& th)
{
	th.getAudioDataProcessor().removeAllListeners();

	if (auto e = getEnvelope(Modulation::Mode::GainMode))
	{
		e->thumbnailToPreview = &th;
		th.getAudioDataProcessor().addListener(*e, EnvelopeTable::processThumbnail, false);
	}

	if (auto f = getEnvelope(Modulation::Mode::PanMode))
	{
		f->thumbnailToPreview = &th;
		th.getAudioDataProcessor().addListener(*f, EnvelopeTable::processThumbnail, false);
	}

	th.setReader(createAudioReader(0));
}

juce::AudioFormatReader* ModulatorSamplerSound::createAudioReader(int micIndex)
{
	micIndex = jlimit(0, getNumMultiMicSamples() - 1, micIndex);

	StreamingSamplerSound::Ptr sound = getReferenceToSound(micIndex);

	if (sound == nullptr)
	{
		jassertfalse;
		return nullptr;
	}

	ScopedPointer<AudioFormatReader> afr;

	if (sound->isMonolithic())
	{
		return sound->createReaderForPreview();
	}
	else
	{
		return PresetHandler::getReaderForFile(sound->getFileName(true));
	}
}

// ====================================================================================================================



// ====================================================================================================================

ModulatorSamplerSoundPool::ModulatorSamplerSoundPool(MainController *mc_, FileHandlerBase* handler) :
PoolBase(mc_, handler),
mc(mc_),
debugProcessor(nullptr),
mainAudioProcessor(nullptr),
updatePool(true),
searchPool(true),
forcePoolSearch(false),
isCurrentlyLoading(false),
asyncCleaner(*this)
{
	
}

void ModulatorSamplerSoundPool::setDebugProcessor(Processor *p)
{
	debugProcessor = p;
}



void ModulatorSamplerSoundPool::addSound(const PoolEntry& newPoolEntry)
{
	jassert(getSampleFromPool(newPoolEntry.r) == nullptr);

	pool.add(newPoolEntry);
}

void ModulatorSamplerSoundPool::removeFromPool(const PoolReference& ref)
{
	for (int i = 0; i < pool.size(); i++)
	{
		if (pool[i].r == ref)
		{
			pool.remove(i);
			return;
		}
	}
}

hise::HlacMonolithInfo* ModulatorSamplerSoundPool::getMonolith(const Identifier& id)
{
	for (const auto& m : loadedMonoliths)
	{
		if (*m == id)
			return m;
	}

	return nullptr;
}

HlacMonolithInfo::Ptr ModulatorSamplerSoundPool::loadMonolithicData(const ValueTree &sampleMap, const Array<File>& monolithicFiles)
{
	jassert(!mc->getMainSynthChain()->areVoicesActive());

	clearUnreferencedMonoliths();
	
	loadedMonoliths.add(new HlacMonolithInfo(monolithicFiles));

	auto hmaf = loadedMonoliths.getLast();

	try
	{
		hmaf->fillMetadataInfo(sampleMap);
		sendChangeMessage();
		return hmaf;
	}
	catch (StreamingSamplerSound::LoadingError l)
	{
		String x;
		x << "Error at loading sample " << l.fileName << ": " << l.errorDescription;
		mc->getDebugLogger().logMessage(x);

#if USE_FRONTEND
		mc->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
		debugError(mc->getMainSynthChain(), x);
#endif

		return nullptr;
	}
}


void ModulatorSamplerSoundPool::setAllowDuplicateSamples(bool shouldAllowDuplicateSamples)
{
	if (allowDuplicateSamples == shouldAllowDuplicateSamples)
		return;

	allowDuplicateSamples = shouldAllowDuplicateSamples;

	Processor::Iterator<ModulatorSampler> it(mc->getMainSynthChain());

	auto& expHandler = mc->getExpansionHandler();

	while (auto sampler = it.getNextProcessor())
	{
		auto ref = sampler->getSampleMap()->getReference();
		auto refExpansion = expHandler.getExpansionForWildcardReference(ref.getReferenceString());

		bool isSameExpansion = refExpansion == getFileHandler();
		bool isRootPoolAndNoExpansionLoaded = dynamic_cast<Expansion*>(getFileHandler()) == nullptr && refExpansion == nullptr;

		if (isSameExpansion || isRootPoolAndNoExpansionLoaded)
			sampler->reloadSampleMap();
	}
}

void ModulatorSamplerSoundPool::clearUnreferencedSamples()
{
	asyncCleaner.triggerAsyncUpdate();
}

StreamingSamplerSound* ModulatorSamplerSoundPool::getSampleFromPool(PoolReference r) const
{
	if (!allowDuplicateSamples || !searchPool)
		return nullptr;

	for (const auto& entry : pool)
	{
		if (r == entry.r)
			return entry.get();
	}

	return nullptr;
}


hise::ModulatorSamplerSoundPool* MainController::SampleManager::getModulatorSamplerSoundPool()
{
	if (auto exp = mc->getExpansionHandler().getCurrentExpansion())
		return exp->pool->getSamplePool();

	return mc->getCurrentFileHandler().pool->getSamplePool();
}

void ModulatorSamplerSoundPool::clearUnreferencedSamplesInternal()
{
	Array<PoolEntry> currentList;
	currentList.ensureStorageAllocated(pool.size());

	for (int i = 0; i < pool.size(); i++)
	{
		if (pool[i].get() != nullptr)
		{
			currentList.add(pool[i]);
		}
	}

	pool.swapWith(currentList);
	if (updatePool) sendChangeMessage();
}



int ModulatorSamplerSoundPool::getNumSoundsInPool() const noexcept
{
	return pool.size();
}

void ModulatorSamplerSoundPool::getMissingSamples(StreamingSamplerSoundArray &missingSounds) const
{
	for (auto s: pool)
	{
		if (s.get() != nullptr && s.get()->isMissing())
		{
			missingSounds.add(s.get());
		}
	}
}


class SampleResolver : public DialogWindowWithBackgroundThread
{
public:

    class HorizontalSpacer: public Component
    {
    public:
        
        HorizontalSpacer()
        {
            setSize(900, 2);
        }
    };
    
	SampleResolver(ModulatorSamplerSoundPool *pool_, Processor *synthChain_):
		DialogWindowWithBackgroundThread("Sample Resolver"),
		pool(pool_),
		mainSynthChain(synthChain_)
	{
		pool->getMissingSamples(missingSounds);

		if (missingSounds.size() == 0)
		{	
			addBasicComponents(false);
		}
		else
		{
			numMissingSounds = missingSounds.size();

			remainingSounds = numMissingSounds;

			String textToShow = "Remaining missing sounds: " + String(remainingSounds) + " / " + String(numMissingSounds) + " missing sounds.";

            addCustomComponent(spacer = new HorizontalSpacer());

			String fileNames = missingSounds[0]->getFileName(true);
            
            String path;

            if(ProjectHandler::isAbsolutePathCrossPlatform(fileNames))
            {
                path = File(fileNames).getParentDirectory().getFullPathName();

            }
            else path = fileNames;
            
			addTextEditor("fileNames", fileNames, "Filenames:");
           
            addTextEditor("search", path, "Search for:");
			addTextEditor("replace", path, "Replace with:");
            
            addButton("Search in Finder", 5);
            
            addBasicComponents(true);
            
            
            
            showStatusMessage(textToShow);
		}
        

	};


	void run() override
	{
		const String search = getTextEditorContents("search");
		const String replace = getTextEditorContents("replace");

		pool->setUpdatePool(false);

		int foundThisTime = 0;

		showStatusMessage("Replacing references");

		try
		{
            const double numMissingSoundsDouble = (double)missingSounds.size();
            
			for (int i = 0; i < missingSounds.size(); i++)
			{
				if (threadShouldExit()) return;

                setProgress(double(i) / numMissingSoundsDouble);

				auto sound = missingSounds[i];

				String newFileName = sound->getFileName(true).replace(search, replace, true);

                String newFileNameSanitized = newFileName.replace("\\", "/");
                
				if (File(newFileNameSanitized).existsAsFile())
				{
					sound->replaceFileReference(newFileNameSanitized);

					foundThisTime++;
					missingSounds.remove(i--);
				}
			}
		}
		catch (StreamingSamplerSound::LoadingError e)
		{
			String x;

			x << "Error at loading sample " << e.fileName << ": " << e.errorDescription;

			mainSynthChain->getMainController()->getDebugLogger().logMessage(x);

			errorMessage = "There was an error at preloading.";
			return;
		}

		remainingSounds -= foundThisTime;
		
		showStatusMessage("Replacing references");

		Processor::Iterator<ModulatorSampler> iter(mainSynthChain);

		int numSamplers = iter.getNumProcessors();
		int index = 0;

		while (ModulatorSampler *s = iter.getNextProcessor())
		{
			setProgress((double)index / (double)numSamplers);

			ModulatorSampler::SoundIterator sIter(s);

			while (auto sound = sIter.getNextSound())
			{
				sound->checkFileReference();
			}

			s->sendChangeMessage();

			index++;
		}
	}

	void threadFinished()
	{
		if (errorMessage.isEmpty())
		{
			PresetHandler::showMessageWindow("Missing Samples resolved", String(numMissingSounds - remainingSounds) + " out of " + String(numMissingSounds) + " were resolved.", PresetHandler::IconType::Info);
		}
		else
		{
			PresetHandler::showMessageWindow("Error", errorMessage, PresetHandler::IconType::Error);
		}

		pool->setUpdatePool(true);
		pool->sendChangeMessage();
	}
    
    void resultButtonClicked(const String &name) override
    {
        if(name == "Search in Finder")
        {
            String file = getTextEditor("fileNames")->getText();
            
            file.replace("\\", "/");
            
            String fileName = file.fromLastOccurrenceOf("/", false, false);
            String pathName = file.upToLastOccurrenceOf("/", true, false);
            
#if JUCE_WINDOWS
            String dialogName = "Explorer";
#else
            String dialogName = "Finder";
#endif
            
            PresetHandler::showMessageWindow("Search file", "Search for the sample:\n\n"+fileName +"\n\nPress OK to open the " + dialogName, PresetHandler::IconType::Info);
            
            FileChooser fc("Search sample location " + fileName);
            
            if(fc.browseForFileToOpen())
            {
                File f = fc.getResult();

				
                
                String newPath = f.getFullPathName().replaceCharacter('\\', '/').upToLastOccurrenceOf("/", true, false);;
                
                getTextEditor("search")->setText(pathName);
                getTextEditor("replace")->setText(newPath);
            }
        }
    };

private:

	StreamingSamplerSoundArray missingSounds;

    ScopedPointer<HorizontalSpacer> spacer;
    
	int remainingSounds;
	int numMissingSounds;

	String errorMessage;

	ModulatorSamplerSoundPool *pool;
	WeakReference<Processor> mainSynthChain;


};


void ModulatorSamplerSoundPool::resolveMissingSamples(Component *childComponentOfMainEditor)
{
#if USE_BACKEND
	auto editor = dynamic_cast<BackendRootWindow*>(childComponentOfMainEditor);
	
	if (editor == nullptr) editor = GET_BACKEND_ROOT_WINDOW(childComponentOfMainEditor);

	SampleResolver *r = new SampleResolver(this, editor->getMainSynthChain());
    
	r->setModalBaseWindowComponent(childComponentOfMainEditor);

#else 

	ignoreUnused(childComponentOfMainEditor);

#endif
}

StringArray ModulatorSamplerSoundPool::getFileNameList() const
{
	StringArray sa;

	for (int i = 0; i < pool.size(); i++)
	{
		sa.add(pool[i].r.getReferenceString());
	}

	return sa;
}

size_t ModulatorSamplerSoundPool::getMemoryUsageForAllSamples() const noexcept
{
	if (mc->getSampleManager().isPreloading()) return 0;

	size_t memoryUsage = 0;

	for (auto s : pool)
	{
		if (s.get() == nullptr)
			continue;

		memoryUsage += s.get()->getActualPreloadSize();
	}

	return memoryUsage;
}



String ModulatorSamplerSoundPool::getTextForPoolTable(int columnId, int indexInPool)
{
#if USE_BACKEND

	const auto& s = pool[indexInPool];

	if (s.r.isValid() && s.get() != nullptr)
	{
		switch (columnId)
		{
		case SamplePoolTable::FileName:	return s.r.getReferenceString();
		case SamplePoolTable::Memory:	return String((int)(s.get()->getActualPreloadSize() / 1024)) + " kB";
		case SamplePoolTable::State:	return String(s.get()->getSampleStateAsString());
		case SamplePoolTable::References:	return String(s.get()->getReferenceCount());
		default:						jassertfalse; return "";
		}
	}
	else
	{
		return "Invalid Index";

	}
#else
	ignoreUnused(columnId, indexInPool);
	return "";

#endif
}

void ModulatorSamplerSoundPool::increaseNumOpenFileHandles()
{
	StreamingSamplerSoundPool::increaseNumOpenFileHandles();

	if(updatePool) sendChangeMessage();
}

void ModulatorSamplerSoundPool::decreaseNumOpenFileHandles()
{
	StreamingSamplerSoundPool::decreaseNumOpenFileHandles();

	if(updatePool) sendChangeMessage();
}

bool ModulatorSamplerSoundPool::isFileBeingUsed(int poolIndex)
{
	if (auto s = pool[poolIndex].get())
	{
		return s->isOpened();
	}

	return false;
}

int ModulatorSamplerSoundPool::getSoundIndexFromPool(int64 hashCode)
{
	if (!searchPool) return -1;

	for (int i = 0; i < pool.size(); i++)
	{
		StreamingSamplerSound::Ptr s = pool[i].get();
		
		if (s == nullptr)
			return -1;

		if (pool[i].r.getHashCode() == hashCode) return i;
	}

	return -1;
}

bool ModulatorSamplerSoundPool::isPoolSearchForced() const
{
	return forcePoolSearch;
}

void ModulatorSamplerSoundPool::clearUnreferencedMonoliths()
{
	for (int i = 0; i < loadedMonoliths.size(); i++)
	{
		if (loadedMonoliths[i]->getReferenceCount() == 2)
		{
			loadedMonoliths.remove(i--);
		}
	}

	if(updatePool) sendChangeMessage();
}

#define SET_IF_NOT_ZERO(id) if (d.hasProperty(id)) data.setProperty(id, sound->getSampleProperty(id), nullptr);

MappingData::MappingData(int r, int lk, int hk, int lv, int hv, int rr) :
	data("sample")
{
	data.setProperty(SampleIds::Root, r, nullptr);
	data.setProperty(SampleIds::LoKey, lk, nullptr);
	data.setProperty(SampleIds::HiKey, hk, nullptr);
	data.setProperty(SampleIds::LoVel, lv, nullptr);
	data.setProperty(SampleIds::HiVel, hv, nullptr);
	data.setProperty(SampleIds::RRGroup, rr, nullptr);
}

void MappingData::fillOtherProperties(ModulatorSamplerSound* sound)
{
	const auto d = sound->getData();

	SET_IF_NOT_ZERO(SampleIds::Volume);
	SET_IF_NOT_ZERO(SampleIds::Pan);
	SET_IF_NOT_ZERO(SampleIds::Pitch);
	SET_IF_NOT_ZERO(SampleIds::SampleStart);
	SET_IF_NOT_ZERO(SampleIds::SampleEnd);
	SET_IF_NOT_ZERO(SampleIds::SampleStartMod);

	// Skip the rest if the loop isn't enabled.
	if (!sound->getSampleProperty(SampleIds::LoopEnabled))
		return;

	SET_IF_NOT_ZERO(SampleIds::LoopEnabled);
	SET_IF_NOT_ZERO(SampleIds::LoopStart);
	SET_IF_NOT_ZERO(SampleIds::LoopEnd);
	SET_IF_NOT_ZERO(SampleIds::LoopXFade);
}

#undef SET_IF_NOT_ZERO



SamplePreviewer::SamplePreviewer(ModulatorSampler* sampler_) :
	ControlledObject(sampler_->getMainController()),
	sampler(sampler_)
{

}

void SamplePreviewer::applySampleProperty(AudioSampleBuffer& b, ModulatorSamplerSound::Ptr sound, const Identifier& id, int offset)
{
	auto value = (int)sound->getSampleProperty(id);

	if (id == SampleIds::Normalized)
	{
		if (sound->isNormalizedEnabled())
		{
			auto gain = sound->getNormalizedPeak();
			b.applyGain(gain);
		}
	}

	if (id == SampleIds::Volume)
	{
		b.applyGain(Decibels::decibelsToGain((float)value));
	}

	if (id == SampleIds::Pan)
	{
		if (b.getNumChannels() == 2)
		{
			auto lGain = BalanceCalculator::getGainFactorForBalance((float)value, true);
			FloatVectorOperations::multiply(b.getWritePointer(0), lGain, b.getNumSamples());

			auto rGain = BalanceCalculator::getGainFactorForBalance((float)value, false);
			FloatVectorOperations::multiply(b.getWritePointer(1), rGain, b.getNumSamples());
		}
	}

	if (id == SampleIds::LoopEnabled)
	{
		if (!value)
			return;

 		auto numMinSamples = roundToInt(sound->getMainController()->getMainSynthChain()->getSampleRate() * 3.0);

		auto numSamplesToUse = jmax(b.getNumSamples(), numMinSamples);

		if (numSamplesToUse != b.getNumSamples())
		{
			AudioSampleBuffer lb(2, numSamplesToUse);

			int numBeforeLoop = (int)sound->getSampleProperty(SampleIds::LoopEnd) - offset;

			lb.copyFrom(0, 0, b, 0, 0, numBeforeLoop);
			lb.copyFrom(1, 0, b, 1, 0, numBeforeLoop);

			int numLeft = numSamplesToUse - numBeforeLoop;
			auto loopStart = (int)sound->getSampleProperty(SampleIds::LoopStart) - offset;
			int loopLength = numBeforeLoop - loopStart;
			int dstPos = numBeforeLoop;

			while (numLeft > 0)
			{
				auto numThisTime = jmin(numLeft, loopLength);

				lb.copyFrom(0, dstPos, b, 0, loopStart, numThisTime);
				lb.copyFrom(1, dstPos, b, 1, loopStart, numThisTime);

				numLeft -= numThisTime;
				dstPos += numThisTime;
			}

			std::swap(b, lb);
		}

	}
	if (id == SampleIds::PitchTable)
	{
		if (auto env = sound->getEnvelope(Modulation::Mode::PitchMode))
		{
			env->processBuffer(b, offset, 0);
		}
	}
	if (id == SampleIds::LowPassTable)
	{
		if (auto env = sound->getEnvelope(Modulation::Mode::PanMode))
		{
			env->processBuffer(b, offset, 0);
		}
	}
	if (id == SampleIds::GainTable)
	{
		if (auto env = sound->getEnvelope(Modulation::Mode::GainMode))
		{
			env->processBuffer(b, offset, 0);
		}
	}
}

void SamplePreviewer::previewSample(ModulatorSamplerSound::Ptr soundToPlay, int micIndex)
{
	if (previewOffset == -1)
	{
		previewSampleWithMidi(soundToPlay);
	}
	else
	{
		previewSampleFromDisk(soundToPlay, micIndex);
	}
}


bool SamplePreviewer::isPlaying() const
{
	if (previewOffset == -1)
		return !previewNote.isEmpty();
	else
		return sampler->getMainController()->getPreviewBufferPosition() > 0;
}

void SamplePreviewer::previewSampleFromDisk(ModulatorSamplerSound::Ptr soundToPlay, int micIndex)
{
	if (soundToPlay == nullptr || currentlyPlayedSound == soundToPlay)
	{
		sampler->getMainController()->stopBufferToPlay();
		currentlyPlayedSound = nullptr;
		return;
	}

	currentlyPlayedSound = soundToPlay;

	micIndex = jlimit<int>(0, soundToPlay->getNumMultiMicSamples() - 1, micIndex);

	auto offset = previewOffset;

	auto f = [micIndex, offset, soundToPlay](Processor* p)
	{
		auto sampler = dynamic_cast<ModulatorSampler*>(p);

		ScopedPointer<AudioFormatReader> afr = soundToPlay->createAudioReader(micIndex);

		int numPreview = 0;
		
		if (afr != nullptr)
			numPreview = (int)afr->lengthInSamples - offset;
		else
			return SafeFunctionCall::OK;

		AudioSampleBuffer copy;

		copy.setSize(2, numPreview);

		afr->read(&copy, 0, numPreview, offset, true, true);

		
		auto fileSampleRate = soundToPlay->getSampleRate();
		auto pitchValue = conversion_logic::st2pitch().getValue((double)soundToPlay->getSampleProperty(SampleIds::Pitch) * 0.01);

		fileSampleRate *= pitchValue;

#if 0
		auto pitchFactor = soundToPlay->getSampleRate() / sampler->getSampleRate();

		pitchFactor *= pitchValue;

		if (pitchFactor != 1.0f)
		{
			auto newNum = roundToInt(numPreview / pitchFactor);

			AudioSampleBuffer resampled(copy.getNumChannels(), newNum);
			resampled.clear();
			LagrangeInterpolator interpolator;

			for (int i = 0; i < copy.getNumChannels(); i++)
			{
				interpolator.process(pitchFactor, copy.getReadPointer(i), resampled.getWritePointer(i), newNum);
			}

			numPreview = newNum;
			std::swap(resampled, copy);
		}
#endif

		auto o = offset - (int)soundToPlay->getSampleProperty(SampleIds::SampleStart);

		applySampleProperty(copy, soundToPlay, SampleIds::PitchTable, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::Pan, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::Normalized, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::Volume, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::GainTable, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::LowPassTable, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::LoopXFade, offset);
		applySampleProperty(copy, soundToPlay, SampleIds::LoopEnabled, offset);
		
		auto sampleLength = soundToPlay->getReferenceToSound(0)->getSampleLength();

		sampler->getMainController()->setBufferToPlay(copy, fileSampleRate, [o, sampler, sampleLength](int pos)
		{
			sampler->getSamplerDisplayValues().currentSamplePos = (double)(pos + o) / (double)sampleLength;
		});

		return SafeFunctionCall::OK;
	};

	sampler->killAllVoicesAndCall(f);
}

void SamplePreviewer::previewSampleWithMidi(ModulatorSamplerSound::Ptr soundToPlay)
{
	if (!previewNote.isEmpty())
	{
		HiseEvent m(HiseEvent::Type::NoteOff, previewNote.getNoteNumber(), 0, 1);

		m.setEventId(previewNote.getEventId());
		m.setArtificial();
		previewNote = {};

		LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock);
		sampler->preHiseEventCallback(m);
		sampler->noteOff(m);
	}

	if (currentlyPlayedSound != soundToPlay && soundToPlay != nullptr)
	{
		currentlyPlayedSound = soundToPlay;

		auto note = (int)soundToPlay->getSampleProperty(SampleIds::Root);
		auto vel = (int)soundToPlay->getSampleProperty(SampleIds::HiVel) - 1;
		auto group = (int)soundToPlay->getSampleProperty(SampleIds::RRGroup);

		bool useRR = sampler->shouldUseRoundRobinLogic();

		sampler->setUseRoundRobinLogic(false);

		previewNote = HiseEvent(HiseEvent::Type::NoteOn, note, vel, 1);
		previewNote.setArtificial();
		getMainController()->getEventHandler().pushArtificialNoteOn(previewNote);

		LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock);
		sampler->preHiseEventCallback(previewNote);
		sampler->noteOn(previewNote);
		sampler->setCurrentGroupIndex(group);
		sampler->setUseRoundRobinLogic(useRR);
	}
	else
	{
		currentlyPlayedSound = nullptr;
	}
}

void ModulatorSamplerSound::EnvelopeTable::timerCallback()
{
	rebuildBuffer();
	stopTimer();
}

ModulatorSamplerSound::EnvelopeTable::EnvelopeTable(ModulatorSamplerSound& parent_, Type type_, const String& b64) :
	parent(parent_),
	type(type_)
{
	table.setUndoManager(parent.undoManager);
	table.getUpdater().addEventListener(this);
	table.setGlobalUIUpdater(parent.getMainController()->getGlobalUIUpdater());
	table.restoreData(b64);
	
	
	switch (type)
	{
	case Type::GainMode:
		table.setYTextConverterRaw(getGainString);
		table.setStartAndEndY(0.5f, 0.5f);
		break;
	case Type::PitchMode:
		table.setYTextConverterRaw(getPitchString);
		table.setStartAndEndY(0.5f, 0.5f);
		break;
	case Type::PanMode:
		table.setYTextConverterRaw(getFreqencyString);
		table.setStartAndEndY(1.0f, 1.0f);
		break;
    default: jassertfalse; break;
	}

	stopTimer();
	rebuildBuffer();
}

ModulatorSamplerSound::EnvelopeTable::~EnvelopeTable()
{
	stopTimer();
	SimpleReadWriteLock::ScopedWriteLock sl(lock);
	table.getUpdater().removeEventListener(this);
}

void ModulatorSamplerSound::EnvelopeTable::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if (t != ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		Identifier propId;

		if (type == Type::GainMode) propId = SampleIds::GainTable;
		if (type == Type::PitchMode) propId = SampleIds::PitchTable;
		if (type == Type::PanMode) propId = SampleIds::LowPassTable;

		parent.setSampleProperty(propId, table.exportData());
		startTimer(JUCE_LIVE_CONSTANT_OFF(200));
	}
}

void ModulatorSamplerSound::EnvelopeTable::rebuildBuffer()
{
	numElements = (int)parent.getReferenceToSound(0)->getLengthInSamples() / DownsamplingFactor + 1;
	sampleRange.setStart((int)parent.getSampleProperty(SampleIds::SampleStart));
	sampleRange.setEnd((int)parent.getSampleProperty(SampleIds::SampleEnd));

	if (numElements > 0)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);

			lookupTable.realloc(numElements);
			table.fillExternalLookupTable(lookupTable, numElements - 1);
			lookupTable[numElements - 1] = lookupTable[numElements - 2];

			for (int i = 0; i < numElements; i++)
			{
				auto& v = lookupTable[i];

				switch (type)
				{
				case Type::GainMode:
					v = getGainValue(v);
					break;
				case Type::PitchMode:
					v = getPitchValue(v);
					break;
				case Type::PanMode:
					v = getFreqValue(v);
					break;
                    default: break;
				}
			}
		}
		
		if(thumbnailToPreview != nullptr)
			thumbnailToPreview->setReader(parent.createAudioReader(0));
	}
}

void ModulatorSamplerSound::EnvelopeTable::processThumbnail(EnvelopeTable& t, var left, var right)
{
	if (t.sampleRange.getLength() == 0)
		return;

	float* data[2] = { nullptr, nullptr };
	data[0] = left.isBuffer() ? left.getBuffer()->buffer.getWritePointer(0, 0) : nullptr;
	data[1] = right.isBuffer() ? right.getBuffer()->buffer.getWritePointer(0, 0) : nullptr;

	if (data[0] == nullptr)
		return;

	AudioSampleBuffer b(data, data[1] != nullptr ? 2 : 1, left.getBuffer()->size);

	t.processBuffer(b, 0, 0);
}

void ModulatorSamplerSound::EnvelopeTable::processBuffer(AudioSampleBuffer& b, int srcOffset, int dstOffset)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	auto lutOffset = srcOffset / DownsamplingFactor;

	if (type == Type::GainMode)
	{
		for (int i = 0; i < numElements - 1; i++)
		{
			auto i0 = jlimit(0, numElements - 1, i + lutOffset);
			auto i1 = jlimit(0, numElements - 1, i + lutOffset + 1);
			auto v0 = lookupTable[i0];
			auto v1 = lookupTable[i1];

			auto o0 = i * DownsamplingFactor + dstOffset;

			if (isPositiveAndBelow(o0 + DownsamplingFactor, b.getNumSamples()))
				b.applyGainRamp(o0, DownsamplingFactor, v0, v1);
			else
				break;
		}
	}
	else if (type == Type::PitchMode)
	{
		double uptime = 0;

		Array<double> deltas;
		deltas.ensureStorageAllocated(b.getNumSamples());

		for (int i = 0; i < b.getNumSamples(); i++)
		{
			auto i0 = jlimit(0, numElements - 1, (i / DownsamplingFactor) + lutOffset);
			auto i1 = jlimit(0, numElements - 1, (i / DownsamplingFactor) + lutOffset + 1);

			auto indexDouble = (double)i / (double)DownsamplingFactor + (double)lutOffset;
			auto alpha = jlimit(0.0, 1.0, indexDouble - (double)i0);

			auto v0 = (double)lookupTable[i0];
			auto v1 = (double)lookupTable[i1];

			auto v = Interpolator::interpolateLinear(v0, v1, alpha);
			deltas.add(v);
			uptime += 1.0 / v;
		}

		auto numSamples = roundToInt(uptime);

		AudioSampleBuffer copy(b.getNumChannels(), numSamples);

		uptime = 0.0;

		auto oldNumSamples = b.getNumSamples();

		for (int i = 0; i < numSamples; i++)
		{
			

			auto i0 = jlimit(0, oldNumSamples -1, ((int)uptime));
			auto i1 = jlimit(0, oldNumSamples - 1, i0 + 1);
			auto alpha = jlimit(0.0f, 1.0f, (float)(uptime - (float)i0));

			auto l = Interpolator::interpolateLinear(b.getSample(0, i0), b.getSample(0, i1), alpha);
			auto r = Interpolator::interpolateLinear(b.getSample(1, i0), b.getSample(1, i1), alpha);

			copy.setSample(0, i, l);
			copy.setSample(1, i, r);

			uptime += deltas[i];
		}

		std::swap(copy, b);
	}
	else
	{
		using namespace scriptnode;

		CascadedEnvelopeLowPass filter(true);

		PrepareSpecs ps;
		ps.sampleRate = parent.getMainController()->getMainSynthChain()->getSampleRate();
		ps.blockSize = DownsamplingFactor;
		ps.numChannels = b.getNumChannels();

		filter.prepare(ps);
		filter.setOrder(JUCE_LIVE_CONSTANT_OFF(1));
		
		int offset = 0;

		snex::PolyHandler::ScopedVoiceSetter svs(filter.polyManager, 0);

		for (int i = 0; i < b.getNumSamples(); i += DownsamplingFactor)
		{
			auto numThisTime = jmin(DownsamplingFactor, b.getNumSamples() - i);
			auto f = lookupTable[i / DownsamplingFactor];

			filter.process(f, b, offset, numThisTime);
			offset += numThisTime;
		}
	}
}

float ModulatorSamplerSound::EnvelopeTable::getFreqValue(float input)
{
	input = 1.0f - input;
	auto v = 1.0f - std::exp(std::log(input) * 0.2f);
	return v * 20000.0f;
}

float ModulatorSamplerSound::EnvelopeTable::getFreqValueInverse(float input)
{
	input /= 20000.0f;
	auto v = 1.0f - input;
	auto v0 = std::log(v);
	auto v1 = v0 / 0.2f;
	auto v2 = std::exp(v1);
	input = 1.0f - v2;
	return input;
}

float ModulatorSamplerSound::EnvelopeTable::getPitchValue(float input)
{
	input = getSemitones(input);
	return scriptnode::conversion_logic::st2pitch().getValue(input);
}

CascadedEnvelopeLowPass::CascadedEnvelopeLowPass(bool isPoly) :
	polyManager(isPoly)
{
	for (int i = 0; i < NumMaxOrders; i++)
		filters.add(new FilterType());

	for (auto f : filters)
	{
		f->setSmoothing(0.0);
		f->setFrequency(20000.0);
	}
}

void CascadedEnvelopeLowPass::process(float frequency, AudioSampleBuffer& b, int startSampleInBuffer, int maxNumSamples)
{
	int numChannels = b.getNumChannels();
	int numSamples = b.getNumSamples() - startSampleInBuffer;

	if (maxNumSamples != 0)
		numSamples = jmin(numSamples, maxNumSamples);

	auto data = (float**)alloca(numChannels * sizeof(float*));
	memcpy(data, b.getArrayOfReadPointers(), numChannels * sizeof(float**));

	for (int i = 0; i < numChannels; i++)
		data[i] += startSampleInBuffer;

	snex::Types::ProcessDataDyn pd(data, numSamples, numChannels);

	for (auto f : *this)
	{
		f->setFrequency(frequency);
		f->process(pd);
	}
}

} // namespace hise
