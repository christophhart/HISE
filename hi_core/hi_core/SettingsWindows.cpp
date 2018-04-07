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

#define P(p) if(prop == p) \
        { s << "### " << String(p) << "\n";
#define D(x) s << x << "\n";
#define P_() return s; } 

struct SettingDescription
{
    
    
    static void addChildElement(XmlElement &element, const String& attributeName, const String &childValue, const String &description)
    {
        XmlElement *child = new XmlElement(attributeName);
        child->setAttribute("value", childValue);
        child->setAttribute("type", "TEXT");
        child->setAttribute("description", description);
        
        element.addChildElement(child);
    }
    
    static void addChildElementWithOptions(XmlElement &element, const String& attributeName, const String &childValue, const String &description, const String &optionsAsLines)
    {
        XmlElement *child = new XmlElement(attributeName);
        child->setAttribute("value", childValue);
        child->setAttribute("type", "LIST");
        child->setAttribute("description", description);
        child->setAttribute("options", optionsAsLines);
        
        element.addChildElement(child);
    }
    
    static void addFileAsChildElement(XmlElement &element, const String& attributeName, const String &childValue, const String &description)
    {
        XmlElement *child = new XmlElement(attributeName);
        child->setAttribute("value", childValue);
        child->setAttribute("type", "FILE");
        child->setAttribute("description", description);
        
        element.addChildElement(child);
    }
    
    static ValueTree createNewValueTree(SettingWindows::Settings s, ProjectHandler* handler)
    {
        switch(s)
        {
            case SettingWindows::Settings::Project:
            {
                ScopedPointer<XmlElement> xml = new XmlElement("ProjectSettings");
                
                addChildElement(*xml, "Name", handler->getWorkDirectory().getFileName(), "Project Name");
                addChildElement(*xml, "Version", "0.1.0", "Project version");
                addChildElement(*xml, "Description", "", "Project description");
                addChildElement(*xml, "BundleIdentifier", "com.myCompany.product", "Bundle Identifier(eg.com.myCompany.bundle)");
                addChildElement(*xml, "PluginCode", "Abcd", "a 4 character ID code(eg. 'Abcd')");
                addChildElementWithOptions(*xml, "EmbedAudioFiles", "No", "Embed Audio files in plugin", "Yes\nNo");
                addChildElement(*xml, "AdditionalDspLibraries", "", "comma separated list of all static dsp factory classes");
                addChildElement(*xml, "WindowsStaticLibFolder", "", "Windows static library folder");
                addChildElement(*xml, "OSXStaticLibs", "", "Additional static libs (OSX only)");
                addChildElement(*xml, "ExtraDefinitionsWindows", "", "Extra preprocessor definitions for Windows");
                addChildElement(*xml, "ExtraDefinitionsOSX", "", "Extra preprocessor definitions for OSX");
                addChildElement(*xml, "ExtraDefinitionsIOS", "", "Extra preprocessor definitions for IOS");
                addChildElement(*xml, "AppGroupId", "", "App Group ID (use this for shared resources on iOS");
                
                return ValueTree::fromXml(*xml);
                
            }
            case SettingWindows::Settings::User:
            {
                ScopedPointer<XmlElement> xml = new XmlElement("UserSettings");
                
                addChildElement(*xml, "Company", "My Company", "Company Name");
                addChildElement(*xml, "CompanyCode", "Abcd", "Company Code (4 characters, first must be uppercase)");
                addChildElement(*xml, "CompanyCopyright", "(c)2017, Company", "Company Copyright");
                addChildElement(*xml, "CompanyURL", "http://yourcompany.com", "Company Website");
                addChildElement(*xml, "TeamDevelopmentId", "", "Apple Distribution ID");
                
                return ValueTree::fromXml(*xml);
            }
            case SettingWindows::Settings::Compiler:
            {
                ScopedPointer<XmlElement> xml = new XmlElement("CompilerSettings");
                
                addFileAsChildElement(*xml, "HisePath", "", "Path to HISE modules");
                addChildElementWithOptions(*xml, "VisualStudioVersion", "Visual Studio 2017", "Installed VisualStudio version", "Visual Studio 2015\nVisual Studio 2017");
                addChildElementWithOptions(*xml, "UseIPP", "Yes", "Use IPP", "Yes\nNo");

                return ValueTree::fromXml(*xml);
            }
        }
        
        return {};
    }
    
	static String getDescription(const String& prop)
	{
		String s;

		P("Name");
		D("The name of the project. This will be also the name of the plugin binaries");
		P_();

		P("Version");
		D("The version number of the project. Try using semantic versioning (`1.0.0`) for this.  ");
		D("The version number will be used to handle the user preset backward compatibility.");
		D("> Be aware that some hosts (eg. Logic) are very picky when they detect different plugin binaries with the same version.");
		P_();

		P("Bundle Identifier");
		D("This is a unique identifier used by Apple OS to identify the app. It must be formatted as reverse domain like this:");
		D("> `com.your-company.product`");
		P_();

		P("Plugin Code");
		D("The code used to identify the plugins. This has to be four characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		P("Embed Audio Files");
		D("If this is **enabled**, it will embed all audio files (impulse responses & loops) **as well as images** into the plugin.");
		D("This will not affect samples - they will always be streamed.  ");
		D("If it's **disabled**, it will use the resource files found in the app data directory and you need to make sure that your installer");
		D("copies them to the right location:");
		D("> **Windows:** `%APPDATA%\\Company\\Product\\`");
		D("> **macOS:** `~/Library/Application Support/Company/Product/`");
		D("Normally you would try to embed them into the binary, however if you have a lot of images and audio files (> 50MB)");
		D("the compiler will crash with an **out of heap space** error, so in this case you're better off not embedding them.");
		P_();

		P("Additional Dsp Libraries");
		D("If you have written custom DSP objects that you want to embed statically, you have to supply the class names of each DspModule class here");
		P_();

		P("Windows Static Lib Folder");
		D("If you need to link a static library on Windows, supply the absolute path to the folder here. Unfortunately, relative paths do not work well with the VS Linker");
		P_();

		P("OSXStatic Libs");
		D("If you need to link a static library on macOS, supply the path to the .a library file here.");
		P_();

		P("Extra Definitions Windows");
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();
			
		P("Extra Definitions OSX");
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();

		P("Extra Definitions IOS");
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();
	
		P("App Group ID");
		D("If you're compiling an iOS app, you need to add an App Group to your Apple ID for this project and supply the name here.");
		D("App Group IDs must have reverse-domain format and start with group, like:");
		D("> `group.company.product`");
		P_();

		P("Company");
		D("Your company name. This will be used for the path to the app data directory so make sure you don't use weird characters here");
		P_();

		P("Company Code");
		D("The unique code to identify your company. This must be 4 characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		
		P("Team Development ID");
		D("If you have a Apple Developer Account, enter the Developer ID here in order to code sign your app / plugin after compilation");
		P_();

		P("Visual Studio Version");
		D("Set the VS version that you've installed. Make sure you always use the latest one, since I need to regularly deprecate the oldest version");
		P_();

		P("Use IPP");
		D("If enabled, HISE uses the FFT routines from the Intel Performance Primitive library (which can be downloaded for free) in order");
		D("to speed up the convolution reverb");
		D("> If you use the convolution reverb in your project, this is almost mandatory, but there are a few other places that benefit from having this library");
		P_();

		return s;

	};
};

#undef P
#undef D
#undef P_


#define GET_VALUE_FROM_XML(attribute) xmlSettings.getChildByName(getAttributeNameForSetting((int)attribute))->getStringAttribute("value");

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

void SettingWindows::BaseSettingsWindow::threadFinished()
{
	const String sanityCheckResult = sanityCheck(*settings);

	if (sanityCheckResult.isNotEmpty())
	{
		PresetHandler::showMessageWindow("Warning", sanityCheckResult, PresetHandler::IconType::Warning);
	}
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

	jassertfalse;
	return ValueTypes::numValueTypes;
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

		if (compare->getNumChildElements() > settings->getNumChildElements())
		{
			for (int i = 0; i < compare->getNumChildElements(); i++)
			{
				if (settings->getChildByName(compare->getChildElement(i)->getTagName()) == nullptr)
				{
					settings->addChildElement(XmlDocument::parse(compare->getChildElement(i)->createDocument("")));
				}
			}
		}
	}

	for (int i = 0; i < settings->getNumChildElements(); i++)
	{
		ValueTypes type = getValueType(settings->getChildElement(i));

        auto settingsChild = settings->getChildElement(i);

        const String tag = settingsChild->getTagName();

        auto templateChild = compare->getChildByName(tag);

        if (templateChild != nullptr)
        {
            const String value = settings->getChildElement(i)->getStringAttribute("value");
            const String description = settings->getChildElement(i)->getStringAttribute("description");

            if (type == ValueTypes::List)
            {
                const StringArray options = StringArray::fromLines(templateChild->getStringAttribute("options"));
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
        else
            jassertfalse;
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
	case SettingWindows::ProjectSettingWindow::Attributes::EmbedAudioFiles:	 return "EmbedAudioFiles";
	case SettingWindows::ProjectSettingWindow::Attributes::AdditionalDspLibraries:	return "AdditionalDspLibraries";
	case SettingWindows::ProjectSettingWindow::Attributes::OSXStaticLibs:   return "OSXStaticLibs";
	case SettingWindows::ProjectSettingWindow::Attributes::WindowsStaticLibFolder: return "WindowsStaticLibFolder";
	case SettingWindows::ProjectSettingWindow::Attributes::ExtraDefinitionsOSX: return "ExtraDefinitionsOSX";
	case SettingWindows::ProjectSettingWindow::Attributes::ExtraDefinitionsWindows: return "ExtraDefinitionsWindows";
	case SettingWindows::ProjectSettingWindow::Attributes::ExtraDefinitionsIOS: return "ExtraDefinitionsIOS";
    case Attributes::AppGroupId: return "AppGroupID";
	case SettingWindows::ProjectSettingWindow::Attributes::numAttributes:
	default: return "";
	}
}

XmlElement * SettingWindows::ProjectSettingWindow::createNewSettingsFile() const
{
	XmlElement *xml = new XmlElement("ProjectSettings");

	addChildElement(*xml, (int)Attributes::Name, handler->getWorkDirectory().getFileName(), "Project Name");
	addChildElement(*xml, (int)Attributes::Version, "0.1.0", "Project version");
	addChildElement(*xml, (int)Attributes::Description, "", "Project description");
	addChildElement(*xml, (int)Attributes::BundleIdentifier, "com.myCompany.product", "Bundle Identifier(eg.com.myCompany.bundle)");
	addChildElement(*xml, (int)Attributes::PluginCode, "Abcd", "a 4 character ID code(eg. 'Abcd')");
	addChildElementWithOptions(*xml, (int)Attributes::EmbedAudioFiles, "No", "Embed Audio files in plugin", "Yes\nNo");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::AdditionalDspLibraries, "", "comma separated list of all static dsp factory classes");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::WindowsStaticLibFolder, "", "Windows static library folder");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::OSXStaticLibs, "", "Additional static libs (OSX only)");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::ExtraDefinitionsWindows, "", "Extra preprocessor definitions for Windows");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::ExtraDefinitionsOSX, "", "Extra preprocessor definitions for OSX");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::ExtraDefinitionsIOS, "", "Extra preprocessor definitions for IOS");
    addChildElement(*xml, (int)Attributes::AppGroupId, "", "App Group ID (use this for shared resources on iOS");
    
	return xml;
}



