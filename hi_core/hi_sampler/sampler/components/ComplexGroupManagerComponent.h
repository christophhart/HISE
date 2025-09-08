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



struct ComponentWithGroupManagerConnection: public ControlledObject
{
	/** A super simple flexbox implementation:
	 *
	 *  - only left to right
	 *	- Justification: center or left (both vertically and horizontally)
	 *	- optional odd row offset
	 *
	 */
	struct SimpleFlexbox
	{
		int padding = 10;

		int oddRowOffset = 0;

		template <typename ContainerType> void applyBoundsListToComponents(const ContainerType& t, const std::vector<Rectangle<int>> & bounds)
		{
			for(int i = 0; i < t.size(); i++)
				t[i]->setBounds(bounds[i]);
		}

		template <typename ContainerType> std::vector<Rectangle<int>> createBoundsListFromComponents(const ContainerType& t)
		{
			std::vector<Rectangle<int>> list;

			for(auto c: t)
				list.push_back(c->getLocalBounds());

			return list;
		}

		Justification justification = Justification::topLeft;

		int apply(std::vector<Rectangle<int>>& elements, Rectangle<int> availableSpace);
	};

	ComponentWithGroupManagerConnection(ModulatorSampler* s);;

	ModulatorSampler* getSampler() { return sampler.get(); }
	const ModulatorSampler* getSampler() const { return sampler.get(); }

	SampleEditHandler* getSampleEditHandler() { return sampler->getSampleEditHandler(); }
	const SampleEditHandler* getSampleEditHandler() const { return sampler->getSampleEditHandler(); }

	ComplexGroupManager* getComplexGroupManager() { return sampler->getComplexGroupManager(); }
	const ComplexGroupManager* getComplexGroupManager() const { return sampler->getComplexGroupManager(); }

	UndoManager* getUndoManager() { return sampler->getUndoManager(); }

	

	virtual int getHeightToUse() const = 0;

	void setBoundsWithoutRecursion(Rectangle<int> newBounds)
	{
		if(!recursive)
		{
			ScopedValueSetter<bool> svs(recursive, true);

			auto asComponent = dynamic_cast<Component*>(this);

			if(asComponent->getBoundsInParent() == newBounds)
				asComponent->resized();
			else
				dynamic_cast<Component*>(this)->setBounds(newBounds);
		}
	}

	void resizeWithoutRecursion()
	{
		if(!recursive)
		{
			ScopedValueSetter<bool> svs(recursive, true);
			dynamic_cast<Component*>(this)->resized();
		}
	}

protected:

	

	void clearRulersAndLabels()
	{
		rulers.clear();
		labels.clear();
	}

	void addRuler(Rectangle<int> r)
	{
		rulers.add(r);
	}

	void addLabel(Rectangle<int> b, const String& label)
	{
		labels.add({ b.toFloat() , label });
	}

	void drawLabelsAndRulers(Graphics& g);

private:

	bool recursive = false;

	Array<std::pair<Rectangle<float>, String>> labels;
	Array<Rectangle<int>> rulers;

	WeakReference<ModulatorSampler> sampler;
};


