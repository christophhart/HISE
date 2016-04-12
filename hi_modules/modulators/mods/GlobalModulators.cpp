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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

GlobalModulator::GlobalModulator(MainController *mc):
originalModulator(nullptr),
connectedContainer(nullptr),
useTable(false)
{
	table = new MidiTable();

	ModulatorSynthChain *chain = mc->getMainSynthChain();

	Processor::Iterator<GlobalModulatorContainer> iter(chain, false);

	GlobalModulatorContainer *c;

	while ((c = iter.getNextProcessor()) != nullptr)
	{
		c->addChangeListenerToHandler(this);
	}

}

GlobalModulator::~GlobalModulator()
{
	table = nullptr;

	

}

Modulator * GlobalModulator::getOriginalModulator()
{
	return dynamic_cast<Modulator*>(originalModulator.get());
}

const Modulator * GlobalModulator::getOriginalModulator() const
{
	return dynamic_cast<const Modulator*>(originalModulator.get());
}

GlobalModulatorContainer * GlobalModulator::getConnectedContainer()
{
	return dynamic_cast<GlobalModulatorContainer*>(connectedContainer.get());
}

const GlobalModulatorContainer * GlobalModulator::getConnectedContainer() const
{
	return dynamic_cast<const GlobalModulatorContainer*>(connectedContainer.get());
}

StringArray GlobalModulator::getListOfAllModulatorsWithType()
{
	StringArray list;

	ModulatorSynthChain *rootChain = dynamic_cast<Modulator*>(this)->getMainController()->getMainSynthChain();

	Processor::Iterator<Processor> iter(rootChain, false);

	Processor *p;

	while ((p = iter.getNextProcessor()) != nullptr)
	{
		if (p == dynamic_cast<Processor*>(this)) return list; // Don't search beyond the GlobalModulator itself...

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
			case Envelope: matches = dynamic_cast<EnvelopeModulator*>(chain->getHandler()->getProcessor(i)) != nullptr; break;
			}

			if (matches)
			{
				list.add(getItemEntryFor(c, chain->getHandler()->getProcessor(i)));
			}
		}
	}

	return list;
}

void GlobalModulator::connectToGlobalModulator(const String &itemEntry)
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

				originalModulator = ProcessorHelpers::getFirstProcessorWithName(c, modulatorId);
			}

		}

		jassert(isConnected());
	}
}

String GlobalModulator::getItemEntryFor(const GlobalModulatorContainer *c, const Processor *p)
{
	if (c == nullptr || p == nullptr) return String::empty;

	return c->getId() + ":" + p->getId();
}

void GlobalModulator::saveToValueTree(ValueTree &v) const
{
	v.setProperty("UseTable", useTable, nullptr);

	saveTable(table, "TableData");

	v.setProperty("Connection", getItemEntryFor(getConnectedContainer(), getOriginalModulator()), nullptr);

}

void GlobalModulator::loadFromValueTree(const ValueTree &v)
{
	useTable = v.getProperty("UseTable");

	loadTable(table, "TableData");

	connectToGlobalModulator(v.getProperty("Connection"));
}

void GlobalModulator::removeFromAllContainers()
{
	ModulatorSynthChain *chain = dynamic_cast<Processor*>(this)->getMainController()->getMainSynthChain();

	if (chain != nullptr)
	{
		Processor::Iterator<GlobalModulatorContainer> iter(chain, false);

		GlobalModulatorContainer *c;

		while ((c = iter.getNextProcessor()) != nullptr)
		{
			c->removeChangeListenerFromHandler(this);
		}

	}

	
}

GlobalVoiceStartModulator::GlobalVoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
VoiceStartModulator(mc, id, numVoices, m),
Modulation(m),
GlobalModulator(mc)
{
	parameterNames.add("UseTable");
}

GlobalVoiceStartModulator::~GlobalVoiceStartModulator()
{
	removeFromAllContainers();
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

ProcessorEditorBody * GlobalVoiceStartModulator::createEditor(BetterProcessorEditor *parentEditor)
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
	default:				jassertfalse; break;
	}
}

float GlobalVoiceStartModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

float GlobalVoiceStartModulator::calculateVoiceStartValue(const MidiMessage &m)
{
	if (isConnected())
	{
		jassert(m.isNoteOn());

		const int noteNumber = m.getNoteNumber();

		float globalValue = getConnectedContainer()->getConstantVoiceValue(getOriginalModulator(), noteNumber);

		if (useTable)
		{
			const int index = (int)((float)globalValue * 127.0f);
			globalValue = table->get(index);

			sendTableIndexChangeMessage(false, table, (float)index / 127.0f);
		}

		return globalValue;
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

ProcessorEditorBody * GlobalTimeVariantModulator::createEditor(BetterProcessorEditor *parentEditor)
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
	default:				jassertfalse; break;
	}
}

float GlobalTimeVariantModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
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

			const float inputValue = data[0];

			int i = 0;

			const int startIndex = startSample;

			while (--numSamples >= 0)
			{
				const int tableIndex = (int)(data[i++] * 127.0f);

				internalBuffer.setSample(0, startSample++, table->get(tableIndex));
			}

			setOutputValue(internalBuffer.getSample(0, startIndex));
			sendTableIndexChangeMessage(false, table, inputValue);

		}
		else
		{
			FloatVectorOperations::copy(internalBuffer.getWritePointer(0, startSample), getConnectedContainer()->getModulationValuesForModulator(getOriginalModulator(), startSample), numSamples);
			setOutputValue(internalBuffer.getSample(0, startSample));
		}

		
	}
	else
	{
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples);
		setOutputValue(1.0f);
	}
}


GlobalEnvelopeModulator::GlobalEnvelopeModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
EnvelopeModulator(mc, id, numVoices, m),
Modulation(m),
GlobalModulator(mc)
{

}

void GlobalEnvelopeModulator::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);
	loadFromValueTree(v);
}

ValueTree GlobalEnvelopeModulator::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();
	saveToValueTree(v);

	return v;
}

ProcessorEditorBody * GlobalEnvelopeModulator::createEditor(BetterProcessorEditor *parentEditor)
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
	case UseTable:			useTable = (newValue != 0.0f); break;
	default:				jassertfalse; break;
	}
}

float GlobalEnvelopeModulator::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case UseTable:			return useTable ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

void GlobalEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = polyManager.getCurrentVoice();

	if (isConnected())
	{
		if (useTable)
		{
			const float *data = getConnectedContainer()->getModulationValuesForModulator(getOriginalModulator(), startSample, voiceIndex);

			const float inputValue = data[0];

			int i = 0;

			const int startIndex = startSample;

			while (--numSamples >= 0)
			{
				const int tableIndex = (int)(data[i++] * 127.0f);

				internalBuffer.setSample(0, startSample++, table->get(tableIndex));
			}

			setOutputValue(internalBuffer.getSample(0, startIndex));
			sendTableIndexChangeMessage(false, table, inputValue);

		}
		else
		{
			FloatVectorOperations::copy(internalBuffer.getWritePointer(0, startSample), getConnectedContainer()->getModulationValuesForModulator(getOriginalModulator(), startSample, voiceIndex), numSamples);
			setOutputValue(internalBuffer.getSample(0, startSample));
		}

	}
	else
	{
		FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), 1.0f, numSamples);
		setOutputValue(1.0f);
	}


	
	/*
	if (--numSamples >= 0)
	{

		const float value = 1.0f;
		internalBuffer.setSample(0, startSample, value);
		++startSample;
		if (voiceIndex == polyManager.getLastStartedVoice()) setOutputValue(value);
	}

	while (--numSamples >= 0)
	{
		internalBuffer.setSample(0, startSample, 1.0f);
		++startSample;
	}
	*/
}

bool GlobalEnvelopeModulator::isPlaying(int voiceIndex) const
{
	if (isConnected())
	{
		return dynamic_cast<const EnvelopeModulator*>(getOriginalModulator())->isPlaying(voiceIndex);
	}
	else
	{
		return false;
	}
}
