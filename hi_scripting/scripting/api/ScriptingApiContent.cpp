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
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionX);
    API_METHOD_WRAPPER_0(ScriptComponent, getGlobalPositionY);
	API_VOID_METHOD_WRAPPER_1(ScriptComponent, setControlCallback);
};

#define ADD_SCRIPT_PROPERTY(id, name) static const Identifier id(name); propertyIds.add(id);

ScriptingApi::Content::ScriptComponent::ScriptComponent(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int width, int height, int numConstants) :
ConstScriptingObject(base, numConstants),
name(name_),
value(0.0),
parent(parentContent),
skipRestoring(false),
componentProperties(new DynamicObject()),
changed(false),
customControlCallback(var()),
parentComponentIndex(-1)
{
	//static const Identifier textId("text"); propertyIds.add(textId);

	ADD_SCRIPT_PROPERTY(textId, "text");

	ADD_SCRIPT_PROPERTY(vId, "visible");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(eId, "enabled");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(xId, "x");					ADD_AS_SLIDER_TYPE(0, 900, 1);
	ADD_SCRIPT_PROPERTY(yId, "y");					ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	ADD_SCRIPT_PROPERTY(wId, "width");				ADD_AS_SLIDER_TYPE(0, 900, 1);
	ADD_SCRIPT_PROPERTY(hId, "height");				ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	ADD_SCRIPT_PROPERTY(mId1, "min");
	ADD_SCRIPT_PROPERTY(mId2, "max");
	ADD_SCRIPT_PROPERTY(tId, "tooltip");
	ADD_SCRIPT_PROPERTY(bId, "bgColour");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(iId1, "itemColour");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(iId2, "itemColour2");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(tId2, "textColour");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	ADD_SCRIPT_PROPERTY(mId3, "macroControl");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(sId1, "saveInPreset");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(iId4, "isPluginParameter"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId, "pluginParameterName");
	ADD_SCRIPT_PROPERTY(uId, "useUndoManager");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(pId2, "parentComponent");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId3, "processorId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(pId4, "parameterId");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	deactivatedProperties.add(getIdFor(isPluginParameter));

	setDefaultValue(Properties::text, name.toString());
	setDefaultValue(Properties::visible, true);
	setDefaultValue(Properties::enabled, true);
	setDefaultValue(Properties::x, x);
	setDefaultValue(Properties::y, y);
	setDefaultValue(Properties::width, width);
	setDefaultValue(Properties::height, height);
	setDefaultValue(Properties::min, 0.0);
	setDefaultValue(Properties::max, 1.0);
	setDefaultValue(Properties::tooltip, "");
	setDefaultValue(Properties::bgColour, (int64)0x55FFFFFF);
	setDefaultValue(Properties::itemColour, (int64)0x66333333);
	setDefaultValue(Properties::itemColour2, (int64)0xfb111111);
	setDefaultValue(Properties::textColour, (int64)0xFFFFFFFF);
	setDefaultValue(Properties::macroControl, -1);
	setDefaultValue(Properties::saveInPreset, true);
	setDefaultValue(Properties::isPluginParameter, false);
	setDefaultValue(Properties::pluginParameterName, "");
	setDefaultValue(Properties::useUndoManager, false);
	setDefaultValue(Properties::parentComponent, "");
	setDefaultValue(Properties::processorId, "");
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
    ADD_API_METHOD_0(getGlobalPositionX);
    ADD_API_METHOD_0(getGlobalPositionY);
	ADD_API_METHOD_1(setControlCallback);

	//setName(name_.toString());


    //enableAllocationFreeMessages(30);
    
	SEND_MESSAGE(this);
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
		auto c = getScriptProcessor()->getScriptingContent();

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

	if (data.isString() && data.toString().startsWith("JSON"))
	{
		String jsonData = data.toString().fromFirstOccurrenceOf("JSON", false, false);

		value = JSON::fromString(jsonData);
	}
	else
	{
		value = (double)data;
	}
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
        reportScriptError("You must specify the unique component name, not the object itself");
    }
    
	componentProperties->setProperty(id, newValue);

	if (id == getIdFor(macroControl))
	{
		StringArray sa = getOptionsFor(id);

		const int index = sa.indexOf(newValue.toString()) - 1;

		if (index >= -1) addToMacroControl(index);
	}
	else if (id == getIdFor(parentComponent))
	{
		auto c = getScriptProcessor()->getScriptingContent();

		int oldParentIndex = parentComponentIndex;

		if (newValue.toString().isEmpty())
		{
			parentComponentIndex = -1;

		}
		else
		{
			parentComponentIndex = c->getComponentIndex(Identifier(newValue.toString()));

			if (parentComponentIndex != -1)
			{
				if (!c->getComponent(parentComponentIndex)->addChildComponent(this))
				{
					// something went wrong...
					parentComponentIndex = -1;
				}
			}
			else
			{
				reportScriptError("parent component " + newValue.toString() + " not found.");
			}
		}

		if (oldParentIndex != parentComponentIndex)
		{
			parent->contentNeedsRebuilding = true;
		}
	}
	else if (id == getIdFor(x) || id == getIdFor(y) || id == getIdFor(width) ||
		id == getIdFor(height) || id == getIdFor(visible) || id == getIdFor(enabled))
	{
		notifyChildComponents();
	}
	else if (id == getIdFor(processorId))
	{
		auto pId = newValue.toString();

		if (pId.isNotEmpty())
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

	if (notifyEditor == sendNotification)
	{
		SEND_MESSAGE(this);
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
	jassert(componentProperties.get() != nullptr);

	jassert(componentProperties->hasProperty(getIdFor(p)));

	return componentProperties->getProperty(getIdFor(p));
}


var ScriptingApi::Content::ScriptComponent::getNonDefaultScriptObjectProperties() const
{
	DynamicObject::Ptr clone = componentProperties->clone();

	for (int i = 0; i < deactivatedProperties.size(); i++)
	{
		clone->removeProperty(deactivatedProperties[i]);
	}

	for (int i = 0; i < defaultValues.size(); i++)
	{
		const Identifier id = getIdFor(i);

		var a = clone->getProperty(id);
		var b = defaultValues[id];

		if (a == b)
		{
			clone->removeProperty(id);
		}
	}

	return var(clone);
}


String ScriptingApi::Content::ScriptComponent::getScriptObjectPropertiesAsJSON() const
{
	auto v = getNonDefaultScriptObjectProperties();

	return JSON::toString(v, false);
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

	if (!componentProperties->hasProperty(propertyId))
	{
		reportScriptError("the property does not exist");
		return;
	}

    
    
	setScriptObjectPropertyWithChangeMessage(propertyId, newValue, parent->allowGuiCreation ? dontSendNotification : sendNotification);
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

    SEND_MESSAGE(this);
};

void ScriptingApi::Content::ScriptComponent::setColour(int colourId, int colourAs32bitHex)
{
	switch (colourId)
	{
	case 0: setScriptObjectProperty(bgColour, (int64)colourAs32bitHex); break;
	case 1:	setScriptObjectProperty(itemColour, (int64)colourAs32bitHex); break;
	case 2:	setScriptObjectProperty(itemColour2, (int64)colourAs32bitHex); break;
	case 3:	setScriptObjectProperty(textColour, (int64)colourAs32bitHex); break;
	}

	sendAllocationFreeChangeMessage();
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

				setScriptObjectPropertyWithChangeMessage(priorityProperties[i], dataSet[priorityProperties[i]], dontSendNotification);
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
				setScriptObjectPropertyWithChangeMessage(propertyId, dataSet[propertyId], dontSendNotification);
			}
		}


#if 0
		for (int i = 0; i < dataSet.size(); i++)
		{
			Identifier propertyId = dataSet.getName(i);

			if (!propertyIds.contains(propertyId))
				continue;

			if (priorityProperties.contains(propertyId)) continue;

			handleScriptPropertyChange(propertyId);

			setScriptObjectPropertyWithChangeMessage(propertyId, dataSet.getValueAt(i), dontSendNotification);
		}
#endif
	}

	SEND_MESSAGE(this);
}

int ScriptingApi::Content::ScriptComponent::getGlobalPositionX()
{
    const int thisX = getScriptObjectProperty(ScriptComponent::Properties::x);
    
    if(parentComponentIndex == -1)
    {
        return thisX;
    }
    else
    {
        ScriptComponent* parentComponent = parent->getComponent(parentComponentIndex);
        
        return thisX + parentComponent->getGlobalPositionX();
    }
}

int ScriptingApi::Content::ScriptComponent::getGlobalPositionY()
{
    const int thisY = getScriptObjectProperty(ScriptComponent::Properties::y);
    
    if(parentComponentIndex == -1)
    {
        return thisY;
    }
    else
    {
        ScriptComponent* parentComponent = parent->getComponent(parentComponentIndex);
        
        return thisY + parentComponent->getGlobalPositionY();
    }
}

