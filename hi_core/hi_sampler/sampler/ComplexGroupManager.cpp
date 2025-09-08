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

#include "ComplexGroupManager.h"

#include "ModulatorSampler.h"
#include "hi_core/hi_modules/modulators/mods/GlobalModulators.h"

namespace hise { using namespace juce;

bool SynthSoundWithBitmask::ValueWithFilter::operator==(const ValueWithFilter& other) const
{
	return filterMask == other.filterMask &&
		ignoreMask == other.ignoreMask &&
		value == other.value; 
}

void SynthSoundWithBitmask::ValueWithFilter::dump(const String message)
{
	DBG(message);
	Helpers::dumpBitmask("value ", value);
	Helpers::dumpBitmask("filter", filterMask);
	Helpers::dumpBitmask("ignore", ignoreMask);
}

bool SynthSoundWithBitmask::ValueWithFilter::matches(Bitmask otherValue) const
{
	auto fitsMask = (otherValue & filterMask) == value;

	if(!fitsMask && ignoreMask != 0)
		fitsMask = (otherValue & ignoreMask) != 0;

	return fitsMask;
}

bool SynthSoundWithBitmask::FilterList::matches(Bitmask bm) const
{
	auto fitsMask = mainFilter.matches(bm);

	if(fitsMask && !ignorableFilters.isEmpty())
	{
		for(const auto& vf: ignorableFilters)
		{
			fitsMask &= vf.second.matches(bm);

			if(!fitsMask)
				break;
		}
	}

	return fitsMask;
}

SynthSoundWithBitmask::NoteContainer::Iterator::Iterator(const NoteContainer& c, int noteNumber_):
	noteNumber(noteNumber_),
	container(c),
	sl(c.lock)
{}

const SynthSoundWithBitmask* const* SynthSoundWithBitmask::NoteContainer::Iterator::begin() const
{
	return container.notes[noteNumber].data();
}

const SynthSoundWithBitmask* const* SynthSoundWithBitmask::NoteContainer::Iterator::end() const
{
	return container.notes[noteNumber].data() + container.notes[noteNumber].size();
}

SynthSoundWithBitmask::NoteContainer::Iterator SynthSoundWithBitmask::NoteContainer::createIterator(
	int noteNumber) const
{
	return { *this, noteNumber };
}

void SynthSoundWithBitmask::NoteContainer::rebuild(const ReferenceCountedArray<SynthesiserSound>& soundArray)
{
	ContainerType newNotes;
	BigInteger bi;

	for(const auto& s: soundArray)
	{
		auto typed = dynamic_cast<SynthSoundWithBitmask*>(s);
		jassert(typed != nullptr);

		if(typed->isPurged())
			continue;

		auto rangeToSet = typed->getNoteRange();
		bi.setRange(rangeToSet.getStart(), rangeToSet.getLength(), true);
	}

	int idx = 0;

	for(auto& n: newNotes)
	{
		if(bi[idx++])
			n.reserve(soundArray.size() / 2);
	}

	for(const auto& s: soundArray)
	{
		auto typed = dynamic_cast<SynthSoundWithBitmask*>(s);

		if(typed->isPurged())
			continue;

		jassert(typed != nullptr);
		auto rangeToSet = typed->getNoteRange();

		for(int i = rangeToSet.getStart(); i < rangeToSet.getEnd(); i++)
		{
			newNotes[i].push_back(typed);
		}
	}

	for(auto& n: newNotes)
		n.shrink_to_fit();

	SimpleReadWriteLock::ScopedWriteLock sl(lock);
	std::swap(notes, newNotes);
}

SynthSoundWithBitmask::ValueWithFilter SynthSoundWithBitmask::NoteContainer::getValueWithFilter() const
{
	return filter;
}

void SynthSoundWithBitmask::NoteContainer::setValueFilter(ValueWithFilter newFilter)
{
	filter = newFilter;
}

void SynthSoundWithBitmask::Helpers::dumpBitmask(String msg, Bitmask m, char X)
{
	msg += ": ";
	char b[64 + 8];

	int idx = 0;

	for(int i = 0; i < 64; i++)
	{
		auto on = (((Bitmask)1 << i) & m) != 0;
		b[idx++] = on ? X : '0';

		if((i+1) % 8 == 0)
			b[idx++] = ' ';
	}

	DBG(msg + String(b, 64));
}

void SynthSoundWithBitmask::dump()
{
	Helpers::dumpBitmask("sample", getBitmask(), 'S');
}

ComplexGroupManager::ComplexGroupManager(ReferenceCountedArray<SynthesiserSound>* allSounds, PooledUIUpdater* updater):
	soundList(allSounds),
	data(groupIds::Layers),
	updater(*this) 
{
	dataListener.setCallback(data, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(ComplexGroupManager::onDataChange));

	groupRebuildListener.setCallback(data, 
								     { groupIds::tokens, groupIds::purgable, groupIds::cached, groupIds::ignorable }, 
									 valuetree::AsyncMode::Synchronously,
									 BIND_MEMBER_FUNCTION_2(ComplexGroupManager::onRebuildPropertyChange));

}

ComplexGroupManager::~ComplexGroupManager()
{
	layers.clear();
	soundList = nullptr;

	if(sampler != nullptr)
		sampler->getSampleMap()->removeListener(this);
}

float ComplexGroupManager::getXFadeValue(uint8 layerIndex, SampleType* s) const
{
	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		return layers[layerIndex]->getXFadeValue(s);
	}

	return 1.0f;
}

uint8 ComplexGroupManager::getTableFadeValue(Bitmask m) const
{
	if(isPositiveAndBelow(tableFadeLayer, layers.size()))
	{
		return layers[tableFadeLayer]->getUnmaskedValue(m);
	}

	return 0;
}

void ComplexGroupManager::setLockLayer(uint8 layerIndex, bool shouldBeLocked, uint8 lockValue)
{
	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto l = layers[layerIndex];

		if(shouldBeLocked && lockValue != 0)
			applyFilter(layerIndex, lockValue, sendNotificationAsync);

		if(l->isLocked != shouldBeLocked)
		{
			l->isLocked = shouldBeLocked;
			
			if(Helpers::isProcessingHiseEvent(l->propertyFlags))
			{
				if(shouldBeLocked)
					midiProcessors.remove(l);
				else
					midiProcessors.insert(l);
			}

			if(Helpers::isPostProcessingSounds(l->propertyFlags))
			{
				if(shouldBeLocked)
					postProcessors.remove(l);
				else
					postProcessors.insert(l);
			}

			lockBroadcaster.sendMessage(sendNotificationAsync, layerIndex, shouldBeLocked);
		}
	}
}

float* ComplexGroupManager::calculateGroupModulationValuesForVoice(const HiseEvent& e, int voiceIndex, int startSample, int numSamples,
	SampleType::Bitmask m)
{
	auto useData = false;

	if(!modulationLayers.isEmpty())
	{
		auto startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		auto numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		FloatVectorOperations::fill(scratchBuffer.begin() + startSample_cr, 1.0f, numSamples_cr);

		for(auto p: modulationLayers)
			useData |= p->calculateGroupModulationValuesForVoice(e, voiceIndex, scratchBuffer.begin(), startSample_cr, numSamples_cr, m);

		if(useData)
			useData &= ModBufferExpansion::expand(scratchBuffer.begin(), startSample, numSamples, lastRampValues[voiceIndex]);

		if(!useData)
			constantVoiceValue = scratchBuffer[startSample_cr];
		else
			constantVoiceValue = 1.0f;
	}
	else
		constantVoiceValue = 1.0f;
	
	return useData ? scratchBuffer.begin() : nullptr;
}

float ComplexGroupManager::getConstantGroupModulationValue(int voiceIndex, SampleType::Bitmask m)
{
	return constantVoiceValue;
}

