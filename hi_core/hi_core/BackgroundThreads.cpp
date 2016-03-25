
ThreadWithAsyncProgressWindow::ThreadWithAsyncProgressWindow(const String &title, bool synchronous_) :
AlertWindow(title, String::empty, AlertWindow::AlertIconType::NoIcon),
progress(0.0),
isQuasiModal(false),
synchronous(synchronous_)
{
	setLookAndFeel(&laf);

	setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	setColour(AlertWindow::textColourId, Colours::white);

}

ThreadWithAsyncProgressWindow::~ThreadWithAsyncProgressWindow()
{
	if (thread != nullptr)
	{
		thread->stopThread(6000);
	}
}

void ThreadWithAsyncProgressWindow::addBasicComponents(bool addOKButton)
{
	addTextEditor("state", "", "Status", false);

	getTextEditor("state")->setReadOnly(true);

	addProgressBarComponent(progress);
	
	if (addOKButton)
	{
		addButton("OK", 1, KeyPress(KeyPress::returnKey));
	}
	
	addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
}

bool ThreadWithAsyncProgressWindow::threadShouldExit() const
{
	if (thread != nullptr)
	{
		return thread->threadShouldExit();
	};

	return false;
}

void ThreadWithAsyncProgressWindow::handleAsyncUpdate()
{
	threadFinished();
	destroy();
}

void ThreadWithAsyncProgressWindow::buttonClicked(Button* b)
{
	if (b->getName() == "OK")
	{
		if (synchronous)
		{
			runSynchronous();
		}
		else if (thread == nullptr)
		{
			thread = new LoadingThread(this);
			thread->startThread();
		}
		
	}
	else if (b->getName() == "Cancel")
	{
		if (thread != nullptr)
		{
			thread->signalThreadShouldExit();
			thread->notify();
			destroy();
		}
		else
		{
			destroy();
		}
	}
	else
	{
		resultButtonClicked(b->getName());
	}
}

void ThreadWithAsyncProgressWindow::runSynchronous()
{
	// Obviously only available in the message loop!
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	run();
	threadFinished();
	destroy();
}

void ThreadWithAsyncProgressWindow::showStatusMessage(const String &message)
{
	MessageManagerLock lock(thread);

	if (lock.lockWasGained())
	{
		if (getTextEditor("state") != nullptr)
		{
			getTextEditor("state")->setText(message, dontSendNotification);
		}
	}
}

void ThreadWithAsyncProgressWindow::showOnDesktop()
{
	isQuasiModal = false;
	setVisible(true);
	addToDesktop();
	
}

void ThreadWithAsyncProgressWindow::setModalComponentOfMainEditor(Component * childComponentOfMainEditor)
{
	BackendProcessorEditor *editor = dynamic_cast<BackendProcessorEditor*>(childComponentOfMainEditor);
	
	if(editor == nullptr) editor = childComponentOfMainEditor->findParentComponentOfClass<BackendProcessorEditor>();

	jassert(editor != nullptr);

	if (editor != nullptr)
	{
		editor->setModalComponent(this);
		isQuasiModal = true;
	}
}

void ThreadWithAsyncProgressWindow::destroy()
{
	if (isQuasiModal)
	{
		findParentComponentOfClass<BackendProcessorEditor>()->clearModalComponent();
	}
	else
	{
		removeFromDesktop();
		delete this;
	}
}

void ThreadWithAsyncProgressWindow::runThread()
{
	thread = new LoadingThread(this);
	thread->startThread();
}



void SettingWindows::BaseSettingsWindow::run()
{
	for (int i = 0; i < settings->getNumChildElements(); i++)
	{
		const String widgetName = settings->getChildElement(i)->getTagName();

		if (TextEditor *ed = getTextEditor(widgetName))
		{
			settings->getChildElement(i)->setAttribute("value", ed->getText());
		}
		else if (ComboBox *cb = getComboBoxComponent(widgetName))
		{
			settings->getChildElement(i)->setAttribute("value", cb->getText());
		}
		else if (FilenameComponent *fc = getFileNameComponent(widgetName))
		{
			settings->getChildElement(i)->setAttribute("value", fc->getCurrentFile().getFullPathName());
		}
		else
		{
			jassertfalse;
		}

		
	}

	settingsFile.replaceWithText(settings->createDocument(""));
}

void SettingWindows::BaseSettingsWindow::resultButtonClicked(const String &name)
{
	if (name == "Reveal")
	{
		settingsFile.revealToUser();
	}
}

