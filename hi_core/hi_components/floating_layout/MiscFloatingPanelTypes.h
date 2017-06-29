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

	var toDynamicObject() const override;;

	void fromDynamicObject(const var& object) override;


	int getNumDefaultableProperties() const override
	{
		return SpecialPanelIds::numSpecialPanelIds;
	}

	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < FloatingTileContent::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Text, "Text");
		
		jassertfalse;
		return {};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < FloatingTileContent::numPropertyIds)
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

	enum ColourIds
	{
		backgroundColour,
		numColourIds
	};

	SET_PANEL_NAME("Spacer");

	SpacerPanel(FloatingTile* parent) :
		FloatingTileContent(parent)
	{
		setInterceptsMouseClicks(false, false);

		initColours();

	}

	int getNumColourIds() const override { return numColourIds; }
	
	Identifier getColourId(int id) const override
	{
		static const Identifier bgColour("bgColour");

		if (id == ColourIds::backgroundColour)
			return bgColour;

		return Identifier();
	}

	void paint(Graphics& g) override;

	bool showTitleInPresentationMode() const override { return false; }

};

class ResizableFloatingTileContainer;

class VisibilityToggleBar : public FloatingTileContent,
						    public Component
{
public:

	enum ColourIds
	{
		backgroundColour,
		normalColour,
		overColour,
		onColour,
		numColourIds
	};

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
		auto c = getStyleColour(ColourIds::backgroundColour);

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
		if (index < FloatingTileContent::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Alignment, "Alignment");
		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::IconIds, "IconIds");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < FloatingTileContent::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Alignment, var((int)Justification::centred));
		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::IconIds, Array<var>());

		jassertfalse;
		return{};
	}

	void siblingAmountChanged() override;

	int getNumColourIds() const { return numColourIds; }

	Identifier getColourId(int /*colourId*/) const 
	{
		RETURN_STATIC_IDENTIFIER("backgroundColour");
	}

	Colour getDefaultColour(int /*colourId*/) const { return HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId); }

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

	enum ColourIds
	{
		backgroundColour = 0,
		numColourIds
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

	void paint(Graphics& g) override
	{
		g.setColour(getStyleColour(ColourIds::backgroundColour));
		g.fillAll();
	}

	void resized() override
	{
		int maxWidth = CONTAINER_WIDTH;

		if (getWidth() < maxWidth)
			keyboard->setBounds(0, 0, getWidth(), 72);
		else
			keyboard->setBounds((getWidth() - maxWidth) / 2, 0, maxWidth, 72);
	}

	int getNumColourIds() const { return numColourIds; }

	Identifier getColourId(int /*colourId*/) const { RETURN_STATIC_IDENTIFIER("backgroundColour"); }

	Colour getDefaultColour(int /*colourId*/) const { return Colours::transparentBlack; }

	int getFixedHeight() const override { return 72; }

private:
	
	ScopedPointer<CustomKeyboard> keyboard;
};


class PresetBrowserPanel : public FloatingTileContent,
						   public Component
{
public:

	enum ColourIds
	{
		backgroundColour = 0,
		highlightColour,
		numColourIds
	};

	SET_PANEL_NAME("PresetBrowser");

	PresetBrowserPanel(FloatingTile* parent):
		FloatingTileContent(parent)
	{
		addAndMakeVisible(presetBrowser = new MultiColumnPresetBrowser(getMainController()));
		
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
		presetBrowser->setHighlightColourAndFont(getStyleColour(ColourIds::highlightColour), getStyleColour(ColourIds::backgroundColour), GLOBAL_BOLD_FONT().withHeight(16.0f));
	}

	int getNumColourIds() const { return numColourIds; }

	Identifier getColourId(int colourId) const override
	{ 
		if (colourId == ColourIds::backgroundColour) { return Identifier("backgroundColour"); }
		if (colourId == ColourIds::highlightColour) { return Identifier("highlightColour"); }
	}

	Colour getDefaultColour(int colourId) const override
	{ 
		if (colourId == ColourIds::backgroundColour) { return Colours::black.withAlpha(0.97f); }
		if (colourId == ColourIds::highlightColour)  { return Colour(SIGNAL_COLOUR); }
	}

private:

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
