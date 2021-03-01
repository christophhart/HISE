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



PresetBrowserSearchBar::PresetBrowserSearchBar(PresetBrowser* p):
	PresetBrowserChildComponentBase(p)
{
	addAndMakeVisible(inputLabel = new BetterLabel(p));
	inputLabel->setEditable(true, true);
	inputLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	inputLabel->setColour(Label::ColourIds::outlineWhenEditingColourId, Colours::transparentBlack);

	inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::white);
	inputLabel->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);

	inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::transparentBlack);
}

void PresetBrowserSearchBar::paint(Graphics &g)
{
	getPresetBrowserLookAndFeel().drawSearchBar(g, getLocalBounds());

	
}




int TagList::Tag::getTagWidth() const
{
	return (int)parent.getPresetBrowserLookAndFeel().font.getStringWidthFloat(name) + 20;
}


void TagList::Tag::mouseDown(const MouseEvent&)
{
	parent.toggleTag(this);
}

void TagList::Tag::paint(Graphics& g)
{
	auto position = getLocalBounds();
	
	parent.getPresetBrowserLookAndFeel().drawTag(g, parent.on, active, selected, name, position);
}

TagList::TagList(MainController* mc_, PresetBrowser* p) :
	PresetBrowserChildComponentBase(p),
	ControlledObject(mc_),
	editButton("Edit Tags")
{
	editButton.addListener(this);
	addAndMakeVisible(editButton);

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
	auto& sa = getMainController()->getUserPresetHandler().getTagDataBase().getTagList();

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

	if (editButton.isVisible())
		editButton.setBounds(ar.removeFromRight(80).reduced(3));

	for (auto t : tags)
		t->setBounds(ar.removeFromLeft(t->getTagWidth()).reduced(5));
}

void TagList::timerCallback()
{
	on = !on;

	for (auto t : tags)
		t->repaint();
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

		parent->getMainController()->getUserPresetHandler().getTagDataBase().buildDataBase(true);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->tagCacheNeedsRebuilding();
		}
	}
	else
	{
		parent->getMainController()->getUserPresetHandler().getTagDataBase().buildDataBase();

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

PresetBrowserColumn::ColumnListModel::ColumnListModel(PresetBrowser* p, int index_, Listener* listener_) :
	PresetBrowserChildComponentBase(p),
	root(File()),
	index(index_),
	listener(listener_)
{}


int PresetBrowserColumn::ColumnListModel::getNumRows()
{
	if (wildcard.isEmpty() && currentlyActiveTags.isEmpty())
	{
		const File& rootToUse = showFavoritesOnly ? totalRoot : root;

		if (!rootToUse.isDirectory())
		{
			entries.clear();
			return 0;
		}

		entries.clear();
		rootToUse.findChildFiles(entries, displayDirectories ? File::findDirectories : File::findFiles, allowRecursiveSearch || showFavoritesOnly);

		PresetBrowser::DataBaseHelpers::cleanFileList(parent->getMainController(), entries);

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
				const auto& cachedTags = getCachedTags();

				for (auto& t : cachedTags)
				{
					if (t.hashCode == hash)
					{
						matchesTags = t.shown;
						break;
					}
				}
			}

			if (matchesWildcard && matchesTags)
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
			listener->selectionChanged(index, row, entries[row], false);
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
	

	
}

void PresetBrowserColumn::ColumnListModel::updateTags(const StringArray& newSelection)
{
	currentlyActiveTags.clear();

	for (auto s : newSelection)
		currentlyActiveTags.add(Identifier(s));

	

	const auto& cachedTags = parent->getMainController()->getUserPresetHandler().getTagDataBase().getCachedTags();

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
		auto itemName = entries[rowNumber].getFileNameWithoutExtension();
		auto position = Rectangle<int>(0, 1, width, height - 2);
		getPresetBrowserLookAndFeel().drawListItem(g, index, rowNumber, itemName, position, rowIsSelected, deleteOnClick);
	}
}

