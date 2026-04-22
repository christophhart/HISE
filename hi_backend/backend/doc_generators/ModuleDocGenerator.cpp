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

void HiseModuleDatabase::CommonData::Data::createAllProcessors()
{
	MainController::ScopedBadBabysitter sb(bp);

	jassert(bp != nullptr);

	if (!allProcessors.isEmpty())
		return;

	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(1, bp->getMainSynthChain());
	addFromFactory(f);

	f = new MidiProcessorFactoryType(bp->getMainSynthChain());
	addFromFactory(f);

	f = new ModulatorChainFactoryType(1, Modulation::GainMode, bp->getMainSynthChain());
	addFromFactory(f);

	f = new EffectProcessorChainFactoryType(1, bp->getMainSynthChain());
	addFromFactory(f);
}

void HiseModuleDatabase::CommonData::Data::addFromFactory(FactoryType* f)
{
	for (int i = 0; i < f->getNumProcessors(); i++)
	{
		MessageManagerLock mm;
        allProcessors.add(f->createProcessor(i, "id"));
	}
}

juce::String HiseModuleDatabase::CommonData::getProcessorIdFromURL(const MarkdownLink& url)
{
	auto urlFull = url.toString(MarkdownLink::Format::UrlFull);

	if (url.getType() != MarkdownLink::Image && !urlFull.startsWith(moduleWildcard))
		return "";

	return url.toString(MarkdownLink::UrlSubPath);
}

hise::Processor* HiseModuleDatabase::CommonData::getProcessorForURL(const MarkdownLink& url)
{
	auto id = getProcessorIdFromURL(url);

	if (id.isNotEmpty())
	{
		for (auto p : data->allProcessors)
		{
			auto sanitizedId = MarkdownLink::Helpers::getSanitizedFilename(p->getType().toString());

			if (sanitizedId == id)
				return p;
		}
	}

	return nullptr;
}

hise::MarkdownDataBase::Item::Ptr HiseModuleDatabase::ItemGenerator::createItemForProcessor(Processor* p, const MarkdownDataBase::Item* parent)
{
	auto newItem = MarkdownDataBase::Item::createNew();

	newItem->c = p->getColour();
	newItem->url = parent->url.getChildUrl(p->getType().toString()).withRoot(rootDirectory, true);
	newItem->url.setType(MarkdownLink::MarkdownFile);
	
	auto f = newItem->url.getMarkdownFile(rootDirectory);

	if (f.existsAsFile())
	{
		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, newItem.get(), f, newItem->c);
	}

	auto pItem = MarkdownDataBase::Item::createNew();

	pItem->tocString << "Parameters";
	pItem->url = newItem->url.getChildUrl("parameters", true);
	pItem->c = newItem->c;

	newItem->addChild(pItem);
	
	return newItem;
}

hise::MarkdownDataBase::Item::Ptr HiseModuleDatabase::ItemGenerator::createItemForFactory(FactoryType* owned, const String& factoryName, MarkdownDataBase::Item* parent)
{
	
	ScopedPointer<FactoryType> f = owned;

	auto n = f->getNumProcessors();

	auto list = MarkdownDataBase::Item::createNew();
	return list;
}

hise::MarkdownDataBase::Item::Ptr HiseModuleDatabase::ItemGenerator::createItemForCategory(const String& categoryName, const MarkdownDataBase::Item* parent)
{
	auto item = MarkdownDataBase::Item::createNew();

	item->tocString << categoryName;
	item->url = parent->url.getChildUrl(categoryName);
	item->url.setType(MarkdownLink::Folder);
	item->addTocChildren(rootDirectory);

	return item;
}

