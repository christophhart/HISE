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

#if USE_COPY_PROTECTION

String Unlocker::getProductID()
{
	return String(ProjectHandler::Frontend::getProjectName()) + " " + String(ProjectHandler::Frontend::getVersionString());
}

String Unlocker::getWebsiteName()
{
	return ProjectHandler::Frontend::getCompanyWebsiteName();
}

URL Unlocker::getServerAuthenticationURL()
{
	return URL("http://hise.audio/licence/key_file_generator.php");
}

#endif

void CopyPasteTarget::grabCopyAndPasteFocus()
{   
#if USE_BACKEND
    Component *thisAsComponent = dynamic_cast<Component*>(this);
    
    if(thisAsComponent)
    {
		BackendRootWindow *editor = GET_BACKEND_ROOT_WINDOW(thisAsComponent);
        
        if(editor != nullptr)
        {
            editor->setCopyPasteTarget(this);
            isSelected = true;
            thisAsComponent->repaint();
        }
    }
    else
    {
        // You can only use components as CopyAndPasteTargets!
        jassertfalse;
    }
#endif
}


void CopyPasteTarget::dismissCopyAndPasteFocus()
{
#if USE_BACKEND
	Component *thisAsComponent = dynamic_cast<Component*>(this);

	if (thisAsComponent)
	{
		BackendRootWindow *editor = GET_BACKEND_ROOT_WINDOW(thisAsComponent);

		if (editor != nullptr && isSelected)
		{
			editor->setCopyPasteTarget(nullptr);
			isSelected = false;
			thisAsComponent->repaint();
		}
	}
	else
	{
		// You can only use components as CopyAndPasteTargets!
		jassertfalse;
	}
#endif
}



UserPresetData::UserPresetData(MainController* mc_) :
mc(mc_)
{
	userPresets = new PresetCategory("User Presets");

	refreshPresetFileList();
}


UserPresetData::~UserPresetData()
{
	mc = nullptr;
	listeners.clear();
}

void UserPresetData::getCurrentPresetIndexes(int &category, int &preset, String &name) const
{
	category = currentCategoryIndex;
	preset = currentPresetIndex;
	name = currentName;
}

void UserPresetData::addFactoryPreset(const String &name, const String &category, int id, ValueTree &v)
{
	int index = -1;

	for (int i = 0; i < factoryPresetCategories.size(); i++)
	{
		if (factoryPresetCategories[i]->name == category)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		PresetCategory *newCategory = new PresetCategory(category);
		newCategory->presets.add(Entry(name, id, v));
		factoryPresetCategories.add(newCategory);
	}
	else
	{
		factoryPresetCategories[index]->presets.add(Entry(name, id, v));
	}
}

void UserPresetData::addUserPreset(const String &name, int id, const ValueTree &v)
{
	userPresets->presets.add(Entry(name, id, v));
}

void UserPresetData::fillCategoryList(StringArray& listToFill) const
{
	listToFill.clear();

	for (int i = 0; i < factoryPresetCategories.size(); i++)
	{
		listToFill.add(factoryPresetCategories[i]->name);
	}

	listToFill.add(userPresets->name);
}





void UserPresetData::fillPresetList(StringArray& listToFill, int categoryIndex) const
{
	listToFill.clear();

	const PresetCategory* c = getPresetCategory(categoryIndex);

	if (c != nullptr)
	{
		for (int i = 0; i < c->presets.size(); i++)
		{
			listToFill.add(c->presets[i].name);
		}
	}
}


void UserPresetData::addListener(Listener *newListener) const
{
	listeners.addIfNotAlreadyThere(newListener);
}

void UserPresetData::removeListener(Listener *listenerToRemove) const
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

const UserPresetData::PresetCategory* UserPresetData::getPresetCategory(int index) const
{
	if (index == factoryPresetCategories.size())
	{
		return userPresets;
	}
	else if (index < factoryPresetCategories.size())
	{
		return factoryPresetCategories[index];
	}
	else return nullptr;
}

void UserPresetData::loadPreset(int categoryToLoad, int presetToLoad) const
{
	PresetCategory* c = nullptr;

	if (categoryToLoad == factoryPresetCategories.size())
	{
		c = userPresets;
	}
	else
	{
		c = factoryPresetCategories[categoryToLoad];
	}

	if (c != nullptr)
	{
		if (presetToLoad < c->presets.size())
		{
			Entry* e = &c->presets.getReference(presetToLoad);

			if (e != nullptr)
			{
				currentCategoryIndex = categoryToLoad;
				currentPresetIndex = presetToLoad;
				currentName = e->name;

				for (int i = 0; i < listeners.size(); i++)
				{
					listeners[i]->presetLoaded(currentCategoryIndex, currentPresetIndex, e->name);
				}

				UserPresetHelpers::loadUserPreset(mc->getMainSynthChain(), e->v);
			}
		}
	}
}


void UserPresetData::loadNextPreset() const
{
	const PresetCategory* c = getPresetCategory(currentCategoryIndex);

	if (c != nullptr)
	{
		const int nextPresetIndex = currentPresetIndex + 1;

		if (nextPresetIndex < c->presets.size())
			loadPreset(currentCategoryIndex, nextPresetIndex);
		else
		{
			const PresetCategory* category = getPresetCategory(currentCategoryIndex + 1);

			if (category != nullptr)
				loadPreset(currentCategoryIndex + 1, 0);
		}
	}
}

void UserPresetData::loadPreviousPreset() const
{
	const PresetCategory* c = getPresetCategory(currentCategoryIndex);

	if (c != nullptr)
	{
		const int prevPresetIndex = currentPresetIndex - 1;

		if (prevPresetIndex >= 0)
			loadPreset(currentCategoryIndex, prevPresetIndex);

		else
		{
			const PresetCategory* category = getPresetCategory(currentCategoryIndex - 1);

			if (category != nullptr)
				loadPreset(currentCategoryIndex - 1, category->presets.size() - 1);
		}
	}
}

