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




#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))



#include <cmath>

namespace hise { using namespace juce;


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
	API_VOID_METHOD_WRAPPER_0(ScriptComponent, changed);
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionX);
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionY);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setControlCallback);
	API_METHOD_WRAPPER_0(ScriptComponent, getAllProperties);
};

#define ADD_SCRIPT_PROPERTY(id, name) static const Identifier id(name); propertyIds.add(id);
#define ADD_NUMBER_PROPERTY(id, name) ADD_SCRIPT_PROPERTY(id, name); jassert(numberPropertyIds.contains(id));

ScriptingApi::Content::ScriptComponent::ScriptComponent(ProcessorWithScriptingContent* base, Identifier name_, int numConstants /*= 0*/) :
	ConstScriptingObject(base, numConstants),
	UpdateDispatcher::Listener(base->getScriptingContent()->getUpdateDispatcher()),
	name(name_),
	parent(base->getScriptingContent()),
	controlSender(this, base),
	propertyTree(name_.isValid() ? parent->getValueTreeForComponent(name) : ValueTree("Component")),
	value(0.0),
	skipRestoring(false),
	hasChanged(false),
	customControlCallback(var())
{



	jassert(propertyTree.isValid());

	ADD_SCRIPT_PROPERTY(textId, "text");

	ADD_SCRIPT_PROPERTY(vId, "visible");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(eId, "enabled");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
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
	ADD_SCRIPT_PROPERTY(pId72, "linkedTo")			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(uId, "useUndoManager");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId2, "parentComponent");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId3, "processorId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId4, "parameterId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	handleDefaultDeactivatedProperties();


	setDefaultValue(Properties::text, name.toString());
	setDefaultValue(Properties::visible, true);
	setDefaultValue(Properties::enabled, true);
	
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
	setDefaultValue(Properties::linkedTo, "");
	setDefaultValue(Properties::useUndoManager, false);
	setDefaultValue(Properties::parentComponent, "");
	setDefaultValue(Properties::processorId, " ");
	setDefaultValue(Properties::parameterId, "");

	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(get);
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
	ADD_API_METHOD_0(changed);
	ADD_API_METHOD_0(getGlobalPositionX);
	ADD_API_METHOD_0(getGlobalPositionY);
	ADD_API_METHOD_1(setControlCallback);
	ADD_API_METHOD_0(getAllProperties);

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
	else if (id == getIdFor(processorId))
	{
		return ProcessorHelpers::getListOfAllConnectableProcessors(dynamic_cast<Processor*>(getScriptProcessor()));
	}
	else if (id == getIdFor(parameterId) && connectedProcessor.get() != nullptr)
	{
		return ProcessorHelpers::getListOfAllParametersForProcessor(connectedProcessor.get());
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

	value = Content::Helpers::getCleanedComponentValue(data);

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
		id == getIdFor(height) || id == getIdFor(visible) || id == getIdFor(enabled))
	{
		
	}
	else if (id == getIdFor(processorId))
	{
		auto pId = newValue.toString();

		if (pId == " " || pId == "")
		{
			connectedProcessor = nullptr;
			setScriptObjectPropertyWithChangeMessage(getIdFor(parameterId), "", sendNotification);
		}
		else if (pId.isNotEmpty())
		{
			connectedProcessor = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), pId);
		}
	}
	else if (id == getIdFor(parameterId))
	{
		auto parameterName = newValue.toString();

		if (parameterName.isNotEmpty())
		{
			connectedParameterIndex = ProcessorHelpers::getParameterIndexFromProcessor(connectedProcessor, Identifier(parameterName));
		}
		else
			connectedParameterIndex = -1;
	}

	setScriptObjectProperty(propertyIds.indexOf(id), newValue, notifyEditor);
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

	return var(clone);
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

	handleScriptPropertyChange(propertyId);

	if (!defaultValues.contains(propertyId))
	{
		logErrorAndContinue("the property does not exist");
		RETURN_VOID_IF_NO_THROW();
	}

	setScriptObjectPropertyWithChangeMessage(propertyId, newValue, parent->allowGuiCreation ? dontSendNotification : sendNotification);
}

