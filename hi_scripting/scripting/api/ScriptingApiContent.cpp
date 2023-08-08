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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#if 0//JUCE_MAC
#define SEND_MESSAGE(broadcaster) {	broadcaster->sendAllocationFreeChangeMessage(); }
#elif 0
#define SEND_MESSAGE(broadcaster) {	if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); else broadcaster->sendChangeMessage();}
#else
#define SEND_MESSAGE(broadcaster) {	broadcaster->sendChangeMessage();}
#endif



#if USE_BACKEND
#define ADD_TO_TYPE_SELECTOR(x) (selectorTypes->addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (selectorTypes->addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))
#else
#define ADD_TO_TYPE_SELECTOR(x)
#define ADD_AS_SLIDER_TYPE(min, max, interval)
#endif



#include <cmath>

namespace hise { using namespace juce;


ValueTreeUpdateWatcher::ScopedDelayer::ScopedDelayer(ValueTreeUpdateWatcher* watcher_) :
	watcher(watcher_)
{
	if (watcher != nullptr)
		watcher->delayCalls = true;
}

ValueTreeUpdateWatcher::ScopedDelayer::~ScopedDelayer()
{
	if (watcher != nullptr)
	{
		watcher->delayCalls = false;

		if (watcher->shouldCallAfterDelay)
			watcher->callListener();
	}
}

ValueTreeUpdateWatcher::ScopedSuspender::ScopedSuspender(ValueTreeUpdateWatcher* watcher_) :
	watcher(watcher_)
{
	if (watcher.get() != nullptr)
	{
		watcher->isSuspended = true;
	}
}

ValueTreeUpdateWatcher::ScopedSuspender::~ScopedSuspender()
{
	if (watcher.get() != nullptr)
	{
		watcher->isSuspended = false;
	}
}

ValueTreeUpdateWatcher::Listener::~Listener()
{}

ValueTreeUpdateWatcher::ValueTreeUpdateWatcher(ValueTree& v, Listener* l) :
	state(v),
	listener(l)
{
	state.addListener(this);
}

void ValueTreeUpdateWatcher::valueTreePropertyChanged(ValueTree& valueTrees, const Identifier& property)
{
	if (isCurrentlyUpdating)
		return;

	static const Identifier id("id");
	static const Identifier parentComponent("parentComponent");

	if (property == id || property == parentComponent)
		callListener();
}

void ValueTreeUpdateWatcher::valueTreeChildAdded(ValueTree& valueTrees, ValueTree& valueTree)
{
	callListener();
}

void ValueTreeUpdateWatcher::valueTreeChildRemoved(ValueTree& valueTrees, ValueTree& valueTree, int i)
{
	callListener();
}

void ValueTreeUpdateWatcher::valueTreeChildOrderChanged(ValueTree& valueTrees, int i, int i1)
{
	callListener();
}

void ValueTreeUpdateWatcher::valueTreeParentChanged(ValueTree& valueTrees)
{}

void ValueTreeUpdateWatcher::callListener()
{
	if (isSuspended)
		return;

	if (delayCalls)
	{
		shouldCallAfterDelay = true;
		return;
	}

	if (isCurrentlyUpdating)
		return;

	isCurrentlyUpdating = true;



	if (listener.get() != nullptr)
		listener->valueTreeNeedsUpdate();

	isCurrentlyUpdating = false;
}


#define ID(id) Identifier(#id)

void ScriptingApi::Content::initNumberProperties()
{
	if (ScriptComponent::numbersInitialised)
		return;

	ScriptComponent::numberPropertyIds = { ID(x), ID(y), ID(width), ID(height), ID(min), ID(max),
		ID(stepSize), ID(middlePosition), ID(defaultValue), ID(numStrips),
		ID(scaleFactor), ID(mouseSensitivity), ID(radioGroup), ID(fontSize), ID(FontSize),
		ID(sliderAmount), ID(alpha), ID(offset), ID(scale), ID(borderSize),
		ID(borderRadius) };

	ScriptComponent::numbersInitialised = true;
}

#undef ID

Array<Identifier> ScriptingApi::Content::ScriptComponent::numberPropertyIds;
bool ScriptingApi::Content::ScriptComponent::numbersInitialised = false;

void ScriptingApi::Content::PluginParameterConnector::setConnected(ScriptedControlAudioParameter *p)
{
	parameter = p;

	if (parameter != nullptr)
	{
		parameter->setControlledScriptComponent(dynamic_cast<ScriptComponent*>(this));
	}
}

void ScriptingApi::Content::PluginParameterConnector::sendParameterChangeNotification(float newValue)
{
	if (nextUpdateIsDeactivated)
	{
		nextUpdateIsDeactivated = false;
		return;
	}

	if (isConnected() && (parameter->getParameterIndex() != -1))
	{
		parameter->beginChangeGesture();
		parameter->setValueNotifyingHost(newValue);
		parameter->endChangeGesture();
	}
}




struct ScriptingApi::Content::ScriptComponent::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptComponent, set);
	API_METHOD_WRAPPER_1(ScriptComponent, get);
	API_METHOD_WRAPPER_0(ScriptComponent, getId);
	API_METHOD_WRAPPER_0(ScriptComponent, getValue);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setValue);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setValueNormalized);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setValueWithUndo);
	API_METHOD_WRAPPER_0(ScriptComponent, getValueNormalized);
	API_VOID_METHOD_WRAPPER_2(ScriptComponent, setColour);
	API_VOID_METHOD_WRAPPER_4(ScriptComponent, setPosition);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setTooltip);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, showControl);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, addToMacroControl);
	API_METHOD_WRAPPER_0(ScriptComponent, getWidth);
	API_METHOD_WRAPPER_0(ScriptComponent, getHeight);
	API_METHOD_WRAPPER_1(ScriptComponent, getLocalBounds);
	API_METHOD_WRAPPER_0(ScriptComponent, getChildComponents);
	API_VOID_METHOD_WRAPPER_0(ScriptComponent, changed);
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionX);
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionY);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setControlCallback);
	API_METHOD_WRAPPER_0(ScriptComponent, getAllProperties);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setKeyPressCallback);
	API_VOID_METHOD_WRAPPER_0(ScriptComponent, loseFocus);
    API_VOID_METHOD_WRAPPER_0(ScriptComponent, grabFocus);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setZLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setLocalLookAndFeel);
	API_VOID_METHOD_WRAPPER_0(ScriptComponent, sendRepaintMessage);
	API_VOID_METHOD_WRAPPER_2(ScriptComponent, fadeComponent);
	API_VOID_METHOD_WRAPPER_0(ScriptComponent, updateValueFromProcessorConnection);
};

#define ADD_SCRIPT_PROPERTY(id, name) static const Identifier id(name); propertyIds.add(id);
#define ADD_NUMBER_PROPERTY(id, name) ADD_SCRIPT_PROPERTY(id, name); jassert(numberPropertyIds.contains(id));

struct ScriptingApi::Content::ScriptComponent::GlobalCableConnection : public scriptnode::routing::GlobalRoutingManager::CableTargetBase
{
	using Cable = scriptnode::routing::GlobalRoutingManager::Cable;
	static constexpr auto CableType = scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable;

	GlobalCableConnection(ScriptComponent& p) :
		parent(p)
	{};

	~GlobalCableConnection()
	{
		if (cable != nullptr)
		{
			cable->removeTarget(this);
		}
	}

	void call(double v)
	{
		if (cable != nullptr)
			cable->sendValue(this, v);
	}

	virtual void sendValue(double v) override
	{
		// do nothing for now - we don't want to change the UI values by global connections (yet)...
	}

	Path getTargetIcon() const override
	{
		Path path;
		path.loadPathFromData(HiBinaryData::SpecialSymbols::macros, sizeof(HiBinaryData::SpecialSymbols::macros));
		return path;
	}

	String getTargetId() const override
	{
		String s;
		s << "Control: ";
		s << dynamic_cast<Processor*>(parent.getScriptProcessor())->getId();
		s << ".";
		s << parent.getName().toString();

		return s;
	}

	void selectCallback(Component* rootEditor) override
	{
#if USE_BACKEND
		auto brw = dynamic_cast<BackendRootWindow*>(rootEditor);
		brw->gotoIfWorkspace(dynamic_cast<Processor*>(parent.getScriptProcessor()));

		auto sc = &parent;

		auto f = [sc]()
		{
			auto b = sc->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();
			b->setSelection(sc, sendNotificationAsync);
		};

		Timer::callAfterDelay(400, f);
#endif
	}

	void connect(const String& id)
	{
		if (cable != nullptr)
		{
			cable->removeTarget(this);
		}

		auto m = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(parent.getScriptProcessor()->getMainController_());

		cable = dynamic_cast<Cable*>(m->getSlotBase(id, CableType).get());
		cable->addTarget(this);
	}

	ReferenceCountedObjectPtr<Cable> cable;

	ScriptComponent& parent;
};


ScriptingApi::Content::ScriptComponent::ScriptComponent(ProcessorWithScriptingContent* base, Identifier name_, int numConstants /*= 0*/) :
	ConstScriptingObject(base, numConstants),
	UpdateDispatcher::Listener(base->getScriptingContent()->getUpdateDispatcher()),
	name(name_),
	keyboardCallback(base, nullptr, {}, 1),
	parent(base->getScriptingContent()),
	controlSender(this, base),
	asyncValueUpdater(*this),
	propertyTree(name_.isValid() ? parent->getValueTreeForComponent(name) : ValueTree("Component")),
	value(0.0),
    subComponentNotifier(*this),
	skipRestoring(false),
	hasChanged(false),
	customControlCallback(var())
{
	jassert(propertyTree.isValid());

	ADD_SCRIPT_PROPERTY(textId, "text");

	ADD_SCRIPT_PROPERTY(vId, "visible");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(eId, "enabled");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(lId, "locked");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_NUMBER_PROPERTY(xId, "x");					ADD_AS_SLIDER_TYPE(0, 900, 1);
	ADD_NUMBER_PROPERTY(yId, "y");					ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	ADD_NUMBER_PROPERTY(wId, "width");				ADD_AS_SLIDER_TYPE(0, 900, 1);
	ADD_NUMBER_PROPERTY(hId, "height");				ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	ADD_NUMBER_PROPERTY(mId1, "min");
	ADD_NUMBER_PROPERTY(mId2, "max");
	ADD_NUMBER_PROPERTY(mId25, "defaultValue");
	ADD_SCRIPT_PROPERTY(tId, "tooltip");
	ADD_SCRIPT_PROPERTY(bId, "bgColour");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(iId1, "itemColour");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(iId2, "itemColour2");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(tId2, "textColour");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(mId3, "macroControl");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(sId1, "saveInPreset");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(iId4, "isPluginParameter"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId, "pluginParameterName");
    ADD_SCRIPT_PROPERTY(pId76, "isMetaParameter");  ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId72, "linkedTo");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId73, "automationID");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(uId, "useUndoManager");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId2, "parentComponent");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId3, "processorId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId4, "parameterId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	handleDefaultDeactivatedProperties();

	setDefaultValue(Properties::text, name.toString());
	setDefaultValue(Properties::visible, true);
	setDefaultValue(Properties::enabled, true);
	setDefaultValue(Properties::locked, false);
	
	setDefaultValue(Properties::min, 0.0);
	setDefaultValue(Properties::max, 1.0);
	setDefaultValue(Properties::tooltip, "");
	setDefaultValue(Properties::bgColour, (int64)0x55FFFFFF);
	setDefaultValue(Properties::itemColour, (int64)0x66333333);
	setDefaultValue(Properties::itemColour2, (int64)0xfb111111);
	setDefaultValue(Properties::textColour, (int64)0xFFFFFFFF);
	setDefaultValue(Properties::macroControl, -1);
	setDefaultValue(Properties::saveInPreset, true);
	setDefaultValue(Properties::defaultValue, 0);
	setDefaultValue(Properties::isPluginParameter, false);
	setDefaultValue(Properties::pluginParameterName, "");
    setDefaultValue(Properties::isMetaParameter, false);
	setDefaultValue(automationId, "");
	setDefaultValue(Properties::linkedTo, "");
	setDefaultValue(Properties::useUndoManager, false);
	setDefaultValue(Properties::parentComponent, "");
	setDefaultValue(Properties::processorId, " ");
	setDefaultValue(Properties::parameterId, "");

	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_0(getValue);
	ADD_API_METHOD_1(setValue);
	ADD_API_METHOD_1(setValueNormalized);
	ADD_API_METHOD_1(setValueWithUndo);
	ADD_API_METHOD_0(getValueNormalized);
	ADD_API_METHOD_2(setColour);
	ADD_API_METHOD_4(setPosition);
	ADD_API_METHOD_1(setTooltip);
	ADD_API_METHOD_1(showControl);
	ADD_API_METHOD_1(addToMacroControl);
	ADD_API_METHOD_0(getWidth);
	ADD_API_METHOD_0(getHeight);
	ADD_API_METHOD_1(getLocalBounds);
	ADD_API_METHOD_0(getChildComponents);
	ADD_API_METHOD_0(changed);
	ADD_API_METHOD_0(getGlobalPositionX);
	ADD_API_METHOD_0(getGlobalPositionY);
	ADD_API_METHOD_1(setControlCallback);
	ADD_API_METHOD_0(getAllProperties);
	ADD_API_METHOD_1(setZLevel);
	ADD_API_METHOD_1(setKeyPressCallback);
	ADD_API_METHOD_0(loseFocus);
    ADD_API_METHOD_0(grabFocus);
	ADD_API_METHOD_1(setLocalLookAndFeel);
	ADD_API_METHOD_0(sendRepaintMessage);
	ADD_API_METHOD_2(fadeComponent);
	ADD_API_METHOD_0(updateValueFromProcessorConnection);

	//setName(name_.toString());


	//enableAllocationFreeMessages(30);
}

StringArray ScriptingApi::Content::ScriptComponent::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(macroControl))
	{
		StringArray sa;

		sa.add("No MacroControl");
		for (int i = 0; i < 8; i++)
		{
			sa.add("Macro " + String(i + 1));
		}

		return sa;
	}
	else if (id == getIdFor(parentComponent))
	{
		auto c = parent;

		StringArray sa;

		sa.add("");

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			if (c->getComponent(i) == this) break;
			sa.add(c->getComponent(i)->getName().toString());
		}

		return sa;
	}
	else if (id == getIdFor(automationId))
	{
		StringArray sa;
		sa.add("");
		sa.addArray(getScriptProcessor()->getMainController_()->getUserPresetHandler().getCustomAutomationIds());
		return sa;
	}
	else if (id == getIdFor(processorId))
	{
		auto sa = ProcessorHelpers::getListOfAllConnectableProcessors(dynamic_cast<Processor*>(getScriptProcessor()));
		sa.add("GlobalCable");
		return sa;
	}
	else if (id == getIdFor(parameterId))
	{
		if (connectedProcessor.get() != nullptr)
		{
			return ProcessorHelpers::getListOfAllParametersForProcessor(connectedProcessor.get());
		}
		else if (globalConnection != nullptr)
		{
			auto m = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(getScriptProcessor()->getMainController_());

			return m->getIdList(scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable);
		}
	}
	else if (id == getIdFor(linkedTo))
	{
		auto c = parent;

		StringArray sa;

		sa.add("");

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			auto sc = c->getComponent(i);

			if(sc->getObjectName() != getObjectName())
				continue;

			// Only allow connection to controls above
			if (sc == this)
				break;

			sa.add(sc->getName().toString());
		}

		return sa;
	}

	return StringArray();
}

Identifier ScriptingApi::Content::ScriptComponent::getName() const
{
	return name;
}

Identifier ScriptingApi::Content::ScriptComponent::getObjectName() const
{
	jassertfalse;
	return Identifier();
}

ValueTree ScriptingApi::Content::ScriptComponent::exportAsValueTree() const
{
	ValueTree v("Control");

	v.setProperty("type", getObjectName().toString(), nullptr);
	v.setProperty("id", getName().toString(), nullptr);

	if (value.isObject())
	{
		v.setProperty("value", "JSON" + JSON::toString(value, true), nullptr);
	}
	else
	{
		v.setProperty("value", value, nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptComponent::restoreFromValueTree(const ValueTree &v)
{
	const var data = v.getProperty("value", var::undefined());
	bool allowStrings = dynamic_cast<Label*>(this) != nullptr;

	

	auto newValue = Content::Helpers::getCleanedComponentValue(data, allowStrings);

	setValue(newValue);
}

void ScriptingApi::Content::ScriptComponent::doubleClickCallback(const MouseEvent &, Component* /*componentToNotify*/)
{
#if USE_BACKEND

	jassertfalse;
	
#endif
}

var ScriptingApi::Content::ScriptComponent::getAssignedValue(int index) const
{
	return getScriptObjectProperty(index);
}

void ScriptingApi::Content::ScriptComponent::assign(const int index, var newValue)
{
	setScriptObjectProperty(index, newValue);
}

int ScriptingApi::Content::ScriptComponent::getCachedIndex(const var &indexExpression) const
{
	Identifier id(indexExpression.toString());

	for (int i = 0; i < getNumIds(); i++)
	{
		if (deactivatedProperties.contains(getIdFor(i))) continue;

		if (getIdFor(i) == id) return i;
	}

	return -1;
}

void ScriptingApi::Content::ScriptComponent::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	jassert(propertyIds.contains(id));

    if(newValue.isObject())
    {
        logErrorAndContinue("You must specify the unique component name, not the object itself");
    }
    
	if (id == getIdFor(macroControl))
	{
		StringArray sa = getOptionsFor(id);

		const int index = sa.indexOf(newValue.toString()) - 1;

		addToMacroControl(index);
	}
	else if (id == getIdFor(automationId))
	{
		if (currentAutomationData != nullptr)
		{
			currentAutomationData->asyncListeners.removeListener(*this);
		}

		if (!newValue.toString().isEmpty())
		{
			Identifier newCustomId(newValue.toString());

			auto registerAutomationId = [newCustomId](ScriptComponent& sc, bool unused)
			{
				if ((sc.currentAutomationData = sc.getScriptProcessor()->getMainController_()->getUserPresetHandler().getCustomAutomationData(newCustomId)))
				{
					sc.currentAutomationData->asyncListeners.addListener(sc, [](ScriptComponent& c, int index, float v)
					{
						c.setValue(v);
					}, true);
				}
			};

			registerAutomationId(*this, true);

			if(currentAutomationData == nullptr)
			{
				if(getScriptProcessor()->getMainController_()->getUserPresetHandler().getNumCustomAutomationData() != 0)
					logErrorAndContinue("Automation ID " + newValue.toString() + " wasn't found");
				else
				{
					// This usually means that the script that initialises the custom data model hasn't run yet
					// so we can defer the automation setup to later...
					
					getScriptProcessor()->getMainController_()->getUserPresetHandler().deferredAutomationListener.addListener(*this, registerAutomationId, false);
				}
			}
		}
		else
			currentAutomationData = nullptr;
	}
	else if (id == getIdFor(linkedTo))
	{
		if (newValue.toString().isEmpty())
		{
			if (linkedComponent.get() != nullptr)
				linkedComponent->removeLinkedTarget(this);

			linkedComponent = nullptr;
		}
		else
		{
			auto c = parent;

			linkedComponent = c->getComponentWithName(Identifier(newValue).toString());

			if (linkedComponent)
				linkedComponent->addLinkedTarget(this);
			else
				logErrorAndContinue("Component with name " + newValue.toString() + " wasn't found");
		}

		updatePropertiesAfterLink(notifyEditor);

		if (linkedComponent != nullptr)
			setValue(linkedComponent->getValue());
	}
	else if (id == getIdFor(parentComponent))
	{
		auto pName = newValue.toString();

		if (pName.isNotEmpty())
		{
			auto pId = Identifier(pName);

			auto pTree = parent->getValueTreeForComponent(pId);

			if (!pTree.isValid())
			{
				reportScriptError("parentComponent " + newValue.toString() + " can't be found");
				RETURN_VOID_IF_NO_THROW();
			}

			if (pTree.isAChildOf(propertyTree))
			{
				reportScriptError(newValue.toString() + " is a child component of " + name.toString());
				RETURN_VOID_IF_NO_THROW();
			}

			auto oldParent = propertyTree.getParent();

			if (oldParent != pTree)
			{
				propertyTree.getParent().removeChild(propertyTree, nullptr);
				pTree.addChild(propertyTree, -1, nullptr);
			}
		}
		else
		{
			auto root = parent->getContentProperties();

			auto oldParent = propertyTree.getParent();

			if (root != oldParent)
			{
				propertyTree.getParent().removeChild(propertyTree, nullptr);
				root.addChild(propertyTree, -1, nullptr);
			}
		}
	}
	else if (id == getIdFor(x) || id == getIdFor(y) || id == getIdFor(width) ||
		id == getIdFor(height) || id == getIdFor(enabled))
	{
		
	}
	else if (id == getIdFor(visible))
	{
		const bool wasVisible = (bool)getScriptObjectProperty(visible);
		const bool isNowVisible = (bool)newValue;

		setScriptObjectProperty(visible, newValue, notifyEditor);

		if (wasVisible != isNowVisible)
		{
			repaintThisAndAllChildren();
		}
	}
	else if (id == getIdFor(processorId))
	{
		auto pId = newValue.toString();

		auto shouldUseGlobalCable = pId == "GlobalCable";
		auto usesGlobalCable = globalConnection != nullptr;

		if (shouldUseGlobalCable != usesGlobalCable)
		{
			globalConnection = shouldUseGlobalCable ? new GlobalCableConnection(*this) : nullptr;
		}
		
		if (pId == " " || pId == "")
		{
			connectedProcessor = nullptr;
			setScriptObjectPropertyWithChangeMessage(getIdFor(parameterId), "", sendNotification);
		}
		else if (pId.isNotEmpty())
		{
			connectedProcessor = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), pId);
		}
        
        updateValueFromProcessorConnection();
	}
	else if (id == getIdFor(parameterId))
	{
		auto parameterName = newValue.toString();

		if (globalConnection != nullptr)
		{
			globalConnection->connect(parameterName);

			connectedProcessor = nullptr;
			connectedParameterIndex = -1;
		}
		else
		{
			if (parameterName.isNotEmpty())
			{
				connectedParameterIndex = ProcessorHelpers::getParameterIndexFromProcessor(connectedProcessor, Identifier(parameterName));
			}
			else
				connectedParameterIndex = -1;
		}
        
        updateValueFromProcessorConnection();
	}

	setScriptObjectProperty(propertyIds.indexOf(id), newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptComponent::updateValueFromProcessorConnection()
{
    if(connectedProcessor != nullptr && connectedParameterIndex != -1)
    {
        float value = 0.0f;
        
        if (connectedParameterIndex == -2)
        {
            if (auto mod = dynamic_cast<Modulation*>(connectedProcessor.get()))
                value = mod->getIntensity();
        }
        else if (connectedParameterIndex == -3)
            value = connectedProcessor->isBypassed() ? 1.0f : 0.0f;
        else if (connectedParameterIndex == -4)
            value = connectedProcessor->isBypassed() ? 0.0f : 1.0f;
        else
            value = connectedProcessor->getAttribute(connectedParameterIndex);

        FloatSanitizers::sanitizeFloatNumber(value);
        setValue(value);
    }
}

const Identifier ScriptingApi::Content::ScriptComponent::getIdFor(int p) const
{
	jassert(p < getNumIds());

	return propertyIds[p];
}

int ScriptingApi::Content::ScriptComponent::getNumIds() const
{
	return propertyIds.size();
}

const var ScriptingApi::Content::ScriptComponent::getScriptObjectProperty(int p) const
{
	auto id = getIdFor(p);

	return getScriptObjectProperty(id);
}

const var ScriptingApi::Content::ScriptComponent::getScriptObjectProperty(Identifier id) const
{
	if (propertyTree.hasProperty(id))
		return propertyTree.getProperty(id);
	else
	{
		if (defaultValues.contains(id))
			return defaultValues[id];
		else
		{
			jassertfalse;
			return var();
		}
	}
}

var ScriptingApi::Content::ScriptComponent::getNonDefaultScriptObjectProperties() const
{
	DynamicObject::Ptr clone = new DynamicObject();

	for (int i = 0; i < propertyTree.getNumProperties(); i++)
	{
		auto id = propertyTree.getPropertyName(i);
		auto propValue = propertyTree.getProperty(id);

		if (isPropertyDeactivated(id))
			continue;

		if (defaultValues[id] == propValue)
			continue;

		clone->setProperty(id, propValue);
	}

	return var(clone.get());
}


String ScriptingApi::Content::ScriptComponent::getScriptObjectPropertiesAsJSON() const
{
	auto v = getNonDefaultScriptObjectProperties();

	return JSON::toString(v, false, DOUBLE_TO_STRING_DIGITS);
}

bool ScriptingApi::Content::ScriptComponent::isPropertyDeactivated(Identifier &id) const
{
	return deactivatedProperties.contains(id);
}

Rectangle<int> ScriptingApi::Content::ScriptComponent::getPosition() const
{
	const int x = getScriptObjectProperty(Properties::x);
	const int y = getScriptObjectProperty(Properties::y);
	const int w = getScriptObjectProperty(Properties::width);
	const int h = getScriptObjectProperty(Properties::height);

	return Rectangle<int>(x, y, w, h);
}

void ScriptingApi::Content::ScriptComponent::set(String propertyName, var newValue)
{
	Identifier propertyId = Identifier(propertyName);

	if (!propertyIds.contains(propertyId))
	{
		reportScriptError("the property doesn't exist");
		RETURN_VOID_IF_NO_THROW();
	}

	handleScriptPropertyChange(propertyId);

	setScriptObjectPropertyWithChangeMessage(propertyId, newValue, parent->allowGuiCreation ? dontSendNotification : sendNotification);
}

var ScriptComponent::getValue() const
{
	var rv;
	{
		SimpleReadWriteLock::ScopedReadLock sl(valueLock);
		rv = value;
	}
			
	jassert(!value.isString());
	return rv;
}


void ScriptingApi::Content::ScriptComponent::sendValueListenerMessage()
{
	if (valueListener != nullptr)
	{
		auto currentThread = getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread();

		if (currentThread == MainController::KillStateHandler::AudioThread)
		{
			asyncValueUpdater.triggerAsyncUpdate();
			return;
		}

		var a[2];
		a[0] = var(this);
		a[1] = getValue();
		var::NativeFunctionArgs args(var(this), a, 2);
		valueListener->call(nullptr, args, nullptr);
	}
}


void ScriptingApi::Content::ScriptComponent::changed()
{
	if (!parent->asyncFunctionsAllowed())
		return;

	controlSender.sendControlCallbackMessage();
	sendValueListenerMessage();

	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		// We need to throw an error again to stop the execution of the script
		// (a recursive function call to this method will not be terminated because
		// (the error is already consumed by the control callback handling).
		if (!jp->getLastErrorMessage().wasOk())
			reportScriptError("Aborting script execution after error occured during changed() callback");
	}
}

void ScriptingApi::Content::ScriptComponent::AsyncValueUpdater::handleAsyncUpdate()
{
	parent.sendValueListenerMessage();
}



ScriptingApi::Content::ScriptComponent::AsyncControlCallbackSender::AsyncControlCallbackSender(ScriptComponent* parent_, ProcessorWithScriptingContent* p_) :
	UpdateDispatcher::Listener(p_->getScriptingContent()->getUpdateDispatcher()),
	parent(parent_),
	p(p_)
{}

void ScriptingApi::Content::ScriptComponent::AsyncControlCallbackSender::sendControlCallbackMessage()
{
	if (!changePending)
	{
		changePending = true;

		if (p->getMainController_()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::ScriptingThread ||
            p->getMainController_()->isFlakyThreadingAllowed())
			handleAsyncUpdate();
		else
			triggerAsyncUpdate();
	}
}

void ScriptingApi::Content::ScriptComponent::AsyncControlCallbackSender::cancelMessage()
{
	cancelPendingUpdate();
	changePending = false;
}

void ScriptingApi::Content::ScriptComponent::AsyncControlCallbackSender::handleAsyncUpdate()
{
	if (parent != nullptr)
	{
		p->controlCallback(parent, parent->getValue());

		if (auto sp = dynamic_cast<ScriptPanel*>(parent))
			sp->repaint();

		changePending = false;
	}
}


void ScriptingApi::Content::ScriptComponent::setValue(var controlValue)
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
    
    if (controlValue.isString())
    {
        reportScriptError("You must not store Strings as value. Use either numbers or an Object");
    }
    
#endif
    
	if (!controlValue.isObject())
	{
		value = controlValue;
		jassert(!std::isnan((float)value));
	}
	else if (parent != nullptr)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(valueLock);
		std::swap(value, controlValue);
	}

	if (parent->allowGuiCreation)
	{
		skipRestoring = true;
	}

	for (auto t : linkedComponentTargets)
	{
		if (t != nullptr)
		{
			t->setValue(controlValue);
		}
	}

	triggerAsyncUpdate();
	sendValueListenerMessage();
};

