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

namespace hise { using namespace juce;


struct PresetHelpers
{
	static void importPresetsFromClipboard(const File& presetRoot, const File& category)
	{
		auto clipboardData = SystemClipboard::getTextFromClipboard();

		if (clipboardData.startsWith("[START_PRESETS]") && clipboardData.endsWith("[END_PRESETS]"))
		{
			auto data = clipboardData.fromFirstOccurrenceOf("[START_PRESETS]", false, false).upToLastOccurrenceOf("[END_PRESETS]", false, false);

			auto presetCollection = ValueTreeConverters::convertBase64ToValueTree(data, true);

			if (presetCollection.isValid())
			{
				importPresetsFromValueTree(presetRoot, category, presetCollection);
			}
			else
			{
				PresetHandler::showMessageWindow("Preset Data is corrupt", "The preset data can't be parsed from the clipboard data. Aborting...", PresetHandler::IconType::Error);
			}
		}
		else
			PresetHandler::showMessageWindow("No preset data found in clipboard", "Make sure you've copied everything including the [START_PRESETS] and [END_PRESETS] tags", PresetHandler::IconType::Error);

	};

	static void importPresetsFromFile(const File& presetRoot, const File& category)
	{
		FileChooser fc("Select Preset Collection to load", File(), "*.hpa", true);

		if (fc.browseForFileToOpen())
		{
			FileInputStream fis(fc.getResult());
			MemoryBlock mb;
			MemoryOutputStream mos;
			mos.writeFromInputStream(fis, INT_MAX);

			auto presetCollection = PresetHandler::loadValueTreeFromData(mos.getData(), mos.getDataSize(), true);


			importPresetsFromValueTree(presetRoot, category, presetCollection);

			
		}
	}

	

	static void exportPresetsToClipboard(const File& presetRoot, const File& category)
	{
		auto presetTree = exportPresets(presetRoot, category);

		if (presetTree.isValid())
		{
			String compressed;
			
			compressed << "[START_PRESETS]";
			compressed << ValueTreeConverters::convertValueTreeToBase64(presetTree, true);
			compressed << "[END_PRESETS]";
			SystemClipboard::copyTextToClipboard(compressed);
			PresetHandler::showMessageWindow("Success", String(presetTree.getNumChildren()) + " presets were compressed and stored to the clipboard");
		}
	}

	static void exportPresetsToFile(const File& presetRoot, const File& category)
	{
		auto presetTree = exportPresets(presetRoot, category);

		if (presetTree.isValid())
		{
			FileChooser fc("Select Preset Archive Destination", File(), "*.hpa", true);

			if (fc.browseForFileToSave(true))
			{
				auto f = fc.getResult().withFileExtension(".hpa");

				PresetHandler::writeValueTreeAsFile(presetTree, f.getFullPathName(), true);
				PresetHandler::showMessageWindow("Success", String(presetTree.getNumChildren()) + " presets were compressed and stored to " + f.getFullPathName());
			}
		}
	}

	static Array<File> getAllPresets(const File& directory)
	{
		Array<File> presets;

		directory.findChildFiles(presets, File::findFiles, true);

		for (int i = 0; i < presets.size(); i++)
		{
			const bool isNoPresetFile = presets[i].isHidden() || presets[i].getFileName().startsWith(".") || presets[i].getFileExtension() != ".preset";
			
			if (isNoPresetFile)
			{
				presets.remove(i--);
				continue;
			}
		}

		return presets;
	}

private:

	static void importPresetsFromValueTree(const File &presetRoot, const File &category, juce::ValueTree &presetCollection)
	{

		bool importToRoot = !category.isDirectory();
		String message = (importToRoot ? ("Import All Presets from the collection?") : ("Import all presets from the collection into " + category.getRelativePathFrom(presetRoot) + "?"));

		if (PresetHandler::showYesNoWindow("Import Presets", message))
		{
			int numWritten = 0;
			int numSkipped = 0;

			if (presetCollection.isValid())
			{
				const bool replaceExistingFiles = PresetHandler::showYesNoWindow("Replace existing presets", "Do you want to replace existing presets? Press Cancel to keep the old ones.");

				for (const auto& c : presetCollection)
				{
					auto relativePath = c.getProperty("FilePath").toString();

					ScopedPointer<XmlElement> xml = c.createXml();

					xml->removeAttribute("FilePath");

					File newPresetFile = presetRoot.getChildFile(relativePath);

					if (category.isDirectory())
						newPresetFile = category.getChildFile(newPresetFile.getFileName());

					if (!newPresetFile.getParentDirectory().isDirectory())
						newPresetFile.getParentDirectory().createDirectory();

					if (replaceExistingFiles || !newPresetFile.existsAsFile())
					{
						xml->writeToFile(newPresetFile, "");
						numWritten++;
					}
					else
					{
						numSkipped++;
					}
				}

				String m = String(numWritten) + " presets were imported from the collection";

				if (numSkipped != 0)
					m << "\n" + String(numSkipped) + " presets were not updated.";

				PresetHandler::showMessageWindow("Successful", m);
			}
		}
	}