void ScriptingApi::Content::ScriptComponent::changed()
{
	if (!parent->asyncFunctionsAllowed())
		return;

	controlSender.sendControlCallbackMessage();
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

		if (p->getMainController_()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::ScriptingThread)
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
		ScopedLock sl(parent->lock);
		value = controlValue;
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

ScriptingApi::Content::ScriptComponent::~ScriptComponent()
{
	if (linkedComponent != nullptr)
		linkedComponent->removeLinkedTarget(this);
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
		DynamicObject::Ptr obj = new DynamicObject();

		for (const auto& v : idList)
		{
			auto id = getIdFor(v.id);
			var linkedValue = v.value.isUndefined() ? lc->getScriptObjectProperty(id) : v.value;

			obj->setProperty(id, linkedValue);
		}

		var obj_(obj);

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

void ScriptingApi::Content::ScriptComponent::sendSubComponentChangeMessage(ScriptComponent* s, bool wasAdded, NotificationType notify/*=sendNotificationAsync*/)
{
	WeakReference<ScriptComponent> ws(s);
	WeakReference<ScriptComponent> ts(this);

	auto f = [ts, ws, wasAdded]()
	{
		if (ts != nullptr && ws != nullptr)
		{
			for (auto l : ts->subComponentListeners)
			{
				if (l != nullptr)
				{
					if (wasAdded)
						l->subComponentAdded(ws);
					else
						l->subComponentRemoved(ws);
				}
			}
		}
	};

	if (notify == sendNotificationSync)
		f();
	else
		MessageManager::callAsync(f);
}

struct ScriptingApi::Content::ScriptSlider::Wrapper
{
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
	ADD_SCRIPT_PROPERTY(i14, "showTextBox"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

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
	const double midPoint = getScriptObjectProperty(middlePosition);
	const double step = getScriptObjectProperty(stepSize);

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

	handleDefaultDeactivatedProperties();

	initInternalPropertyFromValueTreeOrDefault(ScriptComponent::Properties::text);

	value = var("internal");

	ADD_API_METHOD_1(setEditable);
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

void ScriptingApi::Content::ScriptLabel::handleDefaultDeactivatedProperties()
{

	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
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

	priorityProperties.add(getIdFor(Items));

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 128);
	setDefaultValue(ScriptComponent::Properties::height, 32);
	setDefaultValue(Items, "");
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Default");
	setDefaultValue(ScriptComponent::Properties::defaultValue, 1);
	setDefaultValue(ScriptComponent::min, 1.0f);
	
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

	jassert((int)value <= items.size());

	return items[(int)value - 1];
}

void ScriptingApi::Content::ScriptComboBox::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(Items))
	{
		jassert(isCorrectlyInitialised(Items));

		setScriptObjectProperty(Items, newValue);
		setScriptObjectProperty(max, getItemList().size());
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
	default:		sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

struct ScriptingApi::Content::ScriptTable::Wrapper
{
	API_METHOD_WRAPPER_1(ScriptTable, getTableValue);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, setTablePopupFunction);
	API_VOID_METHOD_WRAPPER_2(ScriptTable, connectToOtherTable);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, setSnapValues);
	API_VOID_METHOD_WRAPPER_1(ScriptTable, referToData);
};

ScriptingApi::Content::ScriptTable::ScriptTable(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name, int x, int y, int , int ) :
ScriptComponent(base, name),
ownedTable(new MidiTable()),
useOtherTable(false),
connectedProcessor(nullptr),
lookupTableIndex(-1)
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

	ADD_API_METHOD_1(getTableValue);
	ADD_API_METHOD_2(connectToOtherTable);
	ADD_API_METHOD_1(setSnapValues);
	ADD_API_METHOD_1(referToData);
	ADD_API_METHOD_1(setTablePopupFunction);

	broadcaster.enablePooledUpdate(base->getMainController_()->getGlobalUIUpdater());
}

ScriptingApi::Content::ScriptTable::~ScriptTable()
{
	broadcaster.removeAllChangeListeners();

	ownedTable = nullptr;
	referencedTable = nullptr;
	connectedProcessor = nullptr;
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptTable::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::TableWrapper(content, this, index);
}

float ScriptingApi::Content::ScriptTable::getTableValue(int inputValue)
{
	value = inputValue;

	

	

	if (useOtherTable)
	{
		if (MidiTable *mt = dynamic_cast<MidiTable*>(referencedTable.get()))
		{
			return mt->get(inputValue);
		}
		else if (SampleLookupTable *st = dynamic_cast<SampleLookupTable*>(referencedTable.get()))
		{
			return st->getInterpolatedValue(inputValue);
		}
		else
		{
			logErrorAndContinue("Connected Table was not found!");
			
			return -1.0f;
		}
	}
	else
	{
		// Connected tables will handle the update automatically, but for owned tables
		// it must be done here explicitely...
		if (auto t = getTable())
		{
			broadcaster.table = t;
			broadcaster.tableIndex = (float)inputValue / (float)t->getTableSize();

			broadcaster.sendPooledChangeMessage();
		}

		return ownedTable->get(inputValue);
	}
}

StringArray ScriptingApi::Content::ScriptTable::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ScriptComponent::Properties::processorId)) return ScriptComponent::getOptionsFor(id);

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	return ProcessorHelpers::getAllIdsForType<LookupTableProcessor>(mp->getOwnerSynth());
};