hise::MarkdownDataBase::Item::Ptr HiseModuleDatabase::ItemGenerator::createRootItem(MarkdownDataBase& )
{
	auto rItem = MarkdownDataBase::Item::createNew();

	rItem->tocString = "HISE Modules";
	rItem->url = { rootDirectory, moduleWildcard };
	
	rItem->fillMetadataFromURL();

	auto bp = data->bp;

	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(NUM_POLYPHONIC_VOICES, bp->getMainSynthChain());

	{
		MainController::ScopedBadBabysitter sb(bp);

		auto sg = createItemForCategory("Sound Generators", rItem.get());
		auto sg2 = createItemForFactory(new ModulatorSynthChainFactoryType(1, bp->getMainSynthChain()),
			"Sound Generators", sg.get());

		sg->addChild(sg2);

		rItem->addChild(sg);
	}
    
	auto mp = createItemForCategory("MIDI Processors", rItem.get());

	auto mp2 = createItemForFactory(new MidiProcessorFactoryType(bp->getMainSynthChain()), "MIDI Processors", mp.get());
	mp->addChild(mp2);

	rItem->addChild(mp);

	auto modItem = createItemForCategory("Modulators", rItem.get());

	
	auto vs = createItemForCategory("Voice Start Modulators", modItem.get());

	auto vs2 = createItemForFactory(new VoiceStartModulatorFactoryType(1, Modulation::GainMode, bp->getMainSynthChain()),
		"Voice Start Modulators", vs.get());

	vs->addChild(vs2);

	vs->isAlwaysOpen = false;

	modItem->addChild(vs);

	auto tv = createItemForCategory("Time Variant Modulators", modItem.get());

	auto tv2 = createItemForFactory(new TimeVariantModulatorFactoryType(Modulation::GainMode, bp->getMainSynthChain()),
		"Time Variant Modulators", tv.get());

	tv->addChild(std::move(tv2));
	tv->isAlwaysOpen = false;

	modItem->addChild(tv);
	

	auto em = createItemForCategory("Envelopes", modItem.get());
	auto em2 = createItemForFactory(new EnvelopeModulatorFactoryType(1, Modulation::GainMode, bp->getMainSynthChain()),
		"Envelopes", em.get());

	em->addChild(em2);
	em->isAlwaysOpen = false;

	modItem->addChild(em);

	rItem->addChild(modItem);

	auto fx = createItemForCategory("Effects", rItem.get());
	auto fx2 = createItemForFactory(new EffectProcessorChainFactoryType(1, bp->getMainSynthChain()), "Effects", fx.get());
	fx->addChild(fx2);
	rItem->addChild(fx);

	rItem->setDefaultColour(colour);

	return rItem;
}

HiseModuleDatabase::Resolver::Resolver(File root_) :
	LinkResolver(),
	root(root_),
	parameterDump(new DynamicObject())
{
	data->createAllProcessors();
}

juce::String HiseModuleDatabase::Resolver::getContent(const MarkdownLink& url)
{
	return {};
}

#if 0
struct HiseModuleDatabase::ScreenshotProvider::RootWindow : public Component,
	public ComponentWithBackendConnection
{
	RootWindow(MainController* mc) :
		root(mc, nullptr)
	{};

	BackendRootWindow * getBackendRootWindow() { return nullptr; }

	const BackendRootWindow* getBackendRootWindow() const { return nullptr; }

	FloatingTile* getRootFloatingTile() { return &root; }
	FloatingTile root;
};
#endif

HiseModuleDatabase::ScreenshotProvider::ScreenshotProvider(MarkdownParser* parent, BackendProcessor* bp_) :
	ImageProvider(parent),
	bp(bp_->getDocProcessor())
{
	w = bp->getDocProcessor()->getDocWindow();

	jassert(w != nullptr);

	data->createAllProcessors();
}

HiseModuleDatabase::ScreenshotProvider::~ScreenshotProvider()
{
	
}

