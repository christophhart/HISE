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

GlobalModulator::GlobalModulator(MainController *mc):
	LookupTableProcessor(mc, 1),
	originalModulator(nullptr),
	connectedContainer(nullptr),
	useTable(false)
{
    referenceShared(ExternalData::DataType::Table, 0);
	

	ModulatorSynthChain *chain = mc->getMainSynthChain();

	Processor::Iterator<GlobalModulatorContainer> iter(chain, false);

	while (auto c = iter.getNextProcessor())
	{
		c->gainChain->getHandler()->addListener(this);
		watchedContainers.add(c);
	}
}

GlobalModulator::~GlobalModulator()
{
	for (auto c : watchedContainers)
	{
		if (c != nullptr)
		{
			c->gainChain->getHandler()->removeListener(this);
		}
	}

	table = nullptr;

	disconnect();

	

	
}

Modulator * GlobalModulator::getOriginalModulator()
{
	return originalModulator.get();
}

const Modulator * GlobalModulator::getOriginalModulator() const
{
	return originalModulator.get();
}

GlobalModulatorContainer * GlobalModulator::getConnectedContainer()
{
	return static_cast<GlobalModulatorContainer*>(connectedContainer.get());
}

const GlobalModulatorContainer * GlobalModulator::getConnectedContainer() const
{
	return static_cast<const GlobalModulatorContainer*>(connectedContainer.get());
}

void GlobalModulator::connectIfPending()
{
    if(pendingConnection.isNotEmpty())
    {
        auto ok = connectToGlobalModulator(pendingConnection);
        
        if(ok)
            pendingConnection = {};
    }
}

void GlobalModulator::processorChanged(EventType /*t*/, Processor* /*p*/)
{
	// Just send a regular update message to update the GUI
	dynamic_cast<Processor*>(this)->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Any);
}

bool isParent(Processor* p, Processor* possibleParent)
{
    if(p == nullptr)
        return false;
    
    if(p == possibleParent)
        return true;
    
    return isParent(p->getParentProcessor(false), possibleParent);
}

StringArray GlobalModulator::getListOfAllModulatorsWithType()
{
	StringArray list;

	ModulatorSynthChain *rootChain = dynamic_cast<Modulator*>(this)->getMainController()->getMainSynthChain();

	Processor::Iterator<Processor> iter(rootChain, true);

	Processor *p;

    auto masterEffectChain = rootChain->getChildProcessor(ModulatorSynth::EffectChain);
    auto masterGainChain = rootChain->getChildProcessor(ModulatorSynth::GainModulation);
    
    
	while ((p = iter.getNextProcessor()) != nullptr)
	{
        DBG(p->getId());
        
        // Don't search beyond the GlobalModulator itself...
		if (p == dynamic_cast<Processor*>(this))
        {
            // But only if it's not in the root FX / gain mod chain
            auto isParentOfRootFX = isParent(p, masterEffectChain);
            auto isParentOfRootGainMod = isParent(p, masterGainChain);
            
            if(!isParentOfRootFX && !isParentOfRootGainMod)
                return list;
        }
            

		GlobalModulatorContainer *c = dynamic_cast<GlobalModulatorContainer*>(p);

		if (c == nullptr) continue;

		ModulatorChain *chain = dynamic_cast<ModulatorChain*>(c->getChildProcessor(ModulatorSynth::GainModulation));

		for (int i = 0; i < chain->getHandler()->getNumProcessors(); i++)
		{
			bool matches = false;

			switch (getModulatorType())
			{
			case VoiceStart: matches = dynamic_cast<VoiceStartModulator*>(chain->getHandler()->getProcessor(i)) != nullptr; break;
			case TimeVariant: matches = dynamic_cast<TimeVariantModulator*>(chain->getHandler()->getProcessor(i)) != nullptr; break;
			case StaticTimeVariant: matches = dynamic_cast<TimeVariantModulator*>(chain->getHandler()->getProcessor(i)) != nullptr; break;
			case Envelope: matches = dynamic_cast<EnvelopeModulator*>(chain->getHandler()->getProcessor(i)) != nullptr; break;
            case numTypes: jassertfalse; break;
			}

			if (matches)
			{
				list.add(getItemEntryFor(c, chain->getHandler()->getProcessor(i)));
			}
		}
	}

	return list;
}