String SettingWindows::BaseSettingsWindow::getNameForValueType(ValueTypes t)
{
	switch (t)
	{
	case SettingWindows::BaseSettingsWindow::ValueTypes::Text: return "TEXT";
		break;
	case SettingWindows::BaseSettingsWindow::ValueTypes::List: return "LIST";
		break;
	case SettingWindows::BaseSettingsWindow::ValueTypes::File: return "FILE";
		break;
	case SettingWindows::BaseSettingsWindow::ValueTypes::numValueTypes:
		break;
	default:
		break;
	}

	return "INVALID";
}

SettingWindows::BaseSettingsWindow::ValueTypes SettingWindows::BaseSettingsWindow::getValueType(XmlElement *xml)
{
	const String attributeValue = xml->getStringAttribute("type");

	for (int i = 0; i < (int)ValueTypes::numValueTypes; i++)
	{
		if (getNameForValueType((ValueTypes)i) == attributeValue)
		{
			return (ValueTypes)i;
		}
	}
}

void SettingWindows::BaseSettingsWindow::addChildElement(XmlElement &element, int attributeIndex, const String &childValue, const String &description) const
{
	XmlElement *child = new XmlElement(getAttributeName(attributeIndex));
	child->setAttribute("value", childValue);
	child->setAttribute("type", getNameForValueType(ValueTypes::Text));
	child->setAttribute("description", description);

	element.addChildElement(child);
}

void SettingWindows::BaseSettingsWindow::addChildElementWithOptions(XmlElement &element, int attributeIndex, const String &childValue, const String &description, const String &optionsAsLines) const
{
	XmlElement *child = new XmlElement(getAttributeName(attributeIndex));
	child->setAttribute("value", childValue);
	child->setAttribute("type", getNameForValueType(ValueTypes::List));
	child->setAttribute("description", description);
	child->setAttribute("options", optionsAsLines);

	element.addChildElement(child);
}

void SettingWindows::BaseSettingsWindow::addFileAsChildElement(XmlElement &element, int attributeIndex, const String &childValue, const String &description) const
{
	XmlElement *child = new XmlElement(getAttributeName(attributeIndex));
	child->setAttribute("value", childValue);
	child->setAttribute("type", getNameForValueType(ValueTypes::File));
	child->setAttribute("description", description);

	element.addChildElement(child);
}

void SettingWindows::BaseSettingsWindow::setSettingsFile()
{
	settingsFile = getFile();

	if (settingsFile.existsAsFile())
	{
		settings = XmlDocument::parse(settingsFile);
	}
	else
	{
		settingsFile.create();
	}

	ScopedPointer<XmlElement> compare = createNewSettingsFile();

	if (settings == nullptr)
	{
		settings = createNewSettingsFile();
	}
	else if (compare->getNumChildElements() != settings->getNumChildElements())
	{
		// Somehow the loaded XML file has not the correct structure.
		jassertfalse;
	}

	for (int i = 0; i < settings->getNumChildElements(); i++)
	{
		ValueTypes type = getValueType(settings->getChildElement(i));

		const String tag = settings->getChildElement(i)->getTagName();
		const String value = settings->getChildElement(i)->getStringAttribute("value");
		const String description = settings->getChildElement(i)->getStringAttribute("description");

		if (type == ValueTypes::List)
		{
			const StringArray options = StringArray::fromLines(settings->getChildElement(i)->getStringAttribute("options"));
			const int index = options.indexOf(value);

			addComboBox(tag, options, description);
			getComboBoxComponent(tag)->setSelectedItemIndex(index);
		}
		else if (type == ValueTypes::Text)
		{
			addTextEditor(tag, value, description);
		}
		else if (type == ValueTypes::File)
		{
			fileComponents.add(new FilenameComponent(tag, value, true, true, false, "", "", description));

			fileComponents.getLast()->setSize(400, 24);

			addCustomComponent(fileComponents.getLast());
		}

	}

	addButton("OK", 1, KeyPress(KeyPress::returnKey));
	addButton("Reveal", 2);
	addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
}

File SettingWindows::ProjectSettingWindow::getFile() const
{
	return SettingWindows::getFileForSettingsWindow(Settings::Project, handler);
}

String SettingWindows::ProjectSettingWindow::getAttributeName(int attribute) const
{
	return getAttributeNameForSetting(attribute);
}

String SettingWindows::ProjectSettingWindow::getAttributeNameForSetting(int attribute)
{
	switch ((Attributes)attribute)
	{
	case SettingWindows::ProjectSettingWindow::Attributes::Name:			 return "Name";
	case SettingWindows::ProjectSettingWindow::Attributes::Version:			 return "Version";
	case SettingWindows::ProjectSettingWindow::Attributes::Description:		 return "Description";
	case SettingWindows::ProjectSettingWindow::Attributes::BundleIdentifier: return "BundleIdentifier";
	case SettingWindows::ProjectSettingWindow::Attributes::PluginCode:		 return "PluginCode";
	case SettingWindows::ProjectSettingWindow::Attributes::numAttributes:
	default: return "";
	}
}

