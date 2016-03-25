/*
  ==============================================================================

    FileBrowser.h
    Created: 2 Jan 2016 2:22:05pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef FILEBROWSER_H_INCLUDED
#define FILEBROWSER_H_INCLUDED

class FileBrowser;

class FileBrowserToolbarFactory : public ToolbarItemFactory
{
public:

	FileBrowserToolbarFactory(FileBrowser *browser_) :
		browser(browser_)
	{

	};

	void getAllToolbarItemIds(Array<int> &ids) override;

	void getDefaultItemSet(Array<int> &ids) override { getAllToolbarItemIds(ids); };

	ToolbarItemComponent * createItem(int itemId);

	struct FileBrowserToolbarPaths
	{
		static Drawable *createPath(int id, bool isOn);
	};

private:

	FileBrowser *browser;

	
};


class BackendProcessorEditor;

class FileBrowser : public Component,
					public DragAndDropContainer,
					public ApplicationCommandTarget,
					public TextEditor::Listener
{
public:

	struct Favorite : public RestorableObject
	{
		Favorite(const String &name_, const File &directory_) :
			name(name_),
			directory(directory_)
		{};

		String name;
		File directory;

		void restoreFromValueTree(const ValueTree &previouslyExportedState)
		{
			name = previouslyExportedState.getProperty("Name");
			directory = File(previouslyExportedState.getProperty("File").toString());
		}

		ValueTree exportAsValueTree() const override
		{
			ValueTree v("Favorite");
			
			v.setProperty("Name", name, nullptr);
			v.setProperty("File", directory.getFullPathName(), nullptr);

			return v;
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Favorite);
	};

	/** All application commands are collected here. */
	enum FileBrowserToolbarCommands
	{
		ShowFavoritePopup = 0x70000,
		AddFavorite,
		RemoveFavorite,
		HardDisks,
		Back,
		Forward,
		numFileBrowserCommands
	};

	FileBrowser(BackendProcessorEditor *editor);

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void goToDirectory(const File &newRoot, bool useUndoManager=true);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;

	void paint(Graphics &g);

	void textEditorReturnKeyPressed(TextEditor& editor) override;

	~FileBrowser();

	bool keyPressed(const KeyPress& key) override;;

	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	void mouseDoubleClick(const MouseEvent& event) override;

private:

	class UndoableBrowseAction : public UndoableAction
	{
	public:

		UndoableBrowseAction(FileBrowser *browser_, File newDirectory):
			currentDirectory(newDirectory),
			browser(browser_)
		{
			lastDirectory = browser->directoryList->getDirectory();
		}

		bool perform() override
		{
            browser->goToDirectory(currentDirectory, false);
			
			return true;
		}

		bool undo() override
		{
            browser->goToDirectory(lastDirectory, false);
            
            return true;
		}

	private:

		File lastDirectory;
		File currentDirectory;

		FileBrowser *browser;
	};

	class AudioFileFilter : public FileFilter
	{
	public:

		AudioFileFilter() :
			FileFilter("AudioFileFilter")
		{};

		bool isFileSuitable(const File &file) const override
		{
			return file.hasFileExtension("hip") || AudioSampleBufferComponent::isAudioFile(file.getFullPathName());
		}
		

		bool isDirectorySuitable(const File &) const override
		{
			return true;
		}

	};
    
    void saveFavoriteFile()
    {
        ValueTree v("Favorites");
        
        for(int i = 0; i < favorites.size(); i++)
        {
            v.addChild(favorites[i]->exportAsValueTree(), -1, nullptr);
        }
        
        File favoritesFile(PresetHandler::getDataFolder() + "/Favorites.xml");
        
        favoritesFile.replaceWithText(v.toXmlString());
    }
    
    void loadFavoriteFile()
    {
        File favoritesFile(PresetHandler::getDataFolder() + "/Favorites.xml");
        
        ScopedPointer<XmlElement> xml = XmlDocument::parse(favoritesFile);
        
        if(xml == nullptr) return;
        
        ValueTree v = ValueTree::fromXml(*xml);
        
        for(int i = 0; i < v.getNumChildren(); i++)
        {
            favorites.add(new Favorite("", File::nonexistent));
        
            favorites.getLast()->restoreFromValueTree(v.getChild(i));
        }
    }
    

	friend class FileBrowserToolbarFactory;

	FileBrowserLookAndFeel laf;

	ScopedPointer<DirectoryContentsList> directoryList;

	TimeSliceThread directorySearcher;

	ScopedPointer<FileTreeComponent> fileTreeComponent;

	ScopedPointer<ShapeButton> favoriteButton;
	ScopedPointer<ShapeButton> addToFavoriteButton;
	ScopedPointer<ShapeButton> removeFromFavoritesButton;

	ScopedPointer<AudioFileFilter> fileFilter;

	var fileArray;

	PopupLookAndFeel plaf;

	ScopedPointer<UndoManager> browseUndoManager;
	ScopedPointer<ApplicationCommandManager> browserCommandManager;

	ScopedPointer<Toolbar> browserToolbar;
	ScopedPointer<ToolbarItemFactory> browserToolbarFactory;

	ScopedPointer<TextEditor> textEditor;

	OwnedArray<Favorite> favorites;
};



#endif  // FILEBROWSER_H_INCLUDED