String SettingWindows::ProjectSettingWindow::sanityCheck(const XmlElement& xmlSettings)
{
	const String version = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::Version);

	SemanticVersionChecker versionChecker(version, version);

	if (!versionChecker.newVersionNumberIsValid())
	{
		return "The version number is not a valid semantic version number. Use something like 1.0.0.\n " \
			   "This is required for the user presets to detect whether it should ask for updating the presets after a version bump.";
	};

	const String pluginCode = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::PluginCode);
	const String codeWildcard = "[A-Z][a-z][a-z][a-z]";

	if (!RegexFunctions::matchesWildcard(codeWildcard, pluginCode))
	{
		return "The plugin code doesn't match the required formula. Use something like 'Abcd'\n" \
			   "This is required for exported AU plugins to pass the AU validation.";
	};

	const String projectName = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::Name);

	if (!projectName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
	{
		return "Illegal Project name\n" \
			   "The Project name must not contain exotic characters";
	}

	return String();
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
	case Attributes::HisePath: return "HisePath";
	case Attributes::VisualStudioVersion: return "VisualStudioVersion";
	case Attributes::UseIPP: return "UseIPP";
	case Attributes::numCompilerSettingAttributes: return "";
	default: return "";
	}
}

XmlElement * SettingWindows::CompilerSettingWindow::createNewSettingsFile() const
{
	XmlElement *xml = new XmlElement("CompilerSettings");

	addFileAsChildElement(*xml, (int)Attributes::HisePath, "", "Path to HISE modules");
	addChildElementWithOptions(*xml, (int)Attributes::VisualStudioVersion, "Visual Studio 2017", "Installed VisualStudio version", "Visual Studio 2015\nVisual Studio 2017");
	addChildElementWithOptions(*xml, (int)Attributes::UseIPP, "Yes", "Use IPP", "Yes\nNo");

	return xml;
}


