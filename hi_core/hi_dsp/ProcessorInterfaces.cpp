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

ProcessorWithExternalData::ProcessorWithExternalData(MainController* mc):
	mc_internal(mc)
{}

ProcessorWithExternalData::~ProcessorWithExternalData()
{}

bool ProcessorWithExternalData::removeDataObject(ExternalData::DataType t, int index)
{
	return true;
}

void ProcessorWithExternalData::linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex)
{
	Random r;
	Colour c((uint32)r.nextInt());

	SharedReference d1(type, srcIndex, c);
	SharedReference d2(type, dstIndex, c);

	if(auto ped = dynamic_cast<ProcessorWithExternalData*>(&src))
	{
		ped->sharedReferences.addIfNotAlreadyThere(d1);
		sharedReferences.addIfNotAlreadyThere(d2);

		referenceShared(type, dstIndex);
	}
	else
	{
		jassertfalse;
	}
        
}

FilterDataObject* ProcessorWithExternalData::getFilterData(int index)
{
	jassertfalse; // soon...
	return nullptr;
}

Colour ProcessorWithExternalData::getSharedReferenceColour(ExternalData::DataType t, int index) const
{
	SharedReference d(t, index, Colours::transparentBlack);

	for (const auto& r : sharedReferences)
	{
		if (d == r)
			return r.c;
	}

	return Colours::transparentBlack;
}

ComplexDataUIBase* ProcessorWithExternalData::createAndInit(ExternalData::DataType t)
{
	ComplexDataUIBase* d;

	d = ExternalData::create(t);
		
	if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(d))
		af->setProvider(new hise::PooledAudioFileDataProvider(mc_internal));

	d->setGlobalUIUpdater(getUpdater());
	d->setUndoManager(getUndoManager());

	return d;
}

void ProcessorWithExternalData::referenceShared(ExternalData::DataType type, int index)
{
	jassertfalse;
}

ProcessorWithExternalData::SharedReference::SharedReference(ExternalData::DataType type_, int index_, Colour c_):
	type(type_),
	index(index_),
	c(c_)
{}

bool ProcessorWithExternalData::SharedReference::operator==(const SharedReference& other) const
{
	return index == other.index && type == other.type;
}

PooledUIUpdater* ProcessorWithExternalData::getUpdater()
{
	return mc_internal->getGlobalUIUpdater();
}

UndoManager* ProcessorWithExternalData::getUndoManager()
{
	return mc_internal->getControlUndoManager();
}

ProcessorWithSingleStaticExternalData::ProcessorWithSingleStaticExternalData(MainController* mc,
	ExternalData::DataType t, int numObjects):
	ProcessorWithExternalData(mc),
	dataType(t)
{
	for (int i = 0; i < numObjects; i++)
	{
		ownedObjects.add(createAndInit(t));
	}
}

ProcessorWithSingleStaticExternalData::~ProcessorWithSingleStaticExternalData()
{}

Table* ProcessorWithSingleStaticExternalData::getTable(int index)
{ 
	return static_cast<Table*>(getWithoutCreating(ExternalData::DataType::Table, index));
}

SliderPackData* ProcessorWithSingleStaticExternalData::getSliderPack(int index)
{
	return static_cast<SliderPackData*>(getWithoutCreating(ExternalData::DataType::SliderPack, index));
}

MultiChannelAudioBuffer* ProcessorWithSingleStaticExternalData::getAudioFile(int index)
{
	return static_cast<MultiChannelAudioBuffer*>(getWithoutCreating(ExternalData::DataType::AudioFile, index));
}

SimpleRingBuffer* ProcessorWithSingleStaticExternalData::getDisplayBuffer(int index)
{
	return static_cast<SimpleRingBuffer*>(getWithoutCreating(ExternalData::DataType::DisplayBuffer, index));
}

int ProcessorWithSingleStaticExternalData::getNumDataObjects(ExternalData::DataType t) const
{
	return t == dataType ? ownedObjects.size() : 0;
}