XmlElement * SettingWindows::ProjectSettingWindow::createNewSettingsFile() const
{
	XmlElement *xml = new XmlElement("ProjectSettings");

	addChildElement(*xml, (int)Attributes::Name, handler->getWorkDirectory().getFileName(), "Project Name");
	addChildElement(*xml, (int)Attributes::Version, "0.0", "Project version");
	addChildElement(*xml, (int)Attributes::Description, "", "Project description");
	addChildElement(*xml, (int)Attributes::BundleIdentifier, "", "Bundle Identifier(eg.com.myCompany.bundle)");
	addChildElement(*xml, (int)Attributes::PluginCode, "", "a 4 character ID code(eg. 'Abcd')");

	return xml;
}

File SettingWindows::CompilerSettingWindow::getFile() const
{
	return SettingWindows::getFileForSettingsWindow(Settings::Compiler);
}

String SettingWindows::CompilerSettingWindow::getAttributeName(int attribute) const
{
	return getAttributeNameForSetting(attribute);
}

String SettingWindows::CompilerSettingWindow::getAttributeNameForSetting(int attribute)
{
	switch ((Attributes)attribute)
	{
	case Attributes::JucePath: return "JucePath";
	case Attributes::HisePath: return "HisePath";
	case Attributes::IntrojucerPath: return "IntrojucerPath";
	case Attributes::VSTSDKPath: return "VSTSDKPath";
	case Attributes::VisualStudioVersion: return "VisualStudioVersion";
	case Attributes::UseIPP: return "UseIPP";
	case Attributes::IPPLinker: return "IPPLinker";
	case Attributes::IPPInclude: return "IPPInclude";
	case Attributes::IPPLibrary: return "IPPLibrary";
	case Attributes::numCompilerSettingAttributes: return "";
	default: return "";
	}
}

XmlElement * SettingWindows::CompilerSettingWindow::createNewSettingsFile() const
{
#if JUCE_MAC
	// See: https://software.intel.com/en-us/articles/intel-integrated-performance-primitives-for-mac-os-how-to-link-to-the-intel-ipp-for-mac-os-x-libraries-in-application

	const String IPPHeaderPaths = "/opt/intel/ipp/include";
	const String IPPLibraryPaths = "/opt/intel/ipp/lib";
	const String IPPLinkerFlags = "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a";

	const String VstPath = "c:/SDKs/vstsdk2.4";

#else
	// See: https://software.intel.com/en-us/articles/intel-integrated-performance-primitives-for-mac-os-how-to-link-to-the-intel-ipp-for-mac-os-x-libraries-in-application

	const String IPPHeaderPaths = "/opt/intel/ipp/include";
	const String IPPLibraryPaths = "/opt/intel/ipp/lib";
	const String IPPLinkerFlags = "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a";

	const String VstPath = "c:/SDKs/vstsdk2.4";

#endif

	XmlElement *xml = new XmlElement("CompilerSettings");

	addFileAsChildElement(*xml, (int)Attributes::JucePath, "", "Path to JUCE modules");
	addFileAsChildElement(*xml, (int)Attributes::HisePath, "", "Path to HISE modules");
	addFileAsChildElement(*xml, (int)Attributes::IntrojucerPath, "", "Path to Introjucer");
	addFileAsChildElement(*xml, (int)Attributes::VSTSDKPath, VstPath, "Path to VST SDK");
	addChildElementWithOptions(*xml, (int)Attributes::VisualStudioVersion, "Visual Studio 2013", "Installed VisualStudio version", "Visual Studio 2010\nVisual Studio 2012\nVisual Studio 2013\nVisual Studio 2015");
	addChildElementWithOptions(*xml, (int)Attributes::UseIPP, "Yes", "Use IPP", "Yes\nNo");
	addChildElement(*xml, (int)Attributes::IPPLinker, IPPLinkerFlags, "IPP Linker flags");
	addChildElement(*xml, (int)Attributes::IPPInclude, IPPHeaderPaths, "IPP Include path");
	addChildElement(*xml, (int)Attributes::IPPLibrary, IPPLinkerFlags, "IPP Library path");

	return xml;
}


File SettingWindows::UserSettingWindow::getFile() const
{
	return SettingWindows::getFileForSettingsWindow(Settings::User);
}