void ScriptingApi::Content::ScriptComponent::setColour(int colourId, int colourAs32bitHex)
{
	switch (colourId)
	{
	case 0: propertyTree.setProperty(getIdFor(bgColour), (int64)colourAs32bitHex, nullptr); break;
	case 1:	propertyTree.setProperty(getIdFor(itemColour), (int64)colourAs32bitHex, nullptr); break;
	case 2:	propertyTree.setProperty(getIdFor(itemColour2), (int64)colourAs32bitHex, nullptr); break;
	case 3:	propertyTree.setProperty(getIdFor(textColour), (int64)colourAs32bitHex, nullptr); break;
	}
}


void ScriptingApi::Content::ScriptComponent::setPropertiesFromJSON(const var &jsonData)
{
	if (!jsonData.isUndefined() && jsonData.isObject())
	{
		NamedValueSet dataSet = jsonData.getDynamicObject()->getProperties();

		for (int i = 0; i < priorityProperties.size(); i++)
		{
			if (dataSet.contains(priorityProperties[i]))
			{
				handleScriptPropertyChange(priorityProperties[i]);

				setScriptObjectPropertyWithChangeMessage(priorityProperties[i], dataSet[priorityProperties[i]], sendNotification);
			}
		}

		for (int i = 0; i < propertyIds.size(); i++)
		{
			auto propertyId = propertyIds[i];

			if (priorityProperties.contains(propertyId))
				continue;

			if (dataSet.contains(propertyId))
			{
				handleScriptPropertyChange(propertyId);
				setScriptObjectPropertyWithChangeMessage(propertyId, dataSet[propertyId], sendNotification);
			}
		}
	}
}

int ScriptingApi::Content::ScriptComponent::getGlobalPositionX()
{
    const int thisX = getScriptObjectProperty(ScriptComponent::Properties::x);
    
	if (auto p = getParentScriptComponent())
	{
		return thisX + p->getGlobalPositionX();
	}
	else
		return thisX;
}

int ScriptingApi::Content::ScriptComponent::getGlobalPositionY()
{
    const int thisY = getScriptObjectProperty(ScriptComponent::Properties::y);
    
	if (auto p = getParentScriptComponent())
	{
		return thisY + p->getGlobalPositionY();
	}
	else
		return thisY;
}

void ScriptingApi::Content::ScriptComponent::setPosition(int x, int y, int w, int h)
{
	propertyTree.setProperty(getIdFor(Properties::x), x, nullptr);
	propertyTree.setProperty(getIdFor(Properties::y), y, nullptr);
	propertyTree.setProperty(getIdFor(Properties::width), w, nullptr);
	propertyTree.setProperty(getIdFor(Properties::height), h, nullptr);
}


void ScriptingApi::Content::ScriptComponent::addToMacroControl(int macroIndex)
{
    connectedMacroIndex = macroIndex;
    
};

void ScriptingApi::Content::ScriptComponent::setDefaultValue(int p, const var &defaultValue)
{
	defaultValues.set(getIdFor(p), defaultValue);
}

void ScriptingApi::Content::ScriptComponent::showControl(bool shouldBeVisible)
{
	setScriptObjectPropertyWithChangeMessage(getIdFor(visible), shouldBeVisible);
};


void ScriptingApi::Content::ScriptComponent::setTooltip(const String &newTooltip)
{
	setScriptObjectProperty(Properties::tooltip, newTooltip);
}

var ScriptingApi::Content::ScriptComponent::getWidth() const
{
	return (int)getScriptObjectProperty(Properties::width);
}

var ScriptingApi::Content::ScriptComponent::getHeight() const
{
	return (int)getScriptObjectProperty(Properties::height);
}

void ScriptingApi::Content::ScriptComponent::setValueWithUndo(var newValue)
{
	const int parameterIndex = parent->getComponentIndex(name);
	const float oldValue = (float)getValue();
	
	MacroControlledObject::UndoableControlEvent* newEvent = new MacroControlledObject::UndoableControlEvent(getProcessor(), parameterIndex, oldValue, (float)newValue);

	String undoName = getProcessor()->getId();
	undoName << " - " << getProcessor()->getIdentifierForParameterIndex(parameterIndex).toString() << ": " << String((float)newValue, 2);

	getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
}

void ScriptingApi::Content::ScriptComponent::setControlCallback(var controlFunction)
{
	auto obj = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(controlFunction.getDynamicObject());

    if(auto h = dynamic_cast<scriptnode::DspNetwork::Holder*>(getScriptProcessor()))
    {
        if(auto n = h->getActiveNetwork())
        {
            if(controlFunction.isObject() && n->isForwardingControlsToParameters())
            {
                reportScriptError("This script processor has a network that consumes the parameters");
            }
        }
    }
    
	if (obj != nullptr)
	{
		int numParameters = obj->parameterNames.size();

		if (numParameters == 2)
		{
			customControlCallback = controlFunction;
		}
		else
		{
			reportScriptError("Control Callback function must have 2 parameters: component and value");
		}
	}
	else if (controlFunction.isUndefined() || controlFunction == var())
	{
		customControlCallback = var();
	}
	else
	{
		reportScriptError("Control Callback function must be a inline function");
	}
}

ReferenceCountedObject* ScriptingApi::Content::ScriptComponent::getCustomControlCallback()
{
	return customControlCallback.getObject();
}

bool ScriptingApi::Content::ScriptComponent::isShowing(bool checkParentComponentVisibility) const
{
	const bool isVisible = getScriptObjectProperty(ScriptComponent::Properties::visible);

	if (!checkParentComponentVisibility)
		return isVisible;

	if (auto p = getParentScriptComponent())
		return isVisible && p->isShowing();
	else
		return isVisible;
}

bool ScriptingApi::Content::ScriptComponent::isConnectedToProcessor() const
{

	const bool hasProcessorConnection = connectedProcessor.get() != nullptr;

	const bool hasParameterConnection = connectedParameterIndex != -1;

	return hasProcessorConnection && hasParameterConnection;
}

void ScriptingApi::Content::ScriptComponent::updateContentPropertyInternal(int propertyId, const var& newValue)
{
	updateContentPropertyInternal(getIdFor(propertyId), newValue);
}

void ScriptingApi::Content::ScriptComponent::updateContentPropertyInternal(const Identifier& propId, const var& newValue)
{
	setScriptObjectPropertyWithChangeMessage(propId, newValue);
}

void ScriptingApi::Content::ScriptComponent::setScriptObjectProperty(int p, var newValue, NotificationType notifyListeners/*=dontSendNotification*/)
{
	auto id = getIdFor(p);

	auto defaultValue = defaultValues[id];

	const bool isDefaultValue = defaultValue == newValue;

	if (removePropertyIfDefault && isDefaultValue && !isPositionProperty(id))
	{
		propertyTree.removeProperty(id, nullptr);

		if(notifyListeners)
			propertyTree.sendPropertyChangeMessage(id);

		return;
	}
		

	if (notifyListeners == dontSendNotification)
	{
		if (auto propValuePointer = propertyTree.getPropertyPointer(getIdFor(p)))
		{
			var copy(newValue); // rather ugly..
			const_cast<var*>(propValuePointer)->swapWith(copy);
		}
		else
			propertyTree.setProperty(getIdFor(p), newValue, nullptr);
	}
	else
	{
		propertyTree.setProperty(getIdFor(p), newValue, nullptr);
	}
}

bool ScriptingApi::Content::ScriptComponent::hasProperty(const Identifier& id) const
{
	return propertyIds.contains(id);
}

var ScriptingApi::Content::ScriptComponent::get(String propertyName) const
{
	Identifier id(propertyName);

	if(propertyTree.hasProperty(id))
		return propertyTree.getProperty(Identifier(propertyName));

	if (defaultValues.contains(id))
		return defaultValues[id];

	reportScriptError("Property " + propertyName + " not found.");

	RETURN_IF_NO_THROW(var());
}

bool ScriptingApi::Content::ScriptComponent::isPositionProperty(Identifier id) const
{
	return id == getIdFor(x) || id == getIdFor(y) || id == getIdFor(width) || id == getIdFor(height);
}

void ScriptingApi::Content::ScriptComponent::initInternalPropertyFromValueTreeOrDefault(int id, bool justSetInitFlag/*=false*/)
{
	initialisedProperties.setBit(id, true);

	if (justSetInitFlag)
		return;

	auto id_ = getIdFor(id);

	if (propertyTree.hasProperty(id_))
		setScriptObjectPropertyWithChangeMessage(id_, propertyTree.getProperty(id_), dontSendNotification);
	else
		setScriptObjectPropertyWithChangeMessage(id_, defaultValues[id_], dontSendNotification);
}

String ScriptingApi::Content::ScriptComponent::getParentComponentId() const
{
	static const Identifier id("id");

	return propertyTree.getParent().getProperty(id).toString();
}

bool ScriptingApi::Content::ScriptComponent::hasParentComponent() const
{
	static const Identifier comp("Component");

	return propertyTree.getParent().getType() == comp;
}

ScriptingApi::Content::ScriptComponent* ScriptingApi::Content::ScriptComponent::getParentScriptComponent()
{
	if (!hasParentComponent())
		return nullptr;

	const Identifier id_("id");
	auto id = Identifier(propertyTree.getParent().getProperty(id_).toString());
	return parent->getComponentWithName(id);
}

const ScriptingApi::Content::ScriptComponent* ScriptingApi::Content::ScriptComponent::getParentScriptComponent() const
{
	if (!hasParentComponent())
		return nullptr;

	const Identifier id_("id");
	auto id = Identifier(propertyTree.getParent().getProperty(id_).toString());
	return parent->getComponentWithName(id);
}

var ScriptingApi::Content::ScriptComponent::getChildComponents()
{
		ChildIterator<ScriptComponent> iter(this);
		Array<var> list;
		
		while (auto child = iter.getNextChildComponent())
			if (child != this) list.add(child);
			
		return var(list);
}

ScriptingApi::Content::ScriptComponent::~ScriptComponent()
{
	if (linkedComponent != nullptr)
		linkedComponent->removeLinkedTarget(this);

	if (currentAutomationData != nullptr)
		currentAutomationData->asyncListeners.removeListener(*this);
}

void ScriptingApi::Content::ScriptComponent::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isPluginParameter));
}

void ScriptingApi::Content::ScriptComponent::updatePropertiesAfterLink(NotificationType /*notifyEditor*/)
{
	auto idList = getLinkProperties();

	if (auto lc = getLinkedComponent())
	{
		auto obj = new DynamicObject();
		var obj_(obj);

		for (const auto& v : idList)
		{
			auto id = getIdFor(v.id);
			var linkedValue = v.value.isUndefined() ? lc->getScriptObjectProperty(id) : v.value;

			obj->setProperty(id, linkedValue);
		}

		setPropertiesFromJSON(obj_);

		for (const auto& v : idList)
		{
			auto id = getIdFor(v.id);
			deactivatedProperties.addIfNotAlreadyThere(id);
		}
	}
	else
	{
		for (const auto& v : idList)
		{
			deactivatedProperties.removeAllInstancesOf(getIdFor(v.id));
		}

		handleDefaultDeactivatedProperties();
	}
}

juce::Array<hise::ScriptingApi::Content::ScriptComponent::PropertyWithValue> ScriptingApi::Content::ScriptComponent::getLinkProperties() const
{
	Array<PropertyWithValue> vArray;
	vArray.add({ Properties::min });
    vArray.add({ Properties::max });
		vArray.add({ Properties::saveInPreset, false });
	vArray.add({ Properties::macroControl, -1 });
	vArray.add({ Properties::isPluginParameter, false });
	vArray.add({ Properties::pluginParameterName, "" });
	vArray.add({ Properties::isMetaParameter, false });
	vArray.add({ Properties::processorId, "" });
	vArray.add({ Properties::parameterId, "" });

	return vArray;
}

var ScriptingApi::Content::ScriptComponent::getAllProperties()
{
	Array<var> list;

	for (int i = 0; i < getNumIds(); i++)
	{
		auto id = getIdFor(i);

		if (deactivatedProperties.contains(id))
			continue;

		list.add(id.toString());
	}

	return var(list);
}

void ScriptingApi::Content::ScriptComponent::SubComponentNotifier::handleAsyncUpdate()
{
    Array<Item> items;
    
    {
        hise::SimpleReadWriteLock::ScopedReadLock sl(lock);
        items.swapWith(pendingItems);
    }
    
    for (auto l : parent.subComponentListeners)
    {
        if (l != nullptr)
        {
            for(auto i: items)
            {
                if(i.sc != nullptr)
                {
                    if (i.wasAdded)
                        l->subComponentAdded(i.sc);
                    else
                        l->subComponentRemoved(i.sc);
                }
            }
        }
    }
}

void ScriptingApi::Content::ScriptComponent::sendSubComponentChangeMessage(ScriptComponent* s, bool wasAdded, NotificationType notify/*=sendNotificationAsync*/)
{
    {
        hise::SimpleReadWriteLock::ScopedWriteLock sl(subComponentNotifier.lock);
        subComponentNotifier.pendingItems.add({s, wasAdded});
    }
    
    if(notify == sendNotificationSync)
        subComponentNotifier.handleAsyncUpdate();
    else
        subComponentNotifier.triggerAsyncUpdate();
}


void ScriptingApi::Content::ScriptComponent::setZLevel(String zLevelToUse)
{
	static const StringArray validNames = { "Back", "Default", "Front", "AlwaysOnTop" };

	auto idx = validNames.indexOf(zLevelToUse);

	if (idx == -1)
		reportScriptError("Invalid z-Index: " + zLevelToUse);

	auto newLevel = (ZLevelListener::ZLevel)idx;

	if (newLevel != currentZLevel)
	{
		currentZLevel = newLevel;

		for (auto l : zLevelListeners)
		{
			if (l != nullptr)
				l->zLevelChanged(currentZLevel);
		}
	}
}

var ScriptingApi::Content::ScriptComponent::getLocalBounds(float reduceAmount)
{
	Rectangle<float> ar(0.0f, 0.0f, (float)getScriptObjectProperty(Properties::width), (float)getScriptObjectProperty(Properties::height));
	ar = ar.reduced(reduceAmount);

	Array<var> b;
	b.add(ar.getX()); b.add(ar.getY()); b.add(ar.getWidth()); b.add(ar.getHeight());
	return var(b);
}

void ScriptingApi::Content::ScriptComponent::setKeyPressCallback(var keyboardFunction)
{
	keyboardCallback = WeakCallbackHolder(getScriptProcessor(), this, keyboardFunction, 1);
	keyboardCallback.incRefCount();
	keyboardCallback.setThisObject(this);
}

bool ScriptingApi::Content::ScriptComponent::handleKeyPress(const KeyPress& k)
{
	if (keyboardCallback)
	{
		auto obj = new DynamicObject();
		var args(obj);

		obj->setProperty("isFocusChange", false);

		auto c = k.getTextCharacter();

		auto printable    = CharacterFunctions::isPrintable(c);
		auto isWhitespace = CharacterFunctions::isWhitespace(c);
		auto isLetter     = CharacterFunctions::isLetter(c);
		auto isDigit      = CharacterFunctions::isDigit(c);
		
		obj->setProperty("character", printable ? String::charToString(c) : "");
		obj->setProperty("specialKey", !printable);
		obj->setProperty("isWhitespace", isWhitespace);
		obj->setProperty("isLetter", isLetter);
		obj->setProperty("isDigit", isDigit);
		obj->setProperty("keyCode", k.getKeyCode());
		obj->setProperty("description", k.getTextDescription());
		obj->setProperty("shift", k.getModifiers().isShiftDown());
		obj->setProperty("cmd", k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown());
		obj->setProperty("alt", k.getModifiers().isAltDown());

		var rv;

		auto ok = keyboardCallback.callSync(&args, 1, &rv);

		if (ok.wasOk())
			return (bool)rv;
		else
			reportScriptError(ok.getErrorMessage());
	}

	return false;
}

void ScriptingApi::Content::ScriptComponent::handleFocusChange(bool isFocused)
{
	if (keyboardCallback)
	{
		auto obj = new DynamicObject();
		var args(obj);

		obj->setProperty("isFocusChange", true);
		obj->setProperty("hasFocus", isFocused);
		
		try
		{
			auto ok = keyboardCallback.callSync(&args, 1);

			if (!ok.wasOk())
				reportScriptError(ok.getErrorMessage());
		}
		catch (String& s)
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), s);
		}
		

		
	}
}

void ScriptComponent::addSubComponentListener(SubComponentListener* l)
{
	subComponentListeners.addIfNotAlreadyThere(l);
}

void ScriptComponent::removeSubComponentListener(SubComponentListener* l)
{
	subComponentListeners.removeAllInstancesOf(l);
}

void ScriptingApi::Content::ScriptComponent::loseFocus()
{
	for (auto l : zLevelListeners)
	{
		if (l != nullptr)
			l->wantsToLoseFocus();
	}
}

void ScriptingApi::Content::ScriptComponent::grabFocus()
{
    for (auto l : zLevelListeners)
    {
        if (l != nullptr)
        {
            l->wantsToGrabFocus();
            
            return; // this is a exclusive operation so we don't
                    // need to continue the loop
        }
    }
}

void ScriptComponent::handleScriptPropertyChange(const Identifier& id)
{
	if (countJsonSetProperties)
	{
		if (searchedProperty.isNull())
		{
			scriptChangedProperties.addIfNotAlreadyThere(id);
		}
		if (searchedProperty == id)
		{
			throw String("Here...");
		}
	}
}

juce::LookAndFeel* ScriptingApi::Content::ScriptComponent::createLocalLookAndFeel()
{
	if (auto l = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(localLookAndFeel.getObject()))
	{
		return new ScriptingObjects::ScriptedLookAndFeel::LocalLaf(l);
	}

	return nullptr;
}

void ScriptingApi::Content::ScriptComponent::setLocalLookAndFeel(var lafObject)
{
	if (auto l = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(lafObject.getObject()))
	{
		localLookAndFeel = lafObject;

		ChildIterator<ScriptComponent> iter(this);

		while (auto sc = iter.getNextChildComponent())
			sc->localLookAndFeel = lafObject;
	}
	else
		localLookAndFeel = var();
}

void ScriptingApi::Content::ScriptComponent::sendGlobalCableValue(var v)
{
	auto value = (float)v;
	FloatSanitizers::sanitizeFloatNumber(value);

	jassert(globalConnection != nullptr);

	globalConnection->call((double)value);
}

bool ScriptingApi::Content::ScriptComponent::isConnectedToGlobalCable() const
{
	return globalConnection != nullptr && globalConnection->cable != nullptr;
}

ScriptComponent::ScopedPropertyEnabler::ScopedPropertyEnabler(ScriptComponent* c_):
	c(c_)
{
	c->countJsonSetProperties = false;
}

ScriptComponent::ScopedPropertyEnabler::~ScopedPropertyEnabler()
{
	c->countJsonSetProperties = true;
	c = nullptr;
}

void ScriptingApi::Content::ScriptComponent::repaintThisAndAllChildren()
{
	// Call repaint on all children to make sure they are updated...
	ChildIterator<ScriptPanel> iter(this);

	while (auto childPanel = iter.getNextChildComponent())
		childPanel->repaint();
}

void ScriptingApi::Content::ScriptComponent::sendRepaintMessage()
{
	repaintBroadcaster.sendMessage(sendNotificationAsync, true);
}

String ScriptingApi::Content::ScriptComponent::getId() const
{
	return getName().toString();
}



void ScriptingApi::Content::ScriptComponent::fadeComponent(bool shouldBeVisible, int milliseconds)
{
	auto isVisible = (bool)getScriptObjectProperty(getIdFor(Properties::visible));

	if (shouldBeVisible != isVisible)
	{
        setScriptObjectPropertyWithChangeMessage(getIdFor(Properties::visible), shouldBeVisible);

		fadeListener.enableLockFreeUpdate(getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
		fadeListener.sendMessage(sendNotificationAsync, shouldBeVisible, milliseconds);
	}
}

juce::var ScriptingApi::Content::ScriptComponent::getLookAndFeelObject()
{
	return localLookAndFeel;
}

void ScriptComponent::attachValueListener(WeakCallbackHolder::CallableObject* obj)
{
	valueListener = obj;
	sendValueListenerMessage();
}

void ScriptComponent::removeMouseListener(WeakCallbackHolder::CallableObject* obj)
{
	for (int i = 0; i < mouseListeners.size(); i++)
	{
		if (mouseListeners[i].listener == obj)
			mouseListeners.remove(i--);
	}
}

void ScriptComponent::attachMouseListener(WeakCallbackHolder::CallableObject* obj,
	MouseCallbackComponent::CallbackLevel cl, const MouseListenerData::StateFunction& sf,
	const MouseListenerData::StateFunction& ef, const MouseListenerData::StateFunction& tf,
	const std::function<StringArray()>& popupItemFunction, ModifierKeys pm, int delayMs)
{
	for (int i = 0; i < mouseListeners.size(); i++)
	{
		if (mouseListeners[i].listener == nullptr)
			mouseListeners.remove(i--);
	}

	mouseListeners.add({ obj, cl, sf, ef, tf, popupItemFunction, pm, delayMs });
}

struct ScriptingApi::Content::ScriptSlider::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setValuePopupFunction);
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setMidPoint);
	API_VOID_METHOD_WRAPPER_3(ScriptSlider, setRange);
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setMode);
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setStyle);
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setMinValue);
	API_VOID_METHOD_WRAPPER_1(ScriptSlider, setMaxValue);
	API_METHOD_WRAPPER_0(ScriptSlider, getMinValue);
	API_METHOD_WRAPPER_0(ScriptSlider, getMaxValue);
	API_METHOD_WRAPPER_1(ScriptSlider, contains);
};

ScriptingApi::Content::ScriptSlider::ScriptSlider(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name_, int x, int y, int, int) :
ScriptComponent(base, name_),
styleId(Slider::SliderStyle::RotaryHorizontalVerticalDrag),
m(HiSlider::Mode::Linear),
minimum(0.0f),
maximum(1.0f)
{
	CHECK_COPY_AND_RETURN_22(dynamic_cast<Processor*>(base));

	ADD_SCRIPT_PROPERTY(i01, "mode");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i02, "style");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i03, "stepSize");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i04, "middlePosition");
	ADD_SCRIPT_PROPERTY(i06, "suffix");
	ADD_SCRIPT_PROPERTY(i07, "filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_NUMBER_PROPERTY(i08, "numStrips");		
	ADD_SCRIPT_PROPERTY(i09, "isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_NUMBER_PROPERTY(i10, "scaleFactor");	
	ADD_NUMBER_PROPERTY(i11, "mouseSensitivity");
	ADD_SCRIPT_PROPERTY(i12, "dragDirection");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i13, "showValuePopup"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i14, "showTextBox"); 	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i15, "scrollWheel"); 	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i16, "enableMidiLearn"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

#if 0
	componentProperties->setProperty(getIdFor(Mode), 0);
	componentProperties->setProperty(getIdFor(Style), 0);
	componentProperties->setProperty(getIdFor(stepSize), 0);
	componentProperties->setProperty(getIdFor(middlePosition), 0);
	componentProperties->setProperty(getIdFor(defaultValue), 0);
	componentProperties->setProperty(getIdFor(suffix), 0);
	componentProperties->setProperty(getIdFor(filmstripImage), String());
	componentProperties->setProperty(getIdFor(numStrips), 0);
	componentProperties->setProperty(getIdFor(isVertical), true);
	componentProperties->setProperty(getIdFor(mouseSensitivity), 1.0);
	componentProperties->setProperty(getIdFor(dragDirection), 0);
	componentProperties->setProperty(getIdFor(showValuePopup), 0);
#endif

	priorityProperties.add(getIdFor(Mode));

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 128);
	setDefaultValue(ScriptComponent::Properties::height, 48);
	setDefaultValue(ScriptSlider::Properties::Mode, "Linear");
	setDefaultValue(ScriptSlider::Properties::Style, "Knob");
	setDefaultValue(ScriptSlider::Properties::middlePosition, -1.0);
	setDefaultValue(ScriptSlider::Properties::stepSize, 0.01);
	setDefaultValue(ScriptComponent::min, 0.0);
	setDefaultValue(ScriptComponent::max, 1.0);
	setDefaultValue(ScriptComponent::defaultValue, 0.0);
	setDefaultValue(ScriptSlider::Properties::suffix, "");
	setDefaultValue(ScriptSlider::Properties::filmstripImage, "Use default skin");
	setDefaultValue(ScriptSlider::Properties::numStrips, 0);
	setDefaultValue(ScriptSlider::Properties::isVertical, true);
	setDefaultValue(ScriptSlider::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptSlider::Properties::mouseSensitivity, 1.0f);
	setDefaultValue(ScriptSlider::Properties::dragDirection, "Diagonal");
	setDefaultValue(ScriptSlider::Properties::showValuePopup, "No");
	setDefaultValue(ScriptSlider::Properties::showTextBox, true);
	setDefaultValue(ScriptSlider::Properties::scrollWheel, true);
	setDefaultValue(ScriptSlider::Properties::enableMidiLearn, true);

	ScopedValueSetter<bool> svs(removePropertyIfDefault, false);

	const bool dontUpdateMode = !getPropertyValueTree().hasProperty(getIdFor(Mode));

	initInternalPropertyFromValueTreeOrDefault(Properties::Mode, dontUpdateMode);
	
	initInternalPropertyFromValueTreeOrDefault(Properties::Style);
	initInternalPropertyFromValueTreeOrDefault(Properties::middlePosition);
	initInternalPropertyFromValueTreeOrDefault(Properties::stepSize);
	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::min);
	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::max);
	initInternalPropertyFromValueTreeOrDefault(Properties::suffix);
	initInternalPropertyFromValueTreeOrDefault(Properties::filmstripImage);
	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::linkedTo);

	ADD_API_METHOD_1(setValuePopupFunction);
	ADD_API_METHOD_1(setMidPoint);
	ADD_API_METHOD_3(setRange);
	ADD_API_METHOD_1(setMode);
	ADD_API_METHOD_1(setStyle);
	ADD_API_METHOD_1(setMinValue);
	ADD_API_METHOD_1(setMaxValue);
	ADD_API_METHOD_0(getMinValue);
	ADD_API_METHOD_0(getMaxValue);
	ADD_API_METHOD_1(contains);

	//addConstant("Decibel", HiSlider::Mode::Decibel);
	//addConstant("Discrete", HiSlider::Mode::Discrete);
	//addConstant("Frequency", HiSlider::Mode::Frequency);
}

