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




juce::String MenuReferenceDocGenerator::Resolver::getContent(const MarkdownLink& url)
{
	auto workspaceRoot = rootURL.getChildUrl("workspaces");

	if(url.isChildOf(workspaceRoot))
	{
		auto s = url.toString(MarkdownLink::ContentFull);

		auto table = generateIconTable(url.toString(MarkdownLink::UrlSubPath), url.toString(MarkdownLink::ContentHeader));
		s = s.replace("{ICON_TABLE}", table);
		
		return s;
	}

	return {};
}

juce::String MenuReferenceDocGenerator::Resolver::generateIconTable(const String& url, const String& fileContent) const
{
	

	String s;
	String nl = "\n";
	
	PathFactory* fToUse = nullptr;

	for (auto f : data->factories)
	{


		if (MarkdownLink::Helpers::getSanitizedFilename(f->getId()) == url)
		{
			fToUse = f;
			break;
		}
	}

	if (fToUse != nullptr)
	{
		auto keyMappings = fToUse->getKeyMapping();
		
		s << "| Icon | " << (keyMappings.isEmpty() ? "" : " Shortcut | ") << "Name | Description |" << nl;
		s << "| ---:64px | " << (keyMappings.isEmpty() ? "" : "---:100px | ") << "--- | ------ |" << nl;

		fToUse->createPath("");

		auto ids = fToUse->ids;

		Array<PathFactory::Description> descriptions;

		MarkdownParser parser(fileContent);
		parser.parse();
		auto sa = parser.getHeader().getKeyList("items");

		for (const auto& s2 : sa)
		{
			auto key = s2.upToFirstOccurrenceOf(":", false, false);
			auto value = s2.fromFirstOccurrenceOf(":", false, false);

			descriptions.add({ key, value });
		}

		for (auto id : ids)
		{
			auto pretty = MarkdownLink::Helpers::getPrettyName(id);

			s << "| ![" << pretty << "](/images/icon_" << id << ":24px) | ";


			bool addEmpty = keyMappings.size() > 0;

			for (const auto& k : keyMappings)
			{
				if (k.url == id)
				{
					s << "`" << k.k.getTextDescriptionWithIcons() << "` | ";
					addEmpty = false;
					break;
				}
			}

			if (addEmpty)
			{
				s << "` ` | ";
			}

			s << pretty << " | ";


			String description = "description";

			for (auto d : descriptions)
			{
				if (d.url == id)
				{
					description = d.description;
					break;
				}
			}

			s << description << " |" << nl;
		}

		return s;
	}

	return {};
}

void MenuReferenceDocGenerator::ItemGenerator::createMenuReference(MarkdownDataBase::Item& parent)
{
	MarkdownDataBase::Item wItem;
	wItem.url = rootURL.getChildUrl("menu-reference").withRoot(rootDirectory, true);

	auto f = wItem.url.getMarkdownFile({});

	MarkdownParser::createDatabaseEntriesForFile(rootDirectory, wItem, f, parent.c);

	data->createMenuCommandInfos();

	if (data->bp->shouldAbort())
		return;

	StringArray categories;

	for (const auto& r : data->commandInfos)
	{
		if (data->bp->shouldAbort())
			return;

		if (r.categoryName.isNotEmpty())
			categories.addIfNotAlreadyThere(r.categoryName);
	}
	
	for (auto s : categories)
	{
		if (s == "Unused")
			continue;

		if (data->bp->shouldAbort())
			return;

		createMenu(wItem, s);
	}

	wItem.sortChildren();

	parent.addChild(std::move(wItem));
}

void MenuReferenceDocGenerator::ItemGenerator::createMenu(MarkdownDataBase::Item& parent, const String& menuName)
{
	MarkdownDataBase::Item menuItem;
	menuItem.url = parent.url.getChildUrl(menuName);
	menuItem.url.setType(MarkdownLink::Folder);
	menuItem.fillMetadataFromURL();
	menuItem.c = parent.c;
	menuItem.tocString = menuName;
	menuItem.keywords.add(menuName);
	
	

	data->createMenuCommandInfos();

	for (const auto& info : data->commandInfos)
	{
		if (info.categoryName == "Unused")
			continue;

		if (info.categoryName == menuName)
		{
			MarkdownDataBase::Item rItem;

			rItem.c = menuItem.c;
			rItem.url = menuItem.url.getChildUrl(info.shortName, true);
			rItem.tocString = info.shortName;
			rItem.keywords.add("Menu | " + menuName);
			rItem.description = info.shortName;

			menuItem.addChild(std::move(rItem));
		}
	}

	parent.addChild(std::move(menuItem));
}

