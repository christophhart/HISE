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

private:

	bool connectToScript();

	BlackTextButtonLookAndFeel laf;
	ScopedPointer<TextButton> refreshButton;
	WeakReference<Processor> connectedProcessor;
	ScopedPointer<ScriptContentComponent> content;

	void updateSize();
};

class ActivationWindow : 
	public FloatingTileContent,
	public Component,
	public Button::Listener,
	public Timer,
	public OverlayMessageBroadcaster
{
public:

	ActivationWindow(FloatingTile* parent);

	SET_PANEL_NAME("ActivationPanel");

	void paint(Graphics &/*g*/) override
	{

	}

	void buttonClicked(Button*) override;

	void resized() override
	{
		int y = 20;
		productKey->setBounds(10, y, getWidth() - 20, 30);

		y += 40;

		statusLabel->setBounds(10, y, getWidth() - 20, 30);

		y += 40;

		submitButton->setBounds(10, y, getWidth() - 20, 30);

	}

	void timerCallback() override
	{
		refreshStatusLabel();
	}

private:

	bool good = false;

	void refreshStatusLabel();

	MainController* mc;

	ScopedPointer<Label> productKey;
	ScopedPointer<TextButton> submitButton;
	ScopedPointer<Label> statusLabel;

	BlackTextButtonLookAndFeel tblaf;
};



class PopoutButtonPanel : public Component,
	public FloatingTileContent,
	public Button::Listener
{
public:

	enum SpecialPanelIds
	{
		Text = (int)FloatingTileContent::PanelPropertyId::numPropertyIds,
		Width,
		Height,
		PopoutData,
		numSpecialPanelIds
	};

	enum ColourIds
	{
		backgroundColourId,
		textColourId,
		buttonColourId,
		numColourIds
	};

	SET_PANEL_NAME("PopupButton");

	PopoutButtonPanel(FloatingTile* p):
		FloatingTileContent(p)
	{
		addAndMakeVisible(button = new TextButton("Unused"));
		button->addListener(this);
		button->setLookAndFeel(&blaf);
		
		button->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
		button->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
	};

	~PopoutButtonPanel()
	{
		button = nullptr;
	}

	void buttonClicked(Button* b) override;

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	void resized() override;

	var toDynamicObject() const override
	{
		var obj = FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, SpecialPanelIds::Text, button->getButtonText());
		storePropertyInObject(obj, SpecialPanelIds::PopoutData, popoutData);
		storePropertyInObject(obj, SpecialPanelIds::Width, width);
		storePropertyInObject(obj, SpecialPanelIds::Height, height);
		
		return obj;
	}

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		button->setButtonText(getPropertyWithDefault(object, SpecialPanelIds::Text));

		popoutData = getPropertyWithDefault(object, SpecialPanelIds::PopoutData);
		width = getPropertyWithDefault(object, SpecialPanelIds::Width);
		height = getPropertyWithDefault(object, SpecialPanelIds::Height);
	}

	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Text, "ButtonText");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Width, "Width");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Height, "Height");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::PopoutData, "PopoutData");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Text, var("Popout Button"));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Width, var(300));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Height, var(300));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::PopoutData, var());

		jassertfalse;
		return{};
	}


private:

	var popoutData;

	int width;
	int height;

	BlackTextButtonLookAndFeel blaf;

	ScopedPointer<TextButton> button;
};




class EmptyComponent : public Component,
					   public FloatingTileContent,
					   public juce::SettableTooltipClient
{
public:

	SET_PANEL_NAME("Empty");

	EmptyComponent(FloatingTile* p) :
		FloatingTileContent(p)
	{
		Random r;

		setTooltip("Right click to create a Panel");

		setInterceptsMouseClicks(false, true);

		c = Colour(r.nextInt()).withAlpha(0.1f);
	};

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


	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Alignment, "Alignment");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::IconIds, "IconIds");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Alignment, var((int)Justification::centred));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::IconIds, Array<var>());

		jassertfalse;
		return{};
	}

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
	ComplexDataEditorPanel(FloatingTile* parent, snex::ExternalData::DataType t):
		PanelWithProcessorConnection(parent),
		type(t)
	{}

	bool hasSubIndex() const override { return true; }

	Component* createContentComponent(int index) override
	{
		if (auto pb = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
		{
			if (isPositiveAndBelow(index, pb->getNumDataObjects(type)))
			{
				auto obj = pb->getComplexBaseType(type, index);
				return dynamic_cast<Component*>(snex::ExternalData::createEditor(obj));
			}
		}

		return nullptr;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		moduleList.addArray(ProcessorHelpers::getAllIdsForDataType(getMainController()->getMainSynthChain(), type));
	}

	void fillIndexList(StringArray& indexList) override
	{
		if (auto pb = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
		{
			int numObjects = pb->getNumDataObjects(type);

			auto name = ExternalData::getDataTypeName(type);

			for (int i = 0; i < numObjects; i++)
				indexList.add(name + String(i + 1));
		}
	}

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

	}

	SET_PANEL_NAME("Plotter");

	Component* createContentComponent(int /*index*/) override
	{
		auto p = new Plotter();
		if (auto mod = dynamic_cast<Modulation*>(getConnectedProcessor()))
		{
			mod->setPlotter(p);
		}

		return p;
	}

	Identifier getProcessorTypeId() const override;

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<TimeModulation>(moduleList);
	}

private:

};





class ProcessorPeakMeter : public Component,
	public Timer
{
public:

	ProcessorPeakMeter(Processor* p) :
		processor(p)
	{
		addAndMakeVisible(vuMeter = new VuMeter());

		setOpaque(true);

		vuMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
		vuMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
		vuMeter->setColour(VuMeter::outlineColour, Colour(0x22000000));

		startTimer(30);
	}

	~ProcessorPeakMeter()
	{
		stopTimer();
		vuMeter = nullptr;
		processor = nullptr;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));
	}

	void resized() override
	{
		if (getWidth() > getHeight())
			vuMeter->setType(VuMeter::StereoHorizontal);
		else
			vuMeter->setType(VuMeter::StereoVertical);

		vuMeter->setBounds(getLocalBounds());
	}

	void timerCallback() override
	{
		if (processor.get())
		{
			const auto& values = processor->getDisplayValues();

			vuMeter->setPeak(values.outL, values.outR);
		}
	}

private:

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
