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

#ifndef FILEBROWSER_H_INCLUDED
#define FILEBROWSER_H_INCLUDED
#include "hi_tools/hi_markdown/MarkdownRenderer.h"

namespace hise { using namespace juce;

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
		static std::unique_ptr<Drawable> createPath(int id, bool isOn);
	};

private:

	FileBrowser *browser;

	
};


class BackendProcessorEditor;

class FileBrowser : public Component,
					public MidiKeyboardFocusTraverser::ParentWithKeyboardFocus,
					public DragAndDropContainer,
					public ApplicationCommandTarget,
					public TextEditor::Listener,
					public ProjectHandler::Listener,
					public ExpansionHandler::Listener,
					public FileBrowserListener,
					public ChangeListener,
					public Timer
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
		HideEmptyFolders,
		numFileBrowserCommands
	};

	FileBrowser(BackendRootWindow* rootWindow);

	SET_GENERIC_PANEL_ID("FileBrowser");

    static void updateWorkspace(FileBrowser& f, const Identifier& id, Processor* p)
    {
        f.currentWorkspaceProcessor = p;
    }
    
	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void projectChanged(const File& newRootDirectory) override;

	void expansionPackCreated(Expansion* newExpansion) override
	{
		expansionPackLoaded(newExpansion);
	}

	void expansionPackLoaded(Expansion* currentExpansion) override;

	void goToDirectory(const File &newRoot, bool useUndoManager=true);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;

	void paint(Graphics &g);

	void previewFile(const File& f);

	void resetToRoot();

	void textEditorTextChanged(TextEditor& editor) override;

	/** Rescans the current directory (eg. after the Asset Manager wrote files) and keeps the open folders. */
	void refresh();

	~FileBrowser();

	bool keyPressed(const KeyPress& key) override;;

	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	void selectionChanged() override {}

	void fileClicked(const File& file, const MouseEvent& e) override;

	void fileDoubleClicked(const File& file) override;

	void browserRootChanged(const File&) override {}

	void timerCallback() override;

	void changeListenerCallback(ChangeBroadcaster*) override;

	/** Opens a script, stylesheet or shader in a code editor tab. Files that are not included by any
		script processor are opened as detached file. */
	void openInCodeEditor(const File& f);

