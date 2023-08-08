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

					auto xml = c.createXml();

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
				auto xml = XmlDocument::parse(f);

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



PresetBrowser::ModalWindow::ModalWindow(PresetBrowser* p) :
	PresetBrowserChildComponentBase(p)
{
	alaf = PresetHandler::createAlertWindowLookAndFeel();

	addAndMakeVisible(inputLabel = new BetterLabel(p));
	addAndMakeVisible(okButton = new TextButton("OK"));
	addAndMakeVisible(cancelButton = new TextButton("Cancel"));

	inputLabel->setEditable(true, true);
	inputLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::transparentBlack);
	inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::white);
	inputLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
	inputLabel->setColour(TextEditor::ColourIds::highlightColourId, Colours::white);
	inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white);
	inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	inputLabel->setFont(getPresetBrowserLookAndFeel().font);

	okButton->addListener(this);
	cancelButton->addListener(this);

	okButton->setLookAndFeel(alaf);
	cancelButton->setLookAndFeel(alaf);

	inputLabel->refreshWithEachKey = false;

	setWantsKeyboardFocus(true);

	setAlwaysOnTop(true);
}

PresetBrowser::ModalWindow::~ModalWindow()
{
	inputLabel = nullptr;
	okButton = nullptr;
	cancelButton = nullptr;
}

void PresetBrowser::ModalWindow::paint(Graphics& g)
{
	auto area = inputLabel->getBounds().expanded(50);
	Rectangle<int> labelArea(inputLabel->isVisible() ? inputLabel->getBounds() : Rectangle<int>());

	auto title = getTitleText();
	auto command = getCommand();

	getPresetBrowserLookAndFeel().drawModalOverlay(g, area, labelArea, title, command);
}

juce::String PresetBrowser::ModalWindow::getCommand() const
{
	auto le = stack.getLast();

	switch (le.currentAction)
	{
	case PresetBrowser::ModalWindow::Action::Idle:
		break;
	case PresetBrowser::ModalWindow::Action::Rename:
	case PresetBrowser::ModalWindow::Action::Add:
		return "Enter the name";
	case PresetBrowser::ModalWindow::Action::Delete:
		return "Are you sure you want to delete the file " + le.newFile.getFileNameWithoutExtension() + "?";
	case PresetBrowser::ModalWindow::Action::Replace:
		return "Are you sure you want to replace the file " + le.newFile.getFileNameWithoutExtension() + "?";
	case PresetBrowser::ModalWindow::Action::numActions:
		break;
	default:
		break;
	}

	return "";
}

juce::String PresetBrowser::ModalWindow::getTitleText() const
{
	String s;

	StackEntry le = stack.getLast();

	switch (le.currentAction)
	{
	case PresetBrowser::ModalWindow::Action::Idle:
		break;
	case PresetBrowser::ModalWindow::Action::Rename:
		s << "Rename ";
		break;
	case PresetBrowser::ModalWindow::Action::Add:
		s << "Add new ";
		break;
	case PresetBrowser::ModalWindow::Action::Replace:
		s << "Replace ";
		break;
	case PresetBrowser::ModalWindow::Action::Delete:
		s << "Delete ";
		break;
	case PresetBrowser::ModalWindow::Action::numActions:
		break;
	default:
		break;
	}

	if (le.columnIndex == 2)
		s << "User Preset";
	if (le.columnIndex == 1)
		s << "Category";
	if (le.columnIndex == 0)
		s << "Bank";

	return s;
}

bool PresetBrowser::ModalWindow::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::returnKey))
	{
		okButton->triggerClick();
		return true;
	}

	if (key.isKeyCode(KeyPress::escapeKey))
	{
		cancelButton->triggerClick();
		return true;
	}

	return false;
}

void PresetBrowser::ModalWindow::buttonClicked(Button* b)
{
	auto le = stack.getLast();

	stack.removeLast();

	auto p = findParentComponentOfClass<PresetBrowser>();

	if (b == okButton)
	{
		auto text = inputLabel->getText();

		switch (le.currentAction)
		{
		case PresetBrowser::ModalWindow::Action::Idle:
			jassertfalse;
			break;
		case PresetBrowser::ModalWindow::Action::Rename:
			p->renameEntry(le.columnIndex, le.rowIndex, inputLabel->getText());
			break;
		case PresetBrowser::ModalWindow::Action::Add:
			p->addEntry(le.columnIndex, inputLabel->getText());
			break;
		case PresetBrowser::ModalWindow::Action::Delete:
			p->deleteEntry(le.columnIndex, le.oldFile);
			break;
		case PresetBrowser::ModalWindow::Action::Replace:
		{
			auto note = DataBaseHelpers::getNoteFromXml(le.newFile);
			auto tags = DataBaseHelpers::getTagsFromXml(le.newFile);

			le.oldFile.moveFileTo(le.newFile);

			if (note.isNotEmpty())
				DataBaseHelpers::writeNoteInXml(le.newFile, note);

			if (!tags.isEmpty())
				DataBaseHelpers::writeTagsInXml(le.newFile, tags);

			if (le.oldFile.getFileName() == "tempFileBeforeMove.preset")
				le.oldFile.deleteFile();

			p->rebuildAllPresets();
			break;
		}
		case PresetBrowser::ModalWindow::Action::numActions:
			break;
		default:
			break;
		}
	}

	if (le.currentAction == Action::Replace && le.oldFile.getFileName() == "tempFileBeforeMove.preset")
		le.oldFile.deleteFile();

	refreshModalWindow();
}

