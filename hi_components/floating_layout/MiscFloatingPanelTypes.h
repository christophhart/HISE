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


#ifndef MISCFLOATINGPANELTYPES_H_INCLUDED
#define MISCFLOATINGPANELTYPES_H_INCLUDED
			  

class Note : public Component,
	public FloatingTileContent,
	public TextEditor::Listener
{
public:

	enum SpecialPanelIds
	{
		Text = FloatingTileContent::PanelPropertyId::numPropertyIds,
		numSpecialPanelIds
	};

	SET_PANEL_NAME("Note");

	Note(FloatingTile* p);

	void resized() override;

	var toDynamicObject() const override
	{
		var obj = FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, SpecialPanelIds::Text, editor->getText(), String());

		return obj;
	}

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		editor->setText(getPropertyWithDefault(object, SpecialPanelIds::Text));
	}

	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Text, "Text");
		
		jassertfalse;
		return {};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Text, var(""));

		jassertfalse;
		return {};
	}

	void labelTextChanged(Label* l);

	int getFixedHeight() const override
	{
		return 150;
	}

private:

	PopupLookAndFeel plaf;

	ScopedPointer<TextEditor> editor;
};



class InterfaceContentPanel : public FloatingTileContent,
							  public Component,
							  public GlobalScriptCompileListener,
							  public ButtonListener,
							  public GlobalSettingManager::ScaleFactorListener
{
public:

	InterfaceContentPanel(FloatingTile* parent);;

	~InterfaceContentPanel();

	SET_PANEL_NAME("InterfacePanel");

	void paint(Graphics& g) override;

	void resized();

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


class PerformanceLabelPanel : public Component,
	public Timer,
	public FloatingTileContent
{
public:

	PerformanceLabelPanel(FloatingTile* parent):
		FloatingTileContent(parent)
	{
		addAndMakeVisible(statisticLabel = new Label());
		statisticLabel->setEditable(false, false);
		statisticLabel->setColour(Label::ColourIds::textColourId, Colours::white);
		

		startTimer(200);
	}

	SET_PANEL_NAME("PerformanceLabel");

	void timerCallback() override;

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		statisticLabel->setColour(Label::ColourIds::textColourId, findPanelColour(PanelColourId::textColour));
		statisticLabel->setFont(getFont());
	}

	void resized() override
	{
		statisticLabel->setFont(getFont());
		statisticLabel->setBounds(getLocalBounds());
	}


	bool showTitleInPresentationMode() const override
	{
		return false;
	}

private:

	ScopedPointer<Label> statisticLabel;

};

class ActivityLedPanel : public FloatingTileContent,
				    public Timer,
					public Component
{
public:

	enum SpecialPanelIds
	{
		OnImage = FloatingTileContent::PanelPropertyId::numPropertyIds,
		OffImage,
		ShowMidiLabel,
		numSpecialPanelIds
	};

	ActivityLedPanel(FloatingTile* parent):
		FloatingTileContent(parent)
	{
		setOpaque(true);

		

		startTimer(100);
	}

	SET_PANEL_NAME("ActivityLed");
	
	var toDynamicObject() const override
	{
		var obj = FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, (int)SpecialPanelIds::OffImage, offName);
		storePropertyInObject(obj, (int)SpecialPanelIds::OnImage, onName);
		storePropertyInObject(obj, (int)SpecialPanelIds::ShowMidiLabel, showMidiLabel);
		
		return obj;
	}

	void timerCallback()
	{
		const bool midiFlag = getMainController()->checkAndResetMidiInputFlag();

		setOn(midiFlag);
	}

	void fromDynamicObject(const var& object) override;

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OffImage, "OffImage");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OnImage, "OnImage");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowMidiLabel, "ShowMidiLabel");
		
		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OffImage, var(""));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OnImage, var(""));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowMidiLabel, true);
		
		jassertfalse;
		return{};
	}

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
		g.setColour(Colours::white);

		g.setFont(getFont());

		if(showMidiLabel)
			g.drawText("MIDI", 0, 0, 100, getHeight(), Justification::centredLeft, false);

		g.drawImageWithin(isOn ? on : off, showMidiLabel ? 38 : 2, 2, 24, getHeight(), RectanglePlacement::centred);
	}

	void setOn(bool shouldBeOn)
	{
		isOn = shouldBeOn;
		repaint();
	}