void ScriptingApi::Content::ScriptComponent::setPosition(int x, int y, int w, int h)
{
	componentProperties->setProperty("x", x);
	componentProperties->setProperty("y", y);
	componentProperties->setProperty("width", w);
	componentProperties->setProperty("height", h);

	sendAllocationFreeChangeMessage();
}


void ScriptingApi::Content::ScriptComponent::addToMacroControl(int macroIndex)
{
	if (!parent->allowGuiCreation)
	{
		// Will be taken care of at next compilation
		return;
	}

	int knobIndex = parent->components.indexOf(this);

	if (knobIndex == -1)
	{
		reportScriptError("Component not valid");
	}

	if (macroIndex == -1)
	{
		getProcessor()->getMainController()->getMacroManager().removeMacroControlsFor(getProcessor(), getName());
	}
	else
	{
		NormalisableRange<double> range(getScriptObjectProperty(Properties::min), getScriptObjectProperty(Properties::max));

		if (dynamic_cast<ScriptSlider*>(this) != nullptr)
		{
			range.interval = getScriptObjectProperty((int)ScriptSlider::Properties::stepSize);
			HiSlider::setRangeSkewFactorFromMidPoint(range, getScriptObjectProperty((int)ScriptSlider::Properties::middlePosition));
		}

		getProcessor()->getMainController()->getMacroManager().getMacroChain()->addControlledParameter(
			macroIndex, getProcessor()->getId(), knobIndex, name.toString(), range, false);
	}
};

void ScriptingApi::Content::ScriptComponent::setDefaultValue(int p, const var &defaultValue)
{
	defaultValues.set(getIdFor(p), defaultValue);
	setScriptObjectProperty(p, defaultValue);
}

void ScriptingApi::Content::ScriptComponent::showControl(bool shouldBeVisible)
{
	setScriptObjectPropertyWithChangeMessage(getIdFor(visible), shouldBeVisible);
};


void ScriptingApi::Content::ScriptComponent::setTooltip(const String &newTooltip)
{
	setScriptObjectProperty(Properties::tooltip, newTooltip);
}

File ScriptingApi::Content::ScriptComponent::getExternalFile(var newValue)
{
	if (GET_PROJECT_HANDLER(getProcessor()).isActive())
	{
		return GET_PROJECT_HANDLER(getProcessor()).getFilePath(newValue, ProjectHandler::SubDirectories::Images);
	}
	else
	{
		return dynamic_cast<ExternalFileProcessor*>(getScriptProcessor())->getFile(newValue, PresetPlayerHandler::ImageResources);
	}
}

var ScriptingApi::Content::ScriptComponent::getWidth() const
{
	return getScriptObjectProperty(Properties::width);
}

var ScriptingApi::Content::ScriptComponent::getHeight() const
{
	return getScriptObjectProperty(Properties::height);
}

int ScriptingApi::Content::ScriptComponent::getParentComponentIndex() const
{
	return parentComponentIndex;
}

bool ScriptingApi::Content::ScriptComponent::addChildComponent(ScriptComponent* childComponent)
{
	auto sc = getScriptProcessor()->getScriptingContent();

	const int thisIndex = sc->getComponentIndex(getName());
	const int childIndex = sc->getComponentIndex(childComponent->getName());

	if (childIndex < thisIndex)
	{
		reportScriptError("Child component must be declared after parent component");
		RETURN_IF_NO_THROW(false)
	}

	if (childComponent == this)
	{
		reportScriptError("Can't add itself as parent.");
		RETURN_IF_NO_THROW(false)
	}
	if (childComponent->isChildComponent(this))
	{
		reportScriptError("Can't add a parent as child component");
		RETURN_IF_NO_THROW(false)
	}

	childComponents.addIfNotAlreadyThere(childComponent);

	return true;
}

bool ScriptingApi::Content::ScriptComponent::isChildComponent(ScriptComponent* childComponent)
{
	for (int i = 0; i < childComponents.size(); i++)
	{
		if (childComponents[i]->isChildComponent(childComponent))
			return true;
	}

	return childComponents.contains(childComponent);
}

void ScriptingApi::Content::ScriptComponent::notifyChildComponents()
{
	for (int i = 0; i < childComponents.size(); i++)
	{
		ScriptComponent* c = childComponents[i].get();

		if (c != nullptr)
		{
            SEND_MESSAGE(childComponents[i].get());
			childComponents[i].get()->notifyChildComponents();
		}
		else jassertfalse;
	}
}

void ScriptingApi::Content::ScriptComponent::setValueWithUndo(var newValue)
{
	const int parameterIndex = parent->getComponentIndex(name);
	const float oldValue = (float)getValue();
	
	MacroControlledObject::UndoableControlEvent* newEvent = new MacroControlledObject::UndoableControlEvent(getProcessor(), parameterIndex, oldValue, (float)newValue);

	String undoName = getProcessor()->getId();
	undoName << " - " << getProcessor()->getIdentifierForParameterIndex(parameterIndex).toString() << ": " << String((float)newValue, 2);

	getProcessor()->getMainController()->getControlUndoManager()->beginNewTransaction(undoName);
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

bool ScriptingApi::Content::ScriptComponent::isShowing() const
{
	const bool isVisible = getScriptObjectProperty(ScriptComponent::Properties::visible);

	if (auto p = parent->getComponent(parentComponentIndex))
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
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getProcessor()))
	{
		auto componentId = getName();
		auto cTree = parent->getContentProperties();

		auto child = parent->getValueTreeForComponent(getName());

		if (child.isValid())
		{
			child.setProperty(propId, newValue, nullptr);
		}
		else
			jassertfalse;

		setScriptObjectPropertyWithChangeMessage(propId, newValue);
	}
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

ScriptingApi::Content::ScriptSlider::ScriptSlider(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int, int) :
ScriptComponent(base, parentContent, name_, x, y, 128, 48),
styleId(Slider::SliderStyle::RotaryHorizontalVerticalDrag),
m(HiSlider::Mode::Linear),
image(nullptr),
minimum(0.0f),
maximum(1.0f)
{
	CHECK_COPY_AND_RETURN_22(dynamic_cast<Processor*>(base));

	ADD_SCRIPT_PROPERTY(i01, "mode");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i02, "style");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i03, "stepSize");
	ADD_SCRIPT_PROPERTY(i04, "middlePosition");
	ADD_SCRIPT_PROPERTY(i05, "defaultValue");
	ADD_SCRIPT_PROPERTY(i06, "suffix");
	ADD_SCRIPT_PROPERTY(i07, "filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_SCRIPT_PROPERTY(i08, "numStrips");
	ADD_SCRIPT_PROPERTY(i09, "isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i10, "scaleFactor");
	ADD_SCRIPT_PROPERTY(i11, "mouseSensitivity");
	ADD_SCRIPT_PROPERTY(i12, "dragDirection");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	deactivatedProperties.removeAllInstancesOf(getIdFor(isPluginParameter));

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

	priorityProperties.add(getIdFor(Mode));

	setDefaultValue(ScriptSlider::Properties::Mode, "Linear");
	setDefaultValue(ScriptSlider::Properties::Style, "Knob");
	setDefaultValue(ScriptSlider::Properties::middlePosition, -1.0);
	setDefaultValue(ScriptSlider::Properties::defaultValue, 0.0);
	setDefaultValue(ScriptSlider::Properties::stepSize, 0.01);
	setDefaultValue(ScriptComponent::min, 0.0);
	setDefaultValue(ScriptComponent::max, 1.0);
	setDefaultValue(ScriptSlider::Properties::suffix, "");
	setDefaultValue(ScriptSlider::Properties::filmstripImage, "Use default skin");
	setDefaultValue(ScriptSlider::Properties::numStrips, 0);
	setDefaultValue(ScriptSlider::Properties::isVertical, true);
	setDefaultValue(ScriptSlider::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptSlider::Properties::mouseSensitivity, 1.0f);
	setDefaultValue(ScriptSlider::Properties::dragDirection, "Diagonal");

	setScriptObjectPropertyWithChangeMessage(getIdFor(Mode), "Linear", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(Style), "Knob", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptSlider::Properties::middlePosition), -1.0);
	setScriptObjectPropertyWithChangeMessage(getIdFor(stepSize), 0.01, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptComponent::min), 0.0, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(ScriptComponent::max), 1.0, dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(suffix), "", dontSendNotification);
	setScriptObjectPropertyWithChangeMessage(getIdFor(filmstripImage), "Use default skin", dontSendNotification);


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
	image = Image();
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
		setMode(newValue.toString());
	}
	else if (id == propertyIds[Style])
	{
		setStyle(newValue);
	}
	else if (id == getIdFor(middlePosition))
	{
		setMidPoint(newValue);
		if (notifyEditor == sendNotification) parent->sendChangeMessage(); // skip the rest
		return;
	}
	else if (id == getIdFor(filmstripImage))
	{
		ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "Use default skin");
			image = Image();
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);


#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName);

			jassert(image.isValid());

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName());