void ProcessorWithSingleStaticExternalData::linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex,
	int dstIndex)
{
	jassert(type == dataType);
        
	if(isPositiveAndBelow(srcIndex, src.getNumDataObjects(dataType)))
	{
		ComplexDataUIBase::Ptr old = getComplexBaseType(type, dstIndex);
		auto otherData = src.getComplexBaseType(type, srcIndex);
		ownedObjects.set(dstIndex, otherData);
		ProcessorWithExternalData::linkTo(type, src, srcIndex, dstIndex);
	}
}

ComplexDataUIBase* ProcessorWithSingleStaticExternalData::getWithoutCreating(ExternalData::DataType requiredType,
	int index) const
{
	if (dataType == requiredType && isPositiveAndBelow(index, ownedObjects.size()))
	{
		return ownedObjects[index].get();
	}

	return nullptr;
}

ProcessorWithStaticExternalData::ProcessorWithStaticExternalData(MainController* mc, int numTables, int numSliderPacks,
	int numAudioFiles, int numDisplayBuffers):
	ProcessorWithExternalData(mc)
{
	for (int i = 0; i < numTables; i++)
		tables.add(static_cast<Table*>(createAndInit(ExternalData::DataType::Table)));

	for (int i = 0; i < numSliderPacks; i++)
		sliderPacks.add(static_cast<SliderPackData*>(createAndInit(ExternalData::DataType::SliderPack)));

	for (int i = 0; i < numAudioFiles; i++)
		audioFiles.add(static_cast<MultiChannelAudioBuffer*>(createAndInit(ExternalData::DataType::AudioFile)));

	for (int i = 0; i < numDisplayBuffers; i++)
		displayBuffers.add(static_cast<SimpleRingBuffer*>(createAndInit(ExternalData::DataType::DisplayBuffer)));
}

int ProcessorWithStaticExternalData::getNumDataObjects(ExternalData::DataType t) const
{
	switch (t)
	{
	case ExternalData::DataType::Table: return tables.size();
	case ExternalData::DataType::SliderPack: return sliderPacks.size();
	case ExternalData::DataType::AudioFile: return audioFiles.size();
	case ExternalData::DataType::DisplayBuffer: return displayBuffers.size();
	default: return 0;
	}
}

Table* ProcessorWithStaticExternalData::getTable(int index)
{
	if(isPositiveAndBelow(index, tables.size()))
		return tables[index].get();

	return nullptr;
}

SliderPackData* ProcessorWithStaticExternalData::getSliderPack(int index)
{
	if (isPositiveAndBelow(index, sliderPacks.size()))
		return sliderPacks[index].get();

	return nullptr;
}

MultiChannelAudioBuffer* ProcessorWithStaticExternalData::getAudioFile(int index)
{
	if (isPositiveAndBelow(index, audioFiles.size()))
		return audioFiles[index].get();

	return nullptr;
}

SimpleRingBuffer* ProcessorWithStaticExternalData::getDisplayBuffer(int index)
{
	if (isPositiveAndBelow(index, displayBuffers.size()))
		return displayBuffers[index].get();

	return nullptr;
}

SampleLookupTable* ProcessorWithStaticExternalData::getTableUnchecked(int index)
{
	return static_cast<SampleLookupTable*>(*(tables.begin() + index));
}

const SampleLookupTable* ProcessorWithStaticExternalData::getTableUnchecked(int index) const
{
	return static_cast<SampleLookupTable*>(*(tables.begin() + index));
}

SliderPackData* ProcessorWithStaticExternalData::getSliderPackDataUnchecked(int index)
{
	return *(sliderPacks.begin() + index);
}

const SliderPackData* ProcessorWithStaticExternalData::getSliderPackDataUnchecked(int index) const
{
	return *(sliderPacks.begin() + index);
}

