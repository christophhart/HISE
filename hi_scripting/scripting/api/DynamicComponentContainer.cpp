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

struct Factory: public ReferenceCountedObject
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

struct Data::DragContainerHandler::EffectChainManager
{
	template <typename T> static void initIdAndSetEffect(Processor* fxChain, const ValueTree& componentData, ValueTree& v, bool wasAdded)
	{
		Range<int> dragEffectRange = {
			(int)componentData[dyncomp::dcid::min],
			(int)componentData[dyncomp::dcid::max]
		};

		auto fxName = wasAdded ? v[dyncomp::dcid::text].toString() : "";
		jassert(v.hasProperty(dyncomp::dcid::index));
		auto fxIndex = (int)v[dyncomp::dcid::index] + dragEffectRange.getStart();

		Processor::Iterator<T> iter(fxChain, false);

		int index = 0;

		while(auto fx = iter.getNextProcessor())
		{
			if(index == fxIndex)
			{
				if(!v.hasProperty(dyncomp::dcid::id))
				{
					auto newId = componentData[dyncomp::dcid::id].toString() + "_" + v[dyncomp::dcid::index].toString();
					v.setProperty(dyncomp::dcid::id, fx->getId(), nullptr);
				}

				if(auto swappable = dynamic_cast<HotswappableProcessor*>(fx))
				{
					swappable->setEffect(fxName, false);
					break;
				}
			}

			index++;
		}
	}

	static void setFXOrder(EffectProcessorChain* fxChain, const ValueTree& componentData, const var& value)
	{
		auto isPoly = (bool)componentData[dyncomp::dcid::polyFX];
		auto low = (int)componentData[dyncomp::dcid::min];
		auto high = (int)componentData[dyncomp::dcid::max];

		Range<int> r(low, high);

		if(isPoly)
			fxChain->setFXOrderInternal<VoiceEffectProcessor>(r, value);
		else
			fxChain->setFXOrderInternal<MasterEffectProcessor>(r, value);
	}
};

Data::DragContainerHandler::DragContainerHandler(Data* d, const ValueTree& v):
	SubDataHandler(d, v)
{
	connectionListener.setCallback(componentData, 
	                               valuetree::AsyncMode::Synchronously,
	                               VT_BIND_CHILD_LISTENER(onChildAdd));

	initValueListener();

	dragEffectRange = {
		(int)componentData[dyncomp::dcid::min],
		(int)componentData[dyncomp::dcid::max]
	};
}

void Data::DragContainerHandler::onValue(const Identifier& id, const var& newValue)
{
	if(!newValue.isVoid() && newValue != lastValue)
	{
		lastValue = newValue;

		if(auto fxChain = getDragContainerEffectChain())
		{
			EffectChainManager::setFXOrder(fxChain, componentData, lastValue);
		}
	}
}

EffectProcessorChain* Data::DragContainerHandler::getDragContainerEffectChain()
{
	auto pid = componentData[dyncomp::dcid::processorId].toString();

	if(auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), pid))
	{
		if(auto fx = dynamic_cast<EffectProcessorChain*>(p->getChildProcessor(ModulatorSynth::EffectChain)))
			return fx;
	}

	return nullptr;
}

void Data::DragContainerHandler::onChildAdd(ValueTree v, bool wasAdded)
{
	if(wasAdded)
	{
		// write the index && id properties if they do not exist yet
		if(!v.hasProperty(dyncomp::dcid::index))
		{
			VoiceBitMap<256, uint32> map;

			for(auto c: componentData)
			{
				if(c == v)
					break;

				auto cIndex = (int)c[dyncomp::dcid::index];
				map.setBit(cIndex, true);
			}

			auto thisIndex = map.getFirstFreeBit();

			if(thisIndex >= dragEffectRange.getLength())
			{
				jassertfalse;
				return;
			}

			v.setProperty(dyncomp::dcid::index, thisIndex, nullptr);

			auto idToUse = dcid::Helpers::get(componentData, dcid::isVertical) ? dyncomp::dcid::y : dyncomp::dcid::x;
			v.setProperty(idToUse, dyncomp::dcid::Helpers::getMaxPositionOfChildComponents(componentData) + 1, nullptr);
		}
	}

	if(auto fxChain = getDragContainerEffectChain())
	{
		auto isPoly = (bool)componentData[dyncomp::dcid::polyFX];

		if(isPoly)
			EffectChainManager::initIdAndSetEffect<VoiceEffectProcessor>(fxChain, componentData, v, wasAdded);
		else
			EffectChainManager::initIdAndSetEffect<MasterEffectProcessor>(fxChain, componentData, v, wasAdded);
	}

	auto order = dyncomp::dcid::Helpers::getChildComponentOrderFromPosition(componentData);
	auto orderAsValue = dyncomp::dcid::Helpers::getDragContainerIndexValues(order, componentData);
	auto id = componentData[dyncomp::dcid::id].toString();

	values.setProperty(id, orderAsValue, nullptr);
}

