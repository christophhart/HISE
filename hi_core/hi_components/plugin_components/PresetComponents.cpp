
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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


class PresetBrowserWindow::CategoryList : public ListBoxModel
{
public:

	CategoryList(PresetBrowserWindow* browser_) : browser(browser_) {}

	int getNumRows() override
	{
		return categoryList.size();
	};

	void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override
	{
		String c = categoryList[rowNumber];

		if (rowIsSelected)
		{
			g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.6f), 0.0f, 0.0f,
											 Colours::white.withAlpha(0.4f), 0.0f, (float)height, false));
			
			g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 1.0f);
		}

		g.setColour(rowIsSelected ? Colours::black : Colours::white);

		g.setFont(GLOBAL_BOLD_FONT());
		
		g.drawText(c, 5, 0, width-20, height, Justification::centredLeft);
	};

	void selectedRowsChanged(int row)
	{
		browser->setCategory(row);
	}

	StringArray categoryList;

	Component::SafePointer<PresetBrowserWindow> browser;
};

class PresetBrowserWindow::PresetList : public ListBoxModel
{
public:

	PresetList(PresetBrowserWindow* browser_) : browser(browser_) {}

	int getNumRows() override
	{
		return list.size();
	};

	void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override
	{
		
		String c = list[rowNumber];

		if (rowIsSelected)
		{
			g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.6f), 0.0f, 0.0f,
				Colours::white.withAlpha(0.4f), 0.0f, (float)height, false));

			g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 1.0f);
		}

		g.setColour(rowIsSelected ? Colours::black : Colours::white);

		if (rowNumber == browser->presetIndex && browser->selectedCategoryIndex == browser->categoryIndex)
		{
			float h = 4.0f;
			float offset = ((float)height - h) / 2.0f;

			g.fillEllipse(offset, offset, h, h);
		}

		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText(c, 20, 0, width - 20, height, Justification::centredLeft);
	};

	void returnKeyPressed(int lastRowSelected)
	{
		browser->setPreset(lastRowSelected);
	}

	void listBoxItemDoubleClicked(int row, const MouseEvent&)
	{
		browser->setPreset(row);
	}

	StringArray list;

	Component::SafePointer<PresetBrowserWindow> browser;
};


PresetBrowserWindow::PresetBrowserWindow(const UserPresetData *data_) :
data(data_)
{
	addAndMakeVisible(categoryList = new ListBox());
	categoryListModel = new CategoryList(this);
	categoryList->setModel(categoryListModel);
	categoryList->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.0f));
	categoryList->setColour(ListBox::ColourIds::outlineColourId, Colours::white.withAlpha(0.2f));

	addAndMakeVisible(presetList = new ListBox());
	presetListModel = new PresetList(this);
	presetList->setModel(presetListModel);
	presetList->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.0f));
	presetList->setColour(ListBox::ColourIds::outlineColourId, Colours::white.withAlpha(0.2f));

	Path closeShape;
	addAndMakeVisible(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white.withAlpha(0.9f)));
	closeShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	closeButton->setShape(closeShape, true, true, true);
	closeButton->addListener(this);

	data->fillCategoryList(categoryListModel->categoryList);
	data->addListener(this);

	String u;

	data->getCurrentPresetIndexes(categoryIndex, presetIndex, u);

	presetList->updateContent();
	categoryList->updateContent();

	setSize(600, 500);
}

PresetBrowserWindow::~PresetBrowserWindow()
{
	data->removeListener(this);
	presetList = nullptr;
	categoryList = nullptr;
	presetListModel = nullptr;
	categoryListModel = nullptr;
}

void PresetBrowserWindow::resized()
{
	const int offset = 20;
	const int width = (getWidth() - 40) / 2 - 20;

	closeButton->setBounds(getWidth() - 28, 10, 18, 18);
	categoryList->setBounds(offset, 70, width, 400);
	presetList->setBounds(getWidth() / 2 + offset, 70, width, 400);
}