void PresetBrowser::ModalWindow::resized()
{
	inputLabel->centreWithSize(300, 30);

	okButton->setBounds(inputLabel->getX(), inputLabel->getBottom() + 20, 100, 30);
	cancelButton->setBounds(inputLabel->getRight() - 100, inputLabel->getBottom() + 20, 100, 30);
}

void PresetBrowser::ModalWindow::refreshModalWindow()
{
	auto le = stack.getLast();

	inputLabel->setVisible(le.currentAction == Action::Rename || le.currentAction == Action::Add);

	setVisible(le.currentAction != Action::Idle);

	repaint();

	if (inputLabel->isVisible())
		inputLabel->showEditor();
	else if (isShowing())
		grabKeyboardFocus();
}

void PresetBrowser::ModalWindow::addActionToStack(Action actionToDo, const String& preEnteredText/*=String()*/, int newColumnIndex/*=-1*/, int newRowIndex/*=-1*/)
{
	inputLabel->setText(preEnteredText, dontSendNotification);

	StackEntry ne;

	ne.currentAction = actionToDo;
	ne.columnIndex = newColumnIndex;
	ne.rowIndex = newRowIndex;

	stack.add(ne);

	refreshModalWindow();
}

void PresetBrowser::ModalWindow::confirmDelete(int columnIndex, const File& fileToDelete)
{
	StackEntry ne;

	ne.currentAction = Action::Delete;
	ne.oldFile = fileToDelete;
	ne.columnIndex = columnIndex;

	stack.add(ne);
	refreshModalWindow();
}

void PresetBrowser::ModalWindow::confirmReplacement(const File& oldFile, const File& newFile)
{
	StackEntry ne;

	ne.oldFile = oldFile;
	ne.newFile = newFile;

	ne.currentAction = Action::Replace;
	ne.columnIndex = -1;
	ne.rowIndex = -1;

	stack.add(ne);
	refreshModalWindow();
}

PresetBrowser::PresetBrowser(MainController* mc, int width, int height) :
ControlledObject(mc),
expHandler(mc->getExpansionHandler())
{
	setName("Preset Browser");

#if USE_BACKEND
	rootFile = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else

    try
    {
        rootFile = FrontendHandler::getUserPresetDirectory();
    }
    catch(String& s)
    {
		getMainController()->sendOverlayMessage(DeactiveOverlay::State::CriticalCustomErrorMessage, s);
    }

	if (auto e = FullInstrumentExpansion::getCurrentFullExpansion(mc))
		rootFile = e->getSubDirectory(FileHandlerBase::UserPresets);

#endif

	mc->getUserPresetHandler().getTagDataBase().setRootDirectory(rootFile);

	loadPresetDatabase(rootFile);

	getMainController()->getUserPresetHandler().addListener(this);

	addAndMakeVisible(bankColumn = new PresetBrowserColumn(mc, this, 0, rootFile, this));
	addAndMakeVisible(categoryColumn = new PresetBrowserColumn(mc, this, 1, rootFile, this));
	addAndMakeVisible(presetColumn = new PresetBrowserColumn(mc, this, 2, rootFile, this));
	addAndMakeVisible(searchBar = new PresetBrowserSearchBar(this));
	addChildComponent(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	addAndMakeVisible(noteLabel = new BetterLabel(this));
	noteLabel->addListener(this);

	noteLabel->setEditable(true, true);
	noteLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	noteLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	noteLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::transparentBlack);
	noteLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::white);
	noteLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	noteLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
	noteLabel->setJustificationType(Justification::centred);

	addAndMakeVisible(tagList = new TagList(mc, this));

	addAndMakeVisible(favoriteButton = new ShapeButton("Show Favorites", Colours::white, Colours::white, Colours::white));
	favoriteButton->addListener(this);

	addAndMakeVisible(modalInputWindow = new ModalWindow(this));
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

	addAndMakeVisible(saveButton = new TextButton("Save Preset"));
	saveButton->addListener(this);

  addAndMakeVisible(manageButton = new TextButton(HiseDeviceSimulator::isMobileDevice() ? "Sync" : "More"));
	manageButton->addListener(this);

	setSize(width, height);

	defaultRoot = rootFile;

	if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
	{
		rootFile = e->getSubDirectory(FileHandlerBase::UserPresets);
		currentlySelectedExpansion = e;
	}
	
	bankColumn->setNewRootDirectory(rootFile);

	rebuildAllPresets();

	showLoadedPreset();

	updateFavoriteButton();

	setOpaque(false);
	setLookAndFeel(&laf);

	if(getMainController()->getExpansionHandler().isEnabled())
		expHandler.addListener(this); //Setup expansion handler listener
	
	

}

