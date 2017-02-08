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
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licensed for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

void PresetBrowserColumn::ButtonLookAndFeel::drawButtonBackground(Graphics& /*g*/, Button& /*button*/, const Colour& /*backgroundColour*/, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{

}

void PresetBrowserColumn::ButtonLookAndFeel::drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{
	g.setColour(highlightColour);

    g.setFont(font.withHeight(18.0f));
    
	g.drawText(button.getButtonText(), 0, 0, button.getWidth(), button.getHeight(), Justification::centred);
}

PresetBrowserColumn::ColumnListModel::ColumnListModel(int index_, Listener* listener_) :
root(File::nonexistent),
index(index_),
listener(listener_)
{
	deleteIcon = Image(); //ImageCache::getFromMemory(CustomFrontendToolbarImages::deleteIcon_png, sizeof(CustomFrontendToolbarImages::deleteIcon_png));
	
	
}

int PresetBrowserColumn::ColumnListModel::getNumRows()
{
    if(wildcard.isEmpty())
    {   
        if (!root.isDirectory())
	    {
		    entries.clear();
		    return 0;
	    }

	    entries.clear();
	    root.findChildFiles(entries, displayDirectories ? File::findDirectories : File::findFiles, false);

		for (int i = 0; i < entries.size(); i++)
		{
			if (entries[i].isHidden() || entries[i].getFileName().startsWith("."))
			{
				entries.remove(i--);
			}
		}
			
	    entries.sort();

	    return entries.size();
    }
    else
    {
        Array<File> allFiles;
        
        totalRoot.findChildFiles(allFiles, File::findFiles, true);
        
        entries.clear();
        
        for(int i = 0; i < allFiles.size(); i++)
        {
            if(allFiles[i].getFullPathName().contains(wildcard))
                entries.add(allFiles[i]);
        }
        
		for (int i = 0; i < entries.size(); i++)
		{
			if (entries[i].isHidden() || entries[i].getFileName().startsWith("."))
			{
				entries.remove(i--);
			}
		}

        entries.sort();
        
        return entries.size();
    }
}

void PresetBrowserColumn::ColumnListModel::listBoxItemClicked(int row, const MouseEvent &e)
{
	if (editMode)
	{
		const String title = index != 2 ? "Directory" : "Preset";

		if (e.getMouseDownX() < 40)
		{
			if (PresetHandler::showYesNoWindow("Delete " + title, "Do you want to delete this " + title + "?", PresetHandler::IconType::Question))
			{
				if (listener != nullptr)
				{
					listener->deleteEntry(index, row, entries[row], false);
				}
			}
		}
		else
		{
			listener->renameEntry(index, row, entries[row], false);
		}
	}
	else
	{
		if (listener != nullptr)
		{
			listener->selectionChanged(index, row, entries[row], false);
		}
	}
}


void PresetBrowserColumn::ColumnListModel::returnKeyPressed(int row)
{
	if (listener != nullptr)
	{
		listener->selectionChanged(index, row, entries[row], false);
	}
}


void PresetBrowserColumn::ColumnListModel::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < entries.size())
	{
		Rectangle<int> area(0, 1, width, height - 2);

		
		g.setColour(rowIsSelected ? highlightColour.withAlpha(0.3f) : Colour(0x00222222));
		g.fillRect(area);
		g.setColour(Colours::white.withAlpha(0.4f));
		if(rowIsSelected) g.drawRect(area, 1);

		if (editMode)
		{
			g.setColour(Colours::red);

			const float widthOfDeleteCircle = ((float)height - 10.0f);

			g.fillEllipse(5.0f, 5.0f, widthOfDeleteCircle, widthOfDeleteCircle);

			g.setColour(Colours::white);

			g.fillRect(5.0f + widthOfDeleteCircle * 0.5f - 6.0f, 5.0f + widthOfDeleteCircle * 0.5f - 1.0f, 12.0f, 2.0f);

			g.drawText(entries[rowNumber].getFileNameWithoutExtension(), 30, 0, width-20, height, Justification::centredLeft);
		}
		else
		{
			g.setColour(Colours::white);
			g.setFont(font.withHeight(16.0f));
			g.drawText(entries[rowNumber].getFileNameWithoutExtension(), 10, 0, width-20, height, Justification::centredLeft);
		}
	}
}