#endif
		}
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
	case filmstripImage:
		sa.add("Load new File");
		sa.add("Use default skin");
		sa.addArray(getProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());
		break;
	case dragDirection:
		sa.add("Diagonal");
		sa.add("Vertical");
		sa.add("Horizontal");
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

	const bool illegalMidPoint = !range.contains(valueForMidPoint);
	if (illegalMidPoint)
	{
		reportScriptError("setMidPoint() value must be in the knob range.");
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
}

void ScriptingApi::Content::ScriptSlider::setMinValue(double min) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		minimum = min;
		sendChangeMessage();
	}
	else
	{
		reportScriptError("setMinValue() can only be called on sliders in 'Range' mode.");
	}
}

void ScriptingApi::Content::ScriptSlider::setMaxValue(double max) noexcept
{
	if (styleId == Slider::TwoValueHorizontal)
	{
		maximum = max;
		sendChangeMessage();
	}
	else
	{
		reportScriptError("setMaxValue() can only be called on sliders in 'Range' mode.");
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
		reportScriptError("getMinValue() can only be called on sliders in 'Range' mode.");
		RETURN_IF_NO_THROW(0.0)
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
		reportScriptError("getMaxValue() can only be called on sliders in 'Range' mode.");
		RETURN_IF_NO_THROW(1.0)
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
		reportScriptError("contains() can only be called on sliders in 'Range' mode.");
		RETURN_IF_NO_THROW(false)
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

		reportScriptError(errorMessage);
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

		reportScriptError(errorMessage);
#endif

		return 0.0;
	}
}

void ScriptingApi::Content::ScriptSlider::setRange(double min_, double max_, double stepSize_)
{
	setScriptObjectProperty(ScriptComponent::Properties::min, var(min_));
	setScriptObjectProperty(ScriptComponent::Properties::max, var(max_));
	setScriptObjectProperty(Properties::stepSize, stepSize_);

	sendChangeMessage();
}

void ScriptingApi::Content::ScriptSlider::setMode(String mode)
{
	StringArray sa = getOptionsFor(getIdFor(Mode));

	const int index = sa.indexOf(mode);

	if (index == -1)
	{
		reportScriptError("invalid slider mode: " + mode);
		return;
	}

	m = (HiSlider::Mode) index;

	NormalisableRange<double> nr = HiSlider::getRangeForMode(m);

	setScriptObjectProperty(Mode, mode);

	if ((nr.end - nr.start) != 0)
	{
		setScriptObjectProperty(ScriptComponent::Properties::min, nr.start);
		setScriptObjectProperty(ScriptComponent::Properties::max, nr.end);
		setScriptObjectProperty(stepSize, nr.interval);
		setScriptObjectProperty(ScriptSlider::Properties::suffix, HiSlider::getSuffixForMode(m, getValue()));

		setMidPoint(HiSlider::getMidPointFromRangeSkewFactor(nr));


		//setMidPoint(getScriptObjectProperty(ScriptSlider::Properties::middlePosition));
	}
}


struct ScriptingApi::Content::ScriptButton::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptButton, setPopupData);
};

ScriptingApi::Content::ScriptButton::ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int, int) :
ScriptComponent(base, parentContent, name, x, y, 128, 32),
image(nullptr)
{
	ADD_SCRIPT_PROPERTY(i00, "filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_SCRIPT_PROPERTY(i01, "numStrips");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i02, "isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "scaleFactor");
	ADD_SCRIPT_PROPERTY(i05, "radioGroup");
	ADD_SCRIPT_PROPERTY(i04, "isMomentary");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));

	setDefaultValue(ScriptButton::Properties::filmstripImage, "");
	setDefaultValue(ScriptButton::Properties::numStrips, "2");
	setDefaultValue(ScriptButton::Properties::isVertical, true);
	setDefaultValue(ScriptButton::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptButton::Properties::radioGroup, 0);
	setDefaultValue(ScriptButton::Properties::isMomentary, 0);

	ADD_API_METHOD_2(setPopupData);
}

ScriptingApi::Content::ScriptButton::~ScriptButton()
{
	
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptButton::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ButtonWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptButton::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(filmstripImage))
	{
		ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "");
			image = Image();
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);

#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName);

			jassert(image.isValid());

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName());

#endif
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
		sa.addArray(getProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());

		return sa;
	}
	else if (id == getIdFor(numStrips))
	{
		StringArray sa;

		sa.add("2");
		sa.add("6");

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



struct ScriptingApi::Content::ScriptLabel::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptLabel, setEditable);
};

ScriptingApi::Content::ScriptLabel::ScriptLabel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int) :
ScriptComponent(base, parentContent, name, x, y, width, 16)
{
	ADD_SCRIPT_PROPERTY(i01, "fontName");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i02, "fontSize");	ADD_AS_SLIDER_TYPE(1, 200, 1);
	ADD_SCRIPT_PROPERTY(i03, "fontStyle");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i04, "alignment");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "editable");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i06, "multiline");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	componentProperties->setProperty(getIdFor(FontName), 0);
	componentProperties->setProperty(getIdFor(FontSize), 0);
	componentProperties->setProperty(getIdFor(FontStyle), 0);
	componentProperties->setProperty(getIdFor(Alignment), 0);
	componentProperties->setProperty(getIdFor(Editable), 0);
	componentProperties->setProperty(getIdFor(Multiline), 0);

	setDefaultValue(text, name.toString());
	setDefaultValue(bgColour, (int64)0x00000000);
	setDefaultValue(itemColour, (int64)0x00000000);
	setDefaultValue(textColour, (int64)0xffffffff);
	setDefaultValue(FontStyle, "plain");
	setDefaultValue(FontSize, 13.0f);
	setDefaultValue(FontName, "Arial");
	setDefaultValue(Editable, true);
	setDefaultValue(Multiline, false);

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));

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

Justification ScriptingApi::Content::ScriptLabel::getJustification()
{
	auto justAsString = getScriptObjectProperty(Alignment);

	return ApiHelpers::getJustification(justAsString);
}

struct ScriptingApi::Content::ScriptComboBox::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptComboBox, addItem);
	API_METHOD_WRAPPER_0(ScriptComboBox, getItemText);
};

ScriptingApi::Content::ScriptComboBox::ScriptComboBox(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int) :
ScriptComponent(base, parentContent, name, x, y, width, 32)
{
	propertyIds.add(Identifier("items"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	propertyIds.add(Identifier("isPluginParameter")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::height));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));

	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));

	priorityProperties.add(getIdFor(Items));

	componentProperties->setProperty(getIdFor(Items), 0);

	setDefaultValue(Items, "");
	setDefaultValue(ScriptComponent::min, 1.0f);
	setDefaultValue(isPluginParameter, false);

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

	setScriptObjectProperty(Items, newItemList);

	int size = getScriptObjectProperty(max);

	setScriptObjectProperty(ScriptComponent::Properties::min, 1);
	setScriptObjectProperty(ScriptComponent::Properties::max, size + 1);

	sendChangeMessage();
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

struct ScriptingApi::Content::ScriptTable::Wrapper
{
	API_METHOD_WRAPPER_1(ScriptTable, getTableValue);
	API_VOID_METHOD_WRAPPER_2(ScriptTable, connectToOtherTable);
};

ScriptingApi::Content::ScriptTable::ScriptTable(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, name, x, y, width, height),
ownedTable(new MidiTable()),
useOtherTable(false),
connectedProcessor(nullptr),
lookupTableIndex(-1)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour2));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("tableIndex");
	propertyIds.add("processorId"); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	componentProperties->setProperty(getIdFor(ProcessorId), 0);
	componentProperties->setProperty(getIdFor(TableIndex), 0);

	setDefaultValue(ScriptTable::Properties::ProcessorId, "");
	setDefaultValue(ScriptTable::Properties::TableIndex, 0);

	ADD_API_METHOD_1(getTableValue);
	ADD_API_METHOD_2(connectToOtherTable);
}

ScriptingApi::Content::ScriptTable::~ScriptTable()
{
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

	parent->sendChangeMessage();

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
			reportScriptError("Connected Table was not found!");
			RETURN_IF_NO_THROW(-1.0f)
		}
	}
	else
	{
		return ownedTable->get(inputValue);
	}
}

StringArray ScriptingApi::Content::ScriptTable::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ProcessorId)) return ScriptComponent::getOptionsFor(id);

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	return ProcessorHelpers::getAllIdsForType<LookupTableProcessor>(mp->getOwnerSynth());
};

void ScriptingApi::Content::ScriptTable::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	if (getIdFor(ProcessorId) == id)
	{
		const int tableIndex = getScriptObjectProperty(TableIndex);

		connectToOtherTable(newValue, tableIndex);
	}
	else if (getIdFor(TableIndex) == id)
	{
		connectToOtherTable(getScriptObjectProperty(ProcessorId), newValue);
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

void ScriptingApi::Content::ScriptTable::connectToOtherTable(const String &otherTableId, int index)
{
	if (otherTableId.isEmpty()) return;

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

    reportScriptError(otherTableId + " was not found.");
}

LookupTableProcessor * ScriptingApi::Content::ScriptTable::getTableProcessor() const
{
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

struct ScriptingApi::Content::ScriptSliderPack::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPack, setSliderAtIndex);
	API_METHOD_WRAPPER_1(ScriptSliderPack, getSliderValueAt);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, setAllValues);
	API_METHOD_WRAPPER_0(ScriptSliderPack, getNumSliders);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPack, referToData);
};