File SettingWindows::UserSettingWindow::getFile() const
{
	return SettingWindows::getFileForSettingsWindow(Settings::User, handler);
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
    case CompanyCopyright:  return "CompanyCopyright";
    case TeamDevelopmentId: return "TeamDevelopmentID";
	default: return "";
	}
}

XmlElement * SettingWindows::UserSettingWindow::createNewSettingsFile() const
{
	XmlElement *xml = new XmlElement("UserSettings");

	addChildElement(*xml, (int)Attributes::Company, "My Company", "Company Name");
	addChildElement(*xml, (int)Attributes::CompanyCode, "Abcd", "Company Code (4 characters, first must be uppercase)");
    addChildElement(*xml, (int)Attributes::CompanyCopyright, "(c)2017, Company", "Company Copyright");
	addChildElement(*xml, (int)Attributes::CompanyURL, "http://yourcompany.com", "Company Website");
    addChildElement(*xml, (int)Attributes::TeamDevelopmentId, "", "Apple Distribution ID");

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
		f = getFileForSettingsWindow(s, handler);
		name = UserSettingWindow::getAttributeNameForSetting(attributeIndex);
	}
	else
	{
		// You need to supply a valid enum!
		jassertfalse;
		return "";
	}

	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		XmlElement *child = xml->getChildByName(name);
		if (child != nullptr)
		{
			String valueText = child->getStringAttribute("value");

			return valueText;
		}
		else
		{
			jassertfalse;
			return "";
		}
	}
	else
	{
		// The setting file couldn't be parsed...s
#if USE_BACKEND
		jassert(CompileExporter::isExportingFromCommandLine());
#endif
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

	while (i < (int)ProjectSettingWindow::Attributes::numAttributes)
	{
		if (getSettingValue(i, handler).isEmpty())
		{
			missingProjectSettings = true;
			missingNames.add(ProjectSettingWindow::getAttributeNameForSetting(i));
		}

		i++;
	}

	while (i < (int)CompilerSettingWindow::Attributes::numCompilerSettingAttributes)
	{
		if (getSettingValue(i, handler).isEmpty())
		{
			missingCompilerSettings = true;
			missingNames.add(CompilerSettingWindow::getAttributeNameForSetting(i));
		}

		i++;

	}

	while (i < (int)UserSettingWindow::Attributes::numUserSettingAttributes)
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
		if (PresetHandler::showYesNoWindow("Missing settings found", "Missing Settings:\n\n" + missingNames.joinIntoString("\n") + "\n\nPress OK to open setting windows", PresetHandler::IconType::Warning))
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
		if (handler != nullptr) return handler->getWorkDirectory().getChildFile("project_info.xml");

		// You must specify the project handler for this directory!
		jassertfalse;
		return File();
		break;
	case SettingWindows::Settings::User:
		if (handler != nullptr) return handler->getWorkDirectory().getChildFile("user_info.xml");
		break;
	case SettingWindows::Settings::Compiler: return File(PresetHandler::getDataFolder()).getChildFile("compilerSettings.xml");
		break;
	case SettingWindows::Settings::Audio:	 return File(PresetHandler::getDataFolder()).getChildFile("DeviceSettings.xml");
	case SettingWindows::Settings::Global:	 return File(PresetHandler::getDataFolder()).getChildFile("GeneralSettings.xml");
	case SettingWindows::Settings::numSettings:
		break;
	default:
		break;
	}

	return File();
}