const juce::Array<PresetBrowserColumn::ColumnListModel::CachedTag>& PresetBrowserColumn::ColumnListModel::getCachedTags() const
{
	return parent->getMainController()->getUserPresetHandler().getTagDataBase().getCachedTags();
}

Component* PresetBrowserColumn::ColumnListModel::refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
	if (existingComponentToUpdate != nullptr)
		delete existingComponentToUpdate;

	if (index == 2 && parent.getComponent()->shouldShowFavoritesButton())
	{
		return new FavoriteOverlay(*this, rowNumber);
	}
	else
		return nullptr;
}

void PresetBrowserColumn::ColumnListModel::sendRowChangeMessage(int row)
{
	if (listener != nullptr)
		listener->selectionChanged(index, row, entries[row], false);
}


PresetBrowserColumn::ColumnListModel::FavoriteOverlay::FavoriteOverlay(ColumnListModel& parent_, int index_) :
	parent(parent_),
	index(index_)
{
	auto colour = parent.getPresetBrowserLookAndFeel().textColour;

	addAndMakeVisible(b = new ShapeButton("Favorite", Colours::white.withAlpha(0.2f), colour.withAlpha(0.8f), colour));

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

PresetBrowserColumn::PresetBrowserColumn(MainController* mc_, PresetBrowser* p, int index_, File& rootDirectory, ColumnListModel::Listener* listener) :
	PresetBrowserChildComponentBase(p),
	mc(mc_),
	index(index_)
{
	addAndMakeVisible(editButton = new TextButton("Edit"));
	editButton->addListener(this);

	addAndMakeVisible(addButton = new TextButton("Add"));
	addButton->addListener(this);

	addAndMakeVisible(renameButton = new TextButton("Rename"));
	renameButton->addListener(this);

	addAndMakeVisible(deleteButton = new TextButton("Delete"));
	deleteButton->addListener(this);

	listModel = new ColumnListModel(parent, index, listener);
	listModel->database = dynamic_cast<PresetBrowser*>(listener)->getDataBase();
	listModel->setTotalRoot(rootDirectory);

	startTimer(4000);

	if (index == 2)
	{
		listModel->setDisplayDirectories(false);
	}

	addAndMakeVisible(listbox = new ListBox());

	listbox->setModel(listModel);
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);

	
	listbox->setWantsKeyboardFocus(true);

	if (HiseDeviceSimulator::isMobileDevice())
		listbox->setRowSelectedOnMouseDown(false);

	listbox->getViewport()->setScrollOnDragEnabled(true);

	listbox->addMouseListener(this, true);

	setSize(150, 300);
}