Data::ComplexDataHandler::ComplexDataHandler(Data* d, const ValueTree& v_, const ExternalData::DataType& dt_):
	SubDataHandler(d, v_),
	dt(dt_)
{
	connectionListener.setCallback(componentData, 
	                               { dcid::processorId, dcid::index }, 
	                               valuetree::AsyncMode::Synchronously,
	                               VT_BIND_PROPERTY_LISTENER(onConnectionChange));

	initValueListener();
}

Data::ComplexDataHandler::~ComplexDataHandler()
{
	if(complexData != nullptr)
		complexData->getUpdater().removeEventListener(this);
}



void Data::ComplexDataHandler::handleAsyncUpdate()
{
	if(complexData != nullptr)
	{
		auto um = (bool)componentData[dcid::useUndoManager] ? getMainController()->getControlUndoManager() : nullptr;
		ScopedValueSetter<bool> svs(recursive, true);

		values.setPropertyExcludingListener(&valueListener, this->id, complexData->toBase64String(), um);
	}
}

void Data::ComplexDataHandler::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	jassert(complexData != nullptr);

	if(t == ComplexDataUIUpdaterBase::EventType::ContentChange || 
		t == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
	{
		triggerAsyncUpdate();
	}
}

void Data::ComplexDataHandler::onValue(const Identifier& pid, const var& newValue) 
{
	if(recursive || newValue.isVoid() || newValue.isUndefined())
		return;

	jassert(pid.toString() == id.toString());

	auto b64 = newValue.toString();

	if(complexData != nullptr)
		complexData->fromBase64String(b64);
}

void Data::ComplexDataHandler::onConnectionChange(const Identifier&, const var&)
{
	auto nd = dcid::Helpers::getComplexDataBase(getMainController(), componentData, dt);
			
	if(nd != complexData.get())
	{
		if(complexData != nullptr)
			complexData->getUpdater().removeEventListener(this);

		complexData = nd;
				
		if(complexData != nullptr)
		{
			complexData->getUpdater().addEventListener(this);
		}

		handleAsyncUpdate();
	}
}

Data::RefreshType Data::getRefreshType(const var& t)
{
	if(t.isString())
	{
		auto refreshMode_ = t.toString();

		if (refreshMode_ == "repaint")
			return RefreshType::repaint;
		else if (refreshMode_ == "changed")
			return RefreshType::changed;
		else if (refreshMode_ == "updateValueFromProcessorConnection")
			return RefreshType::updateValueFromProcessorConnection;
		else if (refreshMode_ == "loseFocus")
			return RefreshType::loseFocus;
		else if (refreshMode_ == "resetValueToDefault")
			return RefreshType::resetValueToDefault;
		else
		{
			jassertfalse;
			return RefreshType::numRefreshTypes;
		}
			
	}
	else
		return (RefreshType)(int)t;
}

Data::Data(MainController* mc, const var& obj, Rectangle<int> position):
	ControlledObject(mc),
	data(fromJSON(obj, position)),
	values(ValueTree("Values")),
	factory(new Factory()),
	floatingTileData(obj[dcid::FloatingTileData].getDynamicObject()),
	um(mc->getControlUndoManager())
{
	refreshBroadcaster.enableLockFreeUpdate(mc->getGlobalUIUpdater());
	refreshBroadcaster.setEnableQueue(true);

	valueInitialiser.setTypeToWatch(dcid::Component);
	valueInitialiser.setCallback(getValueTree(TreeType::Data),
		valuetree::AsyncMode::Synchronously,
		VT_BIND_CHILD_LISTENER(onChildAdd));
}

