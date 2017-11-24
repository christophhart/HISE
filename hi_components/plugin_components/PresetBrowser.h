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

		void sendRowChangeMessage(int row);

		void returnKeyPressed(int row) override;

        void setEditMode(bool on) { editMode = on; }
        
        void setTotalRoot(const File& totalRoot_) {totalRoot = totalRoot_;}
        
		

		int getIndexForFile(const File& f) const
		{
			return entries.indexOf(f);
		}

        String wildcard;
        
		Colour highlightColour;
		Font font;

	private:

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

    void setEditMode(bool on) { listModel->setEditMode(on); listbox->repaint(); };
    
	void setHighlightColourAndFont(Colour c, Font fo)
	{
		highlightColour = c;
		listModel->highlightColour = c;
		listModel->font = fo;
		blaf.highlightColour = c;
		blaf.font = fo;
		font = fo;

		addButton->setColours(c, c.withMultipliedBrightness(1.3f), c.withMultipliedBrightness(1.5f));
	}

	void buttonClicked(Button* b);

	void addEntry(const String &newName);

	void paint(Graphics& g) override;
	void resized();

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
		addButton->setVisible(!isResultBar);
		editButton->setVisible(!isResultBar);
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


	bool isResultBar = false;

	Colour highlightColour;
	Font font;
	
	int index;

	File currentRoot;

	File selectedFile;

	ButtonLookAndFeel blaf;

	ScopedPointer<TextButton> editButton;
	ScopedPointer<ShapeButton> addButton;

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
			case MultiColumnPresetBrowser::ModalWindow::Action::Replace:
				return "Are you sure?";
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
			else
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

	private:

		AlertWindowLookAndFeel alaf;

		ScopedPointer<TextButton> okButton;
		ScopedPointer<TextButton> cancelButton;

		Font f;

		

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
		showOnlyPresets = l->getText().isNotEmpty();

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

	MainController* mc;

	void setHighlightColourAndFont(Colour c, Colour bgColour, Font f);

	void setShowCloseButton(bool showButton)
	{
		if (showButton != closeButton->isVisible())
		{
			closeButton->setVisible(showButton);
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

private:

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

	ScopedPointer<ShapeButton> closeButton;
	ScopedPointer<ModalWindow> modalInputWindow;

	ScopedPointer<TextButton> saveButton;
	ScopedPointer<TextButton> showButton;

	Array<File> allPresets;
	int currentlyLoadedPreset = -1;

	bool showOnlyPresets = false;
	String currentWildcard = "*";
	
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColumnPresetBrowser);

	// ============================================================================================

};

} // namespace hise

#endif
