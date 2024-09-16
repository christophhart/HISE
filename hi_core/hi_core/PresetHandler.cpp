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



void UserPresetHelpers::saveUserPreset(ModulatorSynthChain *chain, const String& targetFile/*=String()*/, NotificationType notify/*=sendNotification*/)
{
#if USE_BACKEND

	const String version = dynamic_cast<GlobalSettingManager*>(chain->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Version);

	SemanticVersionChecker versionChecker(version, version);

	if (!versionChecker.newVersionNumberIsValid())
	{
		PresetHandler::showMessageWindow("Invalid version number", "You need semantic versioning (something like 1.0.0) in order to support user presets", PresetHandler::IconType::Error);
		return;
	}

	if (!GET_PROJECT_HANDLER(chain).isActive()) return;
#endif
    
	File presetFile = File(targetFile);
	
    String existingNote;
	StringArray existingTags;
    
#if CONFIRM_PRESET_OVERWRITE

	if (presetFile.existsAsFile() && (!MessageManager::getInstance()->isThisTheMessageThread() || PresetHandler::showYesNoWindow("Confirm overwrite", "Do you want to overwrite the preset (Press cancel to create a new user preset?")))
	{
        existingNote = PresetBrowser::DataBaseHelpers::getNoteFromXml(presetFile);
        existingTags = PresetBrowser::DataBaseHelpers::getTagsFromXml(presetFile);

		presetFile.deleteFile();
	}
#else

	if (presetFile.existsAsFile())
	{
        existingNote = PresetBrowser::DataBaseHelpers::getNoteFromXml(presetFile);
        existingTags = PresetBrowser::DataBaseHelpers::getTagsFromXml(presetFile);

		presetFile.deleteFile();
	}
#endif
	
	if (!presetFile.existsAsFile())
	{
		auto preset = createUserPreset(chain);

		if (preset.isValid())
		{
			auto xml = preset.createXml();

			presetFile.replaceWithText(xml->createDocument(""));

			if (existingNote.isNotEmpty())
				PresetBrowser::DataBaseHelpers::writeNoteInXml(presetFile, existingNote);

			if (!existingTags.isEmpty())
				PresetBrowser::DataBaseHelpers::writeTagsInXml(presetFile, existingTags);

			if (notify)
			{

				chain->getMainController()->getUserPresetHandler().currentlyLoadedFile = presetFile;
				chain->getMainController()->getUserPresetHandler().sendRebuildMessage();
			}
		}
	}

	chain->getMainController()->getUserPresetHandler().postPresetSave();
}

juce::ValueTree UserPresetHelpers::createUserPreset(ModulatorSynthChain* chain)
{
	ValueTree preset;

#if USE_RAW_FRONTEND
	preset = dynamic_cast<FrontendProcessor*>(chain->getMainController())->getRawDataHolder()->exportAsValueTree();
#else

	if (auto sp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(chain->getMainController()))
	{
		preset = ValueTree("Preset");

		if (chain->getMainController()->getUserPresetHandler().isUsingCustomDataModel())
		{
			chain->getMainController()->getUserPresetHandler().saveStateManager(preset, UserPresetIds::CustomJSON);
		}
		else
		{
			ValueTree v = sp->getScriptingContent()->exportAsValueTree();

			v.setProperty("Processor", sp->getId(), nullptr);
			preset.addChild(v, -1, nullptr);
		}

		chain->getMainController()->getUserPresetHandler().saveStateManager(preset, UserPresetIds::Modules);

	}
#endif

	chain->getMainController()->getUserPresetHandler().saveStateManager(preset, UserPresetIds::MidiAutomation);
	chain->getMainController()->getUserPresetHandler().saveStateManager(preset, UserPresetIds::MPEData);

	preset.setProperty("Version", getCurrentVersionNumber(chain), nullptr);

	addRequiredExpansions(chain->getMainController(), preset);

	if(chain->getMainController()->getMacroManager().isMacroEnabledOnFrontend())
		chain->saveMacrosToValueTree(preset);

	// Store the rest...
	chain->getMainController()->getUserPresetHandler().saveStateManager(preset, UserPresetIds::AdditionalStates);

	return preset;
}

void UserPresetHelpers::addRequiredExpansions(const MainController* mc, ValueTree& preset)
{
	ignoreUnused(mc, preset);

	const auto& expHandler = mc->getExpansionHandler();

	String s;

	for (int i = 0; i < expHandler.getNumExpansions(); i++)
	{
		if (expHandler.getExpansion(i)->isActive())
			s << expHandler.getExpansion(i)->getProperty(ExpansionIds::Name) << ";";
	}

	if (s.isNotEmpty())
		preset.setProperty("RequiredExpansions", s, nullptr);
}

StringArray UserPresetHelpers::checkRequiredExpansions(MainController* mc, ValueTree& preset)
{
	StringArray missingExpansions;
	auto& expHandler = mc->getExpansionHandler();

	if (expHandler.isEnabled())
	{
		auto expList = preset.getProperty("RequiredExpansions", "").toString();
		auto sa = StringArray::fromTokens(expList, ";", "");
		sa.removeDuplicates(true);
		sa.removeEmptyStrings();

		for (auto s : sa)
			if (expHandler.getExpansionFromName(s) == nullptr)
				missingExpansions.add(s);

	}

	return missingExpansions;
}



void UserPresetHelpers::loadUserPreset(ModulatorSynthChain *chain, const File &fileToLoad)
{
	chain->getMainController()->getDebugLogger().logMessage("### Loading user preset " + fileToLoad.getFileNameWithoutExtension() + "\n");
	chain->getMainController()->loadUserPresetAsync(fileToLoad);
}

#if 0
void UserPresetHelpers::loadUserPreset(ModulatorSynthChain* chain, const ValueTree &parent)
{
	chain->getMainController()->loadUserPresetAsync(parent);

}
#endif



#if !USE_MIDI_AUTOMATION_MIGRATION
Identifier UserPresetHelpers::getAutomationIndexFromOldVersion(const String& /*oldVersion*/, int /*oldIndex*/)
{	
	jassertfalse;

	// This just returns the an empty Identifier. If you want to use this function, define USE_MIDI_AUTOMATION_MIGRATION
	// and then supply a oldIndex -> newIndex conversion function.
	return Identifier();
}
#endif

bool UserPresetHelpers::updateVersionNumber(ModulatorSynthChain* chain, const File& fileToUpdate)
{
	auto xml = XmlDocument::parse(fileToUpdate);

	const String thisVersion = getCurrentVersionNumber(chain);

	if (xml != nullptr)
	{
		const String presetVersion = xml->getStringAttribute("Version");

		if (presetVersion != thisVersion)
		{
			xml->setAttribute("Version", thisVersion);

			fileToUpdate.replaceWithText(xml->createDocument(""));

			return true;
		}
	}

	return false;
}

bool UserPresetHelpers::checkVersionNumber(ModulatorSynthChain* chain, XmlElement& element)
{
	const String presetVersion = element.getStringAttribute("Version");

	SemanticVersionChecker versionChecker(presetVersion, getCurrentVersionNumber(chain));

	if (!versionChecker.newVersionNumberIsValid())
	{
		PresetHandler::showMessageWindow("Invalid Preset Version", "The preset Version " + presetVersion + " is not valid", PresetHandler::IconType::Error);
		return false;
	}

	return !versionChecker.isMinorVersionUpdate() && !versionChecker.isMajorVersionUpdate();
}

String UserPresetHelpers::getCurrentVersionNumber(ModulatorSynthChain* chain)
{
#if USE_BACKEND
	return dynamic_cast<GlobalSettingManager*>(chain->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Version);
#else
	ignoreUnused(chain);
	return FrontendHandler::getVersionString();
#endif
}



ValueTree parseUserPreset(const File& f)
{
	if (!f.hasFileExtension(".preset") || f.getFileName().startsWith("."))
		return {};

	auto xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		ValueTree p = ValueTree("PresetFile");
		p.setProperty("FileName", f.getFileNameWithoutExtension(), nullptr);
		ValueTree pContent = ValueTree::fromXml(*xml);
		p.setProperty("isDirectory", false, nullptr);
		p.addChild(pContent, -1, nullptr);

		return p;
	}

	jassertfalse;

	return {};
	
}

ValueTree UserPresetHelpers::collectAllUserPresets(ModulatorSynthChain* chain, FileHandlerBase* currentExpansion)
{
	ValueTree v("UserPresets");

	auto presetRoot = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
	
	if (currentExpansion != nullptr)
		presetRoot = currentExpansion->getSubDirectory(FileHandlerBase::UserPresets);

	Array<File> banks;
	Array<File> presetsOnBankLevel;

	presetRoot.findChildFiles(banks, File::findDirectories, false);
	presetRoot.findChildFiles(presetsOnBankLevel, File::findFiles, false, "*.preset");

	for (auto bank : banks)
	{
		ValueTree b("Bank");
		b.setProperty("FileName", bank.getFileNameWithoutExtension(), nullptr);

		Array<File> categories;
		Array<File> presetsOnCategoryLevel;

		bank.findChildFiles(categories, File::findDirectories, false);
		bank.findChildFiles(presetsOnCategoryLevel, File::findFiles, false, "*.preset");

		for (auto cat : categories)
		{
			ValueTree c("Category");
			c.setProperty("FileName", cat.getFileNameWithoutExtension(), nullptr);
			c.setProperty("isDirectory", true, nullptr);

			Array<File> presets;

			cat.findChildFiles(presets, File::findFiles, false, "*.preset");

			for (auto preset : presets)
			{
				auto np = parseUserPreset(preset);

				if(np.isValid())
					c.addChild(np, -1, nullptr);
			}

			c.setProperty("isDirectory", true, nullptr);
			b.addChild(c, -1, nullptr);
		}

		for (auto preset : presetsOnCategoryLevel)
		{
			auto np = parseUserPreset(preset);

			if (np.isValid())
				b.addChild(np, -1, nullptr);
		}
			
		b.setProperty("isDirectory", true, nullptr);
		v.addChild(b, -1, nullptr);
	}

	presetRoot.findChildFiles(presetsOnBankLevel, File::findFiles, false);

	for (auto preset : presetsOnBankLevel)
	{
		auto np = parseUserPreset(preset);

		if (np.isValid())
			v.addChild(np, -1, nullptr);
	}

	return v;
}


juce::StringArray UserPresetHelpers::getExpansionsForUserPreset(const File& userpresetFile)
{
	auto xml = XmlDocument::parse(userpresetFile);

	if (xml != nullptr)
	{
		auto v = xml->getStringAttribute("RequiredExpansions", "");
		return StringArray::fromTokens(v, ";", "");
	}

	return {};
}

void UserPresetHelpers::extractUserPresets(const char* userPresetData, size_t size)
{
#if USE_FRONTEND && !DONT_CREATE_USER_PRESET_FOLDER
	auto userPresetDirectory = FrontendHandler::getUserPresetDirectory();

#if !HISE_OVERWRITE_OLD_USER_PRESETS
	if (userPresetDirectory.isDirectory())
		return;
#else
	auto infoFile = userPresetDirectory.getChildFile("info.json");
	auto infoObj = JSON::parse(infoFile);

	if (infoObj.getProperty(ExpansionIds::Version, "").toString() == FrontendHandler::getVersionString())
		return;
#endif

	LOG_START("Extracting user presets to AppData directory");

	if(!userPresetDirectory.isDirectory())
		userPresetDirectory.createDirectory();

	zstd::ZCompressor<UserPresetDictionaryProvider> decompressor;

	MemoryBlock mb(userPresetData, size);

	ValueTree presetTree;

	decompressor.expand(mb, presetTree);

	extractDirectory(presetTree, userPresetDirectory);

#if HISE_OVERWRITE_OLD_USER_PRESETS
	
	infoObj = var(new DynamicObject());
	infoObj.getDynamicObject()->setProperty(ExpansionIds::Name, FrontendHandler::getProjectName());
	infoObj.getDynamicObject()->setProperty(ExpansionIds::Version, FrontendHandler::getVersionString());
	infoObj.getDynamicObject()->setProperty("Date", Time::getCurrentTime().toISO8601(true));
	infoFile.replaceWithText(JSON::toString(infoObj));

#endif

#else
	ignoreUnused(userPresetData, size);
#endif
}


void UserPresetHelpers::extractPreset(ValueTree preset, File parent)
{
	auto presetName = preset.getProperty("FileName").toString();
	auto presetFile = parent.getChildFile(presetName + ".preset");
	auto presetContent = preset.getChild(0);

	presetFile.replaceWithText(presetContent.toXmlString());
}


