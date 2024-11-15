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

class PresetBrowser;



class PresetBrowserChildComponentBase
{
public:

	PresetBrowserChildComponentBase(PresetBrowser* b);
	
	virtual ~PresetBrowserChildComponentBase() {};

	virtual void update() = 0;

	PresetBrowserLookAndFeelMethods& getPresetBrowserLookAndFeel();

protected:
	
	Component::SafePointer<PresetBrowser> parent;
};

class NiceLabel : public Label
{
public:
	TextEditor* createEditorComponent() override;

	void textEditorReturnKeyPressed(TextEditor& t) override;

	void textWasEdited() override;

	bool refreshWithEachKey = true;
};

class BetterLabel : public NiceLabel,
					public PresetBrowserChildComponentBase
{
public:

	BetterLabel(PresetBrowser* p) :
		PresetBrowserChildComponentBase(p)
	{}

	void update() override;
};

class PresetBrowserSearchBar : public Component,
							   public PresetBrowserChildComponentBase
{
public:

	PresetBrowserSearchBar(PresetBrowser* b);

	void paint(Graphics &g) override;

	void update() override {};

	void resized() override
	{
		inputLabel->setBounds(24, 0, getWidth() - 24, getHeight());
	}

	ScopedPointer<BetterLabel> inputLabel;
};


class TagList : public Component,
				public PresetBrowserChildComponentBase,
				public ControlledObject,
				public MainController::UserPresetHandler::Listener,
				public ButtonListener,
				public Timer
{
public:

	struct Listener
	{
		virtual ~Listener() {};

		virtual void tagSelectionChanged(const StringArray& newSelection) = 0;
		virtual void tagCacheNeedsRebuilding() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	TagList(MainController* mc_, PresetBrowser* p);

	~TagList()
	{
		getMainController()->getUserPresetHandler().removeListener(this);
		editButton.removeListener(this);
	}

	void buttonClicked(Button*) override;

	virtual void presetChanged(const File& newPreset);

	virtual void presetListUpdated()
	{
		rebuildTags();
	}

	void setShowEditButton(bool shouldBeVisible)
	{
		editButton.setVisible(shouldBeVisible);
		resized();
	}

	void rebuildTags();

	bool isActive() const
	{
		return !tags.isEmpty();
	}

	void update() override {};

	void addTagListener(Listener* l) {
		listeners.addIfNotAlreadyThere(l);
	}

	void removeTagListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void resized() override;

	class Tag : public Component
	{
	public:

		Tag(TagList& parent_, const String& name_) :
			parent(parent_),
			name(name_)
		{
			simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".tag-button" });
		}

		int getTagWidth() const;

		void mouseDown(const MouseEvent&) override;

		void setActive(bool shouldBeActive)
		{
			active = shouldBeActive;
			repaint();
		}

		void setSelected(bool shouldBeSelected)
		{
			selected = shouldBeSelected;
			repaint();
		}

		bool isActive() const { return active; };

		void paint(Graphics& g) override;

		TagList& parent;
		String name;
		bool active = false;
		bool selected = false;
	};

	void timerCallback() override;

	void toggleTag(Tag* n);

	File currentFile;
	StringArray currentlyActiveTags;
	StringArray currentlySelectedTags;

	bool on = false;
	bool editMode = false;
	TextButton editButton;

	OwnedArray<Tag> tags;

	Array<WeakReference<Listener>> listeners;
};

class PresetBrowser;

