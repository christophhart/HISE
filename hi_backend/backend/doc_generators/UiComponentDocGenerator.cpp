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

UIComponentDatabase::CommonData::Data::Data() :
	bp(),
	jmp(&bp, "script")
{
	bp.setAllowFlakyThreading(true);

	auto c = jmp.getContent();

	list.add(c->addButton("Button", 0, 0));
	list.add(c->addKnob("Knob", 0, 0));
	list.add(c->addComboBox("Combobox", 0, 0));
	list.add(c->addFloatingTile("Floating Tile", 0, 0));
	list.add(c->addImage("Image", 0, 0));
	list.add(c->addLabel("Label", 0, 0));
	list.add(c->addPanel("Panel", 0, 0));
	list.add(c->addAudioWaveform("Audio Waveform", 0, 0));

	list.getLast()->setScriptObjectProperty(ScriptComponent::width, 512);
	list.getLast()->setScriptObjectProperty(ScriptComponent::height, 100);

	list.add(c->addTable("Table", 0, 0));

	list.getLast()->setScriptObjectProperty(ScriptComponent::width, 512);
	list.getLast()->setScriptObjectProperty(ScriptComponent::height, 100);

	list.add(c->addSliderPack("SliderPack", 0, 0));

	list.getLast()->setScriptObjectProperty(ScriptComponent::width, 512);
	list.getLast()->setScriptObjectProperty(ScriptComponent::height, 100);

	list.add(c->addScriptedViewport("Viewport", 0, 0));

	list.getLast()->setScriptObjectProperty(ScriptComponent::width, 512);
	list.getLast()->setScriptObjectProperty(ScriptComponent::height, 100);
}

hise::ScriptComponent* UIComponentDatabase::CommonData::getComponentForURL(const MarkdownLink& url)
{
	auto id = getComponentIDFromURL(url);

	if (id.isNotEmpty())
	{
		for (auto c : d->list)
		{
			auto id_ = MarkdownLink::Helpers::getSanitizedFilename(c->getName().toString());

			if (id == id_)
				return c;
		}
	}

	return nullptr;
}

juce::String UIComponentDatabase::CommonData::getComponentIDFromURL(const MarkdownLink& url)
{
	return url.toString(MarkdownLink::UrlSubPath);

#if 0
	if (idPrefix.isEmpty())
		idPrefix = "/";

	if (!forceModuleWildcard || url.startsWith(uiComponentWildcard))
	{
		return url.fromLastOccurrenceOf(idPrefix, false, false).upToLastOccurrenceOf(".", false, false);
	}

	return {};
#endif
}

hise::MarkdownDataBase::Item UIComponentDatabase::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	MarkdownDataBase::Item item;

	item.c = Colour(0xFF124958F);
	item.tocString = "UI Components";
	item.type = MarkdownDataBase::Item::Folder;
	item.url = { rootDirectory, uiComponentWildcard };

	MarkdownDataBase::Item pluginItem = item.createChildItem("plugin-components");
	pluginItem.type = MarkdownDataBase::Item::Folder;

	if (pluginItem.url.fileExists(rootDirectory))
	{
		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, pluginItem, pluginItem.url.toFile(MarkdownLink::FileType::ContentFile, rootDirectory), pluginItem.c);
	}

	for (auto c : d->list)
	{
		MarkdownDataBase::Item cItem;
		cItem.type = MarkdownDataBase::Item::Keyword;
		cItem.url = pluginItem.url.getChildUrlWithRoot(MarkdownLink::Helpers::getSanitizedFilename(c->getName().toString()), false);
		cItem.tocString << c->getName();
		cItem.description = "description";
		cItem.c = Colour(0xFF124958F);

		if (cItem.url.fileExists(rootDirectory))
		{
			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, cItem, cItem.url.toFile(MarkdownLink::FileType::ContentFile, rootDirectory), cItem.c);
		}

		pluginItem.children.add(cItem);
	}

	item.children.add(std::move(pluginItem));

	MarkdownDataBase::Item floatingTileItem = item.createChildItem("floating-tiles");
	floatingTileItem.tocString = "Floating Tiles";

	createFloatingTileApi(floatingTileItem);
	
	item.children.add(floatingTileItem);

	applyColour(item);

	return item;
}