void GlobalModulator::disconnect()
{
	if (auto oltp = dynamic_cast<LookupTableProcessor*>(getOriginalModulator()))
	{
		WeakReference<Processor> target = getOriginalModulator();

		if (target->getMainController()->isBeingDeleted())
			return;

		auto f = [target]()
		{
			if (auto ltp = dynamic_cast<LookupTableProcessor*>(target.get()))
			{
				ltp->refreshYConvertersAfterRemoval();
			}
		};

		new DelayedFunctionCaller(f, 300);
	}

	originalModulator = nullptr;
	connectedContainer = nullptr;
    
#if USE_BACKEND
    if(auto asP = dynamic_cast<Processor*>(this))
    {
#if USE_OLD_PROCESSOR_DISPATCH
    asP->getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(asP, MainController::ProcessorChangeHandler::EventType::ProcessorColourChange, false);
#endif
#if USE_NEW_PROCESSOR_DISPATCH
	asP->dispatcher.setColour(Colours::black);
#endif
    }
#endif
}

bool GlobalModulator::connectToGlobalModulator(const String &itemEntry)
{
	if (itemEntry.isNotEmpty())
	{
		StringArray ids = StringArray::fromTokens(itemEntry, ":", "");

		jassert(ids.size() == 2);

		String containerId = ids[0];
		String modulatorId = ids[1];

		Processor::Iterator<GlobalModulatorContainer> iter(dynamic_cast<Processor*>(this)->getMainController()->getMainSynthChain());

		GlobalModulatorContainer *c;

		while ((c = iter.getNextProcessor()) != nullptr)
		{
			if (c->getId() == containerId)
			{
				connectedContainer = c;

				originalModulator = dynamic_cast<Modulator*>(ProcessorHelpers::getFirstProcessorWithName(c, modulatorId));

				if (auto ltp = dynamic_cast<LookupTableProcessor*>(originalModulator.get()))
				{
					ltp->addYValueConverter(defaultYConverter, dynamic_cast<Processor*>(this));
				}
			}
		}

#if USE_BACKEND
        auto asP = dynamic_cast<Processor*>(this);
		ignoreUnused(asP);
#if USE_OLD_PROCESSOR_DISPATCH
	    asP->getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(asP, MainController::ProcessorChangeHandler::EventType::ProcessorColourChange, false);
#endif
#if USE_NEW_PROCESSOR_DISPATCH
		asP->dispatcher.setColour(Colours::black);
#endif

        
#endif
        
        // return false if the connection can't be established (yet)
        return isConnected();
	}
    
    return true;
}

String GlobalModulator::getItemEntryFor(const GlobalModulatorContainer *c, const Processor *p)
{
	if (c == nullptr || p == nullptr) return String();

	return c->getId() + ":" + p->getId();
}

void GlobalModulator::saveToValueTree(ValueTree &v) const
{
	v.setProperty("UseTable", useTable, nullptr);
	v.setProperty("Inverted", inverted, nullptr);

	saveTable(table, "TableData");

	v.setProperty("Connection", getItemEntryFor(getConnectedContainer(), getOriginalModulator()), nullptr);

}

void GlobalModulator::loadFromValueTree(const ValueTree &v)
{
	useTable = v.getProperty("UseTable");
	inverted = v.getProperty("Inverted");

	loadTable(table, "TableData");

    auto id = v.getProperty("Connection").toString();
    
	if(!connectToGlobalModulator(id))
        pendingConnection = id;
    else
        pendingConnection = {};
}

GlobalVoiceStartModulator::GlobalVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
VoiceStartModulator(mc, id, numVoices, m),
Modulation(m),
GlobalModulator(mc)
{
	parameterNames.add("UseTable");
	parameterNames.add("Inverted");
	updateParameterSlots();
}

GlobalVoiceStartModulator::~GlobalVoiceStartModulator()
{
	
}

void GlobalVoiceStartModulator::restoreFromValueTree(const ValueTree &v)
{
	VoiceStartModulator::restoreFromValueTree(v);
	loadFromValueTree(v);
}

ValueTree GlobalVoiceStartModulator::exportAsValueTree() const
{
	ValueTree v = VoiceStartModulator::exportAsValueTree();
	saveToValueTree(v);
	return v;
}

