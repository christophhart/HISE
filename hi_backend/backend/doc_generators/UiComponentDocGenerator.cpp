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

UIComponentDatabase::CommonData::Data::Data() {};


void UIComponentDatabase::CommonData::Data::init(BackendProcessor* bp_)
{
	if (bp != nullptr)
		return;

	bp = bp_;
	root = bp->getDocWindow();
	jmp = new JavascriptMidiProcessor(bp, "script");

	auto c = jmp->getContent();

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
	item.url = { rootDirectory, uiComponentWildcard };
	item.fillMetadataFromURL();

	MarkdownDataBase::Item pluginItem = item.createChildItem("plugin-components");
	
	pluginItem.fillMetadataFromURL();

	if (pluginItem.url.fileExists(rootDirectory))
	{
		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, pluginItem, pluginItem.url.toFile(MarkdownLink::FileType::ContentFile, rootDirectory), pluginItem.c);
	}

	for (auto c : d->list)
	{
		MarkdownDataBase::Item cItem;
		cItem.url = pluginItem.url.getChildUrlWithRoot(MarkdownLink::Helpers::getSanitizedFilename(c->getName().toString()), false);
		cItem.fillMetadataFromURL();
		
		pluginItem.addChild(std::move(cItem));
	}

	item.addChild(std::move(pluginItem));

	MarkdownDataBase::Item floatingTileItem = item.createChildItem("floating-tiles");
	floatingTileItem.tocString = "Floating Tiles";

	createFloatingTileApi(floatingTileItem);
	
	item.addChild(std::move(floatingTileItem));

	applyColour(item);

	return item;
}


void UIComponentDatabase::ItemGenerator::createFloatingTileApi(MarkdownDataBase::Item& item)
{
	Component::SafePointer<BackendRootWindow> root;

	{
		MessageManagerLock mm;

		root = dynamic_cast<BackendProcessor*>(&holder)->getDocWindow();
	}

	FloatingTileContent::Factory f3;
	f3.registerLayoutPanelTypes();

	auto layoutList = f3.getIdList();

	MarkdownDataBase::Item layout = item.createChildItem("layout");
	layout.tocString = "Layout Floating Tiles";
	layout.keywords.add("Layout");

	for (auto id : layoutList)
	{
		layout.addChild(createItemForFloatingTile(layout, f3, id, root->getRootFloatingTile()));
	}

	item.addChild(std::move(layout));

	FloatingTileContent::Factory f;

	f.registerFrontendPanelTypes();

	auto frontendList = f.getIdList();

	

	MarkdownDataBase::Item frontend;
	frontend.url = item.url.getChildUrl("plugin");
	frontend.url.setType(MarkdownLink::Folder);
	frontend.tocString = "Plugin Floating Tiles";

	for (auto id : frontendList)
	{
		frontend.addChild(createItemForFloatingTile(frontend, f, id, root->getRootFloatingTile()));
	}

	FloatingTileContent::Factory f2;

	f2.registerAllPanelTypes();

	auto allList = f2.getIdList();

	MarkdownDataBase::Item backend;
	backend.url = item.url.getChildUrl("hise");
	backend.tocString = "HISE Floating tiles";

	for (auto id : allList)
	{
		if (frontendList.contains(id))
			continue;

		if (layoutList.contains(id))
			continue;

		backend.addChild(createItemForFloatingTile(backend, f2, id, root->getRootFloatingTile()));
	}

	item.addChild(std::move(frontend));
	item.addChild(std::move(backend));

	{
		MessageManagerLock mm;
		root = nullptr;
	};
}

hise::MarkdownDataBase::Item UIComponentDatabase::ItemGenerator::createItemForFloatingTile(MarkdownDataBase::Item& parent, FloatingTileContent::Factory& f, Identifier& id, FloatingTile* tile)
{
	MarkdownDataBase::Item i;

	i.url = parent.url.getChildUrl(id.toString());
	i.url.setType(MarkdownLink::MarkdownFile);

	ScopedPointer<FloatingTileContent> ft;
	
	{
		MessageManagerLock mm;
		ft = f.createFromId(id, tile);
	}

	i.keywords.add(id.toString());
	i.tocString = id.toString();
	i.icon = "/images/icon_" + MarkdownLink::Helpers::getSanitizedFilename(id.toString());

	{
		MessageManagerLock mm;
		ft = nullptr;
	}

	return i;
}