File PresetBrowserColumn::getChildDirectory(File& root, int level, int index)
{
	if (!root.isDirectory()) return File();

	if (level == 0) return root;

	Array<File> childDirectories;

	root.findChildFiles(childDirectories, level > 2 ? File::findFiles : File::findDirectories, false);

	for (int i = 0; i < childDirectories.size(); i++)
	{
		if (childDirectories[i].isHidden() ||
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

void PresetBrowserColumn::touchAndHold(Point<int> /*downPosition*/)
{
	bool scrolling = listbox->getViewport()->isCurrentlyScrollingOnDrag();

	if (!scrolling && !showButtonsAtBottom)
	{
		listModel->deleteOnClick = !listModel->deleteOnClick;
		listbox->repaint();
	}
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
		parent->openModalAction(PresetBrowser::ModalWindow::Action::Add, index == 2 ? "New Preset" : "New Directory", File(), index, -1);
	}
#if !OLD_PRESET_BROWSER
	else if (b == renameButton)
	{
		int selectedIndex = listbox->getSelectedRow(0);

		if (selectedIndex >= 0)
		{
			File oldFile = listModel->getFileForIndex(selectedIndex);

			parent->openModalAction(PresetBrowser::ModalWindow::Action::Rename, oldFile.getFileNameWithoutExtension(), oldFile, index, selectedIndex);
		}
	}
	else if (b == deleteButton)
	{
		int selectedIndex = listbox->getSelectedRow(0);

		if (selectedIndex >= 0)
		{
			File f = listModel->getFileForIndex(selectedIndex);
			parent->openModalAction(PresetBrowser::ModalWindow::Action::Delete, "", f, index, selectedIndex);
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
				parent->confirmReplace(tempFile, newPreset);
			}
			else
			{
				UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), newPreset.getFullPathName());

				setNewRootDirectory(currentRoot);
				parent->rebuildAllPresets();
				parent->showLoadedPreset();
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

	String emptyText;

	StringArray columnNames = { "Expansion", "Nothing", "Bank", "Column" };

	if (currentRoot == File() && listModel->wildcard.isEmpty() && listModel->currentlyActiveTags.isEmpty())
		emptyText = "Select a " + columnNames[jlimit(0, 3, index+1)];
	else if (listModel->isEmpty())
		emptyText = isResultBar ? "No results" : "Add a " + name;

	if (auto exp = dynamic_cast<ExpansionColumnModel*>(listModel.get()))
		emptyText = "";

	getPresetBrowserLookAndFeel().drawColumnBackground(g, listArea, emptyText);
}

void PresetBrowserColumn::resized()
{
	listArea = { 0, 0, getWidth(), getHeight() };
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
}


void PresetBrowserColumn::updateButtonVisibility()
{
	editButton->setVisible(false);

	const bool buttonsVisible = showButtonsAtBottom && !isResultBar && currentRoot.isDirectory();
	const bool fileIsSelected = listbox->getNumSelectedRows() > 0;

	addButton->setVisible(buttonsVisible);
	deleteButton->setVisible(buttonsVisible && fileIsSelected);
	renameButton->setVisible(buttonsVisible && fileIsSelected);
}


PresetBrowserChildComponentBase::PresetBrowserChildComponentBase(PresetBrowser* b):
	parent(b)
{

}

PresetBrowserLookAndFeelMethods &PresetBrowserChildComponentBase::getPresetBrowserLookAndFeel()
{
	return parent->getPresetBrowserLookAndFeel();
}

PresetBrowserColumn::ExpansionColumnModel::ExpansionColumnModel(PresetBrowser* p) :
	ColumnListModel(p, -1, p),
	ControlledObject(p->getMainController())
{
	totalRoot = p->getMainController()->getExpansionHandler().getExpansionFolder();
	root = totalRoot;
}

void PresetBrowserColumn::ExpansionColumnModel::listBoxItemClicked(int row, const MouseEvent & e)
{
	if (listener != nullptr && !e.mouseWasDraggedSinceMouseDown())
	{
		if (lastIndex == row)
		{
			lastIndex = -1;
		}
		else
			lastIndex = row;

		listener->selectionChanged(index, lastIndex, lastIndex == -1 ? File() : entries[lastIndex], false);
	}
}

void PresetBrowserColumn::ExpansionColumnModel::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (lastIndex == -1)
		rowIsSelected = false;

	auto& h = getMainController()->getExpansionHandler();

	String itemName;

	if (auto* e = h.getExpansion(rowNumber))
	{
		itemName = e->getProperty(ExpansionIds::Name);
	}

	if (rowNumber < entries.size())
	{
		auto position = Rectangle<int>(0, 1, width, height - 2);
		getPresetBrowserLookAndFeel().drawListItem(g, index, rowNumber, itemName, position, rowIsSelected, deleteOnClick);
	}
}

int PresetBrowserColumn::ExpansionColumnModel::getNumRows() 
{
	entries.clear();

	// We check for actual valid expansions to display here...
	auto& h = getMainController()->getExpansionHandler();

	for (int i = 0; i < h.getNumExpansions(); i++)
		entries.add(h.getExpansion(i)->getRootFolder());

	return entries.size();
}

}