ProcessorEditorBody * GlobalVoiceStartModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new GlobalModulatorEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void GlobalVoiceStartModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue != 0.0f); break;
	case Inverted:			inverted = (newValue > 0.5f); break;
	default:				jassertfalse; break;
	}
}

float GlobalVoiceStartModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	case Inverted:			return inverted ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float GlobalVoiceStartModulator::calculateVoiceStartValue(const HiseEvent &m)
{
	if (isConnected())
	{
		jassert(m.isNoteOn());

		const int noteNumber = m.getNoteNumber();

		float globalValue = getConnectedContainer()->getConstantVoiceValue(getOriginalModulator(), noteNumber);

		if (useTable)
		{
			globalValue = table->getInterpolatedValue((double)globalValue, sendNotificationAsync);
		}

		return inverted ? 1.0f - globalValue : globalValue;
	}

	return 1.0f;
}

GlobalStaticTimeVariantModulator::GlobalStaticTimeVariantModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
	VoiceStartModulator(mc, id, numVoices, m),
	Modulation(m),
	GlobalModulator(mc)
{
	parameterNames.add("UseTable");
	parameterNames.add("Inverted");
	updateParameterSlots();
}



GlobalStaticTimeVariantModulator::~GlobalStaticTimeVariantModulator()
{
}

void GlobalStaticTimeVariantModulator::restoreFromValueTree(const ValueTree &v)
{
	VoiceStartModulator::restoreFromValueTree(v);
	loadFromValueTree(v);
}

ValueTree GlobalStaticTimeVariantModulator::exportAsValueTree() const
{
	ValueTree v = VoiceStartModulator::exportAsValueTree();
	saveToValueTree(v);
	return v;
}

hise::ProcessorEditorBody * GlobalStaticTimeVariantModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new GlobalModulatorEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif

}

void GlobalStaticTimeVariantModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue > 0.5f); break;
	case Inverted:			inverted = (newValue > 0.5f); break;
	default:				jassertfalse; break;
	}
}

float GlobalStaticTimeVariantModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	case Inverted:			return inverted ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float GlobalStaticTimeVariantModulator::calculateVoiceStartValue(const HiseEvent&)
{
	if (isConnected())
	{
		float globalValue = static_cast<TimeVariantModulator*>(getOriginalModulator())->getLastConstantValue();

		if (useTable)
		{
			globalValue = table->getInterpolatedValue((double)globalValue, sendNotificationAsync);
		}

		return inverted ? 1.0f - globalValue : globalValue;
	}

	return 1.0f;
}

GlobalTimeVariantModulator::GlobalTimeVariantModulator(MainController *mc, const String &id, Modulation::Mode m) :
TimeVariantModulator(mc, id, m),
Modulation(m),
GlobalModulator(mc),
inputValue(1.0f),
currentValue(1.0f)
{
	parameterNames.add("UseTable");
	parameterNames.add("Inverted");
	updateParameterSlots();
}

void GlobalTimeVariantModulator::restoreFromValueTree(const ValueTree &v)
{
	TimeVariantModulator::restoreFromValueTree(v);
	loadFromValueTree(v);
}

ValueTree GlobalTimeVariantModulator::exportAsValueTree() const
{
	ValueTree v = TimeVariantModulator::exportAsValueTree();

	saveToValueTree(v);

	return v;
}

ProcessorEditorBody * GlobalTimeVariantModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new GlobalModulatorEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void GlobalTimeVariantModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue != 0.0f); break;
	case Inverted:			inverted = (newValue > 0.5f); break;
	default:				jassertfalse; break;
	}
}

float GlobalTimeVariantModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	case Inverted:			return inverted ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

void GlobalTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
	if (isConnected())
	{
		if (useTable)
		{
			const float *data = getConnectedContainer()->getModulationValuesForModulator(getOriginalModulator(), startSample);

            if(data != nullptr)
            {
                const float thisInputValue = data[0];
                
                int i = 0;
                
                const int startIndex = startSample;
                
                while (--numSamples >= 0)
                {
                    internalBuffer.setSample(0, startSample++, table->getInterpolatedValue(data[i++], dontSendNotification));
                }
                
				if(numSamples > 0)
					invertBuffer(startSample, numSamples);
                
                
				table->setNormalisedIndexSync(thisInputValue);
                setOutputValue(internalBuffer.getSample(0, startIndex));
                
                return;
            }
		}
		else
		{
            if(auto src = getConnectedContainer()->getModulationValuesForModulator(getOriginalModulator(), startSample))
            {
                FloatVectorOperations::copy(internalBuffer.getWritePointer(0, startSample), src, numSamples);
                invertBuffer(startSample, numSamples);
                
                setOutputValue(internalBuffer.getSample(0, startSample));
                
                return;
            }
		}
	}
	
    FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples);
    setOutputValue(1.0f);
}

