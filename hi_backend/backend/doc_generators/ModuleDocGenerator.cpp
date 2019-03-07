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
	if (!allProcessors.isEmpty())
		return;

	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(1, bp.getMainSynthChain());
	addFromFactory(f);

	f = new MidiProcessorFactoryType(bp.getMainSynthChain());
	addFromFactory(f);

	f = new ModulatorChainFactoryType(1, Modulation::GainMode, bp.getMainSynthChain());
	addFromFactory(f);

	f = new EffectProcessorChainFactoryType(1, bp.getMainSynthChain());
	addFromFactory(f);
}

void HiseModuleDatabase::CommonData::Data::addFromFactory(FactoryType* f)
{
	for (int i = 0; i < f->getNumProcessors(); i++)
	{
		allProcessors.add(f->createProcessor(i, "id"));
	}
}

juce::String HiseModuleDatabase::CommonData::getProcessorIdFromURL(const String& url, bool forceModuleWildcard, String idPrefix)
{
	if (idPrefix.isEmpty())
		idPrefix = "/";

	if (!forceModuleWildcard || url.startsWith(moduleWildcard))
	{
		return url.fromLastOccurrenceOf(idPrefix, false, false).upToLastOccurrenceOf(".", false, false);
	}

	return {};
}

hise::Processor* HiseModuleDatabase::CommonData::getProcessorForURL(const String& url, bool forceModuleWildcard, String prefix)
{
	auto id = getProcessorIdFromURL(url, forceModuleWildcard, prefix);

	if (id.isNotEmpty())
	{
		for (auto p : data->allProcessors)
		{
			auto sanitizedId = HtmlGenerator::getSanitizedFilename(p->getName());

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

	newItem.c = p->getColour();
	newItem.url << parent.url << "/" << HtmlGenerator::getSanitizedFilename(p->getName()) << ".md";
	newItem.fileName = p->getName();

	return newItem;
}

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createItemForFactory(FactoryType* owned, const String& factoryName, const MarkdownDataBase::Item& parent)
{
	ScopedPointer<FactoryType> f = owned;

	MarkdownDataBase::Item item;

	item.tocString << factoryName;
	item.url << parent.url << "/" << HtmlGenerator::getSanitizedFilename(factoryName);
	item.fileName = factoryName;
	item.type = MarkdownDataBase::Item::Folder;

	auto n = f->getNumProcessors();

	for (int i = 0; i < n; i++)
	{
		ScopedPointer<Processor> p = f->createProcessor(i, "funky");

		if (p->getDescription() == "deprecated")
			continue;

		item.c = p->getColour();

		item.children.add(createItemForProcessor(p, item));
	}

	item.children.sort(MarkdownDataBase::Item::Sorter());

	auto d = HtmlGenerator::getLocalFileForSanitizedURL(rootDirectory, item.url, File::findDirectories);

	if (d.isDirectory())
	{
		auto readme = d.getChildFile("Readme.md");

		if (readme.existsAsFile())
		{

			MarkdownDataBase::Item rItem;

			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, rItem, readme, item.c);

			item.children.insert(0, rItem);
		}
	}



	return item;
}

hise::MarkdownDataBase::Item HiseModuleDatabase::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	MarkdownDataBase::Item rootItem;

	rootItem.type = MarkdownDataBase::Item::Folder;
	rootItem.tocString = "HISE Modules";
	rootItem.url << moduleWildcard;
	rootItem.fileName = rootItem.url;

	auto& bp = data->bp;

	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(NUM_POLYPHONIC_VOICES, bp.getMainSynthChain());

	auto sg = createItemForFactory(new ModulatorSynthChainFactoryType(1, bp.getMainSynthChain()),
		"Sound Generators", rootItem);

	auto mp = createItemForFactory(new MidiProcessorFactoryType(bp.getMainSynthChain()), "MIDI Processors", rootItem);



	MarkdownDataBase::Item modItem;
	modItem.tocString << "Modulators";
	modItem.url << moduleWildcard << "/" << "modulators";
	modItem.fileName = modItem.tocString;
	modItem.type = MarkdownDataBase::Item::Folder;

	auto vs = createItemForFactory(new VoiceStartModulatorFactoryType(1, Modulation::GainMode, bp.getMainSynthChain()),
		"Voice Start", modItem);

	modItem.children.add(vs);

	auto tv = createItemForFactory(new TimeVariantModulatorFactoryType(Modulation::GainMode, bp.getMainSynthChain()),
		"Time Variant", modItem);

	modItem.children.add(tv);

	auto em = createItemForFactory(new EnvelopeModulatorFactoryType(1, Modulation::GainMode, bp.getMainSynthChain()),
		"Envelopes", modItem);

	modItem.children.add(em);


	modItem.c = Colour(0xffbe952c);

	for (auto& i : modItem.children)
	{

		i.c = modItem.c;

		for (auto& s : i.children)
			s.c = modItem.c;
	}


	auto poly = createItemForFactory(new EffectProcessorChainFactoryType(1, bp.getMainSynthChain()), "Effects", rootItem);

	rootItem.children.add(std::move(sg));
	rootItem.children.add(std::move(mp));
	rootItem.children.add(std::move(modItem));
	rootItem.children.add(std::move(poly));

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


juce::String HiseModuleDatabase::Resolver::getContent(const String& url)
{
	if (auto p = getProcessorForURL(url, true, {}))
	{
		String s;
		NewLine nl;

		s << "# " << p->getName() << nl;
		s << "Type ID: `" << p->getType() << "`  " << nl;

		s << "**" << p->getDescription() << "**  " << nl;

		auto id = getProcessorIdFromURL(url, true, {});

		s << "![](/images/module_screenshot_" << id << ".png)  " << nl;

		auto f = HtmlGenerator::getLocalFileForSanitizedURL(root, url.replace(".md", ""), File::findFiles, "*.md");

		if (f.existsAsFile())
		{
			s << f.loadFileAsString() << nl;
		}

		ScopedPointer<ProcessorDocumentation> doc = p->createDocumentation();

		if (doc == nullptr)
			doc = new DummyProcessorDoc();

		doc->fillMissingParameters(p);

		if (ProcessorHelpers::is<ModulatorSynth>(p))
		{
			doc->setOffset(ModulatorSynth::Parameters::numModulatorSynthParameters,
				ModulatorSynth::numInternalChains);
		}

		s << doc->createHelpText();

		return s;
	}
	else if (url.contains(moduleWildcard))
	{
		auto f = HtmlGenerator::getLocalFileForSanitizedURL(root, url, File::findDirectories).getChildFile("Readme.md");

		if (f.existsAsFile())
			return f.loadFileAsString();
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
	w = new RootWindow(&data->bp);

	w->setLookAndFeel(&laf);
	data->createAllProcessors();
}

HiseModuleDatabase::ScreenshotProvider::~ScreenshotProvider()
{
	delete w;
}

juce::Image HiseModuleDatabase::ScreenshotProvider::getImage(const String& url, float width)
{
	data->bp.getMainSynthChain()->setId("Autogenerated");

	if (auto p = getProcessorForURL(url, false, "module_screenshot_"))
	{
		ScopedPointer<ProcessorEditorContainer> c = new ProcessorEditorContainer();

		w->addAndMakeVisible(c);

		p->setParentProcessor(data->bp.getMainSynthChain());

		if (auto mod = dynamic_cast<Modulator*>(p))
			mod->setColour(Colour(0xffbe952c));

		p->setId(p->getName());

		ScopedPointer<ProcessorEditor> editor = new ProcessorEditor(c, 1, p, nullptr);

		w->addAndMakeVisible(editor);

		editor->setLookAndFeel(&laf);

		return editor->createComponentSnapshot(editor->getLocalBounds());
	}

	return {};
}

}