ScriptingApi::Content::ScriptSlider::~ScriptSlider()
{
	image.clear();
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptSlider::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::SliderWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptSlider::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	jassert(propertyIds.contains(id));

	if (id == Identifier("mode"))
	{
		jassert(isCorrectlyInitialised(id));

		setMode(newValue.toString());
		return;
	}
	else if (id == propertyIds[Style])
	{
		jassert(isCorrectlyInitialised(id));

		setStyle(newValue);
		return;
	}
	else if (id == getIdFor(middlePosition))
	{
		jassert(isCorrectlyInitialised(id));

		setMidPoint(newValue);
		return;
	}
    else if (id == getIdFor(defaultValue))
    {
        float v = (float)jlimit((double)getScriptObjectProperty(ScriptComponent::Properties::min),
                                (double)getScriptObjectProperty(ScriptComponent::Properties::max),
                                (double)newValue);
        
        v = FloatSanitizers::sanitizeFloatNumber(v);
        
        setScriptObjectProperty(defaultValue, var(v));
        
        
        return;
    }



	else if (id == getIdFor(filmstripImage))
	{
		jassert(isCorrectlyInitialised(id));

		
		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "Use default skin");

			image.clear();
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);

			PoolReference ref(getProcessor()->getMainController(), newValue.toString(), ProjectHandler::SubDirectories::Images);

			image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
		}

		return;
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

ValueTree ScriptingApi::Content::ScriptSlider::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	if (getScriptObjectProperty(Properties::Style) == "Range")
	{
		v.setProperty("rangeMin", minimum, nullptr);
		v.setProperty("rangeMax", maximum, nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptSlider::restoreFromValueTree(const ValueTree &v)
{
	ScriptComponent::restoreFromValueTree(v);
	
	minimum = v.getProperty("rangeMin", 0.0f);
	maximum = v.getProperty("rangeMax", 1.0f);
}

void ScriptingApi::Content::ScriptSlider::resetValueToDefault()
{
	auto f = (float)getScriptObjectProperty(ScriptComponent::defaultValue);
	FloatSanitizers::sanitizeFloatNumber(f);
	setValue(f);
}

StringArray ScriptingApi::Content::ScriptSlider::getOptionsFor(const Identifier &id)
{
	const int index = propertyIds.indexOf(id);

	StringArray sa;

	switch (index)
	{
	case Properties::Mode:
		sa.add("Frequency");
		sa.add("Decibel");
		sa.add("Time");
		sa.add("TempoSync");
		sa.add("Linear");
		sa.add("Discrete");
		sa.add("Pan");
		sa.add("NormalizedPercentage");
		break;
	case Properties::Style:	sa.add("Knob");
		sa.add("Horizontal");
		sa.add("Vertical");
		sa.add("Range");
		break;
	case Properties::stepSize:
		sa.add("0.01");
		sa.add("0.1");
		sa.add("1.0");
		break;
	case filmstripImage:
		sa.add("Load new File");
		sa.add("Use default skin");
		sa.addArray(getProcessor()->getMainController()->getCurrentImagePool()->getIdList());
		break;
	case dragDirection:
		sa.add("Diagonal");
		sa.add("Vertical");
		sa.add("Horizontal");
		break;
	case showValuePopup:
		sa.add("No");
		sa.add("Above");
		sa.add("Below");
		sa.add("Left");
		sa.add("Right");
		break;
	default:				sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

void ScriptingApi::Content::ScriptSlider::setValuePopupFunction(var newFunction)
{
	sliderValueFunction = newFunction;
	getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}

void ScriptingApi::Content::ScriptSlider::setMidPoint(double valueForMidPoint)
{
	if (valueForMidPoint == -1.0f)
	{
		setScriptObjectProperty(middlePosition, valueForMidPoint);
		return;
		//valueForMidPoint = range.getStart() + (range.getEnd() - range.getStart()) / 2.0;
	}

	Range<double> range = Range<double>(getScriptObjectProperty(ScriptComponent::Properties::min), getScriptObjectProperty(ScriptComponent::Properties::max));

	const bool illegalMidPoint = valueForMidPoint == range.getStart() || !range.contains(valueForMidPoint);
	if (illegalMidPoint)
	{
		debugError(parent->getProcessor(), "setMidPoint() value must be in the knob range.");
		valueForMidPoint = (range.getEnd() - range.getStart()) / 2.0 + range.getStart();
	}

	CHECK_COPY_AND_RETURN_11(getProcessor());

	setScriptObjectProperty(middlePosition, valueForMidPoint);
}

void ScriptingApi::Content::ScriptSlider::setStyle(String style)
{
	if (style == "Knob") styleId = Slider::SliderStyle::RotaryHorizontalVerticalDrag;
	else if (style == "Horizontal") styleId = Slider::SliderStyle::LinearBar;
	else if (style == "Vertical") styleId = Slider::SliderStyle::LinearBarVertical;
	else if (style == "Range") styleId = Slider::SliderStyle::TwoValueHorizontal;

	setScriptObjectProperty(Properties::Style, style, sendNotification);
}

void ScriptingApi::Content::ScriptSlider::setMinValue(double min) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		minimum = min;
		triggerAsyncUpdate();
	}
	else
	{
		logErrorAndContinue("setMinValue() can only be called on sliders in 'Range' mode.");
	}
}

void ScriptingApi::Content::ScriptSlider::setMaxValue(double max) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		maximum = max;
		triggerAsyncUpdate();
	}
	else
	{
		logErrorAndContinue("setMaxValue() can only be called on sliders in 'Range' mode.");
	}
}

double ScriptingApi::Content::ScriptSlider::getMinValue() const
{

	if (styleId == Slider::TwoValueHorizontal)
	{
		return minimum;
	}
	else
	{
		logErrorAndContinue("getMinValue() can only be called on sliders in 'Range' mode.");
		return 0.0;
	}
}

double ScriptingApi::Content::ScriptSlider::getMaxValue() const
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		return maximum;
	}
	else
	{
		logErrorAndContinue("getMaxValue() can only be called on sliders in 'Range' mode.");
		return 1.0;
	}
}

bool ScriptingApi::Content::ScriptSlider::contains(double valueToCheck)
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		return minimum <= valueToCheck && maximum >= valueToCheck;
	}
	else
	{
		logErrorAndContinue("contains() can only be called on sliders in 'Range' mode.");
		return false;
	}
}


void ScriptingApi::Content::ScriptSlider::setValueNormalized(double normalizedValue)
{
	const double minValue = getScriptObjectProperty(min);
	const double maxValue = getScriptObjectProperty(max);
	const double midPoint = getScriptObjectProperty(middlePosition);
	const double step = getScriptObjectProperty(stepSize);

	if (minValue < maxValue &&
		midPoint > minValue &&
		midPoint < maxValue &&
		step > 0.0)
	{
		const double skew = log(0.5) / log((midPoint - minValue) / (maxValue - minValue));

		NormalisableRange<double> range = NormalisableRange<double>(minValue, maxValue, step, skew);

		const double actualValue = range.convertFrom0to1(normalizedValue);

		setValue(actualValue);
	}
	else
	{


#if USE_BACKEND
		String errorMessage;

		errorMessage << "Slider range of " << getName().toString() << " is illegal: min: " << minValue << ", max: " << maxValue << ", middlePoint: " << midPoint << ", step: " << step;

		logErrorAndContinue(errorMessage);
#endif
	}
}

double ScriptingApi::Content::ScriptSlider::getValueNormalized() const
{
	const double minValue = getScriptObjectProperty(min);
	const double maxValue = getScriptObjectProperty(max);
	double midPoint = getScriptObjectProperty(middlePosition);
	const double step = getScriptObjectProperty(stepSize);

	Range<double> r(minValue, maxValue);

	if (!r.contains(midPoint))
		midPoint = r.getStart() + 0.5 * r.getLength();
		

	if (minValue < maxValue &&
		midPoint > minValue &&
		midPoint < maxValue &&
		step > 0.0)
	{
		const double skew = log(0.5) / log((midPoint - minValue) / (maxValue - minValue));

		NormalisableRange<double> range = NormalisableRange<double>(minValue, maxValue, step, skew);

		const double unNormalizedValue = getValue();

		return range.convertTo0to1(unNormalizedValue);
	}
	else
	{


#if USE_BACKEND
		String errorMessage;

		errorMessage << "Slider range of " << getName().toString() << " is illegal: min: " << minValue << ", max: " << maxValue << ", middlePoint: " << midPoint << ", step: " << step;

		logErrorAndContinue(errorMessage);
#endif

		return 0.0;
	}
}

void ScriptingApi::Content::ScriptSlider::setRange(double min_, double max_, double stepSize_)
{
	setScriptObjectProperty(ScriptComponent::Properties::min, var(min_), dontSendNotification);
	setScriptObjectProperty(ScriptComponent::Properties::max, var(max_), dontSendNotification);
	setScriptObjectProperty(Properties::stepSize, stepSize_, sendNotification);
}

void ScriptingApi::Content::ScriptSlider::setMode(String mode)
{
	StringArray sa = getOptionsFor(getIdFor(Mode));

	const int index = sa.indexOf(mode);

	if (index == -1)
	{
        m = HiSlider::Mode::Linear;
		//logErrorAndContinue("invalid slider mode: " + mode);
		return;
	}

	m = (HiSlider::Mode) index;

	auto currentModeName = getScriptObjectProperty(ScriptSlider::Mode).toString();
	auto currentMode = sa.indexOf(currentModeName);
	
	auto currentModeDefaultRange = HiSlider::getRangeForMode((HiSlider::Mode)currentMode);

	
	const bool sameStart = currentModeDefaultRange.start == (double)getScriptObjectProperty(ScriptComponent::Properties::min);
	const bool sameEnd = currentModeDefaultRange.end == (double)getScriptObjectProperty(ScriptComponent::Properties::max);
	
	const bool sameStep = currentModeDefaultRange.interval == (double)getScriptObjectProperty(ScriptSlider::Properties::stepSize);
	
	auto skew1 = HiSlider::getMidPointFromRangeSkewFactor(currentModeDefaultRange);
	auto skew2 = (double)getScriptObjectProperty(ScriptSlider::Properties::middlePosition);

	const bool sameSkew = (skew2 == -1.0) || skew1 == skew2;

	bool isUsingDefaultRange = sameStart && sameEnd && sameStep && sameSkew;

	auto nr = HiSlider::getRangeForMode(m);

	setScriptObjectProperty(Mode, mode, sendNotification);

	if (isUsingDefaultRange && (nr.end - nr.start) != 0)
	{
		setScriptObjectProperty(ScriptComponent::Properties::min, nr.start);
		setScriptObjectProperty(ScriptComponent::Properties::max, nr.end);
		setScriptObjectProperty(stepSize, nr.interval);
		setScriptObjectProperty(ScriptSlider::Properties::suffix, HiSlider::getSuffixForMode(m, getValue()));

		setMidPoint(HiSlider::getMidPointFromRangeSkewFactor(nr));


		//setMidPoint(getScriptObjectProperty(ScriptSlider::Properties::middlePosition));
	}
}

void ScriptingApi::Content::ScriptSlider::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.removeAllInstancesOf(getIdFor(isPluginParameter));
}

juce::Array<hise::ScriptingApi::Content::ScriptComponent::PropertyWithValue> ScriptingApi::Content::ScriptSlider::getLinkProperties() const
{
	auto idList = ScriptComponent::getLinkProperties();

	idList.insert(0, { Properties::Mode });
	idList.add({ Properties::middlePosition });
	idList.add({ Properties::stepSize });
	idList.add({ Properties::suffix });
	idList.add({ ScriptComponent::defaultValue });

	return idList;
}

struct ScriptingApi::Content::ScriptButton::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptButton, setPopupData);
};

ScriptingApi::Content::ScriptButton::ScriptButton(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name, int x, int y, int, int) :
ScriptComponent(base, name)
{
	ADD_SCRIPT_PROPERTY(i00, "filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_NUMBER_PROPERTY(i01, "numStrips");		
	ADD_SCRIPT_PROPERTY(i02, "isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_NUMBER_PROPERTY(i03, "scaleFactor");
	ADD_NUMBER_PROPERTY(i05, "radioGroup");
	ADD_SCRIPT_PROPERTY(i04, "isMomentary");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i06, "enableMidiLearn"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i07, "setValueOnClick"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	handleDefaultDeactivatedProperties();

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 128);
	setDefaultValue(ScriptComponent::Properties::height, 28);
	setDefaultValue(ScriptButton::Properties::filmstripImage, "");
	setDefaultValue(ScriptButton::Properties::numStrips, "2");
	setDefaultValue(ScriptButton::Properties::isVertical, true);
	setDefaultValue(ScriptButton::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptButton::Properties::radioGroup, 0);
	setDefaultValue(ScriptButton::Properties::isMomentary, 0);
	setDefaultValue(ScriptButton::Properties::enableMidiLearn, true);
    setDefaultValue(ScriptButton::Properties::setValueOnClick, false);

	initInternalPropertyFromValueTreeOrDefault(filmstripImage);

	ADD_API_METHOD_2(setPopupData);
}

ScriptingApi::Content::ScriptButton::~ScriptButton()
{
	image.clear();
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptButton::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ButtonWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptButton::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(filmstripImage))
	{
		jassert(isCorrectlyInitialised(id));

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "");
			image.clear();
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);

			PoolReference ref(getProcessor()->getMainController(), newValue.toString(), ProjectHandler::SubDirectories::Images);

			image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
		}
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptButton::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(filmstripImage))
	{
		StringArray sa;

		sa.add("Load new File");

		sa.add("Use default skin");
		sa.addArray(getProcessor()->getMainController()->getCurrentImagePool()->getIdList());

		return sa;
	}

	return ScriptComponent::getOptionsFor(id);
}

void ScriptingApi::Content::ScriptButton::setPopupData(var jsonData, var position)
{
	popupData = jsonData;

	Result r = Result::ok();

	popupPosition = ApiHelpers::getIntRectangleFromVar(position, &r);

	if (r.failed())
	{
		throw String("position must be an array with this structure: [x, y, w, h]");
	}
}

void ScriptingApi::Content::ScriptButton::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));
}


struct ScriptingApi::Content::ScriptLabel::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptLabel, setEditable);
};

ScriptingApi::Content::ScriptLabel::ScriptLabel(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name, int x, int y, int , int) :
ScriptComponent(base, name)
{
	ADD_SCRIPT_PROPERTY(i01, "fontName");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i02, "fontSize");	ADD_AS_SLIDER_TYPE(1, 200, 1);
	ADD_SCRIPT_PROPERTY(i03, "fontStyle");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "alignment");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "editable");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i06, "multiline");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i07, "updateEachKey"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    
	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 128);
	setDefaultValue(ScriptComponent::Properties::height, 28);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(text, name.toString());
	setDefaultValue(bgColour, (int64)0x00000000);
	setDefaultValue(itemColour, (int64)0x00000000);
	setDefaultValue(textColour, (int64)0xffffffff);
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Arial");
	setDefaultValue(Alignment, "centred");
	setDefaultValue(Editable, true);
	setDefaultValue(Multiline, false);
    setDefaultValue(SendValueEachKeyPress, false);

	handleDefaultDeactivatedProperties();

	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::Properties::text);

	value = var("internal");

	ADD_API_METHOD_1(setEditable);
}

ScriptingApi::Content::ScriptLabel::~ScriptLabel()
{}

Identifier ScriptingApi::Content::ScriptLabel::getStaticObjectName()
{ RETURN_STATIC_IDENTIFIER("ScriptLabel"); }

Identifier ScriptingApi::Content::ScriptLabel::getObjectName() const
{ return getStaticObjectName(); }

var ScriptingApi::Content::ScriptLabel::getValue() const
{
	return getScriptObjectProperty(ScriptComponent::Properties::text);
}

void ScriptingApi::Content::ScriptLabel::setValue(var newValue)
{
	jassert(newValue != "internal");

	if (newValue.isString())
	{
		setScriptObjectProperty(ScriptComponent::Properties::text, newValue);
		triggerAsyncUpdate();
	}
}

void ScriptingApi::Content::ScriptLabel::resetValueToDefault()
{
	setValue("");
}

void ScriptingApi::Content::ScriptLabel::setScriptObjectPropertyWithChangeMessage(const Identifier& id, var newValue,
	NotificationType notifyEditor)
{
	if (id == getIdFor((int)ScriptComponent::Properties::text))
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::text));
		setValue(newValue.toString());
	}
			
	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

}

bool ScriptingApi::Content::ScriptLabel::isClickable() const
{
	return getScriptObjectProperty(Editable) && ScriptComponent::isClickable();
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptLabel::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::LabelWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptLabel::setEditable(bool shouldBeEditable)
{
	if (!parent->allowGuiCreation)
	{
		reportScriptError("the editable state of a label can't be changed after onInit()");
		return;
	}

	setScriptObjectProperty(Editable, shouldBeEditable);
}

StringArray ScriptingApi::Content::ScriptLabel::getOptionsFor(const Identifier &id)
{
	StringArray sa;

	int index = propertyIds.indexOf(id);

	Font f("Arial", 13.0f, Font::plain);

	switch (index)
	{
	case FontStyle:	
		sa.addArray(f.getAvailableStyles());
		sa.add("Password");
		break;
	case FontName:	sa.add("Default");
		sa.add("Oxygen");
		sa.add("Source Code Pro");
		getScriptProcessor()->getMainController_()->fillWithCustomFonts(sa);
		sa.addArray(Font::findAllTypefaceNames());
		break;
	case Alignment: sa = ApiHelpers::getJustificationNames();
		break;
	default:		sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

Justification ScriptingApi::Content::ScriptLabel::getJustification()
{
	auto justAsString = getScriptObjectProperty(Alignment);

	return ApiHelpers::getJustification(justAsString);
}

void ScriptingApi::Content::ScriptLabel::restoreFromValueTree(const ValueTree& v)
{
	setValue(v.getProperty("value", ""));
}

ValueTree ScriptingApi::Content::ScriptLabel::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	v.setProperty("value", getValue(), nullptr);
			
	return v;
}

void ScriptingApi::Content::ScriptLabel::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
}

struct ScriptingApi::Content::ScriptComboBox::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptComboBox, addItem);
	API_METHOD_WRAPPER_0(ScriptComboBox, getItemText);
};

ScriptingApi::Content::ScriptComboBox::ScriptComboBox(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name, int x, int y, int , int) :
ScriptComponent(base, name)
{
	propertyIds.add(Identifier("items"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);

	ADD_SCRIPT_PROPERTY(i01, "fontName");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i02, "fontSize");	ADD_AS_SLIDER_TYPE(1, 200, 1);
	ADD_SCRIPT_PROPERTY(i03, "fontStyle");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "enableMidiLearn"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i05, "popupAlignment"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	priorityProperties.add(getIdFor(Items));

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 128);
	setDefaultValue(ScriptComponent::Properties::height, 32);
	setDefaultValue(Items, "");
    setDefaultValue(ScriptComboBox::Properties::popupAlignment, "bottom");
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Default");
	setDefaultValue(ScriptComponent::Properties::defaultValue, 1);
	setDefaultValue(ScriptComponent::min, 1.0f);
	setDefaultValue(ScriptComboBox::Properties::enableMidiLearn, false);
	
	handleDefaultDeactivatedProperties();
	initInternalPropertyFromValueTreeOrDefault(Items);

	ADD_API_METHOD_1(addItem);
	ADD_API_METHOD_0(getItemText);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptComboBox::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ComboBoxWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptComboBox::addItem(const String &itemName)
{
	String newItemList = getScriptObjectProperty(Items);

	newItemList.append("\n", 1);
	newItemList.append(itemName, 128);

	setScriptObjectProperty(Items, newItemList, sendNotification);

	int size = getScriptObjectProperty(max);

	setScriptObjectProperty(ScriptComponent::Properties::min, 1, dontSendNotification);
	setScriptObjectProperty(ScriptComponent::Properties::max, size + 1, dontSendNotification);
}

String ScriptingApi::Content::ScriptComboBox::getItemText() const
{
	StringArray items = getItemList();

    if(isPositiveAndBelow((int)value, (items.size()+1)))
    {
        return items[(int)value - 1];
    }
    
    return "No options";
}

void ScriptingApi::Content::ScriptComboBox::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(Items))
	{
		jassert(isCorrectlyInitialised(Items));

		setScriptObjectProperty(Items, newValue, sendNotification);
		setScriptObjectProperty(max, getItemList().size(), sendNotification);
	}


	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptComboBox::getItemList() const
{
	String items = getScriptObjectProperty(Items).toString();

	if (items.isEmpty()) return StringArray();

	StringArray sa;

	sa.addTokens(items, "\n", "");

	sa.removeEmptyStrings();

	return sa;
}

void ScriptingApi::Content::ScriptComboBox::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));
}

juce::Array<hise::ScriptingApi::Content::ScriptComponent::PropertyWithValue> ScriptingApi::Content::ScriptComboBox::getLinkProperties() const
{
	auto vArray = ScriptComponent::getLinkProperties();

	vArray.add({ Properties::Items });

	return vArray;
}

juce::StringArray ScriptingApi::Content::ScriptComboBox::getOptionsFor(const Identifier &id)
{
	StringArray sa;

	int index = propertyIds.indexOf(id);

	Font f("Arial", 13.0f, Font::plain);

	switch (index)
	{
	case FontStyle:	sa.addArray(f.getAvailableStyles());
		break;
	case FontName:	sa.add("Default");
		sa.add("Oxygen");
		sa.add("Source Code Pro");
		getScriptProcessor()->getMainController_()->fillWithCustomFonts(sa);
		sa.addArray(Font::findAllTypefaceNames());
		break;
    case popupAlignment:
        sa.add("bottom");
        sa.add("top");
        sa.add("topRight");
        sa.add("bottomRight");
        break;
	default:	sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}


ScriptingApi::Content::ComplexDataScriptComponent::ComplexDataScriptComponent(ProcessorWithScriptingContent* base, Identifier name, snex::ExternalData::DataType type_) :
	ScriptComponent(base, name),
	type(type_)
{
	ownedObject = snex::ExternalData::create(type);
	ownedObject->setGlobalUIUpdater(base->getMainController_()->getGlobalUIUpdater());
	ownedObject->setUndoManager(base->getMainController_()->getControlUndoManager());
}

Table* ScriptingApi::Content::ComplexDataScriptComponent::getTable(int)
{
	return static_cast<Table*>(getUsedData(snex::ExternalData::DataType::Table));
}

SliderPackData* ScriptingApi::Content::ComplexDataScriptComponent::getSliderPack(int)
{
	return static_cast<SliderPackData*>(getUsedData(snex::ExternalData::DataType::SliderPack));
}

MultiChannelAudioBuffer* ScriptingApi::Content::ComplexDataScriptComponent::getAudioFile(int)
{
	return static_cast<MultiChannelAudioBuffer*>(getUsedData(snex::ExternalData::DataType::AudioFile));
}

FilterDataObject* ScriptingApi::Content::ComplexDataScriptComponent::getFilterData(int index)
{
	jassertfalse;
	return nullptr; // soon;
}

SimpleRingBuffer* ScriptingApi::Content::ComplexDataScriptComponent::getDisplayBuffer(int index)
{
	jassertfalse;
	return nullptr; // soon
}

int ScriptingApi::Content::ComplexDataScriptComponent::getNumDataObjects(ExternalData::DataType t) const
{
	return t == type ? 1 : 0;
}

bool ScriptingApi::Content::ComplexDataScriptComponent::removeDataObject(ExternalData::DataType t, int index)
{ return true; }

void ScriptingApi::Content::ComplexDataScriptComponent::setScriptObjectPropertyWithChangeMessage(const Identifier& id,
	var newValue, NotificationType notifyEditor)
{
	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

	if (getIdFor(processorId) == id || getIdFor(getIndexPropertyId()) == id)
	{
		updateCachedObjectReference();
	}

	if (getIdFor(parameterId) == id)
	{
		// don't do anything if you try to connect a table to a parameter...
		return;
	}
}

void ScriptingApi::Content::ComplexDataScriptComponent::referToDataBase(var newData)
{
	if (auto td = dynamic_cast<ScriptingObjects::ScriptComplexDataReferenceBase*>(newData.getObject()))
	{
		if (td->getDataType() != type)
			reportScriptError("Data Type mismatch");

		otherHolder = td->getHolder();

		setScriptObjectPropertyWithChangeMessage(getIdFor(getIndexPropertyId()), td->getIndex(), sendNotification);
		updateCachedObjectReference();
	}
	else if (auto cd = dynamic_cast<ComplexDataScriptComponent*>(newData.getObject()))
	{
		if (cd->type != type)
			reportScriptError("Data Type mismatch");

		otherHolder = cd;
		updateCachedObjectReference();
	}
	else if ((newData.isInt() || newData.isInt64()) && (int)newData == -1)
	{
		// just go back to its own data object
		otherHolder = nullptr;
		updateCachedObjectReference();
	}
}

ComplexDataUIBase* ScriptingApi::Content::ComplexDataScriptComponent::getCachedDataObject() const
{ return cachedObjectReference.get(); }

void ScriptingApi::Content::ComplexDataScriptComponent::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t,
	var data)
{
			
}

ComplexDataUIBase::SourceWatcher& ScriptingApi::Content::ComplexDataScriptComponent::getSourceWatcher()
{ return sourceWatcher; }

void ScriptingApi::Content::ComplexDataScriptComponent::updateCachedObjectReference()
{
	if (cachedObjectReference != nullptr)
		cachedObjectReference->getUpdater().removeEventListener(this);

	cachedObjectReference = getComplexBaseType(type, 0);

	if (cachedObjectReference != nullptr)
		cachedObjectReference->getUpdater().addEventListener(this);

	sourceWatcher.setNewSource(cachedObjectReference);
}

juce::StringArray ScriptingApi::Content::ComplexDataScriptComponent::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ScriptComponent::Properties::processorId))
		return ScriptComponent::getOptionsFor(id);

	auto parentSynth = ProcessorHelpers::findParentProcessor(dynamic_cast<Processor*>(getScriptProcessor()), true);

	return ProcessorHelpers::getAllIdsForDataType(parentSynth, type);
}

hise::ComplexDataUIBase* ScriptingApi::Content::ComplexDataScriptComponent::getUsedData(snex::ExternalData::DataType requiredType)
{
	if (type != requiredType)
		return nullptr;

	if (auto eh = getExternalHolder())
	{
		auto externalIndex = (int)getScriptObjectProperty(getIndexPropertyId());
		cachedObjectReference = eh->getComplexBaseType(type, externalIndex);
	}
	else
		cachedObjectReference = ownedObject.get();

	return cachedObjectReference;
}

snex::ExternalDataHolder* ScriptingApi::Content::ComplexDataScriptComponent::getExternalHolder()
{
	if (otherHolder != nullptr)
		return otherHolder;

	if (auto eh = dynamic_cast<ExternalDataHolder*>(getConnectedProcessor()))
		return eh;

	return nullptr;
}

juce::ValueTree ScriptingApi::Content::ComplexDataScriptComponent::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	if (cachedObjectReference != nullptr)
		v.setProperty("data", cachedObjectReference->toBase64String(), nullptr);

	return v;
}

void ScriptingApi::Content::ComplexDataScriptComponent::restoreFromValueTree(const ValueTree &v)
{
	ScriptComponent::restoreFromValueTree(v);

	if (cachedObjectReference != nullptr)
		cachedObjectReference->fromBase64String(v.getProperty("data", String()));
}

void ScriptingApi::Content::ComplexDataScriptComponent::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(parameterId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isPluginParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
}

var ScriptingApi::Content::ComplexDataScriptComponent::registerComplexDataObjectAtParent(int index /*= -1*/)
{
	if (auto d = dynamic_cast<ProcessorWithDynamicExternalData*>(getScriptProcessor()))
	{
		otherHolder = d;
		d->registerExternalObject(type, index, ownedObject.get());

		setScriptObjectProperty(getIndexPropertyId(), index, sendNotification);
		updateCachedObjectReference();

		switch (type)
		{
		case ExternalData::DataType::Table: return new ScriptingObjects::ScriptTableData(getScriptProcessor(), index);
		case ExternalData::DataType::SliderPack: return new ScriptingObjects::ScriptSliderPackData(getScriptProcessor(), index);
		case ExternalData::DataType::AudioFile: return new ScriptingObjects::ScriptAudioFile(getScriptProcessor(), index);
		default: jassertfalse; return var();
		}
	}

	return var();
}

void ScriptingApi::Content::ScriptAudioWaveform::handleDefaultDeactivatedProperties()
{
	ComplexDataScriptComponent::handleDefaultDeactivatedProperties();

	deactivatedProperties.addIfNotAlreadyThere(getIdFor(text));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(macroControl));
}