class PresetBrowserColumn : public Component,
	public PresetBrowserChildComponentBase,
	public TouchAndHoldComponent,
	public ButtonListener,
	public Label::Listener,
	public TagList::Listener,
	public Timer
{
public:
	// ============================================================================================

	struct HoverInformation
	{
		int columnIndex;
		int rowIndex;
	};

	class ColumnListModel : public ListBoxModel,
							public PresetBrowserChildComponentBase
	{
	public:

		using CachedTag = MainController::UserPresetHandler::TagDataBase::CachedTag;

		class Listener
		{
		public:

			virtual ~Listener() {};

			virtual void selectionChanged(int columnIndex, int rowIndex, const File& file, bool doubleClick) = 0;
			virtual void deleteEntry(int columnIndex, const File& f) = 0;
			virtual void renameEntry(int columnIndex, int rowIndex, const String& newName) = 0;
		};

		ColumnListModel(PresetBrowser* p, int index_, Listener* listener_);

		void setRootDirectory(const File& newRootDirectory) { root = newRootDirectory; }
		void toggleEditMode() { editMode = !editMode; }
		void setDisplayDirectories(bool shouldDisplayDirectories) { displayDirectories = shouldDisplayDirectories; }

		int getNumRows() override;
		void listBoxItemClicked(int row, const MouseEvent &) override;
		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;

		void update() override {};

		bool isMouseHover(int rowNumber) const;

		const Array<CachedTag>& getCachedTags() const;

		bool isEmpty() const
		{
			return empty;
		}

		class FavoriteOverlay : public Component,
			public ButtonListener
		{
		public:

			FavoriteOverlay(ColumnListModel& parent_, int index_);
			~FavoriteOverlay();

			void refreshShape();

			void buttonClicked(Button*) override;
			void resized() override;
			void refreshIndex(int newIndex)
			{
				index = newIndex;
			}

			ScopedPointer<ShapeButton> b;

			ColumnListModel& parent;
			int index;
		};

		Component* refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, Component* existingComponentToUpdate) override;

		void sendRowChangeMessage(int row);

		void returnKeyPressed(int row) override;

		void setEditMode(bool on) { editMode = on; }

		void setTotalRoot(const File& totalRoot_) { totalRoot = totalRoot_; }

		void setShowFavoritesOnly(bool shouldShowFavoritesOnly)
		{
			showFavoritesOnly = shouldShowFavoritesOnly;
		}

		File getFileForIndex(int fileIndex) const
		{
			return entries[fileIndex];
		};

		int getIndexForFile(const File& f) const
		{
			return entries.indexOf(f);
		}
		
		String wildcard;

		var database;

		Array<Identifier> currentlyActiveTags;

		void rebuildCachedTagList();

		void updateTags(const StringArray& newSelection);

		bool allowRecursiveSearch = false;
		bool deleteOnClick = false;

		int getColumnIndex() const { return index; }

	protected:

		bool empty = false;
		bool showFavoritesOnly = false;

		Listener* listener;
		bool editMode = false;
		bool displayDirectories = true;
		Array<File> entries;
		File root;
		const int index;

		File totalRoot;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColumnListModel)
	};

	struct ExpansionColumnModel : public ColumnListModel,
								  public ControlledObject
	{
		ExpansionColumnModel(PresetBrowser* p);;

		void listBoxItemClicked(int row, const MouseEvent &) override;


		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;

		int getNumRows() override;

		void setLastIndex(int newIndex)
		{
			lastIndex = newIndex;
		}

		int lastIndex = -1;
	};

	

	// ============================================================================================

	PresetBrowserColumn(MainController* mc_, PresetBrowser* p, int index_, File& rootDirectory, ColumnListModel::Listener* listener);

	static File getChildDirectory(File& root, int level, int index);
	void setNewRootDirectory(const File& newRootDirectory);

	void setShowFavoritesOnly(bool shouldShow)
	{
		listModel->setShowFavoritesOnly(shouldShow);
		listbox->updateContent();
	}

	int getColumnIndex() const { return listModel->getColumnIndex(); }
	
	File getFileForIndex(int fileIndex) const
	{
		return listModel->getFileForIndex(fileIndex);
	};

	int getIndexForPosition(Point<int> pos)
	{
		return listbox->getRowContainingPosition(pos.getX(), pos.getY());
	}

	void setEditMode(bool on) { listModel->setEditMode(on); listbox->repaint(); };

	void setAllowRecursiveFileSearch(bool shouldAllow)
	{
		listModel->allowRecursiveSearch = shouldAllow;
		listbox->updateContent();
	}

	void setShowButtons(int buttonId, bool shouldBeShown)
	{
		enum ButtonIndexes
		{
			All = 0,
			AddButton,
			RenameButton,
			DeleteButton
		};
		
		switch (buttonId)
		{
			case All: showButtonsAtBottom = shouldBeShown; break;
			case AddButton: shouldShowAddButton = shouldBeShown; break;
			case RenameButton: shouldShowRenameButton = shouldBeShown; break;
			case DeleteButton: shouldShowDeleteButton = shouldBeShown; break;
		}
		
		
		resized();
	}
	void setEditButtonOffset(int offset)
	{
		editButtonOffset = offset;
		resized();
	}

	void setButtonsInsideBorder(bool inside)
	{
		buttonsInsideBorder = inside;
		resized();
	}

	void setListAreaOffset(Array<var> offset)
	{
		listAreaOffset = offset;
		resized();
	}
	
	Array<var> getListAreaOffset();

	void setRowPadding(double padding);

	void update() override;

	void touchAndHold(Point<int> /*downPosition*/) override;

	void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& /*e*/) override;
	void mouseMove(const MouseEvent& e) override;

	void buttonClicked(Button* b);

	void addEntry(const String &newName);

	void paint(Graphics& g) override;
	void resized();

	void updateButtonVisibility(bool isReadOnly);

	void tagSelectionChanged(const StringArray& newSelection) override
	{
		listModel->updateTags(newSelection);
		listbox->deselectAllRows();
		listbox->updateContent();
		listbox->repaint();
	}

	void tagCacheNeedsRebuilding() override
	{
		listModel->rebuildCachedTagList();
	}

	void labelTextChanged(Label* l) override
	{
		listModel->wildcard = l->getText();

		listbox->deselectAllRows();
		listbox->updateContent();
		listbox->repaint();
	}

	void setIsResultBar(bool shouldBeResultBar)
	{
		isResultBar = shouldBeResultBar;
		updateButtonVisibility(false);
	}

	void timerCallback() override;

	void setSelectedFile(const File& file, NotificationType notifyListeners = dontSendNotification);

	void setModel(ColumnListModel* newModel, const File& totalRoot)
	{
		listbox->setModel(newModel);
		newModel->setTotalRoot(totalRoot);
		listModel = newModel;
	}

	void setDatabase(var db)
	{
		listModel->database = db;
	}

	void showAddButton()
	{
		addButton->setVisible(true && shouldShowAddButton);
	}

	Component* getListbox() { return listbox.get(); }

private:

	bool deleteByTouch = false;

	// ============================================================================================

	bool showButtonsAtBottom = true;
	bool shouldShowAddButton = true;
	bool shouldShowRenameButton = true;
	bool shouldShowDeleteButton = true;
	bool buttonsInsideBorder = false;
	int editButtonOffset = 10;
	double rowPadding = 0;
	Rectangle<int> listArea;
	Array<var> listAreaOffset;
	bool isResultBar = false;
	int index;
	File currentRoot;
	File selectedFile;
	
	ScopedPointer<TextButton> editButton;

	ScopedPointer<TextButton> addButton;
	ScopedPointer<TextButton> renameButton;
	ScopedPointer<TextButton> deleteButton;
	ScopedPointer<ColumnListModel> listModel;
	ScopedPointer<ListBox> listbox;

	int numFiles;

	MainController* mc;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserColumn);

	// ============================================================================================
};


}