private:

	Path midiShape;

	bool showMidiLabel = true;

	String onName;
	String offName;

	bool isOn = false;

	Image on;
	Image off;
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
		Text = FloatingTileContent::PanelPropertyId::numPropertyIds,
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
		Alignment = FloatingTileContent::PanelPropertyId::numPropertyIds,
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

class MidiKeyboardPanel : public FloatingTileContent,
						  public Component,
					      public ComponentWithKeyboard
{
public:

	enum SpecialPanelIds
	{
		CustomGraphics = FloatingTileContent::PanelPropertyId::numPropertyIds,
		KeyWidth,
		LowKey,
		HiKey,
		BlackKeyRatio,
		DefaultAppearance,
		DisplayOctaveNumber,
		numProperyIds
	};

	SET_PANEL_NAME("Keyboard");

	MidiKeyboardPanel(FloatingTile* parent);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	CustomKeyboard* getKeyboard() const override
	{
		return keyboard;
	}

	~MidiKeyboardPanel()
	{
		keyboard = nullptr;
	}

	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numProperyIds;
	}

	var toDynamicObject() const override
	{
		var obj = FloatingTileContent::toDynamicObject();

		storePropertyInObject(obj, SpecialPanelIds::KeyWidth, keyboard->getKeyWidth());


		
		storePropertyInObject(obj, SpecialPanelIds::DisplayOctaveNumber, keyboard->isShowingOctaveNumbers());
		storePropertyInObject(obj, SpecialPanelIds::LowKey, keyboard->getRangeStart());
		storePropertyInObject(obj, SpecialPanelIds::HiKey, keyboard->getRangeEnd());
		storePropertyInObject(obj, SpecialPanelIds::CustomGraphics, keyboard->isUsingCustomGraphics());
		storePropertyInObject(obj, SpecialPanelIds::DefaultAppearance, defaultAppearance);
		storePropertyInObject(obj, SpecialPanelIds::BlackKeyRatio, keyboard->getBlackNoteLengthProportion());
		

		return obj;
	}

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		keyboard->setUseCustomGraphics(getPropertyWithDefault(object, SpecialPanelIds::CustomGraphics));
		
		keyboard->setRange(getPropertyWithDefault(object, SpecialPanelIds::LowKey),
						   getPropertyWithDefault(object, SpecialPanelIds::HiKey));
		
		keyboard->setKeyWidth(getPropertyWithDefault(object, SpecialPanelIds::KeyWidth));

		defaultAppearance = getPropertyWithDefault(object, SpecialPanelIds::DefaultAppearance);

		keyboard->setShowOctaveNumber(getPropertyWithDefault(object, SpecialPanelIds::DisplayOctaveNumber));

		keyboard->setBlackNoteLengthProportion(getPropertyWithDefault(object, SpecialPanelIds::BlackKeyRatio));
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::CustomGraphics, "CustomGraphics");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::KeyWidth, "KeyWidth");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::LowKey, "LowKey");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::HiKey, "HiKey");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::BlackKeyRatio, "BlackKeyRatio");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DefaultAppearance, "DefaultAppearance");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DisplayOctaveNumber, "DisplayOctaveNumber");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::CustomGraphics, false);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::KeyWidth, 14);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::LowKey, 9);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::HiKey, 127);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::BlackKeyRatio, 0.7);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DefaultAppearance, true);
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DisplayOctaveNumber, false);

		jassertfalse;
		return{};
	}

	

	void paint(Graphics& g) override
	{
		g.setColour(findPanelColour(PanelColourId::bgColour));
		g.fillAll();
	}

	void resized() override
	{
		if (defaultAppearance)
		{
			int width = jmin<int>(getWidth(), CONTAINER_WIDTH);

			keyboard->setBounds((getWidth() - width)/2, 0, width, 72);
		}
		else
		{
			keyboard->setBounds(0, 0, getWidth(), getHeight());
		}
        
	}

	int getFixedHeight() const override
	{
		return defaultAppearance ? 72 : FloatingTileContent::getFixedHeight();
	}

