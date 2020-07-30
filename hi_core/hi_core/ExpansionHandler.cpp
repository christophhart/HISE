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



namespace hise {
using namespace juce;



void ExpansionHandler::Helpers::createFrontendLayoutWithExpansionEditing(FloatingTile* root, bool putInTabWithMainInterface)
{
#if HI_ENABLE_EXPANSION_EDITING
	FloatingInterfaceBuilder ib(root);

	int mainTabs = 0;
	int expansionRoot;

	if (putInTabWithMainInterface)
	{
		ib.setNewContentType<FloatingTabComponent>(mainTabs);
		auto interface = ib.addChild<InterfaceContentPanel>(mainTabs);
		expansionRoot = ib.addChild<HorizontalTile>(mainTabs);
		ib.setCustomName(interface, "Interface");
	}
	else
	{
		ib.setNewContentType<HorizontalTile>(mainTabs);
		expansionRoot = mainTabs;
	}

	auto topBar = ib.addChild<VerticalTile>(expansionRoot);

	ib.addChild<ExpansionEditBar>(topBar);
	ib.addChild<TooltipPanel>(topBar);

	ib.getPanel(topBar)->getLayoutData().setCurrentSize(28.0);

	ib.setDynamic(topBar, false);
	ib.setFoldable(topBar, false, { false, false });
	ib.setSizes(topBar, { -0.5, 300.0 });

	auto connector = ib.addChild<GlobalConnectorPanel<ModulatorSampler>>(expansionRoot);
	auto editor = ib.addChild<SampleEditorPanel>(expansionRoot);
	auto mapEditor = ib.addChild<SampleMapEditorPanel>(expansionRoot);

	if (putInTabWithMainInterface)
	{
		auto keyboard = ib.addChild<MidiKeyboardPanel>(expansionRoot);
		ib.getPanel(keyboard)->setCanBeFolded(true);
	}


	ib.getContent(editor)->setStyleProperty("showConnectionBar", false);
	ib.getContent(mapEditor)->setStyleProperty("showConnectionBar", false);




	ib.setCustomName(expansionRoot, "Expansion Editing");

	auto c = ib.getContent<GlobalConnectorPanel<ModulatorSampler>>(connector);

	ib.setVisibility(connector, false, {});

	if (auto s = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(root->getMainController()->getMainSynthChain()))
	{
		c->setContentWithUndo(s, 0);
	}

	ib.finalizeAndReturnRoot();

	ib.setDynamic(mainTabs, false);
	ib.setDynamic(expansionRoot, false);
#else
	root->setNewContent(GET_PANEL_NAME(InterfaceContentPanel));
#endif
}

bool ExpansionHandler::Helpers::isValidExpansion(const File& directory)
{
#if HI_ENABLE_EXPANSION_EDITING
	return Expansion::Helpers::getExpansionInfoFile(directory, Expansion::FileBased).existsAsFile() ||
		   Expansion::Helpers::getExpansionInfoFile(directory, Expansion::Intermediate).existsAsFile() ||
		   Expansion::Helpers::getExpansionInfoFile(directory, Expansion::Encrypted).existsAsFile();
#else
	return Expansion::Helpers::getExpansionInfoFile(directory, Expansion::Encrypted).existsAsFile() ||
	Expansion::Helpers::getExpansionInfoFile(directory, Expansion::FileBased).existsAsFile();
#endif
}

juce::Identifier ExpansionHandler::Helpers::getSanitizedIdentifier(const String& idToSanitize)
{
	String s = idToSanitize.replaceCharacters(".:/\\ ()", "_______");

	return Identifier(s);
}

bool ExpansionHandler::Helpers::equalJSONData(var first, var second)
{
	if (first.isObject() && second.isObject())
	{
		auto a = JSON::toString(first, true);
		auto b = JSON::toString(second, true);
		return a.compare(b) == 0;
	}

	return false;
}

juce::int64 ExpansionHandler::Helpers::getJSONHash(var obj)
{
	if (obj.isObject())
		return JSON::toString(obj, true).hashCode64();

	return -1;
}

ExpansionHandler::ExpansionHandler(MainController* mc_):
	mc(mc_),
	notifier(*this)
{
}

void ExpansionHandler::createNewExpansion(const File& expansionFolder)
{
	if (Helpers::isValidExpansion(expansionFolder))
		return;

	expansionList.add(createExpansionForFile(expansionFolder));
	notifier.sendNotification(Notifier::EventType::ExpansionCreated);
}

juce::File ExpansionHandler::getExpansionFolder() const
{
	auto f = mc->getSampleManager().getProjectHandler().getRootFolder().getChildFile("Expansions");

	if (!f.isDirectory())
		f.createDirectory();

#if JUCE_WINDOWS
	auto linkFile = f.getChildFile("LinkWindows");
#else
	auto linkFile = f.getChildFile("LinkOSX");
#endif

	if (linkFile.existsAsFile())
		return File(linkFile.loadFileAsString());
	else
		return f;
}

void ExpansionHandler::createAvailableExpansions()
{
	Array<File> folders;

	getExpansionFolder().findChildFiles(folders, File::findDirectories, false);

    OwnedArray<Expansion> newList;
    
	bool didSomething = false;

	for (auto f : folders)
	{
		bool exists = false;

		//Check if expansion is already in expansionList
		for (auto e : expansionList)
		{
			if (e->getRootFolder() == f)
			{
				exists = true;
				break;
			}
		}

		if (exists)
			continue;

		if (Helpers::isValidExpansion(f))
		{
			auto e = createExpansionForFile(f);

			if (e != nullptr && !uninitialisedExpansions.contains(e))
			{
				expansionList.add(e);
				didSomething = true;
			}
		}
	}
    
	if (didSomething)
	{
		Expansion::Sorter s;
		expansionList.sort(s, true);

		notifier.sendNotification(Notifier::EventType::ExpansionCreated);
	}
}

hise::Expansion* ExpansionHandler::createExpansionForFile(const File& f)
{
	Expansion* e = nullptr;

#if HISE_USE_CUSTOM_EXPANSION_TYPE
	e = createCustomExpansion(f);
#else

	// Call setExpansionType with the class that you want to create
	// before any expansion is created!
	jassert(expansionCreateFunction);

	if (expansionCreateFunction)
		e = expansionCreateFunction(f);
#endif

	if (e != nullptr)
	{
		auto r = e->initialise();

		if (r.failed())
		{
			uninitialisedExpansions.add(e);
			setErrorMessage(r.getErrorMessage(), false);
		}
	}
		
	return e;
}

var ExpansionHandler::getListOfAvailableExpansions() const
{
	Array<var> ar;

	for (auto e : expansionList)
	{
		ar.add(e->getProperty(ExpansionIds::Name));
	}

	return var(ar);
}



const juce::OwnedArray<hise::Expansion>& ExpansionHandler::getListOfUnavailableExpansions() const
{
	return uninitialisedExpansions;
}

bool ExpansionHandler::setCurrentExpansion(const String& expansionName)
{
	currentExpansion = nullptr;

	if (expansionName.isEmpty())
	{
		notifier.sendNotification(Notifier::EventType::ExpansionLoaded);
		return true;
	}

	for (auto e : expansionList)
	{
		if (e->getProperty(ExpansionIds::Name) == expansionName)
		{
			setCurrentExpansion(e);
			return true;
		}
	}

	return false;
}

void ExpansionHandler::setCurrentExpansion(Expansion* e)
{
	if (currentExpansion != e)
	{
		currentExpansion = e;
		notifier.sendNotification(Notifier::EventType::ExpansionLoaded);
	}
}

PooledAudioFile ExpansionHandler::loadAudioFileReference(const PoolReference& sampleId)
{
	AudioSampleBufferPool* pool = nullptr;
	getPoolForReferenceString(sampleId, &pool);
	return pool->loadFromReference(sampleId, PoolHelpers::LoadAndCacheWeak);
}

double ExpansionHandler::getSampleRateForFileReference(const PoolReference& sampleId)
{
	return (double)getMetadata(sampleId).getProperty(MetadataIDs::SampleRate, 0.0);
}

const var ExpansionHandler::getMetadata(const PoolReference& sampleId)
{
	AudioSampleBufferPool* pool = nullptr;
	getPoolForReferenceString(sampleId, &pool);

	return pool->getAdditionalData(sampleId);
}

hise::PooledSampleMap ExpansionHandler::loadSampleMap(const PoolReference& sampleMapId)
{
	SampleMapPool* pool = nullptr;
	getPoolForReferenceString(sampleMapId, &pool);
	return pool->loadFromReference(sampleMapId, PoolHelpers::LoadAndCacheWeak);
}

hise::Expansion* ExpansionHandler::getExpansionForWildcardReference(const String& poolReferenceString) const
{
	if (!isEnabled())
		return nullptr;

	auto id = Expansion::Helpers::getExpansionIdFromReference(poolReferenceString);

	if (id.isNotEmpty())
	{
		for (auto e : expansionList)
		{
			if (e->getProperty(ExpansionIds::Name) == id)
				return e;
		}

		return nullptr;
	}

	return nullptr;
}

void ExpansionHandler::clearPools()
{
	for (auto e : expansionList)
	{
		e->pool->clear();
	}

}

FileHandlerBase* ExpansionHandler::getFileHandler(MainController* mc_)
{
    return &mc_->getCurrentFileHandler();
}

hise::PooledImage ExpansionHandler::loadImageReference(const PoolReference& imageId, PoolHelpers::LoadingType loadingType /*= PoolHelpers::LoadAndCacheWeak*/)
{
	ImagePool* pool = nullptr;
	getPoolForReferenceString(imageId, &pool);
	return pool->loadFromReference(imageId, loadingType);
}

PoolCollection* ExpansionHandler::getCurrentPoolCollection()
    {
        return mc->getCurrentFileHandler().pool;
    }


hise::Expansion::ExpansionType Expansion::getExpansionTypeFromFolder(const File& f)
{
	if (Helpers::getExpansionInfoFile(f, Encrypted).existsAsFile())
		return Encrypted;
	if (Helpers::getExpansionInfoFile(f, Intermediate).existsAsFile())
		return Intermediate;
	if (Helpers::getExpansionInfoFile(f, FileBased).existsAsFile())
		return FileBased;

	jassertfalse;
	return numExpansionType;
}

hise::PoolReference Expansion::createReferenceForFile(const String& relativePath, FileHandlerBase::SubDirectories fileType)
{
	auto directory = getSubDirectory(fileType);

	auto p = directory.getChildFile(relativePath);

	return PoolReference(getMainController(), p.getFullPathName(), fileType);
}



PooledAudioFile Expansion::loadAudioFile(const PoolReference& audioFileId)
{
	jassert(Helpers::getExpansionIdFromReference(audioFileId.getReferenceString()).isNotEmpty());

	return pool->getAudioSampleBufferPool().loadFromReference(audioFileId, PoolHelpers::LoadAndCacheWeak);
}

PooledImage Expansion::loadImageFile(const PoolReference& imageId)
{
	jassert(Helpers::getExpansionIdFromReference(imageId.getReferenceString()).isNotEmpty());

	return pool->getImagePool().loadFromReference(imageId, PoolHelpers::LoadAndCacheWeak);
}

#if 0
juce::String Expansion::getSampleMapReference(const String& sampleMapId)
{
	if (isEncrypted)
	{
		jassertfalse;
		return {};
	}
	else
	{
		Array<File> sampleMapFiles;
		getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

		String expStart = "{EXP::" + name.get() + "}";

		Array<var> list;

		for (auto f : sampleMapFiles)
		{
			auto thisId = f.getFileNameWithoutExtension();

			if (thisId == sampleMapId)
				return expStart + thisId;
		}

		return {};
	}
}
#endif

PooledAdditionalData Expansion::loadAdditionalData(const String& relativePath)
{
	auto ref = createReferenceForFile(relativePath, FileHandlerBase::AdditionalSourceCode);

	return pool->getPool<AdditionalDataReference>()->loadFromReference(ref, PoolHelpers::LoadAndCacheWeak);
}


void Expansion::saveExpansionInfoFile()
{
	if (Helpers::getExpansionInfoFile(root, Intermediate).existsAsFile() ||
		Helpers::getExpansionInfoFile(root, Encrypted).existsAsFile() || 
		!root.isDirectory())
		return;

	auto file = Helpers::getExpansionInfoFile(root, FileBased);

	file.replaceWithText(data->v.toXmlString());
}

juce::String Expansion::Helpers::getExpansionIdFromReference(const String& referenceId)
{


	static const String expStart = "{EXP::";

	if (!referenceId.startsWith(expStart))
		return {};

	static const String regexp = R"(^\{EXP::(.*)\})";

	auto matches = RegexFunctions::getFirstMatch(regexp, referenceId);

	if (matches.size() == 2)
	{
		return matches[1];
	}

	return {};
}

var Expansion::Data::toPropertyObject() const
{
	return ValueTreeConverters::convertValueTreeToDynamicObject(v);
}

}
