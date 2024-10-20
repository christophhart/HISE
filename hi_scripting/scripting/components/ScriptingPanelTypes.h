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

#pragma once
#include "hi_core/hi_components/floating_layout/PanelWithProcessorConnection.h"

namespace hise { 
using namespace juce;

#define DECLARE_ID(x) static const juce::Identifier x(#x);

namespace InterfaceDesignerShortcuts
{
	DECLARE_ID(id_toggle_edit);
	DECLARE_ID(id_deselect_all);
	DECLARE_ID(id_rebuild);
	DECLARE_ID(id_lock_selection);
	DECLARE_ID(id_show_json);
    DECLARE_ID(id_show_panel_data_json);
	DECLARE_ID(id_duplicate);
}
#undef DECLARE_ID

class CodeEditorPanel : public PanelWithProcessorConnection,
						public GlobalScriptCompileListener

{
public:

	CodeEditorPanel(FloatingTile* parent);;

	~CodeEditorPanel();

	SET_PANEL_NAME("ScriptEditor");

	Component* createContentComponent(int index) override;

	void fillModuleList(StringArray& moduleList) override;

	void contentChanged() override;

    void setScriptFile(const String& sp)
    {
        scriptPath = sp;
    }
    
    void preSelectCallback(ComboBox* cb) override
    {
        scriptPath = {};
    }
    
	void fromDynamicObject(const var& object) override;

	var toDynamicObject() const override;

	static CodeEditorPanel* showOrCreateTab(FloatingTabComponent* parentTab, JavascriptProcessor* jp, int index);

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	void mouseDown(const MouseEvent& event) override;

	var getAdditionalUndoInformation() const override;

	void performAdditionalUndoInformation(const var& undoInformation) override;

	Identifier getProcessorTypeId() const override;

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override;

	void gotoLocation(Processor* p, const String& fileName, int charNumber);

	float scaleFactor = -1.0f;

private:

	ScopedPointer<JavascriptTokeniser> tokeniser;
    
    String scriptPath;
};



class ScriptContentPanel : public PanelWithProcessorConnection,
						   public GlobalScriptCompileListener
{
public:

	enum SpecialPanelIds
	{
		ZoomAmount = PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
		EditMode,
		numSpecialPanelIds
	};

	class Factory : public PathFactory
	{
	public:

		String getId() const override { return "Canvas"; }

		Path createPath(const String& id) const override;

		Array<KeyMapping> getKeyMapping() const override;

	};

	struct Canvas;

	static void initKeyPresses(Component* root);

	class Editor : public WrapperWithMenuBarBase,
				   public Button::Listener,
				   public ScriptComponentEditListener
	{
	public:

		Editor(Canvas* c);

        ~Editor()
        {
            zoomSelector->setLookAndFeel(nullptr);
        }
        
		void rebuildAfterContentChange() override;

		ValueTree getBookmarkValueTree() override { return {}; }

		void bookmarkUpdated(const StringArray& idsToShow) override
		{

		}

		void addButton(const String& b) override;;

		void scriptComponentSelectionChanged() override;

		void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue) override;

		virtual void zoomChanged(float newScalingFactor);;

#if 0
		void resized() override;
#endif

		void refreshContent();

		void paint(Graphics& g) override
		{
			auto total = getLocalBounds();

			auto topRow = total.removeFromTop(24);

			g.setColour(Colours::black.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.2f)));

			g.fillRect(topRow);

            PopupLookAndFeel::drawFake3D(g, topRow);
            
