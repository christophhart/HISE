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
		allProcessors.add(f->createProcessor(i, "id"));
	}
}

juce::String HiseModuleDatabase::CommonData::getProcessorIdFromURL(const MarkdownLink& url)
{
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

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createItemForProcessor(Processor* p, const MarkdownDataBase::Item& parent)
{
	MarkdownDataBase::Item newItem;

	newItem.tocString << p->getName();
	newItem.type = MarkdownDataBase::Item::Keyword;
	newItem.keywords.add(p->getName());
	newItem.description = p->getDescription();
	

	newItem.c = p->getColour();
	newItem.url = parent.url.getChildUrl(p->getType().toString());

	auto f = newItem.url.getMarkdownFile(rootDirectory);

	if (f.existsAsFile())
	{
		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, newItem, f, newItem.c);
	}

	MarkdownDataBase::Item pItem;

	pItem.type = MarkdownDataBase::Item::Headline;
	pItem.tocString << "Parameters";
	pItem.url = newItem.url.getChildUrl("parameters", true);
	pItem.c = newItem.c;

	newItem.addChild(std::move(pItem));
	
	return newItem;
}

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createItemForFactory(FactoryType* owned, const String& factoryName, MarkdownDataBase::Item& parent)
{
	ScopedPointer<FactoryType> f = owned;

	auto n = f->getNumProcessors();

	MarkdownDataBase::Item list;
	list.url = parent.url.getChildUrl("list");
	list.tocString = "List of " + factoryName;
	list.keywords.add(factoryName);
	list.type = MarkdownDataBase::Item::Folder;

	for (int i = 0; i < n; i++)
	{
		ScopedPointer<Processor> p = f->createProcessor(i, "funky");

		if (p->getDescription() == "deprecated")
			continue;

		parent.c = p->getColour();
		list.addChild(createItemForProcessor(p, list));
	}
	list.isAlwaysOpen = true;
	list.sortChildren();
	
	return list;
}

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createItemForCategory(const String& categoryName, const MarkdownDataBase::Item& parent)
{
	MarkdownDataBase::Item item;

	item.tocString << categoryName;
	item.url = parent.url.getChildUrl(categoryName);
	item.type = MarkdownDataBase::Item::Folder;

	item.addTocChildren(rootDirectory);

	return item;
}

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	MarkdownDataBase::Item rootItem;

	rootItem.type = MarkdownDataBase::Item::Folder;
	rootItem.tocString = "HISE Modules";
	rootItem.type = MarkdownDataBase::Item::Folder;
	rootItem.url = { rootDirectory, moduleWildcard };
	
	auto bp = data->bp;

	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(NUM_POLYPHONIC_VOICES, bp->getMainSynthChain());

	auto sg = createItemForCategory("Sound Generators", rootItem);

	auto sg2 = createItemForFactory(new ModulatorSynthChainFactoryType(1, bp->getMainSynthChain()),
		"Sound Generators", sg);

	sg.addChild(std::move(sg2));

	rootItem.addChild(std::move(sg));

	auto mp = createItemForCategory("MIDI Processors", rootItem);

	auto mp2 = createItemForFactory(new MidiProcessorFactoryType(bp->getMainSynthChain()), "MIDI Processors", mp);
	mp.addChild(std::move(mp2));

	rootItem.addChild(std::move(mp));

	auto modItem = createItemForCategory("Modulators", rootItem);

	
	auto vs = createItemForCategory("Voice Start Modulators", modItem);

	auto vs2 = createItemForFactory(new VoiceStartModulatorFactoryType(1, Modulation::GainMode, bp->getMainSynthChain()),
		"Voice Start Modulators", vs);

	vs.addChild(std::move(vs2));

	vs.isAlwaysOpen = false;

	modItem.addChild(std::move(vs));

	auto tv = createItemForCategory("Time Variant Modulators", modItem);

	auto tv2 = createItemForFactory(new TimeVariantModulatorFactoryType(Modulation::GainMode, bp->getMainSynthChain()),
		"Time Variant Modulators", tv);

	tv.addChild(std::move(tv2));
	tv.isAlwaysOpen = false;

	modItem.addChild(std::move(tv));
	

	auto em = createItemForFactory(new EnvelopeModulatorFactoryType(1, Modulation::GainMode, bp->getMainSynthChain()),
		"Envelopes", modItem);

	em.isAlwaysOpen = false;

	modItem.addChild(std::move(em));

	rootItem.addChild(std::move(modItem));

	auto fx = createItemForCategory("Effects", rootItem);
	auto fx2 = createItemForFactory(new EffectProcessorChainFactoryType(1, bp->getMainSynthChain()), "Effects", fx);
	fx.addChild(std::move(fx2));
	rootItem.addChild(std::move(fx));

	applyColour(rootItem);

	return rootItem;
}

HiseModuleDatabase::Resolver::Resolver(File root_) :
	LinkResolver(),
	root(root_)
{
	data->createAllProcessors();
}