void ScriptingApi::Content::ScriptTable::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	if (getIdFor(ScriptComponent::Properties::processorId) == id)
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::processorId));

		const int tableIndex = getScriptObjectProperty(TableIndex);

		connectToOtherTable(newValue, tableIndex);
	}
	else if (getIdFor(TableIndex) == id)
	{
		jassert(isCorrectlyInitialised(TableIndex));

		connectToOtherTable(getScriptObjectProperty(ScriptComponent::Properties::processorId), newValue);
	}
	else if (getIdFor(parameterId) == id)
	{
		// don't do anything if you try to connect a table to a parameter...
		return;
	}
	
	

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptTable::connectToOtherTable(const String &otherTableId, int index)
{
	if (otherTableId.isEmpty() || otherTableId == " ")
		return;

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return;

	Processor::Iterator<Processor> it(mp->getOwnerSynth(), false);

	Processor *p;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		if (dynamic_cast<LookupTableProcessor*>(p) != nullptr && p->getId() == otherTableId)
		{
			useOtherTable = true;

			referencedTable = dynamic_cast<LookupTableProcessor*>(p)->getTable(index);

			connectedProcessor = p;

			return;
		}

	}

	useOtherTable = false;
	referencedTable = nullptr;

    logErrorAndContinue(otherTableId + " was not found.");
}


void ScriptingApi::Content::ScriptTable::setSnapValues(var snapValueArray)
{
	if (!snapValueArray.isArray())
		reportScriptError("You must call setSnapValues with an array");

	snapValues = snapValueArray;
	
	// hack: use the unused parameterID property to update the snap values
	// (it will also be used for updating the text function)...
	getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}

LookupTableProcessor * ScriptingApi::Content::ScriptTable::getTableProcessor() const
{
	if (connectedProcessor == nullptr && useOtherTable)
		return const_cast<LookupTableProcessor*>(dynamic_cast<const LookupTableProcessor*>(getScriptProcessor()));

	return dynamic_cast<LookupTableProcessor*>(connectedProcessor.get());
}

ValueTree ScriptingApi::Content::ScriptTable::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	if (getTable() != nullptr) v.setProperty("data", getTable()->exportData(), nullptr);
	else jassertfalse;

	return v;
}

void ScriptingApi::Content::ScriptTable::restoreFromValueTree(const ValueTree &v)
{
	ScriptComponent::restoreFromValueTree(v);

	if (getTable() == nullptr)
		return;

	getTable()->restoreData(v.getProperty("data", String()));

	getTable()->sendChangeMessage();
}

Table * ScriptingApi::Content::ScriptTable::getTable()
{
	return useOtherTable ? referencedTable.get() : ownedTable;
}

const Table * ScriptingApi::Content::ScriptTable::getTable() const
{
	return useOtherTable ? referencedTable.get() : ownedTable;
}

void ScriptingApi::Content::ScriptTable::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(parameterId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
}

void ScriptingApi::Content::ScriptTable::resetValueToDefault()
{
	if (auto t = getTable())
	{
		t->reset();
		t->sendChangeMessage();
	}
}

void ScriptingApi::Content::ScriptTable::referToData(var tableData)
{
	if (auto td = dynamic_cast<ScriptingObjects::ScriptTableData*>(tableData.getObject()))
	{
		referencedTable = td->getTable();
		useOtherTable = true;
	}
	else
	{
		referencedTable = nullptr;
		useOtherTable = false;
		
		reportScriptError("Invalid table");
	}
}

void ScriptingApi::Content::ScriptTable::setTablePopupFunction(var newFunction)
{
	tableValueFunction = newFunction;
	getPropertyValueTree().sendPropertyChangeMessage(getIdFor(parameterId));
}

struct ScriptingApi::Content::ScriptSliderPack::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPack, setSliderAtIndex);
	API_METHOD_WRAPPER_1(ScriptSliderPack, getSliderValueAt);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setAllValues);
	API_METHOD_WRAPPER_0(ScriptSliderPack, getNumSliders);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, referToData);
    API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setWidthArray);
};

ScriptingApi::Content::ScriptSliderPack::ScriptSliderPack(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier name_, int x, int y, int , int ) :
ScriptComponent(base, name_),
packData(new SliderPackData(base->getMainController_()->getControlUndoManager(), base->getMainController_()->getGlobalUIUpdater())),
existingData(nullptr)
{
	

	ADD_NUMBER_PROPERTY(i00, "sliderAmount");		ADD_AS_SLIDER_TYPE(0, 128, 1);
	ADD_NUMBER_PROPERTY(i01, "stepSize");
	ADD_SCRIPT_PROPERTY(i02, "flashActive");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "showValueOverlay");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i05, "SliderPackIndex");     

	packData->setNumSliders(16);

	
	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
	setDefaultValue(bgColour, 0x00000000);
	setDefaultValue(itemColour, 0x77FFFFFF);
	setDefaultValue(itemColour2, 0x77FFFFFF);
	setDefaultValue(textColour, 0x33FFFFFF);

	setDefaultValue(SliderAmount, 0);
	setDefaultValue(StepSize, 0);
	setDefaultValue(FlashActive, true);
	setDefaultValue(ShowValueOverlay, true);
	
	setDefaultValue(SliderPackIndex, 0);

	setDefaultValue(SliderAmount, 16);
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

	ADD_API_METHOD_2(setSliderAtIndex);
	ADD_API_METHOD_1(getSliderValueAt);
	ADD_API_METHOD_1(setAllValues);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_1(referToData);
    ADD_API_METHOD_1(setWidthArray);
}

