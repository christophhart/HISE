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

class MultiColumnPresetBrowser;


#define OLD_PRESET_BROWSER 0

class PresetBrowserLookAndFeel : public LookAndFeel_V3
{
public:

	enum ColourIds
	{
		highlightColourId = 0xf312,
		backgroundColourId,
		tableBackgroundColourId
	};

	Font getFont(bool fontForTitle)
	{
		return fontForTitle ? GLOBAL_BOLD_FONT().withHeight(18.0f) : GLOBAL_BOLD_FONT();
	}
};

class BetterLabel: public Label
{
public:

    virtual TextEditor* createEditorComponent ()
    {
        TextEditor* t = Label::createEditorComponent();
        t->setIndents(4, 7);
        return t;
    }  
    
	void textEditorReturnKeyPressed(TextEditor& t) override
	{
		if (getText() == t.getText())
		{
			hideEditor(true);
			textWasEdited();
			callChangeListeners();
		}
		else
		{
			Label::textEditorReturnKeyPressed(t);
		}
	}

    void textWasEdited() override
    {
        if(refreshWithEachKey)
			textWasChanged();
    }

	bool refreshWithEachKey = true;
	
};

class PresetBrowserSearchBar: public Component
{
public:

	enum ColourIds
	{
		highlightColourId = 0xf312,
		backgroundColourId,
		tableBackgroundColourId
	};

    PresetBrowserSearchBar();
    
	void setHighlightColourAndFont(Colour c, Font f);

    void paint(Graphics &g) override;
    
    void resized() override
    {
        inputLabel->setBounds(24, 0, getWidth() - 24, getHeight());
    }
    
	Colour highlightColour;

	
    ScopedPointer<BetterLabel> inputLabel;
    
};

class PresetBrowserColumn : public Component,
	                       public ButtonListener,
	                       public Label::Listener,
	                       public Timer
{
public:

	// ============================================================================================

	class ButtonLookAndFeel : public LookAndFeel_V3
	{
	public:

		ButtonLookAndFeel() :
			highlightColour(Colour(0xffffa8a8))
		{};

		void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
			bool isMouseOverButton, bool isButtonDown);

		void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/);

		Colour highlightColour;
		Font font;
	};

	// ============================================================================================

	class ColumnListModel : public ListBoxModel
	{
	public:

		class Listener
		{
		public:
            
            virtual ~Listener() {};
            
			virtual void selectionChanged(int columnIndex, int rowIndex, const File& file, bool doubleClick) = 0;
			virtual void deleteEntry(int columnIndex, const File& f) = 0;
			virtual void renameEntry(int columnIndex, int rowIndex, const String& newName) = 0;
		};

		ColumnListModel(int index_, Listener* listener_);

		void setRootDirectory(const File& newRootDirectory) { root = newRootDirectory; }
		void toggleEditMode() { editMode = !editMode; }
		void setDisplayDirectories(bool shouldDisplayDirectories) { displayDirectories = shouldDisplayDirectories; }

		int getNumRows() override;
		void listBoxItemClicked(int row, const MouseEvent &) override;
		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;

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

		Component* refreshComponentForRow(int rowNumber, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
		{
#if OLD_PRESET_BROWSER
			return nullptr;
#else
			if (existingComponentToUpdate != nullptr)
				delete existingComponentToUpdate;

			if (index == 2)
			{
				return new FavoriteOverlay(*this, rowNumber);
			}
			else
				return nullptr;
#endif
		}

		void sendRowChangeMessage(int row);

		void returnKeyPressed(int row) override;

        void setEditMode(bool on) { editMode = on; }
        
        void setTotalRoot(const File& totalRoot_) {totalRoot = totalRoot_;}
        
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
        
		Colour highlightColour;
		Font font;

		var database;


	private:

		bool showFavoritesOnly = false;

		Image deleteIcon;

		Listener* listener;
		bool editMode = false;
		bool displayDirectories = true;
		Array<File> entries;
		File root;
		const int index;
		
		File totalRoot;
		
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColumnListModel)
	};

	// ============================================================================================

	PresetBrowserColumn(MainController* mc_, int index_, File& rootDirectory, ColumnListModel::Listener* listener);

	static File getChildDirectory(File& root, int level, int index);
	void setNewRootDirectory(const File& newRootDirectory);

	void setShowFavoritesOnly(bool shouldShow)
	{
		listModel->setShowFavoritesOnly(shouldShow);
		listbox->updateContent();
	}

    void setEditMode(bool on) { listModel->setEditMode(on); listbox->repaint(); };
    
	void setHighlightColourAndFont(Colour c, Font fo)
	{
		highlightColour = c;
		listModel->highlightColour = c;
		listModel->font = fo;
		blaf.highlightColour = c;
		blaf.font = fo;
		font = fo;

#if OLD_PRESET_BROWSER
		addButton->setColours(c, c.withMultipliedBrightness(1.3f), c.withMultipliedBrightness(1.5f));
#else
		if (auto v = listbox->getViewport())
		{
			v->setColour(ScrollBar::ColourIds::backgroundColourId, Colours::transparentBlack);
			v->setColour(ScrollBar::ColourIds::thumbColourId, highlightColour.withAlpha(0.1f));
		}
#endif


		
	}

	void buttonClicked(Button* b);

	void addEntry(const String &newName);

	void paint(Graphics& g) override;
	void resized();

	void updateButtonVisibility();

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
		updateButtonVisibility();
	}

	void timerCallback() override
    {
        if(!isVisible()) return;
        
	    listbox->updateContent();
	    listbox->repaint();
    }
	
	void setSelectedFile(const File& file, NotificationType notifyListeners=dontSendNotification)
	{
		const int rowIndex = listModel->getIndexForFile(file);

		if (rowIndex >= 0)
		{
			selectedFile = file;

			SparseSet<int> s;
			s.addRange(Range<int>(rowIndex, rowIndex + 1));
			listbox->setSelectedRows(s, dontSendNotification);
		}

		if (notifyListeners == sendNotification)
		{
			listModel->sendRowChangeMessage(rowIndex);
		}
	}