void UserPresetHelpers::extractDirectory(ValueTree directory, File parent)
{
	for (auto category : directory)
	{
		if (category.getProperty("isDirectory"))
		{
			auto subDirectoryName = category.getProperty("FileName").toString();

			if (subDirectoryName.isNotEmpty())
			{
				auto subDirectory = parent.getChildFile(subDirectoryName);
				subDirectory.createDirectory();
				extractDirectory(category, subDirectory);
			}
		}
		else
			extractPreset(category, parent);
	}
}

ModuleStateManager::ModuleStateManager(MainController* mc):
	ControlledObject(mc)
{}

ModuleStateManager::StoredModuleData** ModuleStateManager::begin()
{ return modules.begin(); }

ModuleStateManager::StoredModuleData** ModuleStateManager::end()
{ return modules.end(); }

Identifier ModuleStateManager::getUserPresetStateId() const
{ return UserPresetIds::Modules; }

void ModuleStateManager::resetUserPresetState()
{}

void PresetHandler::saveProcessorAsPreset(Processor *p, const String &directoryPath/*=String()*/)
{
	const bool hasCustomName = p->getName() != p->getId();

	if(!hasCustomName) p->setId(getCustomName(p->getName()));

	File directory = directoryPath.isEmpty() ? getDirectory(p) : File(directoryPath);

	jassert(directory.isDirectory());

	String fileName = directory.getFullPathName() + "/" + p->getId() + ".hip";
	File outputFile(fileName);

	if(!outputFile.existsAsFile() || showYesNoWindow("Overwrite File " + fileName, "Do you want to overwrite the Preset?"))
	{
		debugToConsole(p, "Save " + p->getId() + " to " + directory.getFullPathName());

		ValueTree v = p->exportAsValueTree();
        
        v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);

		FullInstrumentExpansion::setNewDefault(p->getMainController(), v);

		outputFile.deleteFile();

		FileOutputStream fos(outputFile);

		v.writeToStream(fos);
	}
}

String PresetHandler::getProcessorNameFromClipboard(const FactoryType *t)
{
	if(SystemClipboard::getTextFromClipboard() == String()) return String();

	String x = SystemClipboard::getTextFromClipboard();
	auto xml = XmlDocument::parse(x);

	if(xml == nullptr) return String();
	
#if USE_OLD_FILE_FORMAT

	bool isProcessor = true;
	String type = xml->getTagName();

#else

	bool isProcessor = xml->getTagName() == "Processor";
	String type = xml->getStringAttribute("Type");

#endif

	String id = xml->getStringAttribute("ID");
	
	if(!isProcessor || type == String() || id == String()) return String();

	if (t->allowType(type)) return id;
				else		return String();
}

void PresetHandler::copyProcessorToClipboard(Processor *p)
{
	auto xml = p->exportAsValueTree().createXml();
	String x = xml->createDocument(String());
	SystemClipboard::copyTextToClipboard(x);

	debugToConsole(p, p->getId() + " was copied to clipboard.");
}



void* PresetHandler::currentController = nullptr;

String PresetHandler::getCustomName(const String &typeName, const String& thisMessage/*=String()*/)
{
	String message;

    const bool useCustomMessage = thisMessage.isNotEmpty();
    
	if (!useCustomMessage)
	{
		message << "Enter the unique Name for the ";
		message << typeName;
		message << ".\nCamelCase is recommended.";
	}
	else
	{
		message << thisMessage;
	}

	ScopedPointer<LookAndFeel> laf = createAlertWindowLookAndFeel();


	ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(PresetHandler::IconType::Question, laf, message);

    ScopedPointer<AlertWindow> nameWindow = new AlertWindow(useCustomMessage ? ("Enter " + typeName) : ("Enter name for " + typeName), "", AlertWindow::AlertIconType::NoIcon);


	nameWindow->setLookAndFeel(laf);

	nameWindow->addCustomComponent(comp);

	nameWindow->addTextEditor("Name", typeName );
	nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

#if HISE_IOS
    
    const int x = nameWindow->getX();
    const int y = jmax<int>(10, nameWindow->getY() - 150);
    
    nameWindow->setTopLeftPosition(x, y);
    
#endif
    
	nameWindow->getTextEditor("Name")->setSelectAllWhenFocused(true);
	nameWindow->getTextEditor("Name")->grabKeyboardFocusAsync();

	if(nameWindow->runModalLoop()) return nameWindow->getTextEditorContents("Name");
	else return String();
    
};

bool PresetHandler::showYesNoWindow(const String &title, const String &message, PresetHandler::IconType type)
{
#if HISE_HEADLESS
		return true;
#endif

#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
	{
		return true; // surpress Popups in commandline mode
	}
#endif


#if HISE_IOS
    return NativeMessageBox::showOkCancelBox(AlertWindow::AlertIconType::NoIcon, title, message);
    
#else

	MessageManagerLock mm;

	ScopedPointer<LookAndFeel> laf = createAlertWindowLookAndFeel();
	ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(type, laf, message);
	
	ScopedPointer<AlertWindow> nameWindow = new AlertWindow(title, "", AlertWindow::AlertIconType::NoIcon);

	nameWindow->setLookAndFeel(laf);
	nameWindow->addCustomComponent(comp);
	
	nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	return (nameWindow->runModalLoop() == 1);
    
#endif
};

bool PresetHandler::showYesNoWindowIfMessageThread(const String &title, const String &message, bool defaultReturnValue, IconType icon /*= IconType::Question*/)
{
	if (auto mm = MessageManager::getInstanceWithoutCreating())
	{
		if (mm->currentThreadHasLockedMessageManager())
		{
			return showYesNoWindow(title, message, icon);
		}
	}

	return defaultReturnValue;
}

void PresetHandler::showMessageWindow(const String &title, const String &message, PresetHandler::IconType type)
{
	if (MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
	{
#if USE_BACKEND
		if (CompileExporter::isExportingFromCommandLine())
		{
			std::cout << title << ": " << message << std::endl;
			return;
		}

#endif

#if HISE_IOS

		NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::NoIcon, title, message);

#else

		ScopedPointer<LookAndFeel> laf = createAlertWindowLookAndFeel();
		ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(type, laf, message);
		ScopedPointer<AlertWindow> nameWindow = new AlertWindow(title, "", AlertWindow::AlertIconType::NoIcon);

		nameWindow->setLookAndFeel(laf);
		nameWindow->addCustomComponent(comp);
		nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));

		nameWindow->runModalLoop();
#endif

		return;
	}
	else
	{
		MessageManager::callAsync([title, message, type]()
		{
			showMessageWindow(title, message, type);
		});
	}
};

struct CountedProcessorId
{
	CountedProcessorId(Processor *p) :
		processorName(p->getId())
	{
		processors.add(p);
	};

	const String processorName;
	Array<WeakReference<Processor>> processors;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CountedProcessorId)
};




void ProjectHandler::createNewProject(const File &workingDirectory, Component* )
{
	auto wd = workingDirectory;

	if (wd.exists() && wd.isDirectory())
	{
		while (wd.getNumberOfChildFiles(File::findFilesAndDirectories) > 1)
		{
			PresetHandler::showMessageWindow("Directory already exists", "The directory is not empty. Try another one...", PresetHandler::IconType::Warning);
            
            FileChooser fc("Create new project directory");
            
            if (fc.browseForDirectory())
                wd = fc.getResult();
            else
                return;
		}
	}

	for (int i = 0; i < (int)SubDirectories::numSubDirectories; i++)
	{
		File subDirectory = wd.getChildFile(getIdentifier((SubDirectories)i));
		subDirectory.createDirectory();
	}
}

juce::Result ProjectHandler::setWorkingProject(const File &workingDirectory, bool checkDirectories)
{
	IF_NOT_HEADLESS(MessageManagerLock mm);

	if (!workingDirectory.exists()) return Result::fail(workingDirectory.getFullPathName() + " is not a valid folder");;

	if (workingDirectory == currentWorkDirectory) return Result::ok();

	if (!isValidProjectFolder(workingDirectory))
	{
		return Result::fail(workingDirectory.getFullPathName() + "is not a valid project folder");	
	}

	currentWorkDirectory = workingDirectory;

	if(checkDirectories)
		checkSubDirectories();

	jassert(currentWorkDirectory.exists() && currentWorkDirectory.isDirectory());

	if (!recentWorkDirectories.contains(workingDirectory.getFullPathName()))
	{
		const int numTooMuch = recentWorkDirectories.size() - 12;

		if (numTooMuch > 0)
		{
			recentWorkDirectories.removeRange(12, numTooMuch);
		}

		recentWorkDirectories.insert(0, workingDirectory.getFullPathName());
	}
	else
	{
		const int index = recentWorkDirectories.indexOf(workingDirectory.getFullPathName());

		recentWorkDirectories.move(index, 0);
	}

	ScopedPointer<XmlElement> xml = new XmlElement("Projects");

	xml->setAttribute("current", currentWorkDirectory.getFullPathName());

	for (int i = 0; i < recentWorkDirectories.size(); i++)
	{
		XmlElement *child = new XmlElement("Recent");

		child->setAttribute("path", recentWorkDirectories[i]);

		xml->addChildElement(child);
	}

	getAppDataDirectory(nullptr).getChildFile("projects.xml").replaceWithText(xml->createDocument(""));

	ScopedLock sl(listeners.getLock());

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
			listeners[i]->projectChanged(currentWorkDirectory);
		else
			listeners.remove(i--);
	}

	getMainController()->setWebViewRoot(currentWorkDirectory);

	return Result::ok();
}

void ProjectHandler::restoreWorkingProjects()
{
	auto xml = XmlDocument::parse(getAppDataDirectory(nullptr).getChildFile("projects.xml"));

	if (xml != nullptr)
	{
		jassert(xml->getTagName() == "Projects");

		File current = File(xml->getStringAttribute("current"));

		recentWorkDirectories.clear();

		for (int i = 0; i < xml->getNumChildElements(); i++)
		{
			recentWorkDirectories.add(xml->getChildElement(i)->getStringAttribute("path"));
		}

		setWorkingProject(current, false);

		
	}
}

bool ProjectHandler::isValidProjectFolder(const File &file) const
{
	if (!anySubdirectoryExists(file))
		return false;

	if (file == File()) return true;

	const bool isDirectory = file.exists() && file.isDirectory();

	if (!isDirectory) return false;
	
	for (int i = 0; i < (int)SubDirectories::numSubDirectories; i++)
	{
		File sub = file.getChildFile(getIdentifier((SubDirectories)i));

		if (!(sub.exists() && sub.isDirectory()))
		{
			sub.createDirectory();
		}
	}

	return true;
}

bool ProjectHandler::anySubdirectoryExists(const File& possibleProjectFolder) const
{
    for(const auto& dir: getSubDirectoryIds())
    {
        auto id = getIdentifier(dir);
        id.removeCharacters("/");
        if(possibleProjectFolder.getChildFile(id).isDirectory())
            return true;
    }

    return false;
}

File ProjectHandler::getWorkDirectory() const
{
	if (!isActive())
	{
		return File();
	}

	else return currentWorkDirectory;
}


String ProjectHandler::getDefaultUserPreset() const
{
	return GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::DefaultUserPreset);
}

juce::ValueTree ProjectHandler::getEmbeddedNetwork(const String& id)
{
#if USE_BACKEND
	auto f = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::Networks);
	auto nf = f.getChildFile(id).withFileExtension("xml");

	if (nf.existsAsFile())
	{
		if (auto xml = XmlDocument::parse(nf))
		{
			if (!CompileExporter::isExportingFromCommandLine())
			{
				debugToConsole(getMainController()->getMainSynthChain(), "Load network " + nf.getFileName() + " from project folder");
			}

			return ValueTree::fromXml(*xml);
		}
	}
#endif

	jassertfalse;
	return {};
}

struct FileModificationComparator
{
	static int compareElements(const File &first, const File &second)
	{
		const int64 firstTime = first.getLastAccessTime().toMilliseconds();
		const int64 secondTime = second.getLastAccessTime().toMilliseconds();

		if (firstTime > secondTime) return -1;
		else return 1;
	}
};