			g.setColour(Colour(0xFF262626));
			g.fillRect(total);
		}

		static void onInterfaceResize(Editor& e, int w, int h)
		{
			e.refreshContent();
		}

		void buttonClicked(Button* b) override;

		void updateUndoDescription() override;

		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

		void setZoomAmount(double newZoomAmount);

		double getZoomAmount() const;

		bool isEditModeEnabled() const;

		bool keyPressed(const KeyPress& key) override;

	public:

		bool centerAfterFirstCompile = true;

		Rectangle<int> lastBounds;
	
		

		static bool isSelected(Editor& e);
		static bool isSingleSelection(Editor& e);

		struct Actions
		{
			static bool deselectAll(Editor& e);
			static bool rebuild(Editor& e);
			static bool rebuildAndRecompile(Editor& e);
			static bool zoomIn(Editor& e);
			static bool zoomOut(Editor& e);
			static bool toggleEditMode(Editor& e);

			static bool toggleSuspension(Editor& e);

			static bool editJson(Editor& e);

			static bool debugCSS(Editor& e)
			{
				e.callRecursive<simple_css::HeaderContentFooter>(&e, [&](simple_css::HeaderContentFooter* r)
				{

					auto newEditor = new simple_css::HeaderContentFooter::CSSDebugger(*r);

					newEditor->setSize(400, 700);

					e.findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(newEditor, &e, {15, 30});
					return true;
				});
				
				return true;
			}

			static bool move(Editor& e);

			static bool lockSelection(Editor& e);

			static bool distribute(Editor* editor, bool isVertical);
			static bool align(Editor* editor, bool isVertical);
			static bool undo(Editor * e, bool shouldUndo);
		};

		LambdaBroadcaster<Image, float> overlayBroadcaster;
		Image currentOverlayImage;
		Array<File> currentOverlays;

		GlobalHiseLookAndFeel klaf;
		LookAndFeel_V4 slaf;
		ComboBox* zoomSelector;
		ComboBox* overlaySelector;
		Slider* overlayAlphaSlider;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Editor);
	};

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	ScriptContentPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		getMainController()->addScriptListener(this);
	};

	~ScriptContentPanel()
	{
		getMainController()->removeScriptListener(this);
	}
	
	SET_PANEL_NAME("ScriptContent");

	var toDynamicObject() const override;;

	void fromDynamicObject(const var& object) override;



	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds)
			return PanelWithProcessorConnection::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ZoomAmount, "ZoomAmount");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::EditMode, "EditMode");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds)
			return PanelWithProcessorConnection::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ZoomAmount, var(1.0));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::EditMode, var(false));

		jassertfalse;
		return{};
	}


	Identifier getProcessorTypeId() const override;

	Component* createContentComponent(int /*index*/) override;

	void fillModuleList(StringArray& moduleList) override;

	void contentChanged() override
	{
		PanelWithProcessorConnection::contentChanged();

		updateInterfaceListener(dynamic_cast<ProcessorWithScriptingContent*>(getConnectedProcessor()));
	}

	void updateInterfaceListener(ProcessorWithScriptingContent* sp)
	{
		if(sp == nullptr)
		{
			if(auto editor = getContent<Editor>())
			{
				if(lastContent != nullptr)
					lastContent->interfaceSizeBroadcaster.removeListener(*editor);

				lastContent = nullptr;
			}

			return;
		}

		if (sp == dynamic_cast<ProcessorWithScriptingContent*>(getConnectedProcessor()))
		{
			auto content = sp->getScriptingContent();

			if(auto editor = getContent<Editor>())
			{
				if(lastContent != nullptr)
						lastContent->interfaceSizeBroadcaster.removeListener(*editor);

				content->interfaceSizeBroadcaster.addListener(*editor, Editor::onInterfaceResize);

				lastContent = content;
				
				resized();
			}
		}
	}


private:

	WeakReference<ScriptingApi::Content> lastContent;

	Factory pathFactory;

};

class ServerControllerPanel : public PanelWithProcessorConnection
{
public:

	ServerControllerPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("ServerController");

	Identifier getProcessorTypeId() const override;

	Component* createContentComponent(int) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}
};
	

class ComplexDataManager : public PanelWithProcessorConnection
{
public:

