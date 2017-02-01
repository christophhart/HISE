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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


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
	case SettingWindows::ProjectSettingWindow::Attributes::EmbedAudioFiles:	 return "EmbedAudioFiles";
	case SettingWindows::ProjectSettingWindow::Attributes::AdditionalDspLibraries:	return "AdditionalDspLibraries";
	case SettingWindows::ProjectSettingWindow::Attributes::CustomToolbarClassName: return "CustomToolbarClassName";
	case SettingWindows::ProjectSettingWindow::Attributes::OSXStaticLibs:   return "OSXStaticLibs";
	case SettingWindows::ProjectSettingWindow::Attributes::WindowsStaticLibFolder: return "WindowsStaticLibFolder";
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
	addChildElementWithOptions(*xml, (int)Attributes::EmbedAudioFiles, "No", "Embed Audio files in plugin", "Yes\nNo");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::AdditionalDspLibraries, "", "comma separated list of all static dsp factory classes");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::CustomToolbarClassName, "", "Class name for the custom toolbar (leave empty to use the default one");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::WindowsStaticLibFolder, "", "Windows static library folder");
	addChildElement(*xml, (int)ProjectSettingWindow::Attributes::OSXStaticLibs, "", "Additional static libs (OSX only)");

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
	addChildElementWithOptions(*xml, (int)Attributes::VisualStudioVersion, "Visual Studio 2013", "Installed VisualStudio version", "Visual Studio 2013\nVisual Studio 2015");
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
	case SettingWindows::Settings::numSettings:
		break;
	default:
		break;
	}

	return File();
}