void ProjectHandler::createRSAKey() const
{
	AlertWindowLookAndFeel wlaf;

	AlertWindow aw("Create RSA Keys", "Generate an RSA key pair of the desired bit length\n(Higher bit length can take time)", AlertWindow::AlertIconType::NoIcon);

	aw.setLookAndFeel(&wlaf);
	
	aw.addComboBox("keyLength", StringArray(), "Select bit length");

	ComboBox* b = aw.getComboBoxComponent("keyLength");

    StringArray bitLength = { "512", "1024", "2048" };

	b->addItemList(bitLength, 1);
	b->setSelectedItemIndex(0);
	GlobalHiseLookAndFeel::setDefaultColours(*b);

	aw.addButton("Create", 1, KeyPress(KeyPress::returnKey));
	aw.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	const int awResult = aw.runModalLoop();

	if (awResult == 1)
	{
        RSAKey publicKey;
		RSAKey privateKey;

		Random r;
        
        const int l = bitLength[b->getSelectedItemIndex()].getIntValue();
        
		const int seeds[] = { r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt() };

		auto existingFile = getWorkDirectory().getChildFile("RSA.xml");

		if (existingFile.existsAsFile())
		{
			publicKey = RSAKey(getPublicKeyFromFile(existingFile));
			privateKey = RSAKey(getPrivateKeyFromFile(existingFile));
		}
		else
			RSAKey::createKeyPair(publicKey, privateKey, l, seeds, 6);

		AlertWindow w("RSA Keys", "You can use this key pair for the copy protection", AlertWindow::InfoIcon);

		w.setLookAndFeel(&wlaf);

        w.addTextEditor("publicKey", publicKey.toString(), "Public Key", false);
		w.getTextEditor("publicKey")->setReadOnly(true);

		w.addTextEditor("privateKey", privateKey.toString(), "Private Key", false);
		w.getTextEditor("privateKey")->setReadOnly(true);

		w.addButton("OK", 0, KeyPress(KeyPress::returnKey));
		w.addButton("Copy to clipboard", 1);
		w.addButton("Save to project folder", 2);

		const int result = w.runModalLoop();

		String text = "public: " + publicKey.toString() + "\n";
		text << "private: " << privateKey.toString();

		if (result == 1)
		{
			SystemClipboard::copyTextToClipboard(text);
			PresetHandler::showMessageWindow("RSA Keys copied to clipboard", "The RSA keys are copied to the clipboard.", PresetHandler::IconType::Info);
		}
		else if (result == 2)
		{
			ScopedPointer<XmlElement> xml = new XmlElement("KeyPair");

			xml->addChildElement(new XmlElement("PublicKey"));
			xml->addChildElement(new XmlElement("PrivateKey"));

			xml->getChildByName("PublicKey")->setAttribute("value", publicKey.toString());
			xml->getChildByName("PrivateKey")->setAttribute("value", privateKey.toString());

			auto key = privateKey.toString();
			auto numbers = StringArray::fromTokens(key, ",", "");

			BigInteger b1, b2;
			b1.parseString(numbers[0], 16);
			b2.parseString(numbers[1], 16);

			auto n1 = b1.toString(10);
			auto n2 = b2.toString(10);

			xml->addChildElement(new XmlElement("ServerKey1"));
			xml->addChildElement(new XmlElement("ServerKey2"));

			xml->getChildByName("ServerKey1")->setAttribute("value", n1);
			xml->getChildByName("ServerKey2")->setAttribute("value", n2);
			
			File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

			rsaFile.replaceWithText(xml->createDocument(""));
			
			PresetHandler::showMessageWindow("RSA keys exported to file", "The RSA Keys are written to the file " + rsaFile.getFullPathName(), PresetHandler::IconType::Info);
		}
	}
}

String ProjectHandler::getPublicKey() const
{
	File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

	return getPublicKeyFromFile(rsaFile);
}

String ProjectHandler::getPrivateKey() const
{
	File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

	return getPrivateKeyFromFile(rsaFile);
}


juce::String ProjectHandler::getPublicKeyFromFile(const File& f)
{
	auto xml = XmlDocument::parse(f);

	if (xml == nullptr) return "";

	return xml->getChildByName("PublicKey")->getStringAttribute("value", "");
}


juce::String ProjectHandler::getPrivateKeyFromFile(const File& f)
{
	auto xml = XmlDocument::parse(f);

	if (xml == nullptr) return "";

	return xml->getChildByName("PrivateKey")->getStringAttribute("value", "");
}

void ProjectHandler::checkActiveProject()
{
	
}

juce::File ProjectHandler::getAppDataRoot(MainController* mc)
{
#if USE_BACKEND
    
    // Check this property dynamically
    File::SpecialLocationType appDataDirectoryToUse;
    
    if(mc == nullptr)
    {
        // if we pass in nullptr here, it's supposed to return the HISE app data folder
        // which is always local
        appDataDirectoryToUse = File::userApplicationDataDirectory;
    }
    else
    {
#if JUCE_WINDOWS
        auto idToUse = HiseSettings::Project::UseGlobalAppDataFolderWindows;
#else
        auto idToUse = HiseSettings::Project::UseGlobalAppDataFolderMacOS;
#endif
        
        if(GET_HISE_SETTING(mc->getMainSynthChain(), idToUse))
            appDataDirectoryToUse = File::commonApplicationDataDirectory;
        else
            appDataDirectoryToUse = File::userApplicationDataDirectory;
    }
#else
    
    // discard the mc pointer, we'll use a static property
    ignoreUnused(mc);
    
    // Use the preprocessor to figure it out in the compiled plugin
#if HISE_USE_SYSTEM_APP_DATA_FOLDER
    const File::SpecialLocationType appDataDirectoryToUse = File::commonApplicationDataDirectory;
#else
    const File::SpecialLocationType appDataDirectoryToUse = File::userApplicationDataDirectory;
#endif
#endif
    
    
#if JUCE_IOS
	return File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support/");
#elif JUCE_MAC


#if !USE_BACKEND && ENABLE_APPLE_SANDBOX
    ignoreUnused(appDataDirectoryToUse);
	return File::getSpecialLocation(File::userMusicDirectory);
#else
	return File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support");
#endif


#else // WINDOWS
	return File::getSpecialLocation(appDataDirectoryToUse);
#endif

}

juce::File ProjectHandler::getAppDataDirectory(MainController* mc)
{
    auto appDataRoot = getAppDataRoot(mc);


#if JUCE_WINDOWS
	// Windows
	File f = appDataRoot.getChildFile("HISE");
#elif JUCE_MAC

#if HISE_IOS
	// iOS
	File f = appDataRoot
#else
	// OS X
	File f = appDataRoot.getChildFile("HISE");
#endif

#else
	// Linux
	File f = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory).getChildFile(".hise/");
#endif

	if (!f.isDirectory())
        f.createDirectory();

    return f;
}

juce::String FrontendHandler::checkSampleReferences(MainController* mc, bool returnTrueIfOneSampleFound)
{
	Array<File> sampleList;

    StringArray existingFiles;
    StringArray missingFiles;
    
	const File sampleLocation = getSampleLocationForCompiledPlugin();

	sampleLocation.findChildFiles(sampleList, File::findFiles, true);

	String falseName;

    int numCorrectSampleMaps = 0;
    
	auto pool = mc->getCurrentSampleMapPool();

	Array<PooledSampleMap> sampleMaps;

	pool->loadAllFilesFromDataProvider();

	for (int i = 0; i < pool->getNumLoadedFiles(); i++)
	{
		if (auto item = pool->getWeakReferenceToItem(pool->getReference(i)))
		{
			auto sampleMap = *item.getData();

			const String thisFalseName = SampleMap::checkReferences(mc, sampleMap, sampleLocation, sampleList);

			if (thisFalseName.isNotEmpty())
			{
				falseName = thisFalseName;
			}
			else
			{
				numCorrectSampleMaps++;
			}
		}

	}

	if(returnTrueIfOneSampleFound)
    {
        if(numCorrectSampleMaps != 0)
        {
            return String();
        }
        else
        {
            return falseName;
        }
    }
    else
    {
        return falseName;
    }
}


File FrontendHandler::getResourcesFolder()
{
#if HISE_IOS

	File directory = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile);

	if (SystemStats::isRunningInAppExtensionSandbox())
	{
		return directory.getParentDirectory().getParentDirectory();
	}
	else
	{
		return directory;
	}

#else

	// not interesting on Windows or macOS
	jassertfalse;
	return File();

#endif
}

bool ProjectHandler::isActive() const
{
	return currentWorkDirectory != File();
}

void ProjectHandler::setProjectSettings(Component *mainEditor)
{
	ignoreUnused(mainEditor);

#if USE_BACKEND
	BackendCommandTarget::Actions::showFileProjectSettings(mainEditor->findParentComponentOfClass<BackendRootWindow>());
#endif
}

juce::File FrontendHandler::getRootFolder() const
{
	return getAppDataDirectory();
}

juce::File FrontendHandler::getEmbeddedResourceDirectory() const
{
#if HISE_IOS
    return getResourcesFolder().getChildFile("EmbeddedResources");
#else
    return getRootFolder();
#endif
    
}
    
juce::File FrontendHandler::getSubDirectory(SubDirectories directory) const
{
	switch (directory)
	{
	case hise::FileHandlerBase::AudioFiles:
		return getAdditionalAudioFilesDirectory();
		break;
	case hise::FileHandlerBase::Scripts:
	case hise::FileHandlerBase::Images:
	case hise::FileHandlerBase::Presets:
	case hise::FileHandlerBase::Binaries:
	case hise::FileHandlerBase::SampleMaps:
	case hise::FileHandlerBase::XMLPresetBackups:
	case hise::FileHandlerBase::AdditionalSourceCode:
	case hise::FileHandlerBase::numSubDirectories:
	case hise::FileHandlerBase::MidiFiles:
		jassertfalse;
		break; 
	case hise::FileHandlerBase::UserPresets:
		return getRootFolder().getChildFile("User Presets");
	case hise::FileHandlerBase::Samples:
		return getSampleLocationForCompiledPlugin();
	default:
		break;
	}

	jassertfalse;
	return File();
}

File FrontendHandler::getSampleLocationForCompiledPlugin()
{
#if USE_FRONTEND
    
#if HISE_IOS
    
    File f = getResourcesFolder().getChildFile("Samples/");
    
    
    return f;
    
#endif
    
    
    
	File appDataDir = getAppDataDirectory(nullptr);

	// The installer should take care of creating the app data directory...
	jassert(appDataDir.isDirectory());
	
#if JUCE_MAC && ENABLE_APPLE_SANDBOX
    
    auto resourceDir = appDataDir.getChildFile("Resources/");
    if(!resourceDir.isDirectory())
    {
        resourceDir.createDirectory();
    }
    
    
    File childFile = ProjectHandler::getLinkFile(resourceDir);
#else
    File childFile = ProjectHandler::getLinkFile(appDataDir);
#endif

	if (childFile.exists())
	{
		File f(childFile.loadFileAsString());

		if (!f.isDirectory())
			f.createDirectory();

		return f;
	}
	else
	{
#if !HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
		// In this case we'll silently pick a sensible default location for the samples

#if JUCE_WINDOWS
		File sensibleDefaultLocation = File::getSpecialLocation(File::userDocumentsDirectory);
#else
		File sensibleDefaultLocation = File::getSpecialLocation(File::userMusicDirectory);
#endif

		auto newSampleLoc = sensibleDefaultLocation.getChildFile(getCompanyName()).getChildFile(getProjectName()).getChildFile("Samples");

		setSampleLocation(newSampleLoc);

		return newSampleLoc;
#else
		return File();
#endif
	}

	

#else
	return File();
#endif
}