void UIComponentDatabase::ItemGenerator::createFloatingTileApi(MarkdownDataBase::Item& item)
{
	ScopedPointer<BackendRootWindow> root;

	{
		MessageManagerLock mm;

		root = new BackendRootWindow(dynamic_cast<BackendProcessor*>(&holder), {});
	}

	FloatingTileContent::Factory f3;
	f3.registerLayoutPanelTypes();

	auto layoutList = f3.getIdList();

	MarkdownDataBase::Item layout = item.createChildItem("layout");
	layout.tocString = "Layout Floating Tiles";
	layout.type = MarkdownDataBase::Item::Folder;
	layout.keywords.add("Layout");

	for (auto id : layoutList)
	{
		layout.children.add(createItemForFloatingTile(layout, f3, id, root.getRootFloatingTile()));
	}

	item.children.add(std::move(layout));

	FloatingTileContent::Factory f;

	f.registerFrontendPanelTypes();

	auto frontendList = f.getIdList();

	

	MarkdownDataBase::Item frontend;
	frontend.url = item.url.getChildUrl("plugin");
	frontend.type = MarkdownDataBase::Item::Folder;
	frontend.tocString = "Plugin Floating Tiles";

	for (auto id : frontendList)
	{
		frontend.children.add(createItemForFloatingTile(frontend, f, id, root.getRootFloatingTile()));
	}

	FloatingTileContent::Factory f2;

	f2.registerAllPanelTypes();

	auto allList = f2.getIdList();

	MarkdownDataBase::Item backend;
	backend.url = item.url.getChildUrl("hise");
	backend.type = MarkdownDataBase::Item::Folder;
	backend.tocString = "HISE Floating tiles";

	for (auto id : allList)
	{
		if (frontendList.contains(id))
			continue;

		if (layoutList.contains(id))
			continue;

		backend.children.add(createItemForFloatingTile(backend, f2, id, root.getRootFloatingTile()));
	}

	item.children.add(std::move(frontend));
	item.children.add(std::move(backend));

}

hise::MarkdownDataBase::Item UIComponentDatabase::ItemGenerator::createItemForFloatingTile(MarkdownDataBase::Item& parent, FloatingTileContent::Factory& f, Identifier& id, FloatingTile* tile)
{
	MarkdownDataBase::Item i;

	i.url = parent.url.getChildUrl(id.toString());
	i.type = MarkdownDataBase::Item::Keyword;

	ScopedPointer<FloatingTileContent> ft = f.createFromId(id, tile);

	DBG(JSON::toString(ft->toDynamicObject()));

	i.keywords.add(id.toString());
	i.tocString = id.toString();
	i.icon = "/images/icon_" + MarkdownLink::Helpers::getSanitizedFilename(id.toString());

	return i;
}


UIComponentDatabase::Resolver::Resolver(File root_) :
	root(root_)
{}

juce::String UIComponentDatabase::Resolver::getContent(const MarkdownLink& url)
{
	if (auto c = getComponentForURL(url))
	{
		String s;
		NewLine nl;

		
#if 0
		auto f = MarkdownLink::Helpers::getLocalFileForSanitizedURL(root, MarkdownLink::Helpers::removeAnchor(url), File::findFiles, "*.md");

		String filecontent;

		MarkdownHeader header;

		if (f.existsAsFile())
		{
			MarkdownParser p(f.loadFileAsString());
			p.parse();
			header = p.getHeader();
			filecontent = p.getCurrentText(false);
		}
#endif

		s << url.toString(MarkdownLink::ContentHeader) << nl;

		auto header = url.getHeaderFromFile(root, false);

		s << "> " << header.getDescription() << nl;

		s << "## Live demo" << nl;

		s << "Select one of the properties and enter a value to see how the UI element changes.  " << nl;
		s << nl;
		s << "```scriptcontent" << nl;

		s << getInitialisationForComponent(c->getObjectName().toString());

		
		s << R"(const var valueLabel = Content.addLabel("valueLabel", 0, 50);)" << nl;
		s << R"(valueLabel.set("text", "Select a property");)" << nl;
		s << R"(valueLabel.set("saveInPreset", false);)" << nl;
		s << R"(valueLabel.set("textColour", 0xFFFFFFFF);)" << nl;
		s << nl;
		s << R"(valueLabel.set("bgColour", 0xFF222222);)" << nl;
		s << nl;
		s << R"(const var selector = Content.addComboBox("selector", 0, 0);)" << nl;
		s << R"(selector.set("saveInPreset", false);)" << nl;
		s << nl;
		s << R"(const var list = ui.getAllProperties();)" << nl;
		s << R"(list.remove("id");)" << nl;
		s << R"(list.remove("type");)" << nl;
		s << nl;
		s << R"(selector.set("items", list.join("\n"));)" << nl;
		s << nl;
		s << R"(inline function selectorCallback(component, value))" << nl;
		s << R"({)" << nl;
		s << R"(    valueLabel.set("text", ui.get(component.getItemText()));)" << nl;
		s << R"(})" << nl;
		s << nl;
		s << R"(selector.setControlCallback(selectorCallback);)" << nl;
		s << nl;
		s << R"(inline function labelCallback(component, value))" << nl;
		s << R"({)" << nl;
		s << R"(    if (selector.getValue() > 2))" << nl;
		s << R"(        ui.set(selector.getItemText(), value);)" << nl;
		s << R"(})" << nl;
		s << nl;
		s << R"(valueLabel.setControlCallback(labelCallback);)" << nl;
		s << R"(```)" << nl;

		//s << "![](/images/ui_screenshot_" << MarkdownLink::Helpers::getSanitizedFilename(c->getName().toString()) << ".png)" << nl;

		s << "## Special Properties" << nl;

		s << "| Property ID | Default Value | Description |" << nl;
		s << "| --- | -- | -------- |" << nl;

		auto idDescriptions = header.getKeyList("properties");

		for (int i = ScriptComponent::Properties::numProperties; i < c->getNumIds(); i++)
		{
			var value = c->getScriptObjectProperty(i);

			auto valueString = MarkdownLink::Helpers::getPrettyVarString(value);

			s << "| `" << c->getIdFor(i).toString() << "` | " << valueString << " |";
			
			String description;

			for (auto prop : idDescriptions)
			{
				auto thisId = c->getIdFor(i).toString();

				if (prop.startsWith(thisId))
				{
					description = prop.fromFirstOccurrenceOf(":", false, false);
					break;
				}
			}

			if (description.isEmpty())
				description = "No description.";
			
			s << description << " |";

			s << nl;
		}

		s << url.toString(MarkdownLink::ContentWithoutHeader);

		return s;
	}

	return {};
}