Data::~Data()
{
	valueInitialiser.shutdown();
	registeredDrawHandlers.clear();
	subDataHandlers.clear();
	valueCallback = {};
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

Image Data::getImage(const String& ref)
{
	if(ip)
	{
		return ip(ref);
	}

	return {};
}

simple_css::StyleSheet::Collection::DataProvider* Data::createDataProvider() const
{
	struct DP: public simple_css::StyleSheet::Collection::DataProvider
	{
		DP() = default;

		Font loadFont(const String& fontName, const String& url) override
		{
			if(fp)
				return fp(fontName, url);

			return Font(fontName, 13.0f, Font::plain);
		}

		String importStyleSheet(const String& url) override
		{
			return {};
		}

		Image loadImage(const String& imageURL) override
		{
			if(ip)
				return ip(imageURL);

			return {};
		}

		ImageProvider ip;
		FontProvider fp;
	};

	auto np = new DP();
	np->ip = ip;
	np->fp = fp;
	return np;
}

ValueTree Data::fromJSON(const var& obj, Rectangle<int> position)
{
	var ft;

	ValueTree v;

	if(obj.hasProperty(dcid::ContentProperties))
	{
		v = ValueTreeConverters::convertDynamicObjectToContentProperties(obj[dcid::ContentProperties]);
		ft = obj[dcid::FloatingTileData];
	}
	else
	{
		v = ValueTreeConverters::convertDynamicObjectToContentProperties(obj);
	}

	v.setProperty(dcid::x, position.getX(), nullptr);
	v.setProperty(dcid::y, position.getY(), nullptr);
	v.setProperty(dcid::width, position.getWidth(), nullptr);
	v.setProperty(dcid::height, position.getHeight(), nullptr);

	std::map<String, String> typesToConvert = {
		{ "ScriptButton", "Button" },
		{ "ScriptSlider", "Slider" },
		{ "ScriptComboBox", "ComboBox" },
		{ "ScriptLabel", "Label" },
		{ "ScriptPanel", "Panel" },
		{ "ScriptViewport", "Viewport"}
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
	
	return getFactory()->create(this, v);
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

void Data::setValueCallback(const ValueCallback& v)
{
	valueCallback = v;
}

ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> Data::createGraphicsObject(const ValueTree& dataTree,
	ConstScriptingObject* obj)
{
	jassert(dataTree.getType() == Identifier("Component"));

	for(auto r: registeredDrawHandlers)
	{
		if(r->first == dataTree)
			return r->second;
	}

	auto n = new std::pair<ValueTree, ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject>>();
	n->first = dataTree;
	n->second = new ScriptingObjects::GraphicsObject(obj->getScriptProcessor(), obj);
	registeredDrawHandlers.add(n);
	return registeredDrawHandlers.getLast()->second;
}

DrawActions::Handler* Data::getDrawHandler(const ValueTree& dataTree) const
{
	for(auto r: registeredDrawHandlers)
	{
		if(r->first == dataTree)
			return &r->second->getDrawHandler();
	}

	return nullptr;
}

Factory* Data::getFactory()
{
    return dynamic_cast<Factory*>(factory.get());
}

void Data::onChildAdd(const ValueTree& v, bool wasAdded)
{
	auto vt = getValueTree(TreeType::Values);
	auto id = Identifier(v[dcid::id].toString());

	auto type = v[dcid::type].toString();

	ExternalData::forEachType([&](ExternalData::DataType dt)
	{
		auto name = ExternalData::getDataTypeName(dt, false);

		if(name == type)
			this->updateComplexDataHandler(v, wasAdded, dt);
	});

	if(type == "DragContainer")
	{
		this->updateDragContainer(v, wasAdded);
	}

	if(wasAdded)
	{
		auto defaultValue = v[dcid::defaultValue];

		if(!(defaultValue.isVoid() || defaultValue.isUndefined()))
			vt.setProperty(id, defaultValue, nullptr);
	}
	else
	{
		vt.removeProperty(id, nullptr);
	}
}

Data::SubDataHandler::SubDataHandler(Data* d, const ValueTree& v):
	ControlledObject(d->getMainController()),
	componentData(v),
	values(d->getValueTree(TreeType::Values)),
	id(v[dcid::id].toString())
{
			
}

void Data::SubDataHandler::initValueListener()
{
	valueListener.setCallback(values, { id }, valuetree::AsyncMode::Synchronously,
	                          VT_BIND_PROPERTY_LISTENER(onValue));
}

void Data::SubDataHandler::onValue(const Identifier& id, const var& newValue)
{
	// If you hit this, you've haven't overriden this method!
	jassertfalse;
}


void Data::updateComplexDataHandler(const ValueTree& v, bool wasAdded, ExternalData::DataType dt)
{
	if(wasAdded)
	{
		subDataHandlers.add(new ComplexDataHandler(this, v, dt));
	}
	else
	{
		for(auto c: subDataHandlers)
		{
			if(c->matches(v))
			{
				subDataHandlers.removeObject(c);
				break;
			}
		}
	}
}

void Data::updateDragContainer(const ValueTree& v, bool wasAdded)
{
	if(wasAdded)
	{
		subDataHandlers.add(new DragContainerHandler(this, v));
	}
	else
	{
		for(auto c: subDataHandlers)
		{
			if(c->matches(v))
			{
				subDataHandlers.removeObject(c);
				break;
			}
		}
	}
}

void Data::setDataProviderCallbacks(const ImageProvider& ip_, const FontProvider& fp_)
{
	ip = ip_;
	fp = fp_;
}

Base::Base(Data::Ptr d, const ValueTree& v):
	data(d),
	dataTree(v),
	valueReference(data->getValueTree(Data::TreeType::Values))
{
	auto basicProperties = dcid::Helpers::getBasicProperties();

	basicPropertyListener.setCallback(dataTree, basicProperties, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Base::updateBasicProperties));

	positionListener.setCallback(dataTree, { dcid::x, dcid::y, dcid::width, dcid::height}, valuetree::AsyncMode::Coallescated, BIND_MEMBER_FUNCTION_2(Base::updatePosition));

	childListener.setCallback(dataTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Base::updateChild));

	cssListener.setCallback(dataTree, dcid::Helpers::getCSSProperties(), valuetree::AsyncMode::Coallescated, VT_BIND_PROPERTY_LISTENER(updateCSSProperties));

	d->refreshBroadcaster.addListener(*this, onRefreshStatic, false);

	if(getId().isValid())
	{
		valueListener.setCallback(valueReference, { getId() }, valuetree::AsyncMode::Asynchronously, [&](const Identifier& id, const var& newValue)
		{
			onValue(getValueOrDefault());
		});
	}
}

Base::~Base()
{
	data->refreshBroadcaster.removeListener(*this);
}

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
				baseChildChanged(c, false);
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

		auto c = getContentComponent();

		auto existingClasses = simple_css::FlexboxComponent::Helpers::getClassSelectorFromComponentClass(c);

		for(auto cl: existingClasses)
			classes.add(cl.toString());

		classes.removeDuplicates(false);
		classes.removeEmptyStrings();
		
		simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*c, classes);
		simple_css::FlexboxComponent::Helpers::invalidateCache(*c);
    }
	if(id == dcid::elementStyle)
	{
	    simple_css::FlexboxComponent::Helpers::writeInlineStyle (*getContentComponent(), newValue.toString());
	}
	if(id == dcid::bgColour || id == dcid::itemColour || id == dcid::itemColour2 || id == dcid::textColour)
	{
		if(auto root = simple_css::CSSRootComponent::find(*this))
		{
			if(auto ptr = root->css.getForComponent(this))
			{
				ptr->setPropertyVariable(id, newValue);
				repaint();
			}
		}
	}
	if(id == dcid::enabled)
		setEnabled((bool)newValue);
	if(id == dcid::visible)
	{
		if(auto p = findParentComponentOfClass<Base>())
			p->hideChild(this, (newValue));
		else
			setVisible((bool)newValue);
	}
}