juce::File FrontendHandler::getAppDataDirectory(MainController* /*unused*/)
{
    auto root = ProjectHandler::getAppDataRoot(nullptr);
    
	auto f = root.getChildFile(getCompanyName() + "/" + getProjectName());

	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

juce::ValueTree FrontendHandler::getEmbeddedNetwork(const String& id)
{
	for (auto n : networks)
	{
		if (n["ID"].toString() == id)
			return n;
	}

#if USE_FRONTEND
	if (ScopedPointer<scriptnode::dll::FactoryBase> f = scriptnode::DspNetwork::createStaticFactory())
	{
		// We need to look in the compiled networks and return a dummy ValueTree
		int numNodes = f->getNumNodes();

		for (int i = 0; i < numNodes; i++)
		{
			if (f->getId(i) == id)
			{
				ValueTree v(PropertyIds::Network);
				v.setProperty(PropertyIds::ID, id, nullptr);

				ValueTree r(PropertyIds::Node);
				r.setProperty(PropertyIds::FactoryPath, "container.chain", nullptr);
				r.setProperty(PropertyIds::ID, id, nullptr);
				v.addChild(r, -1, nullptr);

				return v;
			}
		}
	}
#endif

	jassertfalse;
	return {};
}

void FrontendHandler::loadSamplesAfterSetup()
{
	if (shouldLoadSamplesAfterSetup())
	{
		LOG_START("Loading samples");

		dynamic_cast<AudioProcessor*>(getMainController())->suspendProcessing(false);

		getMainController()->getSampleManager().preloadEverything();
	}
	else
	{
		dynamic_cast<AudioProcessor*>(getMainController())->suspendProcessing(true);
	}
}

void FrontendHandler::checkAllSampleReferences()
{
#if HISE_IOS

	samplesCorrectlyLoaded = true;

#else
	const String missingSampleName = checkSampleReferences(getMainController(), true);

	samplesCorrectlyLoaded = missingSampleName.isEmpty();

	if (missingSampleName.isNotEmpty())
	{
		dynamic_cast<MainController*>(getMainController())->sendOverlayMessage(DeactiveOverlay::State::SamplesNotFound, "The sample " + missingSampleName + " was not found.");
	}
#endif
}

File FrontendHandler::getLicenseKey()
{
#if USE_FRONTEND
	return getAppDataDirectory().getChildFile(getProjectName() + getLicenseKeyExtension());
#else

	return File();

#endif
}

String FrontendHandler::getLicenseKeyExtension()
{

#if JUCE_WINDOWS
	return ".license_x64";
#else
	return ".license";
#endif
}

void FrontendHandler::setSampleLocation(const File &newLocation)
{
#if USE_FRONTEND
	
	if (newLocation.isDirectory())
		newLocation.createDirectory();

	auto linkFile = getSampleLinkFile();

	linkFile.replaceWithText(newLocation.getFullPathName());

#else

	ignoreUnused(newLocation);

#endif
}



File FrontendHandler::getSampleLinkFile()
{
	File appDataDir = getAppDataDirectory();

	// The installer should take care of creating the app data directory...
	jassert(appDataDir.isDirectory());

#if JUCE_MAC && ENABLE_APPLE_SANDBOX
    
    auto resourceDir = appDataDir.getChildFile("Resources/");
    if(!resourceDir.isDirectory())
    {
        resourceDir.createDirectory();
    }
    
    
    File childFile = ProjectHandler::getLinkFile(resourceDir);
#else
	File childFile = ProjectHandler::getLinkFile(appDataDir);
#endif

	return childFile;
}




File FrontendHandler::getUserPresetDirectory(bool getRedirect)
{
#if HISE_IOS
    
    NSString *appGroupID = [NSString stringWithUTF8String: getAppGroupId().toUTF8()];
    
    NSFileManager* fm = [NSFileManager defaultManager];
    NSURL *containerURL = [fm containerURLForSecurityApplicationGroupIdentifier:appGroupID];
    
    if(containerURL == nullptr)
    {
        // Oops, the App Group ID was not valid
        jassertfalse;
    }
    
    String tmp = ([containerURL.relativeString UTF8String]);
    File resourcesRoot(tmp.substring(6));
    
    File userPresetDirectory = resourcesRoot.getChildFile("UserPresets");
    
    if(!userPresetDirectory.isDirectory())
    {
        File factoryPresets = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getChildFile("UserPresets/");
        
        
        if(!factoryPresets.isDirectory())
           throw String("Please start the Standalone app once to copy the user presets to the shared location");
        
        jassert(factoryPresets.isDirectory());
        
        userPresetDirectory.createDirectory();
        
        factoryPresets.copyDirectoryTo(userPresetDirectory);
    }
    
    return userPresetDirectory;
    
#else
	File presetDir = getAppDataDirectory().getChildFile("User Presets");
    return FileHandlerBase::getFolderOrRedirect(presetDir);
#endif
}


File FrontendHandler::getAdditionalAudioFilesDirectory()
{
#if USE_BACKEND
    return File();
#else
    
#if USE_RELATIVE_PATH_FOR_AUDIO_FILES
	File searchDirectory = getAppDataDirectory().getChildFile("AudioFiles");

	if (!searchDirectory.isDirectory())
		searchDirectory.createDirectory();

	return searchDirectory;

#else
	return File();
#endif
#endif
}


String FrontendHandler::getRelativePathForAdditionalAudioFile(const File& audioFile)
{
#if USE_RELATIVE_PATH_FOR_AUDIO_FILES
	String fileName;

	File searchDirectory = getAdditionalAudioFilesDirectory();

	jassert(searchDirectory.isDirectory());

	if (audioFile.isAChildOf(searchDirectory))
	{
		fileName = "{AUDIO_FILES}" + audioFile.getRelativePathFrom(searchDirectory);
	}
	else
	{
		fileName = audioFile.getFullPathName();
	}
#else
	auto fileName = audioFile.getFullPathName();
#endif

	return fileName;
}


File FrontendHandler::getAudioFileForRelativePath(const String& relativePath)
{
	auto root = getAdditionalAudioFilesDirectory();

	if (root.isDirectory() && relativePath.startsWith("{AUDIO_FILES}"))
	{
		String path = relativePath.fromFirstOccurrenceOf("{AUDIO_FILES}", false, true);

		return root.getChildFile(path);
	}

	return File();
}



const bool FrontendHandler::checkSamplesCorrectlyInstalled()
{
	return getSampleLinkFile().existsAsFile();
}


#if USE_BACKEND
juce::String FrontendHandler::getProjectName()
{
	jassertfalse;
	return {};
}

juce::String FrontendHandler::getCompanyName()
{
	jassertfalse;
	return {};
}

juce::String FrontendHandler::getCompanyWebsiteName()
{
	jassertfalse;
	return {};
}

juce::String FrontendHandler::getCompanyCopyright()
{
	jassertfalse;
	return {};
}

juce::String FrontendHandler::getVersionString()
{
	jassertfalse;
	return {};
}

juce::String FrontendHandler::getAppGroupId()
{
	jassertfalse;
	return {};
}
#endif

StringArray ProjectHandler::recentWorkDirectories = StringArray();

// ================================================================================================================================ Preset Handler

void PresetHandler::checkProcessorIdsForDuplicates(Processor *rootProcessor, bool /*silentMode*/)
{

	bool duplicatesFound;

	do
	{
		duplicatesFound = false;
		Processor::Iterator<Processor> iter(rootProcessor);

		Processor *ip;

		OwnedArray<CountedProcessorId> countedProcessorIds;

		Processor *currentSynth = nullptr;
		Processor *currentChain = nullptr;

		while ((ip = iter.getNextProcessor()) != nullptr)
		{
			const bool isModulatorChain = dynamic_cast<ModulatorChain*>(ip) != nullptr;
			const bool isMidiProcessorChain = dynamic_cast<MidiProcessorChain*>(ip) != nullptr;
			const bool isEffectChain = dynamic_cast<EffectProcessorChain*>(ip) != nullptr;

			if (isModulatorChain || isMidiProcessorChain || isEffectChain)
			{
				currentChain = ip;
				continue;
			}

			if (dynamic_cast<ModulatorSynth*>(ip) != nullptr)
			{
				currentSynth = ip;
			}

			String currentName = ip->getId();

			bool found = false;

			for (int i = 0; i < countedProcessorIds.size(); i++)
			{
				if (countedProcessorIds[i]->processorName == currentName)
				{
					countedProcessorIds[i]->processors.add(ip);
					found = true;
					duplicatesFound = true;
					break;
				}
			}
			if (!found)
			{
				countedProcessorIds.insert(-1, new CountedProcessorId(ip));
			}
		}

		if (duplicatesFound)
		{
			for (int i = 0; i < countedProcessorIds.size(); i++)
			{
				if (countedProcessorIds[i]->processors.size() == 1) continue;

				CountedProcessorId *currentId = countedProcessorIds[i];

				for (int j = 1; j < currentId->processors.size(); j++)
				{
					String newId = currentId->processorName + String(j);

					currentId->processors[j]->setId(newId);
				}
			}
		}
	} 
	while (duplicatesFound);
}

File PresetHandler::getDirectory(Processor *p)
{
#if USE_BACKEND
	if (GET_PROJECT_HANDLER(p).isActive())
	{
		return GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Presets);
	}
	else
	{
		return File();
	}
#else
	ignoreUnused(p);
	return File();
#endif

}


PopupMenu PresetHandler::getAllSavedPresets(int minIndex, Processor *p)
{
    PopupMenu m;
	
#if HISE_IOS
    
#else
	
	File directoryToScan = PresetHandler::getDirectory(p);
	
    for(auto f: RangedDirectoryIterator(directoryToScan, false, "*", File::TypesOfFileToFind::findFilesAndDirectories))
    {
        File directory = f.getFile();
        
        if (directory.isDirectory())
        {
            PopupMenu sub;
            
            for(auto pf: RangedDirectoryIterator(directory, false, "*.hip", File::TypesOfFileToFind::findFiles))
                sub.addItem(minIndex++, pf.getFile().getFileNameWithoutExtension());
            
            m.addSubMenu(directory.getFileName(), sub, true);
            
        }
        else if (directory.hasFileExtension(".hip"))
        {
            m.addItem(minIndex++, directory.getFileNameWithoutExtension());
        }
    }

#endif
    
	return m;
}

void PresetHandler::stripViewsFromPreset(ValueTree& preset)
{
	preset.removeProperty("views", nullptr);
	preset.removeProperty("currentView", nullptr);

	preset.removeProperty("EditorState", nullptr);

	for(int i = 0; i < preset.getNumChildren(); i++)
	{
		ValueTree child = preset.getChild(i);
            
		stripViewsFromPreset(child);
	}
}

File PresetHandler::loadFile(const String& extension)
{
	jassert(extension.isEmpty() || extension.startsWith("*"));

	FileChooser fc("Load File", File(), extension, true);
        
	if(fc.browseForFileToOpen())
	{
            
		return fc.getResult();
	}
	return File();
}

void PresetHandler::saveFile(const String& dataToSave, const String& extension)
{
	jassert(extension.isEmpty() || extension.startsWith("*"));

	FileChooser fc("Save File", File(), extension);
        
	if(fc.browseForFileToSave(true))
	{
		fc.getResult().deleteFile();
		fc.getResult().create();
		fc.getResult().appendText(dataToSave);
	}
        
}

File PresetHandler::getSampleFolder(const String& libraryName)
{
	const bool search = NativeMessageBox::showOkCancelBox(AlertWindow::WarningIcon, "Sample Folder can't be found", "The sample folder for " + libraryName + "can't be found. Press OK to search or Cancel to abort loading");

	if(search)
	{
		FileChooser fc("Searching Sample Folder");

		if(fc.browseForDirectory())
		{
			File sampleFolder = fc.getResult();

				

			return sampleFolder;
		}
	}
		

	return File();
		
		
}

ValueTree PresetHandler::loadValueTreeFromData(const void* data, size_t size, bool wasCompressed)
{
	if (wasCompressed)
	{
		return ValueTree::readFromGZIPData(data, size);
	}
	else
	{
		return ValueTree::readFromData(data, size);
	}
}

void PresetHandler::writeValueTreeAsFile(const ValueTree& v, const String& fileName, bool compressData)
{
	File file(fileName);
	file.deleteFile();
	file.create();

	if (compressData)
	{
		FileOutputStream fos(file);

		GZIPCompressorOutputStream gzos(&fos, 9, false);

		MemoryOutputStream mos;

		v.writeToStream(mos);

		gzos.write(mos.getData(), mos.getDataSize());
		gzos.flush();
	}
	else
	{
		FileOutputStream fos(file);

		v.writeToStream(fos);
	}
}

var PresetHandler::writeValueTreeToMemoryBlock(const ValueTree& v, bool compressData)
{

	juce::MemoryBlock mb;

	if (compressData)
	{
		MemoryOutputStream mos(mb, false);

		GZIPCompressorOutputStream gzos(&mos, 9, false);

		MemoryOutputStream internalMos;

		v.writeToStream(internalMos);

		gzos.write(internalMos.getData(), internalMos.getDataSize());
		gzos.flush();
	}
	else
	{
		MemoryOutputStream mos(mb, false);

		v.writeToStream(mos);
	}

	return var(mb.getData(), mb.getSize());
}

void PresetHandler::setCurrentMainController(void* mc)
{
	currentController = mc;
}

LookAndFeel* PresetHandler::createAlertWindowLookAndFeel()
{
	return HiseColourScheme::createAlertWindowLookAndFeel(currentController);
}

