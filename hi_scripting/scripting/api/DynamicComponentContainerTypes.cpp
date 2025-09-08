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



namespace hise {
namespace dyncomp
{


template <typename ComponentType> struct WrapperBase: public Base
{
	WrapperBase(Data::Ptr d, const ValueTree& v):
	  Base(d, v),
	  component(v[dcid::id.toString()])
	{
		addAndMakeVisible(component);

		

		if constexpr(std::is_base_of<SettableTooltipClient, ComponentType>())
		{
			auto tooltip = this->dataTree[dcid::tooltip].toString();
			component.setTooltip(tooltip);
		}
	};

	virtual ~WrapperBase() {};

	void resized() override
	{
		component.setBounds(getLocalBounds());
	}

	void initSpecialProperties(const Array<Identifier>& ids)
	{
		specialProperties.setCallback(this->dataTree, ids, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(WrapperBase::updateSpecialProperties));
	}

	bool forwardToFirstChild() const override { return true; }

protected:

	virtual void updateSpecialProperties(const Identifier& id, const var& newValue) = 0;

	bool useUndoManager() const
	{
		return (bool)dataTree[dcid::useUndoManager];
	}

	ComponentType component;

	valuetree::PropertyListener specialProperties;
};

struct Button: public WrapperBase<hise::MomentaryToggleButton>
{
	SN_NODE_ID("Button");

	Button(Data::Ptr d, const ValueTree& v):
	  WrapperBase<hise::MomentaryToggleButton>(d, v)
	{
		this->component.onClick = [&]()
		{
			data->onValueChange(getId(), this->component.getToggleState(), useUndoManager());

			auto rid = this->component.getRadioGroupId();

			if(rid != 0)
			{
				if(auto r = findParentComponentOfClass<Root>())
				{
					Component::callRecursive<Button>(r, [&](Button* b)
					{
						if(this == b)
							return false;

						if(b->component.getRadioGroupId() == rid)
							b->data->onValueChange(b->getId(), false, b->useUndoManager());

						return false;
					});
				}
			}
		};

		component.setIsMomentary((bool)this->dataTree[dcid::isMomentary]);
		component.setClickingTogglesState(!(bool)this->dataTree[dcid::isMomentary]);
		component.setTriggeredOnMouseDown((bool)this->dataTree[dcid::setValueOnClick]);
		
		initSpecialProperties({ dcid::text, dcid::radioGroupId });
		initCSSForChildComponent();
	};

	void onValue(const var& newValue) override
	{
		

		this->component.setToggleState((bool)newValue, dontSendNotification);
	}

	void updateSpecialProperties(const Identifier& id, const var& newValue) override
	{
		if(id == dcid::text)
		{
			this->component.setButtonText(newValue.toString());
		}
		if(id == dcid::radioGroupId)
		{
			this->component.setRadioGroupId((int)newValue, dontSendNotification);
		}
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new Button(d, v); }
};

struct ComboBox: public WrapperBase<hise::SubmenuComboBox>
{
	SN_NODE_ID("ComboBox");

	ComboBox(Data::Ptr d, const ValueTree& v):
	  WrapperBase<juce::SubmenuComboBox>(d, v)
	{
		component.setUseCustomPopup((bool)this->dataTree[dcid::useCustomPopup]);

		this->component.onChange = [&]()
		{
			data->onValueChange(getId(), this->component.getSelectedId(), useUndoManager());
		};

		initSpecialProperties({ dcid::items});

		// Call it once synchronously so that the init value works
		updateSpecialProperties(dcid::items, this->dataTree[dcid::items]);
		initCSSForChildComponent();
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new ComboBox(d, v); }

	void onValue(const var& newValue) override
	{
		this->component.setSelectedId((int)newValue, dontSendNotification);
	}