void ComplexGroupManager::prepare(PrepareSpecs ps)
{
	lastSpecs = ps;

	ps.numChannels = 1;
	
	if(!modulationLayers.isEmpty())
		DspHelpers::increaseBuffer(scratchBuffer, ps, false);		

	ps.sampleRate /= (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	ps.blockSize /= (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	for(auto l: layers)
		l->prepare(ps);
}

int ComplexGroupManager::getLayerIndex(const Identifier& groupId) const
{
	int idx = 0;

	for(auto l: layers)
	{
		if(l->groupId == groupId)
			return idx;

		idx++;
	}

	return -1;
}

struct ComplexGroupManager::LegatoLayer final : public ComplexGroupManager::Layer
{
	LegatoLayer(const ValueTree& v, int flags, ModulatorSampler* sampler):
	  Layer(v, Helpers::getId(v), LogicType::LegatoInterval, {}, flags | Flags::FlagProcessHiseEvent | Flags::FlagProcessPostSounds),
	  range(Helpers::getKeyRange(v, sampler))
	{
		uint8 layerValue = 1;

		for(int i = 0; i < 128; i++)
		{
			if(range[i])
			{
				tokens.add(MidiMessage::getMidiNoteName(i, true, true, 3));
				layerValues[i] = layerValue++;
			}
			else
				layerValues[i] = 0;
		}

		numItems = tokens.size();
	}

	int getPredelay(const ComplexGroupManager& parent, const HiseEvent& e, uint8 layerValue, int sampleSampleRate) const override
	{
		return 0;
	}

	std::map<SynthSoundWithBitmask*, std::vector<int>> zeroCrossings;

	void onCacheRebuild(ComplexGroupManager& parent) override
	{
		ModulatorSampler::SoundIterator iter(parent.sampler);

		zeroCrossings.clear();

		while(auto s = iter.getNextSound())
		{
			auto d = s->getSampleProperty(SampleIds::FileName).toString();
			zeroCrossings[s] = s->getReferenceToSound(0)->calculateZeroCrossings();
		}
	}

	double fadeTime = 0.05;

	void postVoiceStart(ComplexGroupManager& parent, const UnorderedStack<ModulatorSynthSound*>& soundsThatWereStarted) override
	{
		/*TODO:

- fix sample length updating zero crossing
- fix all note off resetting the filter values OK

- check with multiple groups
- check different sample rate & pitch ratios
- check support with existing start offset (use phase lock)
- check with looping OK
- check sustain pedal
- check retrigger

- add poly legato
- add timestretch to legato transition?

- refactor zero crossing to another place

- add parameters to UI: LockPhase, FadeTime
- add API methods



*/

		bool someTransitionPlaying = false;

		int firstZeroCrossing = 0;

		for(auto s: soundsThatWereStarted)
		{
			auto typed = static_cast<SynthSoundWithBitmask*>(s);
			auto v = getUnmaskedValue(typed->getBitmask());

			auto isTransition =  (v != IgnoreFlag);

			if(isTransition)
			{
				const auto& zp = zeroCrossings[typed];

				if(!zp.empty())
				{
					auto ms = static_cast<ModulatorSamplerSound*>(s);

					auto fadeLength = ms->getSampleRate() * fadeTime;

					auto l = zp[zp.size() - 1];

					auto position = l - fadeLength * 2;

					auto pos = std::lower_bound(zp.begin(), zp.end(), position);

					if(pos != zp.end())
						firstZeroCrossing = *pos;
				}
			}

			someTransitionPlaying |= isTransition;
		}

		if(someTransitionPlaying)
		{
			int np = 0;

			for(auto av: parent.sampler->activeVoices)
			{
				if(av->getCurrentHiseEvent().getEventId() == lastProcessedEvent.getEventId())
				{
					auto currentUptime = (int)av->getVoicePositionInSamples(true);

					auto s = static_cast<SynthSoundWithBitmask*>(av->getCurrentlyPlayingSound().get());

					const auto& zp = zeroCrossings[s];

					auto pos = std::upper_bound(zp.begin(), zp.end(), (int)currentUptime);

					if(pos != zp.end())
						np = (int)hmath::round(currentUptime) - *(pos - 1);

					av->setVolumeFade(fadeTime, 0.0);

					if(np != 0)
						break;
				}
			}

			for(auto s: soundsThatWereStarted)
			{
				auto v = getUnmaskedValue(static_cast<SynthSoundWithBitmask*>(s)->getBitmask());

				auto ms = static_cast<ModulatorSamplerSound*>(s);
				auto pf = ms->getReferenceToSound()->getPitchFactor(lastEvent.getNoteNumberIncludingTransposeAmount(), ms->getRootNote());
				pf *= ms->getSampleRate() / parent.sampler->getSampleRate();

				auto delay = firstZeroCrossing - np;
				auto delayPitched = roundToInt((double)firstZeroCrossing / pf) - np;

				HiseEvent m;
				m.setTimeStamp(delay);
				m.alignToRaster<HISE_EVENT_RASTER>(INT_MAX);
				auto aligned = m.getTimeStamp();
				m.setTimeStamp(delayPitched);
				m.alignToRaster<HISE_EVENT_RASTER>(INT_MAX);
				auto alignedPitched = m.getTimeStamp(); 

				auto delta = delayPitched - alignedPitched;
				auto offset = 0.0;

				if(delta > 0)
				{
					alignedPitched += HISE_EVENT_RASTER;
					offset = HISE_EVENT_RASTER - delta;
				}
				else
				{
					offset = -1 * delta;
				}

				if(v == IgnoreFlag)
				{
					
					// here we factor in the delay time
					parent.delayEventByFilter(layerIndex, IgnoreFlag, alignedPitched);
					parent.addStartOffsetByFilter(layerIndex, IgnoreFlag, offset);
					parent.fadeInEventByFilter(layerIndex, IgnoreFlag, fadeTime);
				}
				else
				{
					parent.delayEventByFilter(layerIndex, v, 0.0);
					parent.addStartOffsetByFilter(layerIndex, v, np);
					parent.fadeInEventByFilter(layerIndex, v, fadeTime);

					// here we use the delay value in the sample's time domain
					parent.setFixedLengthByFilter(layerIndex, v, aligned + np);
				}
			}
		}
		else
		{
			parent.delayEventByFilter(layerIndex, IgnoreFlag, 0.0);
			parent.addStartOffsetByFilter(layerIndex, IgnoreFlag, 0.0);
			parent.fadeInEventByFilter(layerIndex, IgnoreFlag, 0.0);
			parent.setFixedLengthByFilter(layerIndex, IgnoreFlag, 0.0);
		}
	}

	void resetPlayState(ComplexGroupManager& parent) override
	{
		lastEvent = {};
		lastProcessedEvent = {};

	}

	void handleHiseEvent(ComplexGroupManager& parent, const HiseEvent& e) override
	{
		auto n = e.getNoteNumberIncludingTransposeAmount();

		if(!range[n])
			return;

		if(e.isNoteOn())
		{
			auto lastNoteNumber = lastEvent.getNoteNumberIncludingTransposeAmount();

			if(isPositiveAndBelow(lastNoteNumber, 128) && layerValues[lastNoteNumber])
			{
				auto legatoGroup = layerValues[lastNoteNumber];
				parent.applyFilter(layerIndex, legatoGroup, sendNotificationAsync);
			}
			else
			{
				parent.applyFilter(layerIndex, IgnoreFlag, sendNotificationAsync);
				
			}

			lastProcessedEvent = lastEvent;
			lastEvent = e;
		}
		else if (e.isNoteOff())
		{
			if(lastEvent.getEventId() == e.getEventId())
			{
				lastEvent = {};
				lastProcessedEvent = lastEvent;
				parent.applyFilter(layerIndex, IgnoreFlag, sendNotificationAsync);
			}
		}
	}

	VoiceBitMap<128> range;
	std::array<uint8, 128> layerValues;
	
	HiseEvent lastEvent;
	HiseEvent lastProcessedEvent;
};

struct ComplexGroupManager::TableFadeLayer final : public ComplexGroupManager::Layer
{
	TableFadeLayer(ComplexGroupManager& parent, const ValueTree& v, const Identifier& g, const StringArray& tokens_, int flags):
	  Layer(v, g, LogicType::TableFade, tokens_, flags)
	{
		if(parent.sampler != nullptr)
		{
			parent.sampler->setAttribute(ModulatorSampler::Parameters::CrossfadeGroups, true, sendNotificationAsync);
			parent.sampler->setAttribute(ModulatorSampler::Parameters::RRGroupAmount, tokens.size(), sendNotificationAsync);
		}
	}
};

struct ComplexGroupManager::XFadeLayer final : public ComplexGroupManager::Layer
{
	enum class FaderTypes
	{
		Linear,
		RMS,
		CosineHalf,
		Overlap,
		Switch,
		numFaderTypes
	};

	enum class SourceTypes
	{
		EventData,
		MidiCC,
		GlobalMod,
		numSourceTypes
	};

	XFadeLayer(ComplexGroupManager& parent_, const ValueTree& layerData, int flags):
	  Layer(layerData, Helpers::getId(layerData), LogicType::XFade, Helpers::getTokens(layerData), flags),
	  parent(parent_)
	{
		auto t = const_cast<ValueTree*>(&layerData);

		if(!layerData.hasProperty(groupIds::fader))
			t->setProperty(groupIds::fader, getFaderNames()[0], nullptr);

		if(!layerData.hasProperty(groupIds::sourceType))
			t->setProperty(groupIds::sourceType, getSourceNames()[1], nullptr);

		if(!layerData.hasProperty(groupIds::slotIndex))
			t->setProperty(groupIds::slotIndex, 1, nullptr);

		typeListener.setCallback(layerData,
								 { groupIds::fader, groupIds::slotIndex, groupIds::sourceType }, 
								 valuetree::AsyncMode::Synchronously, 
								 BIND_MEMBER_FUNCTION_2(XFadeLayer::update));

		BACKEND_ONLY(valueUpdater.enableLockFreeUpdate(parent.sampler->getMainController()->getGlobalUIUpdater()));
	}

	static StringArray getSourceNames()
	{
		return { "Event Data", "Midi CC", "GlobalMod" };
	}

	static StringArray getFaderNames()
	{
		return { "Linear", "RMS", "Cosine half", "Overlap", "Switch" };
	}

	void prepare(PrepareSpecs ps) override
	{
		smoothedValue.prepare(ps.sampleRate, 1000.0);
	}

	void resetPlayState(ComplexGroupManager& parent) override
	{
		smoothedValue.set(midiValue);
		smoothedValue.reset();
	}

	double getFadeValue(Bitmask m, double input)
	{
		auto groupValue = getUnmaskedValue(m);

		if(groupValue == 1)
			return getFaderValueInternal<0>(input, numItems, currentType);
		if(groupValue == 2)
			return getFaderValueInternal<1>(input, numItems, currentType);
		if(groupValue == 3)
			return getFaderValueInternal<2>(input, numItems, currentType);
		if(groupValue == 4)
			return getFaderValueInternal<3>(input, numItems, currentType);
		if(groupValue == 5)
			return getFaderValueInternal<4>(input, numItems, currentType);
		if(groupValue == 6)
			return getFaderValueInternal<5>(input, numItems, currentType);
		if(groupValue == 7)
			return getFaderValueInternal<6>(input, numItems, currentType);
		if(groupValue == 8)
			return getFaderValueInternal<7>(input, numItems, currentType);

		return 0.0;
	}

	template <int N> static double getFaderValueInternal(double input, int numItems, FaderTypes t)
	{
		switch(t)
		{
		case FaderTypes::Linear: return faders::linear().getFadeValue<N>(numItems, input);
		case FaderTypes::RMS: return faders::rms().getFadeValue<N>(numItems, input);
		case FaderTypes::CosineHalf: return faders::cosine_half().getFadeValue<N>(numItems, input);
		case FaderTypes::Overlap: return faders::overlap().getFadeValue<N>(numItems, input);
		case FaderTypes::Switch: return faders::switcher().getFadeValue<N>(numItems, input);
		case FaderTypes::numFaderTypes: break;
		default: ;
		}

		return 1.0f;
	}

	void handleHiseEvent(ComplexGroupManager& parent, const HiseEvent& e) override
	{
		if(currentSource == SourceTypes::MidiCC)
		{
			if(e.isControllerOfType((int)slotIndex) || (slotIndex == 0 && e.isPitchWheel()))
			{
				midiValue = e.getControllerValue() / 127.0f;
				BACKEND_ONLY(valueUpdater.sendMessage(sendNotificationAsync, 0, midiValue));
				smoothedValue.set(midiValue);
			}
		}
	};

	

	bool calculateGroupModulationValuesForVoice(const HiseEvent& e, int voiceIndex, float* data, int startSample, int numSamples, Bitmask m) override
	{
		float value = 0.0f;
		float const* values = nullptr;

		if(currentSource == SourceTypes::EventData)
		{
			auto v = gm->additionalEventStorage.getValue(e.getEventId(), slotIndex);

			if(v.first)
			{
				value = (float)v.second;
			}
			else
			{
				data[0] = 1.0f;
				return false;
			}
		}
		else if(currentSource == SourceTypes::GlobalMod)
		{
			if(connectedGlobalMod == nullptr)
			{
				data[0] = 1.0f;
				return false;
			}

			switch(currentModType)
			{
			case GlobalModulator::ModulatorType::VoiceStart:
				value = globalModContainer->getConstantVoiceValue(connectedGlobalMod, e.getNoteNumberIncludingTransposeAmount());
				break;
			case GlobalModulator::TimeVariant:
				values = globalModContainer->getModulationValuesForModulator(connectedGlobalMod, startSample);
				break;
			case GlobalModulator::Envelope:
			{
				auto idx = globalModContainer->getEnvelopeIndex(connectedGlobalMod);
				values = globalModContainer->getEnvelopeValuesForModulator(idx, startSample, e);
				break;
			}
			case GlobalModulator::numTypes: 
				break;
			default: ;
			}
		}
		else if (currentSource == SourceTypes::MidiCC)
		{
			for(int i = 0; i < numSamples; i++)
			{
				value = smoothedValue.advance();
				value = getFadeValue(m, value);
				data[startSample + i] *= value;
			}

			return true;
		}
		
		if(values == nullptr)
		{
			auto input = jlimit(0.0f, 1.0f, value);
            ignoreUnused(input);
			BACKEND_ONLY(valueUpdater.sendMessage(sendNotificationAsync, voiceIndex, input));
			value = getFadeValue(m, value);
			FloatVectorOperations::multiply(data + startSample, value, numSamples);
		}
		else
		{
			auto input = jlimit(0.0f, 1.0f, values[0]);
            ignoreUnused(input);
			BACKEND_ONLY(valueUpdater.sendMessage(sendNotificationAsync, voiceIndex, input));

			for(int i = 0; i < numSamples; i++)
			{
				value = values[i];
				value = getFadeValue(m, value);
				data[startSample + i] *= value;
			}
		}

		return true;
	}

	LambdaBroadcaster<int, double> valueUpdater;

private:

	void rebuildConnection();

	void update(const Identifier& id, const var& newValue)
	{
		if(id == groupIds::fader)
		{
			auto idx = getFaderNames().indexOf(newValue.toString());

			if(isPositiveAndBelow(idx, (int)FaderTypes::numFaderTypes))
				currentType = (FaderTypes)idx;
		}
		if(id == groupIds::slotIndex)
		{
			slotIndex = (uint8)jlimit<int>(0, AdditionalEventStorage::NumDataSlots, (int)newValue);
			rebuildConnection();
		}
		if(id == groupIds::sourceType)
		{
			auto idx = getSourceNames().indexOf(newValue.toString());

			if(isPositiveAndBelow(idx, (int)SourceTypes::numSourceTypes))
				currentSource = (SourceTypes)idx;

			rebuildConnection();
		}
	}

	valuetree::PropertyListener typeListener;
	scriptnode::routing::GlobalRoutingManager::Ptr gm;
	WeakReference<Processor> connectedGlobalMod;
	WeakReference<GlobalModulatorContainer> globalModContainer;
	int8 slotIndex = -1;

	ComplexGroupManager& parent;

	sfloat smoothedValue;

	FaderTypes currentType = FaderTypes::Linear;
	SourceTypes currentSource = SourceTypes::EventData;
	GlobalModulator::ModulatorType currentModType = GlobalModulator::ModulatorType::TimeVariant;

	float midiValue = 0.0f;
};

void ComplexGroupManager::XFadeLayer::rebuildConnection()
{
	if(slotIndex == -1)
		return;

	switch(currentSource)
	{
	case SourceTypes::EventData:
		gm = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(parent.sampler->getMainController());
		break;
	case SourceTypes::MidiCC: 
		break;
	case SourceTypes::GlobalMod: 
	{
		auto chain = parent.sampler->getMainController()->getMainSynthChain();
		globalModContainer = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain);

		if(globalModContainer != nullptr)
		{
			connectedGlobalMod = globalModContainer->getChildProcessor(ModulatorSynth::GainModulation)->getChildProcessor(slotIndex);

			if(auto env = dynamic_cast<EnvelopeModulator*>(connectedGlobalMod.get()))
				currentModType = GlobalModulator::ModulatorType::Envelope;
			else if(auto tv = dynamic_cast<TimeVariantModulator*>(connectedGlobalMod.get()))
				currentModType = GlobalModulator::ModulatorType::TimeVariant;
			else if (auto vs = dynamic_cast<VoiceStartModulator*>(connectedGlobalMod.get()))
				currentModType = GlobalModulator::ModulatorType::VoiceStart;
		}
		
		break;
	}
	case SourceTypes::numSourceTypes: break;
	default: ;
	}
		
}

struct ComplexGroupManager::KeyswitchLayer final : public ComplexGroupManager::Layer
{
	KeyswitchLayer(const ValueTree& v, int flags):
	  Layer(v, Helpers::getId(v), LogicType::Keyswitch, Helpers::getTokens(v), flags | Flags::FlagProcessHiseEvent)
	{
		setPropertiesToWatch({ SampleIds::LoKey, groupIds::isChromatic });
		
	}

	void onValueTreeUpdate(const Identifier& id, const var& newValue)
	{
		auto map = Helpers::getKeyRange(data);

		uint8 layerIndex = 1;

		for(int i = 0; i < 128; i++)
		{
			if(map[i])
				layerValues[i] = layerIndex++;
			else
				layerValues[i] = 0;
		}
	}
	
	void handleHiseEvent(ComplexGroupManager& parent, const HiseEvent& e) override
	{
		if(e.isNoteOn())
		{
			auto n = e.getNoteNumberIncludingTransposeAmount();

			if(auto keyswitch = layerValues[n])
				parent.applyFilter(layerIndex, keyswitch, sendNotificationAsync);
		}
	}

	std::array<uint8, 128> layerValues;
};

struct ComplexGroupManager::RRLayer final : public ComplexGroupManager::Layer
{
	RRLayer(const ValueTree& v, const Identifier& g, const StringArray& tokens_, int flags):
	  Layer(v, g, LogicType::RoundRobin, tokens_, flags | Flags::FlagProcessHiseEvent | Flags::FlagProcessPostSounds)
	{}

	void postVoiceStart(ComplexGroupManager& parent, const UnorderedStack<ModulatorSynthSound*>& soundsThatWereStarted) override
	{
		bool shouldBump = false;

		for(auto s: soundsThatWereStarted)
		{
			if((static_cast<SampleType*>(s)->getBitmask() & filter) != 0)
			{
				shouldBump = true;
				break;
			}
		}

		if(shouldBump)
		{
			currentRRGroup++;

			if(currentRRGroup > numItems)
				currentRRGroup = 1;
		}
		else
		{
			resetPlayState(parent);
		}
	}

	void handleHiseEvent(ComplexGroupManager& parent, const HiseEvent& e) override
	{
		if(e.isNoteOn())
		{
			parent.applyFilter(layerIndex, currentRRGroup, sendNotificationAsync);
		}
	}

	void resetPlayState(ComplexGroupManager& parent) override
	{
		currentRRGroup = 1;
		parent.applyFilter(layerIndex, currentRRGroup, sendNotificationAsync);
	}
	
	uint8 currentRRGroup = 1;
};

void ComplexGroupManager::addLayer(const Identifier& groupId, LogicType lt, const StringArray& tokens, int flags)
{
	jassertfalse;
}

uint8 ComplexGroupManager::Helpers::getNumBitsRequired(uint8 numItems)
{
	auto numMaxBit = numItems + 1; // one-based to prevent zero mask
	auto numBitsRequired = roundToInt(log2(nextPowerOfTwo(numMaxBit)));
	return numBitsRequired;
}

bool ComplexGroupManager::Helpers::canBePurged(int flag)	{ return flag & (int)FlagPurgable; }
bool ComplexGroupManager::Helpers::canBeIgnored(int flag)	{ return flag & (int)FlagIgnorable; }
bool ComplexGroupManager::Helpers::shouldBeCached(int flag) { return flag & (int)FlagCached; }
bool ComplexGroupManager::Helpers::isXFade(int flag)		{ return flag & (int)FlagXFade; }

int ComplexGroupManager::Helpers::getDefaultValue(LogicType lt, Flags flag)
{
	static constexpr int UNSPECIFIED = -1;
	static constexpr int NO = 0;
	static constexpr int YES = 1;

	switch(lt)
	{
	case LogicType::Undefined:
		return -1;
	case LogicType::Custom:
		return -1;
	case LogicType::RoundRobin:
		switch(flag)
		{
		case FlagIgnorable: return NO;
		case FlagCached:	return UNSPECIFIED;
		case FlagPurgable:	return NO;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::Keyswitch:
		switch(flag)
		{
		case FlagIgnorable: return NO;
		case FlagCached:	return YES;
		case FlagPurgable:	return YES;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::TableFade:
		switch(flag)
		{
		case FlagIgnorable: return NO;
		case FlagCached:	return NO;
		case FlagPurgable:	return NO;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::XFade:
		switch(flag)
		{
		case FlagIgnorable: return NO;
		case FlagCached:	return NO;
		case FlagPurgable:	return NO;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::LegatoInterval:
		switch(flag)
		{
		case FlagIgnorable: return YES;
		case FlagCached:	return NO;
		case FlagPurgable:	return NO;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::ReleaseTrigger:
		switch(flag)
		{
		case FlagIgnorable: return NO;
		case FlagCached:	return YES;
		case FlagPurgable:	return YES;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::Choke:
		switch(flag)
		{
		case FlagIgnorable: return YES;
		case FlagCached:	return NO;
		case FlagPurgable:	return NO;
		default: ;			return UNSPECIFIED;
		}
	case LogicType::numLogicTypes:
	default: return -1;;
	}
}

bool ComplexGroupManager::Helpers::isProcessingHiseEvent(int flag)  { return flag & (int)FlagProcessHiseEvent; }
bool ComplexGroupManager::Helpers::isPostProcessingSounds(int flag) { return flag & (int)FlagProcessPostSounds; }
bool ComplexGroupManager::Helpers::isProcessingModulation(int flag) { return flag & (int)FlagProcessModulation; }

Identifier ComplexGroupManager::Helpers::getNextFreeId(LogicType lt, const ValueTree& data)
{
	auto id = getLogicTypeName(lt);

	int numExisting = 0;

	for(auto c: data)
	{
		if(c[groupIds::id].toString().startsWith(id))
			numExisting++;
	}

	if(numExisting == 0)
		return id;

	return id + String(numExisting);
}

StringArray ComplexGroupManager::Helpers::getLogicTypeNames()
{
	static const StringArray sa({ "Ignore", "Custom", "RR", "Keyswitch", "TableFade", "XFade", "Legato", "Release", "Choke" });
	return sa;
}

String ComplexGroupManager::Helpers::getLogicTypeName(LogicType lt)
{
	return getLogicTypeNames()[(int)lt];
}

String ComplexGroupManager::Helpers::getItemName(LogicType lt, const StringArray& items)
{
	static const StringArray sa({ "Ignore", "groups", "groups", "articulations", "layers", "layers", "states", "funky", "states" });
	return sa[(int)lt];
}

ValueTree ComplexGroupManager::Helpers::getRoot(const ValueTree& v)
{
	return valuetree::Helpers::findParentWithType(v, groupIds::Layers);
}

uint8 ComplexGroupManager::Helpers::getLayerIndex(const ValueTree& layerData)
{
	jassert(layerData.getParent().isValid());
	jassert(layerData.getType() == groupIds::Layer);
	auto p = layerData.getParent();
	jassert(p.getType() == groupIds::Layers);
	return (uint8)p.indexOf(layerData);
}

ComplexGroupManager::LogicType ComplexGroupManager::Helpers::getLogicType(const ValueTree& layerData)
{
	jassert(layerData.getType() == groupIds::Layer);
	auto sa = getLogicTypeNames();
	auto idx = sa.indexOf(layerData[groupIds::type].toString());
	return (LogicType)idx;
}

StringArray ComplexGroupManager::Helpers::getTokens(const ValueTree& layerData, const ModulatorSampler* sampler)
{
	jassert(layerData.getType() == groupIds::Layer);

	auto lt = getLogicType(layerData);

	if(lt == LogicType::LegatoInterval)
	{
		if(sampler != nullptr)
		{
			StringArray tokens;

			BigInteger bi;

			ModulatorSampler::SoundIterator iter(sampler);

			while(auto s = iter.getNextSound())
			{
				auto nr = s->getNoteRange();
				bi.setRange(nr.getStart(), nr.getLength(), true);
			}

			if(bi.isZero())
				throw Result::fail("You need to add samples before creating a legato layer");

			for(int i = 0; i < 127; i++)
			{
				if(bi[i])
					tokens.add(String(i));
			}

			return tokens;
		}
		else
		{
			// The tokens must be calculated otherwise...
			return {};
		}
	}
	else
	{
		auto sa = StringArray::fromTokens(layerData[groupIds::tokens].toString(), ",", "");

		sa.removeDuplicates(false);
		sa.removeEmptyStrings(true);
		sa.trim();

		return sa;
	}
}

Identifier ComplexGroupManager::Helpers::getId(const ValueTree& layerData)
{
	auto id = layerData[groupIds::id].toString();

	if(id.isEmpty())
		id = getLogicTypeName(getLogicType(layerData));

	return Identifier(id);
}

VoiceBitMap<128> ComplexGroupManager::Helpers::getKeyRange(const ValueTree& data, const ModulatorSampler* sampler)
{
	auto lt = getLogicType(data);

	VoiceBitMap<128> range;

	
	auto lowIndex = (int)data[SampleIds::LoKey];

	if(lt == LogicType::Keyswitch)
	{
		if(!data.hasProperty(SampleIds::LoKey))
			return range;

		auto numElements = Helpers::getTokens(data).size();

		const auto isChromatic = (bool)data[groupIds::isChromatic];

		auto i = lowIndex;

		static const bool black[12] = { false, true, false, true, false, false, true, false, true, false, true, false };

		while(numElements > 0)
		{
			auto key = i++ % 12;
			auto isBlack = black[key];

			if(!isChromatic && isBlack)
				continue;
			
			range.setBit(i-1, true);
			numElements--;
		}
	}
	if(lt == LogicType::LegatoInterval)
	{
		jassert(sampler != nullptr);

		auto tokens = getTokens(data, sampler);

		if(!tokens.isEmpty())
		{
			lowIndex = tokens[0].getIntValue();
			auto end = tokens[tokens.size()-1].getIntValue();

			for(int i = lowIndex; i <= end; i++)
				range.setBit(i, true);
		}
	}

	return range;
}

String ComplexGroupManager::Helpers::getSampleFilename(const SynthSoundWithBitmask* fs)
{
	if(auto s = dynamic_cast<const ModulatorSamplerSound*>(fs))
	{
		auto ref = s->getSampleProperty(SampleIds::FileName).toString();

		PoolReference r(s->getMainController(), ref, FileHandlerBase::SubDirectories::Samples);

		return r.getFile().getFileNameWithoutExtension();
	}

	return {};
}

StringArray ComplexGroupManager::Helpers::getFileTokens(const SynthSoundWithBitmask* fs, String separator)
{
	auto f = getSampleFilename(fs);
	return getFileTokens(f, separator);
	
}

StringArray ComplexGroupManager::Helpers::getFileTokens(const String& fileName, String separator)
{
	auto t = StringArray::fromTokens(fileName, separator, "");
	t.trim();
	t.removeEmptyStrings(true);
	return t;
}

Identifier ComplexGroupManager::getLayerId(uint8 layerIndex) const
{
	if(isPositiveAndBelow(layerIndex, layers.size()))
		return layers[layerIndex]->groupId;

	return {};
}

#if JUCE_DEBUG
void ComplexGroupManager::dumpLayers()
{
	String s;
	s.preallocateBytes(64 + 8);

	int idx = 0;

	for(auto l_: layers)
	{
		auto& l = *l_;
		auto c = (char)l.groupId.toString()[0];

		auto numBits = Helpers::getNumBitsRequired(l.numItems);

		for(int i = 0; i < numBits; i++)
		{
			if((idx) % 8 == 0)
				s << ' ';

			s << c;
			idx++;
		}

		if(Helpers::canBeIgnored(l.propertyFlags))
		{
			if((idx) % 8 == 0)
				s << ' ';

			s << 'I';
			idx++;
		}
	}

	DBG("layers:" + s);
}
#endif

void ComplexGroupManager::finalize()
{
	if(skipUpdate)
		return;

	jassert(soundList != nullptr);

	midiProcessors.clearQuick();
	postProcessors.clearQuick();

	tableFadeLayer = -1;
	uint8 bitPosition = 0;

	currentFilters.clear();

	int idx = 0;

	for(auto l_: layers)
	{
		auto& l = *l_;
		auto numBitsRequired = Helpers::getNumBitsRequired(l.numItems);

		Bitmask filter = 0;

		for(int i = 0; i < numBitsRequired; i++)
		{
			filter = filter << 1;
			filter |= 0x01;
		}

		l.layerIndex = idx;
		l.filter = filter << bitPosition;
		l.bitShift = bitPosition;
		bitPosition += numBitsRequired;
			
		if(l.lt == LogicType::TableFade)
		{
			if(tableFadeLayer != -1)
			{
				// only one table fade layer allowed
				jassertfalse;
			}

			tableFadeLayer = idx;
		}

		if(Helpers::canBeIgnored(l.propertyFlags))
		{
			l.ignoreFlag = (Bitmask)1 << bitPosition;
			bitPosition++;
		}

		if(Helpers::isPostProcessingSounds(l.propertyFlags))
			postProcessors.insertWithoutSearch(&l);

		if(Helpers::isProcessingHiseEvent(l.propertyFlags))
			midiProcessors.insertWithoutSearch(&l);

		if(Helpers::isProcessingModulation(l.propertyFlags))
			modulationLayers.insertWithoutSearch(&l);
			
		// layers must not be ignoreable and cached at once!
		jassert(!(Helpers::canBeIgnored(l.propertyFlags) && Helpers::shouldBeCached(l.propertyFlags)));

		if(Helpers::canBeIgnored(l.propertyFlags))
		{
			for(auto s: *soundList)
			{
				auto bm = static_cast<SynthSoundWithBitmask*>(s)->getBitmask();

				auto isAssigned = (bm & l.filter) != 0;
				auto isIgnored = (bm & l.ignoreFlag) != 0;

				if(isAssigned && isIgnored)
				{
					// if that happens, then the bit mask for this sample was all over the place
					// (it should never be assigned & ignored at the same time, which is usually a result
					// of an old bit value hanging around messing up the logic.
					debugToConsole(sampler.get(), "Clear layer values for " + l.groupId.toString());

					// clear out the values for the entire layer
					auto mask = ~(l.filter | l.ignoreFlag);
					bm &= mask;

					static_cast<SynthSoundWithBitmask*>(s)->storeBitmask(bm, true);
				}
			}
		}

		

		idx++;
	}

#if JUCE_DEBUG
	dumpLayers();
#endif
	rebuildGroups();

	if(lastSpecs)
		prepare(lastSpecs);
}

uint8 ComplexGroupManager::getLayerValue(Bitmask m, uint8 layerIndex) const
{
	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];
		return l.getUnmaskedValue(m);
	}

	return 0;
}

String ComplexGroupManager::getLayerValueAsToken(SampleType* s, uint8 layerIndex) const
{
	if(auto v = getLayerValue(s->getBitmask(), layerIndex))
	{
		if(v == IgnoreFlag)
			return "0xFF";
		else
			return layers[layerIndex]->tokens[v-1];
	}

	return {};
}

String ComplexGroupManager::getLayerValueAsToken(uint8 layerIndex, uint8 layerValue) const
{
	jassert(layerValue != 0);
	jassert(layerValue != IgnoreFlag);

	if(auto p = layers[layerIndex])
		return p->tokens[layerValue-1];

	return {};
}

void ComplexGroupManager::setPurged(const Identifier& id, const String& token, bool shouldBePurged)
{
	ValueWithFilter f;

	uint8 idx = 0;

	for(auto l_: layers)
	{
		auto& l = *l_;

		if(l.groupId == id)
		{
			if(!Helpers::canBePurged(l.propertyFlags))
				throw Result::fail("CanBePurged flag is not set");

			if(auto value = l.tokens.indexOf(token) + 1)
			{
				l.setValueFilter(f, value);
				// for purging we don't want to ignore the given filter
				f.ignoreMask = 0; 

				for(auto s: *soundList)
				{
					auto typed = dynamic_cast<SynthSoundWithBitmask*>(s);
					auto bm = typed->getBitmask();

					if(f.matches(bm))
						typed->setPurged(shouldBePurged);
				}
			}
			else
			{
				throw Result::fail("Can't find token " + token + " in layer " + id);
			}

			break;
		}

		idx++;
	}

	refreshCache();
}

void ComplexGroupManager::applyFilter(const Identifier& layerId, const String& tokenValue, NotificationType n)
{
	uint8 idx = 0;

	for(auto l_: layers)
	{
		auto& l = *l_;

		if(l.groupId == layerId)
		{
			if(auto v = l.tokens.indexOf(tokenValue) + 1)
				applyFilter(idx, (uint8)v, n);
			else if (tokenValue == "all" && Helpers::canBeIgnored(l.propertyFlags))
				applyFilter(idx, IgnoreFlag, n);
			else
				throw Result::fail("Cannot find token " + tokenValue + " at layer " + layerId);
				
			break;
		}

		idx++;
	}
}

Array<ComplexGroupManager::ValueWithFilter> ComplexGroupManager::getAllFiltersForLayer(uint8 layerIndex) const
{
	Array<ValueWithFilter> list;

	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];

		for(uint8 v = 0; v < l.numItems; v++)
		{
			ValueWithFilter newFilter;
			l.setValueFilter(newFilter, v+1);
			list.add(newFilter);
		}
	}

	return list;
}

void ComplexGroupManager::applyFilter(uint8 layerIndex, uint8 value, NotificationType n)
{
	// must never be zero!
	jassert(value != 0);
	
	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];

		if(Helpers::canBeIgnored(l.propertyFlags))
		{
			ValueWithFilter newFilter;
			l.setValueFilter(newFilter, value);

			auto& l = currentFilters.ignorableFilters;
			bool found = false;

			for(auto& e: l)
			{
				if(e.first == layerIndex)
				{
					e.second = newFilter;
					found = true;
					break;
				}
			}

			if(!found)
				l.insertWithoutSearch({ layerIndex, newFilter });
		}
		else
		{
			l.setValueFilter(currentFilters.mainFilter, value);
			jassert(currentFilters.mainFilter.ignoreMask == 0);
		}

		if(Helpers::shouldBeCached(l.propertyFlags))
			l.mask(currentGroupFilter, value);

		l.playStateBroadcaster.sendMessage(n, value);
	}
}

void ComplexGroupManager::applyEventDataInternal(StartData::Type dataType, uint8 layerIndex, uint8 layerValue, double value)
{
	if(auto l = layers[layerIndex])
	{
		StartData sd;
		sd.layerIndex = layerIndex;
		sd.layerValue = layerValue;

		auto idx = activeDelayLayers.indexOf(sd);

		if(idx != -1)
		{
			auto& ed = *(activeDelayLayers.begin() + idx);
			ed.data[(int)dataType] = value;

			if(ed.isEmpty())
				activeDelayLayers.removeElement(idx);
		}
		else
		{
			sd.data[(int)dataType] = value;
			activeDelayLayers.insertWithoutSearch(sd);
		}
	}
}

void ComplexGroupManager::delayEventByFilter(uint8 layerIndex, uint8 value, double delayInSamples)
{
	applyEventDataInternal(StartData::Type::DelayTimeIndex, layerIndex, value, delayInSamples);
}

void ComplexGroupManager::fadeInEventByFilter(uint8 layerIndex, uint8 value, double fadeInTime)
{
	applyEventDataInternal(StartData::Type::FadeTimeIndex, layerIndex, value, fadeInTime);
}

void ComplexGroupManager::setFixedLengthByFilter(uint8 layerIndex, uint8 value, double numSamplesToPlayBeforeFadeout)
{
	applyEventDataInternal(StartData::Type::FadeOutOffset, layerIndex, value, numSamplesToPlayBeforeFadeout);
}

void ComplexGroupManager::addStartOffsetByFilter(uint8 layerIndex, uint8 value, double startOffset)
{
	applyEventDataInternal(StartData::Type::StartOffsetIndex, layerIndex, value, startOffset);
}

void ComplexGroupManager::collectSounds(const HiseEvent& m, UnorderedStack<ModulatorSynthSound*>& soundsToBeStarted)
{
	auto nn = m.getNoteNumberIncludingTransposeAmount();

	if(groups.size() == 1)
	{
		for(const auto& t: groups[0]->createIterator(nn))
		{
			if(matchesCurrentFilter(m, const_cast<SampleType*>(t)))
				soundsToBeStarted.insert(const_cast<SampleType*>(t));
		}
	}
	else
	{
		for(auto& g: groups)
		{
			auto v = g->getValueWithFilter();

			if(!v.matches(currentGroupFilter))
				continue;

			for(const auto& t: g->createIterator(nn))
			{
				if(matchesCurrentFilter(m, const_cast<SampleType*>(t)))
					soundsToBeStarted.insert(const_cast<SampleType*>(t));
			}
		}
	}

	if(!deactivatePostProcessing)
	{
		for(auto p: postProcessors)
			p->postVoiceStart(*this, soundsToBeStarted);
	}
}

ModulatorSynth::SoundCollectorBase::SpecialStart ComplexGroupManager::getSpecialSoundStart(const HiseEvent& m, ModulatorSynthSound* sound) const
{
	if(!activeDelayLayers.isEmpty())
	{
		for(const auto& startData: activeDelayLayers)
		{
			auto l = layers[startData.layerIndex];
			jassert(l != nullptr);
			auto sm = static_cast<SynthSoundWithBitmask*>(sound)->getBitmask();

			if(l->getUnmaskedValue(sm) == startData.layerValue)
			{
				SoundCollectorBase::SpecialStart sd;

				sd.m = m;
				sd.sound = sound;
				sd.delayTimeSamples = startData.data[(int)StartData::Type::DelayTimeIndex];
				sd.startOffset = startData.data[(int)StartData::Type::StartOffsetIndex];
				sd.fadeInTimeSeconds = startData.data[(int)StartData::Type::FadeTimeIndex];
				sd.fixedLengthSamples = startData.data[(int)StartData::Type::FadeOutOffset];

				jassert(sd);
				return sd;
			}
		}
	}

	return {};
}

int ComplexGroupManager::getPredelayForVoice(const ModulatorSynthVoice* voice) const
{
	int delay = 0;

	if(!deactivatePostProcessing)
	{
		auto m = voice->getCurrentHiseEvent();
		auto s = static_cast<const ModulatorSamplerSound*>(voice->getCurrentlyPlayingSound().get());
		auto sr = (int)s->getSampleRate();
		auto bm = s->getBitmask();

		for(auto l: postProcessors)
		{
			auto v = l->getUnmaskedValue(bm);
			delay += l->getPredelay(*this, m, v, sr);
		}
	}

	return delay;
}

ComplexGroupManager::Bitmask ComplexGroupManager::parseBitmask(const String& filenameWithoutExtension,
                                                               uint8 layerOffset) const
{
	auto tokens = StringArray::fromTokens(filenameWithoutExtension, "_", "");

	Bitmask m = 0;

	for(int i = layerOffset; i < tokens.size(); i++)
	{
		auto li = i - layerOffset;

		if(isPositiveAndBelow(li, layers.size()))
		{
			auto& l = *layers[li];

			if(isPositiveAndBelow(li, layers.size()))
			{
				auto id = l.groupId;

				auto t = tokens[i].trim();

				if(t.toLowerCase() == "all")
				{
					l.mask(m, IgnoreFlag);
				}
				else if(auto v = l.tokens.indexOf(tokens[i].trim()) + 1)
				{
					l.mask(m, v);
				}
			}
		}
	}

	return m;
}

bool ComplexGroupManager::matchesCurrentFilter(const HiseEvent& m, SampleType* typed) const
{
	auto fitsMask = currentFilters.matches(typed->getBitmask());
		
	// Already handled by the note container
	jassert(typed->appliesToNote(m.getNoteNumberIncludingTransposeAmount()));
	fitsMask &= typed->appliesToVelocity(m.getVelocity());

	return fitsMask;
}

void ComplexGroupManager::setSampleId(SampleType* s, const Array<std::pair<Identifier, uint8>>& layerValues, bool useUndo) const
{
	Bitmask m = s->getBitmask();

	for(const auto l_: layers)
	{
		auto& l = *l_;

		for(const auto& c: layerValues)
		{
			if(c.first == l.groupId)
			{
				jassert(isPositiveAndBelow(c.second-1, l.numItems) || c.second == IgnoreFlag);
				l.mask(m, c.second);
				break;
			}
		}
	}

	s->storeBitmask(m, useUndo);
}

void ComplexGroupManager::clearSampleId(SampleType* s, const Array<Identifier>& layerIds, bool useUndo)
{
	Bitmask m = s->getBitmask();

	for(const auto l_: layerIds)
	{
		auto idx = getLayerIndex(l_);

		if(isPositiveAndBelow(idx, layers.size()))
		{
			auto l = layers[idx];
			l->clearBits(m);
		}
	}

	s->storeBitmask(m, useUndo);
}

ComplexGroupManager::ValueWithFilter ComplexGroupManager::getDisplayFilter(
	const Array<std::pair<Identifier, uint8>>& layerValues) const
{
	ValueWithFilter filter;

	for(auto& lv: layerValues)
	{
		auto idx = getLayerIndex(lv.first);

		if(auto l = layers[idx])
		{
			l->setValueFilter(filter, lv.second);
		}
	}

	filter.ignoreMask = 0;
	return filter;
}

Array<WeakReference<ComplexGroupManager::SampleType>> ComplexGroupManager::getFilteredSelection(
	uint8 layerIndex, uint8 value, bool includeIgnoredSamples) const
{
	jassert(soundList != nullptr);
	Array<WeakReference<SampleType>> list;
	list.ensureStorageAllocated(soundList->size());

	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];

		ValueWithFilter newFilter;

		l.setValueFilter(newFilter, value);

		if(!includeIgnoredSamples)
			newFilter.ignoreMask = 0;

		for(auto s: *soundList)
		{
			auto typed = dynamic_cast<SampleType*>(s);

			auto bm = typed->getBitmask();

			if(newFilter.matches(bm))
				list.add(typed);
		}
	}
	
	return list;
}