PresetBrowserColumn::PresetBrowserColumn(MainController* mc_, int index_, File& rootDirectory, ColumnListModel::Listener* listener) :
mc(mc_),
index(index_)
{
	addAndMakeVisible(editButton = new TextButton("Edit"));
	addAndMakeVisible(addButton = new ShapeButton("Add", Colours::white, Colours::white, Colours::white));

	Path addShape;

	addShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));

	addButton->setShape(addShape, true, true, false);

	editButton->setLookAndFeel(&blaf);
	addButton->setLookAndFeel(&blaf);

	addButton->addListener(this);
	editButton->addListener(this);

	listModel = new ColumnListModel(index, listener);

	
	listModel->setTotalRoot(rootDirectory);
	
	startTimer(4000);
	
	if (index == 2)
	{
		listModel->setDisplayDirectories(false);
	}

	addAndMakeVisible(listbox = new ListBox());

	listbox->setModel(listModel);
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.15f));
	listbox->setRowHeight(30);
	listbox->setWantsKeyboardFocus(true);
	
	listbox->getViewport()->setScrollOnDragEnabled(true);
	
	setSize(150, 300);
    
    browser = dynamic_cast<MultiColumnPresetBrowser*>(listener);
}

File PresetBrowserColumn::getChildDirectory(File& root, int level, int index)
{
	if (!root.isDirectory()) return File::nonexistent;

	if (level == 0) return root;

	Array<File> childDirectories;

	root.findChildFiles(childDirectories, level > 2 ? File::findFiles : File::findDirectories, false);

    for(int i = 0; i < childDirectories.size(); i++)
    {
        if(childDirectories[i].isHidden() ||
           (!childDirectories[i].isDirectory() && childDirectories[i].getFileExtension() != ".preset"))
        {
            childDirectories.remove(i--);
        }
    }
    
    childDirectories.sort();

	return childDirectories[index];
}

void PresetBrowserColumn::setNewRootDirectory(const File& newRootDirectory)
{
	currentRoot = newRootDirectory;

	listModel->setRootDirectory(newRootDirectory);
	listbox->deselectAllRows();
	listbox->updateContent();
	listbox->repaint();
}

void PresetBrowserColumn::buttonClicked(Button* b)
{
	if (b == editButton)
	{
		listModel->toggleEditMode();
		listbox->repaint();
	}
	else if (b == addButton)
	{
		if (!currentRoot.isDirectory()) return;

		if (index != 2)
		{
			const String newDirectoryName = PresetHandler::getCustomName("Directory");
			File newDirectory = currentRoot.getChildFile(newDirectoryName);
			newDirectory.createDirectory();

			setNewRootDirectory(currentRoot);
            
            
		}
		else
		{
			const String newPresetName = PresetHandler::getCustomName("Preset");

			if (newPresetName.isNotEmpty())
			{
				File newPreset = currentRoot.getChildFile(newPresetName + ".preset");
				UserPresetHandler::saveUserPreset(mc->getMainSynthChain(), newPreset.getFullPathName());
                
				setNewRootDirectory(currentRoot);
                
                browser->rebuildAllPresets();
			}
		}
	}
}