	void updateSpecialProperties(const Identifier& id, const var& newValue) override
	{
		if(id == dcid::items)
		{
			auto currentId = this->component.getSelectedId();
			auto items = StringArray::fromLines(newValue.toString());
			items.removeEmptyStrings();

			this->component.clear(dontSendNotification);
			this->component.addItemList(items, 1);
			this->component.setSelectedId(currentId, dontSendNotification);
			this->component.rebuildPopupMenu();
		}
	}
};



struct Slider: public Base,
			   public ControlledObject,
			   public juce::Slider::Listener	
{
	SN_NODE_ID("Slider");

	struct UnconnectedSlider: public hise::SliderWithShiftTextBox,
							  juce::Slider
	{
		UnconnectedSlider(const String& name):
		  juce::Slider(name)
		{}

		void mouseDoubleClick(const MouseEvent& e) override
		{
			performModifierAction(e, true);
		}

		void mouseDown(const MouseEvent& e) override
		{
			if(performModifierAction(e, false))
				return;

			Slider::mouseDown(e);
		}
	};

	Slider(Data::Ptr d, const ValueTree& v):
	  Base(d, v),
	  ControlledObject(d->getMainController())
	{
		connectionListener.setCallback(v, getSliderIds(), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Slider::updateSliderProperty));
		updateSliderProperty(dcid::processorId, dataTree[dcid::processorId]);
	}

	void sliderValueChanged(juce::Slider* slider) override
	{
		this->data->onValueChange (getId(), this->slider->getValue(), false);
	}

	static Array<Identifier> getSliderIds()
	{
		static const Array<Identifier> sliderIds({
		 dcid::min, 
		 dcid::max, 
		 dcid::middlePosition, 
		 dcid::stepSize, 
		 dcid::mode, 
		 dcid::suffix, 
		 dcid::style, 
		 dcid::showValuePopup,
         dcid::processorId,
		 dcid::parameterId
		});

		return sliderIds;
	}

	void lookAndFeelChanged() override
	{
		if(slider != nullptr)
			slider->setLookAndFeel(&getLookAndFeel());
	}

	std::pair<Processor*, int> getConnectedParameter()
	{
		auto pid = dataTree[dcid::processorId].toString();

		auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), pid);

		if(p != nullptr)
		{
			auto parameter = dataTree[dcid::parameterId];
			int pIndex = -1;

			if(parameter.isString())
				pIndex = p->getParameterIndexForIdentifier(Identifier(parameter));
			else
				pIndex = (int)parameter;

			if(pIndex != -1)
				return { p, pIndex };
		}