void MenuReferenceDocGenerator::ItemGenerator::createAndAddWorkspacesItem(MarkdownDataBase::Item& parent)
{
	MarkdownDataBase::Item wItem;

	wItem.c = parent.c;
	wItem.tocString = "Workspaces";
	wItem.url = parent.url.getChildUrl(wItem.tocString);
	wItem.url.setType(MarkdownLink::Folder);

	createAndAddWorkspace(wItem, "Main Workspace");
	createAndAddWorkspace(wItem, "Scripting Workspace");
	createAndAddWorkspace(wItem, "Sampler Workspace");
	createAndAddWorkspace(wItem, "Custom Workspace");

	parent.addChild(std::move(wItem));
}

void MenuReferenceDocGenerator::ItemGenerator::createAndAddWorkspace(MarkdownDataBase::Item& parent, const String& id)
{
	MarkdownDataBase::Item wItem;

	wItem.c = parent.c;
	wItem.tocString = id;
	wItem.url = parent.url.getChildUrl(id).withRoot(rootDirectory, true);
	wItem.icon = wItem.url.getHeaderFromFile({}).getIcon();


	auto d = wItem.url.getDirectory({});

	Array<File> files;

	d.findChildFiles(files, File::findFiles, true, "*.md");

	for (auto f : files)
	{
		if (MarkdownLink::Helpers::isReadme(f))
			continue;

		MarkdownDataBase::Item i;
		i.url = { rootDirectory, f.getRelativePathFrom(rootDirectory) };

		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, i, f, wItem.c);
		wItem.addChild(std::move(i));
	}

	parent.addChild(std::move(wItem));
}

void MenuReferenceDocGenerator::ItemGenerator::createSettingsItem(MarkdownDataBase::Item& parent)
{
	auto url = parent.url.getChildUrlWithRoot("settings");

	auto f = url.getMarkdownFile({});
	auto header = url.getHeaderFromFile({});

	MarkdownDataBase::Item sItem(rootDirectory,
		f,
		header.getKeywords(),
		header.getDescription());

	sItem.url = url;

	sItem.tocString << header.getDescription();
	sItem.c = parent.c;
	sItem.icon = header.getKeyValue("icon");

	createSettingSubMenu(sItem, "Project");
	createSettingSubMenu(sItem, "Development");

	createSettingSubMenu(sItem, "Audio MIDI");
	
	parent.addChild(std::move(sItem));
}

void MenuReferenceDocGenerator::ItemGenerator::createSettingSubMenu(MarkdownDataBase::Item& parent, const String& name)
{
	auto url = parent.url.getChildUrlWithRoot(name);
	auto f = url.getMarkdownFile({});
	auto header = url.getHeaderFromFile({});

	MarkdownDataBase::Item subItem(rootDirectory, f, header.getKeywords(), header.getDescription());

	subItem.url = url;
	subItem.url.setType(MarkdownLink::MarkdownFile);
	subItem.tocString << name;

	auto subId = MarkdownLink::Helpers::getSanitizedFilename(name);

	if(subId == "project")
	{
		addItemForSettingList(HiseSettings::Project::getAllIds(), "Project", subItem);
		addItemForSettingList(HiseSettings::User::getAllIds(), "User", subItem);
	}
	if (subId == "development")
	{
		addItemForSettingList(HiseSettings::Compiler::getAllIds(), "Compiler", subItem);
		addItemForSettingList(HiseSettings::Scripting::getAllIds(), "Scripting", subItem);
		addItemForSettingList(HiseSettings::Other::getAllIds(), "Other", subItem);
		addItemForSettingList(HiseSettings::Documentation::getAllIds(), "Documentation", subItem);
	}
	if (subId == "audio-midi")
	{
		addItemForSettingList(HiseSettings::Audio::getAllIds(), "Audio", subItem);
		addItemForSettingList(HiseSettings::Midi::getAllIds(), "MIDI", subItem);
	}

	parent.addChild(std::move(subItem));
}

void MenuReferenceDocGenerator::ItemGenerator::addItemForSettingList(const Array<Identifier>& idList, const String& subName, MarkdownDataBase::Item& parent)
{
	for (auto id : idList)
	{
		MarkdownDataBase::Item item;
		item.keywords.add("Settings | " + subName);
		
		item.description = HiseSettings::ConversionHelpers::getUncamelcasedId(id);
		item.tocString = item.description;
		item.url = parent.url.getChildUrl(item.description, true);
		item.c = parent.c;

		parent.addChild(std::move(item));
	}
}