PresetBrowser::~PresetBrowser()
{
	getMainController()->getUserPresetHandler().removeListener(this);

	if(rootFile.isDirectory())
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

	setLookAndFeel(nullptr);

	expHandler.removeListener(this);
}

bool PresetBrowser::isReadOnly(const File& f)
{
	if (rootFile == f)
		return false;

	return getMainController()->getUserPresetHandler().isReadOnly(f);
}

void PresetBrowser::expansionPackLoaded(Expansion* currentExpansion)
{
	if(expansionColumn != nullptr && currentExpansion != nullptr)
		selectionChanged(-1, -1, currentExpansion->getRootFolder(), false);
	else
		selectionChanged(-1, -1, File(), false);
}

void PresetBrowser::expansionPackCreated(Expansion* newExpansion)
{
	if (expansionColumn != nullptr)
		expansionColumn->update();
}

hise::PresetBrowserLookAndFeelMethods& PresetBrowser::getPresetBrowserLookAndFeel()
{
	if (auto o = dynamic_cast<PresetBrowserLookAndFeelMethods*>(&getLookAndFeel()))
		return *o;

	return laf;
}

Point<int> PresetBrowser::getMouseHoverInformation() const
{
	Point<int> p(-9000, -9000);

	auto mp = getMouseXYRelative();

	auto setIfHover = [&](PresetBrowserColumn* c)
	{
		if (c == nullptr)
			return false;

		if (!c->isVisible())
			return false;

		if(c->getBoundsInParent().contains(mp))
		{
			auto lp = c->getLocalPoint(this, mp);
			auto offset = c->getListAreaOffset();

			lp.setY(lp.getY() -(int)offset[1]);

			p = { c->getColumnIndex(), c->getIndexForPosition(lp) };
			return true;
		};

		return false;
	};

	if (setIfHover(expansionColumn))
		return p;

	if (setIfHover(bankColumn))
		return p;

	if (setIfHover(categoryColumn))
		return p;

	if (setIfHover(presetColumn))
		return p;
	
	return p;
}

void PresetBrowser::presetChanged(const File& newPreset)
{
	if (allPresets[currentlyLoadedPreset] == newPreset)
	{
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);
		return;
	}

	File pFile = newPreset;
	File cFile;
	File bFile;

	if (numColumns > 2)
	{
		cFile = pFile.getParentDirectory();
	}

	if (numColumns > 1)
	{
		bFile = numColumns > 2 ? cFile.getParentDirectory() : pFile.getParentDirectory();
		bankColumn->setSelectedFile(bFile, sendNotification);
	}

	// For some reason I needed to call twice the same condition...
	if (numColumns > 2)
	{
		categoryColumn->setSelectedFile(cFile, sendNotification);
	}
	
	presetColumn->setSelectedFile(newPreset, dontSendNotification);

	saveButton->setEnabled(!isReadOnly(newPreset));

	noteLabel->setText(DataBaseHelpers::getNoteFromXml(newPreset), dontSendNotification);
}

void PresetBrowser::presetListUpdated()
{
	rebuildAllPresets();
}