String SettingWindows::UserSettingWindow::getAttributeName(int attribute) const
{
	return getAttributeNameForSetting(attribute);
}

String SettingWindows::UserSettingWindow::getAttributeNameForSetting(int attribute)
{
	switch ((Attributes)attribute)
	{
	case Company: return "Company";
	case CompanyCode: return "CompanyCode";
	case CompanyURL: return "CompanyURL";
	default: return "";
	}
}

XmlElement * SettingWindows::UserSettingWindow::createNewSettingsFile() const
{
	XmlElement *xml = new XmlElement("UserSettings");

	addChildElement(*xml, (int)Attributes::Company, "", "Company Name");
	addChildElement(*xml, (int)Attributes::CompanyCode, "", "Company Code (4 characters, first must be uppercase)");
	addChildElement(*xml, (int)Attributes::CompanyURL, "http://yourcompany.com", "Company Website");

	return xml;
}

String SettingWindows::getSettingValue(int attributeIndex, ProjectHandler *handler)
{
	Settings s;
	File f;
	String name;

	if (attributeIndex < (int)ProjectSettingWindow::Attributes::numAttributes)
	{
		s = Settings::Project;
		f = getFileForSettingsWindow(s, handler);
		name = ProjectSettingWindow::getAttributeNameForSetting(attributeIndex);
	}
	else if (attributeIndex < (int)CompilerSettingWindow::Attributes::numCompilerSettingAttributes)
	{
		s = Settings::Compiler;
		f = getFileForSettingsWindow(s);
		name = CompilerSettingWindow::getAttributeNameForSetting(attributeIndex);
	}
	else if (attributeIndex < (int)UserSettingWindow::Attributes::numUserSettingAttributes)
	{
		s = Settings::User;
		f = getFileForSettingsWindow(s);
		name = UserSettingWindow::getAttributeNameForSetting(attributeIndex);
	}
	else
	{
		// You need to supply a valid enum!
		jassertfalse;
		return "";
	}

	// The setting file doesn't exist...
	jassert(f.existsAsFile());

	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		XmlElement *child = xml->getChildByName(name);
		if (child != nullptr)
		{
			String s = child->getStringAttribute("value");

			return s;
		}
		else
		{
			jassertfalse;
			return "";
		}
	}
	else
	{
		// The setting file couldn't be parsed...
		jassertfalse;
		return "";
	}
}

void SettingWindows::checkAllSettings(ProjectHandler *handler)
{
	StringArray missingNames;

	bool missingProjectSettings = false;
	bool missingCompilerSettings = false;
	bool missingUserSettings = false;

	int i = 0;

	while(i < (int)ProjectSettingWindow::Attributes::numAttributes)
	{
		if (getSettingValue(i, handler).isEmpty())
		{
			missingProjectSettings = true;
			missingNames.add(ProjectSettingWindow::getAttributeNameForSetting(i));
		}

		i++;
	}

	while(i < (int)CompilerSettingWindow::Attributes::numCompilerSettingAttributes)
	{
		if (getSettingValue(i, handler).isEmpty())
		{
			missingCompilerSettings = true;
			missingNames.add(CompilerSettingWindow::getAttributeNameForSetting(i));
		}

		i++;

	}

	while(i < (int)UserSettingWindow::Attributes::numUserSettingAttributes)
	{
		if (getSettingValue(i).isEmpty())
		{

			missingUserSettings = true;
			missingNames.add(UserSettingWindow::getAttributeNameForSetting(i));
		}

		i++;
	}

	if (missingNames.size() != 0)
	{
		if (PresetHandler::showYesNoWindow("Missing settings found", "Missing Settings:\n\n" + missingNames.joinIntoString("\n") + "\n\nPress OK to open setting windows"))
		{
			if (missingProjectSettings)
			{
				
			}

			if (missingCompilerSettings)
			{
				
			}

			if (missingUserSettings)
			{
				
			}
		}
	}
}

File SettingWindows::getFileForSettingsWindow(Settings s, ProjectHandler *handler)
{
	switch (s)
	{
	case SettingWindows::Settings::Project: 
		if(handler != nullptr) return handler->getWorkDirectory().getChildFile("project_info.xml");
		
		// You must specify the project handler for this directory!
		jassertfalse;
		return File::nonexistent;
		break;
	case SettingWindows::Settings::User: return File(PresetHandler::getDataFolder()).getChildFile("user_info.xml");
		break;
	case SettingWindows::Settings::Compiler: return File(PresetHandler::getDataFolder()).getChildFile("compilerSettings.xml");
		break;
	case SettingWindows::Settings::numSettings:
		break;
	default:
		break;
	}
}