void PresetBrowserWindow::paint(Graphics &g)
{
	g.fillAll(Colours::black.withAlpha(0.9f));
	g.setGradientFill(ColourGradient(Colour(0xFF777777), 0.0f, 0.0f,
									 Colour(0xFF444444), 0.0f, 50.0f, false));

	
	

	g.fillRect(0, 0, getWidth(), 35);

	g.setColour(Colour(0xFF999999));

	g.drawRect(getLocalBounds());


	g.setColour(Colours::white.withAlpha(0.1f));
	g.fillRoundedRectangle(FLOAT_RECTANGLE(presetList->getBounds()), 2.0f);
	g.fillRoundedRectangle(FLOAT_RECTANGLE(categoryList->getBounds()), 2.0f);

	g.setColour(Colours::white.withAlpha(presetList->hasKeyboardFocus(true) ? 0.5f : 0.2f));
	g.drawRoundedRectangle(FLOAT_RECTANGLE(presetList->getBounds()), 2.0f, 1.0f);

	g.setColour(Colours::white.withAlpha(categoryList->hasKeyboardFocus(true) ? 0.5f: 0.2f));
	g.drawRoundedRectangle(FLOAT_RECTANGLE(categoryList->getBounds()), 2.0f, 1.0f);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(22.0f));
	
	g.setColour(Colours::white);
	g.drawText("Preset Browser", 0, 0, getWidth(), 35, Justification::centred);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));

	g.drawText("Presets", presetList->getBounds().expanded(0, 25), Justification::centredTop);
	g.drawText("Categories", categoryList->getBounds().expanded(0, 25), Justification::centredTop);

}

bool PresetBrowserWindow::keyPressed(const KeyPress &k)
{
	if (k == KeyPress::leftKey)
	{
		categoryList->grabKeyboardFocus();
		repaint();
		return true;
	}
	else if (k == KeyPress::rightKey)
	{
		if (presetList->getSelectedRow() == -1)
		{
			presetList->selectRow(0);
		}

		presetList->grabKeyboardFocus();
		repaint();
		return true;
	}

	return false;
}

void PresetBrowserWindow::setCategory(int newCategoryIndex)
{
	categoryIndex = newCategoryIndex;

	data->fillPresetList(presetListModel->list, categoryIndex);

	presetList->deselectAllRows();
	presetList->updateContent();
	presetList->repaint();
	repaint();
}

void PresetBrowserWindow::setPreset(int newPresetIndex)
{
	presetIndex = newPresetIndex;
	selectedCategoryIndex = categoryIndex;

	data->loadPreset(categoryIndex, presetIndex);

	presetList->repaint();
	repaint();
}

void PresetBrowserWindow::buttonClicked(Button* /*b*/)
{
	destroy();
}

void PresetBrowserWindow::presetLoaded(int categoryIndex_, int presetIndex_, const String &/*presetName*/)
{
	setCategory(categoryIndex_);
	presetIndex = presetIndex_;
	categoryList->selectRow(categoryIndex_);
	presetList->selectRow(presetIndex_);
	repaint();
}