void PresetBrowser::paint(Graphics& g)
{
	getPresetBrowserLookAndFeel().drawPresetBrowserBackground(g, this);
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

	File f = getMainController()->getUserPresetHandler().getCurrentlyLoadedFile();

	currentlyLoadedPreset = allPresets.indexOf(f);

	if (numColumns == 1)
	{
		presetColumn->setNewRootDirectory(rootFile);

		presetColumn->setEditMode(false);
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

		bankColumn->setEditMode(false);
		presetColumn->updateButtonVisibility(isReadOnly(f));
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

	if (searchBarBounds.size() == 4)
		searchBar->setBounds((int)searchBarBounds[0], (int)searchBarBounds[1], (int)searchBarBounds[2], (int)searchBarBounds[3]);
		
	if (showCloseButton)
	{

#if HISE_IOS
		closeButton->setBounds(getWidth() - 30, 6, 24, 24);
#else
		closeButton->setBounds(getWidth() - 35, 5, 20, 20);
#endif

		y += 30;
		Rectangle<int> ar(3, y + 5, getWidth() - 6, 30);

		saveButton->setBounds(ar.removeFromRight(100));
		manageButton->setBounds(ar.removeFromLeft(100));
		
		if (searchBarBounds.size() != 4)
			searchBar->setBounds(ar);

		y += 40;

	}
	else
	{
		Rectangle<int> ar(3, 3 + 3, getWidth() - 6, 30);


		bool somethingInTopRow = false;

		auto r = Result::ok();

		auto saveBounds = ApiHelpers::getIntRectangleFromVar(saveButtonBounds, &r);
		
		if (r.wasOk())
			saveButton->setBounds(saveBounds);
		else
			saveButton->setBounds(ar.removeFromRight(100));

		r = Result::ok();
		
		auto moreBounds = ApiHelpers::getIntRectangleFromVar(moreButtonBounds, &r);

		if (r.wasOk())
			manageButton->setBounds(moreBounds);
		else
			manageButton->setBounds(ar.removeFromLeft(100));

		r = Result::ok();

		favoriteButton->setVisible(showFavoritesButton);

		if (showFavoritesButton)
		{
			auto bounds = ApiHelpers::getIntRectangleFromVar(favoriteButtonBounds, &r);

			if (r.wasOk())
				favoriteButton->setBounds(bounds);
			else
				favoriteButton->setBounds(ar.removeFromLeft(30));
		}

		ar.removeFromLeft(10);

		if (searchBarBounds.size() != 4)
			searchBar->setBounds(ar);

		somethingInTopRow |= saveButton->isVisible();
		somethingInTopRow |= manageButton->isVisible();
		somethingInTopRow |= showFavoritesButton;
		somethingInTopRow |= searchBar->getHeight() > 0;

		if(somethingInTopRow)
			y += 40;
	}

	int x = 3;

	
	

	bankColumn->setVisible(!showOnlyPresets && numColumns > 1);
	categoryColumn->setVisible(!showOnlyPresets && numColumns > 2);
	presetColumn->setIsResultBar(showOnlyPresets);

	auto listArea = Rectangle<int>(x, y, getWidth() - 6, getHeight() - y - 3);

	

	if (noteLabel->isVisible())
	{
		auto labelArea = listArea.removeFromTop(40);
		noteLabel->setBounds(labelArea.reduced(3, 5));
	}

	if (tagList->isActive())
		tagList->setBounds(listArea.removeFromTop(30));

	if (showOnlyPresets)
	{
		if (expansionColumn != nullptr)
			listArea.removeFromLeft(expansionColumn->getWidth() + 4);

		presetColumn->setBounds(listArea.reduced(2));
	}
	else
	{
		const int folderOffset = expansionColumn != nullptr ? 1 : 0;
		const int numColumnsToShow = jlimit(1, 4, numColumns + folderOffset);
		int columnWidths[4] = { 0, 0, 0, 0 };
		auto w = (double)getWidth();

		if (columnWidthRatios.size() == numColumnsToShow)
		{
			for (int i = 0; i < numColumnsToShow; i++)
			{
				auto r = jlimit(0.0, 1.0, (double)columnWidthRatios[i]);
				columnWidths[i] = roundToInt(w * r);
			}
		}
		else
		{
			// column amount mismatch, use equal spacing...
			const int columnWidth = roundToInt(w / (double)numColumnsToShow);

			for (int i = 0; i < numColumnsToShow; i++)
				columnWidths[i] = columnWidth;
		}

		if(expansionColumn != nullptr)
			expansionColumn->setBounds(listArea.removeFromLeft(columnWidths[0]).reduced(2, 2));

		if(numColumns > 1)
			bankColumn->setBounds(listArea.removeFromLeft(columnWidths[0+folderOffset]).reduced(2, 2));

		if(numColumns > 2)
			categoryColumn->setBounds(listArea.removeFromLeft(columnWidths[numColumnsToShow-2]).reduced(2, 2));

		presetColumn->setBounds(listArea.removeFromLeft(columnWidths[numColumnsToShow-1]).reduced(2, 2));
	}


}


void PresetBrowser::tagSelectionChanged(const StringArray& newSelection)
{
	currentTagSelection = newSelection;

	showOnlyPresets = !currentTagSelection.isEmpty() || currentWildcard != "*" || favoriteButton->getToggleState();

	resized();
}

void PresetBrowser::attachAdditionalMouseProperties(const MouseEvent& e, var& obj)
{
    auto dyn = obj.getDynamicObject();
    
    jassert(dyn != nullptr);
    
    // feel free to add more stuff here...
    
    if(auto lb = e.eventComponent->findParentComponentOfClass<ListBox>())
    {
        int rowNumber = lb->getRowNumberOfComponent(e.eventComponent);
        auto column = e.eventComponent->findParentComponentOfClass<PresetBrowserColumn>();
        int columnIndex = column->getColumnIndex();
				String file = column->getFileForIndex(rowNumber).getFullPathName();
				
        dyn->setProperty("target", "listItem");
        dyn->setProperty("rowIndex", rowNumber);
        dyn->setProperty("columnIndex", columnIndex);
				dyn->setProperty("file", file);			
        
        return;
    }
    
    if(e.eventComponent == favoriteButton)
    {
        dyn->setProperty("target", "favoriteButton");
        dyn->setProperty("buttonState", favoriteButton->getToggleState());
        return;
    }
		
		if(e.eventComponent == saveButton)
		{
				dyn->setProperty("target", "saveButton");
				return;
		}
}

void PresetBrowser::labelTextChanged(Label* l)
{
	if (l == noteLabel)
	{
		auto currentPreset = allPresets[currentlyLoadedPreset];
		auto newNote = noteLabel->getText();

		DataBaseHelpers::writeNoteInXml(currentPreset, newNote);
	}
	else
	{
		showOnlyPresets = !currentTagSelection.isEmpty() || l->getText().isNotEmpty() || favoriteButton->getToggleState();

		if (showOnlyPresets)
			currentWildcard = "*" + l->getText() + "*";
		else
			currentWildcard = "*";

		resized();
	}
}

void PresetBrowser::updateFavoriteButton()
{
	const bool on = favoriteButton->getToggleState();

	showOnlyPresets = currentWildcard != "*" || on;

	auto path = getPresetBrowserLookAndFeel().createPresetBrowserIcons(on ? "favorite_on" : "favorite_off");


	favoriteButton->setShape(path, false, true, true);

	if (presetColumn == nullptr)
		return;

	presetColumn->setShowFavoritesOnly(on);

	resized();
}

void PresetBrowser::lookAndFeelChanged()
{
	Component::lookAndFeelChanged();
	updateFavoriteButton();
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

void PresetBrowser::setShowFavorites(bool shouldShowFavorites)
{
	showFavoritesButton = shouldShowFavorites;
}

void PresetBrowser::setHighlightColourAndFont(Colour c, Colour bgColour, Font f)
{
	auto& lf = getPresetBrowserLookAndFeel();

	lf.backgroundColour = bgColour;
	lf.font = f;
	lf.highlightColour = c;

	favoriteButton->setColours(c.withAlpha(0.7f), c.withAlpha(0.5f), c.withAlpha(0.6f));

	setOpaque(bgColour.isOpaque());
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


void PresetBrowser::setShowButton(int buttonId, bool newValue)
{
	enum ButtonIndexes
	{
		ShowFolderButton = 0,
		SaveButton,
		numButtonsToShow
	};

	if (buttonId == SaveButton)
		saveButton->setVisible(newValue);
	else if (buttonId == ShowFolderButton)
		manageButton->setVisible(newValue);

	resized();
}

void PresetBrowser::setShowNotesLabel(bool shouldBeShown)
{
	if (shouldBeShown != noteLabel->isVisible())
	{
		noteLabel->setVisible(shouldBeShown);
		resized();
	}
}

void PresetBrowser::setShowEditButtons(int buttonId, bool show)
{	
	if (expansionColumn != nullptr)
		expansionColumn->setShowButtons(buttonId, show);

	bankColumn->setShowButtons(buttonId, show);
	categoryColumn->setShowButtons(buttonId, show);
	presetColumn->setShowButtons(buttonId, show);
	
	if (buttonId == 0)
		tagList->setShowEditButton(show);
}

void PresetBrowser::setButtonsInsideBorder(bool inside)
{
	if (expansionColumn != nullptr)
		expansionColumn->setButtonsInsideBorder(inside);

	bankColumn->setButtonsInsideBorder(inside);
	categoryColumn->setButtonsInsideBorder(inside);
	presetColumn->setButtonsInsideBorder(inside);
}

void PresetBrowser::setEditButtonOffset(int offset)
{
	if (expansionColumn != nullptr)
		expansionColumn->setEditButtonOffset(offset);

	bankColumn->setEditButtonOffset(offset);
	categoryColumn->setEditButtonOffset(offset);
	presetColumn->setEditButtonOffset(offset);
}

void PresetBrowser::setListAreaOffset(Array<var> offset)
{	
	if (expansionColumn != nullptr)
		expansionColumn->setListAreaOffset(offset);

	bankColumn->setListAreaOffset(offset);
	categoryColumn->setListAreaOffset(offset);
	presetColumn->setListAreaOffset(offset);
}

Array<var> PresetBrowser::getListAreaOffset()
{	
	return presetColumn->getListAreaOffset();
}

void PresetBrowser::setColumnRowPadding(Array<var> padding)
{	
	if (expansionColumn != nullptr)
		expansionColumn->setRowPadding((double)padding[0]);

	bankColumn->setRowPadding((double)padding[1]);
	categoryColumn->setRowPadding((double)padding[2]);
	presetColumn->setRowPadding((double)padding[3]);
}

void PresetBrowser::setShowCloseButton(bool shouldShowButton)
{
	if (shouldShowButton != closeButton->isVisible())
	{
		closeButton->setVisible(shouldShowButton);
		resized();
	}
}

void PresetBrowser::incPreset(bool next, bool stayInSameDirectory/*=false*/)
{
	getMainController()->getUserPresetHandler().incPreset(next, stayInSameDirectory);
}

void PresetBrowser::setCurrentPreset(const File& f, NotificationType /*sendNotification*/)
{
	int newIndex = allPresets.indexOf(f);

	if (newIndex != -1)
	{
		currentlyLoadedPreset = newIndex;
		presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);
	}
}

void PresetBrowser::openModalAction(ModalWindow::Action action, const String& preEnteredText, const File& fileToChange, int columnIndex, int rowIndex)
{
	if (action == ModalWindow::Action::Delete)
		modalInputWindow->confirmDelete(columnIndex, fileToChange);
	else
		modalInputWindow->addActionToStack(action, preEnteredText, columnIndex, rowIndex);
}

void PresetBrowser::confirmReplace(const File& oldFile, const File &newFile)
{
	modalInputWindow->confirmReplacement(oldFile, newFile);
}

void PresetBrowser::showLoadedPreset()
{
	if (currentlyLoadedPreset != -1)
	{
		File f = allPresets[currentlyLoadedPreset];

		File category = f.getParentDirectory();
		File bank = category.getParentDirectory();

		if (numColumns == 2)
			bank = category;

		bankColumn->setSelectedFile(bank, dontSendNotification);
		categoryColumn->setNewRootDirectory(bank);
		categoryColumn->setSelectedFile(category, dontSendNotification);
		presetColumn->setNewRootDirectory(category);
		presetColumn->setSelectedFile(f, dontSendNotification);
		saveButton->setEnabled(!isReadOnly(f));

		if (expansionColumn != nullptr)
		{
			if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
			{
				expansionColumn->setSelectedFile(e->getRootFolder(), dontSendNotification);
			}
				
		}
	}
}

void PresetBrowser::setOptions(const Options& newOptions)
{
	if (newOptions.showExpansions)
	{
		auto expRoot = getMainController()->getExpansionHandler().getExpansionFolder();
		addAndMakeVisible(expansionColumn = new PresetBrowserColumn(getMainController(), this, -1, expRoot, this));
		expansionColumn->setModel(new PresetBrowserColumn::ExpansionColumnModel(this), expRoot);

		expansionColumn->update();

		showLoadedPreset();
	}
		
	else
		expansionColumn = nullptr;

	setHighlightColourAndFont(newOptions.highlightColour, newOptions.backgroundColour, newOptions.font);

	getPresetBrowserLookAndFeel().textColour = newOptions.textColour;
	setNumColumns(newOptions.numColumns);
	columnWidthRatios.clear();
	columnWidthRatios.addArray(newOptions.columnWidthRatios);
	
	searchBarBounds.clear();
	searchBarBounds.addArray(newOptions.searchBarBounds);

	favoriteButtonBounds.clear();
	favoriteButtonBounds.addArray(newOptions.favoriteButtonBounds);
	
	saveButtonBounds.clear();
	saveButtonBounds.addArray(newOptions.saveButtonBounds);
	moreButtonBounds.clear();
	moreButtonBounds.addArray(newOptions.moreButtonBounds);

	setShowButton(0, newOptions.showFolderButton);
	setShowButton(1, newOptions.showSaveButtons);
	setShowEditButtons(0, newOptions.showEditButtons);
	setShowEditButtons(1, newOptions.showAddButton);
	setShowEditButtons(2, newOptions.showRenameButton);
	setShowEditButtons(3, newOptions.showDeleteButton);
	setButtonsInsideBorder(newOptions.buttonsInsideBorder);
	setEditButtonOffset(newOptions.editButtonOffset);
	setListAreaOffset(newOptions.listAreaOffset);
	setColumnRowPadding(newOptions.columnRowPadding);
	setShowNotesLabel(newOptions.showNotesLabel);
	setShowFavorites(newOptions.showFavoriteIcons);

	if (expansionColumn != nullptr)
		expansionColumn->update();

	searchBar->update();
	bankColumn->update();
	categoryColumn->update();
	presetColumn->update();

	noteLabel->update();
	tagList->update();
	modalInputWindow->update();

	resized();
}

void PresetBrowser::selectionChanged(int columnIndex, int /*rowIndex*/, const File& file, bool /*doubleClick*/)
{
	const bool showCategoryColumn = numColumns == 3;

	auto readOnly = isReadOnly(file);

	if (columnIndex == -1) // Expansions
	{
		currentBankFile = File();
		currentCategoryFile = File();
		currentlyLoadedPreset = 0;
		
		if (file == File())
		{
			if (FullInstrumentExpansion::isEnabled(getMainController()))
				rootFile = File();
			else
				rootFile = defaultRoot;
				
			currentlySelectedExpansion = nullptr;
		}
		else
		{
			// Already selected, don't do nothing...
			if (rootFile.isAChildOf(file))
				return;

			rootFile = file.getChildFile("UserPresets");
			currentlySelectedExpansion = getMainController()->getExpansionHandler().getExpansionFromRootFile(file);
		}

		if(expansionColumn != nullptr)
			expansionColumn->repaint();

		bankColumn->setModel(new PresetBrowserColumn::ColumnListModel(this, 0, this), rootFile);
		bankColumn->setNewRootDirectory(rootFile);
		categoryColumn->setModel(new PresetBrowserColumn::ColumnListModel(this, 1, this), rootFile);
		categoryColumn->setNewRootDirectory(currentCategoryFile);
		presetColumn->setNewRootDirectory(File());
		
		auto pc = new PresetBrowserColumn::ColumnListModel(this, 2, this);
		pc->setDisplayDirectories(false);
		presetColumn->setModel(pc, rootFile);
		
		loadPresetDatabase(file.getChildFile("UserPresets"));
		presetColumn->setDatabase(getDataBase());
		rebuildAllPresets();
	}

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

			bankColumn->updateButtonVisibility(readOnly);
			bankColumn->showAddButton();

			noteLabel->setText("", dontSendNotification);
		}
		else
		{
			presetColumn->setNewRootDirectory(currentBankFile);

			presetColumn->setEditMode(false);
			presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

			bankColumn->setEditMode(false);
			bankColumn->updateButtonVisibility(readOnly);
			bankColumn->showAddButton();
			
			presetColumn->updateButtonVisibility(readOnly);
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

		categoryColumn->updateButtonVisibility(readOnly);
		presetColumn->updateButtonVisibility(readOnly);

		noteLabel->setText("", dontSendNotification);
	}
	else if (columnIndex == 2)
	{
		getMainController()->getExpansionHandler().setCurrentExpansion(currentlySelectedExpansion, sendNotificationSync);

		loadPreset(file);

		bankColumn->setEditMode(false);
		categoryColumn->setEditMode(false);

		presetColumn->updateButtonVisibility(readOnly);
	}
}


void PresetBrowser::renameEntry(int columnIndex, int rowIndex, const String& newName)
{
	if (columnIndex == 0 && (numColumns == 3 || numColumns == 2))
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
	else if (columnIndex == 1 && numColumns == 3)
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
	else if (columnIndex == 2 || (columnIndex == 1 && numColumns == 2) || (columnIndex == 0 && numColumns == 1))
	{
		File current;

		if (numColumns == 3)
			current = currentCategoryFile;
		else if (numColumns == 2)
		 	current = currentBankFile;
		else if (numColumns == 1)
		 	current = rootFile;

		auto presetFile = getMainController()->getUserPresetHandler().getCurrentlyLoadedFile();


		//File presetFile = PresetBrowserColumn::getChildDirectory(current, 3, rowIndex);

		if (presetFile.existsAsFile() && newName.isNotEmpty())
		{
			File newFile = presetFile.getSiblingFile(newName + ".preset");

			if (newFile.existsAsFile())
				modalInputWindow->confirmReplacement(presetFile, newFile);
			else
			{
				presetFile.moveFileTo(newFile);
				presetColumn->setNewRootDirectory(current);
				rebuildAllPresets();
				showLoadedPreset();
			}
		}
	}
}

void PresetBrowser::deleteEntry(int columnIndex, const File& f)
{
	if (columnIndex == 0 && (numColumns == 3 || numColumns == 2))
	{
		File bankToDelete = f;

		bankToDelete.deleteRecursively();

		bankColumn->setNewRootDirectory(rootFile);
		categoryColumn->setNewRootDirectory(File());
		presetColumn->setNewRootDirectory(File());
	}
	else if (columnIndex == 1 && numColumns == 3)
	{
		File categoryToDelete = f;

		categoryToDelete.deleteRecursively();

		categoryColumn->setNewRootDirectory(currentBankFile);
		presetColumn->setNewRootDirectory(File());

	}
	else if (columnIndex == 2 || (columnIndex == 1 && numColumns == 2) || (columnIndex == 0 && numColumns == 1))
	{
		File presetFile = f;

		File current;

		if (numColumns == 3)
			current = currentCategoryFile;
		else if (numColumns == 2)
		 	current = currentBankFile;
		else if (numColumns == 1)
		 	current = rootFile;

		presetFile.deleteFile();
		presetColumn->setNewRootDirectory(current);
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
		if (getMainController()->getUserPresetHandler().getCurrentlyLoadedFile().existsAsFile())
		{
			auto fileToBeReplaced = getMainController()->getUserPresetHandler().getCurrentlyLoadedFile();
			File tempFile = fileToBeReplaced.getSiblingFile("tempFileBeforeMove.preset");

			UserPresetHelpers::saveUserPreset(getMainController()->getMainSynthChain(), tempFile.getFullPathName(), dontSendNotification);
			confirmReplace(tempFile, fileToBeReplaced);
		}
	}
	else if (b == manageButton)
	{
		PopupMenu p;

		auto& plaf = getMainController()->getGlobalLookAndFeel();
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
            p.addSeparator();

            p.addItem(ImportPresetsFromClipboard, "Import " + destination + " from Clipboard");
            p.addItem(ExportPresetsToClipboard, "Export " + destination + " to Clipboard");

            p.addSeparator();

			p.addItem(ImportPresetsFromFile, "Import " + destination + " from Collection");
			p.addItem(ExportPresetsToFile, "Export " + destination + " as Collection");
		}

        auto result = (ID)PopupLookAndFeel::showAtComponent(p, b, true);

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
		UserPresetHelpers::loadUserPreset(getMainController()->getMainSynthChain(), f);
        currentlyLoadedPreset = allPresets.indexOf(f);

		noteLabel->setText(DataBaseHelpers::getNoteFromXml(f), dontSendNotification);
		saveButton->setEnabled(!isReadOnly(f));
    }
}