File PresetHandler::getPresetFileFromMenu(int menuIndexDelta, Processor *parent)
{
	File directory = getDirectory(parent);
	
	int i = 0;

    for(auto de: RangedDirectoryIterator(directory, false, "*", File::TypesOfFileToFind::findFilesAndDirectories))
    {
        auto fileToCheck = de.getFile();
        
        if (fileToCheck.isDirectory())
        {
            for(auto pf: RangedDirectoryIterator(fileToCheck, false, "*.hip", File::TypesOfFileToFind::findFiles))
            {
                if (i == menuIndexDelta) return pf.getFile();
                i++;
            }
        }
        
        else if (fileToCheck.hasFileExtension(".hip"))
        {
            if (i == menuIndexDelta) return fileToCheck;
            i++;
        }
    }
	

	return File();
}


Processor *PresetHandler::createProcessorFromPreset(int menuIndexDelta, Processor *parent)
{
	File f = getPresetFileFromMenu(menuIndexDelta, parent);

	if (f.existsAsFile())
	{
		return loadProcessorFromFile(f, parent);
	}
	else
	{
		debugToConsole(parent, "Nothing found");
		jassertfalse;
		return nullptr;
	}

	
}


void PresetHandler::setChanged(Processor *p)
{
    if(p != nullptr)
    {
        p->getMainController()->setChanged();
    }
}

Processor* PresetHandler::createProcessorFromClipBoard(Processor *parent)
{
	try
	{
		String x = SystemClipboard::getTextFromClipboard();
		auto parsedXml = XmlDocument::parse(x);
		ValueTree v = ValueTree::fromXml(*parsedXml);

		if(parsedXml->getStringAttribute("ID") != v.getProperty("ID", String()).toString() )
		{
			jassertfalse;
			debugToConsole(parent, "Clipboard could not be loaded");
			return nullptr;
		};

		String name = v.getProperty("ID", "Unnamed");
		
		Identifier type = v.getProperty("Type", String()).toString();
		FactoryType *t = dynamic_cast<Chain*>(parent)->getFactoryType();
		
		// Look in every processor when inserting from clipboard.

		const bool validType = type.isValid();
		const bool allowedType = t->allowType(type.toString());

		if(!(validType && allowedType) )
		{
			jassertfalse;
			return nullptr;
		}

		Processor *p = MainController::createProcessor(t, type.toString(), name);
		p->restoreFromValueTree(v);

		
		debugToConsole(p, name + " added from Clipboard.");

		return p;
	}
	catch(...)
	{
		debugToConsole(parent, "ERROR WHILE READING");
		jassertfalse;
		return nullptr;
	};
}

void PresetHandler::setUniqueIdsForProcessor(Processor * p)
{
	const String uniqueId = FactoryType::getUniqueName(p);
	p->setId(uniqueId);

	if (ProcessorHelpers::is<ModulatorSynthChain>(p) || ProcessorHelpers::is<ModulatorSynthGroup>(p))
	{
		const int numChildSynths = dynamic_cast<Chain*>(p)->getHandler()->getNumProcessors();

		for (int i = 0; i < numChildSynths; i++)
		{
			Processor *cp = dynamic_cast<Chain*>(p)->getHandler()->getProcessor(i);

			setUniqueIdsForProcessor(cp);
		}
	}
	else
	{
		for (int i = 0; i < p->getNumChildProcessors(); i++)
		{
			if (auto childChain = dynamic_cast<Chain*>(p->getChildProcessor(i)))
			{
				for (int j = 0; j < childChain->getHandler()->getNumProcessors(); j++)
				{
					Processor *cp = childChain->getHandler()->getProcessor(j);
					const String newUniqueId = FactoryType::getUniqueName(cp);
					cp->setId(newUniqueId);
				}
			}
		}
	}
}


juce::ValueTree PresetHandler::changeFileStructureToNewFormat(const ValueTree &v)
{
	ValueTree newTree("Processor");

	newTree.copyPropertiesFrom(v, nullptr);
	newTree.removeProperty("MacroControls", nullptr);
	newTree.removeProperty("EditorState", nullptr);

	newTree.setProperty("Type", v.getType().toString(), nullptr);

	auto editorValueSet = XmlDocument::parse(v.getProperty("EditorState", var::undefined()));

	if (newTree.hasProperty("Content"))
	{
		MemoryBlock b = *v.getProperty("Content", MemoryBlock()).getBinaryData();

		ValueTree restoredContentValues = ValueTree::readFromData(b.getData(), b.getSize());

		newTree.removeProperty("Content", nullptr);

		newTree.addChild(restoredContentValues, -1, nullptr);
	};

	if (editorValueSet != nullptr)
	{
		ValueTree editorStateValueTree = ValueTree::fromXml(*editorValueSet);
		newTree.addChild(editorStateValueTree, -1, nullptr);
	}

	auto macroControlData = XmlDocument::parse(v.getProperty("MacroControls", String()));

	if (macroControlData != nullptr)
	{
		ValueTree macros = ValueTree::fromXml(*macroControlData);
		newTree.addChild(macros, -1, nullptr);
	}

	ValueTree childProcessors("ChildProcessors");

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		ValueTree newChild = changeFileStructureToNewFormat(v.getChild(i));

		childProcessors.addChild(newChild, -1, nullptr);
	}

	newTree.addChild(childProcessors, -1, nullptr);

	return newTree;
}

Processor *PresetHandler::loadProcessorFromFile(File fileName, Processor *parent)
{
	FileInputStream fis(fileName);
	ValueTree v = ValueTree::readFromStream(fis);

	if(v.getType() != Identifier("Processor"))
	{
		debugToConsole(parent, "Old file format detected, converting to new format ...");

		v = PresetHandler::changeFileStructureToNewFormat(v);

	}

	String name = v.getProperty("ID", "Unnamed");

	// Look in every processor when inserting from clipboard.

#if USE_OLD_FILE_FORMAT

	if(fileName.getFileNameWithoutExtension() != v.getProperty("ID", String()).toString() )
	{

		jassertfalse;
		debugToConsole(parent, fileName.getFullPathName() + " could not be loaded");
		return nullptr;
	};

	Processor *p = MainController::createProcessor(dynamic_cast<Chain*>(parent)->getFactoryType(), 
																			String(v.getType()), 
																			name);
#else
	Processor *p = MainController::createProcessor(dynamic_cast<Chain*>(parent)->getFactoryType(), 
																			Identifier(v.getProperty("Type", String())).toString(), 
																			name);
#endif

	

	if(p != nullptr)
	{
		p->restoreFromValueTree(v);
		
		

		debugToConsole(parent, fileName.getFileNameWithoutExtension() + " was loaded");
		return p;
	}
	else
	{
		debugToConsole(parent, "Error with loading " +  fileName.getFileNameWithoutExtension());

		return nullptr;
	}	
}





XmlElement * PresetHandler::buildFactory(FactoryType *t, const String &factoryName)
{
	
	XmlElement *xml = new XmlElement(factoryName);

	for (int j = 0; j < t->getNumProcessors(); j++)
	{

		ScopedPointer<Processor> p = t->createProcessor(j, "X");

		if (p == nullptr) continue;

		// "Hardcoded Master FX", aaarg!
		auto tagName = p->getType().toString().removeCharacters(" ");

		XmlElement *child = new XmlElement(tagName);

		for (int i = 0; i < p->getNumParameters(); i++)
		{
			Identifier id = p->getIdentifierForParameterIndex(i);


			child->setAttribute(Identifier("id" + String(i)), id.toString());
		}
        
        

		xml->addChildElement(child);
	}

	return xml;
}

juce::File PresetHandler::getGlobalScriptFolder(Processor* p)
{
	auto f = dynamic_cast<GlobalSettingManager*>(p->getMainController())->getSettingsObject().getSetting(HiseSettings::Scripting::GlobalScriptPath);

	return File(f.toString());
}

AudioFormatReader * PresetHandler::getReaderForFile(const File &file)
{
	AudioFormatManager afm;
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	return afm.createReaderFor(file);
}

AudioFormatReader * PresetHandler::getReaderForInputStream(InputStream *stream)
{
	AudioFormatManager afm;
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	return afm.createReaderFor(std::unique_ptr<InputStream>(stream));
}

bool forEachScriptComponent(ScriptingApi::Content* c, DynamicObject* obj, const std::function<bool(DynamicObject* obj, ScriptComponent*)>& f, ScriptComponent* toSkip=nullptr)
{
	int numComponents = c->getNumComponents();

	for (int i = 0; i < numComponents; i++)
	{
		auto sc = c->getComponent(i);

		if (sc == toSkip)
			continue;

		auto sip = (bool)sc->getScriptObjectProperty(sc->getIdFor(ScriptingApi::Content::ScriptComponent::saveInPreset));

		if (!sip)
			continue;

		auto isParameter = (bool)sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::isPluginParameter);

		if (!isParameter)
			continue;

		if (!f(obj, sc))
			return false;
	}

	return true;
}

void PresetHandler::checkMetaParameters(Processor* p)
{
	if (auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(p->getMainController()))
	{
		auto prevState = jmp->exportAsValueTree();
		auto content = jmp->getContent();

		try
		{
			StringArray existingNames;

			forEachScriptComponent(content, nullptr, [&existingNames](DynamicObject* obj, ScriptComponent* c)
			{
				auto name = c->getScriptObjectProperty(ScriptComponent::pluginParameterName).toString();

				if (name.isEmpty())
					throw Result::fail(c->getName().toString() + " has undefined pluginParameterName property but is defined as plugin parameter");

				if (existingNames.contains(name))
					throw Result::fail(c->getName() + " has a duplicate plugin parameter ID");

				existingNames.add(name);

				return true;
			});

			forEachScriptComponent(content, nullptr, [content](DynamicObject* obj, ScriptComponent* c)
			{
				auto writeToObj = [](DynamicObject* obj, ScriptComponent* c)
				{
					auto id = c->getName();
					obj->setProperty(id, c->getValue());
					return true;
				};

				auto checkAsExpected = [](DynamicObject* obj, ScriptComponent* c)
				{
					auto expectedValue = obj->getProperty(c->getName());

					if (expectedValue != c->getValue())
						throw c->getName().toString();

					return true;
				};

				auto id = c->getName();
				NormalisableRange<double> range;
				range.start = (double)c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::min);
				range.end = (double)c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::max);

				auto isMeta = (bool)c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::isMetaParameter);

				if (isMeta)
					return true;

				DynamicObject::Ptr values = new DynamicObject();

				forEachScriptComponent(content, values.get(), writeToObj, c);

				var newValue;

				if (dynamic_cast<ScriptingApi::Content::ScriptSlider*>(c) != nullptr ||
					dynamic_cast<ScriptingApi::Content::ScriptPanel*>(c))
				{
					newValue = Random::getSystemRandom().nextDouble() * range.getRange().getLength() + range.start;
				}
				else if (dynamic_cast<ScriptingApi::Content::ScriptButton*>(c) != nullptr)
				{
					newValue = 1 - (int)c->getValue();
				}
				else if (dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(c) != nullptr)
				{
					auto max = (int)c->getScriptObjectProperty(ScriptComponent::max);
					newValue = Random::getSystemRandom().nextInt({ 1, max });
				}

				

				auto index = content->getComponentIndex(id);
				values->setProperty(id, newValue);
				content->getScriptProcessor()->setControlValue(index, newValue);

				Thread::sleep(300);

				try
				{
					forEachScriptComponent(content, values.get(), checkAsExpected, c);
				}
				catch (String& s)
				{
					String e;
					e << c->getName().toString() << " changed " << s << " without being marked as meta parameter";
					throw Result::fail(e);
				}

				return false;
			});
		}
		catch (Result& r)
		{
			showMessageWindow("Meta Parameter Flag not set", r.getErrorMessage(), IconType::Error);
		}

		jmp->restoreFromValueTree(prevState);

	}

	
}

void PresetHandler::writeSampleMapsToValueTree(ValueTree &sampleMapTree, ValueTree &preset)
{
	if (preset.getChildWithName("samplemap").isValid())
	{
		ValueTree copy = preset.getChildWithName("samplemap");

		preset.removeChild(copy, nullptr);

		copy.setProperty("FileName", preset.getProperty("ID"), nullptr);

		sampleMapTree.addChild(copy, -1, nullptr);
	}

	ValueTree childProcessors = preset.getChildWithName("ChildProcessors");

	if (childProcessors.isValid())
	{
		for (int i = 0; i < childProcessors.getNumChildren(); i++)
		{
			ValueTree child = childProcessors.getChild(i);

			writeSampleMapsToValueTree(sampleMapTree, child);
		}
	}
}

AboutPage::AboutPage()
{
#if USE_BACKEND
    
    
    
#endif
    
    addAndMakeVisible(checkUpdateButton = new TextButton("Check Updates"));
};