void ScriptingApi::Content::ScriptTable::handleDefaultDeactivatedProperties()
{
	ComplexDataScriptComponent::handleDefaultDeactivatedProperties();

	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::textColour));
}

struct ScriptingApi::Content::ScriptTable::Wrapper
{
	API_METHOD_WRAPPER_1(ScriptTable, getTableValue);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, setTablePopupFunction);
	API_VOID_METHOD_WRAPPER_2(ScriptTable, connectToOtherTable);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, setSnapValues);
	API_METHOD_WRAPPER_1(ScriptTable, registerAtParent);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, referToData);
};

ScriptingApi::Content::ScriptTable::ScriptTable(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name, int x, int y, int , int ) :
ComplexDataScriptComponent(base, name, snex::ExternalData::DataType::Table)
{
	propertyIds.add("tableIndex");
	propertyIds.add("customColours"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 100);
	setDefaultValue(ScriptComponent::Properties::height, 50);
	setDefaultValue(ScriptTable::Properties::TableIndex, 0);
	setDefaultValue(ScriptTable::Properties::customColours, 0);

	handleDefaultDeactivatedProperties();
	
	
	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::Properties::processorId);
	initInternalPropertyFromValueTreeOrDefault(Properties::TableIndex);

	updateCachedObjectReference();

	ADD_API_METHOD_1(getTableValue);
	ADD_API_METHOD_2(connectToOtherTable);
	ADD_API_METHOD_1(setSnapValues);
	ADD_API_METHOD_1(referToData);
	ADD_API_METHOD_1(setTablePopupFunction);
	ADD_API_METHOD_1(registerAtParent);
}

ScriptingApi::Content::ScriptTable::~ScriptTable()
{
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptTable::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::TableWrapper(content, this, index);
}

float ScriptingApi::Content::ScriptTable::getTableValue(float inputValue)
{
	if (auto t = getCachedTable())
	{
		if (SampleLookupTable *st = dynamic_cast<SampleLookupTable*>(t))
		{
			return st->getInterpolatedValue(inputValue, sendNotificationAsync);
		}
	}

	return 0.0f;
};


void ScriptingApi::Content::ScriptTable::setSnapValues(var snapValueArray)
{
	if (!snapValueArray.isArray())
		reportScriptError("You must call setSnapValues with an array");

	snapValues = snapValueArray;
	
	// hack: use the unused parameterID property to update the snap values
	// (it will also be used for updating the text function)...
	getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}




void ScriptingApi::Content::ScriptTable::resetValueToDefault()
{
	if (auto t = getCachedTable())
	{
		t->reset();
	}
}

void ScriptingApi::Content::ScriptTable::setTablePopupFunction(var newFunction)
{
	tableValueFunction = newFunction;
	getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}

void ScriptingApi::Content::ScriptTable::connectToOtherTable(String processorId, int index)
{
	setScriptObjectProperty(ScriptingApi::Content::ScriptComponent::processorId, processorId, dontSendNotification);
	setScriptObjectProperty(getIndexPropertyId(), index, sendNotification);
}

void ScriptingApi::Content::ScriptTable::referToData(var tableData)
{
	referToDataBase(tableData);
}

var ScriptingApi::Content::ScriptTable::registerAtParent(int index)
{
	return registerComplexDataObjectAtParent(index);
}

struct ScriptingApi::Content::ScriptSliderPack::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPack, setSliderAtIndex);
	API_METHOD_WRAPPER_1(ScriptSliderPack, getSliderValueAt);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setAllValues);
	API_METHOD_WRAPPER_0(ScriptSliderPack, getNumSliders);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, referToData);
    API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setWidthArray);
	API_METHOD_WRAPPER_1(ScriptSliderPack, registerAtParent);
	API_METHOD_WRAPPER_0(ScriptSliderPack, getDataAsBuffer);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setAllValueChangeCausesCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setUsePreallocatedLength);
};

ScriptingApi::Content::ScriptSliderPack::ScriptSliderPack(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name_, int x, int y, int , int ) :
ComplexDataScriptComponent(base, name_, snex::ExternalData::DataType::SliderPack)
{
	ADD_NUMBER_PROPERTY(i00, "sliderAmount");		ADD_AS_SLIDER_TYPE(0, 128, 1);
	ADD_NUMBER_PROPERTY(i01, "stepSize");
	ADD_SCRIPT_PROPERTY(i02, "flashActive");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "showValueOverlay");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i05, "SliderPackIndex");  
	ADD_SCRIPT_PROPERTY(i06, "mouseUpCallback"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
	setDefaultValue(ScriptComponent::defaultValue, 1.0f);
	setDefaultValue(bgColour, 0x00000000);
	setDefaultValue(itemColour, 0x77FFFFFF);
	setDefaultValue(itemColour2, 0x77FFFFFF);
	setDefaultValue(textColour, 0x33FFFFFF);
	setDefaultValue(CallbackOnMouseUpOnly, false);

	setDefaultValue(SliderAmount, 0);
	setDefaultValue(StepSize, 0);
	setDefaultValue(FlashActive, true);
	setDefaultValue(ShowValueOverlay, true);
	
	setDefaultValue(SliderPackIndex, 0);

	setDefaultValue(SliderAmount, hise::SliderPackData::NumDefaultSliders);
	setDefaultValue(StepSize, 0.01);

	handleDefaultDeactivatedProperties();
	

	initInternalPropertyFromValueTreeOrDefault(SliderAmount);
	initInternalPropertyFromValueTreeOrDefault(min);
	initInternalPropertyFromValueTreeOrDefault(max);
	initInternalPropertyFromValueTreeOrDefault(StepSize);
	initInternalPropertyFromValueTreeOrDefault(FlashActive);
	initInternalPropertyFromValueTreeOrDefault(ShowValueOverlay);
	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::Properties::processorId);
	initInternalPropertyFromValueTreeOrDefault(SliderPackIndex);
	initInternalPropertyFromValueTreeOrDefault(CallbackOnMouseUpOnly);

	updateCachedObjectReference();

	ADD_API_METHOD_2(setSliderAtIndex);
	ADD_API_METHOD_1(getSliderValueAt);
	ADD_API_METHOD_1(setAllValues);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_1(referToData);
    ADD_API_METHOD_1(setWidthArray);
	ADD_API_METHOD_1(registerAtParent);
	ADD_API_METHOD_0(getDataAsBuffer);
	ADD_API_METHOD_1(setAllValueChangeCausesCallback);
	ADD_API_METHOD_1(setUsePreallocatedLength);
}

ScriptingApi::Content::ScriptSliderPack::~ScriptSliderPack()
{
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptSliderPack::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::SliderPackWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptSliderPack::setSliderAtIndex(int index, double newValue)
{
	if (auto d = getCachedSliderPack())
	{
		value = index;
		d->setValue(index, (float)newValue, dontSendNotification);

		if(allValueChangeCausesCallback)
			d->getUpdater().sendDisplayChangeMessage((float)index, sendNotificationAsync);
	}
}

double ScriptingApi::Content::ScriptSliderPack::getSliderValueAt(int index)
{
	if (auto d = getCachedSliderPack())
	{
		d->setDisplayedIndex(index);
		return d->getValue(index);
	}

	return 0.0;
}



void ScriptingApi::Content::ScriptSliderPack::setAllValues(var value)
{
	if (auto d = getCachedSliderPack())
	{
		auto isMultiValue = value.isBuffer() || value.isArray();

		int maxIndex = value.isBuffer() ? (value.getBuffer()->size) : (value.isArray() ? value.size() : 0);

		for (int i = 0; i < d->getNumSliders(); i++)
		{
			if (!isMultiValue || isPositiveAndBelow(i, maxIndex))
			{
				auto vToSet = isMultiValue ? (float)value[i] : (float)value;
				d->setValue(i, (float)vToSet, dontSendNotification);
			}
		}

		value = -1;

		if (allValueChangeCausesCallback)
		{
			d->getUpdater().sendContentChangeMessage(sendNotificationAsync, -1);
		}
		else
			d->getUpdater().sendDisplayChangeMessage(-1, sendNotificationAsync, true);
	}
}

int ScriptingApi::Content::ScriptSliderPack::getNumSliders() const
{
	if (auto d = getCachedSliderPack())
		return d->getNumSliders();

	return 0;
}

StringArray ScriptingApi::Content::ScriptSliderPack::getOptionsFor(const Identifier &id)
{
	if(id == getIdFor(Properties::StepSize))
	{
		StringArray sa;

		sa.add("0.01");
		sa.add("0.1");
		sa.add("1.0");
		
		return sa;
	}
	
	return ComplexDataScriptComponent::getOptionsFor(id);
};

void ScriptingApi::Content::ScriptSliderPack::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	static const Identifier oldProcessorId = Identifier("ProcessorId");

	if (id == getIdFor(SliderAmount))
	{
		jassert(isCorrectlyInitialised(SliderAmount));

		if(auto d = getCachedSliderPack())
			d->setNumSliders((int)newValue);
	}
	else if (id == getIdFor(defaultValue))
	{
		if (auto d = getCachedSliderPack())
			d->setDefaultValue((float)newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::min))
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::min));

		if(auto d = getCachedSliderPack())
			d->setRange((double)newValue, d->getRange().getEnd(), d->getStepSize());
	}
	else if (id == getIdFor(ScriptComponent::Properties::max))
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::max));

		if (auto d = getCachedSliderPack())
			d->setRange(d->getRange().getStart(), (double)newValue, d->getStepSize());
	}
	else if (id == getIdFor(StepSize))
	{
		jassert(isCorrectlyInitialised(StepSize));

		if (auto d = getCachedSliderPack())
			d->setRange(d->getRange().getStart(), d->getRange().getEnd(), (double)newValue);
	}
	else if (id == getIdFor(FlashActive))
	{
		jassert(isCorrectlyInitialised(FlashActive));

		if(auto d = getCachedSliderPack())
			d->setFlashActive((bool)newValue);
	}
	else if (id == getIdFor(ShowValueOverlay))
	{
		jassert(isCorrectlyInitialised(ShowValueOverlay));

		if (auto d = getCachedSliderPack())
			d->setShowValueOverlay((bool)newValue);
	}
	else if (getIdFor(parameterId) == id )
	{
		// don't do anything if you try to connect a slider pack to a parameter...
		return;
	}
	
	ComplexDataScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}


void ScriptingApi::Content::ScriptSliderPack::setValue(var newValue)
{
	ScriptComponent::setValue(newValue);

	value = newValue;

	if (auto array = newValue.getArray())
	{
		if (auto d = getCachedSliderPack())
			d->swapData(*array, dontSendNotification);
	}
	else if (auto b = newValue.getBuffer())
	{
		if (auto d = getCachedSliderPack())
			d->swapData(newValue, dontSendNotification);
	}
}

void ScriptingApi::Content::ScriptSliderPack::resetValueToDefault()
{
	auto f = (float)getScriptObjectProperty(defaultValue);
	FloatSanitizers::sanitizeFloatNumber(f);
	setAllValues((double)f);
}

var ScriptingApi::Content::ScriptSliderPack::getValue() const
{
	if (auto d = getCachedSliderPack())
		return d->getDataArray();

	return {};
}

void ScriptingApi::Content::ScriptSliderPack::referToData(var sliderPackData)
{
	referToDataBase(sliderPackData);
}

void ScriptingApi::Content::ScriptSliderPack::setWidthArray(var normalizedWidths)
{
    if((getNumSliders() + 1) != normalizedWidths.size())
    {
        logErrorAndContinue("Width array length must be numSliders + 1");
    }
    
    if(auto ar = normalizedWidths.getArray())
    {
        widthArray = *ar;
        sendChangeMessage();
    }
}

var ScriptingApi::Content::ScriptSliderPack::registerAtParent(int pIndex)
{
	return registerComplexDataObjectAtParent(pIndex);
}

void ScriptingApi::Content::ScriptSliderPack::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
{
	if (t == ComplexDataUIUpdaterBase::EventType::ContentChange)
	{
		auto sliderIndex = (int)data;
		value = sliderIndex;;
		changed();
	}
}

void ScriptingApi::Content::ScriptSliderPack::changed()
{
	getScriptProcessor()->controlCallback(this, value);
}

juce::var ScriptingApi::Content::ScriptSliderPack::getDataAsBuffer()
{
	return getCachedSliderPack()->getDataArray();
}

void ScriptingApi::Content::ScriptSliderPack::setAllValueChangeCausesCallback(bool shouldBeEnabled)
{
	allValueChangeCausesCallback = shouldBeEnabled;
}

void ScriptingApi::Content::ScriptSliderPack::setUsePreallocatedLength(int numMaxSliders)
{
	getCachedSliderPack()->setUsePreallocatedLength(numMaxSliders);
}

struct ScriptingApi::Content::ScriptAudioWaveform::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, referToData);
	API_METHOD_WRAPPER_0(ScriptAudioWaveform, getRangeStart);
	API_METHOD_WRAPPER_0(ScriptAudioWaveform, getRangeEnd);
	API_METHOD_WRAPPER_1(ScriptAudioWaveform, registerAtParent);
	API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, setDefaultFolder);
	API_VOID_METHOD_WRAPPER_1(ScriptAudioWaveform, setPlaybackPosition);
};

ScriptingApi::Content::ScriptAudioWaveform::ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier waveformName, int x, int y, int, int) :
	ComplexDataScriptComponent(base, waveformName, snex::ExternalData::DataType::AudioFile)
{
	ADD_SCRIPT_PROPERTY(i01, "itemColour3"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(i02, "opaque"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "showLines"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i04, "showFileName"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	
	ADD_SCRIPT_PROPERTY(i05, "sampleIndex");
	ADD_SCRIPT_PROPERTY(i06, "enableRange"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);

	setDefaultValue(Properties::itemColour3, 0x22FFFFFF);
	setDefaultValue(ScriptComponent::Properties::bgColour, (int64)0xFF555555);
	setDefaultValue(ScriptComponent::Properties::itemColour2, (int64)0xffcccccc);
	setDefaultValue(ScriptComponent::Properties::itemColour, (int64)0xa2181818);

	setDefaultValue(Properties::opaque, true);
	setDefaultValue(Properties::showLines, false);
	setDefaultValue(Properties::showFileName, true);
	setDefaultValue(Properties::sampleIndex, 0);
	setDefaultValue(Properties::enableRange, true);

	handleDefaultDeactivatedProperties();

	updateCachedObjectReference();

	ADD_API_METHOD_1(referToData);
	ADD_API_METHOD_0(getRangeStart);
	ADD_API_METHOD_0(getRangeEnd);
	ADD_API_METHOD_1(setDefaultFolder);
	ADD_API_METHOD_1(registerAtParent);
	ADD_API_METHOD_1(setPlaybackPosition);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptAudioWaveform::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::AudioWaveformWrapper(content, this, index);
}


ValueTree ScriptingApi::Content::ScriptAudioWaveform::exportAsValueTree() const
{
	ValueTree v = ComplexDataScriptComponent::exportAsValueTree();

	if (auto af = getCachedAudioFile())
	{
		auto sr = af->getCurrentRange();
		v.setProperty("rangeStart", sr.getStart(), nullptr);
		v.setProperty("rangeEnd", sr.getEnd(), nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptAudioWaveform::restoreFromValueTree(const ValueTree &v)
{
	ComplexDataScriptComponent::restoreFromValueTree(v);

	if (auto af = getCachedAudioFile())
	{
		// Old versions of HISE used fileName instead of the data property for storing the file reference
		if (v.hasProperty("fileName") && !v.hasProperty("data"))
		{
			af->fromBase64String(v.getProperty("fileName", "").toString());
		}

		Range<int> range(v.getProperty("rangeStart", 0), v.getProperty("rangeEnd", 0));
		af->setRange(range);
	}
}

StringArray ScriptingApi::Content::ScriptAudioWaveform::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(processorId))
	{
		auto list = ComplexDataScriptComponent::getOptionsFor(id);

		auto os = ProcessorHelpers::findParentProcessor(dynamic_cast<Processor*>(getScriptProcessor()), true);

		auto samplers = ProcessorHelpers::getAllIdsForType<ModulatorSampler>(os);
		list.addArray(samplers);
		return list;
	}
	else
	{
		return ScriptComponent::getOptionsFor(id);
	}
}

ModulatorSampler* ScriptingApi::Content::ScriptAudioWaveform::getSampler()
{
	return dynamic_cast<ModulatorSampler*>(getConnectedProcessor());
}

void ScriptingApi::Content::ScriptAudioWaveform::resetValueToDefault()
{
	if (auto af = getCachedAudioFile())
		af->fromBase64String({});
}

void ScriptingApi::Content::ScriptAudioWaveform::referToData(var audioData)
{
	referToDataBase(audioData);
}

int ScriptingApi::Content::ScriptAudioWaveform::getRangeStart()
{
	if (auto af = getCachedAudioFile())
		return af->getCurrentRange().getStart();

	return 0;
}

int ScriptingApi::Content::ScriptAudioWaveform::getRangeEnd()
{
	if (auto af = getCachedAudioFile())
		return af->getCurrentRange().getEnd();

	return 0;
}

var ScriptingApi::Content::ScriptAudioWaveform::registerAtParent(int pIndex)
{
	return registerComplexDataObjectAtParent(pIndex);
}

void ScriptingApi::Content::ScriptAudioWaveform::setDefaultFolder(var newDefaultFolder)
{
	if (auto af = getCachedAudioFile())
	{
		if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(newDefaultFolder.getObject()))
			af->getProvider()->setRootDirectory(sf->f);
		else
			reportScriptError("newDefaultFolder must be a File object");
	}
}

void ScriptingApi::Content::ScriptAudioWaveform::setPlaybackPosition(double normalisedPosition)
{
	if (auto af = getCachedAudioFile())
	{
		auto sampleIndex = roundToInt((double)af->getCurrentRange().getLength() * normalisedPosition);
		af->getUpdater().sendDisplayChangeMessage(sampleIndex, sendNotificationAsync, true);
	}
}

struct ScriptingApi::Content::ScriptImage::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptImage, setImageFile);
	API_VOID_METHOD_WRAPPER_1(ScriptImage, setAlpha);
	
};

ScriptingApi::Content::ScriptImage::ScriptImage(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier imageName, int x, int y, int , int ) :
ScriptComponent(base, imageName)
{
	ADD_NUMBER_PROPERTY(i00, "alpha");				ADD_AS_SLIDER_TYPE(0.0, 1.0, 0.01);
	ADD_SCRIPT_PROPERTY(i01, "fileName");			ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_NUMBER_PROPERTY(i02, "offset");
	ADD_NUMBER_PROPERTY(i03, "scale");
	ADD_SCRIPT_PROPERTY(i07, "blendMode");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "allowCallbacks");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "popupMenuItems");		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i06, "popupOnRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	priorityProperties.add(getIdFor(FileName));

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 50);
	setDefaultValue(ScriptComponent::Properties::height, 50);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(BlendMode, "Normal");
	setDefaultValue(Alpha, 1.0f);
	setDefaultValue(FileName, String());
	setDefaultValue(Offset, 0);
	setDefaultValue(Scale, 1.0);
	setDefaultValue(AllowCallbacks, false);
	setDefaultValue(PopupMenuItems, "");
	setDefaultValue(PopupOnRightClick, true);

	handleDefaultDeactivatedProperties();

	initInternalPropertyFromValueTreeOrDefault(FileName);
	
	ADD_API_METHOD_2(setImageFile);
	ADD_API_METHOD_1(setAlpha);
}

ScriptingApi::Content::ScriptImage::~ScriptImage()
{
	image.clear();
};


StringArray ScriptingApi::Content::ScriptImage::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(FileName))
	{
		StringArray sa;

		sa.add("Load new File");

		sa.addArray(getProcessor()->getMainController()->getCurrentImagePool()->getIdList());

		return sa;
	}
	else if (id == getIdFor(AllowCallbacks))
	{
		return MouseCallbackComponent::getCallbackLevels();
	}
	else if (id == getIdFor(BlendMode))
	{
		return {
			"Normal",
			"Lighten",
			"Darken",
			"Multiply",
			"Average",
			"Add",
			"Subtract",
			"Difference",
			"Negation",
			"Screen",
			"Exclusion",
			"Overlay",
			"SoftLight",
			"HardLight",
			"ColorDodge",
			"ColorBurn",
			"LinearDodge",
			"LinearBurn",
			"LinearLight",
			"VividLight",
			"PinLight",
			"HardMix",
			"Reflect",
			"Glow",
			"Phoenix"
		};
	}

	return ScriptComponent::getOptionsFor(id);
}



void ScriptingApi::Content::ScriptImage::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	CHECK_COPY_AND_RETURN_17(getProcessor());

	if (id == getIdFor(FileName))
	{
		jassert(isCorrectlyInitialised(FileName));

		setImageFile(newValue, true);
	}

	if (id == getIdFor(BlendMode))
	{
		auto idx = getOptionsFor(id).indexOf(newValue.toString());
		blendMode = (gin::BlendMode)idx;
		updateBlendMode();
	}
	
	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptImage::setImageFile(const String &absoluteFileName, bool forceUseRealFile)
{
	ignoreUnused(forceUseRealFile);

	CHECK_COPY_AND_RETURN_10(getProcessor());

	if (absoluteFileName.isEmpty())
	{
		image.clear();
		setScriptObjectProperty(FileName, absoluteFileName, sendNotification);
		return;
	}

	PoolReference ref(getProcessor()->getMainController(), absoluteFileName, ProjectHandler::SubDirectories::Images);
	image.clear();
	image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);

	updateBlendMode();

	setScriptObjectProperty(FileName, absoluteFileName, sendNotification);
};



ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptImage::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ImageWrapper(content, this, index);
}

const Image ScriptingApi::Content::ScriptImage::getImage() const
{
	if (blendMode != gin::Normal)
	{
		return blendImage.isValid() ? blendImage : PoolHelpers::getEmptyImage(getScriptObjectProperty(ScriptComponent::Properties::width),
			getScriptObjectProperty(ScriptComponent::Properties::height));
	}

	return image ? *image.getData() : PoolHelpers::getEmptyImage(getScriptObjectProperty(ScriptComponent::Properties::width),
		getScriptObjectProperty(ScriptComponent::Properties::height));
}

StringArray ScriptingApi::Content::ScriptImage::getItemList() const
{
	String items = getScriptObjectProperty(PopupMenuItems).toString();

	if (items.isEmpty()) return StringArray();

	StringArray sa;
	sa.addTokens(items, "\n", "");
	sa.removeEmptyStrings();

	return sa;
}

void ScriptingApi::Content::ScriptImage::setAlpha(float newAlphaValue)
{
	setScriptObjectPropertyWithChangeMessage(getIdFor(Alpha), newAlphaValue);
}

void ScriptingApi::Content::ScriptImage::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::itemColour));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::itemColour2));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
	
}

void ScriptingApi::Content::ScriptImage::updateBlendMode()
{
	if (blendMode == gin::Normal)
		return;

	if (image)
	{
		auto original = image->data;
		
		blendImage = Image(Image::ARGB, original.getWidth(), original.getHeight(), true);
		gin::applyBlend(blendImage, original, blendMode);
	}
}

ScriptingApi::Content::ScriptPanel::MouseCursorInfo::MouseCursorInfo() = default;

ScriptingApi::Content::ScriptPanel::MouseCursorInfo::MouseCursorInfo(MouseCursor::StandardCursorType t):
	defaultCursorType(t)
{}

ScriptingApi::Content::ScriptPanel::MouseCursorInfo::MouseCursorInfo(const Path& p, Colour c_, Point<float> hp):
	path(p),
	c(c_),
	hitPoint(hp)
{}

ScriptingApi::Content::ScriptPanel::AnimationListener::~AnimationListener()
{}

Identifier ScriptingApi::Content::ScriptPanel::getStaticObjectName()
{ RETURN_STATIC_IDENTIFIER("ScriptPanel"); }

Identifier ScriptingApi::Content::ScriptPanel::getObjectName() const
{ return getStaticObjectName(); }

bool ScriptingApi::Content::ScriptPanel::isAutomatable() const
{ return true; }

void ScriptingApi::Content::ScriptPanel::preRecompileCallback()
{
	cachedList.clear();

	ScriptComponent::preRecompileCallback();

            
	timerRoutine.clear();
	loadRoutine.clear();
	mouseRoutine.clear();

	stopTimer();
}

void ScriptingApi::Content::ScriptPanel::sendRepaintMessage()
{
	ScriptComponent::sendRepaintMessage();
	repaint();
}

void ScriptingApi::Content::ScriptPanel::setIsModalPopup(bool shouldBeModal)
{
	isModalPopup = shouldBeModal;
}

int ScriptingApi::Content::ScriptPanel::getNumSubPanels() const
{ return childPanels.size(); }

ScriptingApi::Content::ScriptPanel* ScriptingApi::Content::ScriptPanel::getSubPanel(int index)
{
	return childPanels[index].get();
}

#if HISE_INCLUDE_RLOTTIE
bool ScriptingApi::Content::ScriptPanel::isAnimationActive() const
{ return animation != nullptr && animation->isValid(); }

RLottieAnimation::Ptr ScriptingApi::Content::ScriptPanel::getAnimation()
{ return animation.get(); }
#endif

void ScriptingApi::Content::ScriptPanel::forcedRepaint()
{
}

ScriptingApi::Content::ScriptPanel::MouseCursorInfo ScriptingApi::Content::ScriptPanel::getMouseCursorPath() const
{
	return mouseCursorPath;
}

LambdaBroadcaster<ScriptingApi::Content::ScriptPanel::MouseCursorInfo>& ScriptingApi::Content::ScriptPanel::
getCursorUpdater()
{ return cursorUpdater; }

void ScriptingApi::Content::ScriptPanel::setScriptObjectPropertyWithChangeMessage(const Identifier& id, var newValue,
	NotificationType notifyEditor)
{
			

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);

#if HISE_INCLUDE_RLOTTIE
	if (id == getIdFor((int)ScriptComponent::height) ||
		id == getIdFor((int)ScriptComponent::width))
	{
		if (animation != nullptr)
		{
			auto pos = getPosition();
			animation->setSize(pos.getWidth(), pos.getHeight());
		}
	}
#endif
}

bool ScriptingApi::Content::ScriptPanel::isModal() const
{
	return isModalPopup;
}

bool ScriptingApi::Content::ScriptPanel::isUsingCustomPaintRoutine() const
{ return HiseJavascriptEngine::isJavascriptFunction(paintRoutine); }

bool ScriptingApi::Content::ScriptPanel::isUsingClippedFixedImage() const
{ return usesClippedFixedImage; }

void ScriptingApi::Content::ScriptPanel::scaleFactorChanged(float)
{}

var ScriptingApi::Content::ScriptPanel::getJSONPopupData() const
{ return jsonPopupData; }

void ScriptingApi::Content::ScriptPanel::cancelPendingFunctions()
{
	stopTimer();
}

void ScriptingApi::Content::ScriptPanel::resetValueToDefault()
{
	auto f = (float)getScriptObjectProperty(defaultValue);
	FloatSanitizers::sanitizeFloatNumber(f);
	setValue(f);
	repaint();
}

Rectangle<int> ScriptingApi::Content::ScriptPanel::getPopupSize() const
{ return popupBounds; }

Image ScriptingApi::Content::ScriptPanel::getLoadedImage(const String& prettyName) const
{
	for (const auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
			return img.image ? *img.image.getData() : Image();
	}

	return Image();
}

bool ScriptingApi::Content::ScriptPanel::isShowing(bool checkParentComponentVisibility) const
{
	if (!ScriptComponent::isShowing(checkParentComponentVisibility))
		return false;

	if (!getScriptObjectProperty(isPopupPanel))
		return true;

	return shownAsPopup;
}

DrawActions::Handler* ScriptingApi::Content::ScriptPanel::getDrawActionHandler()
{
	if (graphics != nullptr)
		return &graphics->getDrawHandler();

	return nullptr;
}

