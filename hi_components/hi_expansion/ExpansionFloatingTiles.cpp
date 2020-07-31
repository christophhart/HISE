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



class ExpansionPathFactory : public PathFactory
{
	String getId() const override { return "Expansion Pack"; }

	Path createPath(const String& id) const override
	{
		auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

		Path p;

		LOAD_PATH_IF_URL("new", EditorIcons::newFile);
		LOAD_PATH_IF_URL("open", EditorIcons::openFile);
		LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
		LOAD_PATH_IF_URL("undo", EditorIcons::undoIcon);
		LOAD_PATH_IF_URL("redo", EditorIcons::redoIcon);
		LOAD_PATH_IF_URL("encode", SampleMapIcons::monolith);

		return p;
	};
};



ExpansionEditBar::ExpansionEditBar(FloatingTile* parent) :
	FloatingTileContent(parent),
	factory(new ExpansionPathFactory())
{
	ExpansionPathFactory f;

	buttons.add(new HiseShapeButton("New", this, f));  buttons.getLast()->setTooltip("Create a new expansion pack folder");
	buttons.add(new HiseShapeButton("Rebuild", this, f)); buttons.getLast()->setTooltip("Refresh the expansion pack data");
	buttons.add(new HiseShapeButton("Encode", this, f)); buttons.getLast()->setTooltip("Encode this expansion pack");

	addAndMakeVisible(expansionSelector = new ComboBox("Expansion Selector"));

	expansionSelector->addListener(this);
	expansionSelector->setTextWhenNothingSelected("Select Expansion");
	expansionSelector->setTextWhenNoChoicesAvailable("No Expansions available");
	
	getMainController()->skin(*expansionSelector);

	refreshExpansionList();

	auto& handler = getMainController()->getExpansionHandler();

	handler.addListener(this);
		

	for (auto b : buttons)
		addAndMakeVisible(b);
}

void ExpansionEditBar::refreshExpansionList()
{
	auto& handler = getMainController()->getExpansionHandler();

	auto list = *handler.getListOfAvailableExpansions().getArray();

	expansionSelector->clear(dontSendNotification);

	expansionSelector->addItem("No expansion", 4096);

	for (int i = 0; i < list.size(); i++)
	{
		expansionSelector->addItem(list[i].toString(), i + 1);
	}
}

ExpansionEditBar::~ExpansionEditBar()
{
	auto& handler = getMainController()->getExpansionHandler();

	handler.removeListener(this);
}

void ExpansionEditBar::resized()
{
	const int widthForIcon = getHeight();
	const int spacerWidth = 15;

	auto area = getLocalBounds().reduced(3);

	getButton("New")->setBounds(area.removeFromLeft(widthForIcon));
	
	area.removeFromLeft(spacerWidth);

	expansionSelector->setBounds(area.removeFromLeft(150).expanded(1));

	area.removeFromLeft(spacerWidth);

	getButton("Rebuild")->setBounds(area.removeFromLeft(widthForIcon));

	area.removeFromLeft(spacerWidth);

	getButton("Encode")->setBounds(area.removeFromLeft(widthForIcon));
}

void ExpansionEditBar::buttonClicked(Button* b)
{
	auto& handler = getMainController()->getExpansionHandler();

	if (b->getName() == "New")
	{
		FileChooser fc("Create new Expansion", handler.getExpansionFolder(), "", true);

		if (fc.browseForDirectory())
		{
			handler.createNewExpansion(fc.getResult());
			refreshExpansionList();
		}
	}
	if (b->getName() == "Rebuild")
	{
		handler.clearExpansions();
		handler.createAvailableExpansions();
		refreshExpansionList();
	}
	if (b->getName() == "Encode")
	{
		if (auto e = handler.getCurrentExpansion())
		{
			e->encodeExpansion();
		}
	}
}

void ExpansionEditBar::expansionPackLoaded(Expansion* e)
{
	if (e != nullptr)
	{
		expansionSelector->setText(e->getProperty(ExpansionIds::Name), dontSendNotification);
	}
	else
	{
		expansionSelector->setText("No expansion", dontSendNotification);
	}
}

void ExpansionEditBar::comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/)
{
	auto& handler = getMainController()->getExpansionHandler();

	if (expansionSelector->getText() == "No expansion")
	{
		handler.setCurrentExpansion("");
	}
	else
	{
		handler.setCurrentExpansion(expansionSelector->getText());
	}

	
}

}