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

LookupTableProcessor::LookupTableProcessor()
{

}

LookupTableProcessor::~LookupTableProcessor()
{
	
}

void LookupTableProcessor::addTableChangeListener(SafeChangeListener *listener)
{
	tableChangeBroadcaster.addChangeListener(listener);
}

void LookupTableProcessor::removeTableChangeListener(SafeChangeListener *listener)
{
	tableChangeBroadcaster.removeChangeListener(listener);
}

void LookupTableProcessor::sendTableIndexChangeMessage(bool sendSynchronous, Table *table, float tableIndex)
{
	ScopedLock sl(tableChangeBroadcaster.lock);

	tableChangeBroadcaster.table = table;
	tableChangeBroadcaster.tableIndex = tableIndex;

	if (sendSynchronous) tableChangeBroadcaster.sendSynchronousChangeMessage();
    else tableChangeBroadcaster.sendAllocationFreeChangeMessage();
}

FactoryType::FactoryType(Processor *owner_) :
owner(owner_),
baseClassCalled(false),
constrainer(nullptr)
{

}

FactoryType::~FactoryType()
{
	constrainer = nullptr;
	ownedConstrainer = nullptr;
}

int FactoryType::fillPopupMenu(PopupMenu &m, int startIndex /*= 1*/)
{
	Array<ProcessorEntry> types = getAllowedTypes();

	int index = startIndex;

	for (int i = 0; i < types.size(); i++)
	{
		m.addItem(i + startIndex, types[i].name);

		index++;
	}

	return index;
}

Identifier FactoryType::getTypeNameFromPopupMenuResult(int resultFromPopupMenu)
{
	jassert(resultFromPopupMenu > 0);

	Array<ProcessorEntry> types = getAllowedTypes();

	return types[resultFromPopupMenu - 1].type;
}

String FactoryType::getNameFromPopupMenuResult(int resultFromPopupMenu)
{
	jassert(resultFromPopupMenu > 0);

	Array<ProcessorEntry> types = getAllowedTypes();

	return types[resultFromPopupMenu - 1].name;
}

int FactoryType::getProcessorTypeIndex(const Identifier &typeName) const
{
	Array<ProcessorEntry> entries = getTypeNames();

	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].type == typeName) return i;
	}

	return -1;
}

int FactoryType::getNumProcessors()
{
	return getAllowedTypes().size();
}

bool FactoryType::allowType(const Identifier &typeName) const
{
	baseClassCalled = true;

	bool isConstrained = (constrainer != nullptr) && !constrainer->allowType(typeName);

	if (isConstrained) return false;

	Array<ProcessorEntry> entries = getTypeNames();

	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].type == typeName) return true;
	}

	return false;
}

Array<FactoryType::ProcessorEntry> FactoryType::getAllowedTypes()
{
	Array<ProcessorEntry> allTypes = getTypeNames();

	Array<ProcessorEntry> allowedTypes;

	for (int i = 0; i < allTypes.size(); i++)
	{
		if (allowType(allTypes[i].type)) allowedTypes.add(allTypes[i]);

		// You have to call the base class' allowType!!!
		jassert(baseClassCalled);

		baseClassCalled = false;
	};
	return allowedTypes;
}

void FactoryType::setConstrainer(Constrainer *newConstrainer, bool ownConstrainer /*= true*/)
{
	constrainer = newConstrainer;

	if (ownConstrainer)
	{
		ownedConstrainer = newConstrainer;
	}
}

FactoryType::Constrainer * FactoryType::getConstrainer()
{
	return ownedConstrainer.get() != nullptr ? ownedConstrainer.get() : constrainer;
}

FactoryType::ProcessorEntry::ProcessorEntry(const Identifier t, const String &n) :
type(t),
name(n)
{

}

AudioSampleProcessor::~AudioSampleProcessor()
{
	data = nullptr;
}

void AudioSampleProcessor::saveToValueTree(ValueTree &v) const
{
	const String fileName = data->ref.getReferenceString();

	v.setProperty("FileName", fileName, nullptr);

	v.setProperty("min", sampleRange.getStart(), nullptr);
	v.setProperty("max", sampleRange.getEnd(), nullptr);

	v.setProperty("loopStart", loopRange.getStart(), nullptr);
	v.setProperty("loopEnd", loopRange.getEnd(), nullptr);
}

void AudioSampleProcessor::restoreFromValueTree(const ValueTree &v)
{
	const String savedFileName = v.getProperty("FileName", "");

	setLoadedFile(savedFileName, true);

	Range<int> range = Range<int>(v.getProperty("min", 0), v.getProperty("max", 0));

	setRange(range);
}


void AudioSampleProcessor::setLoopFromMetadata(const var& metadata)
{
	if (metadata.getProperty(MetadataIDs::LoopEnabled, false))
	{
		loopRange = Range<int>((int)metadata.getProperty(MetadataIDs::LoopStart, 0), (int)metadata.getProperty(MetadataIDs::LoopEnd, 0) + 1); // add 1 because of the offset
		sampleRange.setEnd(loopRange.getEnd());
		setUseLoop(true);
	}
	else
	{
		loopRange = {};
		setUseLoop(false);
	}
}

AudioSampleProcessor::AudioSampleProcessor(Processor *p) :
length(0),
sampleRateOfLoadedFile(-1.0)
{
	// A AudioSampleProcessor must be derived from Processor!
	jassert(p != nullptr);

	mc = p->getMainController();
	data = new PoolEntry<AudioSampleBuffer>(PoolReference());
}



int AudioSampleProcessor::getConstrainedLoopValue(String metadata)
{
	return jlimit<int>(sampleRange.getStart(), sampleRange.getEnd(), metadata.getIntValue());
}

} // namespace hise
