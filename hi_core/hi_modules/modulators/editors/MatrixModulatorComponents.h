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

namespace hise { using namespace juce;


struct MatrixBase: public Component,
				   public ControlledObject
					 
{
	static constexpr int Margin = 10;

	struct IntensitySlider: public Slider,
								public SliderWithShiftTextBox
	{
		IntensitySlider(const Identifier& id);

		void setCustomPopupFunction(const std::function<void(const MouseEvent&)>& f)
		{
			this->customPopupFunction = f;
		}

		void mouseDown(const MouseEvent& event) override
		{
			if(performModifierAction(event, false, true))
				return;

			down = true;

			Slider::mouseDown(event);
		}

		void mouseUp(const MouseEvent& event) override
		{
			if(performModifierAction(event, false, false))
				return;

			down = false;

			Slider::mouseUp(event);
		}

		void mouseDrag(const MouseEvent& event) override
		{
			if(performModifierAction(event, false, false))
				return;

			Slider::mouseDrag(event);
		}

		void mouseDoubleClick(const MouseEvent& event) override;

		void setActive(bool shouldBeActive, const ValueTree& connectionData, bool updateIntensity);

		int lastPseudoState = 0;
		modulation::TargetMode lastMode = modulation::TargetMode::Raw;
		ValueToTextConverter vtc;

		bool down = false;
	};

	struct HeaderBase: public simple_css::FlexboxComponent
	{
		HeaderBase();
		~HeaderBase() override {};

		int getHeightToUse(int w);
		void setCSS(simple_css::StyleSheet::Collection& css) override;
		Component* addHeaderItem(const Identifier& id, const String& prettyName);

		bool initialised = false;
	};

	struct SearchBar: public Label,
					  public Label::Listener,
					  public Timer
	{
		struct ClearButton: public Button
		{
			ClearButton();

			void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

			void resized() override
			{
				PathFactory::scalePath(icon, this, 4.0f);
			}

			String pathData;
			Path icon;
		};

		SearchBar():
		  Label("search"),
		  clearButton(),
		  searchIcon("274.t01CH7BQhhDuCIF35qBQhjbvCA0ElPDb.T7P+PMHDAG.EOjXAlsDDAG.EOD..d.QB1jqCA.fGPz7WI4PhA.fGPD7Dy1PAlsDDstW+LzOTCBQq60OCI11N6BQq60OC4GJ5PD7Dy1P9ghNDM+URNjX9ghND4QLcNzOygCQC9yoCYsh0PTY985PrE.fEQDEo87Pr4.C+PDiPw8Pr8.BuPjnHw6Pi01OTCBQJ9CWCI1gUaAQJ9CWCgBtNPDA0x2PnfqCDM+URNjXnfqCDIVUlNzgUaAQe.osC8C0fPzGPZ6Ph0vzpPzGPZ6PgAuLDIVUlNTXvKCQyekjCIVXvKCQDTKeC0vzpPjh+v0P+PMHDo3ObMzXkA")
		{
			setEditable(true);
			simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".search" });
			simple_css::FlexboxComponent::Helpers::appendToElementStyle(*this, "color:white;padding:10px;text-align:left;");



			addListener(this);
			setRepaintsOnMouseActivity(true);
			addChildComponent(clearButton);
		}

		void updateSearch(const String& searchTerm);

		void timerCallback() override
		{
			updateSearch(currentSearchTerm);
			stopTimer();
		}

		int getHeightToUse();

		String currentSearchTerm;

        void editorShown (Label*, TextEditor& te) override;

		void resized() override
		{
			Label::resized();
			clearButton.setBounds(getLocalBounds().removeFromRight(getHeight()));
		}

        void editorHidden (Label*, TextEditor& te)
		{
			te.removeListener(this);
			clearButton.setVisible(getText().isNotEmpty());
		}

		void labelTextChanged(Label*) override {}

		void textEditorTextChanged(TextEditor& te) override
		{
			currentSearchTerm = te.getText().toLowerCase();
			startTimer(300);
			
		}

