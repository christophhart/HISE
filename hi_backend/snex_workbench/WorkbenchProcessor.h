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

struct DspNetworkCompileExporter: public hise::DialogWindowWithBackgroundThread,
								  public ControlledObject
{
	DspNetworkCompileExporter(MainController* mc):
		DialogWindowWithBackgroundThread("Compile DSP networks"),
		ControlledObject(mc)
	{

		addBasicComponents(true);
	};

	void run() override;

	void threadFinished() override
	{

	}
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
		ToolsCompileNetworks,
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

	void recompiled(snex::ui::WorkbenchData::Ptr p)
	{
		if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(p->getCodeProvider()))
		{
			if (dnp->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
			{
				dgp->getParentShell()->setOverlayComponent(nullptr, 300);
				ep->getParentShell()->setOverlayComponent(new DspNetworkCodeProvider::OverlayComponent(dnp->source, p), 300);
			}
			else
			{
				ep->getParentShell()->setOverlayComponent(nullptr, 300);
				dgp->getParentShell()->setOverlayComponent(new DspNetworkCodeProvider::OverlayComponent(dnp->source, p), 300);
			}
		}
	}

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
	SnexWorkbenchEditor(const String &commandLine);

	~SnexWorkbenchEditor();

	void paint(Graphics&);
	void resized();
	void requestQuit();

private:

	File getRootFolder() const
	{
		return getProjectHandler().getSubDirectory(FileHandlerBase::DspNetworks);
	}

	void addFile(const File& f);

	
	WeakReference<DspNetworkProcessor> dnp;

	WeakReference<DspNetworkCodeProvider> df;

	SnexEditorPanel* ep;

	scriptnode::DspNetworkGraphPanel* dgp = nullptr;

	OpenGLContext context;

	hise::StandaloneProcessor standaloneProcessor;
	ApplicationCommandManager mainManager;
	ScopedPointer<FloatingTile> rootTile;
	MenuBarComponent menuBar;

	WeakReference<snex::ui::WorkbenchData> wb;

	Array<File> activeFiles;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnexWorkbenchEditor)
};

}