ScriptingApi::Content::ScriptSliderPack::~ScriptSliderPack()
{
	packData = nullptr;
	existingData = nullptr;
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptSliderPack::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::SliderPackWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptSliderPack::setSliderAtIndex(int index, double newValue)
{
	getSliderPackData()->setValue(index, (float)newValue, sendNotification);
}

double ScriptingApi::Content::ScriptSliderPack::getSliderValueAt(int index)
{
	SliderPackData *dataToUse = getSliderPackData();

	jassert(dataToUse != nullptr);

	dataToUse->setDisplayedIndex(index);

	return (double)dataToUse->getValue(index);
}

void ScriptingApi::Content::ScriptSliderPack::setAllValues(double newValue)
{
	SliderPackData *dataToUse = getSliderPackData();

	jassert(dataToUse != nullptr);

	for (int i = 0; i < dataToUse->getNumSliders(); i++)
	{
		dataToUse->setValue(i, (float)newValue, dontSendNotification);
	}

	getSliderPackData()->sendChangeMessage();
}

int ScriptingApi::Content::ScriptSliderPack::getNumSliders() const
{
	return getSliderPackData()->getNumSliders();
}

void ScriptingApi::Content::ScriptSliderPack::connectToOtherSliderPack(const String &newPackId)
{
	if (newPackId.isEmpty() || newPackId == " ")
		return;

	otherPackId = newPackId;

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return;

	Processor::Iterator<Processor> it(mp->getOwnerSynth(), false);

	Processor *p;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		if ((dynamic_cast<SliderPackProcessor*>(p) != nullptr) && p->getId() == newPackId)
		{
			existingData = dynamic_cast<SliderPackProcessor*>(p)->getSliderPackData(otherPackIndex);

			if (existingData == nullptr)
				logErrorAndContinue("Slider Pack for " + newPackId + " with index " + String(otherPackIndex) + " wasn't found");

			return;
		}
	}

	existingData = nullptr;

    logErrorAndContinue(newPackId + " was not found.");
}

StringArray ScriptingApi::Content::ScriptSliderPack::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(ScriptComponent::Properties::processorId))
	{
		MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
		if (mp == nullptr) return StringArray();

		return ProcessorHelpers::getAllIdsForType<SliderPackProcessor>(mp->getOwnerSynth());
	}
	else if(id == getIdFor(Properties::StepSize))
	{
		StringArray sa;

		sa.add("0.01");
		sa.add("0.1");
		sa.add("1.0");
		
		return sa;
	}
	
	return ScriptComponent::getOptionsFor(id);
};

void ScriptingApi::Content::ScriptSliderPack::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	static const Identifier oldProcessorId = Identifier("ProcessorId");

	if (id == getIdFor(SliderAmount))
	{
		jassert(isCorrectlyInitialised(SliderAmount));

		if (existingData.get() != nullptr) return;
		packData->setNumSliders(newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::min))
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::min));

		if (existingData.get() != nullptr) return;
		packData->setRange(newValue, packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::max))
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::min));

		if (existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), newValue, newValue);
	}
	else if (id == getIdFor(StepSize))
	{
		jassert(isCorrectlyInitialised(StepSize));

		if (existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(FlashActive))
	{
		jassert(isCorrectlyInitialised(FlashActive));

		if (existingData.get() != nullptr) return;
		packData->setFlashActive((bool)newValue);
	}
	else if (id == getIdFor(ShowValueOverlay))
	{
		jassert(isCorrectlyInitialised(ShowValueOverlay));

		if (existingData.get() != nullptr) return;
		packData->setShowValueOverlay((bool)newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::processorId) || id == oldProcessorId)
	{
		jassert(isCorrectlyInitialised(ScriptComponent::Properties::processorId));

		connectToOtherSliderPack(newValue.toString());
	}
	else if (getIdFor(parameterId) == id )
	{
		// don't do anything if you try to connect a slider pack to a parameter...
		return;
	}
	else if (id == getIdFor(SliderPackIndex))
	{
		jassert(isCorrectlyInitialised(SliderPackIndex));

		otherPackIndex = (int)newValue;
		connectToOtherSliderPack(otherPackId);
	}
	
	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

ValueTree ScriptingApi::Content::ScriptSliderPack::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	v.setProperty("data", getSliderPackData()->toBase64(), nullptr);

	return v;
}

void ScriptingApi::Content::ScriptSliderPack::restoreFromValueTree(const ValueTree &v)
{
	ScriptComponent::restoreFromValueTree(v);

	getSliderPackData()->fromBase64(v.getProperty("data", String()));
	getSliderPackData()->sendChangeMessage();
}

SliderPackData * ScriptingApi::Content::ScriptSliderPack::getSliderPackData()
{
	return (existingData != nullptr) ? existingData.get() : packData.get();
}