void Base::updatePosition(const Identifier&, const var&)
{
	Rectangle<int> b((int)dataTree[dcid::x], (int)dataTree[dcid::y], (int)dataTree[dcid::width], (int)dataTree[dcid::height]);

	if(auto parent = findParentComponentOfClass<Base>())
	{
		parent->resizeChild(this, b);
	}
}

void Base::baseChildChanged(Base::Ptr c, bool wasAdded)
{
	auto content = getContentComponent();

	if(wasAdded)
	{
		content->addAndMakeVisible(c.get());
		c->updatePosition({}, {});
	}
	else
	{
		content->removeChildComponent(c.get());
	}
		
}

void Base::resizeChild(Base::Ptr b, Rectangle<int> newBounds)
{
	b->setBounds(newBounds);
	b->resized();
}

void Base::hideChild(Base::Ptr b, bool shouldBeVisible)
{
	b->setVisible(shouldBeVisible);
}

void Base::updateCSSProperties(const Identifier&, const var&)
{
	writeComponentPropertiesToStyleSheet(true);
}

Base* Base::findBaseParent(Component* c)
{
	if(auto b = dynamic_cast<Base*>(c))
		return b;

	return c->findParentComponentOfClass<Base>();
}

void Base::onRefreshStatic(Base& b, const ValueTree& v, Data::RefreshType rt, bool isRecursive)
{
	if(b.dataTree == v)
		b.onRefresh(rt, isRecursive);
}