#undef GET_VALUE_FROM_XML




class SettingWindows::NewSettingWindows::Content : public Component
{
public:

	Content(NewSettingWindows* parent)
	{
		addAndMakeVisible(&properties);
		properties.setLookAndFeel(&pplaf);

		pplaf.setFontForAll(GLOBAL_BOLD_FONT());
		pplaf.setLabelWidth(190);

		setSearchText("");
	}

	void setSearchText(const String& searchText)
	{
		properties.clear();
	}

	void resized() override
	{
		properties.setBounds(getLocalBounds());
	}

	HiPropertyPanelLookAndFeel pplaf;

	PropertyPanel properties;

};

SettingWindows::NewSettingWindows::NewSettingWindows(ProjectHandler* handler_) :
	handler(handler_),
	projectSettings("Project"),
	globalSettings("Global"),
	allSettings("All"),
	applyButton("Save"),
	cancelButton("Cancel")
{
	addAndMakeVisible(&projectSettings);
	projectSettings.addListener(this);
	projectSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&globalSettings);
	globalSettings.addListener(this);
	globalSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&allSettings);
	allSettings.addListener(this);
	allSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&applyButton);
	applyButton.addListener(this);
	applyButton.setLookAndFeel(&alaf);
	applyButton.addShortcut(KeyPress(KeyPress::returnKey));

	addAndMakeVisible(&cancelButton);
	cancelButton.addListener(this);
	cancelButton.setLookAndFeel(&alaf);
	cancelButton.addShortcut(KeyPress(KeyPress::escapeKey));

	projectSettings.setRadioGroupId(1, dontSendNotification);
	allSettings.setRadioGroupId(1, dontSendNotification);
	globalSettings.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(currentContent = new Content(this));

	

	addAndMakeVisible(&fuzzySearchBox);
	fuzzySearchBox.addListener(this);
	fuzzySearchBox.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	fuzzySearchBox.setFont(GLOBAL_FONT());
	fuzzySearchBox.setSelectAllWhenFocused(true);
	fuzzySearchBox.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

	settings.add(getProperlyFormattedValueTree(Settings::Project));
	settings.add(getProperlyFormattedValueTree(Settings::User));
	settings.add(getProperlyFormattedValueTree(Settings::Compiler));

	

	setSize(800, 650);

	allSettings.setToggleState(true, sendNotificationSync);
}