const SliderPackData * ScriptingApi::Content::ScriptSliderPack::getSliderPackData() const
{
	return (existingData != nullptr) ? existingData.get() : packData.get();
}

void ScriptingApi::Content::ScriptSliderPack::setValue(var newValue)
{
	ScriptComponent::setValue(newValue);

	if (auto array = newValue.getArray())
	{
		getSliderPackData()->swapData(*array);
		
		triggerAsyncUpdate();
	}
	else
	{
		logErrorAndContinue("You must call setValue() with an array for Slider Packs");
	}
}

var ScriptingApi::Content::ScriptSliderPack::getValue() const
{
	return getSliderPackData()->getDataArray();
}

void ScriptingApi::Content::ScriptSliderPack::referToData(var sliderPackData)
{
	if (auto spd = dynamic_cast<ScriptingObjects::ScriptSliderPackData*>(sliderPackData.getObject()))
	{
		existingData = spd->getSliderPackData();
		existingData->sendChangeMessage();
	}
	else
		logErrorAndContinue("not a valid SliderPackData object");
}

void ScriptingApi::Content::ScriptSliderPack::setWidthArray(var normalizedWidths)
{
    if(getNumSliders() != normalizedWidths.size() + 1)
    {
        logErrorAndContinue("Width array length must be numSliders + 1");
    }
    
    if(auto ar = normalizedWidths.getArray())
    {
        widthArray = *ar;
        sendChangeMessage();
    }
    
    
    
    
}
    
void ScriptingApi::Content::ScriptSliderPack::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(parameterId));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isPluginParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
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
	ADD_SCRIPT_PROPERTY(i04, "allowCallbacks");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "popupMenuItems");		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i06, "popupOnRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	priorityProperties.add(getIdFor(FileName));

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 50);
	setDefaultValue(ScriptComponent::Properties::height, 50);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
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

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptImage::setImageFile(const String &absoluteFileName, bool forceUseRealFile)
{
	ignoreUnused(forceUseRealFile);

	CHECK_COPY_AND_RETURN_10(getProcessor());

	if (absoluteFileName.isEmpty())
	{
		setScriptObjectProperty(FileName, absoluteFileName, sendNotification);
		image.clear();
		return;
	}

	setScriptObjectProperty(FileName, absoluteFileName, sendNotification);


	PoolReference ref(getProcessor()->getMainController(), absoluteFileName, ProjectHandler::SubDirectories::Images);
	image.clear();
	image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
};



ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptImage::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ImageWrapper(content, this, index);
}

const Image ScriptingApi::Content::ScriptImage::getImage() const
{
	return image ? *image.getData() :
		PoolHelpers::getEmptyImage(getScriptObjectProperty(ScriptComponent::Properties::width),
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
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
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
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, startTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, stopTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, changed);
	API_VOID_METHOD_WRAPPER_2(ScriptPanel, loadImage);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setDraggingBounds);
	API_VOID_METHOD_WRAPPER_2(ScriptPanel, setPopupData);
    API_VOID_METHOD_WRAPPER_3(ScriptPanel, setValueWithUndo);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, showAsPopup);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, closeAsPopup);
	API_METHOD_WRAPPER_0(ScriptPanel, addChildPanel);
	API_METHOD_WRAPPER_0(ScriptPanel, removeFromParent);
	API_METHOD_WRAPPER_0(ScriptPanel, getChildPanelList);
	API_METHOD_WRAPPER_0(ScriptPanel, getParentPanel);

#if HISE_INCLUDE_RLOTTIE
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setAnimation);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setAnimationFrame);
	API_METHOD_WRAPPER_0(ScriptPanel, getAnimationData);
#endif
	API_METHOD_WRAPPER_0(ScriptPanel, isVisibleAsPopup);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setIsModalPopup);
};

ScriptingApi::Content::ScriptPanel::ScriptPanel(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier panelName, int x, int y, int , int ) :
ScriptComponent(base, panelName, 1),
PreloadListener(base->getMainController_()->getSampleManager()),
graphics(new ScriptingObjects::GraphicsObject(base, this)),
isChildPanel(true)
{
	init();
}

ScriptingApi::Content::ScriptPanel::ScriptPanel(ScriptPanel* parent) :
	ScriptComponent(parent->getScriptProcessor(), {}, 1),
	PreloadListener(parent->getScriptProcessor()->getMainController_()->getSampleManager()),
	graphics(new ScriptingObjects::GraphicsObject(parent->getScriptProcessor(), this)),
	parentPanel(parent)
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

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 100);
	setDefaultValue(ScriptComponent::Properties::height, 50);
	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
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
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_2(loadImage);
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

#if HISE_INCLUDE_RLOTTIE
	ADD_API_METHOD_0(getAnimationData);
	ADD_API_METHOD_1(setAnimation);
	ADD_API_METHOD_1(setAnimationFrame);
#endif
}