	static ValueTree exportPresets(const File& presetRoot, const File& category)
	{
		bool exportAll = !category.isDirectory();
		String message = (exportAll ? ("Export All Presets?") : ("Export all presets from the Category " + category.getRelativePathFrom(presetRoot) + "?"));

		if (PresetHandler::showYesNoWindow("Export Presets", message))
		{
			ValueTree presetTree("PresetCollection");

			auto presetList = getAllPresets(exportAll ? presetRoot : category);

			for (auto f : presetList)
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

				if (xml != nullptr)
				{
					auto path = f.getRelativePathFrom(presetRoot).replaceCharacter('\\', '/');

					xml->setAttribute("FilePath", path);
					auto child = ValueTree::fromXml(*xml);
					presetTree.addChild(child, -1, nullptr);
				}
				else
				{
					PresetHandler::showMessageWindow("Error", "The preset " + f.getFullPathName() + " could not be found");
					return ValueTree();
				}
			}

			return presetTree;
		}

		return ValueTree();
	};
	
};


void PresetBrowserLookAndFeel::ButtonLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
{
	if (button.getToggleState())
	{
		auto r = button.getLocalBounds();

		g.setColour(highlightColour.withAlpha(0.1f));
		g.fillRoundedRectangle(r.reduced(3, 1).toFloat(), 2.0f);
	}
}

void PresetBrowserLookAndFeel::ButtonLookAndFeel::drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
#if OLD_PRESET_BROWSER
	g.setColour(highlightColour);
	g.setFont(font.withHeight(18.0f));
	g.drawText(button.getButtonText(), 0, 0, button.getWidth(), button.getHeight(), Justification::centred);

	ignoreUnused(isMouseOverButton, isButtonDown);

#else
	g.setColour(highlightColour.withAlpha(isMouseOverButton || button.getToggleState() ? 1.0f : 0.7f));
    g.setFont(font);
	g.drawText(button.getButtonText(), 0, isButtonDown ? 1 : 0, button.getWidth(), button.getHeight(), Justification::centred);

	if (isMouseOverButton)
	{
		auto r = button.getLocalBounds();

		g.setColour(highlightColour.withAlpha(0.1f));
		g.fillRoundedRectangle(r.reduced(3, 1).toFloat(), 2.0f);
	}
#endif

}

PresetBrowserColumn::ColumnListModel::ColumnListModel(int index_, Listener* listener_) :
root(File()),
index(index_),
listener(listener_)
{
	deleteIcon = Image(); //ImageCache::getFromMemory(CustomFrontendToolbarImages::deleteIcon_png, sizeof(CustomFrontendToolbarImages::deleteIcon_png));
	
	
}

void PresetBrowser::ModalWindow::paint(Graphics& g)
{
    g.setColour(Colours::black.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.7f)));
    g.fillAll();
    
    auto area = inputLabel->getBounds().expanded(50);
    
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xfa212121)));
    g.fillRoundedRectangle(area.expanded(40).toFloat(), 2.0f);
    
    g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x228e8e8e)));
    
    if(inputLabel->isVisible())
        g.fillRect(inputLabel->getBounds());
    
    g.setColour(Colours::white.withAlpha(0.8f));
    g.setFont(f.withHeight(18));
    g.drawText(getTitleText(), 0, inputLabel->getY() - 80, getWidth(), 30, Justification::centredTop);
        
    g.setFont(f);
    
    g.drawText(getCommand(), area, Justification::centredTop);
}

int PresetBrowserColumn::ColumnListModel::getNumRows()
{
    if(wildcard.isEmpty() && currentlyActiveTags.isEmpty())
    {   
		const File& rootToUse = showFavoritesOnly ? totalRoot : root;

		if (!rootToUse.isDirectory())
	    {
		    entries.clear();
		    return 0;
	    }
		
	    entries.clear();
	    rootToUse.findChildFiles(entries, displayDirectories ? File::findDirectories : File::findFiles, allowRecursiveSearch || showFavoritesOnly);

		PresetBrowser::DataBaseHelpers::cleanFileList(entries);

		if (showFavoritesOnly && index == 2)
		{
			for (int i = 0; i < entries.size(); i++)
			{
				if (!PresetBrowser::DataBaseHelpers::isFavorite(database, entries[i]))
				{
					entries.remove(i--);
					continue;
				}
			}
		}
			
	    entries.sort();

		empty = entries.isEmpty();

	    return entries.size();
    }
    else
    {
		jassert(index == 2);

        Array<File> allFiles;
        
        totalRoot.findChildFiles(allFiles, File::findFiles, true);
        
        entries.clear();
        
		for (int i = 0; i < allFiles.size(); i++)
		{
			const bool matchesWildcard = wildcard.isEmpty() || allFiles[i].getFullPathName().containsIgnoreCase(wildcard);

			bool matchesTags = currentlyActiveTags.size() == 0;

			auto hash = allFiles[i].hashCode64();

			if (currentlyActiveTags.size() > 0)
			{
				for (auto& t : cachedTags)
				{
					if (t.hashCode == hash)
					{
						matchesTags = t.shown;
						break;
					}
				}
			}

			if(matchesWildcard && matchesTags)
                entries.add(allFiles[i]);
        }
        
		for (int i = 0; i < entries.size(); i++)
		{
			const bool isNoPresetFile = entries[i].isHidden() || entries[i].getFileName().startsWith(".") || entries[i].getFileExtension() != ".preset";
			
			if (isNoPresetFile)
			{
				entries.remove(i--);
			}
		}

		if (showFavoritesOnly && index == 2)
		{
			for (int i = 0; i < entries.size(); i++)
			{
				if (!PresetBrowser::DataBaseHelpers::isFavorite(database, entries[i]))
				{
					entries.remove(i--);
					continue;
				}
			}
		}


        entries.sort();
        
		empty = entries.isEmpty();

        return entries.size();
    }
}