Array<WeakReference<ComplexGroupManager::SampleType>> ComplexGroupManager::getUnassignedSamples(uint8 layerIndex) const
{
	Array<WeakReference<SampleType>> list;

	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];

		auto layerMask = l.filter;
		layerMask |= l.ignoreFlag;

		for(auto s: *soundList)
		{
			auto typed = dynamic_cast<SampleType*>(s);
			auto bm = typed->getBitmask();

			if((bm & layerMask) == 0)
				list.add(typed);
		}
	}

	return list;
}

int ComplexGroupManager::getNumFiltered(uint8 layerIndex, uint8 value, bool includeIgnoredSamples) const
{
	// Make this faster at some point...
	return getFilteredSelection(layerIndex, value, includeIgnoredSamples).size();
}

void ComplexGroupManager::preHiseEventCallback(const HiseEvent& e)
{
	if(e.isAllNotesOff())
		resetPlayingState();

	for(auto mp: midiProcessors)
		mp->handleHiseEvent(*this, e);
}

void ComplexGroupManager::resetPlayingState()
{
	for(auto l: layers)
		l->resetPlayState(*this);
}

ComplexGroupManager::ScopedUpdateDelayer::ScopedUpdateDelayer(ComplexGroupManager& gm_):
	gm(gm_),
	prevValue(gm_.skipUpdate)
{
	gm.skipUpdate = true;
}