void AboutPage::paint(Graphics &g)
{
#if USE_BACKEND

	static const unsigned char pathData[] = { 110,109,82,208,82,68,229,80,110,67,98,49,184,85,68,47,189,120,67,94,10,89,68,49,152,128,67,201,198,92,68,254,212,131,67,98,68,131,96,68,203,17,135,67,174,63,100,68,33,176,136,67,25,252,103,68,33,176,136,67,98,143,162,108,68,33,176,136,67,219,41,112,68,
		59,223,134,67,111,146,114,68,80,61,131,67,98,236,249,116,68,201,54,127,67,201,46,118,68,188,148,117,67,201,46,118,68,248,147,105,67,98,201,46,118,68,209,66,99,67,27,223,117,68,25,228,93,67,174,63,117,68,16,120,89,67,98,66,160,116,68,8,12,85,67,164,192,
		115,68,145,45,81,67,236,161,114,68,106,220,77,67,98,68,131,113,68,133,139,74,67,92,47,112,68,10,119,71,67,80,165,110,68,125,159,68,67,98,90,28,109,68,174,199,65,67,178,109,107,68,223,239,62,67,143,154,105,68,16,24,60,67,108,63,101,94,68,244,157,41,67,
		98,43,103,92,68,63,117,38,67,102,110,90,68,143,130,34,67,242,122,88,68,162,197,29,67,98,109,135,86,68,180,8,25,67,199,195,84,68,23,89,19,67,240,47,83,68,10,183,12,67,98,41,156,81,68,254,20,6,67,0,88,80,68,33,176,252,66,150,99,79,68,0,0,235,66,98,43,111,
		78,68,223,79,217,66,254,244,77,68,147,24,197,66,254,244,77,68,160,90,174,66,98,254,244,77,68,207,183,149,66,74,164,78,68,209,162,125,66,2,3,80,68,223,79,83,66,98,170,97,81,68,231,251,40,66,63,69,83,68,43,7,4,66,145,173,85,68,84,227,200,65,98,244,21,88,
		68,82,184,137,65,131,248,90,68,72,225,48,65,80,85,94,68,137,65,212,64,98,29,178,97,68,6,129,13,64,152,94,101,68,0,0,0,0,209,90,105,68,0,0,0,0,98,135,22,110,68,0,0,0,0,125,167,114,68,188,116,99,64,63,13,119,68,92,143,42,65,98,123,116,123,68,209,34,142,
		65,133,59,127,68,117,147,218,65,72,49,129,68,205,76,29,66,108,154,9,123,68,31,197,148,66,98,61,98,120,68,147,88,132,66,106,164,117,68,193,202,110,66,137,209,114,68,10,215,91,66,98,168,254,111,68,84,227,72,66,174,215,108,68,121,105,63,66,209,90,105,68,
		121,105,63,66,98,215,115,101,68,121,105,63,66,217,86,98,68,41,92,76,66,182,3,96,68,150,67,102,66,98,147,176,93,68,254,20,128,66,10,135,92,68,145,237,145,66,10,135,92,68,133,171,168,66,98,10,135,92,68,8,172,180,66,104,225,92,68,33,240,190,66,4,150,93,
		68,76,119,199,66,98,176,74,94,68,119,254,207,66,27,63,95,68,242,146,215,66,84,115,96,68,63,53,222,66,98,125,167,97,68,10,215,228,66,133,11,99,68,141,215,234,66,76,159,100,68,70,54,240,66,98,35,51,102,68,123,148,245,66,154,209,103,68,193,202,250,66,176,
		122,105,68,147,216,255,66,108,174,143,116,68,248,115,17,67,98,66,248,118,68,119,62,21,67,182,43,121,68,127,170,25,67,61,42,123,68,16,184,30,67,98,82,40,125,68,227,197,35,67,164,224,126,68,244,157,41,67,143,42,128,68,0,64,48,67,98,123,228,128,68,12,226,
		54,67,102,118,129,68,193,138,62,67,246,224,129,68,94,58,71,67,98,51,75,130,68,252,233,79,67,82,128,130,68,104,241,89,67,82,128,130,68,98,80,101,67,98,82,128,130,68,176,242,113,67,225,42,130,68,2,203,125,67,246,128,129,68,139,108,132,67,98,10,215,128,
		68,150,243,137,67,164,192,127,68,217,206,142,67,174,55,125,68,53,254,146,67,98,92,175,122,68,145,45,151,67,47,157,119,68,152,126,154,67,213,0,116,68,72,241,156,67,98,139,100,112,68,248,99,159,67,80,77,108,68,80,157,160,67,90,188,103,68,80,157,160,67,
		98,154,65,98,68,80,157,160,67,168,246,92,68,248,163,158,67,133,219,87,68,39,177,154,67,98,98,192,82,68,119,190,150,67,92,63,78,68,51,35,145,67,98,88,74,68,92,223,137,67,108,82,208,82,68,229,80,110,67,99,109,68,107,209,67,162,197,157,67,108,233,134,180,
		67,162,197,157,67,108,233,134,180,67,182,243,181,64,108,68,107,209,67,182,243,181,64,108,68,107,209,67,193,74,2,67,108,37,182,9,68,193,74,2,67,108,37,182,9,68,182,243,181,64,108,82,40,24,68,182,243,181,64,108,82,40,24,68,162,197,157,67,108,37,182,9,68,
		162,197,157,67,108,37,182,9,68,31,37,50,67,108,68,107,209,67,31,37,50,67,108,68,107,209,67,162,197,157,67,99,109,156,212,44,68,182,243,181,64,108,201,70,59,68,182,243,181,64,108,201,70,59,68,162,197,157,67,108,156,212,44,68,162,197,157,67,108,156,212,
		44,68,182,243,181,64,99,109,154,113,162,68,207,119,80,66,108,61,34,145,68,207,119,80,66,108,61,34,145,68,113,61,3,67,108,41,196,159,68,113,61,3,67,108,41,196,159,68,199,171,49,67,108,61,34,145,68,199,171,49,67,108,61,34,145,68,86,142,134,67,108,246,16,
		163,68,86,142,134,67,108,246,16,163,68,162,197,157,67,108,246,232,137,68,162,197,157,67,108,246,232,137,68,182,243,181,64,108,154,113,162,68,182,243,181,64,108,154,113,162,68,207,119,80,66,99,108,154,113,180,68,250,142,153,67,101,0,0 };

	Path title;
	title.loadPathFromData(pathData, sizeof(pathData));

	const float logoWidth = 200.0f;
	const float logoOffset = ((float)getWidth() - logoWidth) / 2.0f;

	g.setColour(Colour(SIGNAL_COLOUR));
	title.scaleToFit(logoOffset, -10.0f, logoWidth, 100.0f, true);
	g.fillPath(title);

	g.setColour(Colours::white);
	g.setFont(GLOBAL_FONT().withHeight(20.0f));
	g.drawText("The open source framework for sample based instruments", 0, 80, getWidth(), 30, Justification::centred);

#else

	g.fillAll(Colour(0xFF252525));
	g.setColour(Colours::white.withAlpha(0.4f));

#endif

	const float textOffset = 150.f;

	infoData.draw(g, Rectangle<float>(40.0f, textOffset, (float)getWidth() - 80.0f, (float)getHeight() - textOffset));
}

UserPresetStateManager::~UserPresetStateManager()
{}

bool UserPresetStateManager::restoreUserPresetState(const ValueTree& root)
{
	auto r = root.getChildWithName(getUserPresetStateId());

	if (r.isValid())
		restoreFromValueTree(r);
	else
		resetUserPresetState();

	return true;
}

void UserPresetStateManager::saveUserPresetState(ValueTree& presetRoot) const
{
	auto v = exportAsValueTree();

	if (!v.isValid())
		return;

	jassert(v.getType() == getUserPresetStateId());
	presetRoot.addChild(v, -1, nullptr);
}


void AboutPage::refreshText()
{
	infoData.clear();

	Font normal = GLOBAL_FONT().withHeight(15.0f);
	Font bold = GLOBAL_BOLD_FONT().withHeight(15.0f);

#if USE_BACKEND

	Colour bright = Colours::white;

	infoData.append("HISE\n", bold, bright);
	infoData.append("Hart Instruments Sampler Engine\n", normal, bright);

	infoData.append("\nVersion: ", bold, bright);
	infoData.append(PresetHandler::getVersionString(), normal, bright);
	infoData.append("\nBuild time: ", bold, bright);
	infoData.append(Time::getCompilationDate().toString(true, false, false, true), normal, bright);
	infoData.append("\nBuild version: ", bold, bright);
	infoData.append(String(BUILD_SUB_VERSION), normal, bright);

	infoData.append("\nCreated by: ", bold, bright);
	infoData.append("Christoph Hart", normal, bright);

#else

	Colour bright(0xFF999999);

#if 0
	infoData.append("Product: ", bold, bright);
	infoData.append(JucePlugin_Name, normal, bright);
	infoData.append("\nVersion: ", bold, bright);
    infoData.append(String(JucePlugin_VersionString), normal, bright);
    infoData.append("\nHISE build version: ", bold, bright);
    infoData.append(GlobalSettingManager::getHiseVersion(), normal, bright);
	infoData.append("\nBuild date: ", bold, bright);
	infoData.append(Time::getCompilationDate().toString(true, false, false, true), normal, bright);
	infoData.append("\nCreated by: ", bold, bright);
	infoData.append(JucePlugin_Manufacturer, normal, bright);
#endif

#if USE_COPY_PROTECTION

	

#endif


#endif

#if USE_IPP
	infoData.append("\n\naccelerated by FFT routines from the IPP library\n", normal, bright);
#endif


	repaint();
}

void AboutPage::buttonClicked(Button *)
{

}

void AboutPage::mouseDown(const MouseEvent &)
{
#if USE_BACKEND
	findParentComponentOfClass<FloatingTilePopup>()->deleteAndClose();
#endif
}

FileHandlerBase::~FileHandlerBase()
{
	pool = nullptr;
}

juce::File FileHandlerBase::getSubDirectory(SubDirectories dir) const
{
	for (const auto& s : subDirectories)
	{
		if (s.directoryType == dir)
			return s.file;
	}

	//jassertfalse;
	return {};
}

juce::String FileHandlerBase::getIdentifier(SubDirectories dir)
{
	switch (dir)
	{
	case SubDirectories::Scripts:			return "Scripts/";
	case SubDirectories::AdditionalSourceCode:	return "AdditionalSourceCode/";
	case SubDirectories::Binaries:			return "Binaries/";
	case SubDirectories::Presets:			return "Presets/";
	case SubDirectories::XMLPresetBackups:	return "XmlPresetBackups/";
	case SubDirectories::Samples:			return "Samples/";
	case SubDirectories::Images:			return "Images/";
	case SubDirectories::AudioFiles:		return "AudioFiles/";
	case SubDirectories::UserPresets:		return "UserPresets/";
	case SubDirectories::SampleMaps:		return "SampleMaps/";
	case SubDirectories::MidiFiles:			return "MidiFiles/";
	case SubDirectories::Documentation:		return "Documentation/";
	case SubDirectories::DspNetworks:		return "DspNetworks";
	case SubDirectories::numSubDirectories:
	default:								jassertfalse; return String();
	}
}

hise::FileHandlerBase::SubDirectories FileHandlerBase::getSubDirectoryForIdentifier(Identifier id)
{
	for (int i = 0; i < (int)SubDirectories::numSubDirectories; i++)
	{
		if (id == Identifier(getIdentifier((SubDirectories)i)))
		{
			return (ProjectHandler::SubDirectories)i;
		}
	}

	return SubDirectories::numSubDirectories;
}

juce::String FileHandlerBase::getFilePath(const String &pathToFile, SubDirectories subDir) const
{
	if (ProjectHandler::isAbsolutePathCrossPlatform(pathToFile)) return pathToFile;

	PoolReference ref(getMainController(), pathToFile, subDir);

	return ref.getFile().getFullPathName();
}

const juce::String FileHandlerBase::getFileReference(const String &absoluteFileName, SubDirectories dir) const
{
	PoolReference ref(getMainController(), absoluteFileName, dir);

	return ref.getReferenceString();
}