void PresetBrowserColumn::ColumnListModel::listBoxItemClicked(int row, const MouseEvent &e)
{
	auto maxWidth = e.eventComponent->getWidth() - e.eventComponent->getHeight();

	bool deleteAction = deleteOnClick && e.getMouseDownX() > maxWidth;

	if (deleteAction)
	{
		const String title = index != 2 ? "Directory" : "Preset";

		auto name = entries[row].getFileNameWithoutExtension();

		auto pb = dynamic_cast<PresetBrowser*>(listener);

		if (pb == nullptr)
			return;

		pb->openModalAction(PresetBrowser::ModalWindow::Action::Delete, name, entries[row], index, row);

	}
	else
	{
		if (listener != nullptr && !e.mouseWasDraggedSinceMouseDown())
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


void PresetBrowserColumn::ColumnListModel::rebuildCachedTagList()
{
	cachedTags.clear();

	Array<File> allPresets;

	totalRoot.findChildFiles(allPresets, File::findFiles, true, "*.preset");

	PresetBrowser::DataBaseHelpers::cleanFileList(allPresets);

	for (auto f : allPresets)
	{
		auto sa = PresetBrowser::DataBaseHelpers::getTagsFromXml(f);

		CachedTag newTag;
		newTag.hashCode = f.hashCode64();
		for (auto t : sa)
			newTag.tags.add(Identifier(t));

		cachedTags.add(std::move(newTag));
	}
}

void PresetBrowserColumn::ColumnListModel::updateTags(const StringArray& newSelection)
{
	currentlyActiveTags.clear();

	for (auto s : newSelection)
		currentlyActiveTags.add(Identifier(s));

	for (auto& t : cachedTags)
	{
		t.shown = true;

		for (auto st : currentlyActiveTags)
		{
			if (!t.tags.contains(st))
			{
				t.shown = false;
				break;
			}
		}
	}
}

void PresetBrowserColumn::ColumnListModel::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < entries.size())
	{
		Rectangle<int> area(0, 1, width, height - 2);

#if OLD_PRESET_BROWSER
		

		
		g.setColour(rowIsSelected ? highlightColour.withAlpha(0.3f) : Colour(0x00222222));
		g.fillRect(area);
		g.setColour(Colours::white.withAlpha(0.4f));
		if(rowIsSelected) g.drawRect(area, 1);

        const float fontSize = HiseDeviceSimulator::isiPhone() ? 14.0f : 16.0f;
        g.setFont(font.withHeight(fontSize));
        
		if (editMode)
		{
			g.setColour(Colour(0xFFAA0000));

			const float widthOfDeleteCircle = ((float)height - 10.0f);

			g.fillEllipse(5.0f, 5.0f, widthOfDeleteCircle, widthOfDeleteCircle);

			g.setColour(Colours::white);

			g.fillRect(5.0f + widthOfDeleteCircle * 0.5f - 6.0f, 5.0f + widthOfDeleteCircle * 0.5f - 1.0f, 12.0f, 2.0f);

			g.drawText(entries[rowNumber].getFileNameWithoutExtension(), 30, 0, width-20, height, Justification::centredLeft);
		}
		else
		{
            
            
			g.setColour(Colours::white);
			
			g.drawText(entries[rowNumber].getFileNameWithoutExtension(), 10, 0, width-20, height, Justification::centredLeft);
		}

#else

		g.setGradientFill(ColourGradient(highlightColour.withAlpha(0.3f), 0.0f, 0.0f,
						  highlightColour.withAlpha(0.2f), 0.0f, (float)height, false));

		if(rowIsSelected)
			g.fillRect(area);

		g.setColour(Colours::white.withAlpha(0.9f));

		if (deleteOnClick)
		{
			Path p;
			
			p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

			auto r = area.removeFromRight(area.getHeight()).reduced(3).toFloat();
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), true);

			g.fillPath(p);
		}

		g.setColour(Colours::white.withAlpha(0.9f));
		g.setFont(font.withHeight(16.0f));
		g.drawText(entries[rowNumber].getFileNameWithoutExtension(), index == 2 ? 10 + 26 : 10, 0, width - 20, height, Justification::centredLeft);

#endif

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
	editButton->addListener(this);
	editButton->setLookAndFeel(&blaf);

#if OLD_PRESET_BROWSER
	addAndMakeVisible(addButton = new ShapeButton("Add", Colours::white, Colours::white, Colours::white));
	Path addShape;
	addShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));
	addButton->setShape(addShape, true, true, false);
	addButton->setLookAndFeel(&blaf);
	addButton->addListener(this);
#else
	addAndMakeVisible(addButton = new TextButton("Add"));
	addButton->addListener(this);
	addButton->setLookAndFeel(&blaf);

	addAndMakeVisible(renameButton = new TextButton("Rename"));
	renameButton->addListener(this);
	renameButton->setLookAndFeel(&blaf);

	addAndMakeVisible(deleteButton = new TextButton("Delete"));
	deleteButton->addListener(this);
	deleteButton->setLookAndFeel(&blaf);

#endif


	
	

	listModel = new ColumnListModel(index, listener);

	listModel->database = dynamic_cast<PresetBrowser*>(listener)->getDataBase();
	
	listModel->setTotalRoot(rootDirectory);
	
	startTimer(4000);
	
	if (index == 2)
	{
		listModel->setDisplayDirectories(false);
	}

	addAndMakeVisible(listbox = new ListBox());

	listbox->setModel(listModel);