struct ScriptingApi::Content::ScriptPanel::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, repaint);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, repaintImmediately);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setPaintRoutine);
	API_VOID_METHOD_WRAPPER_3(ScriptPanel, setImage)
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setMouseCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setLoadingCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setTimerCallback);
	API_VOID_METHOD_WRAPPER_3(ScriptPanel, setFileDropCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, startTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, stopTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, changed);
	API_VOID_METHOD_WRAPPER_2(ScriptPanel, loadImage);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, unloadAllImages);
	API_METHOD_WRAPPER_1(ScriptPanel, isImageLoaded);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setDraggingBounds);
	API_VOID_METHOD_WRAPPER_2(ScriptPanel, setPopupData);
  API_VOID_METHOD_WRAPPER_3(ScriptPanel, setValueWithUndo);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, showAsPopup);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, closeAsPopup);
	API_VOID_METHOD_WRAPPER_3(ScriptPanel, setMouseCursor);
	API_METHOD_WRAPPER_0(ScriptPanel, addChildPanel);
	API_METHOD_WRAPPER_0(ScriptPanel, removeFromParent);
	API_METHOD_WRAPPER_0(ScriptPanel, getChildPanelList);
	API_METHOD_WRAPPER_0(ScriptPanel, getParentPanel);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setAnimation);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setAnimationFrame);
	API_METHOD_WRAPPER_0(ScriptPanel, getAnimationData);
	API_METHOD_WRAPPER_0(ScriptPanel, isVisibleAsPopup);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setIsModalPopup);
	API_METHOD_WRAPPER_3(ScriptPanel, startExternalFileDrag);
	API_METHOD_WRAPPER_1(ScriptPanel, startInternalDrag);
};

ScriptingApi::Content::ScriptPanel::ScriptPanel(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier panelName, int x, int y, int , int ) :
ScriptComponent(base, panelName, 1),
PreloadListener(base->getMainController_()->getSampleManager()),
graphics(new ScriptingObjects::GraphicsObject(base, this)),
isChildPanel(true),
loadRoutine(base, nullptr, var(), 1),
timerRoutine(base, nullptr, var(), 0),
mouseRoutine(base, nullptr, var(), 1),
fileDropRoutine(base, nullptr, var(), 1),
mouseCursorPath(MouseCursor::NormalCursor)
{
	init();
}

ScriptingApi::Content::ScriptPanel::ScriptPanel(ScriptPanel* parent) :
	ScriptComponent(parent->getScriptProcessor(), {}, 1),
	PreloadListener(parent->getScriptProcessor()->getMainController_()->getSampleManager()),
	graphics(new ScriptingObjects::GraphicsObject(parent->getScriptProcessor(), this)),
	parentPanel(parent),
	loadRoutine		(parent->getScriptProcessor(), nullptr, var(), 1),
	mouseRoutine	(parent->getScriptProcessor(), nullptr, var(), 1),
	timerRoutine	(parent->getScriptProcessor(), nullptr, var(), 0),
	fileDropRoutine	(parent->getScriptProcessor(), nullptr, var(), 1),
	mouseCursorPath(MouseCursor::NormalCursor)
{
	
	init();
}


void ScriptingApi::Content::ScriptPanel::init()
{
	ADD_NUMBER_PROPERTY(i00, "borderSize");					ADD_AS_SLIDER_TYPE(0, 20, 1);
	ADD_NUMBER_PROPERTY(i01, "borderRadius");				ADD_AS_SLIDER_TYPE(0, 20, 1);
	ADD_SCRIPT_PROPERTY(i02, "opaque");						ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "allowDragging");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i04, "allowCallbacks");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "popupMenuItems");				ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i06, "popupOnRightClick");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i07, "popupMenuAlign");  ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i08, "selectedPopupIndex");
	ADD_NUMBER_PROPERTY(i09, "stepSize");
	ADD_SCRIPT_PROPERTY(i10, "enableMidiLearn");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i11, "holdIsRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i12, "isPopupPanel");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i13, "bufferToImage");      ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    
	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 100);
	setDefaultValue(ScriptComponent::Properties::height, 50);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(ScriptComponent::Properties::isPluginParameter, false);
	setDefaultValue(textColour, 0x23FFFFFF);
	setDefaultValue(itemColour, 0x30000000);
	setDefaultValue(itemColour2, 0x30000000);
	setDefaultValue(borderSize, 2.0f);
	setDefaultValue(borderRadius, 6.0f);
	setDefaultValue(opaque, false);
	setDefaultValue(allowDragging, 0);
	setDefaultValue(allowCallbacks, "No Callbacks");
	setDefaultValue(PopupMenuItems, "");
	setDefaultValue(PopupOnRightClick, true);
	setDefaultValue(popupMenuAlign, false);
	setDefaultValue(selectedPopupIndex, -1);
	setDefaultValue(stepSize, 0.0);
	setDefaultValue(enableMidiLearn, false);
	setDefaultValue(holdIsRightClick, true);
	setDefaultValue(isPopupPanel, false);
    setDefaultValue(bufferToImage, false);

	handleDefaultDeactivatedProperties();

	
	addConstant("data", new DynamicObject());

	//initInternalPropertyFromValueTreeOrDefault(visible);

	ADD_API_METHOD_0(repaint);
	ADD_API_METHOD_0(repaintImmediately);
	ADD_API_METHOD_1(setPaintRoutine);
	ADD_API_METHOD_3(setImage);
	ADD_API_METHOD_1(setMouseCallback);
	ADD_API_METHOD_1(setLoadingCallback);
	ADD_API_METHOD_1(setTimerCallback);
	ADD_API_METHOD_3(setFileDropCallback);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_2(loadImage);
	ADD_API_METHOD_0(unloadAllImages);
	ADD_API_METHOD_1(isImageLoaded);
	ADD_API_METHOD_1(setDraggingBounds);
	ADD_API_METHOD_2(setPopupData);
	ADD_API_METHOD_3(setValueWithUndo);
	ADD_API_METHOD_1(showAsPopup);
	ADD_API_METHOD_0(closeAsPopup);
	ADD_API_METHOD_1(setIsModalPopup);
	ADD_API_METHOD_0(isVisibleAsPopup);
	ADD_API_METHOD_0(addChildPanel);
	ADD_API_METHOD_0(removeFromParent);
	ADD_API_METHOD_0(getChildPanelList);
	ADD_API_METHOD_0(getParentPanel);
	ADD_API_METHOD_3(setMouseCursor);
	ADD_API_METHOD_0(getAnimationData);
	ADD_API_METHOD_1(setAnimation);
	ADD_API_METHOD_1(setAnimationFrame);
	ADD_API_METHOD_3(startExternalFileDrag);
	ADD_API_METHOD_1(startInternalDrag);
}


ScriptingApi::Content::ScriptPanel::~ScriptPanel()
{
	if (parentPanel != nullptr)
		parentPanel->sendSubComponentChangeMessage(this, false, sendNotificationAsync);

	stopTimer();

	timerRoutine.clear();
	mouseRoutine.clear();
	loadRoutine.clear();
	paintRoutine = var();
    
    loadedImages.clear();
    
    graphics = nullptr;
}

StringArray ScriptingApi::Content::ScriptPanel::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(allowCallbacks))
	{
		return MouseCallbackComponent::getCallbackLevels();
	}

	return ScriptComponent::getOptionsFor(id);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptPanel::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::PanelWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptPanel::repaint()
{
	auto threadId = getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread();

	if (threadId == MainController::KillStateHandler::SampleLoadingThread ||
		threadId == MainController::KillStateHandler::ScriptingThread ||
		threadId == MainController::KillStateHandler::MessageThread)
	{
		internalRepaint(false);
	}
	else
	{
		getScriptProcessor()->getMainController_()->getJavascriptThreadPool().addDeferredPaintJob(this);
	}
}


void ScriptingApi::Content::ScriptPanel::repaintImmediately()
{
	repaint();
}


void ScriptingApi::Content::ScriptPanel::setPaintRoutine(var paintFunction)
{
	paintRoutine = paintFunction;

	if (HiseJavascriptEngine::isJavascriptFunction(paintFunction) && !parent->allowGuiCreation)
	{
		repaint();
        
        for(auto l: animationListeners)
        {
            if(l != nullptr)
                l->paintRoutineChanged();
        }
	}

}

void ScriptingApi::Content::ScriptPanel::internalRepaint(bool forceRepaint/*=false*/)
{
	if (!usesClippedFixedImage && HiseJavascriptEngine::isJavascriptFunction(paintRoutine))
	{
		auto mc = dynamic_cast<Processor*>(getScriptProcessor())->getMainController();
		auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

		auto safeThis = WeakReference<ScriptPanel>(this);

		auto f = [safeThis, forceRepaint](JavascriptProcessor*)
		{
			if (safeThis != nullptr)
			{
				Result r = Result::ok();
				safeThis.get()->internalRepaintIdle(forceRepaint, r);
				return r;
			}

			return Result::ok();
		};

		mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::LowPriorityCallbackExecution, jp, f);
	}
}


bool ScriptingApi::Content::ScriptPanel::internalRepaintIdle(bool forceRepaint, Result& r)
{
	jassert_locked_script_thread(dynamic_cast<Processor*>(getScriptProcessor())->getMainController());

	const bool parentHasMovedOn = !isChildPanel && !parent->hasComponent(this);

	if (parentHasMovedOn || !parent->asyncFunctionsAllowed())
	{
		return true;
	}

	HiseJavascriptEngine* engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

	if (engine == nullptr)
		return true;

	auto imageBounds = getBoundsForImage();

	int canvasWidth = imageBounds.getWidth();
	int canvasHeight = imageBounds.getHeight();

	if ((!forceRepaint && !isShowing()) || canvasWidth <= 0 || canvasHeight <= 0)
	{
		return true;
	}

	var thisObject(this);
	var arguments = var(graphics.get());
	var::NativeFunctionArgs args(thisObject, &arguments, 1);


	Result::ok();

	if (!engine->isInitialising())
	{
		engine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();
	}

	engine->callExternalFunction(paintRoutine, args, &r);

	if (r.failed())
	{
		debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
	}

	graphics->getDrawHandler().flush();

	return true;
}

void ScriptingApi::Content::ScriptPanel::setLoadingCallback(var loadingCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadingCallback))
	{
		getScriptProcessor()->getMainController_()->getSampleManager().addPreloadListener(this);

		loadRoutine = WeakCallbackHolder(getScriptProcessor(), this, loadingCallback, 1);
		loadRoutine.incRefCount();
		loadRoutine.setThisObject(this);
		loadRoutine.setHighPriority();
	}
    else
    {
        getScriptProcessor()->getMainController_()->getSampleManager().removePreloadListener(this);
		loadRoutine = WeakCallbackHolder(getScriptProcessor(), this, var(), 1);
    }
}


void ScriptingApi::Content::ScriptPanel::preloadStateChanged(bool isPreloading)
{
	if (loadRoutine)
		loadRoutine.call1(isPreloading);
}

void ScriptingApi::Content::ScriptPanel::setFileDropCallback(String callbackLevel, String wildcard, var dropFunction)
{
	fileDropLevel = callbackLevel;
	fileDropExtension = wildcard;
	fileDropRoutine = WeakCallbackHolder(getScriptProcessor(), this, dropFunction, 1);
	fileDropRoutine.incRefCount();
	fileDropRoutine.setThisObject(this);
	fileDropRoutine.setHighPriority();

}

void ScriptingApi::Content::ScriptPanel::setMouseCallback(var mouseCallbackFunction)
{
	mouseRoutine = WeakCallbackHolder(getScriptProcessor(), this, mouseCallbackFunction, 1);
	mouseRoutine.incRefCount();
	mouseRoutine.setThisObject(this);
	mouseRoutine.setHighPriority();
}

void ScriptingApi::Content::ScriptPanel::fileDropCallback(var fileInformation)
{
	const bool parentHasMovedOn = !isChildPanel && !parent->hasComponent(this);

	if (parentHasMovedOn || !parent->asyncFunctionsAllowed())
		return;

	if (fileDropRoutine)
		fileDropRoutine.call1(fileInformation);
}

void ScriptingApi::Content::ScriptPanel::mouseCallback(var mouseInformation)
{
	const bool parentHasMovedOn = !isChildPanel && !parent->hasComponent(this);

	if (parentHasMovedOn || !parent->asyncFunctionsAllowed())
		return;

	if (mouseRoutine)
		mouseRoutine.call1(mouseInformation);
}

void ScriptingApi::Content::ScriptPanel::setTimerCallback(var timerCallback_)
{
	timerRoutine = WeakCallbackHolder(getScriptProcessor(), this, timerCallback_, 0);
	timerRoutine.incRefCount();
	timerRoutine.setThisObject(this);
}



void ScriptingApi::Content::ScriptPanel::timerCallback()
{
	auto mc = dynamic_cast<Processor*>(getScriptProcessor())->getMainController();

	if (mc == nullptr)
		return;

	if (timerRoutine)
		timerRoutine.call(nullptr, 0);
}



void ScriptingApi::Content::ScriptPanel::loadImage(String imageName, String prettyName)
{
	PoolReference ref(getProcessor()->getMainController(), imageName, ProjectHandler::SubDirectories::Images);

	for (auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
		{
			if (img.image.getRef() != ref)
			{
				HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());
				img.image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
			}

			return;
		}
	}

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	if (auto newImage = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref))
		loadedImages.add({ newImage, prettyName });
	else
	{
		debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "Image " + imageName + " not found. ");
	}
}

void ScriptingApi::Content::ScriptPanel::unloadAllImages()
{
	loadedImages.clear();
}

bool ScriptingApi::Content::ScriptPanel::isImageLoaded(String prettyName)
{
	for (auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
			return true;
	}
	
	return false;
}

StringArray ScriptingApi::Content::ScriptPanel::getItemList() const
{
	String items = getScriptObjectProperty(PopupMenuItems).toString();

	if (items.isEmpty()) return StringArray();

	StringArray sa;
	sa.addTokens(items, "\n", "");
	sa.removeEmptyStrings();

	return sa;
}

Rectangle<int> ScriptingApi::Content::ScriptPanel::getDragBounds() const
{
    if(dragBounds.isArray())
    {
        return ApiHelpers::getIntRectangleFromVar(dragBounds);
    }
    else
    {
        return {};
    }
}

void ScriptingApi::Content::ScriptPanel::setDraggingBounds(var area)
{
	dragBounds = area;
}

void ScriptingApi::Content::ScriptPanel::setPopupData(var jsonData, var position)
{
	jsonPopupData = jsonData;
	
	Result r = Result::ok(); 
	
	popupBounds = ApiHelpers::getIntRectangleFromVar(position, &r);

	if (r.failed())
	{
		throw String(r.getErrorMessage());
	}
}

struct PanelComplexDataUndoEvent : public UndoableAction
{
	PanelComplexDataUndoEvent(ScriptingApi::Content::ScriptComponent* c, int index_, var old_, var new_) :
		p(c),
		oldValue(old_),
		newValue(new_),
		index(index_)
	{
	}

	bool perform() override
	{
		if (p != nullptr)
		{
			p->setValue(newValue);
			p->changed();
			return true;
		}

		return false;
	}

	bool undo() override
	{
		if (p != nullptr)
		{
			p->setValue(oldValue);
			p->changed();
			return true;
		}

		return false;
	}

	var oldValue;
	var newValue;

	WeakReference<ScriptComponent> p;
	int index;
};

void ScriptingApi::Content::ScriptPanel::setValueWithUndo(var oldValue, var newValue, var actionName)
{
    auto p = dynamic_cast<Processor*>(getScriptProcessor());
    
    auto sc = getScriptProcessor()->getScriptingContent();
    
    const int index = sc->getComponentIndex(getName());
    
	UndoableAction* newEvent;

	if (newValue.isArray() || newValue.isObject() ||
		oldValue.isArray() || oldValue.isObject())
	{
		newEvent = new PanelComplexDataUndoEvent(this, index, oldValue, newValue);
	}
	else
	{
		newEvent = new BorderPanel::UndoableControlEvent(p, index, (float)oldValue, (float)newValue);
	}
    
    getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
}

void ScriptingApi::Content::ScriptPanel::setImage(String imageName, int xOffset, int yOffset)
{
	jassert_locked_script_thread(getScriptProcessor()->getMainController_());

	paintRoutine = var();
	usesClippedFixedImage = true;

	Image toUse = getLoadedImage(imageName);

	auto b = getPosition().withPosition(0, 0);
		
	int w = 0;
	int h = 0;

	if (xOffset == 0)
	{
		double ratio = (double)b.getHeight() / (double)b.getWidth();
		w = toUse.getWidth();
		h = (int)((double)w * ratio);
		yOffset = jmin<int>(yOffset, toUse.getHeight() - h);
	}
	else if (yOffset == 0)
	{
		double ratio = (double)b.getHeight() / (double)b.getWidth();
		h = toUse.getHeight();
		w = (int)((double)h * ratio);
		xOffset = jmin<int>(xOffset, toUse.getWidth() - xOffset);
	}
	else
	{
		logErrorAndContinue("Can't offset both dimensions. Either x or y must be 0");
	}

	auto img = toUse.getClippedImage(Rectangle<int>(0, yOffset, w, h));

	if (auto drawHandler = getDrawActionHandler())
	{
		drawHandler->beginDrawing();
		drawHandler->addDrawAction(new ScriptedDrawActions::drawImageWithin(img, b.toFloat()));
		drawHandler->flush();
	}
}

void ScriptingApi::Content::ScriptPanel::setMouseCursor(var pathIcon, var colour, var hitPoint)
 {
	getCursorUpdater().enableLockFreeUpdate(getScriptProcessor()->getMainController_()->getGlobalUIUpdater());

	if (auto po = dynamic_cast<ScriptingObjects::PathObject*>(pathIcon.getObject()))
	{
		mouseCursorPath.path = po->getPath();
		mouseCursorPath.c = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);
		
		if (auto ar = hitPoint.getArray())
		{
			if (ar->size() == 2)
			{
				mouseCursorPath.hitPoint = Point<float>((float)((*ar)[0]), (float)((*ar)[1]));
				
				if (!Rectangle<float>(0.0f, 0.0f, 1.0f, 1.0f).contains(mouseCursorPath.hitPoint))
					reportScriptError("hitPoint must be within [0, 0, 1, 1] area");
				
			}
			else
				reportScriptError("hitPoint must be a [x, y] array");
		}
		else
			reportScriptError("hitPoint must be a [x, y] array");
	}
	else if (pathIcon.isString())
	{
		static const StringArray iconIds =
		{
		"ParentCursor",               /**< Indicates that the component's parent's cursor should be used. */
		"NoCursor",                       /**< An invisible cursor. */
		"NormalCursor",                   /**< The standard arrow cursor. */
		"WaitCursor",                     /**< The normal hourglass or spinning-beachball 'busy' cursor. */
		"IBeamCursor",                    /**< A vertical I-beam for positioning within text. */
		"CrosshairCursor",                /**< A pair of crosshairs. */
		"CopyingCursor",                  /**< The normal arrow cursor, but with a "+" on it to indicate that you're dragging a copy of something. */
		"PointingHandCursor",             /**< A hand with a pointing finger, for clicking on web-links. */
		"DraggingHandCursor",             /**< An open flat hand for dragging heavy objects around. */
		"LeftRightResizeCursor",          /**< An arrow pointing left and right. */
		"UpDownResizeCursor",             /**< an arrow pointing up and down. */
		"UpDownLeftRightResizeCursor",    /**< An arrow pointing up, down, left and right. */
		"TopEdgeResizeCursor",            /**< A platform-specific cursor for resizing the top-edge of a window. */
		"BottomEdgeResizeCursor",         /**< A platform-specific cursor for resizing the bottom-edge of a window. */
		"LeftEdgeResizeCursor",           /**< A platform-specific cursor for resizing the left-edge of a window. */
		"RightEdgeResizeCursor",          /**< A platform-specific cursor for resizing the right-edge of a window. */
		"TopLeftCornerResizeCursor",      /**< A platform-specific cursor for resizing the top-left-corner of a window. */
		"TopRightCornerResizeCursor",     /**< A platform-specific cursor for resizing the top-right-corner of a window. */
		"BottomLeftCornerResizeCursor",   /**< A platform-specific cursor for resizing the bottom-left-corner of a window. */
		"BottomRightCornerResizeCursor"  /**< A platform-specific cursor for resizing the bottom-right-corner of a window. */
		};

		auto index = iconIds.indexOf(pathIcon.toString());

		if (isPositiveAndBelow(index, (MouseCursor::NumStandardCursorTypes)))
		{
			auto standardCursor = (MouseCursor::StandardCursorType)index;
			mouseCursorPath = MouseCursorInfo(standardCursor);
		}
		else
			reportScriptError("Unknown Cursor name. Use the JUCE enum as string");
	}
	else
		reportScriptError("pathIcon is not a path");

	getCursorUpdater().sendMessage(sendNotificationAsync, mouseCursorPath);
}

Rectangle<int> ScriptingApi::Content::ScriptPanel::getBoundsForImage() const
{
	auto scaleFactor = getScaleFactorForCanvas();

	int canvasWidth = (int)(scaleFactor * (double)getScriptObjectProperty(ScriptComponent::Properties::width));
	int canvasHeight = (int)(scaleFactor * (double)getScriptObjectProperty(ScriptComponent::Properties::height));

	return Rectangle<int>(0, 0, canvasWidth, canvasHeight);
}

double ScriptingApi::Content::ScriptPanel::getScaleFactorForCanvas() const
{
	double scaleFactor = parent->usesDoubleResolution() ? 2.0 : 1.0;

	scaleFactor *= dynamic_cast<const GlobalSettingManager*>(getScriptProcessor()->getMainController_())->getGlobalScaleFactor();

	scaleFactor = jmin<double>(2.0, scaleFactor);

	return scaleFactor;
}


bool ScriptingApi::Content::ScriptPanel::isVisibleAsPopup()
{
	return shownAsPopup;
}

void ScriptingApi::Content::ScriptPanel::showAsPopup(bool closeOtherPopups)
{
	shownAsPopup = true;

	parent->addPanelPopup(this, closeOtherPopups);

	repaintThisAndAllChildren();

	triggerAsyncUpdate();
}

void ScriptingApi::Content::ScriptPanel::closeAsPopup()
{
	shownAsPopup = false;

	repaintThisAndAllChildren();

	triggerAsyncUpdate();
}

void ScriptingApi::Content::ScriptPanel::handleDefaultDeactivatedProperties()
{
	ScriptComponent::handleDefaultDeactivatedProperties();
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
}

void ScriptingApi::Content::ScriptPanel::showAsModalPopup()
{
	shownAsPopup = true;

	parent->addPanelPopup(this, true);

	repaintThisAndAllChildren();
}

bool ScriptingApi::Content::ScriptPanel::timerCallbackInternal(MainController * mc, Result &r)
{
#if 0
	ignoreUnused(mc);
	jassert_locked_script_thread(mc);

	const bool parentHasMovedOn = !isChildPanel && !parent->hasComponent(this);

	if (parentHasMovedOn || !parent->asyncFunctionsAllowed())
	{
		return true;
	}

	if (HiseJavascriptEngine::isJavascriptFunction(timerRoutine))
	{
		auto engine = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->getScriptEngine();

		if (engine == nullptr)
			return true;

		var thisObject(this);
		var::NativeFunctionArgs args(thisObject, nullptr, 0);

		engine->maximumExecutionTime = HiseJavascriptEngine::getDefaultTimeOut();
		engine->callExternalFunction(timerRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}
	}

#endif

	return true;

}

void ScriptingApi::Content::ScriptPanel::repaintWrapped()
{
	auto mc = getScriptProcessor()->getMainController_();

	if (mc->getKillStateHandler().getCurrentThread() != MainController::KillStateHandler::ScriptingThread)
	{
		auto jp = dynamic_cast<JavascriptProcessor*>(getProcessor());

		auto f = [this](JavascriptProcessor* )
		{
			repaint();

			return Result::ok();
		};

		mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::LowPriorityCallbackExecution, jp, f);
	}
	else
	{
		repaint();
	}
}

var ScriptingApi::Content::ScriptPanel::addChildPanel()
{
	auto s = new ScriptPanel(this);
	childPanels.add(s);

	sendSubComponentChangeMessage(s, true);

	childPanels.getLast()->isChildPanel = true;
	return var(childPanels.getLast().get());
}


var ScriptingApi::Content::ScriptPanel::getAnimationData()
{
#if HISE_INCLUDE_RLOTTIE
	updateAnimationData();
	return var(animationData);
#else
	reportScriptError("RLottie is disabled. Compile with HISE_INCLUDE_RLOTTIE");
	return var();
#endif
}

void ScriptingApi::Content::ScriptPanel::setAnimation(String base64LottieAnimation)
{
#if HISE_INCLUDE_RLOTTIE
	if (base64LottieAnimation.isEmpty())
	{
		animation = nullptr;
	}
	else
	{
		auto rManager = getScriptProcessor()->getMainController_()->getRLottieManager();

		animation = new RLottieAnimation(rManager, base64LottieAnimation);

		auto pos = getPosition();
		animation->setScaleFactor(2.0f);
		animation->setSize(pos.getWidth(), pos.getHeight());
	}

	setAnimationFrame(0);

	for (auto l : animationListeners)
	{
		if (l.get() != nullptr)
			l->animationChanged();
	}
#else
	reportScriptError("RLottie is disabled. Compile with HISE_INCLUDE_RLOTTIE");
#endif
}

void ScriptingApi::Content::ScriptPanel::setAnimationFrame(int numFrame)
{
#if HISE_INCLUDE_RLOTTIE
	if (animation != nullptr)
	{
		animation->setFrame(numFrame);
		updateAnimationData();
		graphics->getDrawHandler().flush();
	}
#else
	reportScriptError("RLottie is disabled. Compile with HISE_INCLUDE_RLOTTIE");
#endif
}

#if HISE_INCLUDE_RLOTTIE
void ScriptingApi::Content::ScriptPanel::updateAnimationData()
{
	DynamicObject::Ptr obj = animationData.getDynamicObject();  
	
	if(obj == nullptr)
		obj = new DynamicObject();

	obj->setProperty("active", isAnimationActive());

	if (animation != nullptr)
	{
		obj->setProperty("currentFrame", animation->getCurrentFrame());
		obj->setProperty("numFrames", animation->getNumFrames());
		obj->setProperty("frameRate", animation->getFrameRate());
	}
	else
	{
		obj->setProperty("currentFrame", 0);
		obj->setProperty("numFrames", 0);
		obj->setProperty("frameRate", 0);
	}

	animationData = var(obj.get());
}
#endif

bool ScriptingApi::Content::ScriptPanel::removeFromParent()
{
	if (parentPanel != nullptr && (parentPanel->childPanels.indexOf(this) != -1))
	{
		parentPanel->sendSubComponentChangeMessage(this, false, sendNotificationAsync);
		parentPanel->childPanels.removeObject(this);
		parentPanel = nullptr;
		return true;
	}

	return false;
}

var ScriptingApi::Content::ScriptPanel::getChildPanelList()
{
	Array<var> cp;

	for (auto p : childPanels)
		cp.add(var(p));

	return cp;
}

var ScriptingApi::Content::ScriptPanel::getParentPanel()
{
	if (parentPanel != nullptr)
		return var(parentPanel);

	return {};
}

void ScriptingApi::Content::ScriptPanel::addAnimationListener(AnimationListener* l)
{
#if HISE_INCLUDE_RLOTTIE
	animationListeners.addIfNotAlreadyThere(l);
#endif
}

void ScriptingApi::Content::ScriptPanel::removeAnimationListener(AnimationListener* l)
{
#if HISE_INCLUDE_RLOTTIE
	animationListeners.removeAllInstancesOf(l);
#endif
}