void UserPresetData::refreshPresetFileList()
{
#if USE_BACKEND

	Array<File> fileList;

	ProjectHandler *handler = &GET_PROJECT_HANDLER(mc->getMainSynthChain());

	handler->getFileList(fileList, ProjectHandler::SubDirectories::UserPresets, "*.preset", false, true);

	factoryPresetCategories.clear();
	userPresets->presets.clear();

	for (int i = 0; i < fileList.size(); i++)
	{
		const File parentDirectory = fileList[i].getParentDirectory();
		const File presetDirectory = handler->getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
		const bool useCategory = (presetDirectory != parentDirectory);
		const String categoryName = useCategory ? fileList[i].getParentDirectory().getFileName() : "Uncategorized";

		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileList[i]);

		if (xml != nullptr)
		{
			ValueTree v = ValueTree::fromXml(*xml);
			addFactoryPreset(fileList[i].getFileNameWithoutExtension(), categoryName, i + 1, v);
		}
	}

#else

	ValueTree factoryPresets = dynamic_cast<FrontendDataHolder*>(mc)->getValueTree(ProjectHandler::SubDirectories::UserPresets);

	factoryPresetCategories.clear();
	userPresets->presets.clear();

	for (int i = 1; i < factoryPresets.getNumChildren(); i++)
	{
		ValueTree c = factoryPresets.getChild(i);

		addFactoryPreset(c.getProperty("FileName"), c.getProperty("Category"), i, c);
	}

    File userPresetDirectory;
    
    try
    {
        userPresetDirectory = ProjectHandler::Frontend::getUserPresetDirectory();
    }

    catch(String& s)
    {
        mc->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
        return;
    }


	Array<File> newUserPresets;
	userPresetDirectory.findChildFiles(newUserPresets, File::findFiles, false, "*.preset");

	for (int i = 0; i < newUserPresets.size(); i++)
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(newUserPresets[i]);

		if (xml != nullptr)
		{
			ValueTree v = ValueTree::fromXml(*xml);

			addUserPreset(newUserPresets[i].getFileNameWithoutExtension(), i, v);
		}
	}

#endif

}



void UserPresetHelpers::saveUserPreset(ModulatorSynthChain *chain, const String& targetFile/*=String()*/)
{
#if USE_BACKEND

	const String version = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(chain));

	SemanticVersionChecker versionChecker(version, version);

	if (!versionChecker.newVersionNumberIsValid())
	{
		PresetHandler::showMessageWindow("Invalid version number", "You need semantic versioning (something like 1.0.0) in order to support user presets", PresetHandler::IconType::Error);
		return;
	}

	if (!GET_PROJECT_HANDLER(chain).isActive()) return;

	File userPresetDir = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

#else

    File userPresetDir;
    
    try
    {
        userPresetDir = ProjectHandler::Frontend::getUserPresetDirectory();
    }
    catch(String& s)
    {
        chain->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
        return;
    }
    
	 

#endif

	
	File presetFile = File(targetFile);
	
	if (presetFile.existsAsFile() && PresetHandler::showYesNoWindow("Confirm overwrite", "Do you want to overwrite the preset (Press cancel to create a new user preset?"))
	{
		presetFile.deleteFile();
		
	}
	
	if (!presetFile.existsAsFile())
	{
		Processor::Iterator<JavascriptMidiProcessor> iter(chain);

		ValueTree autoData = chain->getMainController()->getMacroManager().getMidiControlAutomationHandler()->exportAsValueTree();

		while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
		{
			if (!sp->isFront()) continue;

			ValueTree v = sp->getScriptingContent()->exportAsValueTree();

			v.setProperty("Processor", sp->getId(), nullptr);

			ValueTree preset = ValueTree("Preset");

			preset.setProperty("Version", getCurrentVersionNumber(chain), nullptr);
			preset.addChild(v, -1, nullptr);
			preset.addChild(autoData, -1, nullptr);

			ScopedPointer<XmlElement> xml = preset.createXml();

			presetFile.replaceWithText(xml->createDocument(""));

			chain->getMainController()->getUserPresetHandler().sendRebuildMessage();

			return;
		}
	}
}

void UserPresetHelpers::loadUserPreset(ModulatorSynthChain *chain, const File &fileToLoad)
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);
    
    if(xml != nullptr)
    {
		if (!checkVersionNumber(chain, *xml))
		{
			if (PresetHandler::showYesNoWindow("Update user preset", "This user preset was built with a previous version. Do you want to update it?", PresetHandler::IconType::Question))
			{
				addMissingControlsToUserPreset(chain, fileToLoad);
				updateVersionNumber(chain, fileToLoad);

				xml = XmlDocument::parse(fileToLoad);
			}
		}

        ValueTree parent = ValueTree::fromXml(*xml);
        
		chain->getMainController()->getDebugLogger().logMessage("### Loading user preset " + fileToLoad.getFileNameWithoutExtension() + "\n");

        if (parent.isValid())
        {
			chain->getMainController()->getUserPresetHandler().setCurrentlyLoadedFile(fileToLoad);

            loadUserPreset(chain, parent);
        }
    }
}

void UserPresetHelpers::loadUserPreset(ModulatorSynthChain* chain, const ValueTree &parent)
{
	chain->getMainController()->loadUserPresetAsync(parent);

}

int UserPresetHelpers::addMissingControlsToUserPreset(ModulatorSynthChain* chain, const File& fileToUpdate)
{
	static const Identifier type("type");
	static const Identifier id("id");
	static const Identifier value("value");

	ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToUpdate);

	int numAddedControls = 0;

	if (xml != nullptr)
	{
		ValueTree thisPreset = ValueTree::fromXml(*xml);
		ValueTree contentData = thisPreset.getChild(0);
		jassert(contentData.hasType("Content"));
		const String interfaceId = contentData.getProperty("Processor").toString();
		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(ProcessorHelpers::getFirstProcessorWithName(chain, interfaceId));

		if (pwsc == nullptr)
		{
			PresetHandler::showMessageWindow("Invalid User Preset", "The user preset " + fileToUpdate.getFileNameWithoutExtension() + " is not associated with the current instrument", PresetHandler::IconType::Error);
			return -1;
		}

		auto content = pwsc->getScriptingContent();

		for (int i = 0; i < content->getNumComponents(); i++)
		{
			auto control = content->getComponent(i);
			bool isPresetControl = (int)control->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset) > 0;

			if (!isPresetControl)
				continue;

			ValueTree controlData = control->exportAsValueTree();
			ValueTree existingControlData = contentData.getChildWithProperty(id, controlData.getProperty(id));

			if (existingControlData.isValid())
			{
				continue;
			}

			numAddedControls++;

			var defaultValue = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(control) ?
				control->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::Properties::defaultValue) :
				var(0.0f);

			controlData.setProperty(value, defaultValue, nullptr);
			contentData.addChild(controlData, -1, nullptr);
		}

		if (numAddedControls != 0)
		{
			xml = thisPreset.createXml();

			fileToUpdate.replaceWithText(xml->createDocument(""));
		}

		return numAddedControls;
	}

	return -1;
}