#if OLD_PRESET_BROWSER
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::white.withAlpha(0.15f));
#else
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
#endif

	listbox->setRowHeight(30);
	listbox->setWantsKeyboardFocus(true);
	
	if (HiseDeviceSimulator::isMobileDevice())
		listbox->setRowSelectedOnMouseDown(false);

	listbox->getViewport()->setScrollOnDragEnabled(true);
	
	listbox->addMouseListener(this, true);

	setSize(150, 300);
    
    browser = dynamic_cast<PresetBrowser*>(listener);
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

	updateButtonVisibility();
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
		browser->openModalAction(PresetBrowser::ModalWindow::Action::Add, index == 2 ? "New Preset" : "New Directory", File(), index, -1);
	}
#if !OLD_PRESET_BROWSER
	else if (b == renameButton)
	{
		int selectedIndex = listbox->getSelectedRow(0);

		if (selectedIndex >= 0)
		{
			File oldFile = listModel->getFileForIndex(selectedIndex);

			browser->openModalAction(PresetBrowser::ModalWindow::Action::Rename, oldFile.getFileNameWithoutExtension(), oldFile, index, selectedIndex);
		}
	}
	else if (b == deleteButton)
	{
		int selectedIndex = listbox->getSelectedRow(0);

		if (selectedIndex >= 0)
		{
			File f = listModel->getFileForIndex(selectedIndex);
			browser->openModalAction(PresetBrowser::ModalWindow::Action::Delete, "", f, index, selectedIndex);
		}
	}
#endif
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
				browser->showLoadedPreset();
			}
		}
	}

	updateButtonVisibility();
}


void PresetBrowserColumn::paint(Graphics& g)
{
	String name;

	if (isResultBar) name = "Search results";
	else if (index == 0) name = "Bank";
	else if (index == 1) name = "Category";
	else name = "Preset";


#if OLD_PRESET_BROWSER
	Rectangle<int> textArea = listArea;

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(0, 40, getWidth(), getHeight() - 40, 1);
    g.setColour(Colours::white.withAlpha(0.02f));
    g.fillRect(textArea);

	g.setColour(Colours::white);


	
	g.setFont(font.withHeight(16.0f));
	g.drawText(name, 0, 0, getWidth(), 30, Justification::centred);

#else

	g.setColour(highlightColour.withAlpha(0.1f));

	g.drawRoundedRectangle(listArea.toFloat(), 2.0f, 2.0f);


#endif


	

	if (currentRoot == File() && listModel->wildcard.isEmpty() && listModel->currentlyActiveTags.isEmpty())
	{
		g.setFont(font);
		g.setColour(Colours::white.withAlpha(0.3f));
		g.drawText("Select a " + String(index == 1 ? "Bank" : "Category"), 0, 0, getWidth(), getHeight(), Justification::centred);
	}
	else if (listModel->isEmpty())
	{
		g.setFont(font);
		g.setColour(Colours::white.withAlpha(0.3f));

		String text = isResultBar ? "No results" : "Add a " + name;

		g.drawText(text, 0, 0, getWidth(), getHeight(), Justification::centred);
	}
}

void PresetBrowserColumn::resized()
{
#if OLD_PRESET_BROWSER
	listArea = { 0, 40, getWidth(), getHeight() - 40 };

	listbox->setBounds(listArea);
	addButton->setBounds(getWidth() - 30, 10, 20, 20);
	editButton->setBounds(0, 7, 40, 26);
#else

	listArea = { 0, 0, getWidth(), getHeight()};
	listArea = listArea.reduced(1);

	updateButtonVisibility();
	
	if (showButtonsAtBottom)
	{
		auto buttonArea = listArea.removeFromBottom(28).reduced(2);
		const int buttonWidth = buttonArea.getWidth() / 3;

		addButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
		renameButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
		deleteButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
		listArea.removeFromBottom(10);
	}
	
	listbox->setBounds(listArea.reduced(3));
#endif
}


void PresetBrowserColumn::updateButtonVisibility()
{
#if !OLD_PRESET_BROWSER
	editButton->setVisible(false);

	const bool buttonsVisible = showButtonsAtBottom && !isResultBar && currentRoot.isDirectory();
	const bool fileIsSelected = listbox->getNumSelectedRows() > 0;

	addButton->setVisible(buttonsVisible);
	deleteButton->setVisible(buttonsVisible && fileIsSelected);
	renameButton->setVisible(buttonsVisible && fileIsSelected);
#endif
}