void GlobalTimeVariantModulator::invertBuffer(int startSample, int numSamples)
{
	if (inverted)
	{
		float* d = internalBuffer.getWritePointer(0, startSample);
		FloatVectorOperations::multiply(d, -1.0f, numSamples);
		FloatVectorOperations::add(d, 1.0f, numSamples);
	}
}

GlobalEnvelopeModulator::GlobalEnvelopeModulator(MainController *mc, const String &id, Modulation::Mode m, int numVoices) :
	EnvelopeModulator(mc, id, numVoices, m),
	Modulation(m),
	GlobalModulator(mc)
{
	parameterNames.add("UseTable");
	parameterNames.add("Inverted");
	updateParameterSlots();
}

void GlobalEnvelopeModulator::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);
	loadFromValueTree(v);
}

juce::ValueTree GlobalEnvelopeModulator::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();
	saveToValueTree(v);
	return v;
}

hise::ProcessorEditorBody * GlobalEnvelopeModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new GlobalModulatorEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void GlobalEnvelopeModulator::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case UseTable:			useTable = (newValue > 0.5f); break;
	case Inverted:			inverted = (newValue > 0.5f); break;
	default:				jassertfalse; break;
	}
}

float GlobalEnvelopeModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	case Inverted:			return inverted ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float GlobalEnvelopeModulator::startVoice(int voiceIndex)
{
	if (isConnected())
	{
		return 0.0f;
	}
	else
	{
		active[voiceIndex] = true;
		return getInitialValue();
	}
}

void GlobalEnvelopeModulator::stopVoice(int voiceIndex)
{
	active[voiceIndex] = false;
}

void GlobalEnvelopeModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	connectIfPending();
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
}

bool GlobalEnvelopeModulator::isPlaying(int voiceIndex) const
{
	if (isConnected())
	{
		return static_cast<const EnvelopeModulator*>(getOriginalModulator())->isPlaying(voiceIndex);
	}
	else
	{
		return active[voiceIndex];
	}
}

hise::EnvelopeModulator::ModulatorState * GlobalEnvelopeModulator::createSubclassedState(int voiceIndex) const
{
	return new EnvelopeModulator::ModulatorState(voiceIndex);
}

void GlobalEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	if (isConnected())
	{
		auto voiceIndex = polyManager.getCurrentVoice();

		if (static_cast<ModulatorSynth*>(getParentProcessor(true))->isInGroup())
		{
			auto unisonoAmount = (int)getParentProcessor(true)->getParentProcessor(true)->getAttribute(ModulatorSynthGroup::UnisonoVoiceAmount);

			voiceIndex /= unisonoAmount;
		}
		
		if (useTable)
		{
			const float *data = getConnectedContainer()->getEnvelopeValuesForModulator(getOriginalModulator(), startSample, voiceIndex);

			if (data != nullptr)
			{
				const float thisInputValue = data[0];

				int i = 0;

				const int startIndex = startSample;

				while (--numSamples >= 0)
				{
					internalBuffer.setSample(0, startSample++, table->getInterpolatedValue(data[i++], dontSendNotification));
				}

#if 0
				if (numSamples > 0)
					invertBuffer(startSample, numSamples);
#endif


				table->setNormalisedIndexSync(thisInputValue);
				setOutputValue(internalBuffer.getSample(0, startIndex));

				return;
			}
		}
		else
		{
			if (auto src = getConnectedContainer()->getEnvelopeValuesForModulator(getOriginalModulator(), startSample, voiceIndex))
			{
				FloatVectorOperations::copy(internalBuffer.getWritePointer(0, startSample), src, numSamples);
				//invertBuffer(startSample, numSamples);

				setOutputValue(internalBuffer.getSample(0, startSample));

				return;
			}
		}
	}
	else
	{
		auto v = Modulation::getInitialValue();
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), v, numSamples);
		setOutputValue(v);
	}

	
}

} // namespace hise
