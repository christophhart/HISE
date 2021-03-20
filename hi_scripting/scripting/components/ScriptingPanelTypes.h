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

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	void mouseDown(const MouseEvent& event) override;

	var getAdditionalUndoInformation() const override;

	void performAdditionalUndoInformation(const var& undoInformation) override;

	Identifier getProcessorTypeId() const override;

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override;

	void gotoLocation(Processor* p, const String& fileName, int charNumber);

private:

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

	class Editor : public Component,
				   public ComboBox::Listener,
				   public Button::Listener,
				   public ScriptComponentEditListener,
				   public Timer
	{
	public:

		Editor(Processor* p);

		void scriptComponentSelectionChanged() override;

		void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue) override;

		void resized() override;

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

		void timerCallback() override
		{
			updateUndoDescription();
		}

		void updateUndoDescription() override;

		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

		void setZoomAmount(double newZoomAmount);

		double getZoomAmount() const;

		void setEditMode(bool editModeEnabled);

		bool isEditModeEnabled() const;

		bool keyPressed(const KeyPress& key) override;

	public:

		struct Actions
		{
			static void deselectAll(Editor* e);
			
			static void rebuild(Editor* e);

			static void rebuildAndRecompile(Editor* e);

			static void zoomIn(Editor* e);

			static void zoomOut(Editor* e);

			static void toggleEditMode(Editor* e);

			static void distribute(Editor* editor, bool isVertical);
			static void align(Editor* editor, bool isVertical);
			static void undo(Editor * e, bool shouldUndo);
		};

#if 0
		struct UpdateLevelLed: public Component,
							   public ScriptingApi::Content::RebuildListener,
							   public GlobalScriptCompileListener
		{
		public:

			UpdateLevelLed(ScriptingApi::Content* c_) :
				c(c_)
			{
				c->addRebuildListener(this);
				c->getProcessor()->getMainController()->addScriptListener(this);
			}

			~UpdateLevelLed()
			{
				c->removeRebuildListener(this);
				c->getProcessor()->getMainController()->removeScriptListener(this);
			}

			void paint(Graphics& g) override
			{
				if (level == ScriptingApi::Content::UpdateLevel::FullRecompile)
				{
					g.setColour(Colours::red.withAlpha(0.7f));
				}
				else if (level == ScriptingApi::Content::UpdateLevel::UpdateInterface)
				{
					g.setColour(Colours::yellow.withAlpha(0.7f));
				}
				else
				{
					g.setColour(Colours::green.withAlpha(0.7f));
				}

				g.fillRect(getLocalBounds().reduced(4));

				g.setColour(Colours::white.withAlpha(0.4f));
				g.drawRect(getLocalBounds().reduced(4), 1);

			}

			void mouseDown(const MouseEvent& /*e*/)
			{
				handleAndClearUpdate();
			}
			
			void scriptWasCompiled(JavascriptProcessor *processor) override
			{
				if (dynamic_cast<Processor*>(processor) == c->getProcessor())
				{
					refresh();
				}
			}

			void contentWasRebuilt() override
			{
				refresh();
			}

			void handleAndClearUpdate()
			{
				c->clearRequiredUpdate();

				if (level == ScriptingApi::Content::UpdateLevel::UpdateInterface)
				{
					Actions::rebuild(findParentComponentOfClass<Editor>());
				}
				if (level == ScriptingApi::Content::UpdateLevel::FullRecompile)
				{
					Actions::rebuildAndRecompile(findParentComponentOfClass<Editor>());
				}

				refresh();
			}

		private:

			void refresh()
			{
				level = c->getRequiredUpdate();

				repaint();
			}

			ScriptingApi::Content::UpdateLevel level;

			ScriptingApi::Content* c;
		};
#endif

		double zoomAmount;

		GlobalHiseLookAndFeel klaf;

		ScopedPointer<ComboBox> zoomSelector;
		ScopedPointer<HiseShapeButton> editSelector;
		ScopedPointer<HiseShapeButton> cancelButton;

		ScopedPointer<HiseShapeButton> undoButton;
		ScopedPointer<HiseShapeButton> redoButton;
		ScopedPointer<HiseShapeButton> rebuildButton;

		ScopedPointer<HiseShapeButton> verticalAlignButton;
		ScopedPointer<HiseShapeButton> horizontalAlignButton;
		ScopedPointer<HiseShapeButton> verticalDistributeButton;
		ScopedPointer<HiseShapeButton> horizontalDistributeButton;

		ScopedPointer<MarkdownHelpButton> helpButton;

		ScopedPointer<Viewport> viewport;
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