UIComponentDatabase::Resolver::Resolver(File root_, BackendProcessor* bp) :
	root(root_)
{
	d->init(bp);
}

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
		ScriptContentComponent content(d->jmp);
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
	d->init(dynamic_cast<BackendProcessor*>(&holder));
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

	s << url.toString(MarkdownLink::ContentWithoutHeader);
	
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

	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Introduction"), Colour(0xFFe1c81c)));
	
	


	holder.addItemGenerator(new MenuReferenceDocGenerator::ItemGenerator(markdownRoot, holder));
	//holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Project Management"), Colours::grey));
	holder.addItemGenerator(new ScriptingApiDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new HiseModuleDatabase::ItemGenerator(markdownRoot, *dynamic_cast<BackendProcessor*>(&holder)));
	holder.addItemGenerator(new UIComponentDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Glossary"), Colour(0xFF84305a)));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Tutorials"), Colour(0xFF383f69)));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("cpp_api"), Colours::pink));
}


void AutogeneratedDocHelpers::registerContentProcessor(MarkdownContentProcessor* c)
{
	if (c->getHolder().shouldUseCachedData())
	{
		auto markdownRoot = c->getHolder().getCachedDocFolder();

		c->addLinkResolver(new DatabaseCrawler::Resolver(markdownRoot));
		if (c->getHolder().shouldAbort()) return;

		c->addImageProvider(new DatabaseCrawler::Provider(markdownRoot, nullptr));
		if (c->getHolder().shouldAbort()) return;

		registerGlobalPathFactory(c, {});
	}
	else
	{
		auto markdownRoot = c->getHolder().getDatabaseRootDirectory();
		auto bp = dynamic_cast<BackendProcessor*>(&c->getHolder());

		c->addLinkResolver(new MarkdownParser::FileLinkResolver(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new MarkdownParser::FolderTocCreator(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new MarkdownParser::FileBasedImageProvider(nullptr, markdownRoot));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new UIComponentDatabase::Resolver(markdownRoot, bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new HiseModuleDatabase::Resolver(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new ScriptingApiDatabase::Resolver(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new UIComponentDatabase::ScreenshotProvider(nullptr));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new UIComponentDatabase::FloatingTileScreenshotProvider(nullptr));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new HiseModuleDatabase::ScreenshotProvider(nullptr, bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new MenuReferenceDocGenerator::MenuGenerator(bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new MenuReferenceDocGenerator::SettingsGenerator(*bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new UIComponentDatabase::FloatingTileResolver(*bp));
		if (bp->shouldAbort()) return;

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


void AutogeneratedDocHelpers::createDocFloatingTile(FloatingTile* ft)
{
	FloatingInterfaceBuilder ib(ft);

	ib.setNewContentType<VerticalTile>(0);

	auto m = ib.addChild<VisibilityToggleBar>(0);

	ib.getContent(m)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF444444));

	auto t = ib.addChild<FloatingTabComponent>(0);

	auto e1 = ib.addChild<MarkdownEditorPanel>(t);
	auto p = ib.addChild<MarkdownPreviewPanel>(0);

	ib.setCustomName(e1, "Editor");

	ib.setCustomName(p, "Preview");
	ib.setDynamic(0, false);
	ib.setSizes(0, { 32.0, -0.5, -0.5 });
	ib.setFoldable(0, false, { false, true, true });

	ib.setVisibility(0, true, { false, false, true });

	ib.finalizeAndReturnRoot();

	auto panel = dynamic_cast<MarkdownPreviewPanel*>(ib.getContent(p));
	auto editorPanel = dynamic_cast<MarkdownEditorPanel*>(ib.getContent(e1));

	auto preview = &panel->preview;
	jassert(preview != nullptr);

	editorPanel->setPreview(preview);

	preview->renderer.gotoLink({ {}, "/" });

	preview->getHolder().rebuildDatabase();
}


juce::Image UIComponentDatabase::FloatingTileScreenshotProvider::getImage(const MarkdownLink& url, float width)
{
	if (url.toString(MarkdownLink::UrlFull).startsWith(SnapshotMarkdownCodeComponent::floatingTile_screenshotWildcard))
	{
		MemoryBlock mb;

		auto mainController = dynamic_cast<MainController*>(parent->getHolder());
		auto parentComponent = parent->getHolder()->getRootComponent();

		auto data = JSON::parse(url.getPostData());

		ScopedPointer<Processor> floatingTileProcessor;
		ScopedPointer<FloatingTile> floatingTile = new FloatingTile(mainController, nullptr, {});

		parentComponent->addAndMakeVisible(floatingTile);

		floatingTile->setOpaque(true);
		floatingTile->setContent(data);

		if (auto pc = dynamic_cast<PanelWithProcessorConnection*>(floatingTile->getCurrentFloatingPanel()))
		{
			floatingTileProcessor = pc->createDummyProcessorForDocumentation(mainController);
			pc->setContentWithUndo(floatingTileProcessor, 0);
			floatingTile->getCurrentFloatingPanel()->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF363636));
		}

		int height = floatingTile->getCurrentFloatingPanel()->getPreferredHeight();

		if (height == 0)
			height = 400;

		floatingTile->setSize(800, height);
		return floatingTile->createComponentSnapshot(floatingTile->getLocalBounds());
	}
	
	return {};
}

}