void PresetBrowser::DataBaseHelpers::setFavorite(const var& database, const File& presetFile, bool isFavorite)
{
	if (auto data = database.getDynamicObject())
	{
		auto id = getIdForFile(presetFile);

		if (id.isNull())
			return;

		if (auto entry = data->getProperty(id).getDynamicObject())
		{
			entry->setProperty("Favorite", isFavorite);
		}
		else
		{
			auto e = new DynamicObject();

			e->setProperty("Favorite", isFavorite);
			data->setProperty(id, e);
		}
	}
}

void PresetBrowser::DataBaseHelpers::cleanFileList(MainController* mc, Array<File>& filesToClean)
{
	for (int i = 0; i < filesToClean.size(); i++)
	{
		const bool isNoPresetFile = filesToClean[i].isHidden() || filesToClean[i].getFileName().startsWith(".") || filesToClean[i].getFileExtension() != ".preset";
		const bool isNoDirectory = !filesToClean[i].isDirectory();

		const bool requiresMissingExpansions = mc != nullptr && !matchesAvailableExpansions(mc, filesToClean[i]);

		if ((isNoPresetFile && isNoDirectory) || requiresMissingExpansions)
		{
			filesToClean.remove(i--);
			continue;
		}
	}
}

void PresetBrowser::DataBaseHelpers::writeTagsInXml(const File& currentPreset, const StringArray& tags)
{
	if (currentPreset.existsAsFile())
	{
		auto xml = XmlDocument::parse(currentPreset);

		if (xml != nullptr)
		{
			xml->setAttribute("Tags", tags.joinIntoString(";"));

			auto newPresetContent = xml->createDocument("");

			currentPreset.replaceWithText(newPresetContent);
		}
	}
}