private:
	
	bool defaultAppearance = true;

	ScopedPointer<CustomKeyboard> keyboard;
};

class TooltipPanel : public FloatingTileContent,
	public Component
{
public:

	int getFixedHeight() const override
	{
		return 30;
	}

	SET_PANEL_NAME("TooltipPanel");

	TooltipPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setDefaultPanelColour(PanelColourId::bgColour, Colour(0xFF383838));
		setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.2f));
		setDefaultPanelColour(PanelColourId::textColour, Colours::white.withAlpha(0.8f));

		addAndMakeVisible(tooltipBar = new TooltipBar());;
	}

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		tooltipBar->setColour(TooltipBar::backgroundColour, findPanelColour(PanelColourId::bgColour));
		tooltipBar->setColour(TooltipBar::iconColour, findPanelColour(PanelColourId::itemColour1));
		tooltipBar->setColour(TooltipBar::textColour, findPanelColour(PanelColourId::textColour));
        
        tooltipBar->setFont(getFont());
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	~TooltipPanel()
	{
		tooltipBar = nullptr;
	}

	void resized() override
	{
		tooltipBar->setBounds(getLocalBounds());
	}


private:

	String fontName;
    float fontSize = 14.0f;

	ScopedPointer<TooltipBar> tooltipBar;
};

class PresetBrowserPanel : public FloatingTileContent,
						   public Component
{
public:

	SET_PANEL_NAME("PresetBrowser");

	PresetBrowserPanel(FloatingTile* parent):
		FloatingTileContent(parent)
	{
		setDefaultPanelColour(PanelColourId::bgColour, Colours::black.withAlpha(0.97f));
		setDefaultPanelColour(PanelColourId::itemColour1, Colour(SIGNAL_COLOUR));

		addAndMakeVisible(presetBrowser = new MultiColumnPresetBrowser(getMainController()));
	}

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		presetBrowser->setHighlightColourAndFont(findPanelColour(PanelColourId::itemColour1), findPanelColour(PanelColourId::bgColour), getFont());
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	~PresetBrowserPanel()
	{
		presetBrowser = nullptr;
	}

	void resized() override
	{
        presetBrowser->setBounds(getLocalBounds());
		presetBrowser->setHighlightColourAndFont(findPanelColour(PanelColourId::itemColour1), findPanelColour(PanelColourId::bgColour), getMainController()->getFontFromString(fontName, 16.0f));
	}

private:

	String fontName;
    

	ScopedPointer<MultiColumnPresetBrowser> presetBrowser;
};

class TableEditorPanel : public PanelWithProcessorConnection
{
public:

	TableEditorPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	SET_PANEL_NAME("TableEditor");

	Component* createContentComponent(int index) override
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(getProcessor());
		return new TableEditor(ltp->getTable(index));
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<LookupTableProcessor>(moduleList);
	}

	bool hasSubIndex() const override { return true; }

	void fillIndexList(StringArray& indexList) override
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(getConnectedProcessor());

		if (ltp != nullptr)
		{
			for (int i = 0; i < ltp->getNumTables(); i++)
			{
				indexList.add("Table " + String(i + 1));
			}
		}
	}
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

		auto mod = dynamic_cast<Modulator*>(getConnectedProcessor());

		p->addPlottedModulator(mod);

		return p;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<TimeModulation>(moduleList);
	}

private:

};


class SliderPackPanel : public PanelWithProcessorConnection
{
public:

	SliderPackPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
	};

	SET_PANEL_NAME("SliderPackEditor");

	Component* createContentComponent(int /*index*/) override
	{
		auto p = dynamic_cast<SliderPackProcessor*>(getProcessor());

		auto sp = new SliderPack(p->getSliderPackData(0));

		sp->setOpaque(true);
		sp->setColour(Slider::backgroundColourId, Colour(0xff333333));

		return sp;
	}

	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<SliderPackProcessor>(moduleList);
	}

	void resized() override;
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
		component->setBounds(getParentShell()->getContentBounds());
	}

private:

	ScopedPointer<ContentType> component;
};



#endif  // MISCFLOATINGPANELTYPES_H_INCLUDED