ScriptingApi::Content::ScriptPanel::~ScriptPanel()
{
	if (parentPanel != nullptr)
		parentPanel->sendSubComponentChangeMessage(this, false, sendNotificationAsync);

	stopTimer();

	timerRoutine = var();
	mouseRoutine = var();
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
	internalRepaint(false);
}


void ScriptingApi::Content::ScriptPanel::repaintImmediately()
{
	internalRepaint(false);
}


void ScriptingApi::Content::ScriptPanel::setPaintRoutine(var paintFunction)
{
	paintRoutine = paintFunction;

	if (!parent->allowGuiCreation)
	{
		repaint();
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
	var arguments = var(graphics);
	var::NativeFunctionArgs args(thisObject, &arguments, 1);


	Result::ok();

	if (!engine->isInitialising())
	{
		engine->maximumExecutionTime = RelativeTime(0.2);
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
		loadRoutine = loadingCallback;
	}
    else
    {
        getScriptProcessor()->getMainController_()->getSampleManager().removePreloadListener(this);
        loadRoutine = var();
    }
    
}


void ScriptingApi::Content::ScriptPanel::preloadStateChanged(bool isPreloading)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadRoutine))
	{
		auto f = [this, isPreloading](JavascriptProcessor* )
		{
			Result r = Result::ok();
			preloadStateInternal(isPreloading, r);
			return r;
		};

		auto mc = getScriptProcessor()->getMainController_();
		
		mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
			dynamic_cast<JavascriptProcessor*>(getScriptProcessor()),
			f);
	}
}


void ScriptingApi::Content::ScriptPanel::preloadStateInternal(bool isPreloading, Result& r)
{
	jassert_locked_script_thread(getScriptProcessor()->getMainController_());

	var thisObject(this);
	var b(isPreloading);
	var::NativeFunctionArgs args(thisObject, &b, 1);

	auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

	jassert(engine != nullptr);

	if (engine != nullptr)
	{
		engine->maximumExecutionTime = RelativeTime(0.5);
		engine->callExternalFunction(loadRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}
	}
}



void ScriptingApi::Content::ScriptPanel::setMouseCallback(var mouseCallbackFunction)
{
	mouseRoutine = mouseCallbackFunction;
}


void ScriptingApi::Content::ScriptPanel::mouseCallbackInternal(const var& mouseInformation, Result& r)
{
	var thisObject(this);

	var::NativeFunctionArgs args(thisObject, &mouseInformation, 1);

	auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

	engine->maximumExecutionTime = RelativeTime(0.5);
	engine->callExternalFunction(mouseRoutine, args, &r);

	if (r.failed())
	{
		debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
	}
	
}


void ScriptingApi::Content::ScriptPanel::mouseCallback(var mouseInformation)
{
	const bool parentHasMovedOn = !isChildPanel && !parent->hasComponent(this);

	if (parentHasMovedOn || !parent->asyncFunctionsAllowed())
	{
		return;
	}

	if (HiseJavascriptEngine::isJavascriptFunction(mouseRoutine))
	{
		auto f = [this, mouseInformation](JavascriptProcessor*)
		{
			Result r = Result::ok();
			mouseCallbackInternal(mouseInformation, r);
			return r;
		};

		auto& tp = getScriptProcessor()->getMainController_()->getJavascriptThreadPool();

		tp.addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution, dynamic_cast<JavascriptProcessor*>(getScriptProcessor()), f);
	};
}

void ScriptingApi::Content::ScriptPanel::setTimerCallback(var timerCallback_)
{
	timerRoutine = timerCallback_;
}



void ScriptingApi::Content::ScriptPanel::timerCallback()
{
	auto mc = dynamic_cast<Processor*>(getScriptProcessor())->getMainController();

	if (mc == nullptr)
		return;

	WeakReference<ScriptPanel> tmp(this);

	auto f = [tmp, mc](JavascriptProcessor* )
	{
		Result r = Result::ok();

		if (auto panel = tmp.get())
			panel->timerCallbackInternal(mc, r);

		return r;
	};

	mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::LowPriorityCallbackExecution, dynamic_cast<JavascriptProcessor*>(getScriptProcessor()), f);
}



void ScriptingApi::Content::ScriptPanel::loadImage(String imageName, String prettyName)
{
	PoolReference ref(getProcessor()->getMainController(), imageName, ProjectHandler::SubDirectories::Images);

	for (const auto& img : loadedImages)
	{
		if (img.image.getRef() == ref)
			return;
	}

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	if (auto newImage = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref))
		loadedImages.add({ newImage, prettyName });
	else
	{
		BACKEND_ONLY(reportScriptError("Image " + imageName + " not found. "));
	}
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