private:

	

	// ============================================================================================

	

	Rectangle<int> listArea;

	bool isResultBar = false;

	Colour highlightColour;
	Font font;
	
	int index;

	File currentRoot;

	File selectedFile;

	ButtonLookAndFeel blaf;

	ScopedPointer<TextButton> editButton;

#if OLD_PRESET_BROWSER
	ScopedPointer<ShapeButton> addButton;
#else
	ScopedPointer<TextButton> addButton;
	ScopedPointer<TextButton> renameButton;
	ScopedPointer<TextButton> deleteButton;
#endif

	ScopedPointer<ColumnListModel> listModel;
	ScopedPointer<ListBox> listbox;

    int numFiles;
	
    MultiColumnPresetBrowser* browser;
    
	MainController* mc;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserColumn);

	// ============================================================================================
};


class MultiColumnPresetBrowser : public Component,
							     public QuasiModalComponent,
								 public Button::Listener,
								 public PresetBrowserColumn::ColumnListModel::Listener,
								 public Label::Listener,
								 public MainController::UserPresetHandler::Listener
{
public:

	// ============================================================================================

	class ModalWindow : public Component,
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

		ModalWindow()
		{
            f = GLOBAL_BOLD_FONT();
            
			addAndMakeVisible(inputLabel = new BetterLabel());
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

            inputLabel->setFont(f);
            
			okButton->addListener(this);
			cancelButton->addListener(this);

			okButton->setLookAndFeel(&alaf);
			cancelButton->setLookAndFeel(&alaf);

			inputLabel->refreshWithEachKey = false;

			setWantsKeyboardFocus(true);
		}

		~ModalWindow()
		{
			inputLabel = nullptr;
			okButton = nullptr;
			cancelButton = nullptr;
		}

        void paint(Graphics& g) override;
		

		String getCommand() const
		{
			auto le = stack.getLast();

			switch (le.currentAction)
			{
			case MultiColumnPresetBrowser::ModalWindow::Action::Idle:
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::Rename:
			case MultiColumnPresetBrowser::ModalWindow::Action::Add:
				return "Enter the name";
			case MultiColumnPresetBrowser::ModalWindow::Action::Delete:
				return "Are you sure you want to delete the file " + le.newFile.getFileNameWithoutExtension() + "?";
			case MultiColumnPresetBrowser::ModalWindow::Action::Replace:
				return "Are you sure you want to replace the file " + le.newFile.getFileNameWithoutExtension() + "?";
			case MultiColumnPresetBrowser::ModalWindow::Action::numActions:
				break;
			default:
				break;
			}

			return "";
		}

		String getTitleText() const
		{
			String s;

			StackEntry le = stack.getLast();

			

			switch (le.currentAction)
			{
			case MultiColumnPresetBrowser::ModalWindow::Action::Idle:
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::Rename:
				s << "Rename ";
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::Add:
				s << "Add new ";
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::Replace:
				s << "Replace ";
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::Delete:
				s << "Delete ";
				break;
			case MultiColumnPresetBrowser::ModalWindow::Action::numActions:
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

		bool keyPressed(const KeyPress& key) override
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

		void buttonClicked(Button* b) override
		{
			auto le = stack.getLast();

			stack.removeLast();

			auto p = findParentComponentOfClass<MultiColumnPresetBrowser>();

			if (b == okButton)
			{
				auto text = inputLabel->getText();

				

				switch (le.currentAction)
				{
				case MultiColumnPresetBrowser::ModalWindow::Action::Idle:
					jassertfalse;
					break;
				case MultiColumnPresetBrowser::ModalWindow::Action::Rename:
					p->renameEntry(le.columnIndex, le.rowIndex, inputLabel->getText());
					break;
				case MultiColumnPresetBrowser::ModalWindow::Action::Add:
					p->addEntry(le.columnIndex, inputLabel->getText());
					break;
				case MultiColumnPresetBrowser::ModalWindow::Action::Delete:
					p->deleteEntry(le.columnIndex, le.oldFile);
					break;
				case MultiColumnPresetBrowser::ModalWindow::Action::Replace:
					le.oldFile.moveFileTo(le.newFile);
					if (le.oldFile.getFileName() == "tempFileBeforeMove.preset")
						le.oldFile.deleteFile();

					p->rebuildAllPresets();
					break;
				case MultiColumnPresetBrowser::ModalWindow::Action::numActions:
					break;
				default:
					break;
				}
			}
			
			if (le.currentAction == Action::Replace && le.oldFile.getFileName() == "tempFileBeforeMove.preset")
				le.oldFile.deleteFile();
			
			refreshModalWindow();
			
		}

		void resized() override
		{
			inputLabel->centreWithSize(300, 30);

			okButton->setBounds(inputLabel->getX(), inputLabel->getBottom() + 20, 100, 30);
			cancelButton->setBounds(inputLabel->getRight() - 100, inputLabel->getBottom() + 20, 100, 30);
		}

		void setHighlightColourAndFont(Colour c, Colour bgColour, Font font)
		{
			f = font;

			inputLabel->setFont(f);
			
		}

		void refreshModalWindow()
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

		void addActionToStack(Action actionToDo, const String& preEnteredText=String(), int newColumnIndex=-1, int newRowIndex=-1)
		{
			inputLabel->setText(preEnteredText, dontSendNotification);

			StackEntry ne;

			ne.currentAction = actionToDo;
			ne.columnIndex = newColumnIndex;
			ne.rowIndex = newRowIndex;

			stack.add(ne);

			

			refreshModalWindow();


		}

		void confirmDelete(int columnIndex, const File& fileToDelete)
		{
			StackEntry ne;

			ne.currentAction = Action::Delete;
			ne.oldFile = fileToDelete;
			ne.columnIndex = columnIndex;

			stack.add(ne);
			refreshModalWindow();
		}

		void confirmReplacement(const File& oldFile, const File& newFile)
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


		Font f;

		Colour highlightColour;

	private:

		AlertWindowLookAndFeel alaf;

		ScopedPointer<TextButton> okButton;
		ScopedPointer<TextButton> cancelButton;


		

		struct StackEntry
		{
			StackEntry():
				currentAction(Action::Idle),
				columnIndex(-1),
				rowIndex(-1)
			{

			}

			Action currentAction;

			File newFile;
			File oldFile;

			int columnIndex = -1;
			int rowIndex = -1;
		};

		Array<StackEntry> stack;
		
		ScopedPointer<BetterLabel> inputLabel;
	};

	void colourChanged() override
	{
		searchBar->setColour(PresetBrowserSearchBar::ColourIds::highlightColourId, findColour(PresetBrowserSearchBar::ColourIds::highlightColourId));
	}

	void buttonClicked(Button* b) override;

	MultiColumnPresetBrowser(MainController* mc_, int width=810, int height=500);

	~MultiColumnPresetBrowser();

	void presetChanged(const File& newPreset) override
	{
		if (allPresets[currentlyLoadedPreset] == newPreset)
			return;

		File pFile = newPreset;
		File cFile = pFile.getParentDirectory();
		File bFile = cFile.getParentDirectory();

		bankColumn->setSelectedFile(bFile, sendNotification);
		categoryColumn->setSelectedFile(cFile, sendNotification);
		presetColumn->setSelectedFile(newPreset, dontSendNotification);

		saveButton->setEnabled(true);

		noteLabel->setText(DataBaseHelpers::getNoteFromXml(newPreset), dontSendNotification);

	}

	void presetListUpdated() override
	{
		rebuildAllPresets();
	}

	void rebuildAllPresets();
	String getCurrentlyLoadedPresetName();

	void selectionChanged(int columnIndex, int rowIndex, const File& clickedFile, bool doubleClick);
	void renameEntry(int columnIndex, int rowIndex, const String& newName);
	void deleteEntry(int columnIndex, const File& f);
	void addEntry(int columnIndex, const String& name);

	void loadPreset(const File& f);

	void paint(Graphics& g);
	void resized();

	void labelTextChanged(Label* l) override
	{
		if (l == noteLabel)
		{
			auto currentPreset = allPresets[currentlyLoadedPreset];
			auto newNote = noteLabel->getText();

			DataBaseHelpers::writeNoteInXml(currentPreset, newNote);
		}
		else
		{
			showOnlyPresets = l->getText().isNotEmpty() || favoriteButton->getToggleState();

			if (showOnlyPresets)
			{
				currentWildcard = "*" + l->getText() + "*";
			}
			else
			{
				currentWildcard = "*";
			}

			resized();
		}
	}

	void updateFavoriteButton()
	{
		const bool on = favoriteButton->getToggleState();

		showOnlyPresets = currentWildcard != "*" || on;

		static const unsigned char onShape[] = "nm\xac&=Ca\xee<Cl\x12\x96?C%\xaf""CCl\xde\xc2""FC\xd0\xe9""CClZ\x17""AC\xebPHCl(\x17""CC\xf1""5OCl\xad&=C\xc4-KCl267C\xf1""5OCl\0""69C\xebPHCl}\x8a""3C\xd0\xe9""CClH\xb7:C%\xaf""CCce";

		static const unsigned char offShape[] = { 110,109,0,144,89,67,0,103,65,67,108,0,159,88,67,0,3,68,67,108,129,106,86,67,0,32,74,67,108,1,38,77,67,0,108,74,67,108,1,121,84,67,0,28,80,67,108,129,227,81,67,255,3,89,67,108,1,144,89,67,127,206,83,67,108,1,60,97,67,255,3,89,67,108,129,166,94,67,0,28,
			80,67,108,129,249,101,67,0,108,74,67,108,1,181,92,67,0,32,74,67,108,1,144,89,67,0,103,65,67,99,109,0,144,89,67,1,76,71,67,108,128,73,91,67,1,21,76,67,108,0,94,96,67,129,62,76,67,108,0,90,92,67,129,92,79,67,108,128,196,93,67,129,62,84,67,108,0,144,89,
			67,129,99,81,67,108,0,91,85,67,1,63,84,67,108,128,197,86,67,129,92,79,67,108,128,193,82,67,129,62,76,67,108,0,214,87,67,1,21,76,67,108,0,144,89,67,1,76,71,67,99,101,0,0 };

		Path path;

		if (on)
		{
			path.loadPathFromData(onShape, sizeof(onShape));
		}

		else
		{
			path.loadPathFromData(offShape, sizeof(offShape));
		}

		favoriteButton->setShape(path, false, true, true);

		

		presetColumn->setShowFavoritesOnly(on);

		resized();


	}

	void loadPresetDatabase(const File& rootDirectory);

	void savePresetDatabase(const File& rootDirectory);

	var getDataBase() { return presetDatabase; }

	const var getDataBase() const { return presetDatabase; }

	MainController* mc;

	void setHighlightColourAndFont(Colour c, Colour bgColour, Font f);

	/** SaveButton = 1, ShowFolderButton = 0 */
	void setShowButton(int buttonId, bool newValue)
	{
		enum ButtonIndexes
		{
			ShowFolderButton = 0,
			SaveButton,
			numButtonsToShow
		};

#if !OLD_PRESET_BROWSER
		if (buttonId == SaveButton)
		{
			saveButton->setVisible(newValue);
			
		}
		else if (buttonId == ShowFolderButton)
		{
			showButton->setVisible(newValue);
		}

		resized();
#endif
	}

	void setShowCloseButton(bool shouldShowButton)
	{
		if (shouldShowButton != closeButton->isVisible())
		{
			closeButton->setVisible(shouldShowButton);
			resized();
		}
	}

	void incPreset(bool next, bool stayInSameDirectory=false)
	{
		mc->getUserPresetHandler().incPreset(next, stayInSameDirectory);

	}

	void setCurrentPreset(const File& f, NotificationType /*sendNotification*/)
	{
		int newIndex = allPresets.indexOf(f);

		if (newIndex != -1)
		{
			currentlyLoadedPreset = newIndex;
			
			presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

#if NEW_USER_PRESET
			if (listener != nullptr && sendNotification)
			{
				listener->presetChanged(f.getFileNameWithoutExtension());
			}
#endif
		}
	}

	void openModalAction(ModalWindow::Action action, const String& preEnteredText, const File& fileToChange, int columnIndex, int rowIndex);

	void confirmReplace(const File& oldFile, const File &newFile)
	{
		modalInputWindow->confirmReplacement(oldFile, newFile);
	}


	void showLoadedPreset();

	struct DataBaseHelpers
	{
		static void setFavorite(const var& database, const File& presetFile, bool isFavorite)
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

		static void writeNoteInXml(const File& currentPreset, const String& newNote)
		{
			if (currentPreset.existsAsFile())
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(currentPreset);

				if (xml != nullptr)
				{
					xml->setAttribute("Notes", newNote);

					auto newPresetContent = xml->createDocument("");

					currentPreset.replaceWithText(newPresetContent);
				}
			}
		}

		static String getNoteFromXml(const File& currentPreset)
		{
			if (currentPreset.existsAsFile())
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(currentPreset);

				if (xml != nullptr)
				{
					return xml->getStringAttribute("Notes", "");
				}
			}

			return String();
		}

		static bool isFavorite(const var& database, const File& presetFile)
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

		static Identifier getIdForFile(const File& presetFile)
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

				//s = s.replaceCharacters("öäüèêé", "oaueee");


				s = s.removeCharacters(" \t!+");

				if (Identifier::isValidIdentifier(s))
				{
					return Identifier(s);
				}

				jassertfalse;
				return Identifier();
			}

			return Identifier();
		}
	};