bool PresetBrowser::DataBaseHelpers::matchesTags(const StringArray& currentlyActiveTags, const File& presetToTest)
{
	if (currentlyActiveTags.isEmpty())
		return true;

	auto presetTags = getTagsFromXml(presetToTest);

	if (presetTags.isEmpty())
		return false;

	for (auto t : currentlyActiveTags)
		if (!presetTags.contains(t))
			return false;

	return true;
}

void PresetBrowser::DataBaseHelpers::writeNoteInXml(const File& currentPreset, const String& newNote)
{
	if (currentPreset.existsAsFile())
	{
		auto xml = XmlDocument::parse(currentPreset);

		if (xml != nullptr)
		{
			xml->setAttribute("Notes", newNote);

			auto newPresetContent = xml->createDocument("");

			currentPreset.replaceWithText(newPresetContent);
		}
	}
}

juce::StringArray PresetBrowser::DataBaseHelpers::getTagsFromXml(const File& currentPreset)
{
	StringArray sa;

	if (currentPreset.existsAsFile())
	{
		auto content = currentPreset.loadFileAsString();

		static const String tagAttribute = "Tags=\"";

		if (content.contains(tagAttribute))
		{
			auto tags = content.fromFirstOccurrenceOf(tagAttribute, false, false).upToFirstOccurrenceOf("\"", false, false);
			sa = StringArray::fromTokens(tags, ";", "");
		}
	}

	return sa;
}