		return { nullptr, -1 };
	}

	void setSlider(juce::Slider* s)
	{
		simple_css::FlexboxComponent::Helpers::writeClassSelectors(*s, { simple_css::Selector(".scriptslider") }, true);
		s->setDoubleClickReturnValue(true, (double)dataTree[dcid::defaultValue]);
		s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

		s->addListener(this);

		GlobalHiseLookAndFeel::setDefaultColours(*s);

		slider = s;
		addAndMakeVisible(s);

		for(auto id: getSliderIds())
		{
			if(id == dcid::parameterId || id == dcid::processorId)
				continue;

			updateSliderProperty(id, getPropertyOrDefault(id));
		}

		initCSSForChildComponent();
		resized();
	}

	void onValue(const var& newValue) override
	{
		if(slider != nullptr)
		{
			auto isConnected = dynamic_cast<HiSlider*>(slider.get()) != nullptr;
			slider->setValue((double)newValue, isConnected ? sendNotificationSync : dontSendNotification);
		}
	}

	static bool isFilmStripId(const Identifier& id)
	{
		return id == dcid::filmstripImage || id == dcid::numStrips || id == dcid::isVertical || id == dcid::scaleFactor;
	}

	FilmstripLookAndFeel* createFilmstripLookAndFeel() const
	{
		auto ref = dataTree[dcid::filmstripImage].toString();

		if(ref.isNotEmpty())
		{
			Image img = data->getImage(ref);

			auto isVertical = (bool)dataTree.getProperty(dcid::isVertical, true);
			auto scaleFactor = (double)dataTree.getProperty(dcid::scaleFactor, 1.0);
			auto numStrips = (int)dataTree.getProperty(dcid::numStrips, 0);

			if(numStrips > 0 && img.isValid())
			{
				auto fs = new FilmstripLookAndFeel();
				fs->setFilmstripImage(img, numStrips, isVertical);
				fs->setScaleFactor(scaleFactor);

				return fs;
			}
		}

		return nullptr;
	}


	void updateSliderProperty(const Identifier& id, const var& newValue)
	{
		if(id == dcid::parameterId || id == dcid::processorId)
		{
			auto connection = getConnectedParameter();
			auto name = dataTree[dcid::text].toString();

			if(name.isEmpty())
				name = dataTree[dcid::id].toString();

			if(connection.first != nullptr)
			{
				auto s = new HiSlider(dataTree[dcid::id].toString());
				s->setup(connection.first, connection.second, name);
				setSlider(s);
				auto laf = &getLookAndFeel();
				s->setLookAndFeel(laf);
			}
			else
			{
				auto s = new UnconnectedSlider(name);
				setSlider(s);
			}
		}
		if(id == dcid::suffix)
		{
			this->slider->setTextValueSuffix(newValue.toString());
		}
		else if (id == dcid::style)
		{
			StringArray values({"Knob", "Horizontal", "Vertical"});

			std::array<juce::Slider::SliderStyle, 3> styles = { juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
			  juce::Slider::SliderStyle::LinearBar,
			  juce::Slider::SliderStyle::LinearHorizontal
			};

			auto idx = values.indexOf(newValue.toString());

			if(idx != -1)
				this->slider->setSliderStyle(styles[idx]);
		}
		else if (isFilmStripId(id))
		{
			if(auto laf = createFilmstripLookAndFeel())
			{
				fslaf = laf;
				slider->setLookAndFeel(laf);
			}
		}
		else if(id == dcid::mode)
		{
			auto vtc = hise::ValueToTextConverter::createForMode(newValue.toString());

			if(vtc.active)
			{
				this->slider->textFromValueFunction = vtc;
				this->slider->valueFromTextFunction = vtc;
			}
			else
			{
				this->slider->textFromValueFunction = {};
				this->slider->valueFromTextFunction = {};
			}
		}
		else if (id == dcid::showValuePopup)
		{
			
		}
		else
		{
			auto rng = scriptnode::RangeHelpers::getDoubleRange(this->dataTree, RangeHelpers::IdSet::ScriptComponents);
			this->slider->setRange(rng.rng.getRange(), rng.rng.interval);
			this->slider->setSkewFactor(rng.rng.skew);
		}
	}

	bool forwardToFirstChild() const override { return true; }

	void resized() override
	{
		if(slider != nullptr)
			slider->setBounds(getLocalBounds());
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new Slider(d, v); }

private:

	valuetree::PropertyListener connectionListener;
	ScopedPointer<juce::Slider> slider;
	ScopedPointer<FilmstripLookAndFeel> fslaf;
};


