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


File ExternalFileProcessor::getFileForGlobalReference(const String &reference, PresetPlayerHandler::FolderType type)
{
	jassert(reference.contains("{GLOBAL_FOLDER}"));

	String packageName = dynamic_cast<Processor*>(this)->getMainController()->getMainSynthChain()->getPackageName();

	if (packageName.isEmpty())
	{
		PresetHandler::showMessageWindow("Package Name not set", "Press OK to enter the package name", PresetHandler::IconType::Info);
		packageName = PresetHandler::getCustomName("Package Name");

		dynamic_cast<Processor*>(this)->getMainController()->getMainSynthChain()->setPackageName(packageName);

	}

	return File(PresetPlayerHandler::getSpecialFolder(type, packageName) + reference.fromFirstOccurrenceOf("{GLOBAL_FOLDER}", false, false));
}

File ExternalFileProcessor::getFile(const String &fileNameOrReference, PresetPlayerHandler::FolderType type)
{
	if (isReference(fileNameOrReference))
	{
		File f = getFileForGlobalReference(fileNameOrReference, type);

		jassert(f.existsAsFile());

		return f;
	}
	else
	{
		File f(fileNameOrReference);

		jassert(f.existsAsFile());

		return f;
	}
}

bool ExternalFileProcessor::isReference(const String &fileNameOrReference)
{
	return fileNameOrReference.contains("{GLOBAL_FOLDER}");
}

String ExternalFileProcessor::getGlobalReferenceForFile(const String &file, PresetPlayerHandler::FolderType /*type*/ /*= PresetPlayerHandler::GlobalSampleDirectory*/)
{
	if (isReference(file))
	{
		return file;
	}
	else
	{
		File f(file);

		jassert(f.existsAsFile());

		return "{GLOBAL_FOLDER}/" + f.getFileName();
	}

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
	mc->getSampleManager().getAudioSampleBufferPool()->releasePoolData(sampleBuffer);
}

void AudioSampleProcessor::saveToValueTree(ValueTree &v) const
{
	const Processor *thisAsProcessor = dynamic_cast<const Processor*>(this);

	const String fileName = GET_PROJECT_HANDLER(const_cast<Processor*>(thisAsProcessor)).getFileReference(loadedFileName, ProjectHandler::SubDirectories::AudioFiles);

	v.setProperty("FileName", fileName, nullptr);

	v.setProperty("min", sampleRange.getStart(), nullptr);
	v.setProperty("max", sampleRange.getEnd(), nullptr);
}

void AudioSampleProcessor::restoreFromValueTree(const ValueTree &v)
{
	const String savedFileName = v.getProperty("FileName", "");

#if USE_BACKEND
	String name = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getFilePath(savedFileName, ProjectHandler::SubDirectories::AudioFiles);
#elif USE_FRONTEND
	String name = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(savedFileName);
#endif

	setLoadedFile(name, true);

	Range<int> range = Range<int>(v.getProperty("min", 0), v.getProperty("max", 0));

	if (sampleBuffer != nullptr) setRange(range);
}

void AudioSampleProcessor::changeListenerCallback(SafeChangeBroadcaster *b)
{
	AudioSampleBufferComponent *bc = dynamic_cast<AudioSampleBufferComponent*>(b);

	if (bc != nullptr)
	{
		setLoadedFile(bc->getCurrentlyLoadedFileName(), true);
		bc->setAudioSampleBuffer(sampleBuffer, loadedFileName);

		dynamic_cast<Processor*>(this)->sendSynchronousChangeMessage();
	}
	else jassertfalse;
}

AudioSampleProcessor::AudioSampleProcessor(Processor *p) :
length(0),
sampleRateOfLoadedFile(-1.0),
sampleBuffer(nullptr)
{
	// A AudioSampleProcessor must be derived from Processor!
	jassert(p != nullptr);

	mc = p->getMainController();
}