Array<File> FileHandlerBase::getFileList(SubDirectories dir, bool sortByTime /*= false*/, bool searchInSubfolders /*= false*/) const
{
	Array<File> filesInDirectory;

	auto wildcard = getWildcardForFiles(dir);

	File presetDir = getSubDirectory(dir);

	filesInDirectory.clear();

	presetDir.findChildFiles(filesInDirectory, File::findFiles, searchInSubfolders, wildcard);

#if JUCE_WINDOWS

	// Remove hidden OSX files (in OSX they are automatically ignored...)
	for (int i = 0; i < filesInDirectory.size(); i++)
	{
		if (filesInDirectory[i].getFileName().startsWith("."))
		{
			filesInDirectory.remove(i);
			i--;
		}
	}
#endif

	if (sortByTime)
	{
		FileModificationComparator comparator;
		filesInDirectory.sort(comparator, false);
	}

	return filesInDirectory;
}

bool FileHandlerBase::isAbsolutePathCrossPlatform(const String &pathName)
{
	if (pathName.startsWithChar('{'))
		return false;

	const bool isAbsoluteWindowsPath = pathName.substring(1).startsWith(":\\");
	const bool isAbsoluteOSXPath = pathName.startsWithChar('/');

	return isAbsoluteWindowsPath || isAbsoluteOSXPath || File::isAbsolutePath(pathName);
}

String FileHandlerBase::getFileNameCrossPlatform(String pathName, bool includeParentDirectory)
{
	if (File::isAbsolutePath(pathName))
	{
		File f(pathName);

		if (includeParentDirectory)
			return f.getRelativePathFrom(f.getParentDirectory()).replace("\\", "/");
		else
			return f.getFileName();	
	}

	if (isAbsolutePathCrossPlatform(pathName))
	{
		pathName = pathName.replace("\\", "/");

		StringArray sa = StringArray::fromTokens(pathName, "/", "");

		if (sa.size() > 2)
		{
			if (includeParentDirectory)
				return sa[sa.size() - 2] + "/" + sa[sa.size() - 1];
			else
				return sa[sa.size() - 1];
		}
	}

	if (pathName.contains("}") && pathName.startsWith("{"))
	{
		// Most likely a Pool reference...
		return pathName.fromFirstOccurrenceOf("}", false, false);
	}
	
	return pathName;
}

juce::File FileHandlerBase::getLinkFile(const File &subDirectory)
{
#if JUCE_MAC
	return subDirectory.getChildFile("LinkOSX");
#elif JUCE_LINUX
	return subDirectory.getChildFile("LinkLinux");
#else
	return subDirectory.getChildFile("LinkWindows");
#endif
}

File FileHandlerBase::getFolderOrRedirect(const File& folder)
{
    auto lf = getLinkFile(folder);
    
    if(lf.existsAsFile())
    {
        auto rd = File(lf.loadFileAsString());
        
        if(rd.isDirectory())
            return rd;
    }
    
    return folder;
}

void FileHandlerBase::createLinkFile(SubDirectories dir, const File &relocation)
{
	File subDirectory = getRootFolder().getChildFile(getIdentifier(dir));

	createLinkFileInFolder(subDirectory, relocation);
}

void FileHandlerBase::createLinkFileInFolder(const File& source, const File& target)
{
	File linkFile = getLinkFile(source);

	if (linkFile.existsAsFile())
	{
        if(linkFile.loadFileAsString() == target.getFullPathName())
            return;
        
		if (!target.isDirectory())
		{
			linkFile.deleteFile();
			return;
		}

		if (!PresetHandler::showYesNoWindowIfMessageThread("Already there", "Link redirect file exists. Do you want to replace it?", true))
		{
			return;
		}
	}

	if(!target.isDirectory())
		return;

	linkFile.create();

	linkFile.replaceWithText(target.getFullPathName());
}


void FileHandlerBase::createLinkFileToGlobalSampleFolder(const String& suffix)
{
	auto linkFile = getLinkFile(getRootFolder().getChildFile(getIdentifier(Samples)));
	
	if (!linkFile.existsAsFile())
		linkFile.create();

	linkFile.replaceWithText("{GLOBAL_SAMPLE_FOLDER}" + suffix);

	checkSubDirectories();
}

Array<FileHandlerBase::SubDirectories> FileHandlerBase::getSubDirectoryIds() const
{
	Array<SubDirectories> subDirs;

	for (int i = 0; i < FileHandlerBase::numSubDirectories; i++)
	{
		subDirs.add((FileHandlerBase::SubDirectories)i);
	}

	return subDirs;
}

juce::String FileHandlerBase::getWildcardForFiles(SubDirectories directory)
{
	switch (directory)
	{
	case hise::FileHandlerBase::Samples:
	case hise::FileHandlerBase::AudioFiles:				return "*.wav;*.aif;*.aiff;*.hlac;*.flac;*.WAV;*.AIF;*.AIFF;*.HLAC;*.FLAC";
	case hise::FileHandlerBase::Images:					return "*.jpg;*.png;*.PNG;*.JPG";
	case hise::FileHandlerBase::SampleMaps:				return "*.xml";
	case hise::FileHandlerBase::UserPresets:			return "*.preset";
	case hise::FileHandlerBase::Scripts:				return "*.js";
	case hise::FileHandlerBase::Presets:				return "*.hip";
	case hise::FileHandlerBase::XMLPresetBackups:		return "*.xml";
	case hise::FileHandlerBase::MidiFiles:				return "*.mid;*.MID";
	case hise::FileHandlerBase::DspNetworks:			return "*.xml";
	case hise::FileHandlerBase::Binaries:				
	case hise::FileHandlerBase::AdditionalSourceCode:
	case hise::FileHandlerBase::numSubDirectories:		
	default:											return "*.*";
	}
}

FileHandlerBase::FileHandlerBase(MainController* mc_) :
	ControlledObject(mc_),
	pool(new PoolCollection(mc_, this))
{

}



void FileHandlerBase::checkSubDirectories()
{
	subDirectories.clear();

	if (!getRootFolder().isDirectory())
		return;

	auto subDirList = getSubDirectoryIds();

	for (auto dir : subDirList)
	{
		File f = checkSubDirectory(dir);
		jassert(f.exists() && f.isDirectory());

		File subDirectory = getRootFolder().getChildFile(getIdentifier(dir));
		File linkFile = getLinkFile(subDirectory);
		const bool redirected = linkFile.existsAsFile();

		subDirectories.add({ dir, redirected, f });
	}
}


void FileHandlerBase::checkAllSampleMaps()
{
	Array<File> sampleMaps;
	Array<File> samples;

	getSubDirectory(SubDirectories::Samples).findChildFiles(samples, File::findFiles, true);
	getSubDirectory(SubDirectories::SampleMaps).findChildFiles(sampleMaps, File::findFiles, true, "*.xml;*.XML");

	String falseName;

	for (int i = 0; i < sampleMaps.size(); i++)
	{
		auto xml = XmlDocument::parse(sampleMaps[i]);

		if (xml != nullptr)
		{
			ValueTree v = ValueTree::fromXml(*xml);

			const String id = v.getProperty("ID").toString();

			if (id != sampleMaps[i].getFileNameWithoutExtension())
			{
				PresetHandler::showMessageWindow("Mismatching SampleMap ID", "The SampleMap " + sampleMaps[i].getFileName() + " does not have the correct ID", PresetHandler::IconType::Error);
				return;
			}

			falseName = SampleMap::checkReferences(getMainController(), v, getSubDirectory(SubDirectories::Samples), samples);

			if (falseName.isNotEmpty())
				break;
		}
	}

	if (falseName.isEmpty())
		PresetHandler::showMessageWindow("All sample references are valid", "All sample maps have been scanned", PresetHandler::IconType::Info);
	else
		PresetHandler::showMessageWindow("Missing samples found", "The sample " + falseName + " wasn't found.", PresetHandler::IconType::Error);
}


juce::Result FileHandlerBase::updateSampleMapIds(bool silentMode)
{
	File sampleMapRoot = getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);
	File sampleRoot = getSubDirectory(ProjectHandler::SubDirectories::Samples);

	bool didSomething = false;

	Array<File> sampleMapFiles;

	sampleMapRoot.findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

	for (int i = 0; i < sampleMapFiles.size(); i++)
	{
		auto xml = XmlDocument::parse(sampleMapFiles[i]);

		if (xml != nullptr && xml->hasAttribute("ID"))
		{
			const String id = xml->getStringAttribute("ID");
			const String relativePath = sampleMapFiles[i].getRelativePathFrom(sampleMapRoot).replace("\\", "/").upToFirstOccurrenceOf(".xml", false, true);

			if (id != relativePath)
			{
				if (silentMode || PresetHandler::showYesNoWindow("Mismatch detected", "Filename: \"" + relativePath + "\", ID: \"" + id + "\"\nDo you want to update the ID and rename the monolith samples?"))
				{
					xml->setAttribute("ID", relativePath);
					sampleMapFiles[i].replaceWithText(xml->createDocument(""));

					didSomething = true;

					Array<File> sampleFiles;

					String oldSampleFileName = id.replace("/", "_");

					sampleRoot.findChildFiles(sampleFiles, File::findFiles, false);

					for (auto f : sampleFiles)
					{
						if (f.getFileNameWithoutExtension() == oldSampleFileName)
						{
							File newFileName = sampleRoot.getChildFile(relativePath.replace("/", "_") + f.getFileExtension());

							if (!newFileName.existsAsFile())
							{
								f.moveFileTo(newFileName);

								

								if(!silentMode)
									PresetHandler::showMessageWindow("Sample file renamed", "The sample with the name " + f.getFileName() + " was renamed to " + newFileName.getFileName(), PresetHandler::IconType::Info);
							}
							else
							{
								return Result::fail("The sample with the name " + newFileName.getFullPathName() + " already exists");
							}
						}
					}
				}
			}

		}
		else
		{
			return Result::fail("The samplemap " + sampleMapFiles[i].getFullPathName() + " is corrupt");
		}
	}

	if (didSomething)
	{
		pool->getSampleMapPool().refreshPoolAfterUpdate();
	}

	return Result::ok();
}

juce::File FileHandlerBase::checkSubDirectory(SubDirectories dir)
{
	
	File subDirectory = getRootFolder().getChildFile(getIdentifier(dir));

	jassert(subDirectory.exists());

	File childFile = getLinkFile(subDirectory);

	if (childFile.existsAsFile())
	{
		String absolutePath = childFile.loadFileAsString();

		if (ProjectHandler::isAbsolutePathCrossPlatform(absolutePath))
		{
			if (!File(absolutePath).exists())
			{
				if (PresetHandler::showYesNoWindow("Missing Sample Folder", "The sample relocation folder does not exist. Press OK to choose a new location or Cancel to ignore this.", PresetHandler::IconType::Warning))
				{
					FileChooser fc("Redirect sample folder to the following location");

					if (fc.browseForDirectory())
					{
						File f = fc.getResult();

						createLinkFile(SubDirectories::Samples, f);

						return f;
					}
				}
			}

			return File(absolutePath);
		}
		else if (absolutePath.contains("{GLOBAL_SAMPLE_FOLDER}"))
		{
#if USE_BACKEND
			if (auto gs = dynamic_cast<GlobalSettingManager*>(getMainController()))
			{
				auto path = gs->getSettingsObject().getSetting(HiseSettings::Other::GlobalSamplePath);

				if (isAbsolutePathCrossPlatform(path))
				{
					File globalSamplePath(path);

					auto childPath = absolutePath.fromFirstOccurrenceOf("{GLOBAL_SAMPLE_FOLDER}", false, false);
					return globalSamplePath.getChildFile(childPath);
				}
			}
			else
			{
				// If you hit this assertion it means that the main controller is not fully initialised.
				// Just wait until the constructor went through...
				jassertfalse;
			}
#else
			// The global sample path will resolve to the default sample location for the end user

			return FrontendHandler::getSampleLocationForCompiledPlugin();
#endif
		}
	}

	if (subDirectory.isDirectory())
	{
		return subDirectory;
	}
	else if (subDirectory.isSymbolicLink())
	{
		return subDirectory.getLinkedTarget();
	}
	else
	{
		const String fileExtension = subDirectory.getFileExtension();
		jassertfalse;

		return File();
	}
}

ProjectHandler::ProjectHandler(MainController* mc_):
	FileHandlerBase(mc_)
{
		
}

ProjectHandler::Listener::~Listener()
{}

const StringArray& ProjectHandler::getRecentWorkDirectories()
{ return recentWorkDirectories; }

File ProjectHandler::getRootFolder() const
{ return getWorkDirectory(); }

bool ProjectHandler::isRedirected(ProjectHandler::SubDirectories dir) const
{
	return subDirectories[(int)dir].isReference;
}