struct Label: public WrapperBase<hise::MultilineLabel>,
			  public Timer,
	          public juce::Label::Listener,	
			  public juce::TextEditor::Listener	
{
	SN_NODE_ID("Label");

	Label(Data::Ptr d, const ValueTree& v):
	  WrapperBase<MultilineLabel>(d, v)
	{
		component.addListener(this);

		if((bool)dataTree[dcid::updateEachKey])
		{
			component.onTextChange = [this]()
			{
				this->startTimer(500);
			};
		}

		initSpecialProperties({ dcid::text, dcid::multiline, dcid::editable});

		initCSSForChildComponent();
	}

	void labelTextChanged (juce::Label* labelThatHasChanged) override
	{
		if(!component.isBeingEdited())
			this->data->onValueChange(getId(), component.getText(), useUndoManager());
	}

	void editorShown(juce::Label*, TextEditor& te) override
	{
		if(auto r = simple_css::CSSRootComponent::find(*this))
		{
			if(auto ss = r->css.getForComponent(&te))
			{
				ss->setupComponent(r, &te, 0);
			}
		}

		te.addListener(this);
	}

	void editorHidden(juce::Label*, TextEditor& te) override
	{
		te.removeListener(this);
	}

	String currentText;

	void textEditorTextChanged(TextEditor& te) override
	{
		if((bool)dataTree[dcid::updateEachKey])
		{
			currentText = te.getText();
			startTimer(500);
		}
	}

	void timerCallback() override
	{
		this->data->onValueChange(getId(), currentText, useUndoManager());
		stopTimer();
	}

	void onValue(const var& newValue) override
	{
		if(!component.isBeingEdited())
			component.setText(newValue.toString(), dontSendNotification);
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new Label(d, v); }

	void updateSpecialProperties(const Identifier& id, const var& newValue) override
	{
		if(id == dcid::text)
		{
			component.setText(newValue.toString(), dontSendNotification);
		}
		if(id == dcid::editable)
		{
			component.setEditable((bool)newValue);
		}
		if(id == dcid::multiline)
		{
			component.setMultiline((bool)newValue);
		}
	}
};	

struct FloatingTile: public Base
{
	SN_NODE_ID("FloatingTile");

	FloatingTile(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		onValue(d->getFloatingTileData(v[dcid::id].toString()));
	}

	void lookAndFeelChanged() override
	{
		if(ft != nullptr)
		{
			auto laf = &getLookAndFeel();

			Component::callRecursive<Component>(ft, [laf](Component* c)
			{
				c->setLookAndFeel(laf);
				return false;
			});
		}
	}

	bool forwardToFirstChild() const override { return true; }

	void onValue(const var& newValue) override
	{
		if(newValue.getDynamicObject() != nullptr)
		{
			addAndMakeVisible(ft = new hise::FloatingTile(data->getMainController(), nullptr, newValue));
			simple_css::FlexboxComponent::Helpers::setIsOpaqueWrapper(*this, true);

			auto idSelector = String("#") + getId().toString();
			ft->getProperties().set(dcid::id, idSelector);
			ft->setLookAndFeel(&getLookAndFeel());
			resized();
		}
		else
		{
			ft = nullptr;
		}
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v)
	{
		return new FloatingTile(d, v);
	}

	void resized() override
	{
		if(ft != nullptr)
			ft->setBounds(getLocalBounds());
	}

	ScopedPointer<hise::FloatingTile> ft;
};

struct TextBox: public WrapperBase<SimpleMarkdownDisplay>
{
	SN_NODE_ID("TextBox");

	TextBox(Data::Ptr d, const ValueTree& v):
	  WrapperBase<hise::SimpleMarkdownDisplay>(d, v)
	{
		simple_css::FlexboxComponent::Helpers::setCustomType(component, simple_css::Selector(simple_css::ElementType::Paragraph));
		simple_css::FlexboxComponent::Helpers::setFallbackStyleSheet(component, "width: 100%; height: auto;");
		initSpecialProperties({dcid::text});
		component.setResizeToFit(true);
		simple_css::FlexboxComponent::Helpers::storeDefaultBounds(*this, {});

		initCSSForChildComponent();
	}

	void updateSpecialProperties(const Identifier& id, const var& newValue) override
	{
		if(id == dcid::text)
		{
			if(getLocalBounds().isEmpty())
			{
				waitForNotEmpty = true;
			}
			else
			{
				waitForNotEmpty = false;

				component.setText(newValue.toString());

				if(component.totalHeight > 0.0)
					dataTree.setProperty(dcid::height, component.totalHeight, nullptr);

				
			}
			
		}
	}

	void onValue(const var& newValue) override
	{
		
	}