ScriptingApi::Content::ScriptSliderPack::ScriptSliderPack(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, imageName, x, y, width, height),
packData(new SliderPackData()),
existingData(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	ADD_SCRIPT_PROPERTY(i00, "sliderAmount");		ADD_AS_SLIDER_TYPE(0, 128, 1);
	ADD_SCRIPT_PROPERTY(i01, "stepSize");
	ADD_SCRIPT_PROPERTY(i02, "flashActive");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "showValueOverlay");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i04, "ProcessorId");         ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "SliderPackIndex");     

	packData->setNumSliders(16);

	setDefaultValue(bgColour, 0x00000000);
	setDefaultValue(itemColour, 0x77FFFFFF);
	setDefaultValue(itemColour2, 0x77FFFFFF);
	setDefaultValue(textColour, 0x33FFFFFF);

	setDefaultValue(SliderAmount, 0);
	setDefaultValue(StepSize, 0);
	setDefaultValue(FlashActive, true);
	setDefaultValue(ShowValueOverlay, true);
	setDefaultValue(ProcessorId, "");
	setDefaultValue(SliderPackIndex, 0);

	setDefaultValue(SliderAmount, 16);
	setDefaultValue(StepSize, 0.01);

	ADD_API_METHOD_2(setSliderAtIndex);
	ADD_API_METHOD_1(getSliderValueAt);
	ADD_API_METHOD_1(setAllValues);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_1(referToData);
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
	if (newPackId.isEmpty()) return;

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

			

			return;
		}
	}

	existingData = nullptr;

    reportScriptError(newPackId + " was not found.");
}

StringArray ScriptingApi::Content::ScriptSliderPack::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(ProcessorId))
	{
		MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
		if (mp == nullptr) return StringArray();

		return ProcessorHelpers::getAllIdsForType<SliderPackProcessor>(mp->getOwnerSynth());
	}
	
	return ScriptComponent::getOptionsFor(id);
};

void ScriptingApi::Content::ScriptSliderPack::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(SliderAmount))
	{
		if (existingData.get() != nullptr) return;
		packData->setNumSliders(newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::min))
	{
		if (existingData.get() != nullptr) return;
		packData->setRange(newValue, packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(ScriptComponent::Properties::max))
	{
		if (existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), newValue, newValue);
	}
	else if (id == getIdFor(StepSize))
	{
		if (existingData.get() != nullptr) return;
		packData->setRange(packData->getRange().getStart(), packData->getRange().getEnd(), newValue);
	}
	else if (id == getIdFor(FlashActive))
	{
		if (existingData.get() != nullptr) return;
		packData->setFlashActive((bool)newValue);
	}
	else if (id == getIdFor(ShowValueOverlay))
	{
		if (existingData.get() != nullptr) return;
		packData->setShowValueOverlay((bool)newValue);
	}
	else if (id == getIdFor(ProcessorId))
	{
		connectToOtherSliderPack(newValue.toString());
	}
	else if (id == getIdFor(SliderPackIndex))
	{
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
		
		SEND_MESSAGE(this);
	}
	else
	{
		reportScriptError("You must call setValue() with an array for Slider Packs");
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
		reportScriptError("not a valid SliderPackData object");
}

struct ScriptingApi::Content::ScriptImage::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptImage, setImageFile);
	API_VOID_METHOD_WRAPPER_1(ScriptImage, setAlpha);
};

ScriptingApi::Content::ScriptImage::ScriptImage(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, imageName, x, y, width, height),
image(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::itemColour2));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	ADD_SCRIPT_PROPERTY(i00, "alpha");				ADD_AS_SLIDER_TYPE(0.0, 1.0, 0.01);
	ADD_SCRIPT_PROPERTY(i01, "fileName");			ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	ADD_SCRIPT_PROPERTY(i02, "offset");
	ADD_SCRIPT_PROPERTY(i03, "scale");
	ADD_SCRIPT_PROPERTY(i04, "allowCallbacks");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "popupMenuItems");		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i06, "popupOnRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	priorityProperties.add(getIdFor(FileName));

	componentProperties->setProperty(getIdFor(Alpha), 0);
	componentProperties->setProperty(getIdFor(FileName), 0);
	componentProperties->setProperty(getIdFor(Offset), 0);
	componentProperties->setProperty(getIdFor(Scale), 1.0);
	componentProperties->setProperty(getIdFor(AllowCallbacks), 0);
	componentProperties->setProperty(getIdFor(PopupMenuItems), "");
	componentProperties->setProperty(getIdFor(PopupOnRightClick), true);

	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(Alpha, 1.0f);
	setDefaultValue(FileName, String());
	setDefaultValue(Offset, 0);
	setDefaultValue(Scale, 1.0);
	setDefaultValue(AllowCallbacks, false);
	setDefaultValue(PopupMenuItems, "");
	setDefaultValue(PopupOnRightClick, true);

	ADD_API_METHOD_2(setImageFile);
	ADD_API_METHOD_1(setAlpha);
}

ScriptingApi::Content::ScriptImage::~ScriptImage()
{
	image = Image();
};


StringArray ScriptingApi::Content::ScriptImage::getOptionsFor(const Identifier &id)
{
	if (id == getIdFor(FileName))
	{
		StringArray sa;

		sa.add("Load new File");

		sa.addArray(getProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());

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
		setScriptObjectProperty(FileName, absoluteFileName);
		return;
	}

	ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

	setScriptObjectProperty(FileName, absoluteFileName);

#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(absoluteFileName);

	image = pool->loadFileIntoPool(poolName);

	jassert(image.isValid());

#else

	File actualFile = getExternalFile(absoluteFileName);

	image = pool->loadFileIntoPool(actualFile.getFullPathName());

#endif

	if (image.isValid())
	{
		setScriptObjectProperty(ScriptComponent::width, image.getWidth());
		setScriptObjectProperty(ScriptComponent::height, image.getHeight());
	}

	parent->sendChangeMessage();
};



ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptImage::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ImageWrapper(content, this, index);
}

const Image ScriptingApi::Content::ScriptImage::getImage() const
{
	

	return image.isNull() ? ImagePool::getEmptyImage(getScriptObjectProperty(ScriptComponent::Properties::width),
													 getScriptObjectProperty(ScriptComponent::Properties::height)) : 
							image;
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
};

ScriptingApi::Content::ScriptPanel::ScriptPanel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, panelName, x, y, width, height, 1),
graphics(new ScriptingObjects::GraphicsObject(base, this)),
repainter(this),
repaintNotifier(this),
controlSender(this, base),
loadRoutine(var()),
paintRoutine(var()),
mouseRoutine(var()),
timerRoutine(var())
{
	ADD_SCRIPT_PROPERTY(i00, "borderSize");					ADD_AS_SLIDER_TYPE(0, 20, 1);
	ADD_SCRIPT_PROPERTY(i01, "borderRadius");				ADD_AS_SLIDER_TYPE(0, 20, 1);
    ADD_SCRIPT_PROPERTY(i02, "opaque");						ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i03, "allowDragging");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i04, "allowCallbacks");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	ADD_SCRIPT_PROPERTY(i05, "popupMenuItems");				ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	ADD_SCRIPT_PROPERTY(i06, "popupOnRightClick");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i07, "popupMenuAlign");  ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i08, "selectedPopupIndex");
	ADD_SCRIPT_PROPERTY(i09, "stepSize");
	ADD_SCRIPT_PROPERTY(i10, "enableMidiLearn");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	ADD_SCRIPT_PROPERTY(i11, "holdIsRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	
	componentProperties->setProperty(getIdFor(borderSize), 0);
	componentProperties->setProperty(getIdFor(borderRadius), 0);
	componentProperties->setProperty(getIdFor(allowCallbacks), 0);

	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(textColour, 0x23FFFFFF);
	setDefaultValue(itemColour, 0x30000000);
	setDefaultValue(itemColour2, 0x30000000);
	setDefaultValue(borderSize, 2.0f);
	setDefaultValue(borderRadius, 6.0f);
    setDefaultValue(opaque, false);
	setDefaultValue(allowDragging, 0);
	setDefaultValue(allowCallbacks, 0);
	setDefaultValue(PopupMenuItems, "");
	setDefaultValue(PopupOnRightClick, true);
	setDefaultValue(popupMenuAlign, false);
	setDefaultValue(selectedPopupIndex, -1);
	setDefaultValue(stepSize, 0.0);
	setDefaultValue(enableMidiLearn, false);
	setDefaultValue(holdIsRightClick, true);
	
	addConstant("data", new DynamicObject());

	ADD_API_METHOD_0(repaint);
	ADD_API_METHOD_0(repaintImmediately);
	ADD_API_METHOD_1(setPaintRoutine);
	ADD_API_METHOD_3(setImage);
	ADD_API_METHOD_1(setMouseCallback);
	ADD_API_METHOD_1(setLoadingCallback);
	ADD_API_METHOD_1(setTimerCallback);
	ADD_API_METHOD_0(changed);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_2(loadImage);
	ADD_API_METHOD_1(setDraggingBounds);
	ADD_API_METHOD_2(setPopupData);
    ADD_API_METHOD_3(setValueWithUndo);
	

}

