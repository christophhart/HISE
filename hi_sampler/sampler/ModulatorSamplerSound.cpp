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

			if (sampleData.hasProperty("MonolithSplitIndex"))
				multimicIndex = (int)sampleData.getProperty("MonolithSplitIndex");

			soundArray.add(new StreamingSamplerSound(hmaf, multimicIndex, getId()));
		}
		else
		{
			soundArray.add(new StreamingSamplerSound(ref.getFile().getFullPathName(), pool));
		}

		pool->addSound({ ref, soundArray.getLast().get() });
	}
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

	firstSound = soundArray.getFirst();

    for(auto s: soundArray)
        s->setDelayPreloadInitialisation(true);
    
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
	else if( id == SampleIds::SampleStart)	return firstSound->isLoopEnabled() ? Range<int>(0, jmin<int>((int)firstSound->getLoopStart() - (int)firstSound->getLoopCrossfade(), (int)(firstSound->getSampleEnd() - (int)firstSound->getSampleStartModulation()))) :
																				 Range<int>(0, (int)firstSound->getSampleEnd() - (int)firstSound->getSampleStartModulation());
	else if( id == SampleIds::SampleEnd)		{
		const int sampleStartMinimum = (int)(firstSound->getSampleStart() + firstSound->getSampleStartModulation());
		const int upperLimit = (int)firstSound->getLengthInSamples();

		if (firstSound->isLoopEnabled())
		{
			const int lowerLimit = jmax<int>(sampleStartMinimum, (int)firstSound->getLoopEnd());
			return Range<int>(lowerLimit, upperLimit);

		}
		else
		{
			const int lowerLimit = sampleStartMinimum;
			return Range<int>(lowerLimit, upperLimit);

		}
	}
	else if( id == SampleIds::SampleStartMod)		return Range<int>(0, (int)firstSound->getSampleLength());
	else if( id == SampleIds::LoopEnabled)			return Range<int>(0, 1);
	else if( id == SampleIds::LoopStart)			return Range<int>((int)firstSound->getSampleStart() + (int)firstSound->getLoopCrossfade(), (int)firstSound->getLoopEnd() - (int)firstSound->getLoopCrossfade());
	else if( id == SampleIds::LoopEnd)				return Range<int>((int)firstSound->getLoopStart() + (int)firstSound->getLoopCrossfade(), (int)firstSound->getSampleEnd());
	else if( id == SampleIds::LoopXFade)			return Range<int>(0, jmin<int>((int)(firstSound->getLoopStart() - firstSound->getSampleStart()), (int)firstSound->getLoopLength()));
	else if( id == SampleIds::UpperVelocityXFade)	return Range < int >(0, (int)getSampleProperty(SampleIds::HiVel) - ((int)getSampleProperty(SampleIds::LoVel) + lowerVeloXFadeValue));
	else if( id == SampleIds::LowerVelocityXFade)	return Range < int >(0, (int)getSampleProperty(SampleIds::HiVel) - upperVeloXFadeValue - (int)getSampleProperty(SampleIds::LoVel));
	else if( id == SampleIds::SampleState)			return Range<int>(0, (int)StreamingSamplerSound::numSampleStates - 1);
	
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
				{
					set.deselect(sound.get());
				}
				else
				{
					set.addToSelection(sound.get());
				}
			}
		}
	}
	catch (std::regex_error e)
	{
		debugError(sampler, e.what());
	}
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
	
	return var(0);
}

void ModulatorSamplerSound::setSampleProperty(const Identifier& id, const var& newValue, bool useUndo)
{
	data.setProperty(id, newValue, useUndo ? undoManager : nullptr);
}

var ModulatorSamplerSound::getSampleProperty(const Identifier& id) const
{
	if (id == SampleIds::ID)
		return getId();

	var rv = data.getProperty(id, getDefaultValue(id));

	if (SampleIds::Helpers::isMapProperty(id))
	{
		return jlimit(0, 127, (int)rv);
	}
	else
		return rv;
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
	
	loadedMonoliths.add(new MonolithInfoToUse(monolithicFiles));

	MonolithInfoToUse* hmaf = loadedMonoliths.getLast();

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


} // namespace hise