class ComplexGroupManagerComponent: public Component,
								     public ComponentWithGroupManagerConnection,
									 public SamplerSubEditor,
									 public SampleMap::Listener
{
public:

	using LogicType = ComplexGroupManager::LogicType;
	using Bitmask = SynthSoundWithBitmask::Bitmask;

	struct Factory: public PathFactory
	{
		Path createPath(const String& url) const override;
	};

	int getHeightToUse() const override { return -1; }

	struct Helpers: public ComplexGroupManager::Helpers
	{
		static void drawSelection(Graphics& g, Rectangle<int> bounds);
		static void drawRuler(Graphics& g, Rectangle<int>& bounds);
		static void drawLabel(Graphics& g, Rectangle<float> bounds, const String& text, Justification j = Justification::left);

		static Path getPath(LogicType lt);

		static Colour getLayerColour(const ValueTree& v);
		
	};

	struct LayerComponent: public Component,
						   public ComponentWithGroupManagerConnection
	{
		static constexpr int DefaultHeight = 60;
		static constexpr int TopBarHeight = 30;
		static constexpr int BodyMargin = 20;
		static constexpr int PaddingLeft = 70;

		LayerComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& t, LogicType lt);
		~LayerComponent() override;;

		void paint(Graphics& g) override;
		void resized() override;
		void updateHeight(int newHeight);

		

		void mouseDoubleClick(const MouseEvent& event) override
		{
			if(data.getType() == groupIds::Layer)
			{
				auto isFolded = (bool)data[scriptnode::PropertyIds::Folded];
				data.setProperty(PropertyIds::Folded, !isFolded, nullptr);
			}
		}

		LogicType type = LogicType::numLogicTypes;
		Factory f;
		HiseShapeButton clearButton;
		Path p;
		String typeName;
		ValueTree data;
		valuetree::PropertyListener foldListener;

		Colour c;

		struct KeyboardComponent;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayerComponent);
	};

	struct SelectorLookAndFeel: public PopupLookAndFeel
	{
		void drawComboBox(Graphics& g, int , int , bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb) override;

		void drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& cb, Label&) override;

		void drawLabel(Graphics& g, Label& l) override;
	};

	struct FileTokenSelector: public Component,
							  public ComponentWithGroupManagerConnection
	{
		enum class Action
		{
			DoNothing,
			ParseFileToken,
			SetToFixGroup,
			SetIgnoreFlag,
			Unassign,
			numActions
		};
		
		struct FileToken: public Component,
						  public SettableTooltipClient
		{
			FileToken(const String& t);;

			void mouseDown(const MouseEvent& e) override;
			void paint(Graphics& g) override;

			int getBestWidth() const { return f.getStringWidth(token) + 20; }
			void setActive(bool shouldBeActive, bool shouldBeOk);

			Font f;
			const String token;
			bool ok = true;
			bool active = false;
		};

		FileTokenSelector(ComponentWithGroupManagerConnection& parent, const StringArray& layerTokens, bool showIgnoreButton, bool addMode);

		void checkApplyState();

		void rebuildComboBox();

		void setActiveToken(int idx = -1);
		void updateSelection(const StringArray& fileTokens);
		int getHeightToUse() const override;

		StringArray getValidTokens() const
		{
			if(!legatoMode)
				return layerTokens;
			else
			{
				StringArray validNames;

				for(int i = 0; i < 128; i++)
					validNames.add(String(i));
				for(int i = 0; i < 128; i++)
					validNames.add(MidiMessage::getMidiNoteName(i, true, true, 3));

				return validNames;
			}
		}

		uint8 getCurrentTokenValue(const String& filename) const;

		uint8 getSelectedTokenIndex() const;

		void paint(Graphics& g) override;
		void resized() override;

		bool useFileTokens() const;

		String performAction()
		{
			String msg;
			auto data = findParentComponentOfClass<LayerComponent>()->data;

			auto id = Helpers::getId(data);

			if(currentAction == Action::DoNothing)
				return "Skip layer " + id + "\n";

			if(currentAction == Action::Unassign)
			{
				for(auto s: *getSampleEditHandler())
				{
					getComplexGroupManager()->clearSampleId(s.get(), { id }, true);
					auto filename = Helpers::getSampleFilename(s.get());

					
					msg << "Unassigned layer " << id << " from sample " << filename;
					msg << "\n";
				}
			}
			else
			{
				for(auto s: *getSampleEditHandler())
				{
					Array<std::pair<Identifier, uint8>> values;

					auto filename = Helpers::getSampleFilename(s.get());
					auto v = getCurrentTokenValue(filename);

					values.add({ id, v});

					if(v == ComplexGroupManager::IgnoreFlag)
						msg << filename << ": " << id << " > IGNORE_FLAG (0xFF)";
					else
						msg << filename << ": " << id << " > " << layerTokens[v-1];

					msg << "\n";

					getComplexGroupManager()->setSampleId(s.get(), values, true);
				}
			}

			return msg;
		}

		const bool addMode;
		int height = LayerComponent::TopBarHeight;
		bool ok = false;

		Factory f;
		OwnedArray<FileToken> tokens;
		
		String delimiter = "_";
		StringArray layerTokens;
		Array<Rectangle<float>> delimiters;
		Rectangle<float> tokenLabel;
		bool ignoreable = false;

		SelectorLookAndFeel laf;

		Action currentAction = Action::DoNothing;

		bool legatoMode = false;

		hise::SubmenuComboBox modeSelector;

		bool canUseFileTokens = false;

		HiseShapeButton applyButton;

		StringArray currentFileTokens;

		std::function<void(uint8)> onTokenChange;

		JUCE_DECLARE_WEAK_REFERENCEABLE(FileTokenSelector);
	};

	struct LogicTypeComponent: public LayerComponent,
							   public ButtonListener
	{
		struct BodyBase: public Component,
						 public ComponentWithGroupManagerConnection,
						 public SampleMap::Listener
		{
			static constexpr int ButtonHeight = 24;

			struct ButtonBase: public Component,
							   public SettableTooltipClient
			{
				ButtonBase(const ValueTree& layerData, uint8 layerIndex_);

				virtual ~ButtonBase() {};
				virtual int getWidthToUse() const = 0;

				bool isDisplayed() const;
				
				Colour getButtonColour(bool on) const;
				void mouseDown(const MouseEvent& e) override;
				void setActive(bool shouldBeActive);

				bool active = false;
				bool empty = false;
				const uint8 layerIndex;
			};

			struct UnassignedButton: public ButtonBase
			{
				UnassignedButton(const ValueTree& layerData):
				  ButtonBase(layerData, 0),
				  f(GLOBAL_BOLD_FONT())
				{
					setMouseCursor(MouseCursor::PointingHandCursor);
					p = fa.createPath("unassigned");
					setSize(getWidthToUse(), ButtonHeight);
					setInterceptsMouseClicks(true, true);
					setRepaintsOnMouseActivity(true);
					setTooltip("There are unassigned samples which will not be played back when a note is pressed. Click to open the parser tool");
				}

				int getWidthToUse() const override { return ButtonHeight + f.getStringWidth("Unassigned") + 10; }

				void mouseDown(const MouseEvent& e) override
				{
					PresetHandler::showMessageWindow("Unassigned samples", "Any sample that is not assigned to a group (or has the ignore flag set for this layer) will be skipped when starting a voice at a note on.  \n> Press OK, then assign any unassigned sample to any of the groups in this layer. Note that you can use the green tag selectors to quickly select all samples of a layer.");

					if(auto c = findParentComponentOfClass<ComplexGroupManagerComponent>())
					{
						c->parseButton.setToggleState(true, sendNotificationSync);
						c->parseButton.setToggleStateAndUpdateIcon(true, true);
						c->tagButton.setToggleState(true, sendNotificationSync);
						c->tagButton.setToggleStateAndUpdateIcon(true, true);
					}
				}

				void paint(Graphics& g) override
				{
					float alpha = 0.5f;

					if(isMouseOverOrDragging())
						alpha += 0.1f;

					if(isMouseButtonDown())
						alpha += 0.3f;

					g.setColour(Colours::white.withAlpha(alpha));

					g.setFont(f);
					auto tb = getLocalBounds().toFloat();
					g.fillPath(p);
					tb.removeFromLeft(tb.getHeight());
					g.drawText("Unassigned", tb, Justification::centred);
				}

				void resized() override
				{
					fa.scalePath(p, getLocalBounds().removeFromLeft(getHeight()).toFloat().reduced(3.0f));
				}

				Factory fa;
				Path p;
				Font f;
			};

			struct IgnoreButton: public ButtonBase
			{
				IgnoreButton(const ValueTree& layerData):
				  ButtonBase(layerData, ComplexGroupManager::IgnoreFlag)
				{
					icon = f.createPath("ignorable");

					setSize(getWidthToUse(), ButtonHeight);
				}

				int getWidthToUse() const override { return ButtonHeight; }

				void resized() override
				{
					PathFactory::scalePath(icon, this, 3);
				}

				void paint(Graphics& g) override
				{
					g.setColour(Colours::white.withAlpha(0.5f));
					g.fillPath(icon);
				}

				Factory f;
				Path icon;
			};

			struct KeyswitchButton: public ButtonBase
			{
				KeyswitchButton(const ValueTree& layerData, uint8 layerIndex, const String& n, bool addPowerButton=true);

				int getWidthToUse() const override;

				void resized() override;
				void paint(Graphics& g) override;

				String name;
				Factory f;
				ScopedPointer<HiseShapeButton> purgeButton;
			};

			struct SelectionTag: public Component,
								 public ComponentListener,
								 public SettableTooltipClient
			{
				static constexpr int SelectionSize = 17;

				SelectionTag(ButtonBase* attachedButton, uint8 layerIndex_, uint8 layerValue_);

				void mouseDown(const MouseEvent& e) override;

				void mouseUp(const MouseEvent& e) override
				{
					active = false;
				}

				void refreshNumSamples();

				void refreshSize();

				~SelectionTag();

				void paint(Graphics& g) override;

				void componentMovedOrResized (Component& component,
				                              bool wasMoved,
				                              bool wasResized) override;

				Component::SafePointer<Component> target;

				int numSamples = 0;

				bool active = false;

				uint8 layerIndex;
				uint8 layerValue;
			};

			BodyBase(LogicTypeComponent& parent);

			virtual ~BodyBase() override
			{
				if(getSampler() != nullptr)
					getSampler()->getSampleMap()->removeListener(this);
			}

			void showSelectors(bool shouldShow);

			int getHeightToUse() const override { return 100; }

			/** Overwrite this method and return the center X position for the token. */
			virtual int getCentreXPositionForToken(uint8 tokenIndex) const;

			/** Overwrite this and react to play state changes. The default behaviour just sets the button active. */
			virtual void onPlaystateUpdate(uint8 value);

			void calculateXPositionsForItems(Array<int>& positions) const;

			void resized() override;

			void paint(Graphics& g) override;

			Array<ButtonBase*> getAllButtonsToShow()
			{
				Array<ButtonBase*> list;

				auto idx = Helpers::getLayerIndex(data);

				auto showUnassigned = getComplexGroupManager()->getNumUnassignedAndIgnored(idx).first > 0;

				unassignedButton->setVisible(showUnassigned);

				if(showUnassigned)
					list.add(unassignedButton.get());

				for(auto b: buttons)
					list.add(b);

				auto showIgnorable = (bool)data[groupIds::ignorable];

				ignoreButton->setVisible(showIgnorable);

				if(showIgnorable)
					list.add(ignoreButton.get());

				return list;
			}

			static void onLockUpdate(BodyBase& l, uint8 layerIndex, bool active)
			{
				if(Helpers::getLayerIndex(l.data) == layerIndex)
				{
					l.lockEnabled = active;
					l.refreshLock();
				}
			}

			void refreshLock()
			{
				if(lockEnabled == 0)
					currentLock = {};
				else
				{
					if(isPositiveAndBelow(currentLayerValue-1, buttons.size()))
						currentLock = buttons[currentLayerValue-1]->getBoundsInParent();
				}

				repaint();
			}

			struct Updater: public AsyncUpdater
			{
				Updater(BodyBase& parent_):
				  parent(parent_)
				{}

				void handleAsyncUpdate() override
				{
					parent.refreshButtons(sendNotificationSync);
				}

				BodyBase& parent;
			} updater;

			void refreshButtons(NotificationType n=sendNotificationAsync)
			{
				if(n != sendNotificationSync)
					updater.triggerAsyncUpdate();
				else
				{
					resized();

					auto shouldShow = !selectors.isEmpty();
					selectors.clear();
					showSelectors(shouldShow);
				}
			}

			void sampleMapWasChanged(PoolReference) override { refreshButtons(); }

			void samplePropertyWasChanged(ModulatorSamplerSound*, const Identifier& id, const var&) override
			{
				if(id == SampleIds::RRGroup) refreshButtons();
			};

			void sampleAmountChanged() override { refreshButtons(); }
			void sampleMapCleared() override { refreshButtons(); };

			uint8 currentLayerValue = 0;
			bool lockEnabled = false;
			Rectangle<int> currentLock;

			LogicType lt;
			StringArray tokens;

			ScopedPointer<ButtonBase> ignoreButton;
			ScopedPointer<ButtonBase> unassignedButton;

			OwnedArray<ButtonBase> buttons;
			ValueTree data;

			uint8 currentIndex = 0;

			Rectangle<int> allBounds;

			OwnedArray<SelectionTag> selectors;

			JUCE_DECLARE_WEAK_REFERENCEABLE(BodyBase);
		};

#if 0
		struct SampleCountComponent: public Component,
								     public ComponentWithGroupManagerConnection
		{
			static constexpr int LabelWidth = 80;

			struct CountButton: public Component
			{
				CountButton(uint8 layerIndex_, uint8 layerValue);

				int getMinWidth() const;
				void paint(Graphics& g) override;

				void mouseDown(const MouseEvent& e);

				int numSamples = 0;
				const uint8 layerIndex = 0;
				const uint8 layerValue = 0;

				
			};

			SampleCountComponent(LogicTypeComponent& parent);

			int getHeightToUse() const override;
			
			void applyXPositions(Array<int>& positionsAndCount);;
			bool showSecondRow() const;

			/** Called when the samples change. */
			void updateNumSamples();

			void paint(Graphics& g) override;

			OwnedArray<CountButton> buttons;
			ScopedPointer<CountButton> unassignedButton;
			ScopedPointer<CountButton> ignoredButton;
			ValueTree data;
		};
#endif
		
		struct Parser: public FileTokenSelector
		{
			Parser(ComponentWithGroupManagerConnection& parent, const ValueTree& v);

			
		};

		LogicTypeComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& d);


		void setBody(BodyBase* b);
		int getHeightToUse() const override;

		void refreshHeight();

		void buttonClicked(Button* b) override;
		void paint(Graphics& g) override;
		void resized() override;

		void updateSampleCounters();

		void showSelectors(bool shouldShow);

		static void onPlaystateUpdate(LogicTypeComponent& l, uint8 value);

		

		StringArray items;
		ScopedPointer<BodyBase> body;
		HiseShapeButton lockButton, midiButton;
		Parser parser;

		SelectorLookAndFeel laf;

		//Label unassignedLabel, ignoreLabel;
		OwnedArray<BodyBase::SelectionTag> selectors;



		JUCE_DECLARE_WEAK_REFERENCEABLE(LogicTypeComponent);
	};

	struct RRBody; struct KeyswitchBody; struct XFadeBody; struct LegatoBody;
	struct ChokeBody; struct TableFadeBody;

	struct AddComponent: public LayerComponent,
						 public ButtonListener
	{
		static constexpr int LabelHeight = 28;
		static constexpr int BottomBarHeight = TopBarHeight + 2 * BodyMargin;

		struct LogicTypeSelector;

		AddComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& t);
		~AddComponent();

		LogicTypeComponent* create(const ValueTree& l);
		void addNewLayer();

		void buttonClicked(Button* b) override;
		void resized() override;
		void paint(Graphics& g) override;

		void rebuildLayers(const ValueTree& v, const Identifier& id);

		void layerAddOrRemoved(const ValueTree& v, bool wasAdded);
		

		int getHeightToUse() const override;

		//HiseShapeButton addButton, moreButton;

	private:


		void addSpecialComponents(LogicType nt);

		Array<Component::SafePointer<Component>> getSpecialComponents() const;


		int specialHeight = 0;

		static Array<Identifier> getSpecialIds()
		{
			return { SampleIds::LoKey, SampleIds::HiKey, groupIds::isChromatic };
		}

		Component* addEditor(const Identifier& propertyId, const String& helpText, const String& type);;
		ComponentWithGroupManagerConnection* getGroupComponent(const Identifier& id) const;
		Component* getEditor(const Identifier& id) const;
		var getEditorValue(const Identifier& id) const;

		AlertWindowLookAndFeel alaf;
		TextButton okButton;
		FileTokenSelector tokenSelector;

		

		
		valuetree::ChildListener layerListener;
		valuetree::RecursivePropertyListener groupRebuildListener;

		OwnedArray<Component> editors;
		OwnedArray<MarkdownHelpButton> helpButtons;

		int flagHeight = TopBarHeight;

		JUCE_DECLARE_WEAK_REFERENCEABLE(AddComponent);
	};

	struct ParseToolbar: public LayerComponent,
						 public ButtonListener
	{
		static constexpr int ConsoleHeight = 190;

		ParseToolbar(ComponentWithGroupManagerConnection& parent, const ValueTree& d);

		void buttonClicked(Button* b) override;
		void paint(Graphics& g) override;
		void resized() override;

		int getHeightToUse() const override;

		void setLayersToParse(const Array<LogicTypeComponent*>& layers, int numSamples);
		void logToConsole(const String& message);

		String msg;
		Factory f;

		CodeDocument log;
		ScopedPointer<CodeTokeniser> tokeniser;
		CodeEditorComponent console;

		HiseShapeButton applyButton, resetButton;
		ScrollbarFader sf;
	};

	struct Content: public Component,
					public ComponentWithGroupManagerConnection
	{
		static constexpr int ItemMargin = 5;

		Content(ComponentWithGroupManagerConnection& parent, const ValueTree& d);

		void updateSize();
		void removeLayer(const ValueTree& d);
		void resized() override;

		int getHeightToUse() const override
		{
			return height;
		}

		OwnedArray<LogicTypeComponent> layerComponents;
		ParseToolbar parseToolbar;
		AddComponent addComponent;

		ValueTree data;

		int height = 0;

		bool recursion = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Content);
	};

	struct Logger: public valuetree::AnyListener
	{
		Logger(const ValueTree& d, CodeDocument& doc_);;
		
		void anythingChanged(CallbackType cb) override;

	private:

		CodeDocument& doc;
		ValueTree data;
	};

	ComplexGroupManagerComponent(ModulatorSampler* s);
	~ComplexGroupManagerComponent() override;

	void paint(Graphics& g) override;
	void resized() override;

	void updateInterface() override
	{

	}

	void soundsSelected(int numSelected) override;

	void sampleMapWasChanged(PoolReference newSampleMap) override { rebuildSampleCounters(); }

	void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue)  override;;

	void sampleAmountChanged() override { rebuildSampleCounters(); };

	void sampleMapCleared() override { rebuildSampleCounters(); };

	void setXmlLogger(CodeDocument& doc);

	void setDisplayFilter(uint8 layerIndex, uint8 value);

	void onLayerAddRemove(const ValueTree& v, bool added);

