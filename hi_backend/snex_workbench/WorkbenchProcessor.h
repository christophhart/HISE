/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise {
using namespace juce;




class SnexWorkbenchEditor : public AudioProcessorEditor,
	public juce::MenuBarModel,
	public ApplicationCommandTarget,
	public ModalBaseWindow,
	public snex::ui::WorkbenchData::Listener
{
public:

	enum MenuItems
	{
		FileMenu,
		ToolsMenu,
		TestMenu,
		numMenuItems
	};

	enum MenuCommandIds
	{
		FileNew = 1,
		FileOpen,
		FileSave,
		FileSaveAndReload,
		FileSaveAll,
		FileSetProject,
		FileShowNetworkFolder,
		FileCloseCurrentNetwork,
		ToolsEffectMode,
		ToolsSynthMode,
		ToolsShowKeyboard,
		ToolsEditTestData,
		ToolsAudioConfig,
		ToolsCompileNetworks,
		ToolsClearDlls,
		ToolsRecompile,
		ToolsSetBPM,
		TestsRunAll,
		ProjectOffset = 10000,
		FileOffset = 9000,
		numCommandIds
	};

	StringArray getMenuBarNames() override
	{
		return { "File", "Tools", "Tests" };
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

	void recompiled(snex::ui::WorkbenchData::Ptr p);

	const MainController* getMainControllerToUse() const override
	{
		return  dynamic_cast<const MainController*>(getAudioProcessor());
	}

	virtual MainController* getMainControllerToUse() override
	{
		return dynamic_cast<MainController*>(getAudioProcessor());
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

	void createNewFile();

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
		int topLevelMenuIndex);

	//==============================================================================
	SnexWorkbenchEditor(BackendProcessor* bp);

	~SnexWorkbenchEditor();

	void paint(Graphics&);
	void resized();
	void requestQuit();

	void saveCurrentFile();

	BackendDllManager::Ptr dllManager;

	void setSynthMode(bool shouldBeSynthMode);

	bool getSynthMode() const;

	static DspNetwork::Holder* getNetworkHolderForNewFile(MainController* mc, bool getSynthHolder);

	void closeCurrentNetwork();

private:

	File getCurrentFile() { return currentFiles[synthMode ? 1 : 0]; }

	File currentFiles[2];

	ScopedPointer<DspNetworkListeners::PatchAutosaver> autosaver;

	bool isCppPreviewEnabled();

	DebuggableSnexProcessor* getCurrentSnexProcessor();

	//hise::StandaloneProcessor standaloneProcessor;
	ApplicationCommandManager mainManager;

	bool synthMode = false;

	struct MenuLookAndFeel : public PopupLookAndFeel
	{
		void drawMenuBarBackground(Graphics& g, int width, int height,
			bool, MenuBarComponent& menuBar);

		void drawMenuBarItem(Graphics& g, int width, int height,
			int /*itemIndex*/, const String& itemText,
			bool isMouseOverItem, bool isMenuOpen,
			bool /*isMouseOverBar*/, MenuBarComponent& menuBar);
	} mlaf;

	hise::WorkbenchInfoComponent infoComponent;
	hise::WorkbenchBottomComponent bottomComoponent;

	Array<File> networkFiles;
	StringArray recentProjectFiles;

	void addFile(File f);
	
	void loadNewNetwork(const File& f);

	friend class DspNetworkCompileExporter;

	WeakReference<DspNetworkCodeProvider> df;

	SnexEditorPanel* ep;

	WeakReference<scriptnode::DspNetworkGraphPanel> dgp = nullptr;

	OpenGLContext context;

	
	ScopedPointer<FloatingTile> rootTile;
	MenuBarComponent menuBar;

	WeakReference<snex::ui::WorkbenchData> wb;

	Array<File> activeFiles;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnexWorkbenchEditor)
};

struct BufferTestResult
{
	static bool isAlmostEqual(float a, float b, float errorDb = -80.0f)
	{
		return std::abs(a - b) < Decibels::decibelsToGain(errorDb);
	}

	BufferTestResult(const AudioSampleBuffer& a, const AudioSampleBuffer& b) :
		r(Result::ok())
	{
		if (a.getNumChannels() != b.getNumChannels())
			r = Result::fail("Channel amount mismatch");

		if (a.getNumSamples() != b.getNumSamples())
			r = Result::fail("Sample amount mismatch");

		if (!r.wasOk())
			return;

		for (int c = 0; c < a.getNumChannels(); c++)
		{
			BigInteger sampleStates;
			sampleStates.setBit(a.getNumSamples() - 1, true);
			sampleStates.clearBit(a.getNumSamples() - 1);

			for (int i = 0; i < a.getNumSamples(); i++)
			{
				auto as = a.getSample(c, i);
				auto es = b.getSample(c, i);

				if (!isAlmostEqual(as, es))
				{
					sampleStates.setBit(i, true);

					if (r.wasOk())
					{
						String d;
						d << "data[" << String(c) << "][" << String(i) << "]: value mismatch";

						r = Result::fail(d);
					}
				}
			}

			errorRanges.add(sampleStates);
		}
	}

	Result r;
	Array<BigInteger> errorRanges;
};

struct TestRunWindow : public hise::DialogWindowWithBackgroundThread,
					   public snex::ui::WorkbenchData::Listener,
					   public ControlledObject
{
	TestRunWindow(SnexWorkbenchEditor* editor_) :
		DialogWindowWithBackgroundThread("Run tests"),
		ControlledObject(editor_->getProcessor()),
		editor(editor_)
	{
		addComboBox("tests", collectNetworks(), "Networks to test");
		addComboBox("config", { "All", "Interpreted", "JIT", "Dynamic Library" }, "Configuration");

		addBasicComponents();
	};

	StringArray collectNetworks()
	{
		auto root = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::Networks);

		filesToTest = root.findChildFiles(File::findFiles, false, "*.xml");

		StringArray sa;

		sa.add("All Tests");

		for (auto f : filesToTest)
			sa.add(f.getFileNameWithoutExtension());

		return sa;
	}

	~TestRunWindow()
	{
	}

	using Configurations = Array<DspNetworkCodeProvider::SourceMode>;

	String getSoloTest()
	{
		auto cb = getComboBoxComponent("tests");

		if (cb->getSelectedItemIndex() != 0)
			return cb->getText();

		return "";
	}

	Configurations getConfigurations()
	{
		Configurations c;

		auto cb = getComboBoxComponent("config");

		if (cb->getSelectedItemIndex() == 0)
		{
			c.add(DspNetworkCodeProvider::SourceMode::InterpretedNode);
			c.add(DspNetworkCodeProvider::SourceMode::JitCompiledNode);

			if (editor->dllManager->getBestProjectDll(BackendDllManager::DllType::Current).existsAsFile())
				c.add(DspNetworkCodeProvider::SourceMode::DynamicLibrary);
		}
		else
		{
			c.add((DspNetworkCodeProvider::SourceMode)(cb->getSelectedItemIndex() - 1));
		}

		return c;
	}

	void writeConsoleMessage(const String& m)
	{
		getMainController()->getConsoleHandler().writeToConsole(m, 0, nullptr, Colours::white);
	}


	void run() override;

	void runTest(const File& f, DspNetworkCodeProvider::SourceMode c);

	void threadFinished() override;

	bool allOk = true;


	Array<File> filesToTest;

	SnexWorkbenchEditor* editor;
	WorkbenchData::Ptr data;
};