struct DummyProcessorDoc : public ProcessorDocumentation
{

};


juce::String HiseModuleDatabase::Resolver::getContent(const MarkdownLink& url)
{
	if (auto p = getProcessorForURL(url))
	{
		String s;

		String fileContent;

		NewLine nl;

		auto f = url.getMarkdownFile(root);


		if (!f.existsAsFile() && MessageManager::getInstance()->isThisTheMessageThread())
		{
			if (PresetHandler::showYesNoWindow("Create file", "Do you want to create a file for this module"))
			{
				f = MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(f.getParentDirectory(), p->getType().toString(), p->getDescription());
			}
		}

		MarkdownHeader header = url.getHeaderFromFile(root, false);

		s << url.toString(MarkdownLink::ContentHeader, root);

		s << "Type ID: `" << p->getType() << "`  " << nl;

		StringArray interfaces;

		if (dynamic_cast<SlotFX*>(p) != nullptr)
			interfaces.add("SlotFX");
		if (dynamic_cast<MidiPlayer*>(p) != nullptr)
			interfaces.add("MidiPlayer");
		if (dynamic_cast<ModulatorSampler*>(p) != nullptr)
			interfaces.add("Sampler");
		if (dynamic_cast<AudioSampleProcessor*>(p) != nullptr)
			interfaces.add("AudioSampleProcessor");
		if (dynamic_cast<LookupTableProcessor*>(p) != nullptr)
			interfaces.add("TableProcessor");
		if (dynamic_cast<SliderPackProcessor*>(p) != nullptr)
			interfaces.add("SliderPackProcessor");
		if (dynamic_cast<RoutableProcessor*>(p) != nullptr)
			interfaces.add("RoutingMatrix");
		
		if (interfaces.size() > 0)
		{
			s << "Interface classes: ";

			MarkdownLink iLink(root, ScriptingApiDatabase::apiWildcard);

			for (auto i : interfaces)
			{
				//s << iLink.getChildUrl(i).toString(MarkdownLink::FormattedLinkMarkdown);

				s << "[`" << i << "`](/scripting/scripting-api/" << MarkdownLink::Helpers::getSanitizedFilename(i) << ") ";
			}

			s << " \n";
		}

		s << "> **" << header.getDescription() << "**  " << nl << nl;

		auto id = getProcessorIdFromURL(url);

		s << "![](/images/module_screenshot_" << id << ".png)  " << nl;

		s << url.toString(MarkdownLink::ContentWithoutHeader) << nl;

		ScopedPointer<ProcessorDocumentation> doc = p->createDocumentation();

		if (doc == nullptr)
			doc = new DummyProcessorDoc();

		if (ProcessorHelpers::is<ModulatorSynth>(p))
		{
			doc->setOffset(ModulatorSynth::Parameters::numModulatorSynthParameters,
				ModulatorSynth::numInternalChains);
		}

		doc->fillMissingParameters(p);
		auto pList = header.getKeyList("parameters");

		for (auto& p : doc->parameters)
		{
			if (p.helpText == "-")
			{
				auto id = p.id.toString();

				for (const auto& s : pList)
				{
					if (s.startsWith(id))
						p.helpText = s.fromFirstOccurrenceOf(":", false, false).trim();
				}
			}
		}

		auto cList = header.getKeyList("chains");

		for (auto& c : doc->chains)
		{
			if (c.helpText == "-")
			{
				auto id = c.id.toString();

				for (const auto& s : cList)
				{
					if (s.startsWith(id))
						c.helpText = s.fromFirstOccurrenceOf(":", false, false).trim();
				}
			}
		}

		s << doc->createHelpText();

		return s;
	}
	
	return {};
}

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

HiseModuleDatabase::ScreenshotProvider::ScreenshotProvider(MarkdownParser* parent) :
	ImageProvider(parent)
{
	{
		MessageManagerLock mmLock;

		w = new RootWindow(data->bp);

		w->setLookAndFeel(&laf);
	}
	
	data->createAllProcessors();
}

HiseModuleDatabase::ScreenshotProvider::~ScreenshotProvider()
{
	delete w;
}

juce::Image HiseModuleDatabase::ScreenshotProvider::getImage(const MarkdownLink& url, float width)
{
	auto urlString = url.toString(MarkdownLink::UrlFull);

	if (urlString.contains("module_screenshot_"))
	{
		auto pId = urlString.fromFirstOccurrenceOf("module_screenshot_", false, false).upToFirstOccurrenceOf(".png", false, false);

		MarkdownLink imageURL(url.getRoot(), pId);

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

			ScopedPointer<ProcessorEditor> editor = new ProcessorEditor(c, 1, p, nullptr);

			w->addAndMakeVisible(editor);

			editor->setLookAndFeel(&laf);

			auto img = editor->createComponentSnapshot(editor->getLocalBounds());

			data->cachedImage.add({ url, img });

			return img;
		}
	}

	

	return {};
}


}
