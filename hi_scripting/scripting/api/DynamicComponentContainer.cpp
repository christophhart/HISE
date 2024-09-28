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

#include "DynamicComponentContainer.h"

namespace hise {
namespace dyncomp
{

struct Factory
{
	Factory();

	Base::Ptr create(Data::Ptr d, const ValueTree& v) const
	{
		Identifier id(v[dcid::type].toString());

		for(const auto& i: items)
		{
			if(i.first == id)
				return i.second(d, v);
		}

		jassertfalse;
		return nullptr;
	}

private:

	using CreateFunction = std::function<Base*(Data::Ptr, const ValueTree& v)>;

	Array<std::pair<Identifier, CreateFunction>> items;

	template <typename T> void registerItem()
	{
		items.add({T::getStaticId(), T::createComponent});
	}
};

template <typename ComponentType> struct WrapperBase: public Base
{
	WrapperBase(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		addAndMakeVisible(component);

		auto idSelector = String("#") + getId().toString();
		component.getProperties().set(dcid::id, idSelector);

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

protected:

	virtual void updateSpecialProperties(const Identifier& id, const var& newValue) = 0;

	bool useUndoManager() const
	{
		return (bool)dataTree[dcid::useUndoManager];
	}

	ComponentType component;

	valuetree::PropertyListener specialProperties;
};

struct Button: public WrapperBase<juce::ToggleButton>
{
	SN_NODE_ID("Button");

	Button(Data::Ptr d, const ValueTree& v):
	  WrapperBase<juce::ToggleButton>(d, v)
	{
		this->component.onClick = [&]()
		{
			data->onValueChange(getId(), this->component.getToggleState(), useUndoManager());
		};

		component.setClickingTogglesState(!(bool)this->dataTree[dcid::isMomentary]);
		component.setRadioGroupId((int)this->dataTree[dcid::radioGroupId]);
		component.setTriggeredOnMouseDown((bool)this->dataTree[dcid::setValueOnClick]);
		
		initSpecialProperties({ dcid::text });
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

struct DynSlider: public juce::Slider,
				  public hise::SliderWithShiftTextBox
{
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

struct Slider: public WrapperBase<DynSlider>
{
	SN_NODE_ID("Slider");

	Slider(Data::Ptr d, const ValueTree& v):
	  WrapperBase<DynSlider>(d, v)
	{
		simple_css::FlexboxComponent::Helpers::writeClassSelectors(this->component, { simple_css::Selector(".scriptslider") }, true);
		this->component.setDoubleClickReturnValue(true, (double)this->dataTree[dcid::defaultValue]);
		this->component.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

		this->component.onValueChange = [this]()
		{
			this->data->onValueChange (getId(), this->component.getValue(), this->useUndoManager());
		};

		initSpecialProperties({
		 dcid::min, 
		 dcid::max, 
		 dcid::middlePosition, 
		 dcid::stepSize, 
		 dcid::mode, 
		 dcid::suffix, 
		 dcid::style, 
		 dcid::showValuePopup
		});

		if(auto laf = createFilmstripLookAndFeel())
		{
			fslaf = laf;
			component.setLookAndFeel(laf);
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

	void updateSpecialProperties(const Identifier& id, const var& newValue) override
	{
		if(id == dcid::suffix)
		{
			this->component.setTextValueSuffix(newValue.toString());
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
				this->component.setSliderStyle(styles[idx]);
		}
		else if (isFilmStripId(id))
		{
			
		}
		else if(id == dcid::mode)
		{
			auto vtc = hise::ValueToTextConverter::createForMode(newValue.toString());

			if(vtc.active)
			{
				this->component.textFromValueFunction = vtc;
				this->component.valueFromTextFunction = vtc;
			}
			else
			{
				this->component.textFromValueFunction = {};
				this->component.valueFromTextFunction = {};
			}
		}
		else if (id == dcid::showValuePopup)
		{
			
		}
		else
		{
			auto rng = scriptnode::RangeHelpers::getDoubleRange(this->dataTree, RangeHelpers::IdSet::ScriptComponents);
			this->component.setRange(rng.rng.getRange(), rng.rng.interval);
			this->component.setSkewFactor(rng.rng.skew);
		}
	}

	void onValue(const var& newValue) override
	{
		component.setValue((double)newValue, dontSendNotification);
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new Slider(d, v); }

	ScopedPointer<FilmstripLookAndFeel> fslaf;
};	

struct Label: public WrapperBase<hise::MultilineLabel>
{
	SN_NODE_ID("Label");

	Label(Data::Ptr d, const ValueTree& v):
	  WrapperBase<MultilineLabel>(d, v)
	{
		component.onTextChange = [&]()
		{
			this->data->onValueChange(getId(), component.getText(), useUndoManager());
		};

#if 0
		if((bool)dataTree[dcid::updateEachKey])
		{

			component.onTextChange = component.onReturnKey;
		}
#endif

		initSpecialProperties({ dcid::text, dcid::multiline, dcid::editable});
	}

	void onValue(const var& newValue) override
	{
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

struct DragContainer: public Base
{
	SN_NODE_ID("DragContainer");

	DragContainer(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		auto idSelector = String("#") + getId().toString();
		getProperties().set(dcid::id, idSelector);

		isVertical = v.getProperty(dcid::isVertical, true);
		dragMargin = v.getProperty(dcid::dragMargin, 3);
		animationSpeed = v.getProperty(dcid::animationSpeed, 150);
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v) { return new DragContainer(d, v); }

	void rearrangeChildren(const Array<var>& sortedIndexes, bool force)
	{
		bool same = sortedIndexes.size() == currentIndexes.size();

		for(int i = 0; i < currentIndexes.size(); i++)
			same &= currentIndexes[i] == sortedIndexes[i];

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

			if(auto c = getChildComponent(idx))
			{
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

	void onValue(const var& newValue)
	{
		if(auto a = newValue.getArray())
		{
			Array<var> copy;
			copy.addArray(*a);

			if(copy.size() == getNumChildComponents())
			{
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
		};

		~Dragger()
		{
			if(child != nullptr)
				child->removeMouseListener(this);
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
			parent.data->onValueChange(parent.getId(), var(parent.currentIndexes), (bool)parent.dataTree[dcid::useUndoManager]);

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
		auto slaf = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(&getLookAndFeel());
		slaf->drawComponentBackground(g, this);
	}

	void updateChild(const ValueTree& v, bool wasAdded) override
	{
		Base::updateChild(v, wasAdded);

		if(wasAdded)
		{
			auto c = getChildComponent(getNumChildComponents()-1);
			auto nd = new Dragger(*this, c);
			draggers.add(nd);
		}
		else
		{
			for(auto d: draggers)
			{
				if(*d->child == v)
				{
					draggers.removeObject(d);
				}
			}
		}

		rebuildIndexArrayFromPosition(true);
	}

	void rebuildIndexArrayFromPosition(bool force)
	{
		Array<Component*> list;
		for(int i = 0; i<  getNumChildComponents(); i++)
			list.add(getChildComponent(i));

		struct Sorter
		{
			Sorter(bool isVertical_, int maxValue_):
			  isVertical(isVertical_),
			  maxValue(maxValue_) 
			{};

			int compareElements(Component* c1, Component* c2) const
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

		Array<Component*> sorted;
		sorted.addArray(list);
		sorted.sort(s);

		Array<var> newIndexes;

		for(auto& s: sorted)
		{
			newIndexes.add(list.indexOf(s));
		}
		
		rearrangeChildren(newIndexes, force);
	}

	bool isVertical = true;
	double animationSpeed = 150.0;
	int dragMargin = 3;
	Array<var> currentIndexes;

	OwnedArray<Dragger> draggers;
};

struct Panel: public Base
{
	SN_NODE_ID("Panel");

	Panel(Data::Ptr d, const ValueTree& v):
	  Base(d, v)
	{
		auto idSelector = String("#") + getId().toString();

		simple_css::FlexboxComponent::Helpers::setCustomType(*this, simple_css::Selector(simple_css::ElementType::Panel));

		getProperties().set(dcid::id, idSelector);
	}

	void onValue(const var& newValue) override
	{
		
	}

	void paint(Graphics& g) override
	{
		auto slaf = dynamic_cast<simple_css::StyleSheetLookAndFeel*>(&getLookAndFeel());
		slaf->drawComponentBackground(g, this);

		auto text = dataTree[dcid::text].toString();

		if(text.isNotEmpty())
		{
			slaf->drawGenericComponentText(g, text, this);
		}
	}

	static Base* createComponent(Data::Ptr d, const ValueTree& v)
	{
		return new Panel(d, v);
	}
};




Data::Data(const var& obj, Rectangle<int> position):
	data(fromJSON(obj, position)),
	values(ValueTree("Values")),
	factory(new Factory())
{
		
}

void Data::setValues(const var& valueObject)
{
	if(auto obj = valueObject.getDynamicObject())
	{
		for(auto& nv: obj->getProperties())
		{
			values.setProperty(nv.name, nv.value, um);
		}
	}
}

ValueTree Data::fromJSON(const var& obj, Rectangle<int> position)
{
	auto v = ValueTreeConverters::convertDynamicObjectToContentProperties(obj);

	v.setProperty(dcid::x, position.getX(), nullptr);
	v.setProperty(dcid::y, position.getY(), nullptr);
	v.setProperty(dcid::width, position.getWidth(), nullptr);
	v.setProperty(dcid::height, position.getHeight(), nullptr);

	std::map<String, String> typesToConvert = {
		{ "ScriptButton", "Button" },
		{ "ScriptSlider", "Slider" },
		{ "ScriptComboBox", "ComboBox" },
		{ "ScriptLabel", "Label" },
		{ "ScriptPanel", "Panel" }
	};

	valuetree::Helpers::forEach(v, [&](ValueTree& c)
	{
		auto type = c[dcid::type].toString();

		if(typesToConvert.find(type) != typesToConvert.end())
			c.setProperty(dcid::type, typesToConvert[type], nullptr);

		if(!c.hasProperty(dcid::width))
			c.setProperty(dcid::width, 128, nullptr);
		if(!c.hasProperty(dcid::height))
			c.setProperty(dcid::height, 50, nullptr);

		auto pid = c.getParent()[dcid::id];
		c.setProperty(dcid::parentComponent, pid, nullptr);
			
		return false;
	});

	return v;
}

ReferenceCountedObjectPtr<Base> Data::create(const ValueTree& v)
{
	auto id = v[dcid::id].toString();

	if(id.isNotEmpty() && v.hasProperty(dcid::defaultValue) && !values.hasProperty(Identifier(id)))
		values.setProperty(Identifier(id), v[dcid::defaultValue], nullptr);
	
	return factory->create(this, v);
}

void Data::onValueChange(const Identifier& id, const var& newValue, bool useUndoManager)
{
	values.setProperty(id, newValue, useUndoManager ? um : nullptr);
	if(valueCallback)
		valueCallback(id, newValue);
}

const ValueTree& Data::getValueTree(TreeType t) const
{
	return t == TreeType::Data ? data : values;
}

Base::Base(Data::Ptr d, const ValueTree& v):
	data(d),
	dataTree(v),
	valueReference(data->getValueTree(Data::TreeType::Values))
{
	basicPropertyListener.setCallback(dataTree, { dcid::enabled, dcid::visible, dcid::class_, dcid::elementStyle}, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Base::updateBasicProperties));
	positionListener.setCallback(dataTree, { dcid::x, dcid::y, dcid::width, dcid::height}, valuetree::AsyncMode::Coallescated, BIND_MEMBER_FUNCTION_2(Base::updatePosition));

	childListener.setCallback(dataTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Base::updateChild));

	if(getId().isValid())
	{
		valueListener.setCallback(valueReference, { getId() }, valuetree::AsyncMode::Asynchronously, [&](const Identifier& id, const var& newValue)
		{
			onValue(newValue);
		});
	}
}

Base::~Base()
{}

Identifier Base::getId() const
{
	auto s = dataTree[dcid::id].toString();
	return s.isNotEmpty() ? Identifier(s) : Identifier();
}

void Base::updateChild(const ValueTree& v, bool wasAdded)
{
	if(wasAdded)
	{
		auto newChild = data->create(v);
		addChild(newChild);
	}
	else
	{
		for(auto c: children)
		{
			if(c->dataTree == v)
			{
				removeChildComponent(c);
				children.removeObject(c);
				break;
			}
		}
	}
}

void Base::updateBasicProperties(const Identifier& id, const var& newValue)
{
    if(id == dcid::class_)
    {
		auto classes = StringArray::fromTokens(newValue.toString(), " ", "");

		auto c = getCSSTarget();

		auto existingClasses = simple_css::FlexboxComponent::Helpers::getClassSelectorFromComponentClass(c);

		for(auto cl: existingClasses)
			classes.add(cl.toString());

		classes.removeDuplicates(false);
		classes.removeEmptyStrings();
		
		simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*c, classes);
    }
	if(id == dcid::elementStyle)
	{
	    simple_css::FlexboxComponent::Helpers::writeInlineStyle (*getCSSTarget(), newValue.toString());
	}
	if(id == dcid::enabled)
		setEnabled((bool)newValue);
	if(id == dcid::visible)
		setVisible((bool)newValue);
}

void Base::updatePosition(const Identifier&, const var&)
{
	Rectangle<int> b((int)dataTree[dcid::x], (int)dataTree[dcid::y], (int)dataTree[dcid::width], (int)dataTree[dcid::height]);
	setBounds(b);
	resized();
}

void Base::addChild(Base::Ptr c)
{
	c->setLookAndFeel(&getLookAndFeel());
	children.add(c);
	addAndMakeVisible(c.get());
	c->updatePosition({}, {});
}

Factory::Factory()
{
	registerItem<Button>();
	registerItem<Slider>();
	registerItem<ComboBox>();
	registerItem<Panel>();
	registerItem<Label>();
	registerItem<DragContainer>();
}

void Root::setCSS(const String& styleSheetCode)
{
	simple_css::Parser p(styleSheetCode);
	auto ok = p.parse();
	this->css = p.getCSSValues();

	ScopedPointer<simple_css::StyleSheet::Collection::DataProvider> dp = createDataProvider();

	this->css.performAtRules(dp);

	dp = nullptr;

	this->css.setAnimator(&animator);

	auto newLaf = new simple_css::StyleSheetLookAndFeel(*this);

	Component::callRecursive<Base>(this, [newLaf](Base* b)
	{
		b->setLookAndFeel(newLaf);
		return false;
	});

	laf = newLaf;
}

Root::Root(Data::Ptr d):
  Base(d, d->getValueTree(Data::TreeType::Data))
{
	
}

} // namespace dyncomp
} // namespace hise