ComplexGroupManager::ScopedUpdateDelayer::~ScopedUpdateDelayer()
{
	gm.skipUpdate = prevValue;
	gm.finalize();
}

ComplexGroupManager::ScopedPostProcessorDeactivator::ScopedPostProcessorDeactivator(ComplexGroupManager& gm_):
	gm(gm_),
	prevValue(gm.deactivatePostProcessing)
{
	gm.deactivatePostProcessing = true;
}

ComplexGroupManager::ScopedPostProcessorDeactivator::~ScopedPostProcessorDeactivator()
{
	gm.deactivatePostProcessing = prevValue;
}

void ComplexGroupManager::onDataChange(const ValueTree& c, bool wasAdded)
{
	if(wasAdded)
	{
		auto id = Helpers::getId(c);
		auto lt = Helpers::getLogicType(c);

		auto tokens = Helpers::getTokens(c, sampler.get());

		int flags = 0;

		if((bool)c[groupIds::cached])
			flags |= Flags::FlagCached;

		if((bool)c[groupIds::ignorable])
			flags |= Flags::FlagIgnorable;

		if((bool)c[groupIds::purgable])
			flags |= Flags::FlagPurgable;

		if(Helpers::canBeIgnored(flags) && Helpers::shouldBeCached(flags))
			throw Result::fail("Can't ignore & cache groups");

		for(auto l: layers)
		{
			if(l->groupId == id)
				throw Result::fail("Group ID already exists");
		}

		if(lt == LogicType::LegatoInterval)
		{
			if(Helpers::canBePurged(flags))
				throw Result::fail("Legato intervals should not be purgable");
			
			layers.add(new LegatoLayer(c, flags, sampler.get()));
		}
		else if (lt == LogicType::RoundRobin)
		{
			layers.add(new RRLayer(c, id, tokens, flags));
		}
		else if (lt == LogicType::Keyswitch)
		{
			layers.add(new KeyswitchLayer(c, flags));
		}
		else if (lt == LogicType::TableFade)
		{
			jassert(!Helpers::shouldBeCached(flags));
			layers.add(new TableFadeLayer(*this, c, id, tokens, flags));
		}
		else if (lt == LogicType::XFade)
		{
			flags |= Flags::FlagProcessModulation;
			flags |= Flags::FlagProcessHiseEvent;
			layers.add(new XFadeLayer(*this, c, flags));
		}
		else
		{
			layers.add(new Layer(c, id, lt, tokens, flags));
		}
	}
	else
	{
		auto id = Helpers::getId(c);

		for(auto l: layers)
		{
			if(l->groupId == id)
			{
				layers.removeObject(l);
				break;
			}
		}
	}

	finalize();
}