void ScriptingApi::Content::ScriptPanel::setValueWithUndo(var oldValue, var newValue, var actionName)
{
    auto p = dynamic_cast<Processor*>(getScriptProcessor());
    
    auto sc = getScriptProcessor()->getScriptingContent();
    
    const int index = sc->getComponentIndex(getName());
    
    auto newEvent = new BorderPanel::UndoableControlEvent(p, index, (float)oldValue, (float)newValue);
    
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

void ScriptingApi::Content::ScriptPanel::repaintThisAndAllChildren()
{
	// Call repaint on all children to make sure they are updated...
	ChildIterator<ScriptPanel> iter(this);

	while (auto childPanel = iter.getNextChildComponent())
		childPanel->repaint();
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

		engine->maximumExecutionTime = RelativeTime(0.5);
		engine->callExternalFunction(timerRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}
	}

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
	return var(childPanels.getLast());
}

#if HISE_INCLUDE_RLOTTIE
var ScriptingApi::Content::ScriptPanel::getAnimationData()
{
	updateAnimationData();
	return var(animationData);
}

void ScriptingApi::Content::ScriptPanel::setAnimation(String base64LottieAnimation)
{
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
}

void ScriptingApi::Content::ScriptPanel::setAnimationFrame(int numFrame)
{
	if (animation != nullptr)
	{
		animation->setFrame(numFrame);
		updateAnimationData();
		graphics->getDrawHandler().flush();
	}
}

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

	animationData = var(obj);
}

bool ScriptingApi::Content::ScriptPanel::removeFromParent()
{
	if (parentPanel != nullptr && (parentPanel->childPanels.indexOf(this) != -1))
	{
		parentPanel->sendSubComponentChangeMessage(this, false, sendNotificationAsync);
		parentPanel->childPanels.removeAllInstancesOf(this);
		parentPanel = nullptr;
		return true;
	}

	return false;
}

var ScriptingApi::Content::ScriptPanel::getChildPanelList()
{
	Array<var> cp;

	for (auto p : childPanels)
		cp.add(var(p.get()));

	return cp;
}

var ScriptingApi::Content::ScriptPanel::getParentPanel()
{
	if (parentPanel != nullptr)
		return var(parentPanel);

	return {};
}

#endif

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptedViewport::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ViewportWrapper(content, this, index);
}

ScriptingApi::Content::ScriptedViewport::ScriptedViewport(ProcessorWithScriptingContent* base, Content* /*parentContent*/, Identifier viewportName, int x, int y, int , int ):
	ScriptComponent(base, viewportName)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("scrollBarThickness");		ADD_AS_SLIDER_TYPE(0, 40, 1);
	propertyIds.add("autoHide");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("useList"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("items"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i01, "fontName");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_NUMBER_PROPERTY(i02, "fontSize");		ADD_AS_SLIDER_TYPE(1, 200, 1);
	ADD_SCRIPT_PROPERTY(i03, "fontStyle");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "alignment");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	setDefaultValue(ScriptComponent::Properties::x, x);
	setDefaultValue(ScriptComponent::Properties::y, y);
	setDefaultValue(ScriptComponent::Properties::width, 200);
	setDefaultValue(ScriptComponent::Properties::height, 100);
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

ScriptingApi::Content::ScriptAudioWaveform::ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content* /*parentContent*/, Identifier waveformName, int x, int y, int , int ) :
ScriptComponent(base, waveformName)
{
	ADD_SCRIPT_PROPERTY(i01, "itemColour3"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(i02, "opaque"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "showLines"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i04, "showFileName"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
    ADD_SCRIPT_PROPERTY(i05, "sampleIndex");
    
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
    setDefaultValue(Properties::sampleIndex, -1);

	handleDefaultDeactivatedProperties();
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptAudioWaveform::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::AudioWaveformWrapper(content, this, index);
}


ValueTree ScriptingApi::Content::ScriptAudioWaveform::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	const AudioSampleProcessor *asp = dynamic_cast<const AudioSampleProcessor*>(getConnectedProcessor());

	if (asp != nullptr)
	{
		v.setProperty("rangeStart", asp->getRange().getStart(), nullptr);
		v.setProperty("rangeEnd", asp->getRange().getEnd(), nullptr);
		v.setProperty("fileName", asp->getFileName(), nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptAudioWaveform::restoreFromValueTree(const ValueTree &v)
{
	const String id = v.getProperty("id", "");

	if (id.isNotEmpty())
	{
		const String fileName = v.getProperty("fileName", "");

		if (getAudioProcessor() != nullptr)
		{
			getAudioProcessor()->setLoadedFile(fileName, true, false);

			Range<int> range(v.getProperty("rangeStart"), v.getProperty("rangeEnd"));

			getAudioProcessor()->setRange(range);

			//WHYTHEFUCK
			triggerAsyncUpdate();
		}
	}
}

StringArray ScriptingApi::Content::ScriptAudioWaveform::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(processorId)) return ScriptComponent::getOptionsFor(id);

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	auto aps = ProcessorHelpers::getAllIdsForType<AudioSampleProcessor>(mp->getOwnerSynth());
	auto samplers = ProcessorHelpers::getAllIdsForType<ModulatorSampler>(mp->getOwnerSynth());
	auto cds = ProcessorHelpers::getAllIdsForType<ComplexDataHolder>(mp->getOwnerSynth());

	aps.addArray(samplers);
	aps.addArray(cds);

	return aps;
}

AudioSampleProcessor * ScriptingApi::Content::ScriptAudioWaveform::getAudioProcessor()
{
	return dynamic_cast<AudioSampleProcessor*>(getConnectedProcessor());
}

void ScriptingApi::Content::ScriptAudioWaveform::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /* = sendNotification */)
{
	if (id == getIdFor(parameterId))
	{
		return;
	}

	if (id == getIdFor(sampleIndex))
	{
		if (auto cdh = dynamic_cast<ComplexDataHolder*>(getConnectedProcessor()))
		{
			auto index = (int)newValue;

			if (index < cdh->getNumAudioFiles())
			{
				audioFile = cdh->addOrReturnAudioFile(index);
			}
		}
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

ModulatorSampler* ScriptingApi::Content::ScriptAudioWaveform::getSampler()
{
	return dynamic_cast<ModulatorSampler*>(getConnectedProcessor());
}

void ScriptingApi::Content::ScriptAudioWaveform::handleDefaultDeactivatedProperties()
{
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(text));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(min));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(max));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(defaultValue));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(macroControl));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(pluginParameterName));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(isMetaParameter));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(linkedTo));
	deactivatedProperties.addIfNotAlreadyThere(getIdFor(ScriptComponent::Properties::useUndoManager));
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
	setMethod("addViewport", Wrapper::addScriptedViewport);
	setMethod("addPanel", Wrapper::addPanel);
	setMethod("addAudioWaveform", Wrapper::addAudioWaveform);
	setMethod("addSliderPack", Wrapper::addSliderPack);
	setMethod("addFloatingTile", Wrapper::addFloatingTile);
	setMethod("setContentTooltip", Wrapper::setContentTooltip);
	setMethod("setToolbarProperties", Wrapper::setToolbarProperties);
	setMethod("setHeight", Wrapper::setHeight);
	setMethod("setWidth", Wrapper::setWidth);
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
	setMethod("createPath", Wrapper::createPath);
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
			return components[i];
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
			return components[i];
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