hise::MarkdownDataBase::Item MenuReferenceDocGenerator::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	MarkdownDataBase::Item item;

	item.c = Colours::burlywood;
	item.url = rootURL;
	item.icon = "/images/icon_hise";
	item.fillMetadataFromURL();

	auto url = rootURL.getChildUrl("project-management");

	auto f = url.getDirectory(rootDirectory);

	if (f.isDirectory())
	{
		MarkdownDataBase::DirectoryItemGenerator fGenerator(f, item.c);
		auto pItem = fGenerator.createRootItem(parent);
		item.addChild(std::move(pItem));
	}

	if (data->bp->shouldAbort())
		return item;

	createAndAddWorkspacesItem(item);
	createMenuReference(item);

	if (data->bp->shouldAbort())
		return item;

	createSettingsItem(item);

	if (data->bp->shouldAbort())
		return item;

	item.setDefaultColour(colour);

	return item;
}

juce::String MenuReferenceDocGenerator::MenuGenerator::getContent(const MarkdownLink& url)
{
	if (!url.isChildOf(rootURL.getChildUrl("Menu Reference")))
		return {};

	auto urlWithoutAnchor = url.withAnchor({});

	auto f = urlWithoutAnchor.getMarkdownFile({});

	if (f.existsAsFile())
	{
		String s;

		


		auto name = urlWithoutAnchor.toString(MarkdownLink::UrlSubPath);
		
		s << urlWithoutAnchor.toString(MarkdownLink::ContentFull) << "\n";

		data->createMenuCommandInfos();

		for (const auto& info : data->commandInfos)
		{
			if (MarkdownLink::Helpers::getSanitizedFilename(info.categoryName) == name)
			{
				s << "### " << info.shortName << "\n";

				if (info.defaultKeypresses.size() > 0)
				{
					s << "**Shortcut:** `" << info.defaultKeypresses.getFirst().getTextDescriptionWithIcons() << "`  \n";
				}

				auto child = urlWithoutAnchor.getChildUrlWithRoot(info.shortName);

				s << child.toString(MarkdownLink::ContentWithoutHeader) << "\n";
			}
		}

		return s;
	}

	return {};
}

juce::File MenuReferenceDocGenerator::MenuGenerator::getFileToEdit(const MarkdownLink& url)
{
	if (!url.isChildOf(rootURL.getChildUrl("menu-reference")))
		return {};

	if (url.toString(MarkdownLink::AnchorWithHashtag).isEmpty())
		return {};

	auto folderUrl = url.getParentUrl();

	auto f = url.getDirectory({});

	if (f.isDirectory())
	{
		auto e = f.getChildFile(url.toString(MarkdownLink::AnchorWithoutHashtag) + ".md");

		if (!e.existsAsFile() && PresetHandler::showYesNoWindow("Create file for menu description", "Do you want to create the file " + e.getFullPathName()))
		{
			e.create();
		}

		return e;
	}

	return {};
}

juce::String MenuReferenceDocGenerator::SettingsGenerator::getContent(const MarkdownLink& url)
{
	if (url.isChildOf(rootURL.getChildUrl("settings")))
	{
		auto name = url.toString(MarkdownLink::UrlSubPath);

		String s;
		String nl = "\n";

		s << url.toString(MarkdownLink::ContentFull) << nl;

		if (name == "project")
		{
			addDescriptionForIdentifiers(s, HiseSettings::Project::getAllIds(), "Project");
			addDescriptionForIdentifiers(s, HiseSettings::User::getAllIds(), "User");
		}
		if (name == "development")
		{
			addDescriptionForIdentifiers(s, HiseSettings::Compiler::getAllIds(), "Compiler");
			addDescriptionForIdentifiers(s, HiseSettings::Scripting::getAllIds(), "Scripting");
			addDescriptionForIdentifiers(s, HiseSettings::Other::getAllIds(), "Other");
			addDescriptionForIdentifiers(s, HiseSettings::Documentation::getAllIds(), "Documentation");
		}
		if (name == "audio-midi")
		{
			addDescriptionForIdentifiers(s, HiseSettings::Audio::getAllIds(), "Audio");
			addDescriptionForIdentifiers(s, HiseSettings::Midi::getAllIds(), "MIDI");
		}
		
		return s;
	}

	return {};
}


void MenuReferenceDocGenerator::SettingsGenerator::addDescriptionForIdentifiers(String& s, const Array<Identifier>& ids, const String& name)
{
	String nl = "\n";

	s << "## " << name << nl;

	for (auto id : ids)
	{
		auto d = HiseSettings::SettingDescription::getDescription(id);

		s << d << nl;
	}
}

}