		ClearButton clearButton;
		String searchIcon;
	};

	struct RowBase: public simple_css::FlexboxComponent,
					public ControlledObject
	{
		RowBase(MainController* mc):
		  ControlledObject(mc),
		  simple_css::FlexboxComponent(simple_css::Selector(simple_css::ElementType::TableRow))
		{};

		void setCSS(simple_css::StyleSheet::Collection& css)
		{
			FlexboxComponent::setCSS(css);
			useCSS = true;
		}

		~RowBase() override {};

		virtual void setIntensityConverter(MatrixModulator* mm);
		void setIntensityConverter(JavascriptMidiProcessor* jmp, int componentIndex);

		virtual String getTargetId() const = 0;

		void updateIntensityConverter();

		virtual void updateSearchVisibility(bool shouldBeVisible, const String& searchTerm)
		{
			setVisible(shouldBeVisible);
		}

		virtual int getNumIntensitySliders() const = 0;
		virtual IntensitySlider* getIntensitySlider(int index) = 0;
		virtual int getSourceIndexForIntensitySlider(int index) const = 0;
		virtual modulation::TargetMode getTargetModeForIntensitySlider(int index) const = 0;

		int getHeightToUse(int fullWidth);

		virtual int getDefaultRowHeight() const = 0;

		bool useCSS = false;

		OwnedArray<MatrixIds::Helpers::IntensityTextConverter> customConverters;
	};

	MatrixBase(MainController* mc):
	  ControlledObject(mc)
	{};

	virtual ~MatrixBase() {};

	virtual Identifier getMatrixType() const = 0;

	virtual void setSliderStyle(Slider::SliderStyle s)
	{
		sliderStyle = s;
	}

	void positionSearchBar(Rectangle<int>& b)
	{
		searchBar.setBounds(b.removeFromTop(searchBar.getHeightToUse()));
	}

	static String getColumnStyleSheet(const Identifier& id);

	Slider::SliderStyle sliderStyle = Slider::SliderStyle::RotaryHorizontalVerticalDrag;

	SearchBar searchBar;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MatrixBase);
};

struct MatrixContent: public MatrixBase
{
	static constexpr int RowHeight = 48;

	struct Parent: public simple_css::CSSRootComponent
	{
		Parent();
		~Parent() override {};

		static String getDefaultCSS();
	};

	struct Header: public HeaderBase
	{
		Header():
		  HeaderBase()
		{
			std::map<Identifier, String> pn;

			for(auto id: MatrixIds::Helpers::getWatchableIds())
				pn[id] = id.toString();

			pn[MatrixIds::SourceIndex] = "Source";
			pn[MatrixIds::TargetId] = "Target";
			pn[MatrixIds::AuxIndex] = "Aux";
			pn[MatrixIds::AuxIntensity] = "Aux Intensity";
			
			for(auto id: MatrixIds::Helpers::getWatchableIds())
				addHeaderItem(id, pn[id]);

			addHeaderItem("Plotter", "Plotter");
		}

	};

