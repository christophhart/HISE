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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SCRIPTINGPANELTYPES_H_INCLUDED
#define SCRIPTINGPANELTYPES_H_INCLUDED


class CodeEditorPanel : public PanelWithProcessorConnection

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

class HiseShapeButton : public ShapeButton
{
public:

	HiseShapeButton(const String& name, ButtonListener* listener, Path onShape_, Path offShape_=Path()):
		ShapeButton(name, Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white),
		onShape(onShape_)
	{
		if (offShape_.isEmpty())
			offShape = onShape;
		else
			offShape = offShape_;

		if (listener != nullptr)
			addListener(listener);

		refreshShape();
		refreshButtonColours();
	}

	void refreshButtonColours()
	{
		if (getToggleState())
		{
			setColours(Colour(SIGNAL_COLOUR).withAlpha(0.8f), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));
		}
		else
		{
			setColours(Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white);
		}
	}

	void refreshShape()
	{
		if (getToggleState())
		{
			setShape(onShape, false, true, true);
		}
		else
			setShape(offShape, false, true, true);
	}

	void refresh()
	{
		refreshShape();
		refreshButtonColours();
	}

	void toggle()
	{
		setToggleState(!getToggleState(), dontSendNotification);
		
		refresh();
	}

	void setShapes(Path newOnShape, Path newOffShape)
	{
		onShape = newOnShape;
		offShape = newOffShape;
	}

	Path onShape;
	Path offShape;
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

	struct Canvas;

	class Editor : public Component,
				   public ComboBox::Listener,
				   public Button::Listener
	{
	public:

		Editor(Processor* p);

		void resized() override;

		void refreshContent();

		void buttonClicked(Button* b) override;

		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

		void setZoomAmount(double newZoomAmount);

		double getZoomAmount() const;

		void setEditMode(bool editModeEnabled);

		bool isEditModeEnabled() const;

	public:

		double zoomAmount;

		KnobLookAndFeel klaf;

		ScopedPointer<ComboBox> zoomSelector;
		ScopedPointer<HiseShapeButton> editSelector;
		ScopedPointer<HiseShapeButton> compileButton;
		ScopedPointer<HiseShapeButton> cancelButton;

		ScopedPointer<HiseShapeButton> undoButton;
		ScopedPointer<HiseShapeButton> redoButton;

		ScopedPointer<Viewport> viewport;
	};

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	ScriptContentPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	
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

	void contentChanged() override
	{
		if (getProcessor() != nullptr)
		{
			getProcessor()->getMainController()->addScriptListener(this, false);
		}
	}

	Component* createContentComponent(int /*index*/) override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<JavascriptProcessor>(moduleList);
	}

private:

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

class ConnectorHelpers
{
public:
    
    static void tut(PanelWithProcessorConnection* connector, const Identifier &idToSearch);
    
private:
    
    
};

template <class ProcessorType> class GlobalConnectorPanel : public PanelWithProcessorConnection
{
public:


	GlobalConnectorPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{

	}

	Identifier getIdentifierForBaseClass() const override
	{
		return GlobalConnectorPanel<ProcessorType>::getPanelId();
	}

	static Identifier getPanelId()
	{
		String n;

		n << "GlobalConnector" << ProcessorType::getConnectorId().toString();

		return Identifier(n);
	}

	int getFixedHeight() const override { return 18; }

	Identifier getProcessorTypeId() const override
	{
		RETURN_STATIC_IDENTIFIER("Skip");
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	bool hasSubIndex() const override { return false; }

	Component* createContentComponent(int /*index*/) override
	{
		return new Component();
	}

    void contentChanged() override
	{
        Identifier idToSearch = ProcessorType::getConnectorId();
        
        ConnectorHelpers::tut(this, idToSearch);
        
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ProcessorType>(moduleList);
	};

private:

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



#endif  // SCRIPTINGPANELTYPES_H_INCLUDED
