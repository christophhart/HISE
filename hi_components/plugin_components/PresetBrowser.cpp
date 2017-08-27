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
root(File()),
index(index_),
listener(listener_)
{
	deleteIcon = Image(); //ImageCache::getFromMemory(CustomFrontendToolbarImages::deleteIcon_png, sizeof(CustomFrontendToolbarImages::deleteIcon_png));
	
	
}

void MultiColumnPresetBrowser::ModalWindow::paint(Graphics& g)
{
    g.setColour(Colour(0xcc222222));
    g.fillAll();
    
    auto area = inputLabel->getBounds().expanded(50);
    
    g.setColour(Colour(0xcc333333));
    g.fillRect(area.expanded(40));
    
    g.setColour(Colour(0x33FFFFFF));
    g.drawRect(area.expanded(40), 1);
    
    g.setColour(Colour(0x22000000));
    
    if(inputLabel->isVisible())
        g.fillRect(inputLabel->getBounds());
    
    g.setColour(Colours::white);
    g.setFont(f.boldened().withHeight(24));
    g.drawText(getTitleText(), 0, inputLabel->getY() - 80, getWidth(), 30, Justification::centredTop);
        
    g.setFont(f.boldened());
    
    g.drawText(getCommand(), area, Justification::centredTop);
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
            if(allFiles[i].getFullPathName().containsIgnoreCase(wildcard))
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

		auto name = entries[row].getFileNameWithoutExtension();

		auto pb = dynamic_cast<MultiColumnPresetBrowser*>(listener);

		if (pb == nullptr)
			return;

		pb->openModalAction(e.getMouseDownX() < 40 ? MultiColumnPresetBrowser::ModalWindow::Action::Delete :
														  MultiColumnPresetBrowser::ModalWindow::Action::Rename,
														  name, entries[row], index, row);

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

void PresetBrowserColumn::ColumnListModel::sendRowChangeMessage(int row)
{
	if (listener != nullptr)
	{
		listener->selectionChanged(index, row, entries[row], false);
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
	
	if (HiseDeviceSimulator::isMobileDevice())
		listbox->setRowSelectedOnMouseDown(false);

	listbox->getViewport()->setScrollOnDragEnabled(true);
	
	setSize(150, 300);
    
    browser = dynamic_cast<MultiColumnPresetBrowser*>(listener);
}

File PresetBrowserColumn::getChildDirectory(File& root, int level, int index)
{
	if (!root.isDirectory()) return File();

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
		browser->openModalAction(MultiColumnPresetBrowser::ModalWindow::Action::Add, index == 2 ? "New Preset" : "New Directory", File(), index, -1);
	}
}

void PresetBrowserColumn::addEntry(const String &newName)
{
	if (!currentRoot.isDirectory()) return;

	if (index != 2)
	{

		File newDirectory = currentRoot.getChildFile(newName);
		newDirectory.createDirectory();

		setNewRootDirectory(currentRoot);
	}
	else
	{
		if (newName.isNotEmpty())
		{
			File newPreset = currentRoot.getChildFile(newName + ".preset");

			if (newPreset.existsAsFile())
			{
				File tempFile = newPreset.getSiblingFile("tempFileBeforeMove.preset");

				UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), tempFile.getFullPathName());
				browser->confirmReplace(tempFile, newPreset);
			}
			else
			{
				UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), newPreset.getFullPathName());

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
	setName("Preset Browser");

#if USE_BACKEND
	rootFile = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
    
    try
    {
        rootFile = ProjectHandler::Frontend::getUserPresetDirectory();
    }
    catch(String& s)
    {
        mc->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
    }
    
    
#endif

	mc->getUserPresetHandler().addListener(this);

	addAndMakeVisible(bankColumn = new PresetBrowserColumn(mc, 0, rootFile, this));
	addAndMakeVisible(categoryColumn = new PresetBrowserColumn(mc, 1, rootFile, this));
	addAndMakeVisible(presetColumn = new PresetBrowserColumn(mc, 2, rootFile, this));
	addAndMakeVisible(searchBar = new PresetBrowserSearchBar());
	addChildComponent(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	addAndMakeVisible(modalInputWindow = new ModalWindow());
	modalInputWindow->setVisible(false);

	closeButton->addListener(this);
	Path closeShape;
	closeShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	closeButton->setShape(closeShape, true, true, true);

	searchBar->inputLabel->addListener(this);
	searchBar->inputLabel->addListener(presetColumn);
	
	bankColumn->setNewRootDirectory(rootFile);

	
	setSize(width, height);

	rebuildAllPresets();
	
	showLoadedPreset();

}

MultiColumnPresetBrowser::~MultiColumnPresetBrowser()
{
	mc->getUserPresetHandler().removeListener(this);

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

	File f = mc->getUserPresetHandler().getCurrentlyLoadedFile();

	currentlyLoadedPreset = allPresets.indexOf(f);
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
	modalInputWindow->setBounds(getLocalBounds());

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

void MultiColumnPresetBrowser::openModalAction(ModalWindow::Action action, const String& preEnteredText, const File& fileToChange, int columnIndex, int rowIndex)
{
	if (action == ModalWindow::Action::Delete)
	{
		modalInputWindow->confirmDelete(columnIndex, fileToChange);
	}
	else
	{
		modalInputWindow->addActionToStack(action, preEnteredText, columnIndex, rowIndex);
	}
	
}

void MultiColumnPresetBrowser::showLoadedPreset()
{
	if (currentlyLoadedPreset != -1)
	{
		File f = allPresets[currentlyLoadedPreset];

		File category = f.getParentDirectory();
		File bank = category.getParentDirectory();

		bankColumn->setSelectedFile(bank, dontSendNotification);
		categoryColumn->setNewRootDirectory(bank);
		categoryColumn->setSelectedFile(category, dontSendNotification);
		presetColumn->setNewRootDirectory(category);
		presetColumn->setSelectedFile(f, dontSendNotification);
	}
}

void MultiColumnPresetBrowser::selectionChanged(int columnIndex, int /*rowIndex*/, const File& file, bool /*doubleClick*/)
{
	if (columnIndex == 0)
	{
		currentBankFile = file;

		categoryColumn->setNewRootDirectory(currentBankFile);
		presetColumn->setNewRootDirectory(File());
        
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


void MultiColumnPresetBrowser::renameEntry(int columnIndex, int rowIndex, const String& newName)
{
	if (columnIndex == 0)
	{
        if(newName.isNotEmpty())
        {
            File newBank = currentBankFile.getSiblingFile(newName);
            
			if (newBank.isDirectory())
				return;

            currentBankFile.moveFileTo(newBank);
            
            categoryColumn->setNewRootDirectory(File());
            presetColumn->setNewRootDirectory(File());
        }
        
        rebuildAllPresets();
	}
	else if (columnIndex == 1)
	{
		currentCategoryFile = PresetBrowserColumn::getChildDirectory(currentBankFile, 2, rowIndex);

        
        
        if(newName.isNotEmpty())
        {
            File newCategory = currentCategoryFile.getSiblingFile(newName);
            
            if(newCategory.isDirectory())
				return;
            
            currentCategoryFile.moveFileTo(newCategory);
            
            categoryColumn->setNewRootDirectory(currentBankFile);
            presetColumn->setNewRootDirectory(newCategory);
        }
        
        rebuildAllPresets();
	}
	else if (columnIndex == 2)
	{
		File presetFile = PresetBrowserColumn::getChildDirectory(currentCategoryFile, 3, rowIndex);

		if (newName.isNotEmpty())
		{
			File newFile = presetFile.getSiblingFile(newName + ".preset");

			if (newFile.existsAsFile())
				modalInputWindow->confirmReplacement(presetFile, newFile);
			else
			{
				presetFile.moveFileTo(newFile);
				presetColumn->setNewRootDirectory(currentCategoryFile);
				rebuildAllPresets();
			}
		}

		
	}
}

void MultiColumnPresetBrowser::deleteEntry(int columnIndex, const File& f)
{
	if (columnIndex == 0)
	{
		File bankToDelete = f;

		bankToDelete.deleteRecursively();

		bankColumn->setNewRootDirectory(rootFile);
		categoryColumn->setNewRootDirectory(File());
		presetColumn->setNewRootDirectory(File());
	}
	else if (columnIndex == 1)
	{
		File categoryToDelete = f;

		categoryToDelete.deleteRecursively();

		categoryColumn->setNewRootDirectory(currentBankFile);
		presetColumn->setNewRootDirectory(File());
	}
	else if (columnIndex == 2)
	{
		File presetFile = f;

		

		presetFile.deleteFile();
		presetColumn->setNewRootDirectory(currentCategoryFile);
	}

	rebuildAllPresets();
}


void MultiColumnPresetBrowser::addEntry(int columnIndex, const String& name)
{
	if (columnIndex == 0)	bankColumn->addEntry(name);
	if (columnIndex == 1)	categoryColumn->addEntry(name);
	if (columnIndex == 2)	presetColumn->addEntry(name);
	
}

void MultiColumnPresetBrowser::loadPreset(const File& f)
{
    if(f.existsAsFile())
    {
		UserPresetHelpers::loadUserPreset(mc->getMainSynthChain(), f);
        currentlyLoadedPreset = allPresets.indexOf(f);
        
#if NEW_USER_PRESET
        if (listener != nullptr)
        {
            listener->presetChanged(f.getFileNameWithoutExtension());
        }
#endif
    }
}

PresetBrowserSearchBar::PresetBrowserSearchBar()
{
	highlightColour = Colours::red.withBrightness(0.8f);

	addAndMakeVisible(inputLabel = new BetterLabel());
	inputLabel->setEditable(true, true);
	inputLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::transparentBlack);

	inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::white);
	inputLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
}

void PresetBrowserSearchBar::setHighlightColourAndFont(Colour c, Font f)
{
	highlightColour = c;

	inputLabel->setFont(f);
    inputLabel->setColour(TextEditor::ColourIds::highlightColourId, Colours::white);
	inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, highlightColour);
	inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
}

void PresetBrowserSearchBar::paint(Graphics &g)
{
	g.setColour(highlightColour);
	g.drawRoundedRectangle(1.0f, 1.0f, (float)getWidth() - 2.0f, (float)getHeight() - 2.0f, 2.0f, 1.0f);

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	path.scaleToFit(6.0f, 5.0f, 18.0f, 18.0f, true);

	g.fillPath(path);
}