SettingWindows::NewSettingWindows::~NewSettingWindows()
{
	if (saveOnDestroy)
	{
		for (auto& s : settings)
		{
			s->save();
		}
	}
}

String getUncamelcasedId(const Identifier& id)
{
	auto n = id.toString();

	String pretty;

	auto ptr = n.getCharPointer();

	bool lastWasUppercase = true;

	while (!ptr.isEmpty())
	{
		if (ptr.isUpperCase() && !lastWasUppercase)
		{
			pretty << " ";
		}
		
		lastWasUppercase = ptr.isUpperCase();

		pretty << ptr.getAddress()[0];

		ptr++;
	}

	return pretty;
}





void SettingWindows::NewSettingWindows::resized()
{
	auto area = getLocalBounds().reduced(1);

	area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	searchBar.removeFromLeft(32);
	searchBar.removeFromBottom(4);
	fuzzySearchBox.setBounds(searchBar);

	auto bottom = area.removeFromBottom(80);

	bottom = bottom.withSizeKeepingCentre(200, 40);

	applyButton.setBounds(bottom.removeFromLeft(100).reduced(5));
	cancelButton.setBounds(bottom.removeFromLeft(100).reduced(5));

	auto left = area.removeFromLeft(120);

	
	allSettings.setBounds(left.removeFromTop(40));
	projectSettings.setBounds(left.removeFromTop(40));
	globalSettings.setBounds(left.removeFromTop(40));
	

	currentContent->setBounds(area.reduced(10));
	
}