	void resized() override
	{
		if(waitForNotEmpty && !getLocalBounds().isEmpty())
		{
			if(auto r = simple_css::CSSRootComponent::find(*this))
			{
				auto sd = r->css.getMarkdownStyleData(&component);

				ss = r->css.getForComponent(&component);
				component.r.setStyleData(sd);
			}

			component.setText(dataTree[dcid::text].toString());

			if(component.totalHeight > 0.0)
			{
				waitForNotEmpty = true;
				dataTree.setProperty(dcid::height, component.totalHeight, nullptr);
				updatePosition({},{});
			}
		}

		if(ss != nullptr)
		{
			auto area = getLocalBounds().toFloat();

			auto ma = ss->getArea(area, { "margin", 0});
			auto pa = ss->getArea(ma, { "padding", 0});

			

			this->component.setBounds(pa.toNearestInt());

			auto h = roundToInt(component.totalHeight + (area.getHeight() - pa.getHeight()));
			dataTree.setProperty(dcid::height, h, nullptr);
		}
		else
		{
			WrapperBase::resized();
		}
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new TextBox(d, v); }

	simple_css::StyleSheet::Ptr ss;

	bool waitForNotEmpty = false;
};



template <typename T> struct ComplexDataEditor: public Base
{
	using DataType = typename T::ComplexDataType;
	static constexpr ExternalData::DataType dt = ExternalData::getDataTypeForClass<DataType>();

	ComplexDataEditor(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		addAndMakeVisible(editor);

		std::map<ExternalData::DataType, String> typeClasses;
		typeClasses[ExternalData::DataType::Table] = ".scripttable";
		typeClasses[ExternalData::DataType::SliderPack] = ".scriptsliderpack";
		typeClasses[ExternalData::DataType::AudioFile] = ".scriptaudiowaveform";

		StringArray selectors;
		selectors.add("#" + v[dcid::id].toString());
		selectors.add(typeClasses[dt]);

		simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(editor, selectors);

		connectionListener.setCallback(v,
			{ dcid::processorId, dcid::index},
			valuetree::AsyncMode::Asynchronously,
			VT_BIND_PROPERTY_LISTENER(onConnectionChange));
	}

	static Identifier getStaticId()
	{
		auto dt = ExternalData::getDataTypeForClass<DataType>();
		return Identifier(ExternalData::getDataTypeName(dt, false));
	};

	void onConnectionChange(const Identifier&, const var&)
	{
		auto cd = dcid::Helpers::getComplexDataBase(data->getMainController(), getDataTree(), dt);
		editor.setComplexDataUIBase(cd);
	}

	void lookAndFeelChanged() override
	{
		auto laf = &getLookAndFeel();
		editor.setSpecialLookAndFeel(laf, false);
	}

	void onValue(const var& newValue) override
	{}

	void resized() override
	{
		editor.setBounds(getLocalBounds());
	}

	T editor;
	valuetree::PropertyListener connectionListener;

	bool forwardToFirstChild() const override { return true; }

	static Base* createComponent(Data::Ptr d, const ValueTree& v)
	{
		return new ComplexDataEditor(d, v);
	}
};

struct DragContainer: public Base
{
	SN_NODE_ID("DragContainer");

	DragContainer(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		auto idSelector = String("#") + getId().toString();
		getProperties().set(dcid::id, idSelector);

		isVertical = getPropertyOrDefault(dcid::isVertical);
		dragMargin = v.getProperty(dcid::dragMargin, 3);
		animationSpeed = v.getProperty(dcid::animationSpeed, 150);
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new DragContainer(d, v); }

	bool forwardToFirstChild() const override { return false; }

	void rearrangeChildren(const std::vector<int>& sortedIndexes, bool force)
	{
		bool same = sortedIndexes.size() == currentIndexes.size();

		if(same)
		{
			for(int i = 0; i < currentIndexes.size(); i++)
				same &= currentIndexes[i] == sortedIndexes[i];
		}

		if(!same || force)
		{
			currentIndexes = sortedIndexes;
			rebuildPositions(force);
		}
	}