	struct Row: public RowBase,
				public Slider::Listener,
				public ComboBox::Listener,
				public ButtonListener
	{
		Row(MainController* mc, ValueTree rowData, const String& targetId);
		
		WeakReference<GlobalModulatorContainer> container;

		void lookAndFeelChanged() override;

		void rebuildItemList(const Identifier& id);
		StringArray getItemList(const Identifier& id) const;
		int getIndexInTarget() const;
		void updateValue(const Identifier& id, const var& newValue);

		int getNumIntensitySliders() const override { return 1; };
		IntensitySlider* getIntensitySlider(int index) override { return &intensitySlider; };
		int getSourceIndexForIntensitySlider(int index) const override { return (int)data[MatrixIds::SourceIndex]; }
		String getTargetId() const override { return data[MatrixIds::TargetId].toString(); }

		modulation::TargetMode getTargetModeForIntensitySlider(int index) const override
		{
			return (scriptnode::modulation::TargetMode)(int)data[MatrixIds::Mode];
		}

		void updateSearchVisibility(bool shouldBeVisible, const String& searchTerm) override
		{
			shouldBeVisible |= sourceSelector.getText().toLowerCase().contains(searchTerm);
			shouldBeVisible |= auxSelector.getText().toLowerCase().contains(searchTerm);
			RowBase::updateSearchVisibility(shouldBeVisible, searchTerm);
		}

		void addComponent(Component& c, const Identifier& id);
		void comboBoxChanged(ComboBox* cb) override;
		void sliderValueChanged(Slider* slider) override;

		void buttonClicked(Button* b) override
		{
			Identifier id(b->getName());
			data.setProperty(id, b->getToggleState(), um);
		}

		int valueToComboBoxIndex(const Identifier& id);
		var comboBoxIndexToValue(const Identifier& id, int comboBoxIndex);

		void resized() override;
		
		int getDefaultRowHeight() const override { return RowHeight; }

		void setSliderStyle(Slider::SliderStyle sliderStyle);

		
		ValueTree data;
		valuetree::PropertyListener valueUpdater;

		std::map<Identifier, StringArray> items;
		std::map<Identifier, ComboBox*> comboBoxes;

		
		
		ModPlotter plotter;
		ComboBox sourceSelector;
		ComboBox targetSelector;
		ComboBox modeSelector;
		
		IntensitySlider intensitySlider;
		IntensitySlider auxIntensitySlider;
		ToggleButton invertedButton;
		ComboBox auxSelector;
		UndoManager* um;
	};

	struct Controller: public simple_css::FlexboxComponent,
					   public ControlledObject,
					   public Button::Listener
	{
		struct ModulationDragger: public Component,
								  public simple_css::ComponentWithAutoTextSize,
		                          public SettableTooltipClient
		{
			ModulationDragger(const String& name);

			void mouseDown(const MouseEvent& e) override;
			void mouseUp(const MouseEvent& event) override;

			void paint(Graphics& g) override
			{
				using namespace simple_css;

				if(auto root = findParentComponentOfClass<CSSRootComponent>())
				{
					if(auto ss = root->css.getForComponent(this))
					{
						Renderer r(this, root->stateWatcher);

						auto state = r.getPseudoClassFromComponent(this);

						if(currentlySelected)
							state |= (int)PseudoClassType::Checked;

						r.setPseudoClassState(state, true);

						auto tb = getLocalBounds().toFloat();
						root->stateWatcher.checkChanges(this, ss, state);
						r.drawBackground(g, tb, ss);
						r.renderText(g, tb.reduced(2.0f, 0.0f), getName(), ss);
					}
				}
			}

			String getTextToAutofit() const override { return getName(); }

			String p64;
			Path p;
			bool currentlySelected = false;
		};

		Controller(MainController* mc, Component& p);

		~Controller();

		int getHeightToUse(int w);
		void setValueTree(const ValueTree& data_, const String& targetId_, UndoManager* um_);
		void paint(Graphics& g) override;
		void resized() override;
		void buttonClicked(Button* b) override;
		void initLaf(Component* p);

		int lastWidth = 0;
		int lastHeight = 0;

		ScopedPointer<simple_css::StyleSheetLookAndFeel> laf;
		String targetId;
		ValueTree data;
		UndoManager* um;
		TextButton addButton, removeButton, clearButton;
		OwnedArray<ModulationDragger> draggers;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Controller);
	};

	MatrixContent(MainController* mc, const String& targetId_, bool useViewport_);

	void onRowChange(ValueTree v, bool wasAdded);
	int getHeightToUse(int parentHeight) const;
	void resized() override;

	void paint(Graphics& g) override;

	Identifier getMatrixType() const override { return "TableMatrix"; }

	void setSliderStyle(Slider::SliderStyle newSliderStyle) override;
	void setUseViewport(bool shouldUseViewport);

	bool useViewport = false;

	ScrollbarFader sf;
	Viewport rowViewport;
	Component rowContent;

	
	Header header;
	OwnedArray<Row> rows;

	ValueTree data;
	valuetree::ChildListener rowListener;
	String targetId;
	std::function<void()> resizeFunction;
};


