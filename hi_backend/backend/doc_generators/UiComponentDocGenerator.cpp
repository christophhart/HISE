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

	list.add(c->addViewport("Viewport", 0, 0));

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

hise::MarkdownDataBase::Item UIComponentDatabase::ItemGenerator::createRootItem(MarkdownDataBase& )
{
	MarkdownDataBase::Item item;

	item.c = Colour(0xFF9064FF);
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

	item.setDefaultColour(colour);

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
	backend.url.setType(MarkdownLink::Folder);
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
		//MessageManagerLock mm;
		//ft = f.createFromId(id, tile);
	}

	i.keywords.add(id.toString());
	i.tocString = id.toString();
	i.icon = "/images/icon_" + MarkdownLink::Helpers::getSanitizedFilename(id.toString());

	{
		//MessageManagerLock mm;
		//ft = nullptr;
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

		s << url.toString(MarkdownLink::ContentHeader) << nl;

		auto header = url.getHeaderFromFile(root);

		s << "> " << header.getDescription() << nl;

		s << "![](/images/ui-controls/" << MarkdownLink::Helpers::getSanitizedFilename(c->getName().toString()) << ".png)" << nl;

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

juce::Image UIComponentDatabase::ScreenshotProvider::getImage(const MarkdownLink& , float )
{
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

juce::String UIComponentDatabase::FloatingTileResolver::getCategoryContent(const String& )
{
	return {};
}

juce::String UIComponentDatabase::FloatingTileResolver::getFloatingTileContent(const MarkdownLink& url, FloatingTileContent::Factory& f, const Identifier& id)
{
#if HISE_HEADLESS
	return {};
#endif

    ScopedPointer<FloatingTileContent> ft;
    
    {
		MainController::ScopedBadBabysitter sb(d->root->getBackendProcessor());
        ft = f.createFromId(id, d->root->getRootFloatingTile());
    }
    
	String s;
	String nl = "\n";

	auto header = url.getHeaderFromFile(root.getRoot());

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

		for (auto file : results)
		{
			if (file.getFileNameWithoutExtension() == MarkdownLink::Helpers::getSanitizedFilename(idToLookFor))
			{
				MarkdownLink l (url.getRoot(), file.getRelativePathFrom(url.getRoot()));
				s << l.toString(MarkdownLink::FormattedLinkMarkdown) << nl;
				found = true;
			}
		}

		if(!found)
			s << p->getProcessorTypeId() << nl;
	}
	
	s << "> " << header.getDescription() << nl;

	s << "![screenshot](/images/floating-tiles/" << id.toString().toLowerCase() << ".png)\n";

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
		auto propertyid = ft->getDefaultablePropertyId(i);

		if (commonProperties.contains(propertyid))
			continue;

		auto value = ft->getDefaultProperty(i);

		s << "| `" << ft->getDefaultablePropertyId(i) << "` | ";
		s << MarkdownLink::Helpers::getPrettyVarString(value) << " | ";

		String description = "no description";

		for (auto desc : propertyDescriptions)
		{
			String prop = desc.upToFirstOccurrenceOf(":", false, false).trim();

			if (prop == ft->getDefaultablePropertyId(i).toString())
			{
				description = desc.fromFirstOccurrenceOf(":", false, false).trim();
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
	auto f = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("CachedDocumentation/");
	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

void AutogeneratedDocHelpers::addItemGenerators(MarkdownDatabaseHolder& holder)
{
	if (holder.shouldUseCachedData())
		return;

	auto markdownRoot = holder.getDatabaseRootDirectory();

	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Introduction"), Colour(0xFFA4CC3E)));
	
	
	holder.addItemGenerator(new MenuReferenceDocGenerator::ItemGenerator(markdownRoot, holder));
	//holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Project Management"), Colours::grey));
	holder.addItemGenerator(new ScriptingApiDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new HiseModuleDatabase::ItemGenerator(markdownRoot, *dynamic_cast<BackendProcessor*>(&holder)));
	holder.addItemGenerator(new scriptnode::doc::ItemGenerator(markdownRoot, *dynamic_cast<BackendProcessor*>(&holder)));
	holder.addItemGenerator(new UIComponentDatabase::ItemGenerator(markdownRoot, holder));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Glossary"), Colour(0xFFBD6F50)));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("Tutorials"), Colour(0xFFC5AC43)));
	holder.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(markdownRoot.getChildFile("cpp_api"), Colour(0xFFCCCCCC)));
}


void AutogeneratedDocHelpers::registerContentProcessor(MarkdownContentProcessor* c)
{
	c->addLinkResolver(new MarkdownParser::DefaultLinkResolver(nullptr));

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

		c->addLinkResolver(new scriptnode::doc::Resolver(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new ScriptingApiDatabase::Resolver(markdownRoot));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new HiseModuleDatabase::ScreenshotProvider(nullptr, bp));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new scriptnode::doc::ScreenshotProvider(nullptr));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new MenuReferenceDocGenerator::MenuGenerator(bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new MenuReferenceDocGenerator::SettingsGenerator(*bp));
		if (bp->shouldAbort()) return;

		c->addLinkResolver(new UIComponentDatabase::FloatingTileResolver(*bp));
		if (bp->shouldAbort()) return;

		c->addImageProvider(new MarkdownParser::URLImageProvider(markdownRoot.getChildFile("images/web/"), nullptr));
		

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
	REGISTER_PATH_FACTORY(MidiPlayerBaseType::TransportPaths);
	REGISTER_PATH_FACTORY(SampleMapEditor::Factory);
	REGISTER_PATH_FACTORY(ScriptComponentEditPanel::Factory);
	REGISTER_PATH_FACTORY(ScriptContentPanel::Factory);
	REGISTER_PATH_FACTORY(MarkdownPreview::Topbar::TopbarPaths);
	REGISTER_PATH_FACTORY(WaveformComponent::WaveformFactory);
	REGISTER_PATH_FACTORY(FloatingTileContent::FloatingTilePathFactory);
	REGISTER_PATH_FACTORY(MainToolbarFactory);
	REGISTER_PATH_FACTORY(scriptnode::NodeComponentFactory);

#undef REGISTER_PATH_FACTORY

	c->addImageProvider(p);


	if(markdownRoot.isDirectory())
		c->addLinkResolver(r.release());
}


void AutogeneratedDocHelpers::createDocFloatingTile(FloatingTile* ft, const MarkdownLink& linkToShow)
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
	panel->initPanel();
	panel->getParentShell()->getLayoutData().setId("Preview");
	auto editorPanel = dynamic_cast<MarkdownEditorPanel*>(ib.getContent(e1));

	auto preview = panel->preview.get();
	jassert(preview != nullptr);

	editorPanel->setPreview(preview);

	if (linkToShow.isValid())
		preview->renderer.gotoLink(linkToShow);
	else
		preview->renderer.gotoLink({ {}, "/" });

    preview->getHolder().rebuildDatabase();
    panel->getParentShell()->refreshRootLayout();
}


}