PresetBox::PresetBox(MainController* mc_) :
mc(mc_),
data(mc->getUserPresetData())
{
	addAndMakeVisible(presetNameLabel = new Label());
	addAndMakeVisible(previousButton = new ShapeButton("", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white));
	addAndMakeVisible(nextButton = new ShapeButton("", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white));
	addAndMakeVisible(presetSaveButton = new ShapeButton("Save Preset", Colours::white.withAlpha(.6f), Colours::white.withAlpha(0.8f), Colours::white));

	data->addListener(this);


	Path bp, fp, sp;

	static const unsigned char backPathData[] = { 110, 109, 25, 226, 56, 67, 138, 253, 32, 68, 108, 25, 226, 56, 67, 138, 253, 32, 68, 108, 117, 33, 57, 67, 131, 253, 32, 68, 108, 187, 96, 57, 67, 178, 252, 32, 68, 108, 196, 159, 57, 67, 23, 251, 32, 68, 108, 102, 222, 57, 67, 179, 248, 32, 68, 108, 121, 28, 58, 67, 135, 245, 32, 68, 108, 215, 89, 58, 67, 149, 241, 32,
		68, 108, 87, 150, 58, 67, 225, 236, 32, 68, 108, 211, 209, 58, 67, 108, 231, 32, 68, 108, 36, 12, 59, 67, 59, 225, 32, 68, 108, 38, 69, 59, 67, 82, 218, 32, 68, 108, 181, 124, 59, 67, 180, 210, 32, 68, 108, 171, 178, 59, 67, 103, 202, 32, 68, 108, 232, 230, 59, 67, 112, 193, 32, 68, 108, 73, 25, 60, 67, 213, 183, 32, 68,
		108, 174, 73, 60, 67, 156, 173, 32, 68, 108, 248, 119, 60, 67, 203, 162, 32, 68, 108, 11, 164, 60, 67, 106, 151, 32, 68, 108, 200, 205, 60, 67, 127, 139, 32, 68, 108, 23, 245, 60, 67, 19, 127, 32, 68, 108, 221, 25, 61, 67, 44, 114, 32, 68, 108, 3, 60, 61, 67, 213, 100, 32, 68, 108, 115, 91, 61, 67, 20, 87, 32, 68, 108, 26,
		120, 61, 67, 243, 72, 32, 68, 108, 228, 145, 61, 67, 124, 58, 32, 68, 108, 194, 168, 61, 67, 182, 43, 32, 68, 108, 164, 188, 61, 67, 172, 28, 32, 68, 108, 126, 205, 61, 67, 103, 13, 32, 68, 108, 69, 219, 61, 67, 241, 253, 31, 68, 108, 241, 229, 61, 67, 84, 238, 31, 68, 108, 122, 237, 61, 67, 153, 222, 31, 68, 108, 220, 241,
		61, 67, 204, 206, 31, 68, 108, 25, 243, 61, 67, 202, 192, 31, 68, 108, 25, 243, 61, 67, 202, 192, 31, 68, 108, 25, 243, 61, 67, 106, 237, 20, 68, 108, 25, 243, 61, 67, 106, 237, 20, 68, 108, 127, 241, 61, 67, 148, 221, 20, 68, 108, 188, 236, 61, 67, 200, 205, 20, 68, 108, 209, 228, 61, 67, 17, 190, 20, 68, 108, 197, 217,
		61, 67, 120, 174, 20, 68, 108, 158, 203, 61, 67, 8, 159, 20, 68, 108, 102, 186, 61, 67, 201, 143, 20, 68, 108, 39, 166, 61, 67, 199, 128, 20, 68, 108, 238, 142, 61, 67, 10, 114, 20, 68, 108, 203, 116, 61, 67, 156, 99, 20, 68, 108, 205, 87, 61, 67, 134, 85, 20, 68, 108, 8, 56, 61, 67, 210, 71, 20, 68, 108, 143, 21, 61, 67,
		136, 58, 20, 68, 108, 122, 240, 60, 67, 176, 45, 20, 68, 108, 222, 200, 60, 67, 83, 33, 20, 68, 108, 215, 158, 60, 67, 120, 21, 20, 68, 108, 127, 114, 60, 67, 40, 10, 20, 68, 108, 242, 67, 60, 67, 105, 255, 19, 68, 108, 77, 19, 60, 67, 67, 245, 19, 68, 108, 177, 224, 59, 67, 187, 235, 19, 68, 108, 62, 172, 59, 67, 216, 226,
		19, 68, 108, 20, 118, 59, 67, 160, 218, 19, 68, 108, 87, 62, 59, 67, 24, 211, 19, 68, 108, 42, 5, 59, 67, 68, 204, 19, 68, 108, 179, 202, 58, 67, 42, 198, 19, 68, 108, 21, 143, 58, 67, 204, 192, 19, 68, 108, 120, 82, 58, 67, 47, 188, 19, 68, 108, 3, 21, 58, 67, 86, 184, 19, 68, 108, 220, 214, 57, 67, 66, 181, 19, 68, 108, 43,
		152, 57, 67, 246, 178, 19, 68, 108, 25, 89, 57, 67, 115, 177, 19, 68, 108, 206, 25, 57, 67, 186, 176, 19, 68, 108, 114, 218, 56, 67, 204, 176, 19, 68, 108, 46, 155, 56, 67, 169, 177, 19, 68, 108, 42, 92, 56, 67, 80, 179, 19, 68, 108, 143, 29, 56, 67, 192, 181, 19, 68, 108, 133, 223, 55, 67, 247, 184, 19, 68, 108, 51, 162,
		55, 67, 244, 188, 19, 68, 108, 193, 101, 55, 67, 179, 193, 19, 68, 108, 85, 42, 55, 67, 51, 199, 19, 68, 108, 22, 240, 54, 67, 110, 205, 19, 68, 108, 40, 183, 54, 67, 98, 212, 19, 68, 108, 153, 134, 54, 67, 10, 219, 19, 68, 108, 153, 134, 54, 67, 10, 219, 19, 68, 108, 153, 6, 17, 67, 202, 68, 25, 68, 108, 153, 6, 17, 67, 202,
		68, 25, 68, 108, 137, 208, 16, 67, 13, 77, 25, 68, 108, 50, 156, 16, 67, 250, 85, 25, 68, 108, 180, 105, 16, 67, 140, 95, 25, 68, 108, 48, 57, 16, 67, 188, 105, 25, 68, 108, 198, 10, 16, 67, 132, 116, 25, 68, 108, 145, 222, 15, 67, 221, 127, 25, 68, 108, 176, 180, 15, 67, 192, 139, 25, 68, 108, 60, 141, 15, 67, 37, 152, 25,
		68, 108, 80, 104, 15, 67, 4, 165, 25, 68, 108, 2, 70, 15, 67, 85, 178, 25, 68, 108, 104, 38, 15, 67, 16, 192, 25, 68, 108, 151, 9, 15, 67, 44, 206, 25, 68, 108, 162, 239, 14, 67, 159, 220, 25, 68, 108, 152, 216, 14, 67, 96, 235, 25, 68, 108, 137, 196, 14, 67, 102, 250, 25, 68, 108, 129, 179, 14, 67, 168, 9, 26, 68, 108, 139,
		165, 14, 67, 28, 25, 26, 68, 108, 177, 154, 14, 67, 183, 40, 26, 68, 108, 249, 146, 14, 67, 112, 56, 26, 68, 108, 104, 142, 14, 67, 60, 72, 26, 68, 108, 1, 141, 14, 67, 18, 88, 26, 68, 108, 197, 142, 14, 67, 231, 103, 26, 68, 108, 178, 147, 14, 67, 178, 119, 26, 68, 108, 198, 155, 14, 67, 104, 135, 26, 68, 108, 252, 166,
		14, 67, 255, 150, 26, 68, 108, 76, 181, 14, 67, 109, 166, 26, 68, 108, 173, 198, 14, 67, 169, 181, 26, 68, 108, 20, 219, 14, 67, 168, 196, 26, 68, 108, 117, 242, 14, 67, 97, 211, 26, 68, 108, 191, 12, 15, 67, 202, 225, 26, 68, 108, 226, 41, 15, 67, 219, 239, 26, 68, 108, 204, 73, 15, 67, 138, 253, 26, 68, 108, 104, 108,
		15, 67, 207, 10, 27, 68, 108, 160, 145, 15, 67, 160, 23, 27, 68, 108, 92, 185, 15, 67, 247, 35, 27, 68, 108, 131, 227, 15, 67, 202, 47, 27, 68, 108, 249, 15, 16, 67, 19, 59, 27, 68, 108, 163, 62, 16, 67, 202, 69, 27, 68, 108, 98, 111, 16, 67, 233, 79, 27, 68, 108, 24, 162, 16, 67, 104, 89, 27, 68, 108, 163, 214, 16, 67, 66,
		98, 27, 68, 108, 153, 6, 17, 67, 138, 105, 27, 68, 108, 153, 6, 17, 67, 138, 105, 27, 68, 108, 153, 134, 54, 67, 42, 211, 32, 68, 108, 153, 134, 54, 67, 42, 211, 32, 68, 108, 62, 190, 54, 67, 189, 218, 32, 68, 108, 84, 247, 54, 67, 156, 225, 32, 68, 108, 184, 49, 55, 67, 195, 231, 32, 68, 108, 68, 109, 55, 67, 44, 237, 32,
		68, 108, 209, 169, 55, 67, 214, 241, 32, 68, 108, 58, 231, 55, 67, 188, 245, 32, 68, 108, 87, 37, 56, 67, 220, 248, 32, 68, 108, 0, 100, 56, 67, 53, 251, 32, 68, 108, 13, 163, 56, 67, 197, 252, 32, 68, 108, 25, 226, 56, 67, 138, 253, 32, 68, 108, 25, 226, 56, 67, 138, 253, 32, 68, 99, 101, 0, 0 };


	static const unsigned char forwardPathData[] = { 110, 109, 0, 158, 19, 67, 192, 176, 19, 68, 108, 0, 158, 19, 67, 192, 176, 19, 68, 108, 164, 94, 19, 67, 199, 176, 19, 68, 108, 94, 31, 19, 67, 152, 177, 19, 68, 108, 85, 224, 18, 67, 51, 179, 19, 68, 108, 179, 161, 18, 67, 151, 181, 19, 68, 108, 160, 99, 18, 67, 195, 184, 19, 68, 108, 66, 38, 18, 67, 181, 188, 19, 68, 108,
		194, 233, 17, 67, 105, 193, 19, 68, 108, 70, 174, 17, 67, 222, 198, 19, 68, 108, 244, 115, 17, 67, 15, 205, 19, 68, 108, 242, 58, 17, 67, 248, 211, 19, 68, 108, 100, 3, 17, 67, 150, 219, 19, 68, 108, 110, 205, 16, 67, 227, 227, 19, 68, 108, 49, 153, 16, 67, 218, 236, 19, 68, 108, 208, 102, 16, 67, 117, 246, 19, 68, 108,
		107, 54, 16, 67, 174, 0, 20, 68, 108, 32, 8, 16, 67, 127, 11, 20, 68, 108, 14, 220, 15, 67, 224, 22, 20, 68, 108, 80, 178, 15, 67, 203, 34, 20, 68, 108, 2, 139, 15, 67, 56, 47, 20, 68, 108, 60, 102, 15, 67, 30, 60, 20, 68, 108, 22, 68, 15, 67, 117, 73, 20, 68, 108, 166, 36, 15, 67, 54, 87, 20, 68, 108, 255, 7, 15, 67, 87, 101,
		20, 68, 108, 53, 238, 14, 67, 207, 115, 20, 68, 108, 87, 215, 14, 67, 148, 130, 20, 68, 108, 117, 195, 14, 67, 159, 145, 20, 68, 108, 155, 178, 14, 67, 227, 160, 20, 68, 108, 212, 164, 14, 67, 89, 176, 20, 68, 108, 40, 154, 14, 67, 247, 191, 20, 68, 108, 159, 146, 14, 67, 177, 207, 20, 68, 108, 61, 142, 14, 67, 126, 223,
		20, 68, 108, 0, 141, 14, 67, 128, 237, 20, 68, 108, 0, 141, 14, 67, 128, 237, 20, 68, 108, 0, 141, 14, 67, 224, 192, 31, 68, 108, 0, 141, 14, 67, 224, 192, 31, 68, 108, 154, 142, 14, 67, 182, 208, 31, 68, 108, 93, 147, 14, 67, 130, 224, 31, 68, 108, 72, 155, 14, 67, 57, 240, 31, 68, 108, 84, 166, 14, 67, 210, 255, 31, 68,
		108, 123, 180, 14, 67, 67, 15, 32, 68, 108, 179, 197, 14, 67, 129, 30, 32, 68, 108, 242, 217, 14, 67, 131, 45, 32, 68, 108, 43, 241, 14, 67, 64, 60, 32, 68, 108, 79, 11, 15, 67, 174, 74, 32, 68, 108, 76, 40, 15, 67, 196, 88, 32, 68, 108, 18, 72, 15, 67, 120, 102, 32, 68, 108, 138, 106, 15, 67, 195, 115, 32, 68, 108, 160, 143,
		15, 67, 154, 128, 32, 68, 108, 59, 183, 15, 67, 248, 140, 32, 68, 108, 66, 225, 15, 67, 210, 152, 32, 68, 108, 155, 13, 16, 67, 34, 164, 32, 68, 108, 40, 60, 16, 67, 225, 174, 32, 68, 108, 204, 108, 16, 67, 8, 185, 32, 68, 108, 104, 159, 16, 67, 143, 194, 32, 68, 108, 220, 211, 16, 67, 114, 203, 32, 68, 108, 6, 10, 17, 67,
		170, 211, 32, 68, 108, 195, 65, 17, 67, 50, 219, 32, 68, 108, 239, 122, 17, 67, 6, 226, 32, 68, 108, 103, 181, 17, 67, 32, 232, 32, 68, 108, 5, 241, 17, 67, 126, 237, 32, 68, 108, 161, 45, 18, 67, 27, 242, 32, 68, 108, 23, 107, 18, 67, 244, 245, 32, 68, 108, 62, 169, 18, 67, 8, 249, 32, 68, 108, 238, 231, 18, 67, 84, 251,
		32, 68, 108, 1, 39, 19, 67, 215, 252, 32, 68, 108, 76, 102, 19, 67, 144, 253, 32, 68, 108, 168, 165, 19, 67, 126, 253, 32, 68, 108, 236, 228, 19, 67, 161, 252, 32, 68, 108, 239, 35, 20, 67, 250, 250, 32, 68, 108, 138, 98, 20, 67, 138, 248, 32, 68, 108, 149, 160, 20, 67, 83, 245, 32, 68, 108, 230, 221, 20, 67, 86, 241, 32,
		68, 108, 89, 26, 21, 67, 151, 236, 32, 68, 108, 196, 85, 21, 67, 23, 231, 32, 68, 108, 4, 144, 21, 67, 220, 224, 32, 68, 108, 241, 200, 21, 67, 232, 217, 32, 68, 108, 128, 249, 21, 67, 64, 211, 32, 68, 108, 128, 249, 21, 67, 64, 211, 32, 68, 108, 128, 121, 59, 67, 128, 105, 27, 68, 108, 128, 121, 59, 67, 128, 105, 27, 68,
		108, 144, 175, 59, 67, 61, 97, 27, 68, 108, 231, 227, 59, 67, 80, 88, 27, 68, 108, 101, 22, 60, 67, 190, 78, 27, 68, 108, 232, 70, 60, 67, 142, 68, 27, 68, 108, 83, 117, 60, 67, 198, 57, 27, 68, 108, 136, 161, 60, 67, 109, 46, 27, 68, 108, 105, 203, 60, 67, 138, 34, 27, 68, 108, 221, 242, 60, 67, 37, 22, 27, 68, 108, 201,
		23, 61, 67, 70, 9, 27, 68, 108, 23, 58, 61, 67, 245, 251, 26, 68, 108, 177, 89, 61, 67, 58, 238, 26, 68, 108, 130, 118, 61, 67, 31, 224, 26, 68, 108, 119, 144, 61, 67, 171, 209, 26, 68, 108, 129, 167, 61, 67, 234, 194, 26, 68, 108, 144, 187, 61, 67, 228, 179, 26, 68, 108, 152, 204, 61, 67, 162, 164, 26, 68, 108, 142, 218,
		61, 67, 46, 149, 26, 68, 108, 104, 229, 61, 67, 147, 133, 26, 68, 108, 32, 237, 61, 67, 219, 117, 26, 68, 108, 177, 241, 61, 67, 14, 102, 26, 68, 108, 24, 243, 61, 67, 56, 86, 26, 68, 108, 84, 241, 61, 67, 99, 70, 26, 68, 108, 103, 236, 61, 67, 152, 54, 26, 68, 108, 83, 228, 61, 67, 226, 38, 26, 68, 108, 29, 217, 61, 67, 75,
		23, 26, 68, 108, 205, 202, 61, 67, 221, 7, 26, 68, 108, 108, 185, 61, 67, 161, 248, 25, 68, 108, 5, 165, 61, 67, 162, 233, 25, 68, 108, 165, 141, 61, 67, 233, 218, 25, 68, 108, 90, 115, 61, 67, 128, 204, 25, 68, 108, 55, 86, 61, 67, 111, 190, 25, 68, 108, 77, 54, 61, 67, 192, 176, 25, 68, 108, 177, 19, 61, 67, 123, 163, 25,
		68, 108, 121, 238, 60, 67, 170, 150, 25, 68, 108, 189, 198, 60, 67, 83, 138, 25, 68, 108, 151, 156, 60, 67, 128, 126, 25, 68, 108, 32, 112, 60, 67, 55, 115, 25, 68, 108, 118, 65, 60, 67, 128, 104, 25, 68, 108, 183, 16, 60, 67, 97, 94, 25, 68, 108, 1, 222, 59, 67, 226, 84, 25, 68, 108, 118, 169, 59, 67, 8, 76, 25, 68, 108,
		128, 121, 59, 67, 192, 68, 25, 68, 108, 128, 121, 59, 67, 192, 68, 25, 68, 108, 128, 249, 21, 67, 32, 219, 19, 68, 108, 128, 249, 21, 67, 32, 219, 19, 68, 108, 219, 193, 21, 67, 141, 211, 19, 68, 108, 197, 136, 21, 67, 174, 204, 19, 68, 108, 97, 78, 21, 67, 135, 198, 19, 68, 108, 213, 18, 21, 67, 30, 193, 19, 68, 108, 72,
		214, 20, 67, 116, 188, 19, 68, 108, 223, 152, 20, 67, 142, 184, 19, 68, 108, 194, 90, 20, 67, 110, 181, 19, 68, 108, 25, 28, 20, 67, 21, 179, 19, 68, 108, 11, 221, 19, 67, 133, 177, 19, 68, 108, 0, 158, 19, 67, 192, 176, 19, 68, 108, 0, 158, 19, 67, 192, 176, 19, 68, 99, 101, 0, 0 };

	static const unsigned char savePathData[] = { 110, 109, 0, 0, 144, 65, 0, 0, 0, 64, 108, 0, 0, 176, 65, 0, 0, 0, 64, 108, 0, 0, 176, 65, 0, 0, 48, 65, 108, 0, 0, 144, 65, 0, 0, 48, 65, 99, 109, 0, 0, 192, 64, 0, 0, 208, 65, 108, 0, 0, 208, 65, 0, 0, 208, 65, 108, 0, 0, 208, 65, 0, 0, 224, 65, 108, 0, 0, 192, 64, 0, 0, 224, 65, 99, 109, 0, 0, 192, 64, 0, 0, 176, 65, 108, 0, 0, 208,
		65, 0, 0, 176, 65, 108, 0, 0, 208, 65, 0, 0, 192, 65, 108, 0, 0, 192, 64, 0, 0, 192, 65, 99, 109, 0, 0, 192, 64, 0, 0, 144, 65, 108, 0, 0, 208, 65, 0, 0, 144, 65, 108, 0, 0, 208, 65, 0, 0, 160, 65, 108, 0, 0, 192, 64, 0, 0, 160, 65, 99, 109, 0, 0, 208, 65, 0, 0, 0, 0, 108, 0, 0, 192, 65, 0, 0, 0, 0, 108, 0, 0, 192, 65, 0, 0, 80, 65, 108,
		0, 0, 0, 65, 0, 0, 80, 65, 108, 0, 0, 0, 65, 0, 0, 0, 0, 108, 0, 0, 0, 0, 0, 0, 0, 0, 108, 0, 0, 0, 0, 0, 0, 0, 66, 108, 0, 0, 0, 66, 0, 0, 0, 66, 108, 0, 0, 0, 66, 0, 0, 192, 64, 108, 0, 0, 208, 65, 0, 0, 0, 0, 99, 109, 0, 0, 224, 65, 0, 0, 240, 65, 108, 0, 0, 128, 64, 0, 0, 240, 65, 108, 0, 0, 128, 64, 0, 0, 128, 65, 108, 0, 0, 224, 65, 0, 0,
		128, 65, 108, 0, 0, 224, 65, 0, 0, 240, 65, 99, 101, 0, 0 };

	sp.loadPathFromData(savePathData, sizeof(savePathData));
	bp.loadPathFromData(backPathData, sizeof(backPathData));
	fp.loadPathFromData(forwardPathData, sizeof(forwardPathData));

	previousButton->setShape(bp, true, true, false);
	nextButton->setShape(fp, true, true, false);
	presetSaveButton->setShape(sp, true, true, false);

	presetSaveButton->addListener(this);
	previousButton->addListener(this);
	nextButton->addListener(this);

	presetNameLabel->setFont(GLOBAL_BOLD_FONT());
	presetNameLabel->setEditable(false, false);
	presetNameLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	setTooltip("Current Preset. Click to open Preset Browser");
	previousButton->setTooltip("Load previous preset");
	nextButton->setTooltip("Load next preset");
	presetSaveButton->setTooltip("Save user preset");

	int u1, u2;
	String name;

	data->getCurrentPresetIndexes(u1, u2, name);

	presetNameLabel->setText(name, dontSendNotification);
	presetNameLabel->setInterceptsMouseClicks(false, false);
}