hise::DebugInformationBase::Ptr ScriptingApi::Content::ScriptPanel::createChildElement(DebugWatchIndex i) const
{
	var v;
	String id = "%PARENT%.";

	switch (i)
	{
	case DebugWatchIndex::Data:					
		v = getConstantValue(0); 

		if (auto obj = v.getDynamicObject())
			if (obj->getProperties().isEmpty())
				return nullptr;
		
		id << "data";  break;
	case DebugWatchIndex::PaintRoutine:			
		v = paintRoutine; 
		
		if (v.isUndefined() || v.isVoid())
			return nullptr;
		
		id << "paintRoutine";  break;
	case DebugWatchIndex::TimerCallback:		return timerRoutine.createDebugObject("timerCallback");
	case DebugWatchIndex::MouseCallback:		return mouseRoutine.createDebugObject("mouseCallback");
	case DebugWatchIndex::PreloadCallback:		return this->loadRoutine.createDebugObject("loadingCallback");
	case DebugWatchIndex::ChildPanels:
	{
		if (childPanels.isEmpty())
			return nullptr;

		Array<var> s;
		for (auto p : childPanels)
			s.add(var(p));

		v = s;
		id << "childPanels";
		break;
	}
	case DebugWatchIndex::FileCallback:		   return fileDropRoutine.createDebugObject("fileCallback");
	case DebugWatchIndex::NumDebugWatchIndexes:
	default:
		break;
	}

	auto vf = [v]() {return v; };
	return new LambdaValueInformation(vf, Identifier(id), {}, DebugInformation::Type::Constant, getLocation());
}


void ScriptingApi::Content::ScriptPanel::buildDebugListIfEmpty() const
{
	if (cachedList.isEmpty())
	{
		for (int i = 0; i < (int)DebugWatchIndex::NumDebugWatchIndexes; i++)
		{
			auto ptr = createChildElement((DebugWatchIndex)i);

			if (ptr != nullptr)
				cachedList.add(ptr);
		}
	}
}

hise::DebugInformationBase* ScriptingApi::Content::ScriptPanel::getChildElement(int index)
{
	return cachedList[index].get();
}

int ScriptingApi::Content::ScriptPanel::getNumChildElements() const
{
	buildDebugListIfEmpty();
	return cachedList.size();
}

bool ScriptingApi::Content::ScriptPanel::startExternalFileDrag(var fileToDrag, bool moveOriginal, var finishCallback)
{
	StringArray files;

	auto addToArray = [&](var v)
	{
		if (v.isString())
			files.add(v.toString());

		if (auto fObj = dynamic_cast<ScriptingObjects::ScriptFile*>(v.getObject()))
			files.add(fObj->f.getFullPathName());
	};

	if (fileToDrag.isArray())
	{
		for (const auto& f : *fileToDrag.getArray())
			addToArray(f);
	}
	else
		addToArray(fileToDrag);

	if (files.isEmpty())
		return false;

	WeakReference<ProcessorWithScriptingContent> sp = getScriptProcessor();

	std::function<void()> f;

	WeakReference<ScriptPanel> safeThis(this);

	if (HiseJavascriptEngine::isJavascriptFunction(finishCallback))
	{
		f = [sp, finishCallback, safeThis]()
		{
			WeakCallbackHolder cb(sp, safeThis.get(), finishCallback, 0);
			cb.callSync(nullptr, 0);
		};
	}

    auto f2 = [files, f]()
    {
        DragAndDropContainer::performExternalDragDropOfFiles(files, false, nullptr, f);
    };
    
#if JUCE_WINDOWS
    f2();
#else
    MessageManager::callAsync(f2);
#endif
    
    return true;
}

bool ScriptingApi::Content::ScriptPanel::startInternalDrag(var dragData)
{
	getScriptProcessor()->getScriptingContent()->sendDragAction(Content::RebuildListener::DragAction::Start, this, dragData);

	return true;
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptedViewport::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ViewportWrapper(content, this, index);
}

struct ScriptingApi::Content::ScriptedViewport::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setTableMode);
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setTableColumns);
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setTableRowData);
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setTableCallback);
	API_METHOD_WRAPPER_1(ScriptedViewport, getOriginalRowIndex);
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setEventTypesForValueCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptedViewport, setTableSortFunction);
};

ScriptingApi::Content::ScriptedViewport::ScriptedViewport(ProcessorWithScriptingContent* base, Content* /*parentContent*/, Identifier viewportName, int x, int y, int , int ):
	ScriptComponent(base, viewportName)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("scrollBarThickness");		ADD_AS_SLIDER_TYPE(0, 40, 1);
	propertyIds.add("autoHide");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("useList"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("viewPositionX")); ADD_AS_SLIDER_TYPE(0, 1, 0.01);
	propertyIds.add(Identifier("viewPositionY")); ADD_AS_SLIDER_TYPE(0, 1, 0.01);
	propertyIds.add(Identifier("items"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i01, "fontName");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i02, "fontSize");		ADD_AS_SLIDER_TYPE(1, 200, 1);
	ADD_SCRIPT_PROPERTY(i03, "fontStyle");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "alignment");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
	setDefaultValue(viewPositionX, 0.0);
	setDefaultValue(viewPositionY, 0.0);
	setDefaultValue(scrollbarThickness, 16.0);
	setDefaultValue(autoHide, true);
	setDefaultValue(useList, false);
	setDefaultValue(Items, "");
	setDefaultValue(defaultValue, 0);
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Arial");
	setDefaultValue(Alignment, "centred");

	handleDefaultDeactivatedProperties();

	initInternalPropertyFromValueTreeOrDefault(Items);

	ADD_API_METHOD_1(setTableMode);
	ADD_API_METHOD_1(setTableColumns);
	ADD_API_METHOD_1(setTableRowData);
	ADD_API_METHOD_1(setTableCallback);
	ADD_API_METHOD_1(getOriginalRowIndex);
	ADD_API_METHOD_1(setTableSortFunction);
	ADD_API_METHOD_1(setEventTypesForValueCallback);
}

void ScriptingApi::Content::ScriptedViewport::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */)
{
	if (id == getIdFor(Items))
	{
		jassert(isCorrectlyInitialised(Items));

		//if (newValue.toString().isNotEmpty())
		//{
			currentItems = StringArray::fromLines(newValue.toString());
		//}
	}

	if (id == getIdFor(viewPositionX))
	{
		auto y = (double)getScriptObjectProperty(getIdFor(viewPositionY));
		positionBroadcaster.sendMessage(sendNotificationAsync, (double)newValue, y);
	}

	if (id == getIdFor(viewPositionY))
	{
		auto x = (double)getScriptObjectProperty(getIdFor(viewPositionX));
		positionBroadcaster.sendMessage(sendNotificationAsync, x, (double)newValue);
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptedViewport::getOptionsFor(const Identifier &id)
{
	StringArray sa;

	int index = propertyIds.indexOf(id);

	Font f("Arial", 13.0f, Font::plain);

	switch (index)
	{
	case FontStyle:	sa.addArray(f.getAvailableStyles());
		break;
	case FontName:	sa.add("Default");
		sa.add("Oxygen");
		sa.add("Source Code Pro");
		getScriptProcessor()->getMainController_()->fillWithCustomFonts(sa);
		sa.addArray(Font::findAllTypefaceNames());
		break;
	case Alignment: sa = ApiHelpers::getJustificationNames();
		break;
	default:		sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

Justification ScriptingApi::Content::ScriptedViewport::getJustification()
{
	auto justAsString = getScriptObjectProperty(Alignment);
	return ApiHelpers::getJustification(justAsString);
}

void ScriptingApi::Content::ScriptedViewport::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));

}

juce::Array<hise::ScriptingApi::Content::ScriptComponent::PropertyWithValue> ScriptingApi::Content::ScriptedViewport::getLinkProperties() const
{
	auto vArray = ScriptComponent::getLinkProperties();

	vArray.add({ Items });
	vArray.add({ useList });

	return vArray;
}

struct UndoableTableSelection : public UndoableAction
{
	UndoableTableSelection(ScriptingApi::Content::ScriptedViewport* v, int nc, int nr):
		UndoableAction(),
		vp(v),
		newColumn(nc),
		newRow(nr)
	{
		auto old = v->getValue();

		if (old.isArray())
		{
			oldColumn = old[0];
			oldRow = old[1];
		}
		else
		{
			oldColumn = -1;
			oldRow = -1;
		}
	}

	bool perform() override
	{
		if (vp != nullptr)
		{
			vp->getTableModel()->sendCallback(newRow, newColumn + 1, true, ScriptTableListModel::EventType::SetValue);
		}

		return false;
	}

	bool undo() override
	{
		if (oldColumn == -1 && oldRow == -1)
			return false;

		if (vp != nullptr)
		{
			vp->getTableModel()->sendCallback(oldRow, oldColumn + 1, true, ScriptTableListModel::EventType::Undo);
		}

		return false;
	}

	int oldColumn, oldRow, newColumn, newRow;

	WeakReference<ScriptingApi::Content::ScriptedViewport> vp;
};

void ScriptingApi::Content::ScriptedViewport::setTableMode(var tableMetadata)
{
	if (!getScriptProcessor()->getScriptingContent()->interfaceCreationAllowed())
	{
		reportScriptError("Table Metadata must be set in the onInit callback");
		RETURN_VOID_IF_NO_THROW();
	}

	tableModel = new ScriptTableListModel(getScriptProcessor(), tableMetadata);

	if (tableModel->isMultiColumn())
	{
		WeakReference<ScriptedViewport> safeThis(this);

		tableModel->addAdditionalCallback([safeThis](int c, int r)
		{
			if (safeThis == nullptr)
				return;

			Array<var> v;
			v.add(c);
			v.add(r);

			safeThis->setValue(v);
		});
	}
}

void ScriptingApi::Content::ScriptedViewport::setTableColumns(var columnMetadata)
{
	if (!getScriptProcessor()->getScriptingContent()->interfaceCreationAllowed())
	{
		reportScriptError("Table Metadata must be set in the onInit callback");
		RETURN_VOID_IF_NO_THROW();
	}

	if (tableModel == nullptr)
	{
		reportScriptError("You need to call setTableMode first");
		RETURN_VOID_IF_NO_THROW();
	}

	tableModel->setTableColumnData(columnMetadata);
}

void ScriptingApi::Content::ScriptedViewport::setTableRowData(var tableData)
{
	if (tableModel == nullptr)
	{
		reportScriptError("You need to call setTableMode first");
		RETURN_VOID_IF_NO_THROW();
	}

	

	tableModel->setRowData(tableData);
}

void ScriptingApi::Content::ScriptedViewport::setTableCallback(var callbackFunction)
{
	if (tableModel == nullptr)
	{
		reportScriptError("You need to call setTableMode first");
		RETURN_VOID_IF_NO_THROW();
	}

	if (!getScriptProcessor()->getScriptingContent()->interfaceCreationAllowed())
	{
		reportScriptError("Table callback must be set in the onInit callback");
		RETURN_VOID_IF_NO_THROW();
	}

	tableModel->setCallback(callbackFunction);
}

void ScriptingApi::Content::ScriptedViewport::setValue(var newValue)
{
	if (tableModel != nullptr)
	{
		if (newValue.isArray() && newValue.size() == 2)
		{
			auto c = (int)newValue[0];
			auto r = (int)newValue[1];
			auto useUndo = (bool)getScriptObjectProperty(getIdFor(ScriptComponent::Properties::useUndoManager));

			ScopedPointer<UndoableAction> u = new UndoableTableSelection(this, c, r);

			if (useUndo)
				getScriptProcessor()->getMainController_()->getControlUndoManager()->perform(u.release());
			else
				u->perform();
		}
	}

	ScriptComponent::setValue(newValue);
}

void ScriptingApi::Content::ScriptedViewport::setEventTypesForValueCallback(var eventTypeList)
{
	if (tableModel != nullptr)
	{
		auto r = tableModel->setEventTypesForValueCallback(eventTypeList);

		if (!r.wasOk())
			reportScriptError(r.getErrorMessage());
	}
	else
		reportScriptError("You need to call setTableMode first");
}

void ScriptingApi::Content::ScriptedViewport::setTableSortFunction(var sortFunction)
{
	if (tableModel != nullptr)
	{
		tableModel->setTableSortFunction(sortFunction);
	}
	else
		reportScriptError("You need to call setTableMode first");
}

int ScriptingApi::Content::ScriptedViewport::getOriginalRowIndex(int rowIndex)
{
	if (tableModel != nullptr)
	{
		return tableModel->getOriginalRowIndex(rowIndex);
	}
	else
		reportScriptError("You need to call setTableMode first");

	return 0;
}

// ====================================================================================================== ScriptWebView functions

struct ScriptingApi::Content::ScriptWebView::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptWebView, bindCallback);
	API_VOID_METHOD_WRAPPER_2(ScriptWebView, callFunction);
	API_VOID_METHOD_WRAPPER_2(ScriptWebView, evaluate);
	API_VOID_METHOD_WRAPPER_0(ScriptWebView, reset);
    API_VOID_METHOD_WRAPPER_1(ScriptWebView, setIndexFile);
};

ScriptingApi::Content::ScriptWebView::ScriptWebView(ProcessorWithScriptingContent* base, Content* parentContent, Identifier webViewName, int x, int y, int width, int height):
	ScriptComponent(base, webViewName)
{
	auto mc = base->getMainController_();

	data = mc->getOrCreateWebView(webViewName);

	data->setErrorLogger([mc](const String& error)
	{
		mc->getConsoleHandler().writeToConsole(error, 1, mc->getMainSynthChain(), juce::Colours::orange);
	});

	ADD_SCRIPT_PROPERTY(i01, "enableCache");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i02, "enablePersistence");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "scaleFactorToZoom");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	
    ADD_SCRIPT_PROPERTY(i04,
        "enableDebugMode");    ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    
	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	
	setDefaultValue(Properties::enableCache, false);
	setDefaultValue(Properties::enablePersistence, true);
	setDefaultValue(Properties::scaleFactorToZoom, true);
    setDefaultValue(Properties::enableDebugMode, false);
	
	handleDefaultDeactivatedProperties();

    ADD_API_METHOD_1(setIndexFile);
	ADD_API_METHOD_2(bindCallback);
	ADD_API_METHOD_2(callFunction);
	ADD_API_METHOD_2(evaluate);
	ADD_API_METHOD_0(reset);
}

hise::ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptWebView::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::WebViewWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptWebView::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */)
{
	if (id == getIdFor(Properties::enableCache))
		data->setEnableCache((bool)newValue);
	else if (id == getIdFor(Properties::enablePersistence))
		data->setUsePersistentCalls((bool)newValue);
	else if (id == getIdFor(Properties::scaleFactorToZoom))
		data->setUseScaleFactorForZoom((bool)newValue);
    else if (id == getIdFor(Properties::enableDebugMode))
        data->setEnableDebugMode((bool)newValue);

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptWebView::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::saveInPreset));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isPluginParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::pluginParameterName));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::text));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::tooltip));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::useUndoManager));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::processorId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::parameterId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::linkedTo));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
}

ScriptingApi::Content::ScriptWebView::~ScriptWebView()
{
	data = nullptr;
}

void ScriptingApi::Content::ScriptWebView::callFunction(const String& javascriptFunction, const var& args)
{
	auto copy = data;
	MessageManager::callAsync([copy, javascriptFunction, args]()
	{
		copy->call(javascriptFunction, args);
	});
}

juce::var ScriptingApi::Content::ScriptWebView::HiseScriptCallback::operator()(const var& args)
{
	if (f)
	{
		var copy(args);
		var rv;

		auto ok = f.callSync(&copy, 1, &rv);

		f.reportError(ok);

		if (ok.wasOk())
			return rv;
	}

	return {};
}

void ScriptingApi::Content::ScriptWebView::bindCallback(const String& callbackId, const var& functionToCall)
{
	data->addCallback(callbackId, HiseScriptCallback(this, callbackId, functionToCall));
}

void ScriptingApi::Content::ScriptWebView::evaluate(const String& uid, const String& jsCode)
{
	auto copy = data;

	MessageManager::callAsync([uid, copy, jsCode]()
	{
		copy->evaluate(uid, jsCode);
	});
}

void ScriptingApi::Content::ScriptWebView::setIndexFile(var file)
{
    if(auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(file.getObject()))
    {
        String s = "/" + sf->f.getFileName();
        
        
        data->setRootDirectory(sf->f.getParentDirectory());
        data->setIndexFile(s);
    }
    else
    {
        reportScriptError("setIndexFile must be called with a file object");
    }
}

void ScriptingApi::Content::ScriptWebView::reset()
{
	data->reset(false);
}

// ====================================================================================================== ScriptFloatingTile functions

struct ScriptingApi::Content::ScriptFloatingTile::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptFloatingTile, setContentData);
};

ScriptingApi::Content::ScriptFloatingTile::ScriptFloatingTile(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier panelName, int x, int y, int , int ) :
	ScriptComponent(base, panelName)
{
	
	ADD_SCRIPT_PROPERTY(i05, "itemColour3");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(i06, "updateAfterInit");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i01, "ContentType");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i02, "Font");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i03, "FontSize");			ADD_TO_TYPE_SELECTOR(SelectorTypes::SliderSelector);
	ADD_SCRIPT_PROPERTY(i04, "Data");				ADD_TO_TYPE_SELECTOR(SelectorTypes::CodeSelector);

	priorityProperties.add(getIdFor(ContentType));

	setDefaultValue(Properties::itemColour3, 0);
	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(Properties::updateAfterInit, true);
	setDefaultValue(Properties::ContentType, EmptyComponent::getPanelId().toString());
	setDefaultValue(Properties::Font, "Default");
	setDefaultValue(Properties::FontSize, 14.0);
	setDefaultValue(Properties::Data, "{\n}");

	handleDefaultDeactivatedProperties();

	ADD_API_METHOD_1(setContentData);
}

ScriptingApi::Content::ScriptFloatingTile::~ScriptFloatingTile()
{}

Identifier ScriptingApi::Content::ScriptFloatingTile::getStaticObjectName()
{ RETURN_STATIC_IDENTIFIER("ScriptFloatingTile"); }

Identifier ScriptingApi::Content::ScriptFloatingTile::getObjectName() const
{ return getStaticObjectName(); }

void ScriptingApi::Content::ScriptFloatingTile::setValue(var newValue)
{
	value = newValue;
}

var ScriptingApi::Content::ScriptFloatingTile::getValue() const
{
	return value;
}

var ScriptingApi::Content::ScriptFloatingTile::getContentData()
{ return jsonData; }

void ScriptingApi::Content::ScriptFloatingTile::setContentData(var data)
{
	jsonData = data;

	if (auto obj = jsonData.getDynamicObject())
	{
		auto id = obj->getProperty("Type");

		// We need to make sure that the content type is changed so that the floating tile
		// receives an update message
		setScriptObjectProperty(Properties::ContentType, "", dontSendNotification);

		setScriptObjectProperty(Properties::ContentType, id, sendNotification);
	}
			
	//triggerAsyncUpdate();
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptFloatingTile::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::FloatingTileWrapper(content, this, index);
}

ValueTree ScriptingApi::Content::ScriptFloatingTile::exportAsValueTree() const
{
	auto v = ScriptComponent::exportAsValueTree();
	
	return v;
}

void ScriptingApi::Content::ScriptFloatingTile::restoreFromValueTree(const ValueTree &v)
{
	ScriptComponent::restoreFromValueTree(v);
}

StringArray ScriptingApi::Content::ScriptFloatingTile::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(ContentType))
	{
		FloatingTileContent::Factory factory;
		factory.registerFrontendPanelTypes();

		auto l = factory.getIdList();

		StringArray sa;

		for (auto& i : l)
			sa.add(i.toString());

		return sa;
	}
	else if (id == getIdFor(Font))
	{
		StringArray sa;

		sa.add("Default");
		sa.add("Oxygen");
		sa.add("Source Code Pro");
		getScriptProcessor()->getMainController_()->fillWithCustomFonts(sa);
		sa.addArray(Font::findAllTypefaceNames());

		return sa;
	}

	return ScriptComponent::getOptionsFor(id);
}

void ScriptingApi::Content::ScriptFloatingTile::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */)
{
	if (id == getIdFor(ContentType))
	{
		DynamicObject* obj = createOrGetJSONData();

		obj->setProperty("Type", newValue.toString());

#if 0
		auto f = [newValue, notifyEditor](Dispatchable* obj)
		{
			auto tile = static_cast<ScriptFloatingTile*>(obj);
			
			tile->createFloatingTileComponent(newValue);
			tile->setScriptObjectProperty(Properties::ContentType, newValue, notifyEditor);
			return Status::OK;
		};

		//getProcessor()->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);

		return;
#endif
	}
	else if (id == getIdFor(Data))
	{
		var specialData = JSON::parse(newValue.toString());

		if (auto obj = specialData.getDynamicObject())
		{
			auto dataObject = createOrGetJSONData();
			auto prop = obj->getProperties();

			for (int i = 0; i < prop.size(); i++)
				dataObject->setProperty(prop.getName(i), prop.getValueAt(i));
		}
	}
	else if (id == getIdFor(bgColour) || id == getIdFor(textColour) || id == getIdFor(itemColour) || id == getIdFor(itemColour2) || id == getIdFor(itemColour3))
	{
		auto obj = jsonData.getDynamicObject();

		if (obj == nullptr)
		{
			obj = new DynamicObject();
			jsonData = var(obj);
		}

		// Best. Line. Ever.
		Identifier idToUse = id == getIdFor(itemColour) ? Identifier("itemColour1") : id;

		auto colourObj = obj->getProperty("ColourData").getDynamicObject();

		if (colourObj == nullptr)
		{
			colourObj = new DynamicObject();
			obj->setProperty("ColourData", colourObj);
		}

		colourObj->setProperty(idToUse, newValue);
	}
	else if (id == getIdFor(Font) || id == getIdFor(FontSize))
	{
		auto obj = createOrGetJSONData();

		obj->setProperty(id, newValue);
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

DynamicObject* ScriptingApi::Content::ScriptFloatingTile::createOrGetJSONData()
{
	auto obj = jsonData.getDynamicObject();

	if (obj == nullptr)
	{
		obj = new DynamicObject();
		jsonData = var(obj);
	}

	return obj;
}

void ScriptingApi::Content::ScriptFloatingTile::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::saveInPreset));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isPluginParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::pluginParameterName));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::text));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::tooltip));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::useUndoManager));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::processorId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::parameterId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::linkedTo));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::automationId));
}

bool ScriptingApi::Content::ScriptFloatingTile::fillScriptPropertiesWithFloatingTile(FloatingTile* ft)
{
#if 0
	auto mc = getScriptProcessor()->getMainController_();

	// Do not call this outside the message thread.
	// If you call this from other threads, use the dispatcher for it.
	jassert_dispatched_message_thread(mc);

	FloatingTile ft(mc, nullptr);

	if (ft.getMainController()->shouldAbortMessageThreadOperation())
		return false;

	ft.setNewContent(newValue.toString());
	ft.getMainController()->checkAndAbortMessageThreadOperation();

#endif
	auto ftc = ft->getCurrentFloatingPanel();


	setScriptObjectProperty(bgColour, (int64)ftc->getDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour).getARGB(), sendNotification);
	setScriptObjectProperty(itemColour, (int64)ftc->getDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1).getARGB(), sendNotification);
	setScriptObjectProperty(itemColour2, (int64)ftc->getDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2).getARGB(), sendNotification);
	setScriptObjectProperty(textColour, (int64)ftc->getDefaultPanelColour(FloatingTileContent::PanelColourId::textColour).getARGB(), sendNotification);

	auto data = ftc->toDynamicObject();

	if (auto obj = data.getDynamicObject())
	{
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::ColourData));
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::StyleData));
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::LayoutData));
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::Font));
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::FontSize));
		obj->removeProperty(ftc->getDefaultablePropertyId((int)FloatingTileContent::PanelPropertyId::Type));

		setScriptObjectProperty(Data, JSON::toString(data, false, DOUBLE_TO_STRING_DIGITS), sendNotification);
	}

	return true;
}

// ====================================================================================================== Content functions