private:

	FloatingTabComponent* getScriptEditorTabs();

	JavascriptProcessor* findProcessorThatIncludes(const File& f);

	JavascriptProcessor* findProcessorForCallbackFile(const File& f);

	File getActiveEditorFile();

	void revealActiveEditorFile();

	/** Opens one folder level per call. Returns true once the item exists. */
	bool revealFileInTree(const File& f, bool select = true);

	void showContextMenu();

	void setShowChangedOnly(bool shouldShowChangedOnly);

	/** Returns nullptr if the tree has an item without a name (getOpennessState() asserts on that). */
	std::unique_ptr<XmlElement> getSafeOpennessState();

	/** The open folders are stored per project root in the app data folder. */
	void saveOpennessState();

	void restoreSavedOpennessState();

	/** 1.0 right after the file changed on disk, fading to 0.0. */
	float getHighlightAlpha(const File& f) const;

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
			return file.hasFileExtension("hip") || MultiChannelAudioBufferDisplay::isAudioFile(file.getFullPathName());
		}
		

		bool isDirectorySuitable(const File &) const override
		{
			return true;
		}

	};
    
    class HiseFileBrowserFilter: public FileFilter
    {
    public:
        HiseFileBrowserFilter():
            FileFilter("HiseFileBrowserFilter")
        {
            
        };
        
        bool isFileSuitable(const File &file) const override
        {
			if (isHiddenByChangedFilter(file, false))
				return false;

			if (additionalWildcard.isNotEmpty())
			{
				if (file.isDirectory())
				{
					jassertfalse;
				}

				if (additionalWildcard.contains("*"))
				{
                    return file.getFullPathName().matchesWildcard(additionalWildcard, !File::areFileNamesCaseSensitive());
				}
				else
				{
                    auto st = additionalWildcard;
                    auto sm = file.getFileNameWithoutExtension();
                    
                    return FuzzySearcher::fitsSearch(additionalWildcard, sm, 0.4);
				}
			}

			return isListedFile(file);
		}

		/** The file types the browser shows, without the search and the changed files filter. */
		static bool isListedFile(const File& file)
		{
#if JUCE_WINDOWS
			if (file.getFileName().startsWith(".")) return false; // skip OSX hidden files on windows
#endif

			return file.hasFileExtension("hip") ||

				file.hasFileExtension("ttf") ||
#if JUCE_WINDOWS
				file.getFileName() == "LinkWindows" ||
#elif JUCE_LINUX
				file.getFileName() == "LinkLinux" ||
#else
				file.getFileName() == "LinkOSX" ||
#endif
				isAudioFile(file) || isImageFile(file) || isXmlFile(file) ||
                isScriptFile(file) || isUserPresetFile(file) || isMidiFile(file) || isCSSFile(file) || isShaderFile(file);
        }
        
		static bool isImageFile(const File& file)
		{
			return ImageFileFormat::findImageFormatForFileExtension(file) != nullptr;
		}

		static bool isAudioFile(const File& file)
		{
			return MultiChannelAudioBufferDisplay::isAudioFile(file.getFullPathName());
		}
        
        static bool isMidiFile(const File& file)
        {
            return file.hasFileExtension("mid");
        }

		static bool isCSSFile(const File& file)
        {
	        return file.hasFileExtension("css");
        }

		static bool isXmlFile(const File& file)
		{
			return file.hasFileExtension("xml");
		}

		static bool isUserPresetFile(const File& file)
		{
			return file.hasFileExtension("preset");
		}

        bool isDirectorySuitable(const File& directory) const override;

		static bool isScriptFile(const File &f)
		{
			return f.hasFileExtension("js");
		}

		static bool isShaderFile(const File& f)
		{
			return f.hasFileExtension("glsl");
		}

		static bool isCodeEditorFile(const File& f)
		{
			return isScriptFile(f) || isCSSFile(f) || isShaderFile(f);
		}

		void setWildcard(const String& wc)
		{
			additionalWildcard = wc;
		}

		/** Shows only the given files and their parent folders. nullptr shows everything. */
		void setChangedFiles(const std::map<String, int64>* files);

		bool isChangedOnly() const;

		/** True if the folder contains no files in any subfolder (hidden files like .gitkeep don't count). */
		static bool isEmptyFolder(const File& directory);

		String additionalWildcard;

		std::atomic<bool> hideEmptyFolders { false };

	private:

		// called from the directory scanner thread
		bool isHiddenByChangedFilter(const File& f, bool isFolder) const;

		CriticalSection changedLock;
		bool changedOnly = false;
		std::map<String, bool> changedFiles;
		std::map<String, bool> changedFolders;
    };
    
    
    void saveFavoriteFile()
    {
        ValueTree v("Favorites");
        
		if (auto ff = dynamic_cast<HiseFileBrowserFilter*>(fileFilter.get()))
			v.setProperty("HideEmptyFolders", ff->hideEmptyFolders.load(), nullptr);

        for(int i = 0; i < favorites.size(); i++)
        {
            v.addChild(favorites[i]->exportAsValueTree(), -1, nullptr);
        }
        
        File favoritesFile = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("Favorites.xml");
        
        favoritesFile.replaceWithText(v.toXmlString());
    }
    
    void loadFavoriteFile()
    {
        File favoritesFile = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("Favorites.xml");
        
        auto xml = XmlDocument::parse(favoritesFile);
        
        if(xml == nullptr) return;
        
        ValueTree v = ValueTree::fromXml(*xml);
        
		// applied to the filter in the constructor, the filter doesn't exist yet
		savedHideEmptyFolders = v.getProperty("HideEmptyFolders", false);

        for(int i = 0; i < v.getNumChildren(); i++)
        {
            favorites.add(new Favorite("", File()));
        
            favorites.getLast()->restoreFromValueTree(v.getChild(i));
        }
    }

	/** Colours the rows with the git state and highlights files that just changed. */
	struct GitAwareLookAndFeel : public FileBrowserLookAndFeel
	{
		void drawFileBrowserRow(Graphics& g, int width, int height, const File& file, const String& filename, Image* icon,
								const String& fileSizeDescription, const String& fileTimeDescription, bool isDirectory,
								bool isItemSelected, int itemIndex, DirectoryContentsDisplayComponent& dcc) override;

		Component::SafePointer<BackendRootWindow> rootWindow;
		FileBrowser* browser = nullptr;
	};

    GitAwareLookAndFeel laf;
    PopupLookAndFeel plaf;
    
    TimeSliceThread directorySearcher;
    
	File currentlyPreviewFile;
    

	friend class FileBrowserToolbarFactory;

	

	ScopedPointer<DirectoryContentsList> directoryList;

	
    ScrollbarFader sf;
    
	ScopedPointer<FileTreeComponent> fileTreeComponent;

	ScopedPointer<ShapeButton> favoriteButton;
	ScopedPointer<ShapeButton> addToFavoriteButton;
	ScopedPointer<ShapeButton> removeFromFavoritesButton;

	ScopedPointer<FileFilter> fileFilter;

	Component::SafePointer<BackendRootWindow> rootWindow;

	var fileArray;

	

	ScopedPointer<UndoManager> browseUndoManager;
	ScopedPointer<ApplicationCommandManager> browserCommandManager;

	ScopedPointer<Toolbar> browserToolbar;
	ScopedPointer<ToolbarItemFactory> browserToolbarFactory;

	ScopedPointer<TextEditor> textEditor;

	OwnedArray<Favorite> favorites;
    
    WeakReference<Processor> currentWorkspaceProcessor;
    
	Rectangle<float> pathArea;

	Rectangle<float> searchIcon;


	std::unique_ptr<XmlElement> prevState;

	int numOpennessRestoreTicks = 0;

	File lastActiveEditorFile;

	File pendingRevealFile;

	int numRevealAttempts = 0;

	int64 lastFileSetHash = 0;

	bool savedHideEmptyFolders = false;

	File lastChangedRoot;
	std::map<String, int64> lastChangedFiles;
	std::map<String, uint32> highlightStart;
	Array<File> pendingExpandFiles;
	int numExpandAttempts = 0;

	Component::SafePointer<FloatingTabComponent> scriptEditorTabs;

    JUCE_DECLARE_WEAK_REFERENCEABLE(FileBrowser);
};


} // namespace hise

#endif  // FILEBROWSER_H_INCLUDED