bool UserPresetHelpers::updateVersionNumber(ModulatorSynthChain* chain, const File& fileToUpdate)
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToUpdate);

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
	return SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(chain));
#else
	ignoreUnused(chain);
	return ProjectHandler::Frontend::getVersionString();
#endif
}

File UserPresetHelpers::getUserPresetFile(ModulatorSynthChain *chain, const String &fileNameWithoutExtension)
{
#if USE_BACKEND
    return GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileNameWithoutExtension + ".preset");
#else

	ignoreUnused(chain);

    
    File userPresetDir;
    
    try
    {
        userPresetDir = ProjectHandler::Frontend::getUserPresetDirectory();
    }
    catch(String& s)
    {
        chain->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
        return File();
    }
    
    
	return userPresetDir.getChildFile(fileNameWithoutExtension + ".preset");
#endif
}

ValueTree UserPresetHelpers::collectAllUserPresets(ModulatorSynthChain* chain)
{
	ValueTree v("UserPresets");

	auto presetRoot = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
	
	Array<File> banks;

	presetRoot.findChildFiles(banks, File::findDirectories, false);

	for (auto bank : banks)
	{
		ValueTree b("Bank");
		b.setProperty("FileName", bank.getFileNameWithoutExtension(), nullptr);

		Array<File> categories;

		bank.findChildFiles(categories, File::findDirectories, false);

		for (auto cat : categories)
		{
			ValueTree c("Category");
			c.setProperty("FileName", cat.getFileNameWithoutExtension(), nullptr);

			Array<File> presets;

			cat.findChildFiles(presets, File::findFiles, false, "*.preset");

			for (auto preset : presets)
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(preset);

				if (xml != nullptr)
				{
					ValueTree p = ValueTree("PresetFile");
					p.setProperty("FileName", preset.getFileNameWithoutExtension(), nullptr);
					ValueTree pContent = ValueTree::fromXml(*xml);
					p.setProperty("isDirectory", false, nullptr);
					p.addChild(pContent, -1, nullptr);

					c.addChild(p, -1, nullptr);
				}
				
			}

			c.setProperty("isDirectory", true, nullptr);
			b.addChild(c, -1, nullptr);
		}

		b.setProperty("isDirectory", true, nullptr);
		v.addChild(b, -1, nullptr);
	}

	return v;
}

void UserPresetHelpers::extractUserPresets(const char* userPresetData, size_t size)
{
#if USE_FRONTEND
	auto userPresetDirectory = ProjectHandler::Frontend::getUserPresetDirectory();

	if (userPresetDirectory.isDirectory())
		return;

	userPresetDirectory.createDirectory();

	ValueTree presetTree = PresetHandler::loadValueTreeFromData(userPresetData, size, true);

	for (auto bank : presetTree)
	{
		jassert(bank.getProperty("isDirectory"));

		auto bankName = bank.getProperty("FileName").toString();
		auto bankFile = userPresetDirectory.getChildFile(bankName);
		bankFile.createDirectory();

		for (auto category : bank)
		{
			jassert(category.getProperty("isDirectory"));

			auto catName = category.getProperty("FileName").toString();
			auto catFile = bankFile.getChildFile(catName);
			catFile.createDirectory();

			for (auto preset : category)
			{
				auto presetName = preset.getProperty("FileName").toString();
				auto presetFile = catFile.getChildFile(presetName + ".preset");
				auto presetContent = preset.getChild(0);
				
				presetFile.replaceWithText(presetContent.toXmlString());
			}
		}
	}

#else
	ignoreUnused(userPresetData, size);
#endif
}

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

		outputFile.deleteFile();

		FileOutputStream fos(outputFile);

		v.writeToStream(fos);
	}
}

String PresetHandler::getProcessorNameFromClipboard(const FactoryType *t)
{
	if(SystemClipboard::getTextFromClipboard() == String()) return String();

	String x = SystemClipboard::getTextFromClipboard();
	ScopedPointer<XmlElement> xml = XmlDocument::parse(x);

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
	ScopedPointer<XmlElement> xml = p->exportAsValueTree().createXml();
	String x = xml->createDocument(String());
	SystemClipboard::copyTextToClipboard(x);

	debugToConsole(p, p->getId() + " was copied to clipboard.");
}

class MessageWithIcon : public Component
{
public:
	
	

	MessageWithIcon(PresetHandler::IconType type, const String &message)
	{
		switch (type)
		{
		case PresetHandler::IconType::Info: image = ImageCache::getFromMemory(BinaryData::infoInfo_png, BinaryData::infoInfo_pngSize);
			break;
		case PresetHandler::IconType::Warning: image = ImageCache::getFromMemory(BinaryData::infoWarning_png, BinaryData::infoWarning_pngSize);
			break;
		case PresetHandler::IconType::Question: image = ImageCache::getFromMemory(BinaryData::infoQuestion_png, BinaryData::infoQuestion_pngSize);
			break;
		case PresetHandler::IconType::Error: image = ImageCache::getFromMemory(BinaryData::infoError_png, BinaryData::infoError_pngSize);
			break;
		case PresetHandler::IconType::numIconTypes: image = Image(); jassertfalse;
			break;
		default:
			break;
		}

		Font font = GLOBAL_BOLD_FONT();

		StringArray lines = StringArray::fromLines(message);

		bestWidth = 0;

		AttributedString s;
		
		s.setJustification(Justification::topLeft);
		
		for (int i = 0; i < lines.size(); i++)
		{
			bestWidth = jmax<int>(bestWidth, font.getStringWidth(lines[i]));
			s.append(lines[i] + "\n", i == 0 ? GLOBAL_BOLD_FONT() : GLOBAL_FONT(), Colour(0xFF888888));
		}

		layout.createLayoutWithBalancedLineLengths(s, (float)bestWidth);
		setSize(bestWidth + image.getWidth(), jmax<int>(image.getHeight(), (int)(layout.getHeight() + font.getHeight())));
	}