PresetBrowser::PresetBrowser(MainController* mc_, int width, int height) :
mc(mc_)
{
	setName("Preset Browser");

	setColour(PresetBrowserSearchBar::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	

#if USE_BACKEND
	rootFile = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
    
    try
    {
        rootFile = FrontendHandler::getUserPresetDirectory();
    }
    catch(String& s)
    {
        mc->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
    }
    
    
#endif

	loadPresetDatabase(rootFile);

	mc->getUserPresetHandler().addListener(this);

	addAndMakeVisible(bankColumn = new PresetBrowserColumn(mc, 0, rootFile, this));
	addAndMakeVisible(categoryColumn = new PresetBrowserColumn(mc, 1, rootFile, this));
	addAndMakeVisible(presetColumn = new PresetBrowserColumn(mc, 2, rootFile, this));
	addAndMakeVisible(searchBar = new PresetBrowserSearchBar());
	addChildComponent(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	addAndMakeVisible(noteLabel = new BetterLabel());
	noteLabel->addListener(this);
	
	noteLabel->setEditable(true, true);
	noteLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	noteLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	noteLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::transparentBlack);
	noteLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::white);
	noteLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	noteLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
	noteLabel->setJustificationType(Justification::centred);

	addAndMakeVisible(tagList = new TagList(mc));

	addAndMakeVisible(favoriteButton = new ShapeButton("Show Favorites", Colours::white, Colours::white, Colours::white));
	favoriteButton->addListener(this);

	addAndMakeVisible(modalInputWindow = new ModalWindow());
	modalInputWindow->setVisible(false);

	closeButton->addListener(this);
	Path closeShape;
	closeShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	closeButton->setShape(closeShape, true, true, true);

	searchBar->inputLabel->addListener(this);
	searchBar->inputLabel->addListener(presetColumn);
	
	tagList->addTagListener(presetColumn);
	tagList->addTagListener(this);
	presetColumn->tagCacheNeedsRebuilding();
	presetColumn->setAllowRecursiveFileSearch(true);

	bankColumn->setNewRootDirectory(rootFile);

	addAndMakeVisible(saveButton = new TextButton("Save Preset"));
	saveButton->addListener(this);
	saveButton->setLookAndFeel(&blaf);
	
    addAndMakeVisible(manageButton = new TextButton(HiseDeviceSimulator::isMobileDevice() ? "Sync" : "More"));
	manageButton->addListener(this);
	manageButton->setLookAndFeel(&blaf);

	setSize(width, height);

	rebuildAllPresets();
	
	showLoadedPreset();

	updateFavoriteButton();
    
    setOpaque(true);

}

PresetBrowser::~PresetBrowser()
{
	mc->getUserPresetHandler().removeListener(this);

	savePresetDatabase(rootFile);

	searchBar->inputLabel->removeListener(this);
	searchBar->inputLabel->removeListener(presetColumn);

	tagList->removeTagListener(this);
	tagList->removeTagListener(presetColumn);

	tagList = nullptr;

	closeButton->removeListener(this);

	searchBar = nullptr;
	bankColumn = nullptr;
	categoryColumn = nullptr;
	presetColumn = nullptr;
}

void PresetBrowser::paint(Graphics& g)
{
#if OLD_PRESET_BROWSER
	g.fillAll(Colours::black.withAlpha(0.97f));
	g.fillAll(backgroundColour);
#else

	if (isOpaque())
	{

		g.fillAll(Colour(0xFF666666));

		g.setGradientFill(ColourGradient(backgroundColour.withMultipliedBrightness(1.2f), 0.0f, 0.0f,
			backgroundColour, 0.0f, (float)getHeight(), false));

		g.fillAll();
	}

	g.setColour(blaf.highlightColour.withAlpha(0.1f));

	if(noteLabel->isVisible())
		g.fillRoundedRectangle(noteLabel->getBounds().toFloat(), 2.0f);



#endif


	if (closeButton->isVisible())
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(0, 0, getWidth(), 30);
		g.setFont(GLOBAL_BOLD_FONT().withHeight(20.0f));
		g.setColour(Colours::white);
		g.drawText("Preset Browser", 0, 0, getWidth(), 30, Justification::centred, false);
	}
}

void PresetBrowser::rebuildAllPresets()
{
	allPresets.clear();
	rootFile.findChildFiles(allPresets, File::findFiles, true, "*.preset");

	for (int i = 0; i < allPresets.size(); i++)
	{
		const bool isNoPresetFile = allPresets[i].isHidden() || allPresets[i].getFileName().startsWith(".") || allPresets[i].getFileExtension() != ".preset";
		const bool isNoDirectory = !allPresets[i].isDirectory();

		if (isNoDirectory && isNoPresetFile)
		{
			allPresets.remove(i--);
		}
	}

	File f = mc->getUserPresetHandler().getCurrentlyLoadedFile();

	currentlyLoadedPreset = allPresets.indexOf(f);

	if (numColumns == 1)
	{
		presetColumn->setNewRootDirectory(rootFile);

		presetColumn->setEditMode(false);
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

		bankColumn->setEditMode(false);
		presetColumn->updateButtonVisibility();
	}
}

String PresetBrowser::getCurrentlyLoadedPresetName()
{
	if (currentlyLoadedPreset > 0 && currentlyLoadedPreset < allPresets.size())
	{
		return allPresets[currentlyLoadedPreset].getFileNameWithoutExtension();
	}

	return String();
}

void PresetBrowser::resized()
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

		Rectangle<int> ar(3, y + 5, getWidth() - 6, 30);

#if !OLD_PRESET_BROWSER
		saveButton->setBounds(ar.removeFromRight(100));
		manageButton->setBounds(ar.removeFromLeft(100));
#endif

		searchBar->setBounds(ar);
		
		y += 40;

	}
	else
	{
		Rectangle<int> ar(3, 3 + 3, getWidth() - 6, 30);

		
#if !OLD_PRESET_BROWSER
		saveButton->setBounds(ar.removeFromRight(100));
		manageButton->setBounds(ar.removeFromLeft(100));
		favoriteButton->setBounds(ar.removeFromLeft(30));
#endif

		ar.removeFromLeft(10);

		searchBar->setBounds(ar);
		y += 40;
	}
	
	int x = 3;

	bankColumn->setVisible(!showOnlyPresets && numColumns > 1);
	categoryColumn->setVisible(!showOnlyPresets && numColumns > 2);
	presetColumn->setIsResultBar(showOnlyPresets);

	auto listArea = Rectangle<int>(x, y, getWidth() - 6, getHeight() - y - 3);