	void rebuildPositions(bool writePositionsInValueTree)
	{
		auto b = getLocalBounds().reduced(dragMargin);

		for(auto& idx: currentIndexes)
		{
			Rectangle<int> tb;

			if(auto d = draggers[idx])
			{
				Component* c = d->child.get();

				if(isVertical)
					tb = b.removeFromTop(c->getHeight());
				else
					tb = b.removeFromLeft(c->getWidth());

				if(c == currentlyDraggedComponent)
					continue;

				if(writePositionsInValueTree)
					dynamic_cast<Base*>(c)->writePositionInValueTree(tb, false);

				if(animationSpeed == 0)
					c->setBounds (tb);
				else
                    Desktop::getInstance().getAnimator().animateComponent(c, tb, 1.0, 200, false, 1.0, 0.3);
			}
		}
	}

	void resized() override
	{
		rebuildPositions(false);
	}

	int fxPositionToChildComponentOrder(int indexFromValueTree) const
	{
		indexFromValueTree -= (int)dataTree[dcid::min];

		for(int i = 0; i < draggers.size(); i++)
		{
			auto vi = (int)draggers[i]->child->getPropertyOrDefault(dcid::index);

			if(vi == indexFromValueTree)
				return i;
		}

		return -1;
	}

	void onValue(const var& newValue)
	{
		if(auto a = newValue.getArray())
		{
			std::vector<int> copy;

			for(auto v: *a)
				copy.push_back(fxPositionToChildComponentOrder(v));

			if(copy.size() == getNumChildComponents())
			{
				ScopedValueSetter<double> svs(animationSpeed, 0.0);
				rearrangeChildren(copy, true);
			}
		}
	}

	Component* currentlyDraggedComponent = nullptr;

	void snap()
	{
		rebuildIndexArrayFromPosition(false);
	}

	struct Dragger: public MouseListener,
				    public ComponentBoundsConstrainer
	{
		Dragger(DragContainer& parent_, Component* c):
		  child(dynamic_cast<Base*>(c)),
		  parent(parent_) 
		{
			child->addMouseListener(this, false);
			child->getChildComponent(0)->addMouseListener(this, false);
		};

		~Dragger()
		{
			if(child != nullptr)
			{
				child->removeMouseListener(this);

				if(auto c = child->getChildComponent(0))
					c->removeMouseListener(this);
			}
		}

		void mouseDown(const MouseEvent& e) override
		{
			parent.currentlyDraggedComponent = child.get();
			child->toFront(false);
			d.startDraggingComponent(child, e);
			child->repaint();
		}

		void mouseDrag(const MouseEvent& e) override
		{
			d.dragComponent(child, e, this);
			parent.snap();
		}

		void mouseUp(const MouseEvent& e) override
		{
			parent.currentlyDraggedComponent = nullptr;
			parent.rebuildIndexArrayFromPosition(true);

			auto v = dcid::Helpers::getDragContainerIndexValues(parent.currentIndexes, parent.dataTree);
			parent.data->onValueChange(parent.getId(), v, (bool)parent.dataTree[dcid::useUndoManager]);

			child->repaint();
		}

		void checkBounds (Rectangle<int>& bounds, const Rectangle<int>& previousBounds, const Rectangle<int>& limits, bool ,
                               bool, bool, bool) override
		{
			auto x = parent.isVertical ? parent.dragMargin : 0;
			auto y = parent.isVertical ? 0 : parent.dragMargin;

			auto parentArea = parent.getLocalBounds().reduced(x, y);
			bounds = bounds.constrainedWithin(parentArea);
		}

		DragContainer& parent;
		Base::WeakPtr child;
		ComponentDragger d;
	};

	void paint(Graphics& g) override
	{
		if(auto slaf = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(&getLookAndFeel()))
		{
			slaf->drawComponentBackground(g, this);

			slaf->drawGenericComponentText(g, "", draggers.isEmpty(), this);
		}
			
	}