	ComplexDataManager(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("ComplexDataManager");

	bool hasSubIndex() const override { return true; }

	Identifier getProcessorTypeId() const override;

	void fillIndexList(StringArray& l) override
	{
		l.add("Audio Files");
		l.add("Tables");
		l.add("Slider Packs");
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ExternalDataHolder>(moduleList);
	}

	Component* createContentComponent(int index) override;
};



class ScriptWatchTablePanel : public PanelWithProcessorConnection,
                              public snex::ui::WorkbenchManager::WorkbenchChangeListener
{
public:

    ScriptWatchTablePanel(FloatingTile* parent);

    ~ScriptWatchTablePanel();
    
    void workbenchChanged(snex::ui::WorkbenchData::Ptr newWorkbench) override
    {
        if(auto w = getContent<ScriptWatchTable>())
        {
            if(newWorkbench != nullptr && newWorkbench->getCodeProvider()->providesCode())
            {
                w->setHolder(newWorkbench.get());
            }
            else
            {
                w->setHolder(dynamic_cast<JavascriptProcessor*>(getProcessor()));
            }
        }
    }
    
	SET_PANEL_NAME("ScriptWatchTable");

	Identifier getProcessorTypeId() const override;

	Component* createContentComponent(int /*index*/) override;

	void fromDynamicObject(const var& object) override
	{
		columnData = object.getProperty("VisibleColumns", {});
		PanelWithProcessorConnection::fromDynamicObject(object);
	}

	var toDynamicObject() const override
	{
		var cToUse = columnData;

		if (auto sw = getContent<ScriptWatchTable>())
		{
			cToUse = sw->getColumnVisiblilityData();
		}

		if (!cToUse.isArray())
			cToUse = var(Array<var>());

		auto obj = PanelWithProcessorConnection::toDynamicObject();
		obj.getDynamicObject()->setProperty("VisibleColumns", cToUse);

		return obj;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

private:

	var columnData;
	const Identifier showConnectionBar;
};


class OSCLogger : public FloatingTileContent,
				  public Component,
				  private ListBoxModel,
				  public AsyncUpdater,
				  public PathFactory,
				  private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>
{
public:

	HiseShapeButton filterButton, clearButton, pauseButton;
	
	Path createPath(const String& url) const override;

	scriptnode::OSCConnectionData::Ptr lastData;

	ScopedPointer<OSCAddressPattern> searchPattern;

	struct MessageItem
	{
		MessageItem() :
			address("/")
		{};

		String message;
		Colour c;
		bool matchesDomain;
		bool isError;
		bool scaled = false;
		bool hasScriptCallback = false;
		bool hasCableConnection = false;

		OSCAddress address;
	};

	OSCLogger(FloatingTile* parent);

	~OSCLogger();

	SET_PANEL_NAME("OSCLogger");

	//==============================================================================
	int getNumRows() override
	{
		return displayedItems.size();
	}

	//==============================================================================
	void paintListBoxItem(int row, Graphics& g, int width, int height, bool rowIsSelected) override;

	//==============================================================================
	void addOSCMessage(const OSCMessage& message, int level = 0);

	void oscMessageReceived(const OSCMessage& message) override
	{
		addOSCMessage(message);
	}

	void oscBundleReceived(const OSCBundle& bundle) override
	{
		addOSCBundle(bundle);
	}

	//==============================================================================
	void addOSCBundle(const OSCBundle& bundle, int level = 0);

	//==============================================================================
	void addOSCMessageArgument(const MessageItem& m, const OSCArgument& arg, int level, const String& cableId);

	//==============================================================================
	void addInvalidOSCPacket(const char* /* data */, int dataSize);

	//==============================================================================
	void clear();

	void paint(Graphics& g);

	void resized() override;

	static void updateConnection(OSCLogger& logger, scriptnode::OSCConnectionData::Ptr data);

	//==============================================================================
	void handleAsyncUpdate() override;

	

private:

	ScrollbarFader fader;

	TextEditor searchBox;
	

	static String getIndentationString(int level)
	{
		return String().paddedRight(' ', 2 * level);
	}

	//==============================================================================
	Array<MessageItem> oscLogList;

	Array<MessageItem> displayedItems;

	scriptnode::routing::GlobalRoutingManager::Ptr rm;

	Rectangle<int> topRow;

	ListBox list;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCLogger);
	JUCE_DECLARE_WEAK_REFERENCEABLE(OSCLogger);
};

class ConsolePanel : public FloatingTileContent,
	public Component
{
public:

	SET_PANEL_NAME("Console");

	ConsolePanel(FloatingTile* parent);

	void resized() override;

	Console* getConsole() const { return console; }

	

private:

	ScopedPointer<Console> console;

};

// Forward declare a few floating tiles so that we can move the header to the implementation files

namespace ScriptingObjects {

struct ScriptBroadcasterPanel : public PanelWithProcessorConnection
{
	enum SpecialProperties
	{
		Active = PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
		numSpecialProperties
	};

	ScriptBroadcasterPanel(FloatingTile* parent);;

	SET_PANEL_NAME("ScriptBroadcasterMap");

	Identifier getProcessorTypeId() const override;

	Component* createContentComponent(int) override;

	void fillModuleList(StringArray& moduleList) override;

	bool active = false;

	void fromDynamicObject(const var& object) override
	{
		active = object.getProperty("Active", false);
		PanelWithProcessorConnection::fromDynamicObject(object);
	}

	var toDynamicObject() const override
	{
		auto obj = PanelWithProcessorConnection::toDynamicObject();
		obj.getDynamicObject()->setProperty("Active", active);

		return obj;
	}

	int getNumDefaultableProperties() const override
	{
		return SpecialProperties::numSpecialProperties;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds)
			return PanelWithProcessorConnection::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialProperties::Active, "Active");
		
		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds)
			return PanelWithProcessorConnection::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialProperties::Active, var(false));

		jassertfalse;
		return{};
	}
};

}


struct BackendCommandIcons
{
	static Path getIcon(int commandId);
};


} // namespace hise