void ComplexGroupManager::onRebuildPropertyChange(const ValueTree& t, const Identifier& id)
{
	layers.clear();

	ScopedUpdateDelayer sd(*this);

	for(auto c: data)
	{
		onDataChange(c, true);
	}
}

std::pair<int, int> ComplexGroupManager::getNumUnassignedAndIgnored(uint8 layerIndex) const
{
	int numAssigned = 0;
	int numIgnored = 0;
	int numTotal = soundList->size();

	if(isPositiveAndBelow(layerIndex, layers.size()))
	{
		auto& l = *layers[layerIndex];

		auto layerMask = l.filter;

		for(auto s: *soundList)
		{
			auto typed = dynamic_cast<SynthSoundWithBitmask*>(s);
			auto bm = typed->getBitmask();

			if((bm & layerMask) != 0)
				numAssigned++;

			if((bm & l.ignoreFlag) != 0)
				numIgnored++;
		}
	}

	return { numTotal - numAssigned - numIgnored, numIgnored };
}

void ComplexGroupManager::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue)
{
	if(id == SampleIds::RRGroup)
		updater.triggerAsyncUpdate();
}

void ComplexGroupManager::setSampler(ModulatorSampler* m)
{
	sampler = m;

	PrepareSpecs ps;
	ps.numChannels = sampler->getMatrix().getNumSourceChannels();
	ps.sampleRate = sampler->getSampleRate();
	ps.blockSize = sampler->getLargestBlockSize();

	if(ps)
		prepare(ps);

	sampler->getSampleMap()->addListener(this);
#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
	sampler->getSampleEditHandler()->registerComplexGroupManager();
#endif
}