struct SliderMatrix: public MatrixBase
{
	SliderMatrix(MainController* mc, const String& targetFilter);;

	struct Header: public HeaderBase
	{
		Header(const StringArray& sources);
	};

	struct Row: public RowBase,
				public SliderListener 
	{
		Row(SliderMatrix& parent, const StringArray& sources, const String& targetId_);

		int getDefaultRowHeight() const override { return 28; }
		ValueTree getConnectionForSlider(int sourceIndex, bool addIfNotExisting=false) const;
		void sliderValueChanged(Slider* slider) override;
		void sliderDragEnded(Slider* slider) override;

		void handlePopup(const MouseEvent& e);
		void resized() override;

		void setIntensityConverter(MatrixModulator* mm) override
		{
			int idx = 0;
			for(auto s: intensitySliders)
			{
				if(!getConnectionForSlider(idx++).isValid())
				{
					auto m = mm->getMode();

					switch(m)
					{
					case Modulation::GainMode:
						s->lastMode = modulation::TargetMode::Gain;
					default:
						s->lastMode = mm->isBipolar() ? modulation::TargetMode::Bipolar : modulation::TargetMode::Unipolar;
						break;
					}
				}
			}

			RowBase::setIntensityConverter(mm);
		}

		String getTargetId() const override { return targetId; }

		int getNumIntensitySliders() const override { return intensitySliders.size(); };
		IntensitySlider* getIntensitySlider(int index) override { return intensitySliders[index]; };
		int getSourceIndexForIntensitySlider(int index) const override
		{
			return index;
		}

		void updateSearchVisibility(bool shouldBeVisible, const String& searchTerm) override;

		modulation::TargetMode getTargetModeForIntensitySlider(int index) const override;

		SliderMatrix& parent;
		String targetId;
		OwnedArray<IntensitySlider> intensitySliders;

		valuetree::RecursivePropertyListener connectionListener;
	};

	void paint(Graphics& g) override;
	void resized() override;
	void setSliderStyle(Slider::SliderStyle newSliderStyle) override;
	Identifier getMatrixType() const override { RETURN_STATIC_IDENTIFIER("SliderMatrix"); }
	void onPropertyChange(const ValueTree& v, const Identifier& id);

	void onConnectionChange(const ValueTree& v, bool wasAdded);

	Component content;
	Viewport viewport;
	ScrollbarFader sf;

	WeakReference<GlobalModulatorContainer> gc;

	StringArray sources;
	StringArray targets;

	
	ScopedPointer<Header> header;
	OwnedArray<Row> rows;

	valuetree::RecursivePropertyListener propertyListener;
	valuetree::ChildListener connectionListener;
};

struct ModulationMatrixBasePanel: public PanelWithProcessorConnection
{
	ModulationMatrixBasePanel(FloatingTile* parent);
	Identifier getProcessorTypeId() const override;
	void fillModuleList(StringArray& moduleList) override;
	Component* createContentComponent(int index) override;
	virtual Component* createMatrixComponent(const String& targetId) = 0;
};

struct ModulationMatrixPanel: public ModulationMatrixBasePanel
{
	enum SpecialPanelIds
	{
		SliderType = PanelWithProcessorConnection::SpecialPanelIds::numSpecialPanelIds,
		MatrixType,
		TargetFilter,
		numSpecialPanelIds
	};

	SET_PANEL_NAME("ModulationMatrix");
	ModulationMatrixPanel(FloatingTile* parent);

	var toDynamicObject() const override;
	void fromDynamicObject(const var& object) override;
	int getNumDefaultableProperties() const override;
	Identifier getDefaultablePropertyId(int index) const override;
	var getDefaultProperty(int index) const override;

	Identifier typeId;
	String targetId;
	Component* createMatrixComponent(const String& targetId) override;
};

struct ModulationMatrixControlPanel: public ModulationMatrixBasePanel
{
	SET_PANEL_NAME("ModulationMatrixController");
	ModulationMatrixControlPanel(FloatingTile* parent);
	Component* createMatrixComponent(const String& targetId) override;
};


} // namespace hise