	void paint(Graphics &g) override
	{

		g.drawImageAt(image, 0, 0);
		g.setColour(Colour(0xFF999999));
		layout.draw(g, Rectangle<float>((float)image.getWidth(), 0.0f, (float)bestWidth, (float)getHeight()));
	}

private:
	
	

private:

	int bestWidth;
	
	TextLayout layout;

	Image image;
};

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

	AlertWindowLookAndFeel laf;

	ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(PresetHandler::IconType::Question, message);

    ScopedPointer<AlertWindow> nameWindow = new AlertWindow(useCustomMessage ? ("Enter " + typeName) : ("Enter name for " + typeName), "", AlertWindow::AlertIconType::NoIcon);

	nameWindow->setLookAndFeel(&laf);

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

	if(nameWindow->runModalLoop()) return nameWindow->getTextEditorContents("Name");
	else return String();
    
};

bool PresetHandler::showYesNoWindow(const String &title, const String &message, PresetHandler::IconType type)
{
#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
	{
		return true; // surpress Popups in commandline mode
	}
#endif


#if HISE_IOS
    return NativeMessageBox::showOkCancelBox(AlertWindow::AlertIconType::NoIcon, title, message);
    
#else
	AlertWindowLookAndFeel laf;

	ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(type, message);
	ScopedPointer<AlertWindow> nameWindow = new AlertWindow(title, "", AlertWindow::AlertIconType::NoIcon);

	nameWindow->setLookAndFeel(&laf);
	nameWindow->addCustomComponent(comp);
	
	nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	return (nameWindow->runModalLoop() == 1);
    
#endif
};

void PresetHandler::showMessageWindow(const String &title, const String &message, PresetHandler::IconType type)
{
#if HISE_IOS
    
    NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::NoIcon, title, message);
    
#else
    
    AlertWindowLookAndFeel laf;
    
    ScopedPointer<MessageWithIcon> comp = new MessageWithIcon(type, message);
    
    ScopedPointer<AlertWindow> nameWindow = new AlertWindow(title, "", AlertWindow::AlertIconType::NoIcon);
    
    nameWindow->setLookAndFeel(&laf);
    nameWindow->addCustomComponent(comp);
    nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
    
    nameWindow->runModalLoop();
#endif

	return ;
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


void ProjectHandler::createNewProject(File &workingDirectory, Component* mainEditor)
{
	if (workingDirectory.exists() && workingDirectory.isDirectory())
	{
		while (workingDirectory.getNumberOfChildFiles(File::findFilesAndDirectories) != 0)
		{
			PresetHandler::showMessageWindow("Directory already exists", "The directory is not empty. Try another one...", PresetHandler::IconType::Warning);
            
            FileChooser fc("Create new project directory");
            
            if (fc.browseForDirectory())
            {
                workingDirectory = fc.getResult();
            }
            else
            {
                return;
            }
		}
	}

	for (int i = 0; i < (int)SubDirectories::numSubDirectories; i++)
	{
		File subDirectory = workingDirectory.getChildFile(getIdentifier((SubDirectories)i));

		subDirectory.createDirectory();

		

	}

	setWorkingProject(workingDirectory, mainEditor);
}

void ProjectHandler::setWorkingProject(const File &workingDirectory, Component* mainEditor)
{
	if (workingDirectory == currentWorkDirectory) return;

	if (!isValidProjectFolder(workingDirectory))
	{
		jassertfalse;
		return;
	}

	currentWorkDirectory = workingDirectory;

	if (!workingDirectory.exists()) return;

	checkSettingsFile(mainEditor);
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

	File(PresetHandler::getDataFolder()).getChildFile("projects.xml").replaceWithText(xml->createDocument(""));

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
			listeners[i]->projectChanged(currentWorkDirectory);
		else
			listeners.remove(i--);
	}
}

void ProjectHandler::restoreWorkingProjects()
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(File(PresetHandler::getDataFolder()).getChildFile("projects.xml"));

	if (xml != nullptr)
	{
		jassert(xml->getTagName() == "Projects");

		File current = File(xml->getStringAttribute("current"));

		recentWorkDirectories.clear();

		for (int i = 0; i < xml->getNumChildElements(); i++)
		{
			recentWorkDirectories.add(xml->getChildElement(i)->getStringAttribute("path"));
		}

		setWorkingProject(current, nullptr);

		jassert(currentWorkDirectory.exists() && currentWorkDirectory.isDirectory());

		
	}
}