#if !OLD_PRESET_BROWSER

	if (noteLabel->isVisible())
	{
		auto labelArea = listArea.removeFromTop(40);
		noteLabel->setBounds(labelArea.reduced(3, 5));
	}

	if (tagList->isActive())
		tagList->setBounds(listArea.removeFromTop(30));

#endif

	if (showOnlyPresets)
	{
		presetColumn->setBounds(listArea);
	}
	else
	{
#if OLD_PRESET_BROWSER
		bankColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
		x += getWidth() / 3;
		categoryColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
		x += getWidth() / 3;
		presetColumn->setBounds(x, y, getWidth() / 3 - 5, getHeight() - y - 3);
#else
		const int columnWidth = getWidth() / numColumns;

		if(numColumns > 1)
			bankColumn->setBounds(listArea.removeFromLeft(columnWidth).reduced(2, 2));

		if(numColumns > 2)
			categoryColumn->setBounds(listArea.removeFromLeft(columnWidth).reduced(2, 2));

		presetColumn->setBounds(listArea.removeFromLeft(columnWidth).reduced(2, 2));
#endif
	}

	
}


void PresetBrowser::loadPresetDatabase(const File& rootDirectory)
{
	auto dbFile = rootDirectory.getChildFile("db.json");

	var d = JSON::parse(dbFile.loadFileAsString());

	if (d.isObject())
		presetDatabase = d;
	else
		presetDatabase = new DynamicObject();
}


void PresetBrowser::savePresetDatabase(const File& rootDirectory)
{
	auto content = JSON::toString(presetDatabase);

	auto dbFile = rootDirectory.getChildFile("db.json");
	
	dbFile.replaceWithText(content);
}

void PresetBrowser::setHighlightColourAndFont(Colour c, Colour bgColour, Font f)
{
	backgroundColour = bgColour;
	outlineColour = c;

	blaf.font = f;
	blaf.highlightColour = c;

	tagList->setHighlightColourAndFont(c, bgColour, f);
	

	modalInputWindow->f = f;
	modalInputWindow->highlightColour = c;

	noteLabel->setFont(f);

	favoriteButton->setColours(c.withAlpha(0.7f), c.withAlpha(0.5f), c.withAlpha(0.6f));

	setOpaque(bgColour.isOpaque());

	searchBar->setHighlightColourAndFont(c, f);
	bankColumn->setHighlightColourAndFont(c, f);
	categoryColumn->setHighlightColourAndFont(c, f);
	presetColumn->setHighlightColourAndFont(c, f);
}

void PresetBrowser::setNumColumns(int newNumColumns)
{
	newNumColumns = jlimit<int>(1, 3, newNumColumns);

	if (newNumColumns != numColumns)
	{
		numColumns = newNumColumns;
		resized();

		if (numColumns == 1)
			rebuildAllPresets();
	}
}

void PresetBrowser::openModalAction(ModalWindow::Action action, const String& preEnteredText, const File& fileToChange, int columnIndex, int rowIndex)
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

void PresetBrowser::showLoadedPreset()
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

void PresetBrowser::selectionChanged(int columnIndex, int /*rowIndex*/, const File& file, bool /*doubleClick*/)
{
	const bool showCategoryColumn = numColumns == 3;
	const bool showBankColumns = numColumns >= 2;

	if (columnIndex == 0)
	{
		currentBankFile = file;

		if (showCategoryColumn)
		{
			

			categoryColumn->setNewRootDirectory(currentBankFile);
			currentCategoryFile = File();
			presetColumn->setNewRootDirectory(File());

			categoryColumn->setEditMode(false);
			presetColumn->setEditMode(false);

			bankColumn->updateButtonVisibility();

			noteLabel->setText("", dontSendNotification);
		}
		else
		{
			presetColumn->setNewRootDirectory(currentBankFile);

			presetColumn->setEditMode(false);
			presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

			bankColumn->setEditMode(false);
			presetColumn->updateButtonVisibility();
		}
		
		noteLabel->setText("", dontSendNotification);
	}
	else if (columnIndex == 1)
	{
		currentCategoryFile = file;

		presetColumn->setNewRootDirectory(currentCategoryFile);
        
        presetColumn->setEditMode(false);
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

        bankColumn->setEditMode(false);
        
		categoryColumn->updateButtonVisibility();
		presetColumn->updateButtonVisibility();

		noteLabel->setText("", dontSendNotification);
	}
	else if (columnIndex == 2)
	{
		loadPreset(file);

		bankColumn->setEditMode(false);
		categoryColumn->setEditMode(false);

		presetColumn->updateButtonVisibility();

	}
}


void PresetBrowser::renameEntry(int columnIndex, int rowIndex, const String& newName)
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
				showLoadedPreset();
			}
		}
	}
}

void PresetBrowser::deleteEntry(int columnIndex, const File& f)
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