ComplexGroupManager::Layer::Layer(const ValueTree& v, const Identifier& g, LogicType lt_, const StringArray& tokens_, int flags):
	data(v),
	groupId(g),
	lt(lt_),
	tokens(tokens_),
	numItems((tokens_.size())),
	propertyFlags(flags)
{
	if(lt == LogicType::TableFade || lt == LogicType::XFade)
		propertyFlags |= FlagXFade;
}

uint8 ComplexGroupManager::Layer::getUnmaskedValue(Bitmask m) const
{
	if(Helpers::canBeIgnored(propertyFlags) && ((m & ignoreFlag) != 0))
		return IgnoreFlag;

	m &= filter;
	auto v = m >> bitShift;
	return (uint8)v;
}

float ComplexGroupManager::Layer::getXFadeValue(SampleType* t) const
{
	jassert(Helpers::isXFade(propertyFlags));

	auto m = t->getBitmask();

	auto value = getUnmaskedValue(m);

	if(value == 0)
		return 1.0f;

	auto v = (float)(value - 1);

	v /= (float)(numItems-1);
	return v;
}

void ComplexGroupManager::Layer::setValueFilter(ValueWithFilter& v, uint8 value) const
{
	mask(v.value, value);
	v.filterMask |= filter;
	v.ignoreMask = ignoreFlag;
}