juce::String PresetBrowser::DataBaseHelpers::getNoteFromXml(const File& currentPreset)
{
	if (currentPreset.existsAsFile())
	{
		auto xml = XmlDocument::parse(currentPreset);

		if (xml != nullptr)
			return xml->getStringAttribute("Notes", "");
	}

	return String();
}

bool PresetBrowser::DataBaseHelpers::matchesAvailableExpansions(MainController* mc, const File& currentPreset)
{
	if (!mc->getExpansionHandler().isEnabled())
		return true;

	if (mc == nullptr)
		return true;

	if (currentPreset.isDirectory())
		return true;

	auto s = currentPreset.loadFileAsString();

	auto m = s.fromFirstOccurrenceOf("RequiredExpansions=\"", false, false).upToFirstOccurrenceOf("\"", false, false);

	if (m.isNotEmpty())
	{
		auto sa = StringArray::fromTokens(m, ";", "");

		sa.removeEmptyStrings(true);

		auto& handler = mc->getExpansionHandler();

		for (int i = 0; i < handler.getNumExpansions(); i++)
		{
			int index = sa.indexOf(handler.getExpansion(i)->getProperty(ExpansionIds::Name));

			if (index != -1)
				sa.remove(index);
		}

		return sa.isEmpty();
	}

	return true;
}


