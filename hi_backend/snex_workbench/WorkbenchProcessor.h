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

struct BackendDllManager: public ReferenceCountedObject,
						  public ControlledObject
{
	struct FileCodeProvider: public snex::cppgen::ValueTreeBuilder::CodeProvider,
							 public ControlledObject
	{
		FileCodeProvider(const MainController* mc) :
			ControlledObject(const_cast<MainController*>(mc))
		{};

		String getCode(const snex::NamespacedIdentifier& path, const Identifier& classId) const override
		{
			auto codeFolder = getSubFolder(getMainController(), FolderSubType::CodeLibrary);
			auto pathFolder = codeFolder.getChildFile(path.getIdentifier().toString());
			auto f = pathFolder.getChildFile(classId.toString()).withFileExtension("h");
			jassert(f.existsAsFile());
			return f.loadFileAsString();
		}
	};

	enum class DllType
	{
		Debug,
		Release,
		Latest,
		numDllTypes
	};

	enum class FolderSubType
	{
		Root,
		Networks,
		Tests,
		CustomNodes,
		CodeLibrary,
		AdditionalCode,
		Binaries,
		DllLocation,
		ProjucerSourceFolder,
		Layouts,
		numFolderSubTypes
	};

	BackendDllManager(MainController* mc) :
		ControlledObject(mc)
	{}

	using Ptr = ReferenceCountedObjectPtr<BackendDllManager>;

	int getHash(bool getDllHash);
	bool unloadDll();
	bool loadDll(bool forceUnload);

	static File createIfNotDirectory(const File& f);
	static File getSubFolder(const MainController* mc, FolderSubType t);

	File getBestProjectDll(DllType t) const
	{
		auto dllFolder = getSubFolder(getMainController(), FolderSubType::DllLocation);

#if JUCE_WINDOWS
		String extension = "*.dll";
#else
		String extension = "*.dylib";
#endif

		auto files = dllFolder.findChildFiles(File::findFiles, false, extension);


		auto removeWildcard = "Never";

		if (t == DllType::Debug)
			removeWildcard = "release";
		if (t == DllType::Release)
			removeWildcard = "debug";

		for (int i = 0; i < files.size(); i++)
		{
			if (files[i].getFileName().contains(removeWildcard))
				files.remove(i--);
		}

		if (files.isEmpty())
			return File();

		struct FileDateComparator
		{
			int compareElements(const File& first, const File& second)
			{
				auto t1 = first.getCreationTime();
				auto t2 = second.getCreationTime();

				if (t1 < t2) return 1;
				if (t2 < t1) return -1;
				return 0;
			}
		};

		FileDateComparator c;
		files.sort(c);

		return files.getFirst();
	}

	scriptnode::dll::ProjectDll::Ptr projectDll;
};

class SnexWorkbenchEditor : public Component,
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
		FileSaveAll,
		FileSetProject,
		FileShowNetworkFolder,
		FileCloseCurrentNetwork,
		ToolsEffectMode,
		ToolsSynthMode,
		ToolsEditTestData,
		ToolsAudioConfig,
		ToolsCompileNetworks,
		ToolsClearDlls,
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
	SnexWorkbenchEditor(const String &commandLine);

	~SnexWorkbenchEditor();

	void paint(Graphics&);
	void resized();
	void requestQuit();

	void saveCurrentFile();

	BackendDllManager::Ptr dllManager;

	void setSynthMode(bool shouldBeSynthMode);

	static DspNetwork::Holder* getNetworkHolderForNewFile(MainController* mc, bool getSynthHolder);

	void closeCurrentNetwork();

private:

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

	

	Array<File> networkFiles;
	StringArray recentProjectFiles;

	void addFile(const File& f);
	
	friend class DspNetworkCompileExporter;

	WeakReference<DspNetworkProcessor> dnp;

	WeakReference<DspNetworkCodeProvider> df;

	SnexEditorPanel* ep;

	scriptnode::DspNetworkGraphPanel* dgp = nullptr;

	OpenGLContext context;

	hise::StandaloneProcessor standaloneProcessor;
	ApplicationCommandManager mainManager;
	ScopedPointer<FloatingTile> rootTile;
	MenuBarComponent menuBar;

	hise::CustomKeyboard keyboard;

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

			if (editor->dllManager->getBestProjectDll(BackendDllManager::DllType::Latest).existsAsFile())
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
	DspNetworkCompileExporter(SnexWorkbenchEditor* editor);

	void run() override;

	void threadFinished() override;

	File getBuildFolder() const override;

private:

	static Array<File> getIncludedNetworkFiles(const File& networkFile);

	SnexWorkbenchEditor* editor;

	ErrorCodes ok = ErrorCodes::UserAbort;

	File getFolder(BackendDllManager::FolderSubType t) const
	{
		return BackendDllManager::getSubFolder(getMainController(), t);
	}

	void createProjucerFile();

	Array<File> includedFiles;

	void createMainCppFile();
};


}