void ComplexGroupManager::Layer::mask(Bitmask& m, uint8 value) const
{
	const Bitmask inverted = ~filter;

	m &= inverted;

	if(value == IgnoreFlag)
	{
		jassert(Helpers::canBeIgnored(propertyFlags));
		m |= ignoreFlag;
	}
	else
	{
		jassert(value != 0);
		Bitmask shifted = (Bitmask)value << bitShift;
		m |= shifted;
	}
}

void ComplexGroupManager::Layer::clearBits(Bitmask& m) const
{
	const Bitmask inverted = ~filter;
	const Bitmask invertedIgnore = ~ignoreFlag;
	m &= inverted;
	m &= invertedIgnore;
}

void ComplexGroupManager::rebuildGroups()
{
	groups.clear();

	std::vector<std::vector<ValueWithFilter>> allMasks;

	for(auto l_: layers)
	{
		auto& l = *l_;

		if(Helpers::shouldBeCached(l.propertyFlags))
		{
			std::vector<ValueWithFilter> thisMask;

			for(int i = 0; i < l.numItems; i++)
			{
				ValueWithFilter vf;
				l.setValueFilter(vf, i+1);
				thisMask.push_back(vf);
			}

			allMasks.push_back(thisMask);
		}
	}

	int numRequired = 1;

	for(const auto& m: allMasks)
		numRequired *= (int)m.size();

	groups.ensureStorageAllocated(numRequired);

	if(allMasks.empty())
	{
		jassert(numRequired == 1);
		groups.add(new SampleType::NoteContainer());
	}
	else if (allMasks.size() == 1)
	{
		for(auto& m: allMasks[0])
		{
			auto n = new SampleType::NoteContainer();
			m.dump("GROUP with value filter");
				
			n->setValueFilter(m);
			groups.add(n);
		}
	}
	else
	{
		// support multiple cached groups soon...
		jassertfalse;
	}

	refreshCache();
}

void ComplexGroupManager::refreshCache()
{
	jassert(soundList != nullptr);

	currentFilters.clear();

	if(groups.size() == 1)
	{
		groups[0]->rebuild(*soundList);
	}
	else
	{
		for(auto& g: groups)
		{
			ReferenceCountedArray<SynthesiserSound> tempList;

			auto vf = g->getValueWithFilter();

			for(auto s: *soundList)
			{
				auto typed = dynamic_cast<SynthSoundWithBitmask*>(s);

				if(vf.matches(typed->getBitmask()))
					tempList.add(s);
			}

			g->rebuild(tempList);
		}
	}

	activeDelayLayers.clear();

	for(auto l: layers)
		l->onCacheRebuild(*this);

	resetPlayingState();
}
} // namespace hise