void PresetBrowser::buttonClicked(Button* b)
{
	if (b == closeButton)
	{
		destroy();
	}
	else if (b == saveButton)
	{
		if (mc->getUserPresetHandler().getCurrentlyLoadedFile().existsAsFile())
		{
			

			auto fileToBeReplaced = mc->getUserPresetHandler().getCurrentlyLoadedFile();

			File tempFile = fileToBeReplaced.getSiblingFile("tempFileBeforeMove.preset");

			UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), tempFile.getFullPathName(), dontSendNotification);

			confirmReplace(tempFile, fileToBeReplaced);
		}
	}
	else if (b == manageButton)
	{
		PopupLookAndFeel plaf;
		PopupMenu p;
		p.setLookAndFeel(&plaf);

		enum ID
		{
			ShowPresetFolder = 1,
			ImportPresetsFromClipboard,
			ImportPresetsFromFile,
			ExportPresetsToClipboard,
			ExportPresetsToFile,
			ClearFavorites,
			ResetToFactoryDefault,
			numIDs
		};


		const bool categoryMode = currentCategoryFile.isDirectory();

		const String destination = categoryMode ? ("presets in " + currentCategoryFile.getFileNameWithoutExtension()) : ("all presets");

        if(HiseDeviceSimulator::isMobileDevice() && !categoryMode)
        {
            p.addItem(numIDs, "You have to select a category for import / export", false, false);
            
        }
		else if (HiseDeviceSimulator::isMobileDevice())
		{
			p.addItem(ImportPresetsFromClipboard, "Import " + destination + " from Clipboard");
			p.addItem(ExportPresetsToClipboard, "Export " + destination + " to Clipboard");
		}
		else
		{
            p.addItem(ShowPresetFolder, "Show Preset Folder");
            //p.addItem(ClearFavorites, "Clear Favorites");
            p.addSeparator();
            
            p.addItem(ImportPresetsFromClipboard, "Import " + destination + " from Clipboard");
            p.addItem(ExportPresetsToClipboard, "Export " + destination + " to Clipboard");
            
            p.addSeparator();
            
			p.addItem(ImportPresetsFromFile, "Import " + destination + " from Collection");
			p.addItem(ExportPresetsToFile, "Export " + destination + " as Collection");
		}

		//p.addSeparator();
		//p.addItem(ResetToFactoryDefault, "Reset all presets to factory default");
	
		

		auto result = (ID)p.show();

		switch (result)
		{
		case ShowPresetFolder:
			rootFile.revealToUser();
			break;
		case ImportPresetsFromClipboard:
			PresetHelpers::importPresetsFromClipboard(rootFile, currentCategoryFile);
			break;
		case ImportPresetsFromFile:
			PresetHelpers::importPresetsFromFile(rootFile, currentCategoryFile);
			break;
		case ExportPresetsToClipboard:
			PresetHelpers::exportPresetsToClipboard(rootFile, currentCategoryFile);
			break;
		case ExportPresetsToFile:
			PresetHelpers::exportPresetsToFile(rootFile, currentCategoryFile);
			break;
		case ClearFavorites:
			break;
		case ResetToFactoryDefault:
			break;
		case numIDs:
			break;
		default:
			break;
		}

		
	}
	else if (b == favoriteButton)
	{
		b->setToggleState(!b->getToggleState(), dontSendNotification);
		updateFavoriteButton();
	}
}



void PresetBrowser::addEntry(int columnIndex, const String& name)
{
	if (columnIndex == 0)	bankColumn->addEntry(name);
	if (columnIndex == 1)	categoryColumn->addEntry(name);
	if (columnIndex == 2)	presetColumn->addEntry(name);
	
}

