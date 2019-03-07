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
	Path createPath(const String& id) const override
	{
		auto url = HtmlGenerator::getSanitizedFilename(id);

		Path path;

		if (url == "new")
		{
			static const unsigned char pathData[] = { 110,109,0,0,140,66,0,0,252,65,108,0,0,64,66,0,0,252,65,108,0,0,42,66,0,0,208,65,108,0,0,208,65,0,0,208,65,98,0,0,184,65,0,0,208,65,0,0,164,65,0,0,228,65,0,0,164,65,0,0,252,65,108,0,0,164,65,0,0,129,66,98,0,0,164,65,0,0,135,66,0,0,184,65,0,0,140,66,0,
				0,208,65,0,0,140,66,108,0,0,140,66,0,0,140,66,98,0,0,146,66,0,0,140,66,0,0,151,66,0,0,135,66,0,0,151,66,0,0,129,66,108,0,0,151,66,0,0,20,66,98,0,0,151,66,0,0,8,66,0,0,146,66,0,0,252,65,0,0,140,66,0,0,252,65,99,109,154,153,134,66,0,0,86,66,108,1,0,108,
				66,0,0,86,66,108,1,0,108,66,51,51,119,66,108,1,0,86,66,51,51,119,66,108,1,0,86,66,0,0,86,66,108,206,204,52,66,0,0,86,66,108,206,204,52,66,0,0,64,66,108,1,0,86,66,0,0,64,66,108,1,0,86,66,205,204,30,66,108,1,0,108,66,205,204,30,66,108,1,0,108,66,0,0,64,
				66,108,154,153,134,66,0,0,64,66,108,154,153,134,66,0,0,86,66,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));

			return path;
		}
		if (url == "open")
		{
			static const unsigned char pathData[] = { 110,109,249,109,142,67,168,198,186,65,98,181,96,143,67,168,198,186,65,200,125,144,67,164,246,176,65,160,229,144,67,138,149,163,65,108,36,201,152,67,123,139,22,193,98,236,65,153,67,185,157,51,193,74,146,152,67,183,237,68,193,18,86,151,67,183,237,68,193,
				108,192,94,143,67,183,237,68,193,108,192,94,143,67,63,138,155,193,98,192,94,143,67,11,61,175,193,86,94,142,67,168,67,191,193,42,35,141,67,168,67,191,193,108,82,69,115,67,168,67,191,193,108,124,96,112,67,102,7,252,193,98,90,19,112,67,129,43,1,194,189,
				81,111,67,57,66,3,194,116,121,110,67,57,66,3,194,108,178,166,94,67,57,66,3,194,98,17,28,94,67,57,66,3,194,232,150,93,67,64,101,2,194,238,52,93,67,63,219,0,194,98,54,211,92,67,125,162,254,193,192,156,92,67,58,121,250,193,136,157,92,67,27,34,246,193,108,
				112,167,92,67,64,138,155,193,108,160,147,92,67,88,40,151,65,98,160,147,92,67,35,219,170,65,116,148,94,67,193,225,186,65,205,10,97,67,193,225,186,65,109,149,85,141,67,142,233,68,193,108,122,237,119,67,142,233,68,193,98,148,83,118,67,142,233,68,193,213,
				207,116,67,88,54,48,193,193,2,116,67,82,135,22,193,108,72,211,101,67,29,79,154,65,108,205,10,97,67,29,79,154,65,98,76,211,96,67,29,79,154,65,244,165,96,67,98,228,152,65,244,165,96,67,89,40,151,65,108,244,165,96,67,45,161,155,193,108,113,165,96,67,182,
				239,229,193,108,96,18,109,67,182,239,229,193,108,54,247,111,67,248,43,169,193,98,88,68,112,67,91,220,162,193,245,5,113,67,237,174,158,193,61,222,113,67,237,174,158,193,108,42,35,141,67,237,174,158,193,98,234,62,141,67,237,174,158,193,150,85,141,67,50,
				68,157,193,150,85,141,67,42,136,155,193,108,150,85,141,67,140,233,68,193,99,101,0,0 };

			path.loadPathFromData(pathData, sizeof(pathData));
			return path;
		}
		if (url == "rebuild")
		{
			return ColumnIcons::getPath(ColumnIcons::moveIcon, sizeof(ColumnIcons::moveIcon));
		}
		if (url == "undo")
		{
			static const unsigned char undoIcon[] = { 110,109,0,93,96,67,64,87,181,67,98,169,116,87,67,119,74,181,67,238,53,75,67,247,66,184,67,128,173,59,67,64,229,191,67,108,0,0,47,67,64,46,186,67,108,0,0,47,67,64,174,203,67,108,0,0,82,67,64,174,203,67,108,0,86,71,67,128,123,197,67,98,221,255,111,67,79,
				174,178,67,128,164,101,67,210,215,207,67,128,228,102,67,64,179,210,67,98,201,215,119,67,101,133,198,67,205,117,117,67,136,117,181,67,0,93,96,67,64,87,181,67,99,101,0,0 };

			path.loadPathFromData(undoIcon, sizeof(undoIcon));

			return path;
		}
		if (url == "redo")
		{
			static const unsigned char redoIcon[] = { 110,109,90,186,64,67,64,87,181,67,98,176,162,73,67,118,74,181,67,108,225,85,67,247,66,184,67,218,105,101,67,64,229,191,67,108,90,23,114,67,64,46,186,67,108,90,23,114,67,64,174,203,67,108,90,23,79,67,64,174,203,67,108,90,193,89,67,128,123,197,67,98,125,
				23,49,67,79,174,178,67,218,114,59,67,211,215,207,67,218,50,58,67,64,179,210,67,98,145,63,41,67,101,133,198,67,141,161,43,67,136,117,181,67,90,186,64,67,64,87,181,67,99,101,0,0 };

			path.loadPathFromData(redoIcon, sizeof(redoIcon));

			return path;
		}

		return path;
	};



};



ExpansionEditBar::ExpansionEditBar(FloatingTile* parent) :
	FloatingTileContent(parent),
	factory(new ExpansionPathFactory())
{
	ExpansionPathFactory f;

	buttons.add(new HiseShapeButton("New", this, f));  buttons.getLast()->setTooltip("Create a new expansion pack folder");
	buttons.add(new HiseShapeButton("Rebuild", this, f)); buttons.getLast()->setTooltip("Refresh the expansion pack data");
	buttons.add(new HiseShapeButton("Undo", this, f)); buttons.getLast()->setTooltip("Undo");
	buttons.add(new HiseShapeButton("Redo", this, f)); buttons.getLast()->setTooltip("Redo");

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

	getButton("Undo")->setBounds(area.removeFromLeft(widthForIcon));
	getButton("Redo")->setBounds(area.removeFromLeft(widthForIcon));
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
}

void ExpansionEditBar::expansionPackLoaded(Expansion* e)
{
	if (e != nullptr)
	{
		expansionSelector->setText(e->name.get(), dontSendNotification);
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