bool ProjectHandler::isValidProjectFolder(const File &file) const
{
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

File ProjectHandler::getLinkFile(const File &subDirectory)
{
#if JUCE_MAC
    return subDirectory.getChildFile("LinkOSX");
#elif JUCE_LINUX
    return subDirectory.getChildFile("LinkLinux");
#else
    return subDirectory.getChildFile("LinkWindows");
#endif
}


void ProjectHandler::checkSubDirectories()
{
	subDirectories.clear();

	for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
	{
		SubDirectories dir = (SubDirectories)i;

		File f = checkSubDirectory(dir);
		jassert(f.exists() && f.isDirectory());

		File subDirectory = currentWorkDirectory.getChildFile(getIdentifier(dir));
		File linkFile = getLinkFile(subDirectory);
		const bool redirected = linkFile.existsAsFile();

		subDirectories.add(FolderReference(dir, redirected, f));
	}
}

File ProjectHandler::checkSubDirectory(SubDirectories dir)
{
	File subDirectory = currentWorkDirectory.getChildFile(getIdentifier(dir));

	jassert(subDirectory.exists());

	File childFile = getLinkFile(subDirectory);

	if (childFile.existsAsFile())
	{
		String absolutePath = childFile.loadFileAsString();

		if (ProjectHandler::isAbsolutePathCrossPlatform(absolutePath))
		{
			if(!File(absolutePath).exists())
            {
                if(PresetHandler::showYesNoWindow("Missing Sample Folder", "The sample relocation folder does not exist. Press OK to choose a new location or Cancel to ignore this.", PresetHandler::IconType::Warning))
                {
                    FileChooser fc("Redirect sample folder to the following location");
                    
                    if (fc.browseForDirectory())
                    {
                        File f = fc.getResult();
                        
                        createLinkFile(ProjectHandler::SubDirectories::Samples, f);
                        
                        return f;
                    }
                }
            }
            
            
            
			return File(absolutePath);
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

void ProjectHandler::checkSettingsFile(Component* mainEditor/*=nullptr*/)
{
	if (!getWorkDirectory().getChildFile("project_info.xml").existsAsFile())
	{
		setProjectSettings(mainEditor);
	}

}

File ProjectHandler::getSubDirectory(SubDirectories dir) const
{
    jassert(isActive());
    
	return subDirectories[(int)dir].file;
}

File ProjectHandler::getWorkDirectory() const
{
	if (!isActive())
	{
		return File();
	}

	else return currentWorkDirectory;
}

String ProjectHandler::getIdentifier(SubDirectories dir)
{
	switch (dir)
	{
	case ProjectHandler::SubDirectories::Scripts:			return "Scripts/";
	case ProjectHandler::SubDirectories::AdditionalSourceCode:	return "AdditionalSourceCode/";
	case ProjectHandler::SubDirectories::Binaries:			return "Binaries/";
	case ProjectHandler::SubDirectories::Presets:			return "Presets/";
	case ProjectHandler::SubDirectories::XMLPresetBackups:	return "XmlPresetBackups/";
	case ProjectHandler::SubDirectories::Samples:			return "Samples/";
	case ProjectHandler::SubDirectories::Images:			return "Images/";
	case ProjectHandler::SubDirectories::AudioFiles:		return "AudioFiles/";
	case ProjectHandler::SubDirectories::UserPresets:		return "UserPresets/";
	case ProjectHandler::SubDirectories::SampleMaps:		return "SampleMaps/";
	case ProjectHandler::SubDirectories::numSubDirectories: 
	default:												jassertfalse; return String();
	}
}

ProjectHandler::SubDirectories ProjectHandler::getSubDirectoryForIdentifier(Identifier id)
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


void ProjectHandler::getFileList(Array<File> &filesInDirectory, SubDirectories dir, const String &wildcard, bool sortByTime /*= false*/, bool searchInSubfolders/*=false*/)
{
    if(!isActive()) return;
    
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
	

}

void ProjectHandler::createRSAKey() const
{
	RSAKey publicKey;
	RSAKey privateKey;

	Random r;

	const int seeds[] = { r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt(), r.nextInt() };

	RSAKey::createKeyPair(publicKey, privateKey, 512, seeds, 6);

	AlertWindowLookAndFeel wlaf;

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

		File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

		rsaFile.replaceWithText(xml->createDocument(""));

		PresetHandler::showMessageWindow("RSA keys exported to file", "The RSA Keys are written to the file " + rsaFile.getFullPathName(), PresetHandler::IconType::Info);
	}
}

String ProjectHandler::getPublicKey() const
{
	File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

	ScopedPointer<XmlElement> xml = XmlDocument::parse(rsaFile);

    if(xml == nullptr) return "";
    
	return xml->getChildByName("PublicKey")->getStringAttribute("value", "");
}

String ProjectHandler::getPrivateKey() const
{
	File rsaFile = getWorkDirectory().getChildFile("RSA.xml");

	ScopedPointer<XmlElement> xml = XmlDocument::parse(rsaFile);

    if(xml == nullptr) return "";
    
	return xml->getChildByName("PrivateKey")->getStringAttribute("value", "");
}

void ProjectHandler::checkActiveProject()
{
	throw std::logic_error("The method or operation is not implemented.");
}




void ProjectHandler::checkAllSampleMaps()
{
	Array<File> sampleMaps;
	Array<File> samples;

	getSubDirectory(SubDirectories::Samples).findChildFiles(samples, File::findFiles, true);
	getSubDirectory(SubDirectories::SampleMaps).findChildFiles(sampleMaps, File::findFiles, true, "*.xml;*.XML");

	String falseName;

	for (int i = 0; i < sampleMaps.size(); i++)
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(sampleMaps[i]);

		if (xml != nullptr)
		{
			ValueTree v = ValueTree::fromXml(*xml);

			const String id = v.getProperty("ID").toString();

			if (id != sampleMaps[i].getFileNameWithoutExtension())
			{
				PresetHandler::showMessageWindow("Mismatching SampleMap ID", "The SampleMap " + sampleMaps[i].getFileName() + " does not have the correct ID", PresetHandler::IconType::Error);
				return;
			}

			falseName = SampleMap::checkReferences(v, getSubDirectory(SubDirectories::Samples), samples);

			if (falseName.isNotEmpty())
			{
				break;
			}
		}
	}

	if (falseName.isEmpty())
	{
		PresetHandler::showMessageWindow("All sample references are valid", "All sample maps have been scanned", PresetHandler::IconType::Info);
	}
	else
	{
		PresetHandler::showMessageWindow("Missing samples found", "The sample " + falseName + " wasn't found.", PresetHandler::IconType::Error);
	}
}

String ProjectHandler::Frontend::checkSampleReferences(const ValueTree &sampleMaps, bool returnTrueIfOneSampleFound)
{
	Array<File> sampleList;

    StringArray existingFiles;
    StringArray missingFiles;
    
	const File sampleLocation = getSampleLocationForCompiledPlugin();

	sampleLocation.findChildFiles(sampleList, File::findFiles, true);

	String falseName;

    int numCorrectSampleMaps = 0;
    
	for (int i = 0; i < sampleMaps.getNumChildren(); i++)
	{
        
        
		ValueTree child = sampleMaps.getChild(i);
		
        const String thisFalseName = SampleMap::checkReferences(child, sampleLocation, sampleList);

        if(thisFalseName.isNotEmpty())
        {
            falseName = thisFalseName;
        }
        else
        {
            numCorrectSampleMaps++;
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


File ProjectHandler::Frontend::getResourcesFolder()
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

bool ProjectHandler::isRedirected(ProjectHandler::SubDirectories dir) const
{
	return subDirectories[(int)dir].isReference;
}

void ProjectHandler::createLinkFile(ProjectHandler::SubDirectories dir, const File &relocation)
{
    File subDirectory = currentWorkDirectory.getChildFile(getIdentifier(dir));
    
	createLinkFileInFolder(subDirectory, relocation);
}


void ProjectHandler::createLinkFileInFolder(const File& source, const File& target)
{
	File linkFile = getLinkFile(source);

	if (linkFile.existsAsFile())
	{
		if (!PresetHandler::showYesNoWindow("Already there", "Link redirect file exists. Do you want to replace it?"))
		{
			return;
		}
	}

	linkFile.create();

	linkFile.replaceWithText(target.getFullPathName());
}

void ProjectHandler::setProjectSettings(Component *mainEditor)
{
	SettingWindows::ProjectSettingWindow *w = new SettingWindows::ProjectSettingWindow(this);

	window = w;

	if (mainEditor == nullptr)
	{
		w->showOnDesktop();
	}
	else
	{
		w->setModalBaseWindowComponent(mainEditor);
	}
}


String ProjectHandler::getFilePath(const String &pathToFile, SubDirectories subDir) const
{
	if (ProjectHandler::isAbsolutePathCrossPlatform(pathToFile)) return pathToFile;

    static String id = "{PROJECT_FOLDER}";


 
#if USE_FRONTEND

	// Everything else must be embedded into the binary...
	jassert(subDir == ProjectHandler::SubDirectories::Samples);
	ignoreUnused(subDir);

	File sampleFolder = dynamic_cast<FrontendDataHolder*>(mc)->getSampleLocation();

	if (sampleFolder.isDirectory())
		return sampleFolder.getChildFile(pathToFile.replace(id, "")).getFullPathName();

	else
		return String();

#else

    static int idLength = id.length();

#if JUCE_MAC
    String pathToUse = pathToFile.replace("\\", "/");
    
	if (pathToUse.startsWith(id))
	{
		return getSubDirectory(subDir).getChildFile(pathToUse.substring(idLength)).getFullPathName(); \
	}

	return pathToFile;

#else
    
	if (pathToFile.startsWith(id))
	{
		return getSubDirectory(subDir).getChildFile(pathToFile.substring(idLength)).getFullPathName(); \
	}
		
	return pathToFile;

#endif

#endif

}

const String ProjectHandler::getFileReference(const String &absoluteFileName, SubDirectories dir) const
{
	static String id = "{PROJECT_FOLDER}";

	if (absoluteFileName.contains(id)) return absoluteFileName;

	File subDir = getSubDirectory(dir);

	if (absoluteFileName.contains(subDir.getFullPathName()))
	{
#if JUCE_WINDOWS
		String fileName = File(absoluteFileName).getRelativePathFrom(subDir).replace("\\", "/");
#else
		String fileName = File(absoluteFileName).getRelativePathFrom(subDir);
#endif

        if(ProjectHandler::isAbsolutePathCrossPlatform(fileName))
        {
            return absoluteFileName;
        }
        else
        {
            return id + fileName;
        }
	}
	else return absoluteFileName;
}

File ProjectHandler::Frontend::getSampleLocationForCompiledPlugin()
{
#if USE_FRONTEND
    
#if HISE_IOS
    
    File f = getResourcesFolder().getChildFile("Samples/");
    
    
    return f;
    
#endif
    
    
    
	File appDataDir = getAppDataDirectory();

	// The installer should take care of creating the app data directory...
	jassert(appDataDir.isDirectory());
	
#if JUCE_MAC && ENABLE_APPLE_SANDBOX
    File childFile = ProjectHandler::getLinkFile(appDataDir.getChildFile("Resources/"));
#else
    File childFile = ProjectHandler::getLinkFile(appDataDir);
#endif

	if (childFile.exists())
	{
		return File(childFile.loadFileAsString());
	}

	return File();

#else
	return File();
#endif
}

File ProjectHandler::Frontend::getAppDataDirectory(ProjectHandler *handler/*=nullptr*/)
{

    const File::SpecialLocationType appDataDirectoryToUse = File::userApplicationDataDirectory;


#if USE_FRONTEND
    
	ignoreUnused(handler);

#if JUCE_IOS
    
    File f = File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support/" + getCompanyName() + "/" + getProjectName());
    
    if(!f.isDirectory())
    {
        f.createDirectory();
    }
    
    return f;
    
#elif JUCE_MAC

    
#if ENABLE_APPLE_SANDBOX
    return File::getSpecialLocation(File::userMusicDirectory).getChildFile(getCompanyName() + "/" + getProjectName());
#else
    return File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support/" + getCompanyName() + "/" + getProjectName());
#endif
    
#else // WINDOWS

	File f = File::getSpecialLocation(appDataDirectoryToUse).getChildFile(getCompanyName() + "/" + getProjectName());

	if (!f.isDirectory())
	{
		f.createDirectory();
	}

	return f;

#endif
#else // BACKEND

	jassert(handler != nullptr);

	const String company = SettingWindows::getSettingValue((int)SettingWindows::UserSettingWindow::Attributes::Company, handler);
	const String product = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, handler);

#if JUCE_MAC
	return File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support/" + company + "/" + product);
#else
	return File::getSpecialLocation(appDataDirectoryToUse).getChildFile(company + "/" + String(product));
#endif

#endif
}

File ProjectHandler::Frontend::getLicenceKey()
{
#if USE_FRONTEND

	return getAppDataDirectory().getChildFile(getProjectName() + getLicenceKeyExtension());

#else

	return File();

#endif
}

String ProjectHandler::Frontend::getLicenceKeyExtension()
{

#if JUCE_WINDOWS

#if JUCE_64BIT
	return ".licence_x64";
#else
	return ".licence_x86";
#endif

#else
	return ".licence";
#endif
}

String ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(const String &absoluteFileName)
{
	static String id = "{PROJECT_FOLDER}";

	return absoluteFileName.substring(16);
}

void ProjectHandler::Frontend::setSampleLocation(const File &newLocation)
{
#if USE_FRONTEND
	File appDataDir = getAppDataDirectory();

	// The installer should take care of creating the app data directory...
	jassert(appDataDir.isDirectory());

#if JUCE_MAC && ENABLE_APPLE_SANDBOX
    File childFile = ProjectHandler::getLinkFile(File(appDataDir.getChildFile("Resources/"));
#else
    File childFile = ProjectHandler::getLinkFile(appDataDir);
#endif

	childFile.replaceWithText(newLocation.getFullPathName());

#else

	ignoreUnused(newLocation);

#endif
}



File ProjectHandler::Frontend::getUserPresetDirectory()
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
	

	return presetDir;
    
#endif
}





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
	if (GET_PROJECT_HANDLER(p).isActive())
	{
		return GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Presets);
	}
	else
	{
		return File();
	}
}


PopupMenu PresetHandler::getAllSavedPresets(int minIndex, Processor *p)
{
    PopupMenu m;
	
#if HISE_IOS
    
#else
	
	File directoryToScan = PresetHandler::getDirectory(p);
	DirectoryIterator directoryIterator(directoryToScan, false, "*", File::TypesOfFileToFind::findFilesAndDirectories);

	while(directoryIterator.next())
	{
		File directory = directoryIterator.getFile();

		if (directory.isDirectory())
		{
			PopupMenu sub;
			DirectoryIterator presetIterator(directory, false, "*.hip", File::TypesOfFileToFind::findFiles);

			while(presetIterator.next())
			{
				File preset = presetIterator.getFile();

				sub.addItem(minIndex++, preset.getFileNameWithoutExtension());
			}

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

File PresetHandler::getPresetFileFromMenu(int menuIndexDelta, Processor *parent)
{
	File directory = getDirectory(parent);
	DirectoryIterator it(directory, true, "*", File::findFilesAndDirectories);
	DirectoryIterator directoryIterator(directory, false, "*", File::TypesOfFileToFind::findFilesAndDirectories);

	int i = 0;

	while (directoryIterator.next())
	{
		File fileToCheck = directoryIterator.getFile();

		if (fileToCheck.isDirectory())
		{
			DirectoryIterator presetIterator(fileToCheck, false, "*.hip", File::TypesOfFileToFind::findFiles);

			while (presetIterator.next())
			{
				if (i == menuIndexDelta) return presetIterator.getFile();
				i++;
			}
		}

		else if (fileToCheck.hasFileExtension(".hip"))
		{
			if (i == menuIndexDelta) return directoryIterator.getFile();

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
		ScopedPointer<XmlElement> parsedXml = XmlDocument::parse(x);
		ValueTree v = ValueTree::fromXml(*parsedXml);

		if(parsedXml->getStringAttribute("ID") != v.getProperty("ID", String()).toString() )
		{
			jassertfalse;
			debugToConsole(parent, "Clipboard could not be loaded");
			return nullptr;
		};

		String name = v.getProperty("ID", "Unnamed");
		
#if USE_OLD_FILE_FORMAT
		Identifier type = v.getType();
#else
		Identifier type = v.getProperty("Type", String()).toString();
#endif

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

		p->setId(FactoryType::getUniqueName(p));

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
			Chain *childChain = dynamic_cast<Chain*>(p->getChildProcessor(i));

			for (int j = 0; j < childChain->getHandler()->getNumProcessors(); j++)
			{
				Processor *cp = childChain->getHandler()->getProcessor(j);

				const String newUniqueId = FactoryType::getUniqueName(cp);

				cp->setId(newUniqueId);
			}
		}
	}
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



void PresetHandler::buildProcessorDataBase(Processor *root)
{
	File f(getDataFolder() + "/moduleEnums.xml");

	if (f.existsAsFile()) return;

	ScopedPointer<XmlElement> xml = new XmlElement("Parameters");
	

	ScopedPointer<FactoryType> t = new ModulatorSynthChainFactoryType(NUM_POLYPHONIC_VOICES, root);

	xml->addChildElement(buildFactory(t, "ModulatorSynths"));

	t = new MidiProcessorFactoryType(root);
	xml->addChildElement(buildFactory(t, "MidiProcessors"));


	t = new VoiceStartModulatorFactoryType(NUM_POLYPHONIC_VOICES, Modulation::GainMode, root);
	xml->addChildElement(buildFactory(t, "VoiceStartModulators"));

	t = new TimeVariantModulatorFactoryType(Modulation::GainMode, root);

	xml->addChildElement(buildFactory(t, "TimeVariantModulators"));

	t = new EnvelopeModulatorFactoryType(NUM_POLYPHONIC_VOICES, Modulation::GainMode, root);

	xml->addChildElement(buildFactory(t, "EnvelopeModulators"));

	t = new EffectProcessorChainFactoryType(NUM_POLYPHONIC_VOICES, root);

	xml->addChildElement(buildFactory(t, "Effects"));


	xml->writeToFile(f, "");
}

XmlElement * PresetHandler::buildFactory(FactoryType *t, const String &factoryName)
{
	
	XmlElement *xml = new XmlElement(factoryName);

	for (int j = 0; j < t->getNumProcessors(); j++)
	{

		ScopedPointer<Processor> p = t->createProcessor(j, "X");

		if (p == nullptr) continue;

		XmlElement *child = new XmlElement(p->getType());

		for (int i = 0; i < p->getNumParameters(); i++)
		{
			Identifier id = p->getIdentifierForParameterIndex(i);


			child->setAttribute(Identifier("id" + String(i)), id.toString());
		}
        
        

		xml->addChildElement(child);
	}

	return xml;
}

File PresetHandler::getGlobalScriptFolder()
{
	File globalScriptFolder = File(getDataFolder()).getChildFile("scripts");

    const File link = ProjectHandler::getLinkFile(globalScriptFolder);
    
	if (link.existsAsFile())
	{
		File linkTarget = File(link.loadFileAsString());

		if (linkTarget.isDirectory())
			globalScriptFolder = linkTarget;
	}

	return globalScriptFolder;
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

	return afm.createReaderFor(stream);
}

String PresetHandler::getGlobalSampleFolder()
{
	return String();
}

String PresetHandler::getDataFolder()
{
#if USE_COMMON_APP_DATA_FOLDER
	const File::SpecialLocationType appDataDirectoryToUse = File::commonApplicationDataDirectory;
#else
	const File::SpecialLocationType appDataDirectoryToUse = File::userApplicationDataDirectory;
#endif


#if JUCE_WINDOWS
    // Windows
    File f = File::getSpecialLocation(appDataDirectoryToUse).getChildFile("HISE");
#elif JUCE_MAC
    
#if HISE_IOS
    // iOS
    File f = File::getSpecialLocation(appDataDirectoryToUse);
#else
    // OS X
    File f = File::getSpecialLocation(appDataDirectoryToUse).getChildFile("Application Support/HISE");
#endif
    
#else
    // Linux
    File f = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory).getChildFile(".hise/");
#endif
    
    if(!f.isDirectory()) f.createDirectory();
    
    return f.getFullPathName();
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
	infoData.append(ProjectInfo::versionString, normal, bright);
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
    infoData.append(String(BUILD_SUB_VERSION), normal, bright);
	infoData.append("\nBuild date: ", bold, bright);
	infoData.append(Time::getCompilationDate().toString(true, false, false, true), normal, bright);
	infoData.append("\nCreated by: ", bold, bright);
	infoData.append(JucePlugin_Manufacturer, normal, bright);
#endif

#if USE_COPY_PROTECTION

	Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

	infoData.append("\n\nRegistered to: ", bold, bright);
	infoData.append(ul->getUserEmail(), normal, bright);

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

String PresetPlayerHandler::getSpecialFolder(FolderType type, const String &packageName /*= String()*/, bool ignoreMissingDirectory)
{
	IGNORE_UNUSED_IN_RELEASE(ignoreMissingDirectory);

	File globalSampleFolder = PresetHandler::getGlobalSampleFolder();

	String packageToUse = packageName;

	
	String returnFile;

	switch (type)
	{
	case FolderType::GlobalSampleDirectory: returnFile = globalSampleFolder.getFullPathName(); break;
	case FolderType::PackageDirectory:		returnFile = globalSampleFolder.getFullPathName() + "/" + packageToUse; break;
	case FolderType::StreamedSampleFolder:	returnFile = globalSampleFolder.getFullPathName() + "/" + packageToUse + "/streamed_samples"; break;
	case FolderType::AudioFiles:			returnFile = globalSampleFolder.getFullPathName() + "/" + packageToUse + "/audio_files"; break;
	case FolderType::ImageResources:		returnFile = globalSampleFolder.getFullPathName() + "/" + packageToUse + "/image_resources"; break;
	default: jassertfalse;
	}

	// You must create the package before using it!
	jassert(ignoreMissingDirectory || File(returnFile).isDirectory());

	return returnFile;
}



void PresetPlayerHandler::checkAndCreatePackage(const String &packageName)
{
	
	File packageFolder = File(getSpecialFolder(PackageDirectory, packageName, true));

	if (packageFolder.isDirectory())
	{

	}
	else
	{
		packageFolder.createDirectory();

		File audioFileFolder = getSpecialFolder(AudioFiles, packageName, true);
		File imageFolder = getSpecialFolder(ImageResources, packageName, true);
		File sampleFolder = getSpecialFolder(StreamedSampleFolder, packageName, true);

		audioFileFolder.createDirectory();
		imageFolder.createDirectory();
		sampleFolder.createDirectory();

		String niceName = PresetHandler::getCustomName("Nice Package Name");
		String version = PresetHandler::getCustomName("Version");
		String author = PresetHandler::getCustomName("Author");

		ScopedPointer<XmlElement> packageXml = new XmlElement("package");
		packageXml->setAttribute("name", niceName);
		packageXml->setAttribute("version", version);
		packageXml->setAttribute("author", author);

		File xmlFile(packageFolder.getFullPathName() + "/package.xml");

		packageXml->writeToFile(xmlFile, "");
	}
}

void PresetPlayerHandler::addInstrumentToPackageXml(const String &instrumentFileName, const String &packageName)
{
	checkAndCreatePackage(packageName);

	File xmlFile(getSpecialFolder(PackageDirectory, packageName) + "/package.xml");

	ScopedPointer<XmlElement> xml = XmlDocument::parse(xmlFile);

	jassert(xml != nullptr);

	if (xml != nullptr)
	{
		XmlElement *child = new XmlElement("instrument");
		child->setAttribute("name", instrumentFileName);
		child->setAttribute("file", instrumentFileName + ".hip");

		xml->addChildElement(child);

		xml->writeToFile(xmlFile, "");
	}


}

void FrontendSampleManager::loadSamplesAfterSetup()
{
	if (shouldLoadSamplesAfterSetup())
	{
		dynamic_cast<AudioProcessor*>(this)->suspendProcessing(false);
		dynamic_cast<MainController*>(this)->getSampleManager().setShouldSkipPreloading(false);
		dynamic_cast<MainController*>(this)->getSampleManager().preloadEverything();
	}
	else
	{
		dynamic_cast<AudioProcessor*>(this)->suspendProcessing(true);
	}
}




void FrontendSampleManager::setAllSampleReferencesCorrect()
{
	samplesCorrectlyLoaded = true;
}

void FrontendSampleManager::checkAllSampleReferences()
{
#if HISE_IOS
    
    samplesCorrectlyLoaded = true;
    
#else
	ValueTree sampleMapTree = dynamic_cast<FrontendDataHolder*>(this)->getValueTree(ProjectHandler::SubDirectories::SampleMaps);

	const String missingSampleName = ProjectHandler::Frontend::checkSampleReferences(sampleMapTree, true);

	samplesCorrectlyLoaded = missingSampleName.isEmpty();

	if (missingSampleName.isNotEmpty())
	{
		dynamic_cast<MainController*>(this)->sendOverlayMessage(DeactiveOverlay::State::SamplesNotFound, "The sample " + missingSampleName + " was not found.");
	}
#endif
}

bool FrontendSampleManager::areSampleReferencesCorrect() const
{
	return samplesCorrectlyLoaded;
}