MultiChannelAudioBuffer* ProcessorWithStaticExternalData::getAudioFileUnchecked(int index)
{
	return *(audioFiles.begin() + index);
}

const MultiChannelAudioBuffer* ProcessorWithStaticExternalData::getAudioFileUnchecked(int index) const
{
	return *(audioFiles.begin() + index);
}

SimpleRingBuffer* ProcessorWithStaticExternalData::getDisplayBufferUnchecked(int index)
{
	return *(displayBuffers.begin() + index);
}

const SimpleRingBuffer* ProcessorWithStaticExternalData::getDisplayBufferUnchecked(int index) const
{
	return *(displayBuffers.begin() + index);
}

void ProcessorWithStaticExternalData::linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex,
	int dstIndex)
{
	if(isPositiveAndBelow(dstIndex, getNumDataObjects(type)))
	{
		ComplexDataUIBase::Ptr old = getComplexBaseType(type, dstIndex);
            
		auto externalData = src.getComplexBaseType(type, srcIndex);

		switch(type)
		{
		case ExternalData::DataType::Table:
			tables.set(dstIndex, dynamic_cast<Table*>(externalData));
			break;
		case ExternalData::DataType::SliderPack:
			sliderPacks.set(dstIndex, dynamic_cast<SliderPackData*>(externalData));
			break;
		case ExternalData::DataType::AudioFile:
			audioFiles.set(dstIndex, dynamic_cast<MultiChannelAudioBuffer*>(externalData));
			break;
		case ExternalData::DataType::DisplayBuffer:
			displayBuffers.set(dstIndex, dynamic_cast<SimpleRingBuffer*>(externalData));
			break;
		default:
			jassertfalse;
			break;
		}
            
		ProcessorWithExternalData::linkTo(type, src, srcIndex, dstIndex);
	}
}

ProcessorWithDynamicExternalData::ProcessorWithDynamicExternalData(MainController* mc):
	ProcessorWithExternalData(mc)
{
}

ProcessorWithDynamicExternalData::~ProcessorWithDynamicExternalData()
{}

SliderPackData* ProcessorWithDynamicExternalData::getSliderPack(int index)
{
	if (auto d = sliderPacks[index])
		return d.get();

	auto s = createAndInit(snex::ExternalData::DataType::SliderPack);

	sliderPacks.set(index, static_cast<SliderPackData*>(s));
	return sliderPacks[index].get();
}

Table* ProcessorWithDynamicExternalData::getTable(int index)
{
	if (auto d = tables[index])
		return d.get();

	auto s = createAndInit(snex::ExternalData::DataType::Table);

	tables.set(index, static_cast<Table*>(s));
	return tables[index].get();
}

MultiChannelAudioBuffer* ProcessorWithDynamicExternalData::getAudioFile(int index)
{
	if (auto d = audioFiles[index])
		return d.get();

	auto s = createAndInit(snex::ExternalData::DataType::AudioFile);

	audioFiles.set(index, static_cast<MultiChannelAudioBuffer*>(s));
	return audioFiles[index].get();
}

SimpleRingBuffer* ProcessorWithDynamicExternalData::getDisplayBuffer(int index)
{
	if (auto d = ringBuffers[index])
		return d.get();

	auto s = createAndInit(snex::ExternalData::DataType::DisplayBuffer);

	ringBuffers.set(index, static_cast<SimpleRingBuffer*>(s));
	return ringBuffers[index].get();
}

FilterDataObject* ProcessorWithDynamicExternalData::getFilterData(int index)
{
	if (auto d = filterData[index])
		return d.get();

	auto s = createAndInit(snex::ExternalData::DataType::FilterCoefficients);

	filterData.set(index, static_cast<FilterDataObject*>(s));
	return filterData[index].get();
}