ScriptingApi::Content::ScriptPanel::~ScriptPanel()
{
    stopTimer();
    
	if (HiseJavascriptEngine::isJavascriptFunction(loadRoutine))
	{
		 getScriptProcessor()->getMainController_()->getSampleManager().removePreloadListener(this);
	}

    loadedImages.clear();
    
    masterReference.clear();
    
    graphics = nullptr;
    
    timerRoutine = var();
    mouseRoutine = var();
    paintRoutine = var();
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
	repainter.triggerAsyncUpdate();
}


void ScriptingApi::Content::ScriptPanel::repaintImmediately()
{
	if (MessageManager::getInstance()->isThisTheMessageThread())
		internalRepaint();
	else
		repaint();
}


void ScriptingApi::Content::ScriptPanel::setPaintRoutine(var paintFunction)
{
	paintRoutine = paintFunction;
	repaint();
}

void ScriptingApi::Content::ScriptPanel::internalRepaint()
{
	if (!usesClippedFixedImage)
	{
		auto imageBounds = getBoundsForImage();

		int canvasWidth = imageBounds.getWidth();
		int canvasHeight = imageBounds.getHeight();

		if (!isShowing())
		{
			paintCanvas = Image();

			return;
		}

		if (paintCanvas.getWidth() != canvasWidth ||
			paintCanvas.getHeight() != canvasHeight)
		{
			paintCanvas = Image(Image::PixelFormat::ARGB, canvasWidth, canvasHeight, !getScriptObjectProperty(Properties::opaque));
		}
		else if (!getScriptObjectProperty(Properties::opaque))
		{
			paintCanvas.clear(Rectangle<int>(0, 0, canvasWidth, canvasHeight));
		}

		Graphics g(paintCanvas);

		g.addTransform(AffineTransform::scale((float)getScaleFactorForCanvas()));

		var thisObject(this);
		var arguments = var(graphics);
		var::NativeFunctionArgs args(thisObject, &arguments, 1);

		graphics->setGraphics(&g, &paintCanvas);

		Result r = Result::ok();

		HiseJavascriptEngine* engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

        if(!engine->isInitialising())
        {
            engine->maximumExecutionTime = RelativeTime(0.2);
        }
        
		engine->callExternalFunction(paintRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}

		graphics->setGraphics(nullptr, nullptr);

		repaintNotifier.sendSynchronousChangeMessage();
	}

	//SEND_MESSAGE(this);
}

void ScriptingApi::Content::ScriptPanel::setLoadingCallback(var loadingCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadingCallback))
	{
		getScriptProcessor()->getMainController_()->getSampleManager().addPreloadListener(this);
		loadRoutine = loadingCallback;
	}
}


void ScriptingApi::Content::ScriptPanel::preloadStateChanged(bool isPreloading)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadRoutine))
	{
		var thisObject(this);
		var b(isPreloading);
		var::NativeFunctionArgs args(thisObject, &b, 1);

		Result r = Result::ok();

		auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

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
}


void ScriptingApi::Content::ScriptPanel::setMouseCallback(var mouseCallbackFunction)
{
	mouseRoutine = mouseCallbackFunction;
}

void ScriptingApi::Content::ScriptPanel::mouseCallback(var mouseInformation)
{
	if (HiseJavascriptEngine::isJavascriptFunction(mouseRoutine))
	{
		var thisObject(this);

		var::NativeFunctionArgs args(thisObject, &mouseInformation, 1);

		Result r = Result::ok();

        auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();
        
        engine->maximumExecutionTime = RelativeTime(0.5);
        
		engine->callExternalFunction(mouseRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}
	}
}

void ScriptingApi::Content::ScriptPanel::setTimerCallback(var timerCallback_)
{
	timerRoutine = timerCallback_;
}

void ScriptingApi::Content::ScriptPanel::timerCallback()
{
	if (HiseJavascriptEngine::isJavascriptFunction(timerRoutine))
	{
		var thisObject(this);
		var::NativeFunctionArgs args(thisObject, nullptr, 0);

		Result r = Result::ok();

        auto engine = dynamic_cast<JavascriptMidiProcessor*>(getScriptProcessor())->getScriptEngine();
        
        engine->maximumExecutionTime = RelativeTime(0.5);
		engine->callExternalFunction(timerRoutine, args, &r);

		if (r.failed())
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
		}
	}
}

void ScriptingApi::Content::ScriptPanel::changed()
{
	controlSender.triggerAsyncUpdate();
}

void ScriptingApi::Content::ScriptPanel::loadImage(String imageName, String prettyName)
{
	for (size_t i = 0; i < loadedImages.size(); i++)
	{
		if (std::get<(int)NamedImageEntries::FileName>(loadedImages[i]) == imageName) return;
	}

	ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(imageName);

	const Image newImage = pool->loadFileIntoPool(poolName);

	jassert(newImage.isValid());

#else

	File actualFile = getExternalFile(imageName);

	const Image newImage = pool->loadFileIntoPool(actualFile.getFullPathName());

#endif

	if (newImage.isValid())
	{
		loadedImages.push_back(NamedImage(newImage, prettyName, imageName));
	}
	else
	{
		BACKEND_ONLY(reportScriptError("Image " + actualFile.getFullPathName() + " not found. "));
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
	return ApiHelpers::getIntRectangleFromVar(dragBounds);
}

void ScriptingApi::Content::ScriptPanel::setDraggingBounds(var area)
{
	dragBounds = area;
}

void ScriptingApi::Content::ScriptPanel::setPopupData(var jsonData, var position)
{
	jsonPopupData = jsonData;
	
	if (position.isArray())
	{
		popupBounds.setX(position[0]);
		popupBounds.setY(position[1]);
		popupBounds.setWidth(position[2]);
		popupBounds.setHeight(position[3]);
	}
}

void ScriptingApi::Content::ScriptPanel::setValueWithUndo(var oldValue, var newValue, var actionName)
{
    auto p = dynamic_cast<Processor*>(getScriptProcessor());
    
    auto sc = getScriptProcessor()->getScriptingContent();
    
    const int index = sc->getComponentIndex(getName());
    
    auto newEvent = new BorderPanel::UndoableControlEvent(p, index, (float)oldValue, (float)newValue);
    
    getProcessor()->getMainController()->getControlUndoManager()->beginNewTransaction(actionName);
    getProcessor()->getMainController()->getControlUndoManager()->perform(newEvent);
    
}

void ScriptingApi::Content::ScriptPanel::setImage(String imageName, int xOffset, int yOffset)
{
	paintRoutine = var();
	usesClippedFixedImage = true;

	Image toUse = getLoadedImage(imageName);

	auto b = getBoundsForImage();

	if (xOffset == 0)
	{
		double ratio = (double)b.getHeight() / (double)b.getWidth();
		int w = toUse.getWidth();
		int h = (int)((double)w * ratio);

		yOffset = jmin<int>(yOffset, toUse.getHeight() - h);

		paintCanvas = toUse.getClippedImage(Rectangle<int>(0, yOffset, w, h));

	}
	else if (yOffset == 0)
	{
		double ratio = (double)b.getHeight() / (double)b.getWidth();
		int h = toUse.getHeight();
		int w = (int)((double)h * ratio);

		xOffset = jmin<int>(xOffset, toUse.getWidth() - xOffset);

		paintCanvas = toUse.getClippedImage(Rectangle<int>(xOffset, 0, w, h));

	}
	else
	{
		reportScriptError("Can't offset both dimensions. Either x or y must be 0");
	}

	repaintNotifier.sendSynchronousChangeMessage();

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

void ScriptingApi::Content::ScriptPanel::AsyncControlCallbackSender::handleAsyncUpdate()
{
	if (parent.get() != nullptr)
	{
		p->controlCallback(parent, parent->getValue());
	}
}


ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptedViewport::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ViewportWrapper(content, this, index);
}

ScriptingApi::Content::ScriptedViewport::ScriptedViewport(ProcessorWithScriptingContent* base, Content* parentContent, Identifier viewportName, int x, int y, int width, int height):
	ScriptComponent(base, parentContent, viewportName, x, y, width, height)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("scrollBarThickness");		ADD_AS_SLIDER_TYPE(0, 40, 1);
	propertyIds.add("autoHide");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);


	setDefaultValue(scrollbarThickness, 16.0);
	setDefaultValue(autoHide, true);
}


