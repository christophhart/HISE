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
					public DragAndDropContainer,
					public ApplicationCommandTarget,
					public TextEditor::Listener,
					public ProjectHandler::Listener,
					public ExpansionHandler::Listener
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

	void textEditorTextChanged(TextEditor& editor) override;

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
                isScriptFile(file) || isUserPresetFile(file) || isMidiFile(file);
        }
        
		bool isImageFile(const File& file) const
		{
			return ImageFileFormat::findImageFormatForFileExtension(file) != nullptr;
		}

		bool isAudioFile(const File& file) const
		{
			return MultiChannelAudioBufferDisplay::isAudioFile(file.getFullPathName());
		}
        
        bool isMidiFile(const File& file) const
        {
            return file.hasFileExtension("mid");
        }
        
		bool isXmlFile(const File& file) const
		{
			return file.hasFileExtension("xml");
		}

		bool isUserPresetFile(const File& file) const
		{
			return file.hasFileExtension("preset");
		}

        bool isDirectorySuitable(const File& directory) const override;

		bool isScriptFile(const File &f) const
		{
			return f.hasFileExtension("js");
		}

		void setWildcard(const String& wc)
		{
			additionalWildcard = wc;
		}

		String additionalWildcard;
    };
    
    
    void saveFavoriteFile()
    {
        ValueTree v("Favorites");
        
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
        
        for(int i = 0; i < v.getNumChildren(); i++)
        {
            favorites.add(new Favorite("", File()));
        
            favorites.getLast()->restoreFromValueTree(v.getChild(i));
        }
    }

    FileBrowserLookAndFeel laf;
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

    JUCE_DECLARE_WEAK_REFERENCEABLE(FileBrowser);
};

#define DECLARE_ID(x) static const Identifier x(#x);

namespace ProjectManagement {

namespace IDs
{
	DECLARE_ID(repository_url);
	DECLARE_ID(api_url);
	DECLARE_ID(token);
	DECLARE_ID(title);
	DECLARE_ID(number);
	DECLARE_ID(comments);
	DECLARE_ID(url);
	DECLARE_ID(body);
	DECLARE_ID(name);
	DECLARE_ID(status);
	DECLARE_ID(labels);
	DECLARE_ID(tags);
	DECLARE_ID(color);
	DECLARE_ID(active);
	DECLARE_ID(expected_duration);
	DECLARE_ID(duration);
	DECLARE_ID(finished);
	DECLARE_ID(order);
	DECLARE_ID(last_time);
	DECLARE_ID(finish_time);
	DECLARE_ID(data);
	DECLARE_ID(priority_tags);
	DECLARE_ID(users);
}

// TODO: fix multiple active tables
// TODO: fix URL when clicking on cell
// TODO: show issue & title when resuming work
// TODO: check logic if don't add to the list
// TODO: implement finished list
// TODO: group / order / pin issues
// TODO: copy "fixed -#412" to clipboard for commit history
struct TableComponent:   public Component,
						 public ControlledObject,
						 public PathFactory,
						 public Timer,
						 public Thread,
						 public ProjectHandler::Listener,
						 public AsyncUpdater
{
	enum class SpecialFileType
	{
		RepositorySettings,
		FinishedTasks,
		CurrentTasks,
		numSpecialFileTypes
	};

	enum class RequestType
	{
		IssueList,
		Image,
		numRequestTypes
	};

	struct ResponseData
	{
		ResponseData():
		  ok(Result::fail("click to refresh issues")),
		  type(RequestType::numRequestTypes),
		  requested(false),
		  performed(false),
		  status(0),
		  url({})
		{}

		using Callback = std::function<void(const ResponseData&)>;

		bool requested = false;
		bool performed = false;
		operator bool() const { return ok.wasOk() && status == 200; }

		Result ok;
		RequestType type;
		URL url;
		int status;
		var obj;
		Callback finishCallback;

		bool perform();
		void draw(Graphics& g, Rectangle<int> area);

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponseData);
	};

	static constexpr int Timeout = 3000;
	static constexpr int TimerInterval = 1;

	static String getGenericPanelId() { return "ProjectManagementTracker"; }

	TableComponent(BackendRootWindow* window);
	~TableComponent();

	void projectChanged(const File& newRootDirectory) override;
	void reset(const File& newRoot);
	void init(SpecialFileType f);
	
	void refreshItems();
	void onResponse(const ResponseData& r);
	void getAsync(const URL& url, RequestType type, const ResponseData::Callback& f);
	void run() override;
	void handleAsyncUpdate() override;
	Path createPath(const String& url) const override;
	
	var load(SpecialFileType f) const;
	bool save(SpecialFileType f, const var& value);
	File getFile(SpecialFileType f) const;

	void timerCallback() override;
	void resized() override;
	void paint(Graphics& g) override;

	static String getSecondsAsTimeString(int numSeconds);
	static int getSecondsFromTimeString(const String& timeString);

private:

	void toggleCurrentTag(const String& s);
	void updateUIList();
	void cleanup();
	void rebuildIssues();

	struct IssueState: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<IssueState>;
		using List = ReferenceCountedArray<IssueState>;

		IssueState(const var& obj_);
		void restoreWebProperties(const var& data);
		Time getTimeFromVar(const String& possibleTimeString);
		var getVarFromTime(const Time& t) const;
		void fromJSON(const var& data);
		var toJSON() const;
		void parseBody(const String& b);