bool PresetBrowser::DataBaseHelpers::isFavorite(const var& database, const File& presetFile)
{
	if (!presetFile.existsAsFile())
		return false;

	if (!presetFile.hasFileExtension(".preset"))
		return false;

	if (auto data = database.getDynamicObject())
	{
		auto id = getIdForFile(presetFile);

		if (id.isNull())
			return false;

		if (auto entry = data->getProperty(id).getDynamicObject())
		{
			return entry->getProperty("Favorite");
		}
	}

	return false;
}

juce::Identifier PresetBrowser::DataBaseHelpers::getIdForFile(const File& presetFile)
{
	if (presetFile.getFileExtension() == ".preset")
	{
		// Yo Dawg, I heard you like parent directories...
		auto rootFile = presetFile.getParentDirectory().getParentDirectory().getParentDirectory();

		auto s = presetFile.getRelativePathFrom(rootFile);

		s = s.upToFirstOccurrenceOf(".preset", false, false);
		s = s.replaceCharacter('/', '_');
		s = s.replaceCharacter('\\', '_');
		s = s.replaceCharacter('\'', '_');

		s = s.removeCharacters(" \t!+&");

		if (Identifier::isValidIdentifier(s))
		{
			return Identifier(s);
		}

		jassertfalse;
		return Identifier();
	}

	return Identifier();
}

} // namespace hise