	void updateChild(const ValueTree& v, bool wasAdded) override
	{
		if(wasAdded)
		{
			Base::updateChild(v, wasAdded);
			auto c = getChildComponent(getNumChildComponents()-1);
			auto nd = new Dragger(*this, c);
			draggers.add(nd);
		}
		else
		{
			for(auto d: draggers)
			{
				if(d->child == nullptr || *d->child == v)
					draggers.removeObject(d);
			}

			Base::updateChild(v, wasAdded);
		}

		ScopedValueSetter<double> svs(animationSpeed, 0.0);
		rebuildIndexArrayFromPosition(true);
		repaint();
	}

	void rebuildIndexArrayFromPosition(bool force)
	{
		Array<Base::WeakPtr> list;

		for(auto d: draggers)
			list.add(d->child);
		
		struct Sorter
		{
			Sorter(bool isVertical_, int maxValue_):
			  isVertical(isVertical_),
			  maxValue(maxValue_) 
			{};

			int compareElements(Base::WeakPtr c1, Base::WeakPtr c2) const
			{
				int pos1, pos2, b1, b2, l1, l2;

				if(isVertical)
				{
					pos1 = c1->getY();
					pos2 = c2->getY();
					b1 = c1->getBottom();
					b2 = c2->getBottom();
					l1 = c1->getHeight();
					l2 = c2->getHeight();
				}
				else
				{
					pos1 = c1->getX();
					pos2 = c2->getX();
					b1 = c1->getRight();
					b2 = c2->getRight();
					l1 = c1->getWidth();
					l2 = c2->getWidth();
				}

				auto h1 = pos1 + l1/2;
				auto h2 = pos2 + l2/2;

				if(l1 == l2)
				{
				    if(pos1 < pos2)
				        return -1;
			        if(pos1 > pos2)
				        return 1;
				}
				if(l1 < l2)
				{
				    if(h1 < h2)
						return -1;
                    if(h1 > h2)
						return 1;
				}
				if(l1 > l2)
				{
					if(pos1 == 0 && pos2 != 0)
						return -1;
					if(pos2 == 0 && pos1 != 0)
						return 1;

					if(b1 == maxValue && b2 != maxValue)
						return 1;
					if(b1 == maxValue && b2 != maxValue)
						return -1;

				    if(h1 < h2)
				        return -1;
			        if(h1 > h2)
				        return 1;
				}

				return 0;
			}

			const bool isVertical;
			const int maxValue;
		};

		Sorter s(isVertical, isVertical ? getHeight() : getWidth());

		Array<Base::WeakPtr> sorted;
		sorted.addArray(list);
		sorted.sort(s);

		std::vector<int> newIndexes;

		DBG("NEW:");

		for(auto& s: sorted)
		{
			auto idx = list.indexOf(s);
			DBG(idx);
			newIndexes.push_back(idx);
		}
		
		rearrangeChildren(newIndexes, force);
	}

	bool isVertical = true;
	double animationSpeed = 150.0;
	int dragMargin = 3;
	std::vector<int> currentIndexes;

	OwnedArray<Dragger> draggers;
};

struct Panel: public Base
{
	SN_NODE_ID("Panel");

	Panel(Data::Ptr d, const ValueTree& v):
	  Base(d, v),
	  bp(getDrawHandler())
	{
		addAndMakeVisible(bp);

		bp.isUsingCustomImage = getDrawHandler() != &fallback;

		setInterceptsMouseClicks(true, true);

		auto idSelector = String("#") + getId().toString();
		simple_css::FlexboxComponent::Helpers::setCustomType(bp, simple_css::Selector(simple_css::ElementType::Panel));
		bp.getProperties().set(dcid::id, idSelector);
		initCSSForChildComponent();
	}

	DrawActions::Handler* getDrawHandler()
	{
		auto dh = data->getDrawHandler(dataTree);
		return dh != nullptr ? dh : &fallback;
	}