void SettingWindows::NewSettingWindows::paint(Graphics& g)
{
	g.fillAll(Colour(bgColour));

	auto area = getLocalBounds().reduced(1);

	
	

	auto top = area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	auto s_ = searchBar.removeFromBottom(4);
	auto shadow = FLOAT_RECTANGLE(s_);


	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));

	g.fillRect(searchBar);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));

	g.fillRect(shadow);

	g.setColour(Colour(tabBgColour));

	g.fillRect(top);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));

	g.setColour(Colours::white);
	g.drawText("Settings", FLOAT_RECTANGLE(top), Justification::centred);

	auto bottom = area.removeFromBottom(80);



	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(tabBgColour)));


	g.fillRect(bottom);

	auto s_2 = bottom.removeFromTop(4);

	auto shadow2 = FLOAT_RECTANGLE(s_2);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow2.getY(), Colours::transparentBlack, 0.0f, shadow2.getBottom(), false));

	g.fillRect(shadow2);


	auto left = area.removeFromLeft(120);

	g.setGradientFill(ColourGradient(Colour(tabBgColour), 0.0f, 0.0f, Colour(0xFF222222), 0.0f, (float)getHeight(), false));


	g.fillRect(left);

	

	g.setColour(Colours::white.withAlpha(0.6f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	path.scaleToFit((float)searchBar.getX()+4.0f, (float)searchBar.getY() + 4.0f, 20.0f, 20.0f, true);

	g.fillPath(path);

	g.setColour(Colour(0xFF666666));
	g.drawRect(getLocalBounds(), 1);

}

void SettingWindows::NewSettingWindows::textEditorTextChanged(TextEditor&)
{
	setContent(currentList);
}

void SettingWindows::NewSettingWindows::setContent(SettingList list)
{
	currentContent->properties.clear();

	currentList = list;

	for (auto vt : settings)
	{
		if (list.contains(vt->s))
		{
			vt->fillPropertyPanel(currentContent->properties, fuzzySearchBox.getText().toLowerCase());
		}
	}
	
	resized();
}

void SettingWindows::NewSettingWindows::TabButtonLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown)
{
	
	auto bounds = b.getLocalBounds();

	

	auto s_ = bounds.removeFromBottom(3);

	auto shadow = FLOAT_RECTANGLE(s_);

	if (b.getToggleState())
	{
		g.setColour(Colour(ColourValues::bgColour));
		g.fillRect(bounds);
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));
		g.fillRect(shadow);
	}
		

	if (isButtonDown)
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(bounds);
	}

	g.setColour(Colours::white.withAlpha(isMouseOverButton ? 1.0f : 0.8f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(b.getButtonText(), bounds.reduced(5), Justification::centredRight);

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(b.getBottom()-3, b.getX(), b.getRight());
}

void SettingWindows::NewSettingWindows::FileBasedValueTree::fillPropertyPanel(PropertyPanel& panel, const String& searchText)
{
	Array<PropertyComponent*> props;



	for (auto c : v)
	{


		auto searchString = c.getProperty("description").toString() + " " + c.getType().toString();
		searchString = searchString.toLowerCase();

		if (searchText.isEmpty() || FuzzySearcher::fitsSearch(searchText, searchString, 0.2))
		{
			addProperty(c, props);
		}
	}

	if (props.size() > 0)
	{
		panel.addSection(getId(), props);

		for (auto p : props)
		{
			auto n = p->getName();

			auto help = SettingDescription::getDescription(n);

			if (help.isNotEmpty())
			{
				MarkdownHelpButton* helpButton = new MarkdownHelpButton();
				helpButton->setFontSize(15.0f);
				helpButton->setHelpText(help);
				helpButton->attachTo(p, MarkdownHelpButton::OverlayLeft);
			}
		}
	}
}

void SettingWindows::NewSettingWindows::FileBasedValueTree::addProperty(ValueTree& c, Array<PropertyComponent*>& props)
{
	auto value = c.getPropertyAsValue("value", nullptr);

	auto type = c.getProperty("type").toString();

	auto name = getUncamelcasedId(c.getType());




	if (type == "TEXT")
	{
		props.add(new TextPropertyComponent(value, name, 1024, name.contains("Extra")));
	}
	else if (type == "LIST")
	{
		auto items = c.getProperty("options").toString();

		if (items == "Yes\nNo")
		{
			auto bpc = new BooleanPropertyComponent(value, name, "Enabled");

			dynamic_cast<ToggleButton*>(bpc->getChildComponent(0))->setLookAndFeel(&blaf);

			bpc->setColour(BooleanPropertyComponent::ColourIds::backgroundColourId, Colours::transparentBlack);
			bpc->setColour(BooleanPropertyComponent::ColourIds::outlineColourId, Colours::transparentBlack);

			props.add(bpc);
		}
		else
		{
			StringArray choices = StringArray::fromLines(items);

			Array<var> choiceValues;

			for (auto s : choices)
				choiceValues.add(s);

			props.add(new ChoicePropertyComponent(value, name, choices, choiceValues));
		}
	}

	


}

juce::String SettingWindows::NewSettingWindows::FileBasedValueTree::getId() const
{
	return getUncamelcasedId(v.getType());
}

SettingWindows::NewSettingWindows::FileBasedValueTree* SettingWindows::NewSettingWindows::getProperlyFormattedValueTree(Settings s)
{
    auto f = getFileForSettingsWindow(s, handler);
    
    ScopedPointer<XmlElement> xml = XmlDocument::parse(f);
                
    if (xml != nullptr)
    {
        return new FileBasedValueTree(s, ValueTree::fromXml(*xml), f);
    }
    
    return new  FileBasedValueTree( s, SettingDescription::createNewValueTree(s, handler) , f );
}
            
void SettingWindows::NewSettingWindows::FileBasedValueTree::save()
{
	for (auto c : v)
	{
		if (c.getProperty("options").toString() == "Yes&#10;No")
		{
			c.setProperty("value", c.getProperty("value")? "Yes" : "No", nullptr);
		}
	}

	ScopedPointer<XmlElement> xml = v.createXml();


	xml->writeToFile(f, "");
}

} // namespace hise