int ProcessorWithDynamicExternalData::getNumDataObjects(ExternalData::DataType t) const
{
	switch (t)
	{
	case ExternalData::DataType::SliderPack: return sliderPacks.size();
	case ExternalData::DataType::Table: return tables.size();
	case ExternalData::DataType::AudioFile: return audioFiles.size();
	case ExternalData::DataType::DisplayBuffer: return ringBuffers.size();
	case ExternalData::DataType::FilterCoefficients: return filterData.size();
	default: return 0;
	}
}

void ProcessorWithDynamicExternalData::saveComplexDataTypeAmounts(ValueTree& v) const
{
	using namespace scriptnode;

	ExternalData::forEachType([&v, this](ExternalData::DataType t)
	{
		auto numObjects = getNumDataObjects(t);

		if (numObjects > 0)
			v.setProperty(ExternalData::getNumIdentifier(t), numObjects, nullptr);
	});
}

void ProcessorWithDynamicExternalData::restoreComplexDataTypes(const ValueTree& v)
{
	using namespace scriptnode;

	ExternalData::forEachType([this, &v](ExternalData::DataType t)
	{
		auto numObjects = (int)v.getProperty(ExternalData::getNumIdentifier(t), 0);

		for (int i = 0; i < numObjects; i++)
			getComplexBaseType(t, i);
	});
}

void ProcessorWithDynamicExternalData::registerExternalObject(ExternalData::DataType t, int index,
	ComplexDataUIBase* obj)
{
	switch (t)
	{
	case ExternalData::DataType::Table:
		tables.set(index, dynamic_cast<Table*>(obj)); break;
	case ExternalData::DataType::SliderPack:
		sliderPacks.set(index, dynamic_cast<SliderPackData*>(obj)); break;
	case ExternalData::DataType::AudioFile:
		audioFiles.set(index, dynamic_cast<MultiChannelAudioBuffer*>(obj)); break;
	case ExternalData::DataType::DisplayBuffer:
		ringBuffers.set(index, dynamic_cast<SimpleRingBuffer*>(obj)); break;
	case ExternalData::DataType::FilterCoefficients:
		filterData.set(index, dynamic_cast<FilterDataObject*>(obj)); break;
	default: jassertfalse; break;
	}
}

void ProcessorWithDynamicExternalData::linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex,
	int dstIndex)
{
	if(isPositiveAndBelow(dstIndex, getNumDataObjects(type)))
	{
		auto ed = src.getComplexBaseType(type, srcIndex);

		registerExternalObject(type, dstIndex, ed);
		ProcessorWithExternalData::linkTo(type, src, srcIndex, dstIndex);
	}
}

LookupTableProcessor::ProcessorValueConverter::ProcessorValueConverter():
	converter(Table::getDefaultTextValue),
	processor(nullptr)
{}

LookupTableProcessor::ProcessorValueConverter::ProcessorValueConverter(const Table::ValueTextConverter& c, Processor* p):
	converter(c),
	processor(p)
{}

LookupTableProcessor::ProcessorValueConverter::ProcessorValueConverter(Processor* p):
	converter(Table::getDefaultTextValue),
	processor(p)
{}

bool LookupTableProcessor::ProcessorValueConverter::operator==(const ProcessorValueConverter& other) const
{
	return other.processor == processor;
}

LookupTableProcessor::ProcessorValueConverter::operator bool() const
{
	return processor != nullptr;
}

String LookupTableProcessor::ProcessorValueConverter::getDefaultTextValue(float input)
{
	if (processor.get())
		return converter(input);
	else
		return Table::getDefaultTextValue(input);
}

void LookupTableProcessor::addYValueConverter(const Table::ValueTextConverter& converter, Processor* p)
{
	if (p == dynamic_cast<Processor*>(this))
		defaultYConverter = converter;
	else
	{
		for (int i = 0; i < yConverters.size(); i++)
		{
			auto thisP = yConverters[i]->processor.get();

			if (thisP == nullptr || thisP == p)
				yConverters.remove(i--);
		}

		yConverters.add(new ProcessorValueConverter(converter, p ));
	}

	updateYConverters();
}