ScriptingApi::Content::Content(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
asyncRebuildBroadcaster(*this),
updateDispatcher(p->getMainController_()),
height(50),
width(600),
name(String()),
allowGuiCreation(true),
dragCallback(p, nullptr, var(), 1),
colour(Colour(0xff777777))
{
#if USE_FRONTEND
	updateDispatcher.suspendUpdates(true);
#endif

	initNumberProperties();

	DynamicObject::Ptr c = new DynamicObject();

	if (auto jp = dynamic_cast<JavascriptProcessor*>(p))
	{
		contentPropertyData = jp->getContentPropertiesForDevice((int)HiseDeviceSimulator::getDeviceType());
	}
	else
	{
		contentPropertyData = ValueTree("ContentProperties");
	}

	setMethod("addButton", Wrapper::addButton);
	setMethod("addKnob", Wrapper::addKnob);
	setMethod("addLabel", Wrapper::addLabel);
	setMethod("addComboBox", Wrapper::addComboBox);
	setMethod("addTable", Wrapper::addTable);
	setMethod("addImage", Wrapper::addImage);
	setMethod("addViewport", Wrapper::addViewport);
	setMethod("addPanel", Wrapper::addPanel);
	setMethod("addAudioWaveform", Wrapper::addAudioWaveform);
	setMethod("addSliderPack", Wrapper::addSliderPack);
	setMethod("addFloatingTile", Wrapper::addFloatingTile);
	setMethod("addWebView", Wrapper::addWebView);
	setMethod("setContentTooltip", Wrapper::setContentTooltip);
	setMethod("setToolbarProperties", Wrapper::setToolbarProperties);
	setMethod("setHeight", Wrapper::setHeight);
	setMethod("setWidth", Wrapper::setWidth);
	setMethod("createScreenshot", Wrapper::createScreenshot);
	setMethod("addVisualGuide", Wrapper::addVisualGuide);
    setMethod("makeFrontInterface", Wrapper::makeFrontInterface);
	setMethod("makeFullScreenInterface", Wrapper::makeFullScreenInterface);
	setMethod("setName", Wrapper::setName);
	setMethod("getComponent", Wrapper::getComponent);
	setMethod("getAllComponents", Wrapper::getAllComponents);
	setMethod("setPropertiesFromJSON", Wrapper::setPropertiesFromJSON);
	setMethod("setValuePopupData", Wrapper::setValuePopupData);
	setMethod("storeAllControlsAsPreset", Wrapper::storeAllControlsAsPreset);
	setMethod("restoreAllControlsFromPreset", Wrapper::restoreAllControlsFromPreset);
	setMethod("setUseHighResolutionForPanels", Wrapper::setUseHighResolutionForPanels);
	setMethod("setColour", Wrapper::setColour);
	setMethod("clear", Wrapper::clear);
	setMethod("isCtrlDown", Wrapper::isCtrlDown);
	setMethod("createPath", Wrapper::createPath);
	setMethod("createShader", Wrapper::createShader);
	setMethod("createMarkdownRenderer", Wrapper::createMarkdownRenderer);
    setMethod("createSVG", Wrapper::createSVG);
	setMethod("getScreenBounds", Wrapper::getScreenBounds);
	setMethod("getCurrentTooltip", Wrapper::getCurrentTooltip);
	setMethod("createLocalLookAndFeel", Wrapper::createLocalLookAndFeel);
	setMethod("isMouseDown", Wrapper::isMouseDown);
	setMethod("getComponentUnderMouse", Wrapper::getComponentUnderMouse);
	setMethod("callAfterDelay", Wrapper::callAfterDelay);
	setMethod("getComponentUnderDrag", Wrapper::getComponentUnderDrag);
	setMethod("refreshDragImage", Wrapper::refreshDragImage);
}

ScriptingApi::Content::~Content()
{
	asyncRebuildBroadcaster.cancelPendingUpdate();

	updateWatcher = nullptr;

	removeAllScriptComponents();

	contentPropertyData = ValueTree();

	masterReference.clear();
	
	
}

ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponentWithName(const Identifier &componentName)
{
	for (int i = 0; i < getNumComponents(); i++)
	{
		if (components[i]->name == componentName)
		{
			return components[i].get();
		}
	}

	return nullptr;
}

const ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponentWithName(const Identifier &componentName) const
{
	for (int i = 0; i < getNumComponents(); i++)
	{
		if (components[i]->name == componentName)
		{
			return components[i].get();
		}
	}

	return nullptr;
}

int ScriptingApi::Content::getComponentIndex(const Identifier &componentName) const
{
	for (int i = 0; i < getNumComponents(); i++)
	{
		if (components[i]->name == componentName)
		{
			return i;
		}
	}

	return -1;
}

ScriptingApi::Content::ScriptComboBox *ScriptingApi::Content::addComboBox(Identifier boxName, int x, int y)
{
	return addComponent<ScriptComboBox>(boxName, x, y);
}

ScriptingApi::Content::ScriptButton * ScriptingApi::Content::addButton(Identifier buttonName, int x, int y)
{
	return addComponent<ScriptButton>(buttonName, x, y);
};

ScriptingApi::Content::ScriptSlider * ScriptingApi::Content::addKnob(Identifier knobName, int x, int y)
{
	return addComponent<ScriptSlider>(knobName, x, y);
};

ScriptingApi::Content::ScriptImage * ScriptingApi::Content::addImage(Identifier knobName, int x, int y)
{
	return addComponent<ScriptImage>(knobName, x, y);
};

ScriptingApi::Content::ScriptLabel * ScriptingApi::Content::addLabel(Identifier labelName, int x, int y)
{
	return addComponent<ScriptLabel>(labelName, x, y);
};


ScriptingApi::Content::ScriptedViewport* ScriptingApi::Content::addViewport(Identifier viewportName, int x, int y)
{
	return addComponent<ScriptedViewport>(viewportName, x, y);
}


ScriptingApi::Content::ScriptTable * ScriptingApi::Content::addTable(Identifier labelName, int x, int y)
{
	return addComponent<ScriptTable>(labelName, x, y);
};


ScriptingApi::Content::ScriptPanel * ScriptingApi::Content::addPanel(Identifier panelName, int x, int y)
{
	return addComponent<ScriptPanel>(panelName, x, y);

};

ScriptingApi::Content::ScriptAudioWaveform * ScriptingApi::Content::addAudioWaveform(Identifier audioWaveformName, int x, int y)
{
	return addComponent<ScriptAudioWaveform>(audioWaveformName, x, y);
}

ScriptingApi::Content::ScriptSliderPack * ScriptingApi::Content::addSliderPack(Identifier sliderPackName, int x, int y)
{
	return addComponent<ScriptSliderPack>(sliderPackName, x, y);
}


ScriptingApi::Content::ScriptWebView* ScriptingApi::Content::addWebView(Identifier webviewName, int x, int y)
{
	return addComponent<ScriptWebView>(webviewName, x, y);
}


ScriptingApi::Content::ScriptFloatingTile* ScriptingApi::Content::addFloatingTile(Identifier floatingTileName, int x, int y)
{
	return addComponent<ScriptFloatingTile>(floatingTileName, x, y);
}


ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponent(int index)
{
	if (index == -1) return nullptr;

	if(index < components.size())
		return components[index].get();

	return nullptr;
}

juce::var ScriptingApi::Content::getScreenBounds(bool getTotalArea)
{
	Rectangle<int> b;

	{
		MessageManagerLock mm;

		if (getTotalArea)
			b = Desktop::getInstance().getDisplays().getMainDisplay().totalArea;
		else
			b = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
	}

	Array<var> vb;
	vb.add(b.getX());
	vb.add(b.getY());
	vb.add(b.getWidth());
	vb.add(b.getHeight());

	return var(vb);
}

var ScriptingApi::Content::getComponent(var componentName)
{
	Identifier n(componentName.toString());

	for (int i = 0; i < components.size(); i++)
	{
		if (n == components[i]->getName())
		{
#if USE_BACKEND
			if (componentToThrowAtDefinition.get() == components[i].get())
			{
				componentToThrowAtDefinition = nullptr;
				reportScriptError(n.toString() + " is defined here");
			}
#endif

			return var(components[i].get());
		}
	}

	logErrorAndContinue("Component with name " + componentName.toString() + " wasn't found.");

	return var();
}

var ScriptingApi::Content::getAllComponents(String regex)
{
	Array<var> list;

    bool getAll = regex == ".*";
    
	for (int i = 0; i < getNumComponents(); i++)
	{	    
		if (getAll || RegexFunctions::matchesWildcard(regex, components[i]->getName().toString()))
		{
			list.add(var(components[i].get()));
		}
	}

	return var(list);
}

void ScriptingApi::Content::setPropertiesFromJSON(const Identifier &componentName, const var &jsonData)
{
	Identifier componentId(componentName);

	for (int i = 0; i < components.size(); i++)
	{
		if (components[i]->getName() == componentId)
		{
			components[i]->setPropertiesFromJSON(jsonData);
		}
	}
}



void ScriptingApi::Content::endInitialization()
{
	allowGuiCreation = false;

	allowAsyncFunctions = true;

	updateWatcher = new ValueTreeUpdateWatcher(contentPropertyData, this);
}


void ScriptingApi::Content::beginInitialization()
{
	allowGuiCreation = true;

	updateWatcher = nullptr;
	guides.clear();
}


void ScriptingApi::Content::setHeight(int newHeight) noexcept
{
	if (!allowGuiCreation)
	{
		reportScriptError("the height can't be changed after onInit()");
		return;
	}

	if (newHeight > 800)
	{
		reportScriptError("Go easy on the height! (" + String(800) + "px is enough)");
		return;
	}

	height = newHeight;
};

void ScriptingApi::Content::setWidth(int newWidth) noexcept
{
	if (!allowGuiCreation)
	{
		reportScriptError("the width can't be changed after onInit()");
		return;
	}

	if (newWidth > 1280)
	{
		reportScriptError("Go easy on the width! (1280px is enough)");
		return;
	}

	width = newWidth;
	
};

void ScriptingApi::Content::makeFrontInterface(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;

    dynamic_cast<JavascriptMidiProcessor*>(getProcessor())->addToFront(true);
    
}

void ScriptingApi::Content::makeFullScreenInterface()
{
	width = HiseDeviceSimulator::getDisplayResolution().getWidth();
	height = HiseDeviceSimulator::getDisplayResolution().getHeight();

	dynamic_cast<JavascriptMidiProcessor*>(getProcessor())->addToFront(true);
}

void ScriptingApi::Content::setToolbarProperties(const var &/*toolbarProperties*/)
{
	reportScriptError("2017...");
}


void ScriptingApi::Content::setUseHighResolutionForPanels(bool shouldUseDoubleResolution)
{
	useDoubleResolution = shouldUseDoubleResolution;
}

bool ScriptingApi::Content::isCtrlDown()
{
	return juce::ModifierKeys::currentModifiers.isCommandDown() || juce::ModifierKeys::currentModifiers.isCtrlDown();
}


void ScriptingApi::Content::storeAllControlsAsPreset(const String &fileName, const ValueTree& automationData)
{
	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	ValueTree v = exportAsValueTree();

	v.setProperty("Processor", getProcessor()->getId(), nullptr);

	if (f.existsAsFile())
	{
		auto existingData = XmlDocument::parse(f);

        if(existingData != nullptr)
        {
            ValueTree preset = ValueTree::fromXml(*existingData);
            
            bool found = false;
            
            for (int i = 0; i < preset.getNumChildren(); i++)
            {
                if (preset.getChild(i).getProperty("Processor") == getProcessor()->getId())
                {
                    preset.getChild(i).copyPropertiesFrom(v, nullptr);
                    found = true;
                    break;
                }
            }
            
            if (!found) preset.addChild(v, -1, nullptr);
            
            preset.addChild(automationData, -1, nullptr);
            
            existingData = preset.createXml();
            
            f.replaceWithText(existingData->createDocument(""));
        }
        else
        {
            reportScriptError(f.getFullPathName() + " is not a valid .xml file");
        }
	}
	else
	{
		ValueTree preset = ValueTree("Preset");

		preset.addChild(v, -1, nullptr);

		preset.addChild(automationData, -1, nullptr);

		auto xml = preset.createXml();

		f.replaceWithText(xml->createDocument(""));
	}
}


juce::StringArray ScriptingApi::Content::getMacroNames()
{
	StringArray macroNames;

	if (components.size() != 0)
	{
		macroNames = components[0]->getOptionsFor(components[0]->getIdFor(ScriptComponent::macroControl));
	}

	return macroNames;
}


void ScriptingApi::Content::restoreAllControlsFromPreset(const String &fileName)
{
#if USE_FRONTEND

	CHECK_COPY_AND_RETURN_23(getProcessor());

	const ValueTree parent = getProcessor()->getMainController()->getSampleManager().getProjectHandler().getValueTree(ProjectHandler::SubDirectories::UserPresets);

	ValueTree v;

	for (int i = 0; i < parent.getNumChildren(); i++)
	{
		ValueTree preset = parent.getChild(i);

		if (preset.getProperty("FileName") != fileName) continue;

		for (int j = 0; j < preset.getNumChildren(); j++)
		{
			if (preset.getChild(j).getProperty("Processor") == getProcessor()->getId())
			{
				v = preset.getChild(j);
				break;
			}
		}
	}

	if (!v.isValid())
	{
		reportScriptError("Preset ID not found");
	}

	restoreAllControlsFromPreset(v);

#else

	File f;

	if (File::isAbsolutePath(fileName))
	{
		f = File(fileName);
	}
	else
	{
		f = File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets).getChildFile(fileName));
	}

	if (f.existsAsFile())
	{
		auto xml = XmlDocument::parse(f);

		ValueTree parent = ValueTree::fromXml(*xml);

		ValueTree v;

		for (int i = 0; i < parent.getNumChildren(); i++)
		{
			if (parent.getChild(i).getProperty("Processor") == getProcessor()->getId())
			{
				v = parent.getChild(i);
				break;
			}
		}

		if (!v.isValid())
		{
			reportScriptError("Preset ID not found");
		}

		restoreAllControlsFromPreset(v);


	}
	else
	{
		reportScriptError("File not found");
	}

#endif
}

void ScriptingApi::Content::restoreAllControlsFromPreset(const ValueTree &preset)
{
	restoreFromValueTree(preset);

	auto macroNames = getMacroNames();

	for (int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset)) continue;


#if ENABLE_SCRIPTING_BREAKPOINTS
		if (auto jsp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
		{
			if (jsp->getLastErrorMessage().getErrorMessage().startsWith("Breakpoint"))
			{
				break;
			}
		}
#endif

		

		static const Identifier id_("id");

		auto presetChild = preset.getChildWithProperty(id_, components[i]->getName().toString());

		var v;

		if (presetChild.isValid())
		{
			static const Identifier value_("value");

			auto allowStrings = dynamic_cast<ScriptLabel*>(components[i].get()) != nullptr;

			v = Helpers::getCleanedComponentValue(presetChild.getProperty(value_), allowStrings);
		}
		else
		{
			components[i]->resetValueToDefault();
			v = components[i]->getValue();
		}

		if (dynamic_cast<ScriptingApi::Content::ScriptLabel*>(components[i].get()) != nullptr)
		{
			getScriptProcessor()->controlCallback(components[i].get(), v);
		}
        else if (auto ssp = dynamic_cast<ScriptingApi::Content::ScriptSliderPack*>(components[i].get()))
        {
            // This must be restored again from the ValueTree in order to maintain the correct value
            if(presetChild.isValid())
				ssp->restoreFromValueTree(presetChild);

            getScriptProcessor()->controlCallback(ssp, ssp->getValue());
        }
		else if (v.isObject())
		{
			getScriptProcessor()->controlCallback(components[i].get(), v);
		}
		else
		{
			getProcessor()->setAttribute(i, (float)v, sendNotification);
		}

		const String macroName = components[i]->getScriptObjectProperty(ScriptComponent::macroControl).toString();

		const int macroIndex = macroNames.indexOf(macroName) - 1;

		if (macroIndex >= 0)
		{
			NormalisableRange<float> range(components[i]->getScriptObjectProperty(ScriptComponent::min), components[i]->getScriptObjectProperty(ScriptComponent::max));

			getProcessor()->getMainController()->getMacroManager().getMacroChain()->setMacroControl(macroIndex, range.convertTo0to1(components[i]->getValue()) * 127.0f, sendNotification);
		}
	}
}



ValueTree ScriptingApi::Content::exportAsValueTree() const
{
	ValueTree v("Content");

	for (int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset)) continue;

		ValueTree child = components[i]->exportAsValueTree();

		v.addChild(child, -1, nullptr);
	}

	return v;
};

void ScriptingApi::Content::restoreFromValueTree(const ValueTree &v)
{
	jassert(v.getType().toString() == "Content");

	static const Identifier id_("id");

	for (int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset)) continue;

		ValueTree child = v.getChildWithProperty(id_, components[i]->name.toString());

		if (child.isValid())
		{
			const String childTypeString = child.getProperty("type");

			if (childTypeString.isEmpty()) continue;

			Identifier childType(childTypeString);

			components[i]->restoreFromValueTree(child);

			if (childType != components[i]->getObjectName())
			{
				debugError(dynamic_cast<Processor*>(getScriptProcessor()), "Type mismatch in preset");
			}
		}
		else
		{
			components[i]->resetValueToDefault();
		}

		
	}
};

bool ScriptingApi::Content::isEmpty()
{
	return components.size() == 0;
}

var ScriptingApi::Content::createPath()
{
	ScriptingObjects::PathObject* obj = new ScriptingObjects::PathObject(getScriptProcessor());

	return var(obj);
}

juce::var ScriptingApi::Content::createLocalLookAndFeel()
{
	return var(new ScriptingObjects::ScriptedLookAndFeel(getScriptProcessor(), false));
}

void ScriptingApi::Content::cleanJavascriptObjects()
{
	allowAsyncFunctions = false;

	for (int i = 0; i < components.size(); i++)
	{
		components[i]->cancelChangedControlCallback();
		components[i]->setControlCallback(var());
		components[i]->cleanScriptChangedPropertyIds();
		components[i]->setLocalLookAndFeel(var());

		if (auto p = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(components[i].get()))
		{
			auto data = p->getConstantValue(0).getDynamicObject();

			data->clear();

			p->cancelPendingFunctions();

			p->setPaintRoutine(var());
			p->setTimerCallback(var());
			p->setMouseCallback(var());
			p->setLoadingCallback(var());
		}
	}

	
}


void ScriptingApi::Content::removeAllScriptComponents()
{
	cleanJavascriptObjects();

	components.clear();
}

void ScriptingApi::Content::rebuildComponentListFromValueTree()
{
    if(isRebuilding)
        return;
    
    ScopedValueSetter<bool> rebuildLimiter(isRebuilding, true);

	ValueTree currentState = exportAsValueTree();

	removeAllScriptComponents();

	components.ensureStorageAllocated(contentPropertyData.getNumChildren());

	addComponentsFromValueTree(contentPropertyData);

	restoreFromValueTree(currentState);

	asyncRebuildBroadcaster.notify();

	auto p = dynamic_cast<Processor*>(getScriptProcessor());

	if (p->getMainController()->getScriptComponentEditBroadcaster()->isBeingEdited(p))
	{
		debugToConsole(p, "Updated Components");
	}
}

Result ScriptingApi::Content::createComponentsFromValueTree(const ValueTree& newProperties, bool buildComponentList/*=true*/)
{
	auto oldData = contentPropertyData;

	updateWatcher = nullptr;

	contentPropertyData = newProperties;

	updateWatcher = new ValueTreeUpdateWatcher(contentPropertyData, this);

	removeAllScriptComponents();

	Identifier errorId;

	if (buildComponentList)
	{
		rebuildComponentListFromValueTree();
	}
	
	return Result::ok();
}

void ScriptingApi::Content::addComponentsFromValueTree(const ValueTree& v)
{
	try
	{
		static const Identifier co("Component");
		static const Identifier coPro("ContentProperties");
		static const Identifier id("id");
		static const Identifier type_("type");
		static const Identifier parent("parentComponent");

		if (v.getType() == co)
		{
			const Identifier thisId = Identifier(v.getProperty(id).toString());

			ReferenceCountedObjectPtr<ScriptComponent> sc = Helpers::createComponentFromValueTree(this, v);

			if(sc == nullptr)
				return;

			DynamicObject* dyn = new DynamicObject();
			var d(dyn);

			auto parentComponentName = v.getParent().getProperty(id).toString();

			dyn->setProperty(parent, parentComponentName);

			ValueTreeConverters::copyValueTreePropertiesToDynamicObject(v, d);

			components.add(sc);

			ScriptComponent::ScopedPropertyEnabler spe(sc.get());
			sc->setPropertiesFromJSON(d);
		}

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			addComponentsFromValueTree(v.getChild(i));
		}
	}
	catch (String& errorMessage)
	{
		debugError(getProcessor(), errorMessage);
	}
}

void ScriptingApi::Content::restoreSavedValue(const Identifier& controlId)
{
	var savedValue = getScriptProcessor()->getSavedValue(controlId);

	if (!savedValue.isUndefined())
	{
		components.getLast()->setValue(savedValue);
	}
}

ValueTree findChildRecursive(const ValueTree& v, const var& n)
{
	String na = n.toString();

	
	static const Identifier id_("id");


	if (v.getProperty(id_) == n)
	{
		return v;
	}

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		auto child = findChildRecursive(v.getChild(i), n);

		if (child.isValid())
			return child;
	}
		

	return ValueTree();
}

ValueTree ScriptingApi::Content::getValueTreeForComponent(const Identifier& id)
{
	var n(id.toString());
	return findChildRecursive(contentPropertyData, n);
}

void ScriptingApi::Content::sendRebuildMessage()
{
	for (int i = 0; i < rebuildListeners.size(); i++)
	{
		if (rebuildListeners[i] != nullptr)
		{
			rebuildListeners[i]->contentWasRebuilt();
		}
		else
		{
			rebuildListeners.remove(i--);
		}
	}

	auto p = dynamic_cast<Processor*>(getScriptProcessor());

	auto b = p->getMainController()->getScriptComponentEditBroadcaster();

	if (b->isBeingEdited(p))
	{
		debugToConsole(p, "Updated UI Data");
	}

	b->clearSelection();
}

void ScriptingApi::Content::setValuePopupData(var jsonData)
{
	valuePopupData = jsonData;
}

void ScriptingApi::Content::suspendPanelTimers(bool shouldBeSuspended)
{
	for (int i = 0; i < components.size(); i++)
	{
		if (auto sp = dynamic_cast<ScriptPanel*>(components[i].get()))
		{
			sp->suspendTimer(shouldBeSuspended);
		}
	}
}

ScriptingApi::Content::AsyncRebuildMessageBroadcaster::AsyncRebuildMessageBroadcaster(Content& parent_):
	parent(parent_)
{}

ScriptingApi::Content::AsyncRebuildMessageBroadcaster::~AsyncRebuildMessageBroadcaster()
{
	cancelPendingUpdate();
}

void ScriptingApi::Content::AsyncRebuildMessageBroadcaster::notify()
{
	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		handleAsyncUpdate();
	}
	else
		triggerAsyncUpdate();
}

void ScriptingApi::Content::AsyncRebuildMessageBroadcaster::handleAsyncUpdate()
{
	parent.sendRebuildMessage();
}

#ifndef HISE_SUPPORT_GLSL_LINE_NUMBERS
#define HISE_SUPPORT_GLSL_LINE_NUMBERS 0
#endif

var ScriptingApi::Content::createShader(const String& fileName)
{
	auto f = new ScriptingObjects::ScriptShader(getScriptProcessor());

	
	addScreenshotListener(f);

#if HISE_SUPPORT_GLSL_LINE_NUMBERS
	f->setEnableLineNumbers(true);
#endif

	if(!fileName.isEmpty())
		f->setFragmentShader(fileName);

	return var(f);
}


void ScriptingApi::Content::createScreenshot(var area, var directory, String name)
{
	if (!screenshotListeners.isEmpty())
	{
		if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(directory.getObject()))
		{
			auto dir = sf->f;

			if (!dir.existsAsFile() && !dir.isDirectory())
				dir.createDirectory();

			if (sf->f.isDirectory())
			{
				auto target = sf->f.getChildFile(name).withFileExtension("png");
				Rectangle<float> a;

				if (auto comp = dynamic_cast<ScriptComponent*>(area.getObject()))
				{
					a.setX((int)comp->getGlobalPositionX());
					a.setY((int)comp->getGlobalPositionY());
					a.setWidth((int)comp->getWidth());
					a.setHeight((int)comp->getHeight());
				}
				else
				{
					auto r = Result::ok();
					a = ApiHelpers::getRectangleFromVar(area, &r);

					if (!r.wasOk())
						reportScriptError(r.getErrorMessage());
				}

				// Send a message to all listeners
				for (auto sc : screenshotListeners)
				{
					if (sc != nullptr)
						sc->prepareScreenshot();
				}

				int timeWaitedMs = 0;

				// Now block until everything is ready
				for (auto sc : screenshotListeners)
				{
					if (sc != nullptr)
						timeWaitedMs += sc->blockWhileWaiting();
				}

				for (auto sc : screenshotListeners)
				{
					if (sc != nullptr)
						sc->makeScreenshot(target, a);
				}
			}
		}
	}
}

void ScriptingApi::Content::addVisualGuide(var guideData, var colour)
{
	if (auto ga = guideData.getArray())
	{
		VisualGuide g;
		g.c = Helpers::getCleanedObjectColour(colour);

		if (ga->size() == 4)
		{
			g.t = VisualGuide::Type::Rectangle;
			g.area = ApiHelpers::getRectangleFromVar(guideData);
		}
		else if (ga->size() == 2)
		{
			auto x = (float)ga->getUnchecked(0);
			auto y = (float)ga->getUnchecked(1);

			if (x == 0)
			{
				g.t = VisualGuide::Type::HorizontalLine;
				g.area = { 0.0f, y, (float)width, 1.0f };
			}
			else if (y == 0)
			{
				g.t = VisualGuide::Type::VerticalLine;
				g.area = { x, 0.0, 1.0f, (float)height };
			}
		}

		guides.add(std::move(g));
	}
	else
		guides.clear();

	for (auto sc : screenshotListeners)
	{
		if(sc != nullptr)
			sc->visualGuidesChanged();
	}
}

String ScriptingApi::Content::getCurrentTooltip()
{
	auto& desktop = Desktop::getInstance();
	auto mouseSource = desktop.getMainMouseSource();
	auto* newComp = mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse();

	if (auto ttc = dynamic_cast<TooltipClient*> (newComp))
		return ttc->getTooltip();

	return {};
}

var ScriptingApi::Content::createSVG(const String& b64)
{
    return var(new ScriptingObjects::SVGObject(getScriptProcessor(), b64));
}

juce::var ScriptingApi::Content::createMarkdownRenderer()
{
	return var(new ScriptingObjects::MarkdownObject(getScriptProcessor()));
}

int ScriptingApi::Content::isMouseDown()
{
	auto mods = Desktop::getInstance().getMainMouseSource().getCurrentModifiers();

	if (mods.isLeftButtonDown())
		return 1;
	if (mods.isRightButtonDown())
		return 2;

	return 0;
}

String ScriptingApi::Content::getComponentUnderMouse()
{
	if (auto c = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse())
	{
		return c->getComponentID();
	}

	return "";
}

void ScriptingApi::Content::callAfterDelay(int milliSeconds, var function, var thisObject)
{
	WeakCallbackHolder cb(getScriptProcessor(), nullptr, function, 0);
	cb.incRefCount();

	if (auto obj = thisObject.getObject())
		cb.setThisObject(obj);

	Timer::callAfterDelay(milliSeconds, [cb]() mutable
	{
		cb.call(nullptr, 0);
	});
}

void ScriptingApi::Content::recompileAndThrowAtDefinition(ScriptComponent* sc)
{
	componentToThrowAtDefinition = sc;

	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		auto rf = [this, jp](const JavascriptProcessor::SnippetResult& r)
		{
			componentToThrowAtDefinition = nullptr;
			jp->compileScript({});
		};

		jp->compileScript(rf);
	}
}

void ScriptingApi::Content::sendDragAction(RebuildListener::DragAction a, ScriptComponent* sc, var& data)
{
	for (auto r : rebuildListeners)
	{
		if (r != nullptr && r->onDragAction(a, sc, data))
			return;
	}
}

bool ScriptingApi::Content::refreshDragImage()
{
	var obj;

	for (auto r : rebuildListeners)
	{
		if (r->onDragAction(RebuildListener::DragAction::Repaint, nullptr, obj))
		{
			return true;
		}
	}

	return false;
}

String ScriptingApi::Content::getComponentUnderDrag()
{
	var obj;

	for (auto r : rebuildListeners)
	{
		if (r->onDragAction(RebuildListener::DragAction::Query, nullptr, obj))
		{
			return obj.toString();
		}
	}

	return obj.toString();
}

#undef ADD_TO_TYPE_SELECTOR
#undef ADD_AS_SLIDER_TYPE
#undef SEND_MESSAGE

Identifier ScriptingApi::Content::Helpers::getUniqueIdentifier(Content* c, const String& id)
{
	int trailingIntValue = id.getTrailingIntValue();

	String clean = id.upToLastOccurrenceOf(String(trailingIntValue), false, false);

	if (trailingIntValue == 0)
		trailingIntValue = 1;

	Identifier newId = Identifier(clean + String(trailingIntValue));

	while (c->getValueTreeForComponent(newId).isValid())
	{
		trailingIntValue++;
		newId = Identifier(clean + String(trailingIntValue));
	}

	return newId;
}


void ScriptingApi::Content::Helpers::deleteSelection(Content* c, ScriptComponentEditBroadcaster* b)
{
	ScriptComponentEditBroadcaster::Iterator iter(b);

	ValueTreeUpdateWatcher::ScopedDelayer sd(c->getUpdateWatcher());

	while (auto sc = iter.getNextScriptComponent())
	{
		deleteComponent(c, sc->getName(), dontSendNotification);
	}


	b->clearSelection(sendNotification);
}


void ScriptingApi::Content::Helpers::deleteComponent(Content* c, const Identifier& id, NotificationType /*rebuildContent*//*=sendNotification*/)
{
	auto childToRemove = c->getValueTreeForComponent(id);

	auto b = c->getProcessor()->getMainController()->getScriptComponentEditBroadcaster();

	childToRemove.getParent().removeChild(childToRemove, &b->getUndoManager());
}

bool ScriptingApi::Content::Helpers::renameComponent(Content* c, const Identifier& id, const Identifier& newId)
{
	auto existingTree = c->getValueTreeForComponent(newId);

	if (existingTree.isValid())
	{
		PresetHandler::showMessageWindow("Existing ID", "The ID " + newId.toString() + " already exists. Pick another one.");
		return false;
	}

	auto childTree = c->getValueTreeForComponent(id);

	auto& undoManager = c->getProcessor()->getMainController()->getScriptComponentEditBroadcaster()->getUndoManager();

	if (childTree.isValid())
	{
		childTree.setProperty("id", newId.toString(), &undoManager);

		for (int i = 0; i < childTree.getNumChildren(); i++)
		{
			auto child = childTree.getChild(i);
			child.setProperty("parentComponent", newId.toString(), &undoManager);
		}
	}

	return true;
}


bool ScriptingApi::Content::Helpers::callRecursive(ValueTree& v, const ValueTreeIteratorFunction& f)
{
	if (!f(v))
		return false;

	for (auto child : v)
	{
		if (!callRecursive(child, f))
			return false;
	}

	return true;
}