void PresetBrowserColumn::paint(Graphics& g)
{
	Rectangle<int> textArea(0, 0, getWidth(), 40);


	

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(0, 40, getWidth(), getHeight() - 40, 1);

    g.setColour(Colours::white.withAlpha(0.02f));
    
    g.fillRect(textArea);
    
	g.setColour(Colours::white);

	String name;

	if (isResultBar) name = "Search results";
	else if (index == 0) name = "Bank";
	else if (index == 1) name = "Category";
	else name = "Preset";

    g.setFont(font.withHeight(16.0f));
	g.drawText(name, textArea, Justification::centred);

	if (currentRoot == File())
	{
		g.setFont(font);
		g.setColour(Colours::white.withAlpha(0.3f));
		g.drawText("Select a " + String(index == 1 ? "Bank" : "Category"), 0, 0, getWidth(), getHeight(), Justification::centred);
	}

	else if (listModel->getNumRows() == 0)
	{
		g.setFont(font);
		g.setColour(Colours::white.withAlpha(0.3f));
		g.drawText("Add a " + name, 0, 0, getWidth(), getHeight(), Justification::centred);
	}

}

void PresetBrowserColumn::resized()
{
	editButton->setBounds(0, 7, 40, 26);
	addButton->setBounds(getWidth() - 30, 10, 20, 20);
	listbox->setBounds(0, 40, getWidth(), getHeight() - 40);
}

MultiColumnPresetBrowser::MultiColumnPresetBrowser(MainController* mc_, int width, int height) :
mc(mc_)
{
	

#if USE_BACKEND
	rootFile = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
    rootFile = ProjectHandler::Frontend::getUserPresetDirectory();
#endif

	addAndMakeVisible(bankColumn = new PresetBrowserColumn(mc, 0, rootFile, this));
	addAndMakeVisible(categoryColumn = new PresetBrowserColumn(mc, 1, rootFile, this));
	addAndMakeVisible(presetColumn = new PresetBrowserColumn(mc, 2, rootFile, this));
	addAndMakeVisible(searchBar = new PresetBrowserSearchBar());

	addChildComponent(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	closeButton->addListener(this);

	Path closeShape;
	closeShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	closeButton->setShape(closeShape, true, true, true);

	searchBar->inputLabel->addListener(this);
	searchBar->inputLabel->addListener(presetColumn);
	
	bankColumn->setNewRootDirectory(rootFile);

	setSize(width, height);

	rebuildAllPresets();
}

MultiColumnPresetBrowser::~MultiColumnPresetBrowser()
{
	searchBar->inputLabel->removeListener(this);
	searchBar->inputLabel->removeListener(presetColumn);

	closeButton->removeListener(this);

	searchBar = nullptr;
	bankColumn = nullptr;
	categoryColumn = nullptr;
	presetColumn = nullptr;
}

void MultiColumnPresetBrowser::paint(Graphics& g)
{
	g.fillAll(backgroundColour);


	if (closeButton->isVisible())
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(0, 0, getWidth(), 30);
		g.setFont(GLOBAL_BOLD_FONT().withHeight(20.0f));
		g.setColour(Colours::white);
		g.drawText("Preset Browser", 0, 0, getWidth(), 30, Justification::centred, false);
	}
}

void MultiColumnPresetBrowser::rebuildAllPresets()
{
	allPresets.clear();
	rootFile.findChildFiles(allPresets, File::findFiles, true, "*.preset");

	for (int i = 0; i < allPresets.size(); i++)
	{
		if (allPresets[i].isHidden() || allPresets[i].getFileName().startsWith("."))
		{
			allPresets.remove(i--);
		}
	}

}

String MultiColumnPresetBrowser::getCurrentlyLoadedPresetName()
{
	if (currentlyLoadedPreset > 0 && currentlyLoadedPreset < allPresets.size())
	{
		return allPresets[currentlyLoadedPreset].getFileNameWithoutExtension();
	}

	return String();
}