	bool forwardToFirstChild() const override { return true; }

	void onValue(const var& newValue) override
	{
		
	}

	void paintOverChildren(Graphics& g) override
	{
		auto text = dataTree[dcid::text].toString();

		if(text.isNotEmpty())
		{
			if(auto slaf = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(&getLookAndFeel()))
				slaf->drawGenericComponentText(g, text, text.isEmpty(), this);
		}
	}

	void resized() override
	{
		bp.setBounds(getLocalBounds());
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v)
	{
		return new Panel(d, v);
	}

	DrawActions::Handler fallback;
	BorderPanel bp;
};

struct FlexBox: public Base,
				public AsyncUpdater
{
	SN_NODE_ID("Viewport");

	FlexBox(Data::Ptr d, const ValueTree& v):
	  Base(d, v),
	  content(simple_css::Selector("#" + v[dcid::id].toString()))
	{
		simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(content, {".scriptviewport"});
		addAndMakeVisible(content);
	}

	bool forwardToFirstChild() const override { return true; }

	void onValue(const var& newValue) override {}

	void handleAsyncUpdate() override
	{
		content.rebuildLayout();
	}

	void baseChildChanged(Base::Ptr c, bool wasAdded) override
	{
		if(wasAdded)
			content.addFlexItem(*c);
		else
			content.removeFlexItem(*c);
		
		triggerAsyncUpdate();
	}

	void hideChild(Base::Ptr b, bool shouldBeVisible)
	{
		content.content.setFlexChildVisibility(b.get(), shouldBeVisible, !shouldBeVisible);
		triggerAsyncUpdate();
	}

	void resizeChild(Base::Ptr b, Rectangle<int> newBounds) override
	{
		simple_css::FlexboxComponent::Helpers::storeDefaultBounds(*b, newBounds);
		triggerAsyncUpdate();
	}

	void resized() override
	{
		if(!initialised)
		{
			if(auto root = simple_css::CSSRootComponent::find(*this))
			{
				content.setCSS(root->css);
				initialised = true;
			}
		}

		if(content.getLocalBounds() != getLocalBounds())
			content.setBounds(getLocalBounds());
		else
			content.resized();
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v)
	{
		return new FlexBox(d, v);
	}

	bool initialised = false;

	simple_css::FlexboxViewport content;
};


Root::Root(Data::Ptr d):
  Base(d, d->getValueTree(Data::TreeType::Data))
{
	simple_css::FlexboxComponent::Helpers::setCustomType(*this, simple_css::Selector(simple_css::ElementType::Body));
	simple_css::FlexboxComponent::Helpers::setFallbackStyleSheet(*this, "background-color:transparent;");
}

void Root::paint(Graphics& g)
{
	using namespace simple_css;

	if(css != nullptr)
	{
		auto root = CSSRootComponent::find(*this);
		Renderer r(this, root->stateWatcher);

		auto state = r.getPseudoClassFromComponent(this);
		root->stateWatcher.checkChanges(this, css, state);
		r.drawBackground(g, getLocalBounds().toFloat(), css);
	}
}


Factory::Factory()
{
	registerItem<dyncomp::Button>();
	registerItem<dyncomp::Slider>();
	registerItem<dyncomp::ComboBox>();
	registerItem<dyncomp::Panel>();
	registerItem<dyncomp::Label>();
	registerItem<dyncomp::FloatingTile>();
	registerItem<dyncomp::DragContainer>();
	registerItem<dyncomp::FlexBox>();
	registerItem<dyncomp::TextBox>();
	registerItem<dyncomp::ComplexDataEditor<TableEditor>>();
	registerItem<dyncomp::ComplexDataEditor<SliderPack>>();
	registerItem<dyncomp::ComplexDataEditor<MultiChannelAudioBufferDisplay>>();
}

} // namespace dyncomp
} // namespace hise