ScriptingApi::Content::ScriptedViewport* ScriptingApi::Content::addScriptedViewport(Identifier viewportName, int x, int y)
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


ScriptingApi::Content::ScriptFloatingTile* ScriptingApi::Content::addFloatingTile(Identifier floatingTileName, int x, int y)
{
	return addComponent<ScriptFloatingTile>(floatingTileName, x, y);
}


ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponent(int index)
{
	if (index == -1) return nullptr;

	if(index < components.size())
		return components[index];

	return nullptr;
}

var ScriptingApi::Content::getComponent(var componentName)
{
	Identifier n(componentName.toString());

	for (int i = 0; i < components.size(); i++)
	{
		if (n == components[i]->getName())
			return var(components[i]);
	}

	logErrorAndContinue("Component with name " + componentName.toString() + " wasn't found.");

	return var();
}

var ScriptingApi::Content::getAllComponents(String regex)
{
	Array<var> list;

	for (int i = 0; i < getNumComponents(); i++)
	{	    
		if (RegexFunctions::matchesWildcard(regex, components[i]->getName().toString()))
		{
			list.add(var(components[i]));
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
		ScopedPointer<XmlElement> existingData = XmlDocument::parse(f);

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

		ScopedPointer<XmlElement> xml = preset.createXml();

		f.replaceWithText(xml->createDocument(""));
	}
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
		ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

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

	StringArray macroNames;

	if (components.size() != 0)
	{
		macroNames = components[0]->getOptionsFor(components[0]->getIdFor(ScriptComponent::macroControl));
	}

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

			v = Helpers::getCleanedComponentValue(presetChild.getProperty(value_));

		}
		else
		{
			components[i]->resetValueToDefault();
			v = components[i]->getValue();
		}

		if (dynamic_cast<ScriptingApi::Content::ScriptLabel*>(components[i].get()) != nullptr)
		{
			getScriptProcessor()->controlCallback(components[i], v);
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
			getScriptProcessor()->controlCallback(components[i], v);
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

void ScriptingApi::Content::cleanJavascriptObjects()
{
	allowAsyncFunctions = false;

	for (int i = 0; i < components.size(); i++)
	{
		components[i]->cancelChangedControlCallback();
		components[i]->setControlCallback(var());
		components[i]->cleanScriptChangedPropertyIds();

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

			ScriptComponent::ScopedPropertyEnabler spe(sc);
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

			DebugableObject::Helpers::gotoLocation(p, info);

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

var ScriptingApi::Content::Helpers::getCleanedComponentValue(const var& data)
{
	if (data.isString() && data.toString().startsWith("JSON"))
	{
		String jsonData = data.toString().fromFirstOccurrenceOf("JSON", false, false);

		return JSON::fromString(jsonData);
	}
	else
	{
		float d = (float)data;
		FloatSanitizers::sanitizeFloatNumber(d);
		return var(d);
	}
}

} // namespace hise
