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

#ifndef BACKEND_EDITOR_H_INCLUDED
#define BACKEND_EDITOR_H_INCLUDED

#if HISE_IOS
#define SCROLLBAR_WIDTH 60
#else
#define SCROLLBAR_WIDTH 16
#endif

namespace hise { using namespace juce;

class ProcessorEditorPanel;
class PopupWindow;
class CustomKeyboard;
class BackendProcessor;
class ScriptContentContainer;

struct BackendHelpers
{
	/** Executes this function if the given Component is not in the root container to prevent false positive triggering. */
	static void callIfNotInRootContainer(std::function<void(void)> func, Component* c);
};


#define UI_OLD 0








class BackendProcessorEditor: public FloatingTileContent,
							  public Component,
							  public GlobalScriptCompileListener,
                              public Label::Listener,
							  public MainController::SampleManager::PreloadListener
{
public:

	enum MainPanelProperties
	{
		ScrollPosition = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		GlobalCodeFontSize,
		Autosaving,
		numPropertyIds
	};

	BackendProcessorEditor(FloatingTile* parent);

	~BackendProcessorEditor();

	SET_PANEL_NAME("MainPanel");

	int getFixedWidth() const override { return 900; }

	String getTitle() const override { return "Main Panel"; };

	void setRootProcessor(Processor *p, int scrollY=0);

	void rebuildContainer();

	void removeContainer();

	

	void preloadStateChanged(bool isPreloading) override;

	void viewedComponentChanged()
	{
		owner->setScrollY(viewport->viewport->getViewPosition().getY());

		clearPopup();
		
		resized();
	};

	var toDynamicObject() const override
	{
		auto obj =  FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, MainPanelProperties::ScrollPosition, viewport->viewport->getViewPosition().getY(), 0);
		

		return obj;
	}

	void fromDynamicObject(const var& obj) override
	{
		FloatingTileContent::fromDynamicObject(obj);

		owner->setScrollY(getPropertyWithDefault(obj, MainPanelProperties::ScrollPosition));

		

		
	}

	int getNumDefaultableProperties() const override
	{
		return (int)MainPanelProperties::numPropertyIds;
	}

