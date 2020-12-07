/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent : public Component,
						     public juce::MenuBarModel,
						     public ApplicationCommandTarget,
						     public ModalBaseWindow
{
public:

	enum MenuItems
	{
		FileMenu,
		ToolsMenu,
		numMenuItems
	};

	enum MenuCommandIds
	{
		FileNew = 1,
		FileOpen,
		FileSave,
		FileSetProject,
		ToolsEditTestData,
		ToolsAudioConfig,
		ProjectOffset = 9000,
		FileOffset = 9000,
		numCommandIds
	};

	StringArray getMenuBarNames() override
	{
		return { "File", "Tools" };
	}

	/** This should return the popup menu to display for a given top-level menu.

		@param topLevelMenuIndex    the index of the top-level menu to show
		@param menuName             the name of the top-level menu item to show
	*/
	PopupMenu getMenuForIndex(int topLevelMenuIndex,
		const String& menuName);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;

	const MainController* getMainControllerToUse() const override
	{ 
		return dynamic_cast<const MainController*>(standaloneProcessor.getCurrentProcessor());
	}

	virtual MainController* getMainControllerToUse() override
	{ 
		return dynamic_cast<MainController*>(standaloneProcessor.getCurrentProcessor());
	}

	void setCommandTarget(ApplicationCommandInfo &result, const String &name, bool active, bool ticked, char shortcut, bool useShortCut = true, ModifierKeys mod = ModifierKeys::commandModifier) {
		result.setInfo(name, name, "Unused", 0);
		result.setActive(active);
		result.setTicked(ticked);

		if (useShortCut) result.addDefaultKeypress(shortcut, mod);
	};

	ProjectHandler& getProjectHandler()
	{
		return getProcessor()->getSampleManager().getProjectHandler();
	}

	const ProjectHandler& getProjectHandler() const
	{
		return getProcessor()->getSampleManager().getProjectHandler();
	}

	File getLayoutFile()
	{
		return getProjectHandler().getWorkDirectory().getChildFile("SnexWorkbenchLayout.js");
	}

	BackendProcessor* getProcessor()
	{
		return dynamic_cast<BackendProcessor*>(getMainControllerToUse());
	}

	const BackendProcessor* getProcessor() const
	{
		return dynamic_cast<const BackendProcessor*>(getMainControllerToUse());
	}

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	/** This is called when a menu item has been clicked on.

		@param menuItemID           the item ID of the PopupMenu item that was selected
		@param topLevelMenuIndex    the index of the top-level menu from which the item was
									chosen (just in case you've used duplicate ID numbers
									on more than one of the popup menus)
	*/
	virtual void menuItemSelected(int menuItemID,
		int topLevelMenuIndex)
	{}

	//==============================================================================
	MainContentComponent(const String &commandLine);

	~MainContentComponent();

	void paint(Graphics&);
	void resized();
	void requestQuit();

private:

	File getRootFolder() const
	{
		return getProjectHandler().getSubDirectory(FileHandlerBase::AdditionalSourceCode);
	}

	void addFile(const File& f);

	OpenGLContext context;

	FloatingTabComponent* tabs;

	hise::StandaloneProcessor standaloneProcessor;
	ApplicationCommandManager mainManager;
	FloatingTile rootTile;
	MenuBarComponent menuBar;

	Array<File> activeFiles;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

#if 0
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

	

	

private:
    //==============================================================================
    // Your private member variables go here...

	ScopedPointer<hise::StandaloneProcessor> processor;

	//juce::MenuBarComponent menuBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
#endif
