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

#ifndef PRESETBROWSER_INCLUDED
#define PRESETBROWSER_INCLUDED

namespace hise { using namespace juce;




/** A UI Component that handles the user preset organisation.
	@ingroup hise_ui

	This is a configurable browser for the file-based user preset system in HISE. It features:

	- automatic synchronisation with the file system.
	- a variable hierarchy based on the folder structure (up to three levels deep)
	- a favorite system that allows the user to flag certain presets that can be filtered.
	- a quasi-modal action window system for editing / saving / removing presets.
	- a tag system that can be used to further organisation of presets.

*/
class PresetBrowser :			 public Component,
								 public ControlledObject,
							     public QuasiModalComponent,
								 public Button::Listener,
                                 public ComponentWithAdditionalMouseProperties,
								 public PresetBrowserColumn::ColumnListModel::Listener,
								 public Label::Listener,
								 public MainController::UserPresetHandler::Listener,
								 public TagList::Listener,
								 public ExpansionHandler::Listener
{
public:

	struct Options
	{
		Colour highlightColour;
		Colour highlightedTextColour;
		Colour backgroundColour;
		Colour textColour;
		Font font;

		int numColumns = 3;
		Array<var> columnWidthRatios;

		bool showNotesLabel = true;
		bool showEditButtons = true;
		bool showAddButton = true;
		bool showRenameButton = true;
		bool showDeleteButton = true;
		bool showSearchBar = true;
		bool buttonsInsideBorder = false;
		int editButtonOffset = 10;
		Array<var> listAreaOffset;
		Array<var> columnRowPadding;
		Array<var> searchBarBounds;
		Array<var> favoriteButtonBounds;

		Array<var> moreButtonBounds;
		Array<var> saveButtonBounds;

		bool showSaveButtons = true;
		bool showFolderButton = true;
		bool showFavoriteIcons = true;
		bool fullPathFavorites = false;
		bool showExpansions = false;
	};

	// ============================================================================================

	class ModalWindow : public Component,
						PresetBrowserChildComponentBase,
						public ButtonListener
	{
	public:

		enum class Action
		{
			Idle = 0,
			Rename,
			Add,
			Delete,
			Replace,
			numActions
		};

		ModalWindow(PresetBrowser* p);
		~ModalWindow();

		String getCommand() const;
		String getTitleText() const;

		void update() override {};
		bool keyPressed(const KeyPress& key) override;
		void buttonClicked(Button* b) override;
		void resized() override;
		void paint(Graphics& g) override;
		void refreshModalWindow();

		void addActionToStack(Action actionToDo, const String& preEnteredText=String(), int newColumnIndex=-1, int newRowIndex=-1);
		void confirmDelete(int columnIndex, const File& fileToDelete);
		void confirmReplacement(const File& oldFile, const File& newFile);

	private:

		ScopedPointer<LookAndFeel> alaf;
		ScopedPointer<TextButton> okButton;
		ScopedPointer<TextButton> cancelButton;

		struct StackEntry
		{
			StackEntry(): currentAction(Action::Idle), columnIndex(-1), rowIndex(-1) {}

			Action currentAction;
			File newFile;
			File oldFile;
			int columnIndex = -1;
			int rowIndex = -1;
		};

		Array<StackEntry> stack;
		ScopedPointer<BetterLabel> inputLabel;
	};

	PresetBrowser(MainController* mc_, int width=810, int height=500);
	~PresetBrowser();

	

	bool isReadOnly(const File& f);

	void presetChanged(const File& newPreset) override;
	void presetListUpdated() override;

	void rebuildAllPresets();
	String getCurrentlyLoadedPresetName();

	void buttonClicked(Button* b) override;
	void selectionChanged(int columnIndex, int rowIndex, const File& clickedFile, bool doubleClick);
	void renameEntry(int columnIndex, int rowIndex, const String& newName);
	void deleteEntry(int columnIndex, const File& f);
	void addEntry(int columnIndex, const String& name);

	void loadPreset(const File& f);

	void paint(Graphics& g);
	void resized();

    void attachAdditionalMouseProperties(const MouseEvent& e, var& obj) override;
    
	void tagCacheNeedsRebuilding() override {};
	void tagSelectionChanged(const StringArray& newSelection) override;
	void labelTextChanged(Label* l) override;
	void updateFavoriteButton();
	bool shouldShowFavoritesButton() { return showFavoritesButton; }
	bool shouldShowFullPathFavorites() { return fullPathFavorites; }

	void lookAndFeelChanged() override;

	void loadPresetDatabase(const File& rootDirectory);
	void savePresetDatabase(const File& rootDirectory);
	var getDataBase() { return presetDatabase; }
	const var getDataBase() const { return presetDatabase; }

	void incPreset(bool next, bool stayInSameDirectory=false);
	void setCurrentPreset(const File& f, NotificationType /*sendNotification*/);

	void openModalAction(ModalWindow::Action action, const String& preEnteredText, const File& fileToChange, int columnIndex, int rowIndex);
	void confirmReplace(const File& oldFile, const File &newFile);

	void showLoadedPreset();

	struct DataBaseHelpers
	{
		static void setFavorite(const var& database, const File& presetFile, bool isFavorite);
		static void cleanFileList(MainController* mc, Array<File>& filesToClean);
		static void writeTagsInXml(const File& currentPreset, const StringArray& tags);
		static bool matchesTags(const StringArray& currentlyActiveTags, const File& presetToTest);
		static void writeNoteInXml(const File& currentPreset, const String& newNote);
		static StringArray getTagsFromXml(const File& currentPreset);
		static String getNoteFromXml(const File& currentPreset);
		static bool matchesAvailableExpansions(MainController* mc, const File& currentPreset);
		static bool isFavorite(const var& database, const File& presetFile);
		static Identifier getIdForFile(const File& presetFile);
	};

	void setOptions(const Options& newOptions);

	ExpansionHandler& expHandler;

	void expansionPackLoaded(Expansion* currentExpansion);

	void expansionPackCreated(Expansion* newExpansion) override;

	PresetBrowserLookAndFeelMethods& getPresetBrowserLookAndFeel();

	void hideManageButton()
	{
		manageButton->setVisible(false);
	}

	Point<int> getMouseHoverInformation() const;

	Component* getColumn(int columnIndex)
	{
		switch(columnIndex)
		{
		case -1: return expansionColumn->getListbox();
		case 0: return bankColumn->getListbox();
		case 1: return categoryColumn->getListbox();
		case 2: return presetColumn->getListbox();
		}

		jassertfalse;
		return nullptr;
	}

private:

	DefaultPresetBrowserLookAndFeel laf;

	void setShowFavorites(bool shouldShowFavorites);
	void setShowFullPathFavorites(bool shouldShowFullPathFavorites);
	void setHighlightColourAndFont(Colour c, Colour bgColour, Font f);
	void setNumColumns(int numColumns);

	/** SaveButton = 1, ShowFolderButton = 0 */
	void setShowButton(int buttonId, bool newValue);
	void setShowNotesLabel(bool shouldBeShown);
	void setShowEditButtons(int buttonId, bool showEditButtons);
	void setShowSearchBar(bool shouldBeShown);
	void setButtonsInsideBorder(bool inside);
	void setEditButtonOffset(int offset);
	void setListAreaOffset(Array<var> offset);
	void setColumnRowPadding(Array<var> offset);
	void setShowCloseButton(bool shouldShowButton);
	Array<var> getListAreaOffset();

	// ============================================================================================

	int numColumns = 3;
	Array<var> columnWidthRatios;
	Array<var> searchBarBounds;
	Array<var> favoriteButtonBounds;
	Array<var> saveButtonBounds;
	Array<var> moreButtonBounds;
	
	File defaultRoot;
	File rootFile;
	File currentBankFile;
	File currentCategoryFile;

	bool refreshColumnUpdatesAfterExpansionSwitch = false;

	ScopedPointer<PresetBrowserSearchBar> searchBar;
	ScopedPointer<PresetBrowserColumn> expansionColumn;
	ScopedPointer<PresetBrowserColumn> bankColumn;
	ScopedPointer<PresetBrowserColumn> categoryColumn;
	ScopedPointer<PresetBrowserColumn> presetColumn;

	ScopedPointer<BetterLabel> noteLabel;
	ScopedPointer<TagList> tagList;
	ScopedPointer<ShapeButton> closeButton;
	ScopedPointer<ShapeButton> favoriteButton;
	ScopedPointer<ModalWindow> modalInputWindow;

	ScopedPointer<TextButton> saveButton;
	ScopedPointer<TextButton> manageButton;

	Array<File> allPresets;
	int currentlyLoadedPreset = -1;

	bool showFavoritesButton = true;
	bool fullPathFavorites = false;
	bool showOnlyPresets = false;
	String currentWildcard = "*";
	StringArray currentTagSelection;

	WeakReference<Expansion> currentlySelectedExpansion;

	var presetDatabase;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser);

	// ============================================================================================

};

} // namespace hise

#endif
