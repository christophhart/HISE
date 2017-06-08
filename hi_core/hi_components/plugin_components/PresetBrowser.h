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

#ifndef PRESETBROWSER_INCLUDED
#define PRESETBROWSER_INCLUDED

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
    virtual TextEditor* createEditorComponent ()
    {
        TextEditor* t = Label::createEditorComponent();
        t->setIndents(4, 7);
        return t;
    }  
    
    void textWasEdited() override
    {
        textWasChanged();
    }
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

    PresetBrowserSearchBar()
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
    
	void setHighlightColourAndFont(Colour c, Font f)
	{
		highlightColour = c;
		
		inputLabel->setFont(f);
		inputLabel->setColour(TextEditor::ColourIds::highlightColourId, highlightColour.withAlpha(0.4f));
		inputLabel->setColour(TextEditor::ColourIds::focusedOutlineColourId, highlightColour);
		inputLabel->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	}

    void paint(Graphics &g) override
    {
        g.setColour(highlightColour);
        g.drawRoundedRectangle(1.0f, 1.0f, (float)getWidth()-2.0f, (float)getHeight()-2.0f, 2.0f, 1.0f);
        
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
			virtual void deleteEntry(int columnIndex, int rowIndex, const File& file, bool doubleClick) = 0;
			virtual void renameEntry(int columnIndex, int rowIndex, const File& file, bool doubleClick) = 0;
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
								 public Label::Listener
{
public:

	// ============================================================================================

	class Listener
	{
	public:

        virtual ~Listener() {};
        
		virtual void presetChanged(const String& newPresetName) = 0;
	};

	void colourChanged() override
	{
		searchBar->setColour(PresetBrowserSearchBar::ColourIds::highlightColourId, findColour(PresetBrowserSearchBar::ColourIds::highlightColourId));
	}

	void buttonClicked(Button* /*b*/) override
	{
		destroy();
	}

	MultiColumnPresetBrowser(MainController* mc_, int width=810, int height=500);

	~MultiColumnPresetBrowser();

	void rebuildAllPresets();
	String getCurrentlyLoadedPresetName();

	void selectionChanged(int columnIndex, int rowIndex, const File& clickedFile, bool doubleClick);
	void renameEntry(int columnIndex, int rowIndex, const File& f, bool doubleClick);
	void deleteEntry(int columnIndex, int rowIndex, const File& f, bool doubleClick);

	void loadPreset(const File& f);

	void paint(Graphics& g);
	void resized();

	void setListener(Listener* l) { listener = l; }

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

	void setHighlightColourAndFont(Colour c, Colour& bgColour, Font f)
	{
		backgroundColour = bgColour;
		outlineColour = c;

		searchBar->setHighlightColourAndFont(c, f);
		bankColumn->setHighlightColourAndFont(c, f);
		categoryColumn->setHighlightColourAndFont(c, f);
		presetColumn->setHighlightColourAndFont(c, f);
	}

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
		int newIndex = -1;

		if (next)
		{
            newIndex = (currentlyLoadedPreset + 1) % (allPresets.size());
            
			const bool differentDirectories = allPresets[currentlyLoadedPreset].getParentDirectory() != allPresets[newIndex].getParentDirectory();

			if (stayInSameDirectory && differentDirectories)
			{
				// Get the first file of the directory
				File oldParentDirectory = allPresets[currentlyLoadedPreset].getParentDirectory();

				for (int i = 0; i < allPresets.size(); i++)
				{
					if (allPresets[i].getParentDirectory() == oldParentDirectory)
					{
						newIndex = i;
						break;
					}
				}
			}
		}
		else
		{
			if (currentlyLoadedPreset == 0)
			{
				newIndex = allPresets.size() - 1;
			}
			else
			{
				newIndex = (currentlyLoadedPreset - 1) % allPresets.size();
			}

			const bool differentDirectories = allPresets[currentlyLoadedPreset].getParentDirectory() != allPresets[newIndex].getParentDirectory();

			if (stayInSameDirectory && differentDirectories)
			{
				// Get the last file of the directory
				File oldParentDirectory = allPresets[currentlyLoadedPreset].getParentDirectory();

				for (int i = allPresets.size() - 1; i >= 0; i--)
				{
					if (allPresets[i].getParentDirectory() == oldParentDirectory)
					{
						newIndex = i;
						break;
					}
				}
			}

		}

		if (newIndex != currentlyLoadedPreset)
		{
			currentlyLoadedPreset = newIndex;

			File pFile = allPresets[currentlyLoadedPreset];
			File cFile = pFile.getParentDirectory();
			File bFile = cFile.getParentDirectory();

			bankColumn->setSelectedFile(bFile, sendNotification);
			categoryColumn->setSelectedFile(cFile, sendNotification);
			presetColumn->setSelectedFile(pFile, dontSendNotification);

			loadPreset(allPresets[currentlyLoadedPreset]);
		}
	}

	void setCurrentPreset(const File& f, NotificationType sendNotification)
	{
		int newIndex = allPresets.indexOf(f);

		if (newIndex != -1)
		{
			currentlyLoadedPreset = newIndex;
			
			presetColumn->setSelectedFile(allPresets[currentlyLoadedPreset]);

			if (listener != nullptr && sendNotification)
			{
				listener->presetChanged(f.getFileNameWithoutExtension());
			}
		}
	}

private:

	// ============================================================================================

	Colour backgroundColour;
	Colour outlineColour;

	PresetBrowserLookAndFeel pblaf;

	File rootFile;
	File currentBankFile;
	File currentCategoryFile;

	ScopedPointer<PresetBrowserSearchBar> searchBar;
	ScopedPointer<PresetBrowserColumn> bankColumn;
	ScopedPointer<PresetBrowserColumn> categoryColumn;
	ScopedPointer<PresetBrowserColumn> presetColumn;

	ScopedPointer<ShapeButton> closeButton;

	Array<File> allPresets;
	int currentlyLoadedPreset = -1;

	Listener* listener = nullptr;

	bool showOnlyPresets = false;
	String currentWildcard = "*";
	
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColumnPresetBrowser);

	// ============================================================================================

};