juce::Image HiseModuleDatabase::ScreenshotProvider::getImage(const MarkdownLink& url, float width)
{
	auto urlString = url.toString(MarkdownLink::UrlFull);

	if (urlString.contains("module_screenshot_"))
	{
		auto pId = urlString.fromFirstOccurrenceOf("module_screenshot_", false, false).upToFirstOccurrenceOf(".png", false, false);


		auto rootDir = parent->getHolder()->getDatabaseRootDirectory();

		if(rootDir.isDirectory())
		{
 			auto snDir = rootDir.getChildFile("images/override/modules/");

			if(snDir.isDirectory())
			{
				auto of = snDir.getChildFile(pId).withFileExtension(".png");

				if(of.existsAsFile())
				{
					auto img = ImageFileFormat::loadFrom(of);

					updateWidthFromURL(url, width);
					return resizeImageToFit(img, width);
				}
			}
		}


		MarkdownLink imageURL(url.getRoot(), pId);
		imageURL.setType(MarkdownLink::Type::Image);

		data->bp->getMainSynthChain()->setId("Autogenerated");

		if (auto p = getProcessorForURL(imageURL))
		{
			for (auto ci : data->cachedImage)
			{
				if (ci.url == url)
					return ci.cachedImage;
			}

			ScopedPointer<ProcessorEditorContainer> c = new ProcessorEditorContainer();

			w->addAndMakeVisible(c);

			p->setParentProcessor(data->bp->getMainSynthChain());

			if (auto mod = dynamic_cast<Modulator*>(p))
				mod->setColour(Colour(0xffbe952c));

			p->setId(p->getName());

			MainController::ScopedBadBabysitter sb(p->getMainController());
			ScopedPointer<ProcessorEditor> editor = new ProcessorEditor(c, 1, p, nullptr);
			w->addAndMakeVisible(editor);
			editor->setSize(800, editor->getHeight());
			auto img = editor->createComponentSnapshot(editor->getLocalBounds());

			data->cachedImage.add({ url, img });

			return img;
		}
	}

	

	return {};
}


}

namespace scriptnode
{

namespace doc
{
using namespace juce;
using namespace hise;

ItemGenerator::ItemGenerator(File r, BackendProcessor& bp):
	MarkdownDataBase::ItemGeneratorBase(r)
{
    MainController::ScopedBadBabysitter sb(&bp);

	data->sine = new SineSynth(&bp, "Sine", NUM_POLYPHONIC_VOICES);
	data->sine->prepareToPlay(44100.0, 512);
	auto fxChain = dynamic_cast<EffectProcessorChain*>(data->sine->getChildProcessor(ModulatorSynth::EffectChain));
	data->effect = new JavascriptMasterEffect(&bp, "dsp");
	data->network = data->effect->getOrCreate("dsp");
	fxChain->getHandler()->add(data->effect, nullptr);
}


hise::MarkdownDataBase::Item::Ptr ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	auto root = MarkdownDataBase::Item::createNew();
	root->url = MarkdownLink(rootDirectory, getWildcard());
	root->fillMetadataFromURL();
	root->keywords = { "ScriptNode" };
	root->tocString = "ScriptNode";
	root->c = Colour(CommonData::colour);

	{
		MarkdownDataBase::DirectoryItemGenerator mgen(rootDirectory.getChildFile("scriptnode/manual"), root->c);

		auto manual = mgen.createRootItem(parent);
		manual->fillMetadataFromURL();
		root->addChild(manual);
	}

	{
		MarkdownDataBase::DirectoryItemGenerator mgen(rootDirectory.getChildFile("scriptnode/101"), root->c);

		auto manual = mgen.createRootItem(parent);
		manual->fillMetadataFromURL();
		root->addChild(manual);
	}
	
    {
        MarkdownDataBase::DirectoryItemGenerator mgen(rootDirectory.getChildFile("scriptnode/snex_api"), root->c);

        auto manual = mgen.createRootItem(parent);
        manual->fillMetadataFromURL();
        root->addChild(manual);
    }

	MainController::ScopedBadBabysitter sb(data->network->getScriptProcessor()->getMainController_());

	ScopedPointer<NodeBase::Holder> s = new NodeBase::Holder();
	
	auto list = data->network->getListOfAvailableModulesAsTree();

	auto lItem = MarkdownDataBase::Item::createNew();
	lItem->url = root->url.getChildUrl("list");
	lItem->url.setType(MarkdownLink::Folder);
	lItem->tocString = "List of Nodes";
	lItem->c = root->c;

	for (auto f : list)
	{
		addNodeFactoryItem(f, lItem);
	}

	root->addChild(lItem);
    