private:

	class TagArea : public Component,
		public ButtonListener
	{
	public:

		TagArea()
		{
			Array<var> t;
			t.add("Favorites");
			t.add("Bass");
			t.add("Soundscapes");
			t.add("Heavy");
			t.add("Dark");
			t.add("Arpeggiated");
			t.add("Drums");
			t.add("Sequenced");
			t.add("Tuned");
			t.add("Plucked");

			tags = var(t);

			rebuildTags();
		}

		void resized() override
		{
			auto area = getLocalBounds();

			const int numColumns = 6;

			const int numRows = jmax<int>(1, tagButtons.size() / numColumns + 1);
			const int heightPerRow = getHeight() / numRows;
			const int widthPerButton = getWidth() / numColumns;

			for (int i = 0; i < tagButtons.size(); i++)
			{
				const int row = i / numColumns;
				const int column = i % numColumns;

				auto rowArea = Rectangle<int>(column * widthPerButton, row * heightPerRow, widthPerButton, heightPerRow);
				tagButtons[i]->setBounds(rowArea.reduced(2));
			}
		};

		void buttonClicked(Button* b) override
		{
			b->setToggleState(!b->getToggleState(), dontSendNotification);
		}

		void rebuildTags()
		{
			tagButtons.clear();

			if (auto vArray = tags.getArray())
			{
				for (const auto& v : *vArray)
				{
					TextButton* t = new TextButton(v.toString());
					t->setLookAndFeel(&blaf);
					t->addListener(this);
					addAndMakeVisible(t);
					tagButtons.add(t);
				}
			}

			resized();
		}

		var tags;

		PresetBrowserColumn::ButtonLookAndFeel blaf;

		OwnedArray<TextButton> tagButtons;
	};

	// ============================================================================================

	Colour backgroundColour;
	Colour outlineColour;

	PresetBrowserLookAndFeel pblaf;

	PresetBrowserColumn::ButtonLookAndFeel blaf;

	File rootFile;
	File currentBankFile;
	File currentCategoryFile;

	ScopedPointer<PresetBrowserSearchBar> searchBar;
	ScopedPointer<PresetBrowserColumn> bankColumn;
	ScopedPointer<PresetBrowserColumn> categoryColumn;
	ScopedPointer<PresetBrowserColumn> presetColumn;

	ScopedPointer<BetterLabel> noteLabel;

	ScopedPointer<TagArea> tagArea;

	ScopedPointer<ShapeButton> closeButton;

	ScopedPointer<ShapeButton> favoriteButton;

	ScopedPointer<ModalWindow> modalInputWindow;

	ScopedPointer<TextButton> saveButton;
	ScopedPointer<TextButton> showButton;

	Array<File> allPresets;
	int currentlyLoadedPreset = -1;

	bool showOnlyPresets = false;
	String currentWildcard = "*";
	
	var presetDatabase;
	
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColumnPresetBrowser);

	// ============================================================================================

};

} // namespace hise

#endif