struct ScriptingApi::Content::ScriptedPlotter::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptedPlotter, addModulatorToPlotter);
	API_VOID_METHOD_WRAPPER_0(ScriptedPlotter, clearModulatorPlotter);
};

ScriptingApi::Content::ScriptedPlotter::ScriptedPlotter(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, plotterName, x, y, width, height)
{
	ADD_API_METHOD_2(addModulatorToPlotter);
	ADD_API_METHOD_0(clearModulatorPlotter);
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptedPlotter::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::PlotterWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptedPlotter::addModulatorToPlotter(String processorName, String modulatorName)
{
	Processor::Iterator<ModulatorSynth> synthIter(getProcessor()->getMainController()->getMainSynthChain());

	ModulatorSynth *synth;

	while ((synth = synthIter.getNextProcessor()) != nullptr)
	{
		if (synth->getId() == processorName)
		{
			Processor::Iterator<Modulator> modIter(synth);

			Modulator *m;

			while ((m = modIter.getNextProcessor()) != nullptr)
			{
				if (m->getId() == modulatorName)
				{
					addModulator(m);
					return;
				}
			}
		}
	}

	reportScriptError(String(modulatorName) + " was not found");

}

void ScriptingApi::Content::ScriptedPlotter::clearModulatorPlotter()
{
	clearModulators();
}


ScriptingApi::Content::ModulatorMeter::ModulatorMeter(ProcessorWithScriptingContent *base, Content *parentContent, Identifier modulatorName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, modulatorName, x, y, width, height),
targetMod(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));

	propertyIds.add("modulatorId");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	componentProperties->setProperty(getIdFor(ModulatorId), 0);

	setScriptObjectProperty(ModulatorId, "");
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ModulatorMeter::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ModulatorMeterWrapper(content, this, index);
}

void ScriptingApi::Content::ModulatorMeter::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor)
{
	if (getIdFor(ModulatorId) == id)
	{
		setScriptProcessor(getScriptProcessor());
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}



void ScriptingApi::Content::ModulatorMeter::setScriptProcessor(ProcessorWithScriptingContent *sp)
{
	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(sp);

	if (mp == nullptr) reportScriptError("Can only be called from MidiProcessors");

	Processor::Iterator<Modulator> it(mp->getOwnerSynth(), true);

	String modulatorName = getScriptObjectProperty(ModulatorId);

	if (modulatorName.isEmpty()) return;

	Identifier n(modulatorName);

	while (Modulator* m = it.getNextProcessor())
	{
		if (Identifier(m->getId()) == n)
		{
			targetMod = m;
			break;
		}
	}

	if (targetMod == nullptr) debugError(mp, "Modulator " + modulatorName + " not found!");
};


StringArray ScriptingApi::Content::ModulatorMeter::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ModulatorId)) return ScriptComponent::getOptionsFor(id);

	StringArray sa;

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	Processor::Iterator<Modulator> iter(mp->getOwnerSynth());

	Modulator *m;

	while ((m = iter.getNextProcessor()) != nullptr)
	{
		sa.add(m->getId());
	}

	return sa;
};


ScriptingApi::Content::ScriptAudioWaveform::ScriptAudioWaveform(ProcessorWithScriptingContent *base, Content *parentContent, Identifier plotterName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, plotterName, x, y, width, height),
connectedProcessor(nullptr)
{
	propertyIds.add("processorId");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

	deactivatedProperties.add(getIdFor(text));
	deactivatedProperties.add(getIdFor(min));
	deactivatedProperties.add(getIdFor(max));
	deactivatedProperties.add(getIdFor(bgColour));
	deactivatedProperties.add(getIdFor(itemColour));
	deactivatedProperties.add(getIdFor(itemColour2));
	deactivatedProperties.add(getIdFor(textColour));
	deactivatedProperties.add(getIdFor(macroControl));

	componentProperties->setProperty(getIdFor(processorId), "");

#if 0
	setMethod("connectToAudioSampleProcessor", Wrapper::connectToAudioSampleProcessor);
#endif
}

ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptAudioWaveform::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::AudioWaveformWrapper(content, this, index);
}

void ScriptingApi::Content::ScriptAudioWaveform::connectToAudioSampleProcessor(String processorId)
{
	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());

	if (mp == nullptr) return;

	Processor::Iterator<Processor> it(mp->getOwnerSynth(), false);

	Processor *p;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		if (dynamic_cast<AudioSampleProcessor*>(p) != nullptr && p->getId() == processorId)
		{
			connectedProcessor = p;

			return;
		}

	}

	connectedProcessor = nullptr;

    reportScriptError(processorId + " was not found.");
}

void ScriptingApi::Content::ScriptAudioWaveform::setScriptObjectPropertyWithChangeMessage(const Identifier &id, var newValue, NotificationType notifyEditor /*= sendNotification*/)
{
	if (id == getIdFor(processorId))
	{
		connectToAudioSampleProcessor(newValue.toString());
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}


ValueTree ScriptingApi::Content::ScriptAudioWaveform::exportAsValueTree() const
{
	ValueTree v = ScriptComponent::exportAsValueTree();

	const AudioSampleProcessor *asp = dynamic_cast<const AudioSampleProcessor*>(connectedProcessor.get());

	if (asp != nullptr)
	{
		v.setProperty("Processor", connectedProcessor->getId(), nullptr);

		v.setProperty("rangeStart", asp->getRange().getStart(), nullptr);
		v.setProperty("rangeEnd", asp->getRange().getEnd(), nullptr);
		v.setProperty("fileName", asp->getFileName(), nullptr);
	}

	return v;
}

void ScriptingApi::Content::ScriptAudioWaveform::restoreFromValueTree(const ValueTree &v)
{
	const String id = v.getProperty("Processor", "");

	if (id.isNotEmpty())
	{
		if (connectedProcessor.get() == nullptr || connectedProcessor.get()->getId() != id)
		{
			connectToAudioSampleProcessor(id);
		}

		const String fileName = v.getProperty("fileName", "");

		if (getAudioProcessor() != nullptr && fileName.isNotEmpty())
		{
			getAudioProcessor()->setLoadedFile(fileName, true, false);

			Range<int> range(v.getProperty("rangeStart"), v.getProperty("rangeEnd"));

			getAudioProcessor()->setRange(range);

			sendChangeMessage();
		}
	}
}

StringArray ScriptingApi::Content::ScriptAudioWaveform::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(processorId)) return ScriptComponent::getOptionsFor(id);

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	return ProcessorHelpers::getAllIdsForType<AudioSampleProcessor>(mp->getOwnerSynth());
}

AudioSampleProcessor * ScriptingApi::Content::ScriptAudioWaveform::getAudioProcessor()
{
	return dynamic_cast<AudioSampleProcessor*>(connectedProcessor.get());
}

// ====================================================================================================== ScriptFloatingTile functions

struct ScriptingApi::Content::ScriptFloatingTile::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptFloatingTile, setContentData);
};

ScriptingApi::Content::ScriptFloatingTile::ScriptFloatingTile(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height) :
	ScriptComponent(base, parentContent, panelName, x, y, width, height)
{
	propertyIds.add("updateAfterInit");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::saveInPreset));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::isPluginParameter));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::pluginParameterName));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::text));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::tooltip));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::useUndoManager));

	setDefaultValue(ScriptComponent::Properties::saveInPreset, false);
	setDefaultValue(Properties::updateAfterInit, true);

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

// ====================================================================================================== Content functions