	Identifier getDefaultablePropertyId(int id) const override
	{
		if (id < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(id);

		RETURN_DEFAULT_PROPERTY_ID(id, (int)MainPanelProperties::ScrollPosition, "ScrollPosition");
		RETURN_DEFAULT_PROPERTY_ID(id, (int)MainPanelProperties::GlobalCodeFontSize, "GlobalCodeFontSize");
		RETURN_DEFAULT_PROPERTY_ID(id, (int)MainPanelProperties::Autosaving, "Autosaving");

		jassertfalse;
		return {};
	}

	var getDefaultProperty(int id) const override
	{
		if (id < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(id);

		RETURN_DEFAULT_PROPERTY(id, (int)MainPanelProperties::ScrollPosition, var(0));
		RETURN_DEFAULT_PROPERTY(id, (int)MainPanelProperties::GlobalCodeFontSize, var(17));
		RETURN_DEFAULT_PROPERTY(id, (int)MainPanelProperties::Autosaving, var(true));

		jassertfalse;
		return {};
	}

	void paint(Graphics &g);

	void resized();

	void clearPopup();

	void refreshContainer(Processor *selectedProcessor)
	{
		if (container != nullptr)
		{
			const int y = viewport->viewport->getViewPositionY();

			setRootProcessor(container->getRootEditor()->getProcessor(), y);

			ProcessorEditor::Iterator iter(getRootContainer()->getRootEditor());

			while (ProcessorEditor *editor = iter.getNextEditor())
			{
				if (editor->getProcessor() == selectedProcessor)
				{
					editor->grabCopyAndPasteFocus();
				}
			}
		}
	}

	void scriptWasCompiled(JavascriptProcessor *sp) override;

	ModulatorSynthChain *getMainSynthChain() { return owner->synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const { return owner->synthChain; };

	void showPseudoModalWindow(Component *componentToShow, const String &title, bool ownComponent=false);
	
    void labelTextChanged(Label *) override
    {
        owner->setChanged();
    }
    
    void clearPreset();

	void clearModuleList();

    

	void loadNewContainer(const File &f);

	void loadNewContainer(const ValueTree &v);

	/** returns the ProcessorEditor for the path. */
    void refreshInterfaceAfterPresetLoad();
    
	ProcessorEditorContainer *getRootContainer() { return container; };

	BackendProcessor *getBackendProcessor() { return owner; };
	const BackendProcessor *getBackendProcessor() const { return owner; };

	void setViewportPositions(int viewportX, const int viewportY, const int viewportWidth, int viewportHeight);
	

	void setPluginPreviewWindow(PluginPreviewWindow *newWindow)
	{
		previewWindow = nullptr;

		previewWindow = newWindow;

		if (previewWindow != nullptr)
		{
#if HISE_IOS
            showPseudoModalWindow(newWindow, getMainSynthChain()->getId(), true);
#else
            
			newWindow->addToDesktop();
#endif
		}

	}

	bool isPluginPreviewShown() const;;
	bool isPluginPreviewCreatable() const;

	void storeSwatchColours(Colour *coloursFromColourPicker)
	{
		memcpy(swatchColours, coloursFromColourPicker, sizeof(Colour)*8);
	}

	void restoreSwatchColours(Colour *coloursFromColourPicker)
	{
		memcpy(coloursFromColourPicker, swatchColours, sizeof(Colour)*8);
	}

	

private:

	

	BackendRootWindow* parentRootWindow;

	Colour swatchColours[8];

	friend class BackendProcessor;
	
	friend class BackendCommandTarget;
    
	WeakReference<Processor> currentRootProcessor;

	LookAndFeel_V3 lookAndFeelV3;

	ScopedPointer<ProcessorEditorContainer> container;

    ScopedPointer<CachedViewport> viewport;
	
	//SafeChangeBroadcaster moduleListNotifier;

	BackendProcessor *owner;

	PopupLookAndFeel plaf;

	WeakReference<Component> currentPopupComponent;
	ScopedPointer<Component> ownedPopupComponent;

    WeakReference<CopyPasteTarget> currentlySelectedCopyableObject;
    
	ScopedPointer<ProcessorEditor> popupEditor;
	
	
	ScopedPointer<BreadcrumbComponent> breadCrumbComponent;

	ScopedPointer<PluginPreviewWindow> previewWindow;

	ScopedPointer<DebugLoggerComponent> debugLoggerWindow;

	bool rootEditorIsMainSynthChain;

	std::atomic<bool> isLoadingPreset;
};



class MainTopBar : public FloatingTileContent,
				   public Component,
				   public ButtonListener,
				   public FloatingTile::PopupListener,
				   public juce::ApplicationCommandManagerListener,
				   public MainController::SampleManager::PreloadListener,
				   public PooledUIUpdater::SimpleTimer,
				   public ComponentWithHelp
{
public:
	
	enum class PopupType
	{
		About,
		Macro,
		PluginPreview,
		Settings,
		PresetBrowser,
        CustomPopup,
        Keyboard,
		numPopupTypes
	};

	MainTopBar(FloatingTile* parent);

	~MainTopBar();

	int getFixedHeight() const override
	{
		return 40;
	}

	String getMarkdownHelpUrl() const override
	{
		return "/ui-components/floating-tiles/hise/maintopbar";
	}

	void timerCallback() override
	{
		auto newProgress = getMainController()->getSampleManager().getPreloadProgressConst();
		auto m = getMainController()->getSampleManager().getPreloadMessage();
		auto state = preloadState;

		if (newProgress != preloadProgress ||
			m != preloadMessage ||
			state != preloadState)
		{
			preloadState = state;
			preloadMessage = m;
			preloadProgress = newProgress;
			repaint();
		}
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	void popupChanged(Component* newComponent) override;

	void preloadStateChanged(bool isPreloading) override
	{
		preloadState = isPreloading;

		if (isPreloading)
			start();
		else
		{
			timerCallback();
			stop();
		}
        
        repaint();
	}

	void paint(Graphics& g) override;

	void paintOverChildren(Graphics& g) override;

	void mouseDown(const MouseEvent& )
	{
		ComponentWithHelp::openHelp();
	}

	void buttonClicked(Button* b) override;

	void resized() override;

	SET_PANEL_NAME("MainTopBar");

	void togglePopup(PopupType t, bool shouldShow);

	void applicationCommandInvoked(const ApplicationCommandTarget::InvocationInfo& info) override
	{
		switch (info.commandID)
		{
		case BackendCommandTarget::WorkspaceScript: 
			mainWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			scriptingWorkSpaceButton->setToggleStateAndUpdateIcon(true);
			samplerWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			customWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			break;
		case BackendCommandTarget::WorkspaceSampler:
			mainWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			scriptingWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			samplerWorkSpaceButton->setToggleStateAndUpdateIcon(true);
			customWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			break;
		
		case BackendCommandTarget::WorkspaceCustom:
			mainWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			scriptingWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			samplerWorkSpaceButton->setToggleStateAndUpdateIcon(false);
			customWorkSpaceButton->setToggleStateAndUpdateIcon(true);
			break;
		}

	}

	
	void applicationCommandListChanged() {};
	

private:

	bool preloadState = false;
	double preloadProgress = 0.0;
	String preloadMessage;

	Rectangle<int> frontendArea;
	Rectangle<int> workspaceArea;

	ScopedPointer<ImageButton> hiseButton;

	ScopedPointer<ShapeButton> backButton;
	ScopedPointer<ShapeButton> forwardButton;

	ScopedPointer<ProcessorPeakMeter> peakMeter;
	ScopedPointer<ShapeButton> settingsButton;
	ScopedPointer<ShapeButton> layoutButton;

	ScopedPointer<HiseShapeButton> macroButton;
	ScopedPointer<ShapeButton> pluginPreviewButton;
	ScopedPointer<ShapeButton> presetBrowserButton;
    ScopedPointer<ShapeButton> customPopupButton;
    ScopedPointer<ShapeButton> keyboardPopupButton;

	ScopedPointer<HiseShapeButton> mainWorkSpaceButton;
	ScopedPointer<HiseShapeButton> scriptingWorkSpaceButton;
	ScopedPointer<HiseShapeButton> samplerWorkSpaceButton;
	ScopedPointer<HiseShapeButton> customWorkSpaceButton;

    ScopedPointer<Drawable> hiseIcon;
};


} // namespace hise

#endif