struct DspNetworkCompileExporter : public hise::DialogWindowWithBackgroundThread,
	public ControlledObject,
	public CompileExporter
{
	enum class DspNetworkErrorCodes
	{
		OK,
		NoNetwork,
		NonCompiledInclude,
        CppGenError,
		UninitialisedProperties
	};

	DspNetworkCompileExporter(Component* editor, BackendProcessor* bp);

	void run() override;

	void threadFinished() override;

	File getBuildFolder() const override;

private:

	DspNetwork* getNetwork();

	static Array<File> getIncludedNetworkFiles(const File& networkFile);

	SnexWorkbenchEditor* getEditorWorkbench() { return dynamic_cast<SnexWorkbenchEditor*>(editor); }

	BackendDllManager* getDllManager();

	Component* editor;

	ErrorCodes ok = ErrorCodes::UserAbort;

	File getFolder(BackendDllManager::FolderSubType t) const
	{
		return BackendDllManager::getSubFolder(getMainController(), t);
	}

	static bool isInterpretedDataFile(const File& f);

	void createIncludeFile(const File& sourceDir);

	void createProjucerFile();

	String errorMessage;

	Array<File> includedFiles;
	

	File getSourceDirectory(bool isDllMainFile) const;

	void createMainCppFile(bool isDllMainFile);

	snex::cppgen::CustomNodeProperties nodeProperties;
};


}