void Base::initCSSForChildComponent()
{
	jassert(getNumChildComponents() == 1);

	auto isOpaque = getContentComponent() != this;
	simple_css::FlexboxComponent::Helpers::setIsOpaqueWrapper(*this, isOpaque);

	auto idSelector = String("#") + getId().toString();
	getChildComponent(0)->getProperties().set(dcid::id, idSelector);

	if(dataTree.hasProperty(dcid::class_))
		updateBasicProperties(dcid::class_, dataTree[dcid::class_]);
	if(dataTree.hasProperty(dcid::elementStyle))
		updateBasicProperties(dcid::elementStyle, dataTree[dcid::elementStyle]);
}

void Base::onRefresh(Data::RefreshType rt, bool recursive)
{
	switch(rt)
	{
	case Data::RefreshType::repaint:
		getContentComponent()->repaint();
		break;
	case Data::RefreshType::changed:
		onValue(getValueOrDefault());
		break;
	case Data::RefreshType::updateValueFromProcessorConnection:
		jassertfalse;
		break;
	case Data::RefreshType::loseFocus:
		unfocusAllComponents();
		return; // no recursion needed
	case Data::RefreshType::resetValueToDefault:
		onValue(dataTree[dcid::defaultValue]);
	default: ;
	}

	if(recursive)
	{
		for(auto c: children)
			c->onRefresh(rt, true);
	}
}

void Base::addChild(Base::Ptr c)
{
	if(c != nullptr)
	{
		children.add(c);
		c->setLookAndFeel(&getLookAndFeel());
		baseChildChanged(c, true);
	}
}

bool Base::operator==(const Base& other) const noexcept
{
	return dataTree == other.dataTree;
}

bool Base::operator==(const ValueTree& otherData) const noexcept
{
	return dataTree == otherData;
}

void Base::writePositionInValueTree(Rectangle<int> tb, bool useUndoManager)
{
	dataTree.setProperty(dcid::x, tb.getX(), nullptr);
	dataTree.setProperty(dcid::y, tb.getY(), nullptr);
	dataTree.setProperty(dcid::width, tb.getWidth(), nullptr);
	dataTree.setProperty(dcid::height, tb.getHeight(), nullptr);
}

var Base::getValueOrDefault() const
{
	Identifier id_(dataTree[dcid::id].toString());
	auto vt = data->getValueTree(Data::TreeType::Values);

	if(vt.hasProperty(id_))
		return vt[id_];

	return dataTree[dcid::defaultValue];
}

var Base::getPropertyOrDefault(const Identifier& id) const
{
	if(dataTree.hasProperty(id))
		return dataTree[id];

	return dcid::Helpers::getDefaultValue(id);
}

void Base::setColourFromData(const Identifier& id, int colourId)
{
	auto c = getContentComponent();
	c->setColour(colourId, ApiHelpers::getColourFromVar(getPropertyOrDefault(id)));
}

void Base::updateColours()
{
	setColourFromData(dcid::bgColour, HiseColourScheme::ComponentOutlineColourId);
	setColourFromData(dcid::itemColour, HiseColourScheme::ComponentFillTopColourId);
	setColourFromData(dcid::itemColour2, HiseColourScheme::ComponentFillBottomColourId);
	setColourFromData(dcid::textColour, HiseColourScheme::ComponentTextColourId);
}

void Base::writeComponentPropertiesToStyleSheet(bool force)
{
	if(css == nullptr || force)
	{
		updateColours();	

		if(auto r = simple_css::CSSRootComponent::find(*this))
		{
			css = r->css.getForComponent(this);
		}

		if(css != nullptr)
		{

			for(auto id: dcid::Helpers::getCSSProperties())
			{
				auto v = dcid::Helpers::convertColour(id, getPropertyOrDefault(id));
				css->setPropertyVariable(id, v);
			}
					

			repaint();
		}
	}
}


} // namespace dyncomp
} // namespace hise
