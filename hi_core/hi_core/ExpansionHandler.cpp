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
	return Expansion::Helpers::getExpansionInfoFile(directory, Expansion::FileBased).existsAsFile() ||
		   Expansion::Helpers::getExpansionInfoFile(directory, Expansion::Intermediate).existsAsFile() ||
		   Expansion::Helpers::getExpansionInfoFile(directory, Expansion::Encrypted).existsAsFile();
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
	setAllowedExpansions({ Expansion::FileBased, Expansion::Intermediate, Expansion::Encrypted });
}

void ExpansionHandler::createNewExpansion(const File& expansionFolder)
{
	if (Helpers::isValidExpansion(expansionFolder))
		return;

	if (auto e = createExpansionForFile(expansionFolder))
	{
		expansionList.add(e);
		notifier.sendNotification(Notifier::EventType::ExpansionCreated);
	}
}

juce::File ExpansionHandler::getExpansionFolder() const
{
#if USE_FRONTEND
	// We'll automatically use the sample folder to store the expansions
	if (FullInstrumentExpansion::isEnabled(mc))
		return FrontendHandler::getSampleLocationForCompiledPlugin();
#endif

	if (!expansionFolder.isDirectory())
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
			f = File(linkFile.loadFileAsString());
		
		if (!f.isDirectory())
			f.createDirectory();

		expansionFolder = f;
	}

	return expansionFolder;
}

bool ExpansionHandler::createAvailableExpansions()
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

		auto notification = MessageManager::getInstance()->isThisTheMessageThread() ? sendNotificationSync : sendNotificationAsync;
		notifier.sendNotification(Notifier::EventType::ExpansionCreated, notification);
	}

	return didSomething;
}