ScriptingApi::Content::Content(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
height(50),
width(-1),
name(String()),
allowGuiCreation(true),
colour(Colour(0xff777777))
{
	DynamicObject::Ptr c = new DynamicObject();
	contentPropertyData = ValueTree("ContentProperties");

	setMethod("addButton", Wrapper::addButton);
	setMethod("addKnob", Wrapper::addKnob);
	setMethod("addLabel", Wrapper::addLabel);
	setMethod("addComboBox", Wrapper::addComboBox);
	setMethod("addTable", Wrapper::addTable);
	setMethod("addImage", Wrapper::addImage);
	setMethod("addModulatorMeter", Wrapper::addModulatorMeter);
	setMethod("addPlotter", Wrapper::addPlotter);
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
	setMethod("setPropertiesFromJSON", Wrapper::setPropertiesFromJSON);
	setMethod("storeAllControlsAsPreset", Wrapper::storeAllControlsAsPreset);
	setMethod("restoreAllControlsFromPreset", Wrapper::restoreAllControlsFromPreset);
	setMethod("setUseHighResolutionForPanels", Wrapper::setUseHighResolutionForPanels);
	setMethod("setColour", Wrapper::setColour);
	setMethod("clear", Wrapper::clear);
	setMethod("createPath", Wrapper::createPath);
}

ScriptingApi::Content::~Content()
{
	contentPropertyData = ValueTree();

	masterReference.clear();
	removeAllChangeListeners();
	components.clear();
}





template <class Subtype> Subtype* ScriptingApi::Content::addComponent(Identifier name, int x, int y, int width, int height)
{
	if (!allowGuiCreation)
	{
		reportScriptError("Tried to add a component after onInit()");
		return nullptr;
	}

	for (int i = 0; i < components.size(); i++)
	{
		if (components[i]->name == name) return dynamic_cast<Subtype*>(components[i].get());
	}

	Subtype *t = new Subtype(getScriptProcessor(), this, name, x, y, width, height);

	components.add(t);

	var savedValue = getScriptProcessor()->getSavedValue(name);

	auto child = getValueTreeForComponent(name);

	if (!child.isValid())
	{
		// Make a copy of the properties so that they won't get changed when
		// you call set(id, value);

		//contentDataSet.set(name, var(t->getScriptObjectProperties()->clone()));

		ValueTree newChild("Component");
		newChild.setProperty("type", t->getObjectName().toString(), nullptr);
		newChild.setProperty("id", name.toString(), nullptr);
		newChild.setProperty("x", x, nullptr);
		newChild.setProperty("y", y, nullptr);
		newChild.setProperty("width", width, nullptr);
		newChild.setProperty("height", height, nullptr);
		contentPropertyData.addChild(newChild, -1, nullptr);
	}
	else
	{
		auto d = child.getProperty("type").toString();

		if (d == t->getObjectName().toString())
		{
			auto jsonData = ValueTreeConverters::convertValueTreeToDynamicObject(child);

			ScriptComponent::ScopedPropertyEnabler spe(t);
			t->setPropertiesFromJSON(jsonData);
		}
		else
		{
			jassertfalse;
		}


	}


	if (!savedValue.isUndefined())
	{
		components.getLast()->value = savedValue;
	}

	return t;

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
	return addComponent<ScriptComboBox>(boxName, x, y, 128, 32);
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
	return addComponent<ScriptImage>(knobName, x, y, 50, 50);
};

ScriptingApi::Content::ScriptLabel * ScriptingApi::Content::addLabel(Identifier labelName, int x, int y)
{
	return addComponent<ScriptLabel>(labelName, x, y, 100, 50);
};


ScriptingApi::Content::ScriptedViewport* ScriptingApi::Content::addScriptedViewport(Identifier viewportName, int x, int y)
{
	return addComponent<ScriptedViewport>(viewportName, x, y, 200, 100);
}


ScriptingApi::Content::ScriptTable * ScriptingApi::Content::addTable(Identifier labelName, int x, int y)
{
	return addComponent<ScriptTable>(labelName, x, y, 100, 50);
};

ScriptingApi::Content::ModulatorMeter * ScriptingApi::Content::addModulatorMeter(Identifier modulatorName, int x, int y)
{
	ModulatorMeter *m = addComponent<ModulatorMeter>(modulatorName, x, y, 100, 50);

	m->setScriptProcessor(getScriptProcessor());

	return m;

};

ScriptingApi::Content::ScriptedPlotter * ScriptingApi::Content::addPlotter(Identifier plotterName, int x, int y)
{
	return addComponent<ScriptedPlotter>(plotterName, x, y, 100, 50);

};

ScriptingApi::Content::ScriptPanel * ScriptingApi::Content::addPanel(Identifier panelName, int x, int y)
{
	return addComponent<ScriptPanel>(panelName, x, y, 100, 50);

};

ScriptingApi::Content::ScriptAudioWaveform * ScriptingApi::Content::addAudioWaveform(Identifier audioWaveformName, int x, int y)
{
	return addComponent<ScriptAudioWaveform>(audioWaveformName, x, y, 100, 50);
}

ScriptingApi::Content::ScriptSliderPack * ScriptingApi::Content::addSliderPack(Identifier sliderPackName, int x, int y)
{
	return addComponent<ScriptSliderPack>(sliderPackName, x, y, 200, 100);
}


ScriptingApi::Content::ScriptFloatingTile* ScriptingApi::Content::addFloatingTile(Identifier floatingTileName, int x, int y)
{
	return addComponent<ScriptFloatingTile>(floatingTileName, x, y, 200, 100);
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

	return var();
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
}


void ScriptingApi::Content::beginInitialization()
{
	allowGuiCreation = true;

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

	setUseHighResolutionForPanels(HiseDeviceSimulator::isRetina());

	dynamic_cast<JavascriptMidiProcessor*>(getProcessor())->addToFront(true);
}

void ScriptingApi::Content::setToolbarProperties(const var &toolbarProperties)
{
	NamedValueSet *newSet = &toolbarProperties.getDynamicObject()->getProperties();

	NamedValueSet *set = &getProcessor()->getMainController()->getToolbarPropertiesObject()->getProperties();

	set->clear();

	for (int i = 0; i < newSet->size(); i++)
	{
		set->set(newSet->getName(i), newSet->getValueAt(i));
	}
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

	const ValueTree parent = dynamic_cast<FrontendDataHolder*>(getProcessor()->getMainController())->getValueTree(ProjectHandler::SubDirectories::UserPresets);

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

		var v = components[i]->getValue();

		if (dynamic_cast<ScriptingApi::Content::ScriptLabel*>(components[i].get()) != nullptr)
		{
			getScriptProcessor()->controlCallback(components[i], v);
		}
		else if (v.isObject())
		{
			getScriptProcessor()->controlCallback(components[i], v);
		}
		else
		{
			getProcessor()->setAttribute(i, components[i]->getValue(), sendNotification);
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

	for (int i = 0; i < components.size(); i++)
	{
		if (!components[i]->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset)) continue;

		ValueTree child = v.getChildWithProperty("id", components[i]->name.toString());

		const String childTypeString = child.getProperty("type");

		if (childTypeString.isEmpty()) continue;

		Identifier childType(childTypeString);

		if (child.isValid() && childType == components[i]->getObjectName())
		{
			components[i]->restoreFromValueTree(child);
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
	for (int i = 0; i < components.size(); i++)
	{
		components[i]->setControlCallback(var());
		
		components[i]->cleanScriptChangedPropertyIds();

		if (auto p = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(components[i].get()))
		{
			auto data = p->getConstantValue(0).getDynamicObject();

			data->clear();

			p->setPaintRoutine(var());
			p->setTimerCallback(var());
			p->setMouseCallback(var());
			p->setLoadingCallback(var());
		}
	}
}

void ScriptingApi::Content::rebuildComponentListFromValueTree(bool rebuildContent/*=false*/, Identifier* errorId/*=nullptr*/)
{
	components.clear();

	components.ensureStorageAllocated(contentPropertyData.getNumChildren());

	addComponentsFromValueTree(contentPropertyData);

	

	if (rebuildContent)
	{

		sendRebuildMessage();
	}
}

Result ScriptingApi::Content::createComponentsFromValueTree(const ValueTree& newProperties, bool buildComponentList/*=true*/)
{
	auto oldData = contentPropertyData;

	contentPropertyData = newProperties;

	components.clear();

	Identifier errorId;

	if (buildComponentList)
	{
		rebuildComponentListFromValueTree(true, &errorId);
	}
	else
		contentNeedsRebuilding = true;

	return Result::ok();
}

void ScriptingApi::Content::addComponentsFromValueTree(ValueTree& v)
{
	static const Identifier co("Component");
	static const Identifier coPro("ContentProperties");
	static const Identifier id("id");
	static const Identifier type_("type");
	static const Identifier parent("parentComponent");

	if (v.getType() == co)
	{
		auto sc = Helpers::createComponentFromId(this, v.getProperty(type_).toString(), v.getProperty(id).toString(), 0, 0, 100, 100);

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

ValueTree findChildRecursive(const ValueTree& v, const var& n)
{
	String na = n.toString();

	static const Identifier id_("id");

	ValueTree child = v.getChildWithProperty(id_, n);

	if (child.isValid())
		return child;

	for (int i = 0; i < v.getNumChildren(); i++)
		return findChildRecursive(v.getChild(i), n);

	return ValueTree();
}

ValueTree ScriptingApi::Content::getValueTreeForComponent(const Identifier& id)
{
	var n(id.toString());
	return findChildRecursive(contentPropertyData, n);
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

	while (auto sc = iter.getNextScriptComponent())
	{
		deleteComponent(c, sc->getName(), false);
	}

	c->rebuildComponentListFromValueTree(true);

	b->clearSelection(sendNotification);
}


void ScriptingApi::Content::Helpers::deleteComponent(Content* c, const Identifier& id, bool rebuildContent/*=true*/)
{
	auto childToRemove = c->getValueTreeForComponent(id);

	childToRemove.getParent().removeChild(childToRemove, nullptr);

	if (rebuildContent)
	{
		c->rebuildComponentListFromValueTree(true);
	}
}

void ScriptingApi::Content::Helpers::renameComponent(Content* c, const Identifier& id, const Identifier& newId)
{
	auto childTree = c->getValueTreeForComponent(id);

	if (childTree.isValid())
		childTree.setProperty("id", newId.toString(), nullptr);

	c->rebuildComponentListFromValueTree(true);
}

void ScriptingApi::Content::Helpers::duplicateSelection(Content* c, ReferenceCountedArray<ScriptComponent> selection, int deltaX, int deltaY)
{
	Array<Identifier> newIds;
	newIds.ensureStorageAllocated(selection.size());

	static const Identifier x("x");
	static const Identifier y("y");

	for (auto sc : selection)
	{
		int newX = sc->getPosition().getX() + deltaX;
		int newY = sc->getPosition().getY() + deltaY;

		auto newId = getUniqueIdentifier(c, sc->getName().toString());

		auto cTree = c->getValueTreeForComponent(sc->name);

#if 0
		auto p = sc->getNonDefaultScriptObjectProperties();
		auto& set = p.getDynamicObject()->getProperties();

		set.set("x", newX);
		set.set("y", newY);
#endif

		ValueTree sibling = cTree.createCopy();
		
		sibling.setProperty(x, newX, nullptr);
		sibling.setProperty(y, newY, nullptr);

		sibling.setProperty("id", newId.toString(), nullptr);
		
		cTree.getParent().addChild(sibling, -1, nullptr);
	}

	c->rebuildComponentListFromValueTree(true);

	auto b = c->getProcessor()->getMainController()->getScriptComponentEditBroadcaster();

	b->clearSelection(dontSendNotification);
	
	ScriptComponentSelection newSelection;

	for (auto id : newIds)
	{
		auto sc = c->getComponentWithName(id);
		jassert(sc != nullptr);
		newSelection.add(sc);
	}

	b->setSelection(newSelection, sendNotification);
}



ScriptingApi::Content::ScriptComponent* ScriptingApi::Content::Helpers::createComponentFromId(Content* c, const Identifier& typeId, const Identifier& name, int x, int y, int w, int h)
{
	

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

	if (auto sc = createComponentIfTypeMatches<ModulatorMeter>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptAudioWaveform>(c, typeId, name, x, y, w, h))
		return sc;

	if (auto sc = createComponentIfTypeMatches<ScriptFloatingTile>(c, typeId, name, x, y, w, h))
		return sc;

	return nullptr;
}

String ScriptingApi::Content::Helpers::createScriptVariableDeclaration(ReferenceCountedArray<ScriptComponent> selection)
{

	String s;
	NewLine nl;

	for (int i = 0; i < selection.size(); i++)
	{
		auto c = selection[i];

		s << "const var " << c->name.toString() << " = Content.getComponent(\"" << c->name.toString() << "\");" << nl;;
	}

	s << nl;

	return s;
}

void ScriptingApi::Content::Helpers::gotoLocation(ScriptComponent* sc)
{
	Processor* p = dynamic_cast<Processor*>(const_cast<ProcessorWithScriptingContent*>(sc->parent->getScriptProcessor()));
	auto jp = dynamic_cast<JavascriptProcessor*>(p);

	auto engine = jp->getScriptEngine();

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		auto info = engine->getDebugInformation(i);

		if (info->getObject() == sc)
		{
			auto location = info->getLocation();

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

	auto result = jp->compileScript();

	DebugableObject::Helpers::gotoLocation(dynamic_cast<Processor*>(jp)->getMainController()->getMainSynthChain(), result.r.getErrorMessage());

	sc->setPropertyToLookFor(Identifier());
	
	jp->compileScript();
}

void ScriptingApi::Content::Helpers::pasteProperties(ReferenceCountedArray<ScriptComponent> selection, var clipboardData)
{
	if (selection.size() != 0)
	{
		if (auto dyn = clipboardData.getDynamicObject())
		{
			dyn->removeProperty("x");
			dyn->removeProperty("y");

			auto content = selection.getFirst()->parent;

			for (auto s : selection)
			{
				auto child = content->getValueTreeForComponent(s->name);
				
				ValueTreeConverters::copyDynamicObjectPropertiesToValueTree(child, clipboardData);
			}

			content->rebuildComponentListFromValueTree(true);
		}
	}
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

void ScriptingApi::Content::Helpers::setComponentValueTreeFromJSON(Content* c, const Identifier& id, const var& data)
{
	auto oldTree = c->getValueTreeForComponent(id);

	auto parent = oldTree.getParent();

	int index = parent.indexOf(oldTree);

	parent.removeChild(oldTree, nullptr);

	auto newTree = ValueTreeConverters::convertDynamicObjectToContentProperties(data);

	parent.addChild(newTree, index, nullptr);
	
	c->rebuildComponentListFromValueTree(true);
}

struct ContentValueTreeHelpers
{
	static void removeFromParent(ValueTree& v)
	{
		v.getParent().removeChild(v, nullptr);
	}

	static Result setNewParent(ValueTree& newParent, ValueTree& child)
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

	static void updatePosition(ValueTree& v, Point<int> localPoint, Point<int> oldParentPosition)
	{
		static const Identifier x("x");
		static const Identifier y("y");

		auto newPoint = localPoint - oldParentPosition;

		v.setProperty(x, newPoint.getX(), nullptr);
		v.setProperty(y, newPoint.getY(), nullptr);
	}

	static Point<int> getLocalPosition(ValueTree& v)
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

	static bool getAbsolutePosition(ValueTree& v, Point<int>& offset)
	{
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier root("ContentProperties");

		auto parent = v.getParent();

		if (parent.isValid() && parent.getType() != root)
		{
			offset.addXY((int)parent.getProperty(x), (int)parent.getProperty(y));

			return getAbsolutePosition(v.getParent(), offset);
		}

		return false;
	}

	static void addComponentToValueTreeRecursive(Array<Identifier>& usedIds, const ScriptComponentSelection list, ValueTree& v)
	{

		static const Identifier type_("type");
		static const Identifier id_("id");
		static const Identifier parentComponent("parentComponent");
		static const Identifier x("x");
		static const Identifier y("y");
		static const Identifier w("width");
		static const Identifier h("height");

		for (auto sc : list)
		{
			if (usedIds.contains(sc->getName()))
				continue;

			usedIds.add(sc->getName());

			auto data = sc->getNonDefaultScriptObjectProperties();

			ValueTree child("Component");

			auto pos = sc->getPosition();

			child.setProperty(type_, sc->getObjectName().toString(), nullptr);
			child.setProperty(id_, sc->name.toString(), nullptr);

			// Copy these because the default values for the position are a bit funky...
			child.setProperty(x, pos.getX(), nullptr);
			child.setProperty(y, pos.getY(), nullptr);
			child.setProperty(w, pos.getWidth(), nullptr);
			child.setProperty(h, pos.getHeight(), nullptr);

			ValueTreeConverters::copyDynamicObjectPropertiesToValueTree(child, data);

			// Remove this because it's handled via the tree hierarchy...
			child.removeProperty(parentComponent, nullptr);

			ScriptComponentSelection childList;

			for (int i = 0; i < sc->getNumChildComponents(); i++)
			{
				childList.add(sc->getChildComponent(i));
			}

			addComponentToValueTreeRecursive(usedIds, childList, child);

			v.addChild(child, -1, nullptr);
		}
	}
};




void ScriptingApi::Content::Helpers::moveComponentsAfter(ScriptComponent* target, var list)
{
	auto c = target->parent;

	auto tTree = c->getValueTreeForComponent(target->name);

	auto parent = tTree.getParent();

	auto pPosition = ContentValueTreeHelpers::getLocalPosition(parent);

	auto insertAfterTree = tTree;

	if (auto ar = list.getArray())
	{
		for (auto v : *ar)
		{
			auto mc = dynamic_cast<ScriptComponent*>(v.getObject());


			auto mTree = c->getValueTreeForComponent(mc->name);

			auto mPosition = ContentValueTreeHelpers::getLocalPosition(mTree);
			ContentValueTreeHelpers::getAbsolutePosition(mTree, mPosition);

			if (mTree == parent)
			{
				break;
			}

			ContentValueTreeHelpers::removeFromParent(mTree);

			
			int currentIndex = parent.indexOf(insertAfterTree) + 1;

			ContentValueTreeHelpers::updatePosition(mTree, mPosition, pPosition);

			parent.addChild(mTree, currentIndex, nullptr);

			insertAfterTree = mTree;
		}
	}

	c->rebuildComponentListFromValueTree(true);
}


Result ScriptingApi::Content::Helpers::setParentComponent(ScriptComponent* parent, var newChildren, bool rebuildNow/*=true*/)
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

		if (rebuildNow)
		{
			content->rebuildComponentListFromValueTree(true);
			content->getScriptProcessor()->getMainController_()->getScriptComponentEditBroadcaster()->clearSelection(sendNotification);
		}

		return Result::ok();
	}
}

void ScriptingApi::Content::Helpers::copyComponentSnapShotToValueTree(Content* c)
{
	ValueTree v("ContentProperties");

	Array<Identifier> childComponentIds;

	ContentValueTreeHelpers::addComponentToValueTreeRecursive(childComponentIds, c->components, v);

	c->contentPropertyData = v;

	c->rebuildComponentListFromValueTree(true);
}

} // namespace hise