	return root;
}


void ItemGenerator::addNodeFactoryItem(ValueTree factoryTree, MarkdownDataBase::Item::Ptr list)
{
	auto fItem = MarkdownDataBase::Item::createNew();

	fItem->url = list->url.getChildUrl(factoryTree[PropertyIds::ID].toString());
	fItem->url.setType(MarkdownLink::Type::Folder);
	fItem->tocString = factoryTree[PropertyIds::ID].toString();
	fItem->c = Colour(CommonData::colour);

	for (auto nt : factoryTree)
	{
		addNodeItem(nt, fItem);
	}

	list->addChild(fItem);
}


void ItemGenerator::addNodeItem(ValueTree nodeTree, MarkdownDataBase::Item::Ptr factory)
{
	auto path = nodeTree[PropertyIds::ID].toString();
	auto id = path.fromFirstOccurrenceOf(".", false, false);
	
	MessageManagerLock lock;

	NodeBase::Ptr nb = dynamic_cast<NodeBase*>(data->network->create(path, id).getObject());;

	jassert(nb != nullptr);

	auto nItem = MarkdownDataBase::Item::createNew();

	nItem->url = factory->url.getChildUrl(id);
	nItem->url.setType(MarkdownLink::Type::MarkdownFile);
	nItem->tocString = id;
	nItem->c = Colour(CommonData::colour);
	nItem->keywords = { path, id };

	factory->addChild(nItem);
}


Resolver::Resolver(File root_):
	MarkdownParser::LinkResolver()
{
	rootUrl = MarkdownLink(root, getWildcard());
}


juce::String Resolver::getContent(const MarkdownLink& url)
{
	if (url.isChildOf(rootUrl))
	{
		if (url.isChildOf(rootUrl.getChildUrl("list")))
		{
			auto docFile = url.getMarkdownFile(url.getRoot());

			if (!docFile.existsAsFile())
			{
				docFile.create();
				return url.getEditLinkOnGitHub(false);
			}

			auto nodeId = url.toString(MarkdownLink::Format::UrlSubPath);

			auto header = url.getHeaderFromFile(root);

			auto parameterDescriptions = header.getKeyList("parameters");
			auto propertyDescriptions = header.getKeyList("properties");

			

			auto mc = const_cast<MainController*>(data->network->getScriptProcessor()->getMainController_());

			MainController::ScopedBadBabysitter sbs(mc);

			data->network->clear(true, true);

			

			auto factory = url.getParentUrl().toString(MarkdownLink::Format::UrlSubPath);

			if(factory.isEmpty())
			{
				return "";
			}

			jassert(factory.isNotEmpty());

			NodeBase::Ptr node = dynamic_cast<NodeBase*>(data->network->create(factory + "." + nodeId, nodeId).getObject());

			if (node != nullptr)
			{
				auto tree = node->getValueTree();



				String content;
				String nl = "\n";

				content << url.toString(MarkdownLink::Format::ContentHeader);

				auto factory = tree[PropertyIds::FactoryPath].toString();

				if(!inlineDocMode)
					content << "> `" << factory << "`" << nl;

				auto cTree = tree.getChildWithName(PropertyIds::ComplexData);

				snex::ExternalData::forEachType([&](ExternalData::DataType dt)
				{
					auto did = Identifier(ExternalData::getDataTypeName(dt, true));

					auto dtree = cTree.getChildWithName(did);
					auto numChildren = dtree.getNumChildren();

					if(numChildren > 0)
					{
						content << "`" << ExternalData::getDataTypeName(dt, false) << "` slots: **" + String(numChildren) << "**" << nl;
					}
				});

				content << "![screen](/images/sn_screen_" << factory.upToFirstOccurrenceOf(".", false, false) << "__" << nodeId << ".png)";

				

				content << header.getKeyValue("summary") << nl;

				if (node->getNumParameters() > 0)
				{
					if(inlineDocMode)
						content << "### Parameters" << nl;
					else
						content << "## Parameters" << nl;

					content << "| ID | Range | Default | Description |" << nl;
					content << "| --- | --- | --- | ------ |" << nl;

					for (auto param: NodeBase::ParameterIterator(*node))
					{
						auto pId = param->getId();
						auto pTree = param->data;

						content << "| " << pId;

						auto range = RangeHelpers::getDoubleRange(pTree);
						content << " | " << String(range.rng.start, 2) << " - " << String(range.rng.end, 2);
						content << " | " << String((double)pTree[PropertyIds::Value], 2);

						bool found = false;

						for (auto d : parameterDescriptions)
						{
							if (d.trim().startsWith(pId))
							{
								auto desc = d.fromFirstOccurrenceOf(":", false, false).trim();
								content << " | " << desc << " |" << nl;
								found = true;
								break;
							}
						}

						if (!found)
							content << " | " << "no description." << " |" << nl;

					}

					content << nl;
				}

				auto propTree = node->getPropertyTree();

				if(propTree.getNumChildren() > 0)
				{
					if(inlineDocMode)
						content << "### Properties" << nl;
					else
						content << "## Properties" << nl;

					content << "| ID | Type | Default | Description |" << nl;
					content << "| --- | --- | --- | ------ |" << nl;

					for(auto p: propTree)
					{
						auto pId = p[PropertyIds::ID].toString();
						auto dv = p[PropertyIds::Value];

						String typeName;

						if(dv.isBool())
							typeName = "bool";
						else if(dv.isInt() || dv.isInt64() || dv.isDouble() || juce::CharacterFunctions::isDigit(dv.toString()[0]))
							typeName = "int";
						else
							typeName = "String";

						content << "| " << pId;
						content << " | " << typeName;
						content << " | " << (dv.isBool() ? String((bool)dv ? "true" : "false") : dv.toString());

						bool found = false;

						for (auto d : propertyDescriptions)
						{
							if (d.trim().startsWith(pId))
							{
								auto desc = d.fromFirstOccurrenceOf(":", false, false).trim();
								content << " | " << desc << " |" << nl;
								found = true;
								break;
							}
						}

						if (!found)
							content << " | " << "no description." << " |" << nl;

						
					}

					content << nl;
				}



				content << url.toString(MarkdownLink::Format::ContentWithoutHeader);

				return content;
			}
		}
		else
		{
			return url.toString(MarkdownLink::Format::ContentFull);
		}
	}

	return {};
}


hise::Image ScreenshotProvider::getImage(const MarkdownLink& url, float width)
{
	ignoreUnused(width);
	auto imageFileURL = url.toString(MarkdownLink::Format::UrlSubPath).upToFirstOccurrenceOf(".png", false, false);

	if (imageFileURL.startsWith("sn_screen_"))
	{
		auto f__id = imageFileURL.fromFirstOccurrenceOf("sn_screen_", false, false);

		auto id = f__id.fromFirstOccurrenceOf("__", false, false);
		auto factory = f__id.upToFirstOccurrenceOf("__", false, false);

		auto rootDir = parent->getHolder()->getDatabaseRootDirectory();

		if(rootDir.isDirectory())
		{
 			auto snDir = rootDir.getChildFile("images/override/scriptnode/");

			if(snDir.isDirectory())
			{
				auto of = snDir.getChildFile(id).withFileExtension(".png");

				if(of.existsAsFile())
				{
					auto img = ImageFileFormat::loadFrom(of);

					updateWidthFromURL(url, width);
					return resizeImageToFit(img, width);
				}
			}
		}

		auto mc = const_cast<MainController*>(data->network->getScriptProcessor()->getMainController_());

		MainController::ScopedBadBabysitter sbs(mc);

		data->network->clear(true, true);

		NodeBase::Ptr node = dynamic_cast<NodeBase*>(data->network->create(factory + "." + id, id).getObject());

		if (node != nullptr)
		{
			MessageManagerLock mmlock;
			ScopedPointer<Component> c = scriptnode::NodeComponentFactory::createComponent(node);
			c->setBounds(node->getPositionInCanvas({ 0, 0 }));
			return c->createComponentSnapshot(c->getLocalBounds());
		}
	}

	return {};
}

}
}