void PresetBrowser::loadPreset(const File& f)
{
    if(f.existsAsFile())
    {
		UserPresetHelpers::loadUserPreset(mc->getMainSynthChain(), f);
        currentlyLoadedPreset = allPresets.indexOf(f);
        
		noteLabel->setText(DataBaseHelpers::getNoteFromXml(f), dontSendNotification);

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
	setColour(ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));

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


PresetBrowserColumn::ColumnListModel::FavoriteOverlay::FavoriteOverlay(ColumnListModel& parent_, int index_) :
	parent(parent_),
	index(index_)
{
	addAndMakeVisible(b = new ShapeButton("Favorite", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white));


	refreshShape();

	b->addListener(this);

	setInterceptsMouseClicks(false, true);
	setWantsKeyboardFocus(false);
	b->setWantsKeyboardFocus(false);
}


PresetBrowserColumn::ColumnListModel::FavoriteOverlay::~FavoriteOverlay()
{
	b->removeListener(this);
	b = nullptr;
}

void PresetBrowserColumn::ColumnListModel::FavoriteOverlay::refreshShape()
{
	auto f = parent.getFileForIndex(index);

	const bool on = PresetBrowser::DataBaseHelpers::isFavorite(parent.database, f);


	static const unsigned char onShape[] = "nm\xac&=Ca\xee<Cl\x12\x96?C%\xaf""CCl\xde\xc2""FC\xd0\xe9""CClZ\x17""AC\xebPHCl(\x17""CC\xf1""5OCl\xad&=C\xc4-KCl267C\xf1""5OCl\0""69C\xebPHCl}\x8a""3C\xd0\xe9""CClH\xb7:C%\xaf""CCce";

	static const unsigned char offShape[] = { 110,109,0,144,89,67,0,103,65,67,108,0,159,88,67,0,3,68,67,108,129,106,86,67,0,32,74,67,108,1,38,77,67,0,108,74,67,108,1,121,84,67,0,28,80,67,108,129,227,81,67,255,3,89,67,108,1,144,89,67,127,206,83,67,108,1,60,97,67,255,3,89,67,108,129,166,94,67,0,28,
		80,67,108,129,249,101,67,0,108,74,67,108,1,181,92,67,0,32,74,67,108,1,144,89,67,0,103,65,67,99,109,0,144,89,67,1,76,71,67,108,128,73,91,67,1,21,76,67,108,0,94,96,67,129,62,76,67,108,0,90,92,67,129,92,79,67,108,128,196,93,67,129,62,84,67,108,0,144,89,
		67,129,99,81,67,108,0,91,85,67,1,63,84,67,108,128,197,86,67,129,92,79,67,108,128,193,82,67,129,62,76,67,108,0,214,87,67,1,21,76,67,108,0,144,89,67,1,76,71,67,99,101,0,0 };

	Path path;

	if (on)
	{
		b->setColours(Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white);
		path.loadPathFromData(onShape, sizeof(onShape));
	}
		
	else
	{
		b->setColours(Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white);
		path.loadPathFromData(offShape, sizeof(offShape));
	}
	
	b->setToggleState(on, dontSendNotification);

	b->setShape(path, false, true, true);
}


void PresetBrowserColumn::ColumnListModel::FavoriteOverlay::buttonClicked(Button*)
{
	const bool newValue = !b->getToggleState();

	auto f = parent.getFileForIndex(index);

	PresetBrowser::DataBaseHelpers::setFavorite(parent.database, f, newValue);

	
	refreshShape();

	findParentComponentOfClass<PresetBrowserColumn>()->listbox->updateContent();
}


void PresetBrowserColumn::ColumnListModel::FavoriteOverlay::resized()
{
	refreshShape();
	auto r = Rectangle<int>(0, 0, getHeight(), getHeight());
	b->setBounds(r.reduced(4));
}

void TagList::Tag::paint(Graphics& g)
{
	float alpha = active ? 0.4f : 0.1f;
	alpha += (parent.on ? 0.2f : 0.0f);

	auto ar = getLocalBounds().toFloat().reduced(1.0f);

	g.setColour(parent.c.withAlpha(alpha));
	g.fillRoundedRectangle(ar, 2.0f);
	g.drawRoundedRectangle(ar, 2.0f, 1.0f);
	g.setFont(parent.f.withHeight(14.0f));
	g.setColour(Colours::white.withAlpha(selected ? 0.9f : 0.6f));
	g.drawText(name, ar, Justification::centred);

	if(selected)
		g.drawRoundedRectangle(ar, 2.0f, 2.0f);
}

TagList::TagList(MainController* mc_) :
	ControlledObject(mc_),
	editButton("Edit Tags")
{
	
	

	editButton.addListener(this);
	addAndMakeVisible(editButton);
	
	editButton.setLookAndFeel(&blaf);
	getMainController()->getUserPresetHandler().addListener(this);
	rebuildTags();
	presetChanged(getMainController()->getUserPresetHandler().getCurrentlyLoadedFile());
}

void TagList::buttonClicked(Button*)
{
	editMode = !editButton.getToggleState();
	editButton.setToggleState(editMode, dontSendNotification);

	if (editMode)
		startTimer(500);
	else
	{
		on = false;
		stopTimer();

		for (auto t : tags)
			t->repaint();
	}
}

void TagList::presetChanged(const File& newPreset)
{
	currentFile = newPreset;
	editButton.setVisible(currentFile.existsAsFile());

	if (currentFile.existsAsFile())
	{
		currentlyActiveTags = PresetBrowser::DataBaseHelpers::getTagsFromXml(currentFile);
	}
	else
		currentlyActiveTags.clear();

	for (auto t : tags)
		t->setActive(currentlyActiveTags.contains(t->name));
}

void TagList::rebuildTags()
{
	auto& sa = getMainController()->getUserPresetHandler().getTagList();

	tags.clear();

	for (auto n : sa)
	{
		ScopedPointer<Tag> nt = new Tag(*this, n);
		addAndMakeVisible(nt);
		nt->setActive(currentlyActiveTags.contains(n));
		tags.add(nt.release());
	}

	resized();
}

void TagList::resized()
{
	auto ar = getLocalBounds();

	if(editButton.isVisible())
		editButton.setBounds(ar.removeFromRight(80).reduced(3));

	for (auto t : tags)
		t->setBounds(ar.removeFromLeft(t->getTagWidth()).reduced(5));
}

void TagList::setHighlightColourAndFont(Colour c_, Colour bgColour, Font font)
{
	f = font;
	c = c_;

	blaf.font = f;
	blaf.highlightColour = c;
}

void TagList::toggleTag(Tag* n)
{
	if (editMode)
	{
		if (!currentFile.existsAsFile())
			return;

		bool shouldBeInActive = currentlyActiveTags.contains(n->name);

		if (shouldBeInActive)
			currentlyActiveTags.removeString(n->name);
		else
			currentlyActiveTags.add(n->name);

		n->setActive(!shouldBeInActive);

		PresetBrowser::DataBaseHelpers::writeTagsInXml(currentFile, currentlyActiveTags);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->tagCacheNeedsRebuilding();
		}
	}
	else
	{
		bool shouldBeSelected = !n->selected;

		n->setSelected(shouldBeSelected);

		if (currentlySelectedTags.contains(n->name))
			currentlySelectedTags.removeString(n->name);
		else
			currentlySelectedTags.add(n->name);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->tagSelectionChanged(currentlySelectedTags);
		}
	}
}

} // namespace hise
