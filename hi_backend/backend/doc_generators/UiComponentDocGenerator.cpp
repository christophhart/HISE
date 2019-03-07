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

hise::ScriptComponent* UIComponentDatabase::CommonData::getComponentForURL(const String& url, bool forceModuleWildcard, String idPrefix)
{
	auto id = getComponentIDFromURL(url, forceModuleWildcard, idPrefix);

	if (id.isNotEmpty())
	{
		for (auto c : d->list)
		{
			auto id_ = HtmlGenerator::getSanitizedFilename(c->getName().toString());

			if (id == id_)
				return c;
		}
	}

	return nullptr;
}

juce::String UIComponentDatabase::CommonData::getComponentIDFromURL(const String& url, bool forceModuleWildcard, String idPrefix)
{
	if (idPrefix.isEmpty())
		idPrefix = "/";

	if (!forceModuleWildcard || url.startsWith(uiComponentWildcard))
	{
		return url.fromLastOccurrenceOf(idPrefix, false, false).upToLastOccurrenceOf(".", false, false);
	}

	return {};
}

hise::MarkdownDataBase::Item UIComponentDatabase::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	MarkdownDataBase::Item item;

	item.c = Colour(0xFF124958F);
	item.tocString = "UI Components";
	item.type = MarkdownDataBase::Item::Folder;
	item.url = uiComponentWildcard;
	item.fileName = item.tocString;

	for (auto c : d->list)
	{
		MarkdownDataBase::Item cItem;
		cItem.type = MarkdownDataBase::Item::Keyword;
		cItem.url << uiComponentWildcard << "/" << HtmlGenerator::getSanitizedFilename(c->getName().toString()) << ".md";
		cItem.tocString << c->getName();
		cItem.fileName = cItem.tocString;
		cItem.description = "description";
		cItem.c = Colour(0xFF124958F);

		item.children.add(cItem);
	}

	return item;
}

UIComponentDatabase::Resolver::Resolver(File root_) :
	root(root_)
{}

juce::String UIComponentDatabase::Resolver::getContent(const String& url)
{
	if (auto c = getComponentForURL(url, true, ""))
	{
		String s;
		NewLine nl;

		s << "# " << c->getName().toString() << nl;
		s << "";

		s << "![](/images/ui_screenshot_" << HtmlGenerator::getSanitizedFilename(c->getName().toString()) << ".png)" << nl;

		s << "## Special Properties" << nl;

		s << "| Property ID | Default Value |" << nl;
		s << "| ------ | ----- |" << nl;

		for (int i = ScriptComponent::Properties::numProperties; i < c->getNumIds(); i++)
		{
			var value = c->getScriptObjectProperty(i);

			String valueString;

			if (value.isBool())
				valueString = (bool)value ? "`true`" : "`false`";
			else
				valueString = value.toString();
			if (valueString.isEmpty())
				valueString << "`\"\"`";

			s << "| `" << c->getIdFor(i).toString() << "` | " << valueString << " |" << nl;
		}

		return s;
	}

	return {};
}

UIComponentDatabase::ScreenshotProvider::ScreenshotProvider(MarkdownParser* parent) :
	ImageProvider(parent)
{}

juce::Image UIComponentDatabase::ScreenshotProvider::getImage(const String& url, float width)
{
	if (auto c = getComponentForURL(url, false, "ui_screenshot_"))
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

void AutogeneratedDocHelpers::registerAtDatabase(MarkdownDataBase& b, File markdownRoot)
{
	b.addItemGenerator(new UIComponentDatabase::ItemGenerator(markdownRoot));
	b.addItemGenerator(new HiseModuleDatabase::ItemGenerator(markdownRoot));
	b.addItemGenerator(new ScriptingApiDatabase::ItemGenerator(markdownRoot));
}

void AutogeneratedDocHelpers::registerAtCrawler(DatabaseCrawler& c, File markdownRoot)
{
	c.addImageProvider(new UIComponentDatabase::ScreenshotProvider(nullptr));
	c.addLinkResolver(new UIComponentDatabase::Resolver(markdownRoot));

	c.addImageProvider(new HiseModuleDatabase::ScreenshotProvider(nullptr));
	c.addLinkResolver(new HiseModuleDatabase::Resolver(markdownRoot));

	c.addLinkResolver(new ScriptingApiDatabase::Resolver(markdownRoot));

	auto p = new MarkdownParser::GlobalPathProvider(nullptr);
	p->registerFactory<ChainBarPathFactory>();
	p->registerFactory<MPEPanel::Factory>();
	p->registerFactory<MidiPlayerEditor::TransportPaths>();
	p->registerFactory<SampleMapEditor::Factory>();
	p->registerFactory<ScriptComponentEditPanel::Factory>();
	p->registerFactory<ScriptContentPanel::Factory>();

	c.addImageProvider(p);
}

}