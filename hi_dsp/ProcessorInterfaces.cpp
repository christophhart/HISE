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

using namespace snex;

LookupTableProcessor::LookupTableProcessor(MainController* mc, int numTables):
	ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::Table, numTables)
{

}

LookupTableProcessor::~LookupTableProcessor()
{
	
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
	if (currentPool != nullptr)
		currentPool->removeListener(this);
}

void AudioSampleProcessor::saveToValueTree(ValueTree &v) const
{
	
	auto fileName = getBuffer().toBase64String();
	auto sampleRange = getBuffer().getCurrentRange();
	auto loopRange = getBuffer().getLoopRange();

	v.setProperty("FileName", fileName, nullptr);

	v.setProperty("min", sampleRange.getStart(), nullptr);
	v.setProperty("max", sampleRange.getEnd(), nullptr);

	v.setProperty("loopStart",  loopRange.getStart(), nullptr);
	v.setProperty("loopEnd", loopRange.getEnd(), nullptr);
}

void AudioSampleProcessor::restoreFromValueTree(const ValueTree &v)
{
	const String savedFileName = v.getProperty("FileName", "");

	getBuffer().fromBase64String(savedFileName);

	setLoadedFile(savedFileName, true);

	auto sRange = Range<int>(v.getProperty("min", 0), v.getProperty("max", 0));
	auto lRange = Range<int>(v.getProperty("loopStart", 0), v.getProperty("loopEnd", 0));

	getBuffer().setRange(sRange);
	getBuffer().setLoopRange(lRange, dontSendNotification);
}


void AudioSampleProcessor::poolEntryReloaded(PoolReference referenceThatWasChanged)
{
	auto s = referenceThatWasChanged.getReferenceString();

	if (getBuffer().toBase64String() == s)
	{
		getBuffer().fromBase64String({});
		getBuffer().fromBase64String(s);
	}
}

int AudioSampleProcessor::getConstrainedLoopValue(String metadata)
{
	auto sr = getBuffer().getCurrentRange();
	return jlimit<int>(sr.getStart(), sr.getEnd(), metadata.getIntValue());
}

void Chain::Handler::clearAsync(Processor* parentThatShouldBeTakenOffAir)
{
	int numToClear = getNumProcessors();

	if(parentThatShouldBeTakenOffAir != nullptr)
	{
		LOCK_PROCESSING_CHAIN(parentThatShouldBeTakenOffAir);
		parentThatShouldBeTakenOffAir->setIsOnAir(false);
	}

	while (--numToClear >= 0)
	{
		if (auto pToRemove = getProcessor(0))
		{
			
			remove(pToRemove, false);
			jassert(!pToRemove->isOnAir());

			pToRemove->getMainController()->getGlobalAsyncModuleHandler().removeAsync(pToRemove, ProcessorFunction());
		}
		else
		{
			jassertfalse;
		}
	}
}

} // namespace hise