void ProjectHandler::addListener(Listener* newProjectListener, bool sendWithInitialValue)
{
	listeners.addIfNotAlreadyThere(newProjectListener);
        
	if(sendWithInitialValue && currentWorkDirectory.isDirectory())
		newProjectListener->projectChanged(currentWorkDirectory);
}

void ProjectHandler::removeListener(Listener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

void FileHandlerBase::exportAllPoolsToTemporaryDirectory(ModulatorSynthChain* chain, DialogWindowWithBackgroundThread::LogData* logData)
{
	ignoreUnused(chain, logData);

#if USE_BACKEND
	auto folder = getTempFolderForPoolResources();

	if (!folder.isDirectory())
		folder.createDirectory();

	File imageOutputFile, sampleOutputFile, samplemapFile, midiOutputFile;

	samplemapFile = getTempFileForPool(SampleMaps);
	imageOutputFile = getTempFileForPool(Images);
	sampleOutputFile = getTempFileForPool(AudioFiles);
	midiOutputFile = getTempFileForPool(MidiFiles);

	loadOtherReferencedImages(chain);

	if (Thread::currentThreadShouldExit())
		return;

	
	
	
	

	auto previousLogger = Logger::getCurrentLogger();

	ScopedPointer<Logger> outputLogger = new ConsoleLogger(chain);

	if(!CompileExporter::isExportingFromCommandLine())
		Logger::setCurrentLogger(outputLogger);

	auto* progress = logData != nullptr ? &logData->progress : nullptr;

    sampleOutputFile.deleteFile();
    
	if (logData != nullptr) logData->logFunction("Export audio files");
	chain->getMainController()->getCurrentAudioSampleBufferPool()->getDataProvider()->writePool(new FileOutputStream(sampleOutputFile), progress);

    if (Thread::currentThreadShouldExit())
        return;
    
    imageOutputFile.deleteFile();
    
	if (logData != nullptr) logData->logFunction("Export image files");
	chain->getMainController()->getCurrentImagePool()->getDataProvider()->writePool(new FileOutputStream(imageOutputFile), progress);

    if (Thread::currentThreadShouldExit())
        return;
    
    samplemapFile.deleteFile();
    
	if (logData != nullptr) logData->logFunction("Export samplemap files");
	chain->getMainController()->getCurrentSampleMapPool()->getDataProvider()->writePool(new FileOutputStream(samplemapFile), progress);

    if (Thread::currentThreadShouldExit())
        return;
    
    midiOutputFile.deleteFile();
    
	if (logData != nullptr) logData->logFunction("Export MIDI files");
	chain->getMainController()->getCurrentMidiFilePool()->getDataProvider()->writePool(new FileOutputStream(midiOutputFile), progress);

    Logger::setCurrentLogger(previousLogger);

	outputLogger = nullptr;
#else

	jassertfalse;

#endif
}


juce::File FileHandlerBase::getTempFolderForPoolResources() const
{
	return getRootFolder().getChildFile("PooledResources");
}


juce::File FileHandlerBase::getTempFileForPool(SubDirectories dir) const
{
	auto parent = getTempFolderForPoolResources();

	switch (dir)
	{
	case Images:		return parent.getChildFile("ImageResources.dat");
	case SampleMaps:	return parent.getChildFile("SampleMaps.dat");
	case AudioFiles:	return parent.getChildFile("AudioResources.dat");
	case MidiFiles:		return parent.getChildFile("MidiFiles.dat");
	default:			jassertfalse;
						break;
	}

	return {};
}

void FileHandlerBase::loadOtherReferencedImages(ModulatorSynthChain* chainToExport)
{
	auto mc = chainToExport->getMainController();
	auto& handler = GET_PROJECT_HANDLER(chainToExport);

	const bool hasCustomSkin = handler.getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("keyboard").isDirectory();

	if (!hasCustomSkin)
		return;

	auto pool = mc->getCurrentImagePool();

	Array<PooledImage> images;

	for (int i = 0; i < 12; i++)
	{
		PoolReference upRef(mc, "{PROJECT_FOLDER}keyboard/up_" + String(i) + ".png", ProjectHandler::SubDirectories::Images);

		jassert(upRef.isValid());

		images.add(pool->loadFromReference(upRef, PoolHelpers::LoadAndCacheStrong));

		PoolReference downRef(mc, "{PROJECT_FOLDER}keyboard/down_" + String(i) + ".png", ProjectHandler::SubDirectories::Images);

		jassert(downRef.isValid());
		images.add(pool->loadFromReference(downRef, PoolHelpers::LoadAndCacheStrong));
	}

	const bool hasAboutPageImage = handler.getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("about.png").existsAsFile();

	if (hasAboutPageImage)
	{
		PoolReference aboutRef(mc, "{PROJECT_FOLDER}about.png", ProjectHandler::SubDirectories::Images);

		images.add(pool->loadFromReference(aboutRef, PoolHelpers::LoadAndCacheStrong));
	}
}

MessageWithIcon::MessageWithIcon(PresetHandler::IconType type, LookAndFeel* laf, const String &message) :
	t(type),
	r(message, nullptr)
{
	image = defaultLaf.createIcon(type);

	auto s = defaultLaf.getAlertWindowMarkdownStyleData();

	s.f = laf->getAlertWindowFont();
	s.boldFont = laf->getAlertWindowTitleFont();

	if (auto laf_ = dynamic_cast<LookAndFeelMethods*>(laf))
	{
		s = laf_->getAlertWindowMarkdownStyleData();
		image = laf_->createIcon(type);
	}
		
	r.setStyleData(s);

	auto w = jmin(s.f.getStringWidthFloat(message) + 30.0f, 600.0f);

	auto height = (int)r.getHeightForWidth(w);

	setSize((int)w + image.getWidth(), jmax(height, image.getHeight()));
}

void MessageWithIcon::paint(Graphics &g)
{
	if (auto alertWindow = findParentComponentOfClass<AlertWindow>())
	{
		if (auto lm = dynamic_cast<LookAndFeelMethods*>(&alertWindow->getLookAndFeel()))
		{
			lm->paintMessage(*this, g);
			return;
		}
	}

	defaultLaf.paintMessage(*this, g);
}

void MessageWithIcon::LookAndFeelMethods::paintMessage(MessageWithIcon& icon, Graphics& g)
{
	auto img = createIcon(icon.t);

	if (img.isValid())
	{
		g.drawImageAt(img, 0, 0);
	}

	g.setColour(Colour(0xFF999999));

	auto b = icon.getLocalBounds();
	b.removeFromLeft(img.getWidth());
	icon.r.draw(g, b.toFloat());
}

hise::MarkdownLayout::StyleData MessageWithIcon::LookAndFeelMethods::getAlertWindowMarkdownStyleData()
{
	auto s = MarkdownLayout::StyleData::createDarkStyle();
	s.fontSize = 14.0f;
	s.textColour = Colours::white.withAlpha(0.8f);
	return s;
}

juce::Image MessageWithIcon::LookAndFeelMethods::createIcon(PresetHandler::IconType type)
{
	switch (type)
	{
	case PresetHandler::IconType::Info: return ImageCache::getFromMemory(
		BinaryData::infoInfo_png, BinaryData::infoInfo_pngSize);
	case PresetHandler::IconType::Warning: return ImageCache::getFromMemory(BinaryData::infoWarning_png, BinaryData::infoWarning_pngSize);
	case PresetHandler::IconType::Question: return ImageCache::getFromMemory(BinaryData::infoQuestion_png, BinaryData::infoQuestion_pngSize);
	case PresetHandler::IconType::Error: return ImageCache::getFromMemory(BinaryData::infoError_png, BinaryData::infoError_pngSize);
	case PresetHandler::IconType::numIconTypes: 
	default:
		jassertfalse;
		return Image(); 
		
	}
}

void removePropertyRecursive(NamedValueSet& removedProperties, String currentPath, ValueTree v, const Identifier& id)
{
	if (!currentPath.isEmpty())
		currentPath << ":";

	currentPath << v.getType();

	if (v.hasProperty(id))
	{
		auto value = v.getProperty(id);
		v.removeProperty(id, nullptr);
		removedProperties.set(Identifier(currentPath + ":" + id.toString()), value);
	}

	for (auto c : v)
		removePropertyRecursive(removedProperties, currentPath, c, id);
}

ModuleStateManager::StoredModuleData::StoredModuleData(var moduleId, Processor* pToRestore) :
	p(pToRestore)
{
	if (moduleId.isString())
		id = moduleId.toString();
	else
	{
		id = moduleId["ID"].toString();

		auto rp = moduleId["RemovedProperties"];
		auto rc = moduleId["RemovedChildElements"];

		if (rp.isArray() || rc.isArray())
		{
			auto v = p->exportAsValueTree();

			if (rp.isArray())
			{
				for (auto propertyToRemove : *rp.getArray())
				{
					auto pid_ = propertyToRemove.toString();
					if (pid_.isNotEmpty())
					{
						Identifier pid(pid_);
						removePropertyRecursive(removedProperties, {}, v, pid);
					}
				}
			}

			if (rc.isArray())
			{
				for (auto childToRemove : *rc.getArray())
				{
					auto pid_ = childToRemove.toString();

					if (pid_.isNotEmpty())
					{
						Identifier pid(pid_);
						removedChildElements.add(v.getChildWithName(pid).createCopy());
					}
				}
			}

			removedProperties.remove(Identifier("Processor:ID"));
		}
	}
}

void restorePropertiesRecursive(ValueTree v, StringArray path, const var& value, bool restore)
{
	if (path.size() == 2)
	{
		if (Identifier(path[0]) == v.getType())
		{
			auto id = Identifier(path[1]);

			if (restore)
				v.setProperty(id, value, nullptr);
			else
				v.removeProperty(id, nullptr);
		}
	}
	else
	{
		path.remove(0);

		for (auto c : v)
			restorePropertiesRecursive(c, path, value, restore);
	}
}

void ModuleStateManager::StoredModuleData::stripValueTree(ValueTree& v)
{
	for (const auto& rp : removedProperties)
	{
		auto path = StringArray::fromTokens(rp.name.toString(), ":", "\"");

		restorePropertiesRecursive(v, path, {}, false);
	}


	for (const auto& rc : removedChildElements)
	{
		auto cToRemove = v.getChildWithName(rc.getType());

		if (cToRemove.isValid())
			v.removeChild(cToRemove, nullptr);
	}
}

void ModuleStateManager::StoredModuleData::restoreValueTree(ValueTree& v)
{
	stripValueTree(v);

	for (const auto& rp : removedProperties)
	{
		auto path = StringArray::fromTokens(rp.name.toString(), ":", "\"");
		auto value = rp.value;
		restorePropertiesRecursive(v, path, value, true);
	}


	for (const auto& rc : removedChildElements)
		v.addChild(rc.createCopy(), -1, nullptr);
}


juce::ValueTree ModuleStateManager::exportAsValueTree() const
{
	if (modules.isEmpty())
		return {};

	ValueTree v(getUserPresetStateId());

	for (auto ms : modules)
	{
		auto id = ms->id;

		if (auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), id))
		{
			auto mTree = p->exportAsValueTree();

			mTree.removeChild(mTree.getChildWithName("EditorStates"), nullptr);

			ms->stripValueTree(mTree);

			v.addChild(mTree, -1, nullptr);
		}
	}

	return v;
}

void ModuleStateManager::restoreFromValueTree(const ValueTree &v)
{
	auto chain = getMainController()->getMainSynthChain();
	
	bool didSomething = false;

	for (auto m : v)
	{
		auto id = m["ID"].toString();

		bool isModuleState = false;

		for(auto ms: modules)
		{
			if(ms->id == id)
			{
				didSomething = true;
				isModuleState = true;
				break;
			}
				
		}

		if(!isModuleState)
			continue;
		
		auto p = ProcessorHelpers::getFirstProcessorWithName(chain, id);

		if (p != nullptr)
		{
			auto mcopy = m.createCopy();
			
			for (auto ms : modules)
			{
				if (ms->id == id)
				{
					ms->restoreValueTree(mcopy);
					break;
				}
			}

			if (p->getType().toString() == mcopy["Type"].toString())
			{
				p->restoreFromValueTree(mcopy);
				p->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Preset, dispatch::sendNotificationAsync);
			}
		}
	}

	auto& uph = chain->getMainController()->getUserPresetHandler();

	if (didSomething && uph.isUsingCustomDataModel())
	{
		auto numDataObjects = uph.getNumCustomAutomationData();

		// We might need to update the custom automation data values.
		for (int i = 0; i < numDataObjects; i++)
		{
			uph.getCustomAutomationData(i)->updateFromConnectionValue(0);
		}
	}
}

} // namespace hise