juce::String UIComponentDatabase::Resolver::getInitialisationForComponent(const String& name)
{
	String s;
	String nl = "\n";

#define IS(x) name == x::getStaticObjectName().toString()

	if(IS(ScriptingApi::Content::ScriptButton))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addButton("id", 200, 0);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptSlider))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addKnob("id", 200, 0);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptComboBox))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addComboBox("id", 200, 0);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptPanel))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addPanel("id", 200, 0);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptLabel))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addLabel("id", 200, 0);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptAudioWaveform))
	{
		s << "Content.setHeight(150);" << nl;
		s << R"(const var ui = Content.addAudioWaveform("id", 200, 0);)" << nl;
		s << R"(const var f = Synth.addEffect("Convolution", "Convolution Reverb", 3);)" << nl;
		s << R"(ui.set("processorId", "Convolution Reverb");)" << nl;
		s << R"(ui.set("height", 150);)" << nl;
		s << R"(ui.set("width", 512);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptTable))
	{
		s << "Content.setHeight(150);" << nl;
		s << R"(const var ui = Content.addTable("id", 200, 0);)" << nl;
		s << R"(ui.set("height", 150);)" << nl;
		s << R"(ui.set("width", 512);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptSliderPack))
	{
		s << "Content.setHeight(150);" << nl;
		s << R"(const var ui = Content.addSliderPack("id", 200, 0);)" << nl;
		s << R"(ui.set("height", 150);)" << nl;
		s << R"(ui.set("saveInPreset", false);)" << nl;
		s << R"(for(i = 0; i < 16; i++) ui.setSliderAtIndex(i, i/16); )" << nl;
		s << R"(ui.set("width", 512);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptFloatingTile))
	{
		s << "Content.setHeight(300);" << nl;
		s << R"(const var ui = Content.addFloatingTile("id", 200, 0);)" << nl;
		s << R"(ui.set("height", 300);)" << nl;
		s << R"(ui.set("width", 512);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptedViewport))
	{
		s << "Content.setHeight(300);" << nl;
		s << R"(const var ui = Content.addViewport("id", 200, 0);)" << nl;
		s << R"(ui.set("height", 300);)" << nl;
		s << R"(ui.set("width", 512);)" << nl;
	}
	if (IS(ScriptingApi::Content::ScriptImage))
	{
		s << "Content.setHeight(100);" << nl;
		s << R"(const var ui = Content.addImage("id", 200, 0);)" << nl;
		s << R"(ui.set("height", 100);)" << nl;
		s << R"(ui.set("width", 256);)" << nl;
	}

#undef IS

	return s;


}

UIComponentDatabase::ScreenshotProvider::ScreenshotProvider(MarkdownParser* parent) :
	ImageProvider(parent)
{}

juce::Image UIComponentDatabase::ScreenshotProvider::getImage(const MarkdownLink& url, float width)
{
	if (auto c = getComponentForURL(url))
	{
		auto sd = parent->getStyleData();

		c->setScriptObjectProperty(ScriptComponent::bgColour, sd.backgroundColour.withMultipliedBrightness(0.7f).getARGB());
		c->setScriptObjectProperty(ScriptComponent::textColour, sd.textColour.getARGB());
		ScriptContentComponent content(&d->jmp);
		auto cmp = content.getComponentFor(c);
		Rectangle<int> bounds = c->getPosition();
		return cmp->createComponentSnapshot(bounds);
	}

	return {};
}


UIComponentDatabase::FloatingTileResolver::FloatingTileResolver(MarkdownDatabaseHolder& holder_):
	LinkResolver(),
	holder(holder_),
	root(holder.getDatabaseRootDirectory(), floatingTileWildcard)
{
	d->createRootWindow();
}

juce::String UIComponentDatabase::FloatingTileResolver::getContent(const MarkdownLink& url)
{
	if (url.isChildOf(root))
	{
		if (url == root)
			return "# Root\n";

		if (url.getParentUrl() == root)
		{
			return url.toString(MarkdownLink::ContentFull, holder.getDatabaseRootDirectory());
		}
		else
		{
			auto idToSearch = url.toString(MarkdownLink::UrlSubPath);

			FloatingTileContent::Factory f;

			f.registerAllPanelTypes();

			auto l = f.getIdList();

			for (auto id : l)
			{
				if (id.toString().toLowerCase() == idToSearch)
				{
					return getFloatingTileContent(url, f, id);
				}
			}
		}
		
	}
	
	return {};
}

juce::String UIComponentDatabase::FloatingTileResolver::getCategoryContent(const String& categoryName)
{
	return {};
}

juce::String UIComponentDatabase::FloatingTileResolver::getFloatingTileContent(const MarkdownLink& url, FloatingTileContent::Factory& f, const Identifier& id)
{
#if HISE_HEADLESS
	return {};
#endif

	ScopedPointer<FloatingTileContent> ft = f.createFromId(id, d->root->getRootFloatingTile());

	

	String s;
	String nl = "\n";

	auto header = url.getHeaderFromFile(root.getRoot(), false);

	auto propertyDescriptions = header.getKeyList("properties");

	s << url.toString(MarkdownLink::ContentHeader) << "\n";
	s << url.toString(MarkdownLink::ContentWithoutHeader).upToFirstOccurrenceOf("\n", true, false) << nl;

	if (auto p = dynamic_cast<PanelWithProcessorConnection*>(ft.get()))
	{
		s << "Connects to module type: ";
		
		MarkdownLink moduleRoot(url.getRoot(), HiseModuleDatabase::moduleWildcard);
		
		auto moduleDirectory = moduleRoot.toFile(MarkdownLink::FileType::Directory);

		Array<File> results;

		moduleDirectory.findChildFiles(results, File::findFiles, true);

		auto idToLookFor = p->getProcessorTypeId().toString();

		bool found = false;

		for (auto f : results)
		{
			if (f.getFileNameWithoutExtension() == MarkdownLink::Helpers::getSanitizedFilename(idToLookFor))
			{
				MarkdownLink l (url.getRoot(), f.getRelativePathFrom(url.getRoot()));

				

				s << l.toString(MarkdownLink::FormattedLinkMarkdown) << nl;
				found = true;
			}
		}

		if(!found)
			s << p->getProcessorTypeId() << nl;
	}
	
	s << "> " << header.getDescription() << nl;

	s << "```floating-tile" << nl;
	s << JSON::toString(ft->toDynamicObject()) << nl;
	s << "```" << nl;

	s << "| ID | Default Value | Description |" << nl;
	s << "| --- | -- | ------------ |" << nl;
	
	Array<Identifier> commonProperties;
	commonProperties.add("Type");
	commonProperties.add("Title");
	commonProperties.add("StyleData");
	commonProperties.add("ColourData");
	commonProperties.add("LayoutData");


	for (int i = 0; i < ft->getNumDefaultableProperties(); i++)
	{
		auto id = ft->getDefaultablePropertyId(i);

		if (commonProperties.contains(id))
			continue;

		auto value = ft->getDefaultProperty(i);

		s << "| `" << ft->getDefaultablePropertyId(i) << "` | ";
		s << MarkdownLink::Helpers::getPrettyVarString(value) << " | ";

		String description = "no description";

		for (auto d : propertyDescriptions)
		{
			String prop = d.upToFirstOccurrenceOf(":", false, false).trim();

			if (prop == ft->getDefaultablePropertyId(i).toString())
			{
				description = d.fromFirstOccurrenceOf(":", false, false).trim();
				break;
			}
		}

		s << description << " |" <<  nl;
	}

	return s;
}

juce::File AutogeneratedDocHelpers::getCachedDocFolder()
{
	auto f = ProjectHandler::getAppDataDirectory().getChildFile("CachedDocumentation/");
	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

void AutogeneratedDocHelpers::addItemGenerators(MarkdownDatabaseHolder& holder)
{
	if (holder.shouldUseCachedData())
		return;

	auto markdownRoot = holder.getDatabaseRootDirectory();

	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Introduction"), Colours::orange));
	holder.addItemGenerator(new MenuReferenceDocGenerator::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Project Management"), Colours::grey));
	holder.addItemGenerator(new ScriptingApiDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new HiseModuleDatabase::ItemGenerator(markdownRoot));
	holder.addItemGenerator(new UIComponentDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Glossary"), Colours::honeydew));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Tutorials"), Colours::chocolate));
}


void AutogeneratedDocHelpers::registerContentProcessor(MarkdownContentProcessor* c)
{
	if (c->getHolder().shouldUseCachedData())
	{
		auto markdownRoot = c->getHolder().getCachedDocFolder();

		c->addLinkResolver(new DatabaseCrawler::Resolver(markdownRoot));
		c->addImageProvider(new DatabaseCrawler::Provider(markdownRoot, nullptr));

		registerGlobalPathFactory(c, {});
	}
	else
	{
		auto markdownRoot = c->getHolder().getDatabaseRootDirectory();

		c->addLinkResolver(new MarkdownParser::FileLinkResolver(markdownRoot));
		c->addLinkResolver(new MarkdownParser::FolderTocCreator(markdownRoot));
		c->addImageProvider(new MarkdownParser::FileBasedImageProvider(nullptr, markdownRoot));

		c->addLinkResolver(new UIComponentDatabase::Resolver(markdownRoot));
		c->addLinkResolver(new HiseModuleDatabase::Resolver(markdownRoot));
		c->addLinkResolver(new ScriptingApiDatabase::Resolver(markdownRoot));

		c->addImageProvider(new UIComponentDatabase::ScreenshotProvider(nullptr));
		c->addImageProvider(new HiseModuleDatabase::ScreenshotProvider(nullptr));
		c->addLinkResolver(new MenuReferenceDocGenerator::MenuGenerator(dynamic_cast<BackendProcessor*>(&c->getHolder())));
		c->addLinkResolver(new MenuReferenceDocGenerator::SettingsGenerator(*dynamic_cast<BackendProcessor*>(&c->getHolder())));
		c->addLinkResolver(new UIComponentDatabase::FloatingTileResolver(c->getHolder()));

		registerGlobalPathFactory(c, markdownRoot);
	}
}



void AutogeneratedDocHelpers::registerGlobalPathFactory(MarkdownContentProcessor* c, const File& markdownRoot)
{
	ScopedPointer<MenuReferenceDocGenerator::Resolver> r;

	r = new MenuReferenceDocGenerator::Resolver(markdownRoot);

	auto p = new MarkdownParser::GlobalPathProvider(nullptr);

#define REGISTER_PATH_FACTORY(T) r->registerFactories<T>(); p->registerFactory<T>();

	REGISTER_PATH_FACTORY(ChainBarPathFactory);
	REGISTER_PATH_FACTORY(MPEPanel::Factory);
	REGISTER_PATH_FACTORY(MidiPlayerEditor::TransportPaths);
	REGISTER_PATH_FACTORY(SampleMapEditor::Factory);
	REGISTER_PATH_FACTORY(ScriptComponentEditPanel::Factory);
	REGISTER_PATH_FACTORY(ScriptContentPanel::Factory);
	REGISTER_PATH_FACTORY(MarkdownPreview::Topbar::TopbarPaths);
	REGISTER_PATH_FACTORY(WaveformComponent::WaveformFactory);
	REGISTER_PATH_FACTORY(FloatingTileContent::FloatingTilePathFactory);
	REGISTER_PATH_FACTORY(MainToolbarFactory);

#undef REGISTER_PATH_FACTORY

	c->addImageProvider(p);


	if(markdownRoot.isDirectory())
		c->addLinkResolver(r.release());
}

}