void LookupTableProcessor::refreshYConvertersAfterRemoval()
{
	for (int i = 0; i < yConverters.size(); i++)
	{
		auto thisP = yConverters[i]->processor.get();

		if (thisP == nullptr)
			yConverters.remove(i--);
	}

	updateYConverters();
}

SampleLookupTable* LookupTableProcessor::getTableUnchecked(int index)
{
	return static_cast<SampleLookupTable*>(ownedObjects.getUnchecked(index).get());
}

const SampleLookupTable* LookupTableProcessor::getTableUnchecked(int index) const
{
	return static_cast<const SampleLookupTable*>(ownedObjects.getUnchecked(index).get());
}

void LookupTableProcessor::updateYConverters()
{
	const auto cToUse = yConverters.size() == 1 ? yConverters.getFirst()->converter : defaultYConverter;

	for (int i = 0; i < getNumDataObjects(snex::ExternalData::DataType::Table); i++)
		getTable(i)->setYTextConverterRaw(cToUse);
}

SliderPackProcessor::SliderPackProcessor(MainController* mc, int numSliderPacks):
	ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::SliderPack, numSliderPacks)
{}

SliderPackData* SliderPackProcessor::getSliderPackUnchecked(int index)
{
	return static_cast<SliderPackData*>(ownedObjects.getUnchecked(index).get());
		
}

const SliderPackData* SliderPackProcessor::getSliderPackUnchecked(int index) const
{
	return static_cast<const SliderPackData*>(ownedObjects.getUnchecked(index).get());
}

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

AudioSampleProcessor::AudioSampleProcessor(MainController* mc):
	ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::AudioFile, 1)
{
	currentPool = &mc->getActiveFileHandler()->pool->getAudioSampleBufferPool();
}

String AudioSampleProcessor::getFileName() const
{ return getBuffer().toBase64String(); }

double AudioSampleProcessor::getSampleRateForLoadedFile() const
{ return getBuffer().sampleRate; }

AudioSampleBuffer& AudioSampleProcessor::getAudioSampleBuffer()
{ return getBuffer().getBuffer(); }

const AudioSampleBuffer& AudioSampleProcessor::getAudioSampleBuffer() const
{ return getBuffer().getBuffer(); }

MultiChannelAudioBuffer& AudioSampleProcessor::getBuffer()
{ return *getAudioFileUnchecked(0); }

const MultiChannelAudioBuffer& AudioSampleProcessor::getBuffer() const
{ return *getAudioFileUnchecked(0); }

MultiChannelAudioBuffer* AudioSampleProcessor::getAudioFileUnchecked(int index)
{
	return static_cast<MultiChannelAudioBuffer*>(ownedObjects.getUnchecked(index).get());
}

const MultiChannelAudioBuffer* AudioSampleProcessor::getAudioFileUnchecked(int index) const
{
	return static_cast<const MultiChannelAudioBuffer*>(ownedObjects.getUnchecked(index).get());
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

Chain::Handler::Listener::~Listener()
{}

Chain::Handler::~Handler()
{}

void Chain::Handler::moveProcessor(Processor*, int)
{}

void Chain::Handler::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void Chain::Handler::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void Chain::Handler::addPostEventListener(Listener* l)
{
	postEventlisteners.addIfNotAlreadyThere(l);
}

void Chain::Handler::removePostEventListener(Listener* l)
{
	postEventlisteners.removeAllInstancesOf(l);
}

void Chain::Handler::notifyListeners(Listener::EventType t, Processor* p)
{
	ScopedLock sl(listeners.getLock());

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->processorChanged(t, p);
	}
}

void Chain::Handler::notifyPostEventListeners(Listener::EventType t, Processor* p)
{
	ScopedLock sl(postEventlisteners.getLock());

	for (auto l : postEventlisteners)
	{
		if (l != nullptr)
			l->processorChanged(t, p);
	}
}

Chain::~Chain()
{}

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
