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


#ifndef MISCFLOATINGPANELTYPES_H_INCLUDED
#define MISCFLOATINGPANELTYPES_H_INCLUDED


namespace hise { using namespace juce;

class InterfaceContentPanel : public FloatingTileContent,
							  public Component,
							  public GlobalScriptCompileListener,
							  public ButtonListener,
							  public Processor::DeleteListener,
							  public GlobalSettingManager::ScaleFactorListener,
							  public ExpansionHandler::Listener,
							  public MainController::LockFreeDispatcher::PresetLoadListener
{
public:

	InterfaceContentPanel(FloatingTile* parent);;
	~InterfaceContentPanel();

	SET_PANEL_NAME("InterfacePanel");

	void paint(Graphics& g) override;
	void resized();

	void newHisePresetLoaded() override;

	void expansionPackLoaded(Expansion* currentExpansion) override;

	void scriptWasCompiled(JavascriptProcessor *processor) override;
	void buttonClicked(Button* b) override;

	void scaleFactorChanged(float newScaleFactor) override;

    void processorDeleted(Processor* deletedProcessor) override;
        
	void updateChildEditorList(bool forceUpdate) override {};

private:

	bool connectToScript();

	BlackTextButtonLookAndFeel laf;
	ScopedPointer<TextButton> refreshButton;
	WeakReference<Processor> connectedProcessor;
	ScopedPointer<ScriptContentComponent> content;

	void updateSize();
};



class EmptyComponent : public Component,
					   public FloatingTileContent,
					   public juce::SettableTooltipClient
{
public:

	SET_PANEL_NAME("Empty");

	EmptyComponent(FloatingTile* p);;

	void paint(Graphics& g) override;

	void mouseDown(const MouseEvent& event);

	
private:

	
	Colour c;
};

class SpacerPanel : public FloatingTileContent,
					public Component
{
public:

	SET_PANEL_NAME("Spacer");

	SpacerPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setInterceptsMouseClicks(false, false);

	}

	void paint(Graphics& g) override;

	bool showTitleInPresentationMode() const override { return false; }

};

class ResizableFloatingTileContainer;

class VisibilityToggleBar : public FloatingTileContent,
						    public Component
{
public:

	enum SpecialPanelIds
	{
		Alignment = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		IconIds,
		numSpecialPanelIds
	};

	VisibilityToggleBar(FloatingTile* parent);;

	SET_PANEL_NAME("VisibilityToggleBar");

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	int getFixedHeight() const override
	{
		return 26;
	}

	int getFixedWidth() const override
	{
		return 32;
	}

	void paint(Graphics& g) override
	{
		auto c = findPanelColour(PanelColourId::bgColour);

		g.fillAll(c);
	}

	void addCustomPanel(FloatingTile* panel)
	{
		customPanels.add(panel);
	}

	void setControlledContainer(FloatingTileContainer* containerToControl);

	void refreshButtons();

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	int getNumDefaultableProperties() const override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	void siblingAmountChanged() override;
	void resized() override;

private:

	void addIcon(FloatingTile* ft);

	struct Icon : public ButtonListener,
				  public Component
	{
		Icon(FloatingTile* controlledTile_);

		~Icon()
		{
			button->removeListener(this);
			button = nullptr;
			controlledTile = nullptr;
		}

		void resized() override
		{
			button->setBounds(getLocalBounds().reduced(3));
		}

		void refreshColour();

		void buttonClicked(Button*);

		bool on;

		Colour colourOff = Colours::white.withAlpha(0.4f);
		Colour overColourOff = Colours::white.withAlpha(0.5f);
		Colour downColourOff = Colours::white.withAlpha(0.6f);

		Colour colourOn = Colours::white.withAlpha(1.0f);
		Colour overColourOn = Colours::white.withAlpha(1.0f);
		Colour downColourOn = Colours::white.withAlpha(1.0f);

	private:
		
		ScopedPointer<ShapeButton> button;


		Component::SafePointer<FloatingTile> controlledTile;
	};

	StringArray pendingCustomPanels;

	Justification alignment = Justification::centred;

	Component::SafePointer<Component> controlledContainer;

	Array<Component::SafePointer<FloatingTile>> customPanels;

	OwnedArray<Icon> buttons;
};


struct ComplexDataEditorPanel : public PanelWithProcessorConnection
{
	ComplexDataEditorPanel(FloatingTile* parent, snex::ExternalData::DataType t);

	bool hasSubIndex() const override { return true; }

	Component* createContentComponent(int index) override;

	void fillModuleList(StringArray& moduleList) override
	{
		moduleList.addArray(ProcessorHelpers::getAllIdsForDataType(getMainController()->getMainSynthChain(), type));
	}

	void fillIndexList(StringArray& indexList) override;

protected:

	const snex::ExternalData::DataType type;
};


class TableEditorPanel : public ComplexDataEditorPanel
{
public:

	TableEditorPanel(FloatingTile* parent) :
		ComplexDataEditorPanel(parent, snex::ExternalData::DataType::Table)
	{};

	SET_PANEL_NAME("TableEditor");
};

class SliderPackPanel : public ComplexDataEditorPanel
{
public:

	SliderPackPanel(FloatingTile* parent) :
		ComplexDataEditorPanel(parent, snex::ExternalData::DataType::SliderPack)
	{};

	SET_PANEL_NAME("SliderPackEditor");
};

class PlotterPanel : public PanelWithProcessorConnection
{
public:

	PlotterPanel(FloatingTile* parent):
		PanelWithProcessorConnection(parent)
	{
		setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);
		setDefaultPanelColour(PanelColourId::itemColour1, Colour(0x88ffffff));
		setDefaultPanelColour(PanelColourId::itemColour2, Colour(0x44ffffff));
		setDefaultPanelColour(PanelColourId::itemColour3, Colours::transparentWhite);
		setDefaultPanelColour(PanelColourId::textColour, Colours::white);
	}

	SET_PANEL_NAME("Plotter");

	Component* createContentComponent(int /*index*/) override;

	Identifier getProcessorTypeId() const override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<TimeModulation>(moduleList);
	}
};


class ProcessorPeakMeter : public Component,
	public Timer
{
public:

	ProcessorPeakMeter(Processor* p);

	~ProcessorPeakMeter() override;

	void paint(Graphics& g) override;
	void resized() override;
	void timerCallback() override;

protected:

	ScopedPointer<VuMeter> vuMeter;
	WeakReference<Processor> processor;

};

template <class ContentType> class GenericPanel : public Component,
												  public FloatingTileContent
{
public:

	static Identifier getPanelId() { return ContentType::getGenericPanelId(); } 
	
	Identifier getIdentifierForBaseClass() const override { return getPanelId(); }

	GenericPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setInterceptsMouseClicks(false, true);

		addAndMakeVisible(component = new ContentType(getRootWindow()));
	}

	~GenericPanel()
	{
		component = nullptr;
	}

	ContentType* getContentFromGenericPanel() { return component; }

	void resized() override
	{
#if USE_FRONTEND
		component->setBounds(getLocalBounds());
#else
		component->setBounds(getParentContentBounds());
#endif
	}

private:

	ScopedPointer<ContentType> component;
};

} // namespace hise

#endif  // MISCFLOATINGPANELTYPES_H_INCLUDED