PresetBox::~PresetBox()
{
	data->removeListener(this);
	data = nullptr;
}



void PresetBox::resized()
{
	int s = 16;
	int y = (getHeight() - s) / 2;

	presetNameLabel->setBounds(0, 0, getWidth() - 3 * s - 9, getHeight());
	previousButton->setBounds(presetNameLabel->getRight() + 3, y, s, s);
	nextButton->setBounds(previousButton->getRight() + 3, y, s, s);
	presetSaveButton->setBounds(nextButton->getRight() + 3, y, s, s);
}

void PresetBox::mouseDown(const MouseEvent& /*event*/)
{
	ModalBaseWindow* mbw = findParentComponentOfClass<ModalBaseWindow>();

	if (mbw->isCurrentlyModal())
	{
		mbw->clearModalComponent();
	}
	else
	{
		auto r = dynamic_cast<const Component*>(mbw)->getBounds();

		MultiColumnPresetBrowser* pr = new MultiColumnPresetBrowser(mc, r.getWidth() - 80, r.getHeight()-80);
		
		pr->setShowCloseButton(true);
        
        Colour c2 = Colours::black.withAlpha(0.8f);
        Colour c = Colour(SIGNAL_COLOUR);
        
		pr->setHighlightColourAndFont(c, c2, GLOBAL_BOLD_FONT());
		pr->setModalBaseWindowComponent(this, 300);
	}
}

void PresetBox::buttonClicked(Button *b)
{
	if (b == nextButton)
	{
		
	}
	else if (b == previousButton)
	{
		
	}
	else if (b == presetSaveButton)
	{
		UserPresetHelpers::saveUserPreset(mc->getMainSynthChain());

		mc->rebuildUserPresetDatabase();
	}
}

void PresetBox::presetLoaded(int /*categoryIndex*/, int /*presetIndex*/, const String &presetName)
{
	presetNameLabel->setText(presetName, dontSendNotification);
}