bool ExpansionHandler::forceReinitialisation()
{
	bool didSomething = false;

	for (int i = 0; i < expansionList.size(); i++)
	{
		auto e = expansionList[i];
		auto r = e->initialise();

		checkAllowedExpansions(r, e);

		if (!r.wasOk())
		{
			e = expansionList.removeAndReturn(i--);
			initialisationErrors.addIfNotAlreadyThere({ e, r });
			uninitialisedExpansions.add(e);
			didSomething = true;
		}
	}

	auto v = rebuildUnitialisedExpansions();

	auto ok = didSomething || v;

	if(ok)
		setCurrentExpansion(nullptr);

	return ok;
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

		checkAllowedExpansions(r, e);

		if (r.failed())
		{
			initialisationErrors.addIfNotAlreadyThere({ e, r });

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
		if(e != nullptr)
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
	if (currentExpansion != nullptr && expansionName.isEmpty())
	{
		currentExpansion = nullptr;
		notifier.sendNotification(Notifier::EventType::ExpansionLoaded);
		return true;
	}

	currentExpansion = nullptr;

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

void ExpansionHandler::setCurrentExpansion(Expansion* e, NotificationType notifyListeners)
{
	if (currentExpansion != e)
	{
#if USE_BACKEND
		if (FullInstrumentExpansion::isEnabled(mc))
		{
			debugToConsole(mc->getMainSynthChain(), "Skipping loading of expansion " + (e != nullptr ? e->getProperty(ExpansionIds::Name) : "Default"));
			return;
		}
#endif

		currentExpansion = e;
		notifier.sendNotification(Notifier::EventType::ExpansionLoaded, notifyListeners);
	}
}

bool ExpansionHandler::installFromResourceFile(const File& resourceFile)
{
	hlac::HlacArchiver a(nullptr);

	auto obj = a.readMetadataFromArchive(resourceFile);
	auto expansionName = obj.getProperty("HxiName", "").toString();

	if (expansionName.isNotEmpty())
	{
		auto f = [this, expansionName, resourceFile](Processor* p)
		{
			jassert(LockHelpers::freeToGo(mc));

			auto expRoot = getExpansionFolder().getChildFile(expansionName);

			expRoot.createDirectory();
			auto samplesDir = expRoot.getChildFile("Samples");
			samplesDir.createDirectory();

			hlac::HlacArchiver::DecompressData data;

			double unused = 0.0;

			data.option = hlac::HlacArchiver::OverwriteOption::OverwriteIfNewer;
			data.targetDirectory = samplesDir;
			data.progress = &mc->getSampleManager().getPreloadProgress();
			data.totalProgress = &unused;
			data.partProgress = &unused;
			data.sourceFile = resourceFile;

			hlac::HlacArchiver a(Thread::getCurrentThread());
			a.setListener(this);
			auto ok = a.extractSampleData(data);

			ignoreUnused(ok);

			auto headerFile = samplesDir.getChildFile("header.dat");
			jassert(headerFile.existsAsFile());

			if (getCredentials().isObject())
				ScriptEncryptedExpansion::encryptIntermediateFile(mc, headerFile, expRoot);
			else
			{
				auto hxiFile = Expansion::Helpers::getExpansionInfoFile(expRoot, Expansion::Intermediate);
				hxiFile.deleteFile();

				if (headerFile.moveFileTo(hxiFile))
					createAvailableExpansions();
			}

			return SafeFunctionCall::OK;
		};

		mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), f, MainController::KillStateHandler::SampleLoadingThread);

		return true;
	}

	return false;
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

void ExpansionHandler::logStatusMessage(const String& message)
{
	mc->getSampleManager().setCurrentPreloadMessage(message);
	setErrorMessage(message, false);
}

void ExpansionHandler::criticalErrorOccured(const String& message)
{
	setErrorMessage(message, true);
}

bool ExpansionHandler::rebuildUnitialisedExpansions()
{
	bool didSomething = false;

	for (int i = 0; i < uninitialisedExpansions.size(); i++)
	{
		auto e = uninitialisedExpansions[i];

		auto r = e->initialise();

		checkAllowedExpansions(r, e);

		if (r.wasOk())
		{
			initialisationErrors.removeAllInstancesOf({ e, r });

			e = uninitialisedExpansions.removeAndReturn(i--);
			expansionList.add(e);
			didSomething = true;
		}
		else
		{
			for (auto& ie : initialisationErrors)
			{
				if (ie.e == e)
					ie.r = r;
			}

			setErrorMessage(r.getErrorMessage(), false);
		}
	}

	if (didSomething)
	{
		Expansion::Sorter s;
		expansionList.sort(s, true);
		notifier.sendNotification(Notifier::EventType::ExpansionCreated);
	}

	return didSomething;
}

void ExpansionHandler::checkAllowedExpansions(Result& r, Expansion* e)
{
	if (!r.wasOk())
		return;

	if (!allowedExpansions.contains(e->getExpansionType()))
	{
		String s;
		s << "Trying to load a " << Expansion::Helpers::getExpansionTypeName(e->getExpansionType()) << " expansion";
		r = Result::fail(s);
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

juce::ValueTree Expansion::getPropertyValueTree()
{
	if (data != nullptr)
		return data->v;

	return {};
}

juce::ValueTree Expansion::Helpers::loadValueTreeForFileBasedExpansion(const File& root)
{
	auto infoFile = Helpers::getExpansionInfoFile(root, FileBased);

	if (infoFile.existsAsFile())
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(infoFile);

		if (xml != nullptr)
		{
			auto tree = ValueTree::fromXml(*xml);
			return tree;
		}
	}

	return ValueTree("ExpansionInfo");
}

bool Expansion::Helpers::isXmlFile(const File& f)
{
	FileInputStream fis(f);
	return fis.readByte() == '<';
}

juce::File Expansion::Helpers::getExpansionInfoFile(const File& expansionRoot, ExpansionType type)
{
	if (type == Encrypted)		   return expansionRoot.getChildFile("info.hxp");
	else if (type == Intermediate) return expansionRoot.getChildFile("info.hxi");
	else						   return expansionRoot.getChildFile("expansion_info.xml");
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

String Expansion::Helpers::getExpansionTypeName(ExpansionType e)
{
	switch (e)
	{
	case FileBased:		return "FileBased";
	case Intermediate:	return "Intermediate";
	case Encrypted:		return "Encrypted";
	default:			jassertfalse; return {};
	}
}

var Expansion::Data::toPropertyObject() const
{
	return ValueTreeConverters::convertValueTreeToDynamicObject(v);
}

#if USE_BACKEND
ExpansionHandler::ScopedProjectExporter::ScopedProjectExporter(MainController* mc, bool shouldDoSomething) :
	ControlledObject(mc),
	doSomething(shouldDoSomething)
{
	if (doSomething)
	{
		auto& h = getMainController()->getExpansionHandler();

		h.enabled = true;
		expFolder = h.expansionFolder;
		h.expansionFolder = getMainController()->getSampleManager().getProjectHandler().getWorkDirectory().getParentDirectory();
		wasEnabled = h.isEnabled();
	}
}

ExpansionHandler::ScopedProjectExporter::~ScopedProjectExporter()
{
	if (doSomething)
	{
		auto& h = getMainController()->getExpansionHandler();
		h.enabled = wasEnabled;
		h.expansionFolder = expFolder;
	}
}
#endif


}