		// Restore these properties from the web JSON
		String body;
		Array<String> imageLinks;
		Array<std::tuple<String, Colour>> tags;
		bool webPropertiesRestored = false;
		int numComments = 0;

		// Restore these properties from and to the data JSON
		String title;
		float order = 0.0f;
		String status;
		int number;
		Time lastStartTime;
		Time finishTime;
		bool currentlyActive = false;
		bool finished = false;
		int totalNumSeconds = 0;
		int numExpected = 0;

		// non persistent properties
		int numToAddIfResumed = 0;

		void drawProgressBar(Graphics& g, Rectangle<float> area, float alpha);
	};

	struct AssetProvider: public MarkdownParser::ImageProvider
	{
		AssetProvider(MarkdownParser* parent_, TableComponent& parent, IssueState::Ptr c);;

		MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::WebBased; }
		Image getImage(const MarkdownLink& /*imageURL*/, float width) override ;
		ImageProvider* clone(MarkdownParser* newParent) const override { return new AssetProvider(parent, table, state); }
		Identifier getId() const override { RETURN_STATIC_IDENTIFIER("GithubAssetProvider"); };

		struct CachedImage  { Image img; int width; URL url; };

		TableComponent& table;
		Array<String> pendingURLs;
		IssueState::Ptr state;
		Array<CachedImage> cachedImages;
	};

	struct IssueComponent: public Component
	{
		static constexpr int Height = 60;
		static constexpr int Padding = 7;
		static constexpr int NodeMargin = 5;
		static constexpr int FontHeight = 16;

		struct Tag: public Component
		{
			static constexpr int Padding = 10;
			static constexpr int Margin = 10;

			Tag(const std::tuple<String, Colour>& tagData);

			void mouseDown(const MouseEvent& e) override;
			void updateSize(int height, Point<int> topLeft);
			void paint(Graphics& g) override;

			int textWidth = 0;
			String text;
			Colour c;
			bool selected = false;
			Font f;
		};

		

		IssueComponent(IssueState::Ptr state, TableComponent& parent);;

		void paintBackground(Graphics& g);
		void paint(Graphics& g) override;
		void resized() override;
		void mouseDown(const MouseEvent& event) override;
		void updateTagVisibility(const String& currentTag);

		bool matchesSearch(const String& searchTerm) const;

		OwnedArray<Tag> tags;
		TableComponent& parent;
		IssueState::Ptr state;
		HiseShapeButton activeButton;
		MarkdownHelpButton helpButton;
		int64 lastBodyHash = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(IssueComponent);
	};

	struct Content: public Component { void resized() override; } content;

	IssueState::Ptr getActiveIssue() const;

	struct CurrentOverlay: public Component,
						   public MarkdownDatabaseHolder
	{
		CurrentOverlay(TableComponent& parent_, IssueState::Ptr state_);

		void registerContentProcessor(MarkdownContentProcessor* processor) override {}
		void registerItemGenerators() override {}
		File getCachedDocFolder() const override { return {}; }
		File getDatabaseRootDirectory() const override { return {}; }

		void paint(Graphics& g) override;
		void resized() override;

		IssueState::Ptr state;
		TableComponent& parent;
		MarkdownPreview preview;
		HiseShapeButton closeButton;
	};

	struct CurrentStateComponent: public Component,
	                              public Timer
	{
		static constexpr int Height = 100;
		static constexpr int ButtonSize = 28;
		static constexpr int Padding = 10;
		static constexpr int FontHeight = 24;

		CurrentStateComponent(IssueState::Ptr s, TableComponent& parent_);;
		
		void bumpTime();
		void timerCallback() override;
		void paint(Graphics& g) override;
		void resized() override;;

		TableComponent& parent;
		IssueState::Ptr state;
		HiseShapeButton doneButton, suspendButton;
		
	};

	void updateImageLink(IssueState::Ptr s, int width)
	{
		for(auto i: issues)
		{
			if(i->state == s)
			{
				i->helpButton.setPopupWidth(jlimit(500, 1200, width));
				i->helpButton.setHelpText(s->body);
			}
		}

		if(currentOverlay != nullptr)
		{
			currentOverlay->preview.setNewText(currentOverlay->state->body, {}, true);
		}
	}

	void suspendIssue();
	void completeIssue();
	void startIssue(IssueComponent* ac);

	// Non-persistent members
	bool showCompleted = true;

	// State members
	bool initialised = false;
	String currentTag = {}; // property of repository?
	int clock = 0;
	File currentRoot;

	// Repository members
	bool usePrivateToken = false;
	String currentRepository = {};
	String currentApiUrl = {};
	StringArray tagPriorityList;
	StringArray users;

	// Data members
	IssueState::List currentIssues;
	IssueState::List finishedIssues;

	// Web members
	CriticalSection requestLock;
	OwnedArray<ResponseData> pendingRequests;
	OwnedArray<ResponseData> requestedRequests;
	OwnedArray<ResponseData> performedRequests;

	// UI members
	TextEditor searchBox;
	OwnedArray<IssueComponent> issues;
	ScrollbarFader sf;
	Viewport viewport;
	BackendRootWindow* root;
	HiseShapeButton refreshButton;
	ScopedPointer<CurrentStateComponent> currentStateComponent;
	ScopedPointer<CurrentOverlay> currentOverlay;
	
};
} // namespace ProjectTimeTable

} // namespace hise

#endif  // FILEBROWSER_H_INCLUDED