class MultiColumnPresetBrowserBar : public Component,
							   public MultiColumnPresetBrowser::Listener,
							   public Button::Listener
{
public:

	class PresetButton : public Button
	{
	public:
		PresetButton(const String& name, const Image &imageToUse) :
			Button(name),
			image(imageToUse)
		{};

		void setImage(const Image& newImage)
		{
			image = newImage;
			repaint();
		}

	private:

		void paintButton(Graphics& g, bool /*isMouseOverButton*/, bool isButtonDown)
		{
			if (image.isNull())
			{
				g.fillAll(Colours::red);
			}
			else
			{
				Rectangle<int> clipArea(0, isButtonDown ? 103 : 0, image.getWidth(), image.getWidth());
				Image clippedImage = image.getClippedImage(clipArea);
				g.drawImageWithin(clippedImage, 0, 0, getWidth(), getHeight(), RectanglePlacement::centred);
			}
		};

		Image image;
	};

	MultiColumnPresetBrowserBar(MultiColumnPresetBrowser* browser_):
		//displaySkin(ImageCache::getFromMemory(CustomFrontendToolbarImages::PresetDisplay_png, sizeof(CustomFrontendToolbarImages::PresetDisplay_png))),
		displaySkin(Image()),
		browser(browser_)
	{
		browser->setListener(this);

		addAndMakeVisible(prevButton = new PresetButton("Previous", Image()));
		addAndMakeVisible(nextButton = new PresetButton("Next", Image()));
		
		//prevButton->setImage(ImageCache::getFromMemory(CustomFrontendToolbarImages::PresetPrevButton_png, sizeof(CustomFrontendToolbarImages::PresetPrevButton_png)));
		//nextButton->setImage(ImageCache::getFromMemory(CustomFrontendToolbarImages::PresetNextButton_png, sizeof(CustomFrontendToolbarImages::PresetNextButton_png)));

		addAndMakeVisible(dropDownButton = new ShapeButton("Open Preset Browser", Colours::black.withAlpha(0.6f), Colours::black, Colours::black));

		Path p;
		p.startNewSubPath(0.0f, 0.0f);
		p.lineTo(1.0f, 0.0f);
		p.lineTo(0.5f, 1.0f);
		p.closeSubPath();

		dropDownButton->setShape(p, true, true, true);

		prevButton->addListener(this);
		nextButton->addListener(this);
		dropDownButton->addListener(this);

		setSize(305, 30);
	}

	void mouseDown(const MouseEvent& /*event*/)
	{
		if(loadOnClick) showPresetPopup();
	}

	

	void mouseMove(const MouseEvent& event)
	{
		const Rectangle<int> area(75, 2, 230, 28);

		loadOnClick = area.contains(event.getPosition());
		repaint();
	}

	void mouseExit(const MouseEvent& /*event*/)
	{
		loadOnClick = false;
		repaint();
	}

	void presetChanged(const String& newPresetName) override
	{
		currentPresetName = newPresetName;
		repaint();
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::black.withAlpha(loadOnClick ? 1.0f : 0.8f));

		g.drawImageWithin(displaySkin, 75, 2, 230, 28, RectanglePlacement::centred);

		if (currentPresetName.isNotEmpty())
		{
			g.setColour(Colours::black);
			g.drawText(currentPresetName, 75, 2, 230, 28, Justification::centred);
		}
	}

	void resized()
	{
		prevButton->setBounds(5, 2, 30, 30);
		nextButton->setBounds(40, 2, 30, 30);
		dropDownButton->setBounds(280, 9, 18, 14);
	}

	void buttonClicked(Button* b) override
	{
		
		if (b == nextButton)
		{
			browser->incPreset(true);
		}
		else if (b == prevButton)
		{
			browser->incPreset(false);
		}
		else if (b == dropDownButton)
		{
			showPresetPopup();
		}
	}

	void showPresetPopup();

	ScopedPointer<PresetButton> loadButton;
	

private:

	ScopedPointer<ShapeButton> dropDownButton;

	bool loadOnClick = false;

	Component::SafePointer<MultiColumnPresetBrowser> browser;

	ScopedPointer<PresetButton> prevButton;
	ScopedPointer<PresetButton> nextButton;

	Image displaySkin;

	String currentPresetName;

};

#endif