void MultiColumnPresetBrowser::resized()
{
	int y = 0;

	const bool showCloseButton = closeButton->isVisible();

	if (showCloseButton)
	{

#if HISE_IOS
		closeButton->setBounds(getWidth() - 30, 6, 24, 24);
#else
		closeButton->setBounds(getWidth() - 35, 5, 20, 20);
#endif

		y += 30;
		searchBar->setBounds(3, y + 5, getWidth() - 6, 30);
		y += 40;

	}
	else
	{
		searchBar->setBounds(3, 3 + 3, getWidth() - 6, 30);
		y += 40;
	}
	
	int x = 3;

	bankColumn->setVisible(!showOnlyPresets);
	categoryColumn->setVisible(!showOnlyPresets);
	presetColumn->setIsResultBar(showOnlyPresets);

	if (showOnlyPresets)
	{
		presetColumn->setBounds(x, y, getWidth() - 6, getHeight() - y - 3);
	}
	else
	{
		bankColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
		x += getWidth() / 3;
		categoryColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
		x += getWidth() / 3;
		presetColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
	}

	
}

void MultiColumnPresetBrowser::selectionChanged(int columnIndex, int /*rowIndex*/, const File& file, bool /*doubleClick*/)
{
	if (columnIndex == 0)
	{
		currentBankFile = file;

		categoryColumn->setNewRootDirectory(currentBankFile);
		presetColumn->setNewRootDirectory(File::nonexistent);
        
        categoryColumn->setEditMode(false);
        presetColumn->setEditMode(false);
	}
	else if (columnIndex == 1)
	{
		currentCategoryFile = file;

		presetColumn->setNewRootDirectory(currentCategoryFile);
        
        presetColumn->setEditMode(false);
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

        bankColumn->setEditMode(false);
        
	}
	else if (columnIndex == 2)
	{
		loadPreset(file);

		bankColumn->setEditMode(false);
		categoryColumn->setEditMode(false);
	}
}


void MultiColumnPresetBrowser::renameEntry(int columnIndex, int rowIndex, const File& f, bool /*doubleClick*/)
{
	if (columnIndex == 0)
	{
		File bankToDelete = f;

		categoryColumn->setNewRootDirectory(File::nonexistent);
		presetColumn->setNewRootDirectory(File::nonexistent);
	}
	else if (columnIndex == 1)
	{
		currentCategoryFile = PresetBrowserColumn::getChildDirectory(currentBankFile, 2, rowIndex);

		presetColumn->setNewRootDirectory(currentCategoryFile);
	}
	else if (columnIndex == 2)
	{
		File presetFile = PresetBrowserColumn::getChildDirectory(currentCategoryFile, 3, rowIndex);

		const String customName = PresetHandler::getCustomName(presetFile.getFileNameWithoutExtension(), "Enter the new Preset Name");

		if (customName.isNotEmpty())
		{
			File newFile = presetFile.getSiblingFile(customName + ".preset");

			presetFile.moveFileTo(newFile);
			presetColumn->setNewRootDirectory(currentCategoryFile);
		}

		rebuildAllPresets();
	}
}

void MultiColumnPresetBrowser::deleteEntry(int columnIndex, int /*rowIndex*/, const File& f, bool /*doubleClick*/)
{
	if (columnIndex == 0)
	{
		File bankToDelete = f;

		bankToDelete.deleteRecursively();

		bankColumn->setNewRootDirectory(rootFile);
		categoryColumn->setNewRootDirectory(File::nonexistent);
		presetColumn->setNewRootDirectory(File::nonexistent);
	}
	else if (columnIndex == 1)
	{
		File categoryToDelete = f;

		categoryToDelete.deleteRecursively();

		categoryColumn->setNewRootDirectory(currentBankFile);
		presetColumn->setNewRootDirectory(File::nonexistent);
	}
	else if (columnIndex == 2)
	{
		File presetFile = f;

		

		presetFile.deleteFile();
		presetColumn->setNewRootDirectory(currentCategoryFile);
	}

	rebuildAllPresets();
}


void MultiColumnPresetBrowser::loadPreset(const File& f)
{
    if(f.existsAsFile())
    {
        UserPresetHandler::loadUserPreset(mc->getMainSynthChain(), f);
        currentlyLoadedPreset = allPresets.indexOf(f);
        
        if (listener != nullptr)
        {
            listener->presetChanged(f.getFileNameWithoutExtension());
        }
    }
}