void ScriptingApi::Content::Helpers::duplicateSelection(Content* c, ReferenceCountedArray<ScriptComponent> selection, int deltaX, int deltaY, UndoManager* undoManager)
{
	Array<Identifier> newIds;
	newIds.ensureStorageAllocated(selection.size());

	Array<var> duplicationValues;
	duplicationValues.ensureStorageAllocated(selection.size());

	for (auto b : selection)
		duplicationValues.add(b->getValue());

	static const Identifier x("x");
	static const Identifier y("y");

	ValueTreeUpdateWatcher::ScopedDelayer sd(c->getUpdateWatcher());

	for (auto sc : selection)
	{
		int newX = sc->getPosition().getX() + deltaX;
		int newY = sc->getPosition().getY() + deltaY;

		

		auto cTree = c->getValueTreeForComponent(sc->name);

		ValueTree sibling = cTree.createCopy();
		
		sibling.setProperty(x, newX, undoManager);
		sibling.setProperty(y, newY, undoManager);

		

		cTree.getParent().addChild(sibling, -1, undoManager);

		auto tmp = &newIds;

		auto setUniqueId = [c, undoManager, tmp](ValueTree& v)
		{
			auto newId = getUniqueIdentifier(c, v.getProperty("id"));

			tmp->add(newId);

			v.setProperty("id", newId.toString(), undoManager);
			return true;
		};

		callRecursive(sibling, setUniqueId);

		auto updateParentComponentId = [c, undoManager](ValueTree& v)
		{
			auto pId = v.getParent().getProperty("id");

			if (pId.isUndefined())
			{
				jassertfalse;
				return false;
			}

			v.setProperty("parentComponent", pId, undoManager);
			return true;
		};

		for (auto child : sibling)
			callRecursive(child, updateParentComponentId);

	}

	auto b = c->getProcessor()->getMainController()->getScriptComponentEditBroadcaster();

	b->clearSelection(dontSendNotification);
	
	auto f = [newIds, c, b, duplicationValues]()
	{
		ScriptComponentSelection newSelection;

		for (auto id : newIds)
		{
			auto sc = c->getComponentWithName(id);
			jassert(sc != nullptr);
			newSelection.add(sc);
		}

		for (int i = 0; i < newSelection.size(); i++)
			newSelection[i]->setValue(duplicationValues[i]);

		b->setSelection(newSelection, sendNotification);

		
	};

	

	MessageManager::callAsync(f);
}



void ScriptingApi::Content::Helpers::createNewComponentData(Content* c, ValueTree& p, const String& typeName, const String& id)
{
	auto b = c->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster();
	auto undoManager = &b->getUndoManager();

	ValueTree n("Component");
	n.setProperty("type", typeName, nullptr);
	n.setProperty("id", id, nullptr);
	n.setProperty("x", 0, nullptr);
	n.setProperty("y", 0, nullptr);
	n.setProperty("width", 100, nullptr);
	n.setProperty("height", 100, nullptr);

	jassert(p == c->contentPropertyData || p.isAChildOf(c->contentPropertyData));

	p.addChild(n, -1, undoManager);
}

String ScriptingApi::Content::Helpers::createScriptVariableDeclaration(ReferenceCountedArray<ScriptComponent> selection)
{
	String s;
	NewLine nl;

	String variableName = selection.size() == 1 ? "" : PresetHandler::getCustomName("Array", "Enter the name for the array variable or nothing for a list of single statements");

	if (selection.size() == 1 || variableName.isEmpty())
	{
		for (int i = 0; i < selection.size(); i++)
		{
			auto c = selection[i];

			s << "const var " << c->name.toString() << " = Content.getComponent(\"" << c->name.toString() << "\");" << nl;;
		}

		s << nl;

		
	}
	else
	{
		s << "const var " << variableName << " = [";

		int length = s.length();

		for (int i = 0; i < selection.size(); i++)
		{
			auto c = selection[i];

			s << "Content.getComponent(\"" << c->name.toString() << "\")";

			if (i != selection.size() - 1)
			{
				s << "," << nl;
				
				for (int j = 0; j < length; j++)
					s << " ";
			}
		}

		s << "];" << nl;
	}

	return s;
}

bool ScriptingApi::Content::Helpers::hasLocation(ScriptComponent* sc)
{
	if (sc == nullptr)
		return false;

	Processor* p = dynamic_cast<Processor*>(const_cast<ProcessorWithScriptingContent*>(sc->parent->getScriptProcessor()));
	auto jp = dynamic_cast<JavascriptProcessor*>(p);

	auto engine = jp->getScriptEngine();

	if (engine == nullptr)
		return false;

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		auto info = engine->getDebugInformation(i);

		if (info->getObject() == sc)
		{
			return true;
		}
	}

	return false;
}

void ScriptingApi::Content::Helpers::gotoLocation(ScriptComponent* sc)
{
	if (sc == nullptr)
		return;

	Processor* p = dynamic_cast<Processor*>(const_cast<ProcessorWithScriptingContent*>(sc->parent->getScriptProcessor()));
	auto jp = dynamic_cast<JavascriptProcessor*>(p);

	auto engine = jp->getScriptEngine();

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		auto info = engine->getDebugInformation(i);

		if (info->getObject() == sc)
		{
			auto location = info->getObject()->getLocation();

			DebugableObject::Helpers::gotoLocation(p, info.get());

			return;
		}
	}

	PresetHandler::showMessageWindow("Can't find script location", "The variable definition can't be found", PresetHandler::IconType::Warning);
}

void ScriptingApi::Content::Helpers::recompileAndSearchForPropertyChange(ScriptComponent * sc, const Identifier& id)
{
	sc->setPropertyToLookFor(id);

	auto jp = dynamic_cast<JavascriptProcessor*>(const_cast<ProcessorWithScriptingContent*>(sc->getScriptProcessor()));

	auto f = [jp, sc](const JavascriptProcessor::SnippetResult& result)
	{
		DebugableObject::Helpers::gotoLocation(dynamic_cast<Processor*>(jp)->getMainController()->getMainSynthChain(), result.r.getErrorMessage());
		sc->setPropertyToLookFor(Identifier());
		jp->compileScript();
	};

	jp->compileScript(f);
}

String ScriptingApi::Content::Helpers::createLocalLookAndFeelForComponents(ReferenceCountedArray<ScriptComponent> selection)
{
    String code;

#if USE_BACKEND
    
    NewLine nl;
    Random r;
    
    String lafName = "local_laf" + String(r.nextInt({0, 999}));
    
    lafName = PresetHandler::getCustomName(lafName, "Enter the variable name for the LAF object (or press OK to use the random generated name).");
    
    if(lafName.isEmpty())
        return {};
    
    code << "const var " << lafName << " = Content.createLocalLookAndFeel();" << nl << nl;
    
    Array<Identifier> ids;
    
    for(auto sc: selection)
        ids.addIfNotAlreadyThere(sc->getObjectName());
    
    auto addCallback = [&](const String& callbackName, const StringArray& additionalLines={})
    {
        String nlt = "\n\t";
        code << lafName << ".registerFunction(" << callbackName.quoted() << ", function(g, obj)" << nl;
        code << "{";
        
        if(additionalLines.isEmpty())
        {
            code << nlt << "g.setColour(obj.bgColour);";
            code << nlt << "g.fillRect(obj.area);";
            code << nlt << "g.setColour(obj.textColour);";
            code << nlt << "g.drawAlignedText(obj.text, obj.area, \"centred\");";
        }
        else
        {
            for(const auto& l: additionalLines)
                code << nlt << l << ";";
        }
        
        code << nl << "});" << nl << nl;
    };
    
    if(ids.contains(ScriptSlider::getStaticObjectName()))
    {
        bool addRotary = false;
        bool addLinear = false;
        
        for(auto sc: selection)
        {
            auto style = sc->getScriptObjectProperty(ScriptSlider::Properties::Style).toString();
            
            addRotary |= style == "Knob";
            addLinear |= style != "Knob";
        }
        
        if(addRotary)
            addCallback("drawRotarySlider");
        
        if(addLinear)
            addCallback("drawLinearSlider");
    }
    if(ids.contains(ScriptButton::getStaticObjectName()))
        addCallback("drawToggleButton");
    if(ids.contains(ScriptComboBox::getStaticObjectName()))
        addCallback("drawComboBox");
    if(ids.contains(ScriptSliderPack::getStaticObjectName()))
    {
        addCallback("drawSliderPackBackground", {
            "g.setColour(obj.bgColour)",
            "g.fillRect(obj.area)",
            "g.setColour(obj.itemColour)",
            "g.drawRect(obj.area, 1.0)"
        });
        
        addCallback("drawSliderPackFlashOverlay", {
            "g.setColour(Colours.withAlpha(obj.itemColour, obj.intensity))",
            "g.fillRect(obj.area)"
        });
        
        addCallback("drawSliderPackRightClickLine", {
            "g.setColour(obj.itemColour)",
            "g.drawLine(obj.x1, obj.x2, obj.y1, obj.y2, 2.0)"
        });
        
        addCallback("drawSliderPackTextPopup", {
            "g.setColour(obj.textColour)",
            "g.fillRect([0, 0, obj.area[2], 14])",
            "g.drawAlignedText(obj.text, obj.area, \"topRight\")"
        });
        
        addCallback("drawLinearSlider", {
            "g.setColour(obj.itemColour2)",
            "obj.area[1] = (1.0 - obj.valueNormalized) * obj.area[3]",
            "obj.area[3] -= obj.area[1]",
            "g.fillRect(obj.area)"
        });
    }
        
       
    if(selection.size() > 1)
    {
        String arrayDef;
        
        arrayDef << "const var " << lafName << "_targets = [";
        
        String intend;
        
        for(int i = 0; i < arrayDef.length(); i++)
            intend << " ";
        
        code << arrayDef;;
        
        for(auto sc: selection)
        {
            code << "Content.getComponent(" << sc->getId().quoted() << ")";
            
            if(sc != selection.getLast().get())
                code << ", " << nl << intend;
        }
           
        code << "];" << nl << nl;
        
        code << "for(c in " << lafName << "_targets)" << nl;
        code << "    c.setLocalLookAndFeel(" << lafName << ");" << nl;
    }
    else
    {
        code << "Content.getComponent(" << selection.getFirst()->getId().quoted()
             << ").setLocalLookAndFeel(" << lafName << ");" << nl;
    }
    
#endif
    
    return code;
}

String ScriptingApi::Content::Helpers::createCustomCallbackDefinition(ReferenceCountedArray<ScriptComponent> selection)
{
	NewLine nl;
	String code;

	for (int i = 0; i < selection.size(); i++)
	{
		auto c = selection[i];

		auto name = c->getName();

		String callbackName = "on" + name.toString() + "Control";

		code << nl;
		code << "inline function " << callbackName << "(component, value)" << nl;
		code << "{" << nl;
		code << "\t//Add your custom logic here..." << nl;
		code << "};" << nl;
		code << nl;
		code << "Content.getComponent(\"" << name.toString() << "\").setControlCallback(" << callbackName << ");" << nl;

	}

	return code;
	
}

void ScriptingApi::Content::Helpers::setComponentValueTreeFromJSON(Content* c, const Identifier& id, const var& data, UndoManager* undoManager)
{
	auto oldTree = c->getValueTreeForComponent(id);

	auto parent = oldTree.getParent();

	int index = parent.indexOf(oldTree);

	

	parent.removeChild(oldTree, undoManager);

	auto newTree = ValueTreeConverters::convertDynamicObjectToContentProperties(data);

	parent.addChild(newTree, index, undoManager);
}


Result ScriptingApi::Content::Helpers::setParentComponent(ScriptComponent* parent, var newChildren)
{
	static const Identifier x("x");
	static const Identifier y("y");

	if (auto ar = newChildren.getArray())
	{
		ScriptingApi::Content* content = nullptr;

		if (parent != nullptr)
		{
			content = parent->parent;

			auto pTree = content->getValueTreeForComponent(parent->getName());

			for (auto it : *ar)
			{
				auto c = dynamic_cast<ScriptComponent*>(it.getObject());

				if (c == nullptr)
					continue;

				auto cTree = content->getValueTreeForComponent(c->getName());

				jassert(cTree.isValid());

				if (pTree.isAChildOf(cTree))
				{
					return Result::fail("Can't set a child as a parent of its parent");
				}

				if (cTree.getParent() == pTree)
				{
					// nothing do to...
					continue;
				}

				Point<int> cPosition = ContentValueTreeHelpers::getLocalPosition(cTree);
				ContentValueTreeHelpers::getAbsolutePosition(cTree, cPosition);

				Point<int> pPosition(pTree.getProperty(x), pTree.getProperty(y));
				ContentValueTreeHelpers::getAbsolutePosition(pTree, pPosition);

				ContentValueTreeHelpers::updatePosition(cTree, cPosition, pPosition);
				ContentValueTreeHelpers::setNewParent(pTree, cTree);
			}
		}
		else
		{
			auto firstInList = ar->getFirst();

			if (auto firstC = dynamic_cast<ScriptComponent*>(firstInList.getObject()))
			{
				content = firstC->parent;

				for (auto it : *ar)
				{
					auto c = dynamic_cast<ScriptComponent*>(it.getObject());

					if (c == nullptr)
						continue;

					auto cTree = content->getValueTreeForComponent(c->getName());

					jassert(cTree.isValid());

					Point<int> cPosition = ContentValueTreeHelpers::getLocalPosition(cTree);
					ContentValueTreeHelpers::getAbsolutePosition(cTree, cPosition);
					ContentValueTreeHelpers::updatePosition(cTree, cPosition, Point<int>());

					ContentValueTreeHelpers::setNewParent(content->contentPropertyData, cTree);
				}
			}
		}

		if (content == nullptr)
			return Result::fail("Nothing done");

		content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);

		return Result::ok();
	}

	return Result::fail("Invalid var");
}

Result ScriptingApi::Content::Helpers::setParentComponent(Content* content, const var& parentId, const var& childIdList)
{
	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier root("root");

	Identifier pId = Identifier(parentId.toString());

	auto pTree = content->getValueTreeForComponent(pId);

	if (pTree.isValid() && childIdList.isArray())
	{
		auto ar = childIdList.getArray();

		for (auto it : *ar)
		{
			const Identifier cId = Identifier(it.toString());

			auto cTree = content->getValueTreeForComponent(cId);

			jassert(cTree.isValid());

			if (pTree.isAChildOf(cTree))
			{
				return Result::fail("Can't set a child as a parent of its parent");
			}

			if (cTree.getParent() == pTree)
			{
				// nothing do to...
				continue;
			}

			Point<int> cPosition = ContentValueTreeHelpers::getLocalPosition(cTree);
			ContentValueTreeHelpers::getAbsolutePosition(cTree, cPosition);

			Point<int> pPosition(pTree.getProperty(x), pTree.getProperty(y));
			ContentValueTreeHelpers::getAbsolutePosition(pTree, pPosition);

			ContentValueTreeHelpers::updatePosition(cTree, cPosition, pPosition);
			ContentValueTreeHelpers::setNewParent(pTree, cTree);
		}
	}
	else if (pId == root && childIdList.isArray())
	{
		auto ar = childIdList.getArray();

		for (auto it : *ar)
		{
			const Identifier cId = Identifier(it.toString());

			auto cTree = content->getValueTreeForComponent(cId);

			jassert(cTree.isValid());

			Point<int> cPosition = ContentValueTreeHelpers::getLocalPosition(cTree);
			ContentValueTreeHelpers::getAbsolutePosition(cTree, cPosition);
			ContentValueTreeHelpers::updatePosition(cTree, cPosition, Point<int>());
			ContentValueTreeHelpers::setNewParent(content->contentPropertyData, cTree);
		}
	}

	content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);

	return Result::ok();
}

ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::Helpers::createComponentFromValueTree(Content* c, const ValueTree& v)
{
	static const Identifier x_("x");
	static const Identifier y_("y");
	static const Identifier width_("width");
	static const Identifier height_("height");
	static const Identifier id_("id");
	static const Identifier type_("type");

	auto typeId = Identifier(v.getProperty(type_).toString());
	auto name = Identifier(v.getProperty(id_).toString());
	auto x = (int)v.getProperty(x_);
	auto y = (int)v.getProperty(y_);
	auto w = (int)v.getProperty(width_);
	auto h = (int)v.getProperty(height_);

	if (auto sc = createComponentIfTypeMatches<ScriptSlider>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptButton>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptLabel>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptComboBox>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptTable>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptSliderPack>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptImage>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptPanel>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptedViewport>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptAudioWaveform>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptWebView>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptFloatingTile>(c, typeId, name, x, y, w, h))
		return sc;

	return nullptr;
}

void ScriptingApi::Content::Helpers::sanitizeNumberProperties(juce::ValueTree copy)
{
	for (int i = 0; i < copy.getNumProperties(); i++)
	{
		auto id = copy.getPropertyName(i);
		const bool isNumberProperty = ScriptComponent::numberPropertyIds.contains(id);

		if (isNumberProperty)
		{
			float valueAsNumber = (float)copy.getProperty(id);
			valueAsNumber = FloatSanitizers::sanitizeFloatNumber(valueAsNumber);
			copy.setProperty(id, var(valueAsNumber), nullptr);
		}
	}

	for (int i = 0; i < copy.getNumChildren(); i++)
		sanitizeNumberProperties(copy.getChild(i));
}

juce::Colour ScriptingApi::Content::Helpers::getCleanedObjectColour(const var& value)
{
	int64 colourValue = 0;

	if (value.isInt64() || value.isInt())
		colourValue = (int64)value;
	else if (value.isString())
	{
		auto string = value.toString();

		if (string.startsWith("0x"))
			colourValue = string.getHexValue64();
		else
			colourValue = string.getLargeIntValue();
	}

	return Colour((uint32)colourValue);
}

var ScriptingApi::Content::Helpers::getCleanedComponentValue(const var& data, bool allowStrings)
{
	if (data.isString() && (data.toString().startsWith("JSON") || allowStrings))
	{
		if (data.toString().startsWith("JSON"))
		{
			String jsonData = data.toString().fromFirstOccurrenceOf("JSON", false, false);
			return JSON::fromString(jsonData);
		}
		else
		{
			return data;
		}
	}
	else
	{
		float d = (float)data;
		FloatSanitizers::sanitizeFloatNumber(d);
		return var(d);
	}
}

bool ScriptingApi::Content::interfaceCreationAllowed() const
{
	return allowGuiCreation;
}

void ScriptingApi::Content::addPanelPopup(ScriptPanel* panel, bool closeOther)
{
	if (closeOther)
	{
		for (auto p : popupPanels)
		{
			if (p == panel)
				continue;

			p->closeAsPopup();
		}
				

		popupPanels.clear();
	}

	popupPanels.add(panel);
}

void ScriptingApi::Content::setIsRebuilding(bool isCurrentlyRebuilding)
{
	for(auto rl: rebuildListeners)
	{
		if(rl.get() != nullptr)
			rl->contentRebuildStateChanged(isCurrentlyRebuilding);
	}
}


hise::ScriptComponentPropertyTypeSelector::SelectorTypes ScriptComponentPropertyTypeSelector::getTypeForId(const Identifier &id) const
{
	if (toggleProperties.contains(id)) return ToggleSelector;
	else if (sliderProperties.contains(id)) return SliderSelector;
	else if (colourProperties.contains(id)) return ColourPickerSelector;
	else if (choiceProperties.contains(id)) return ChoiceSelector;
	else if (multilineProperties.contains(id)) return MultilineSelector;
	else if (fileProperties.contains(id)) return FileSelector;
	else if (codeProperties.contains(id)) return CodeSelector;
	else return TextSelector;
}

void ScriptComponentPropertyTypeSelector::addToTypeSelector(SelectorTypes type, Identifier id, double min /*= 0.0*/, double max /*= 1.0*/, double interval /*= 0.01*/)
{
	switch (type)
	{
	case ScriptComponentPropertyTypeSelector::ToggleSelector:
		toggleProperties.addIfNotAlreadyThere(id);
		break;
	case ScriptComponentPropertyTypeSelector::ColourPickerSelector:
		colourProperties.addIfNotAlreadyThere(id);
		break;
	case ScriptComponentPropertyTypeSelector::SliderSelector:
	{
		sliderProperties.addIfNotAlreadyThere(id);
		SliderRange range;

		range.min = min;
		range.max = max;
		range.interval = interval;

		sliderRanges.set(id.toString(), range);
		break;
	}
	case ScriptComponentPropertyTypeSelector::ChoiceSelector:
		choiceProperties.addIfNotAlreadyThere(id);
		break;
	case ScriptComponentPropertyTypeSelector::MultilineSelector:
		multilineProperties.addIfNotAlreadyThere(id);
		break;
	case ScriptComponentPropertyTypeSelector::FileSelector:
		fileProperties.addIfNotAlreadyThere(id);
		break;
	case ScriptComponentPropertyTypeSelector::TextSelector:
		break;
	case ScriptComponentPropertyTypeSelector::CodeSelector:
		codeProperties.addIfNotAlreadyThere(id);
	case ScriptComponentPropertyTypeSelector::numSelectorTypes:
		break;
	default:
		break;
	}
}


void ContentValueTreeHelpers::removeFromParent(ValueTree& v)
{
	v.getParent().removeChild(v, nullptr);
}

Result ContentValueTreeHelpers::setNewParent(ValueTree& newParent, ValueTree& child)
{
	if (newParent.isAChildOf(child))
	{
		return Result::fail("Can't set child as parent of child");
	}

	if (child.getParent() == newParent)
		return Result::ok();

	removeFromParent(child);
	newParent.addChild(child, -1, nullptr);

	return Result::ok();
}

void ContentValueTreeHelpers::updatePosition(ValueTree& v, Point<int> localPoint, Point<int> oldParentPosition)
{
	static const Identifier x("x");
	static const Identifier y("y");

	auto newPoint = localPoint - oldParentPosition;

	v.setProperty(x, newPoint.getX(), nullptr);
	v.setProperty(y, newPoint.getY(), nullptr);
}

Point<int> ContentValueTreeHelpers::getLocalPosition(const ValueTree& v)
{
	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier root("ContentProperties");

	if (v.getType() == root)
	{
		return Point<int>();
	}

	return Point<int>(v.getProperty(x), v.getProperty(y));
}

bool ContentValueTreeHelpers::isShowing(const ValueTree& v)
{
	static const Identifier visible("visible");
	static const Identifier co("Component");

	if (v.getProperty(visible, true))
	{
		auto p = v.getParent();

		if (p.getType() == co)
			return isShowing(p);
		else
			return true;
	}
	else
	{
		return false;
	}
}

bool ContentValueTreeHelpers::getAbsolutePosition(const ValueTree& v, Point<int>& offset)
{
	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier root("ContentProperties");

	auto parent = v.getParent();

	bool ok = parent.isValid() && parent.getType() != root;

	while (parent.isValid() && parent.getType() != root)
	{
		offset.addXY((int)parent.getProperty(x), (int)parent.getProperty(y));
		parent = parent.getParent();
	}

	return ok;
}

MapItemWithScriptComponentConnection::MapItemWithScriptComponentConnection(ScriptComponent* c, int width, int height) :
	SimpleTimer(c->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	sc(c),
	w(width),
	h(height)
{
	
}

SimpleVarBody::SimpleVarBody(const var& v) :
	value(v)
{
	s = getSensibleStringRepresentation();
}

void SimpleVarBody::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawText(s, getLocalBounds().toFloat(), Justification::centred);
}

void SimpleVarBody::mouseDown(const MouseEvent& e)
{
	auto isViewable = value.getDynamicObject() != nullptr || value.isArray();

	if (isViewable)
	{
		auto ft = findParentComponentOfClass<FloatingTile>();

		auto editor = new JSONEditor(value);
		editor->setSize(600, 400);
		editor->setName("JSON Viewer");

		ft->showComponentInRootPopup(editor, this, { getWidth() / 2, getHeight() });
	}
}

String SimpleVarBody::getSensibleStringRepresentation() const
{
	if (auto dObj = dynamic_cast<DebugableObjectBase*>(value.getObject()))
	{
		return dObj->getDebugName();
	}

	return value.toString();
}

LiveUpdateVarBody::DisplayType LiveUpdateVarBody::getDisplayType(const Identifier& id)
{
	if (id.toString().containsIgnoreCase("colour"))
		return DisplayType::Colour;

	static const Array<Identifier> boolIds = { "visible", "enabled", "on" };

	if (boolIds.contains(id))
		return DisplayType::Bool;

	return DisplayType::Text;
}



LiveUpdateVarBody::LiveUpdateVarBody(PooledUIUpdater* updater, const Identifier& id_, const std::function<var()>& f) :
	SimpleVarBody(f()),
	SimpleTimer(updater),
	id(id_),
	valueFunction(f),
	displayType(getDisplayType(id_))
{

}

void LiveUpdateVarBody::timerCallback()
{
	auto newValue = valueFunction();

	if (value != newValue)
	{
		alpha.setModValue(1.0);
		value = newValue;

		if (displayType == DisplayType::Colour)
            s = "colour";
		else if (displayType == DisplayType::Bool)
			s = (bool)newValue ? "true" : "";
		else
			s = getSensibleStringRepresentation();

		if (getPreferredWidth() > getWidth())
			resetRootSize();
	}

	if (alpha.setModValueIfChanged(jmax(0.0, alpha.getModValue() - 0.05)))
		repaint();
}

String LiveUpdateVarBody::getTextToDisplay() const
{
	String text;

	if (id.isValid())
		text << id.toString() << ": ";

	if(displayType == DisplayType::Text)
		text << s;

	return text;
}

void LiveUpdateVarBody::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat().reduced(6.0f, 8.0f);

	auto a = (float)jlimit(0.0f, 1.0f, 0.5f + 0.5f * (float)alpha.getModValue());

	g.setColour(Colours::black.withAlpha(a * 0.7f));
	g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

	auto text = getTextToDisplay();
	g.setColour(Colours::white.withAlpha(a));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(text, b.reduced(12.0f, 4.0f), Justification::left);

	auto circle = b.removeFromRight(b.getHeight()).reduced(1.0f);

	switch (displayType)
	{
	case DisplayType::Colour:
	{
		g.drawEllipse(circle, 1.0f);
		g.setColour(scriptnode::PropertyHelpers::getColourFromVar(value));
		g.fillEllipse(circle.reduced(1.0f));
		break;
	}
	case DisplayType::Bool:
	{
		g.drawEllipse(circle, 1.0f);

		if(s == "true")
			g.fillEllipse(circle.reduced(2.0f));

		break;
    default: break;
	}
	}

	
}

PrimitiveArrayDisplay::PrimitiveArrayDisplay(Processor* jp, const var& obj) :
	SimpleTimer(jp->getMainController()->getGlobalUIUpdater()),
	SimpleVarBody(obj)
{
	lastValues.addArray(*obj.getArray());

	auto f = GLOBAL_MONOSPACE_FONT();

	h = roundToInt((float)value.size() * f.getHeight()) + 16;

	id = "data";

#if 0
	if (auto di = DebugableObject::Helpers::getDebugInformation(dynamic_cast<ApiProviderBase::Holder*>(jp)->getProviderBase(), obj))
	{
		id = di->getTextForName();
	}
	else
	{
	}
#endif

	w = 0;

	for (auto v : lastValues)
	{
		w = jmax(w, f.getStringWidth(v.toString()));
	}

	w += 80 + f.getStringWidth(id);
}

void PrimitiveArrayDisplay::timerCallback()
{
	bool changed = false;

	for (int i = 0; i < value.size(); i++)
	{
		changed |= lastValues[i] != value[i];
		lastValues.set(i, value[i]);
	}

	if (changed)
		repaint();
}

hise::ComponentWithPreferredSize* hise::PrimitiveArrayDisplay::create(Component* r, const var& obj)
{
	auto p = dynamic_cast<ProcessorHelpers::ObjectWithProcessor*>(r)->getProcessor();

	if (ScriptingObjects::ScriptBroadcaster::isPrimitiveArray(obj))
		return new PrimitiveArrayDisplay(p, obj);

	return nullptr;
}

void PrimitiveArrayDisplay::paint(Graphics& g)
{
	AttributedString c;

	auto f = GLOBAL_MONOSPACE_FONT();

	auto c1 = Colours::white.withAlpha(0.4f);
	auto c2 = Colours::white.withAlpha(0.8f);

	for (int i = 0; i < lastValues.size(); i++)
	{
		String s1, s2;

		s1 << id << "[" << i << "] = ";
		s2 << lastValues[i].toString() << "\n";

		c.append(s1, f, c1);
		c.append(s2, f, c2);
	}

	c.draw(g, getLocalBounds().toFloat().reduced(8));
}



} // namespace hise
