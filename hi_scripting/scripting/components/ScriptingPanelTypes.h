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

namespace hise { 
using namespace juce;

class CodeEditorPanel : public PanelWithProcessorConnection,
						public GlobalScriptCompileListener

{
public:

	CodeEditorPanel(FloatingTile* parent);;

	~CodeEditorPanel();

	SET_PANEL_NAME("ScriptEditor");

	Component* createContentComponent(int index) override;

	void fillModuleList(StringArray& moduleList) override;

	void contentChanged() override
	{
		refreshIndexList();
	}

	void fromDynamicObject(const var& object) override;

	var toDynamicObject() const override;


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

	var settings;
	ScopedPointer<JavascriptTokeniser> tokeniser;
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

	class Editor : public WrapperWithMenuBarBase,
				   public Button::Listener,
				   public ScriptComponentEditListener
	{
	public:

		Editor(Canvas* c);

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

		struct Actions
		{
			static bool deselectAll(Editor& e);
			static bool rebuild(Editor& e);
			static bool rebuildAndRecompile(Editor& e);
			static bool zoomIn(Editor& e);
			static bool zoomOut(Editor& e);
			static bool toggleEditMode(Editor& e);

			static bool lockSelection(Editor& e);

			static bool distribute(Editor* editor, bool isVertical);
			static bool align(Editor* editor, bool isVertical);
			static bool undo(Editor * e, bool shouldUndo);
		};

		GlobalHiseLookAndFeel klaf;
		ComboBox* zoomSelector;
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

private:

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



class ScriptWatchTablePanel : public PanelWithProcessorConnection
{
public:

	ScriptWatchTablePanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		
	};

	SET_PANEL_NAME("ScriptWatchTable");

	Identifier getProcessorTypeId() const override;

	Component* createContentComponent(int /*index*/) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

private:

	const Identifier showConnectionBar;
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

struct BackendCommandIcons
{
	static Path getIcon(int commandId);
};


} // namespace hise