private:

	Factory f;

	Array<std::pair<Identifier, uint8>> displayFilters;

	Array<std::pair<uint8, uint8>> activeCountButtons;

	void rebuildSampleCounters();
	void addSelectionFilter(const std::pair<uint8, uint8>& filter, ModifierKeys mods);
	bool isFilterSelected(const std::pair<uint8, uint8>& f) const;
	void rebuildSelection();

	ValueTree layerRoot;
	valuetree::ChildListener layerListener;

	ScrollbarFader sf;
	Viewport viewport;
	Content content;
	ScopedPointer<Logger> logger;

	HiseShapeButton editButton, moreButton, tagButton, parseButton;
	MarkdownHelpButton helpStateButton;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ComplexGroupManagerComponent);
};

struct ComplexGroupManagerFloatingTile: public SamplerBasePanel
{;
	ComplexGroupManagerFloatingTile(FloatingTile* parent):
	  SamplerBasePanel(parent)
	{};

	SET_PANEL_NAME("ComplexGroupEditor");

	struct ComplexGroupActivator: public Component
	{
		ComplexGroupActivator(ModulatorSampler* s);

		void paint(Graphics& g) override;

		void resized() override;

		ComplexGroupManagerComponent::Factory f;
		HiseShapeButton b;
		WeakReference<ModulatorSampler> sampler;
	};

	Component* createContentComponent(int /*index*/) override;;
};

}
