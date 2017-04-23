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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
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
#else
#define SEND_MESSAGE(broadcaster) {	if (MessageManager::getInstance()->isThisTheMessageThread()) broadcaster->sendSynchronousChangeMessage(); else broadcaster->sendChangeMessage();}
#endif




#define ADD_TO_TYPE_SELECTOR(x) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::x, propertyIds.getLast()))
#define ADD_AS_SLIDER_TYPE(min, max, interval) (ScriptComponentPropertyTypeSelector::addToTypeSelector(ScriptComponentPropertyTypeSelector::SliderSelector, propertyIds.getLast(), min, max, interval))

#include <cmath>



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
};


ScriptingApi::Content::ScriptComponent::ScriptComponent(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name_, int x, int y, int width, int height, int numConstants) :
ConstScriptingObject(base, numConstants),
name(name_),
value(0.0),
parent(parentContent),
skipRestoring(false),
componentProperties(new DynamicObject()),
changed(false),
parentComponentIndex(-1)
{
	propertyIds.add(Identifier("text"));
	propertyIds.add(Identifier("visible"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("enabled"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("x"));			ADD_AS_SLIDER_TYPE(0, 900, 1);
	propertyIds.add(Identifier("y"));			ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	propertyIds.add(Identifier("width"));		ADD_AS_SLIDER_TYPE(0, 900, 1);
	propertyIds.add(Identifier("height"));		ADD_AS_SLIDER_TYPE(0, MAX_SCRIPT_HEIGHT, 1);
	propertyIds.add(Identifier("min"));
	propertyIds.add(Identifier("max"));
	propertyIds.add(Identifier("tooltip"));
	propertyIds.add(Identifier("bgColour"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("itemColour"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("itemColour2")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("textColour"));  ADD_TO_TYPE_SELECTOR(SelectorTypes::ColourPickerSelector);
	propertyIds.add(Identifier("macroControl")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("zOrder"));
	propertyIds.add(Identifier("saveInPreset")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("isPluginParameter")); ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("pluginParameterName"));
	propertyIds.add(Identifier("useUndoManager"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("parentComponent"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	

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
	setDefaultValue(Properties::zOrder, "Normal order");
	setDefaultValue(Properties::saveInPreset, true);
	setDefaultValue(Properties::isPluginParameter, false);
	setDefaultValue(Properties::pluginParameterName, "");
	setDefaultValue(Properties::useUndoManager, false);
	setDefaultValue(Properties::parentComponent, "");

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
	else if (id == getIdFor(zOrder))
	{
		StringArray sa;

		sa.add("Normal order");
		sa.add("Always on top");

		return sa;
	}
	else if (id == getIdFor(parentComponent))
	{
		auto c = getScriptProcessor()->getScriptingContent();

		StringArray sa;

		for (int i = 0; i < c->getNumComponents(); i++)
		{
			if (c->getComponent(i) == this) break;
			sa.add(c->getComponent(i)->getName().toString());
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

void ScriptingApi::Content::ScriptComponent::doubleClickCallback(const MouseEvent &, Component* componentToNotify)
{
#if USE_BACKEND
	getScriptProcessor()->getMainController_()->setEditedScriptComponent(this, componentToNotify);
#else
	ignoreUnused(componentToNotify);
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
	else if (id == getIdFor(x) || id == getIdFor(y) || id == getIdFor(width) ||
			 id == getIdFor(height) || id == getIdFor(visible) || id == getIdFor(enabled))
	{
		notifyChildComponents();
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

String ScriptingApi::Content::ScriptComponent::getScriptObjectPropertiesAsJSON() const
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

	var string = var(clone);

	return JSON::toString(string, false);
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
				setScriptObjectPropertyWithChangeMessage(priorityProperties[i], dataSet[priorityProperties[i]], dontSendNotification);
			}
		}

		for (int i = 0; i < dataSet.size(); i++)
		{
			Identifier propertyId = dataSet.getName(i);

			if (priorityProperties.contains(propertyId)) continue;

			setScriptObjectPropertyWithChangeMessage(propertyId, dataSet.getValueAt(i), dontSendNotification);
		}
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
		reportScriptError("Tried to change the macro setup after onInit()");

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
		return false;
	}

	if (childComponent == this)
	{
		reportScriptError("Can't add itself as parent.");
		return false;
	}
	if (childComponent->isChildComponent(this))
	{
		reportScriptError("Can't add a parent as child component");
		return false;
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

	propertyIds.add(Identifier("mode"));			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("style"));			ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("stepSize"));
	propertyIds.add(Identifier("middlePosition"));
	propertyIds.add(Identifier("defaultValue"));
	propertyIds.add(Identifier("suffix"));
	propertyIds.add(Identifier("filmstripImage"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	propertyIds.add(Identifier("numStrips"));
	propertyIds.add(Identifier("isVertical"));		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("scaleFactor"));
	propertyIds.add(Identifier("mouseSensitivity"));
	propertyIds.add(Identifier("dragDirection"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

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
	if (getScriptProcessor() != nullptr)
	{
		ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
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

		pool->releasePoolData(image);

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "Use default skin");
			image = nullptr;
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);


#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName, false);

			jassert(image != nullptr);

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName(), false);

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
		reportScriptError("getMaxValue() can only be called on sliders in 'Range' mode.");
		return 0.0;
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
		setMidPoint(getScriptObjectProperty(ScriptSlider::Properties::middlePosition));
	}
}


ScriptingApi::Content::ScriptButton::ScriptButton(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int, int) :
ScriptComponent(base, parentContent, name, x, y, 128, 32),
image(nullptr)
{
	propertyIds.add("filmstripImage");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	propertyIds.add("isVertical");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("scaleFactor");
	propertyIds.add("radioGroup");

	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::textColour));
	deactivatedProperties.removeAllInstancesOf(getIdFor(ScriptComponent::Properties::isPluginParameter));

	setDefaultValue(ScriptButton::Properties::filmstripImage, "");
	setDefaultValue(ScriptButton::Properties::isVertical, true);
	setDefaultValue(ScriptButton::Properties::scaleFactor, 1.0f);
	setDefaultValue(ScriptButton::Properties::radioGroup, 0);
}

ScriptingApi::Content::ScriptButton::~ScriptButton()
{
	if (getScriptProcessor() != nullptr)
	{
		ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
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

		pool->releasePoolData(image);

		if (newValue == "Use default skin" || newValue == "")
		{
			setScriptObjectProperty(filmstripImage, "");
			image = nullptr;
		}
		else
		{
			setScriptObjectProperty(filmstripImage, newValue);

#if USE_FRONTEND

			String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(newValue);

			image = pool->loadFileIntoPool(poolName, false);

			jassert(image != nullptr);

#else


			File actualFile = getExternalFile(newValue);

			image = pool->loadFileIntoPool(actualFile.getFullPathName(), false);

#endif
		}
	}

	ScriptComponent::setScriptObjectPropertyWithChangeMessage(id, newValue, notifyEditor);
}

StringArray ScriptingApi::Content::ScriptButton::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(filmstripImage)) return ScriptComponent::getOptionsFor(id);

	StringArray sa;

	sa.add("Load new File");

	sa.add("Use default skin");
	sa.addArray(getProcessor()->getMainController()->getSampleManager().getImagePool()->getFileNameList());

	return sa;
}


struct ScriptingApi::Content::ScriptLabel::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptLabel, setEditable);
};

ScriptingApi::Content::ScriptLabel::ScriptLabel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier name, int x, int y, int width, int) :
ScriptComponent(base, parentContent, name, x, y, width, 16)
{
	propertyIds.add(Identifier("fontName"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("fontSize"));	ADD_AS_SLIDER_TYPE(1, 200, 1);
	propertyIds.add(Identifier("fontStyle"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("alignment"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add(Identifier("editable"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("multiline"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);


	componentProperties->setProperty(getIdFor(FontName), 0);
	componentProperties->setProperty(getIdFor(FontSize), 0);
	componentProperties->setProperty(getIdFor(FontStyle), 0);
	componentProperties->setProperty(getIdFor(Alignment), 0);
	componentProperties->setProperty(getIdFor(Editable), 0);
	componentProperties->setProperty(getIdFor(Multiline), 0);

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
	case Alignment: sa.add("left");
		sa.add("right");
		sa.add("top");
		sa.add("bottom");
		sa.add("centred");
		sa.add("centredTop");
		sa.add("centredBottom");
		sa.add("topLeft");
		sa.add("topRight");
		sa.add("bottomLeft");
		sa.add("bottomRight");
		break;
	default:		sa = ScriptComponent::getOptionsFor(id);
	}

	return sa;
}

Justification ScriptingApi::Content::ScriptLabel::getJustification()
{
	StringArray options = getOptionsFor(getIdFor(Alignment));

	String justAsString = getScriptObjectProperty(Alignment);
	int index = options.indexOf(justAsString);

	if (index == -1)
	{
		return Justification(Justification::centredLeft);
	}

	Array<Justification::Flags> justifications;
	justifications.ensureStorageAllocated(options.size());

	justifications.add(Justification::left);
	justifications.add(Justification::right);
	justifications.add(Justification::top);
	justifications.add(Justification::bottom);
	justifications.add(Justification::centred);
	justifications.add(Justification::centredTop);
	justifications.add(Justification::centredBottom);
	justifications.add(Justification::topLeft);
	justifications.add(Justification::topRight);
	justifications.add(Justification::bottomLeft);
	justifications.add(Justification::bottomRight);

	return justifications[index];
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
			return -1.0f;
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

    reportScriptError(otherTableId + " was not found.");
    
	useOtherTable = false;
	referencedTable = nullptr;

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
};

ScriptingApi::Content::ScriptSliderPack::ScriptSliderPack(ProcessorWithScriptingContent *base, Content *parentContent, Identifier imageName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, imageName, x, y, width, height),
packData(new SliderPackData()),
existingData(nullptr)
{
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));



	propertyIds.add("sliderAmount");		ADD_AS_SLIDER_TYPE(0, 128, 1);
	propertyIds.add("stepSize");
	propertyIds.add("flashActive");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("showValueOverlay");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("ProcessorId");         ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);

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

	setDefaultValue(SliderAmount, 16);
	setDefaultValue(StepSize, 0.01);

	ADD_API_METHOD_2(setSliderAtIndex);
	ADD_API_METHOD_1(getSliderValueAt);
	ADD_API_METHOD_1(setAllValues);
	ADD_API_METHOD_0(getNumSliders);
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

void ScriptingApi::Content::ScriptSliderPack::connectToOtherSliderPack(const String &otherPackId)
{
	if (otherPackId.isEmpty()) return;

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return;

	Processor::Iterator<Processor> it(mp->getOwnerSynth(), false);

	Processor *p;

	while ((p = it.getNextProcessor()) != nullptr)
	{
		if ((dynamic_cast<SliderPackProcessor*>(p) != nullptr) && p->getId() == otherPackId)
		{
			existingData = dynamic_cast<SliderPackProcessor*>(p)->getSliderPackData(0);

			

			return;
		}
	}

    reportScriptError(otherPackId + " was not found.");
    
	existingData = nullptr;
}

StringArray ScriptingApi::Content::ScriptSliderPack::getOptionsFor(const Identifier &id)
{
	if (id != getIdFor(ProcessorId)) return ScriptComponent::getOptionsFor(id);

	MidiProcessor* mp = dynamic_cast<MidiProcessor*>(getProcessor());
	if (mp == nullptr) return StringArray();

	return ProcessorHelpers::getAllIdsForType<SliderPackProcessor>(mp->getOwnerSynth());
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

	propertyIds.add("alpha");		ADD_AS_SLIDER_TYPE(0.0, 1.0, 0.01);
	propertyIds.add("fileName");	ADD_TO_TYPE_SELECTOR(SelectorTypes::FileSelector);
	propertyIds.add("offset");
	propertyIds.add("scale");
	propertyIds.add("allowCallbacks");		ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add("popupMenuItems");		ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	propertyIds.add("popupOnRightClick");	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);

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
	if (getProcessor() != nullptr)
	{
		ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

		pool->releasePoolData(image);
	}
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

	const bool imageWasEmpty = (image == nullptr);

	CHECK_COPY_AND_RETURN_10(getProcessor());

	if (absoluteFileName.isEmpty())
	{
		setScriptObjectProperty(FileName, absoluteFileName);
		return;
	}

	ImagePool *pool = getProcessor()->getMainController()->getSampleManager().getImagePool();

	pool->releasePoolData(image);

	setScriptObjectProperty(FileName, absoluteFileName);

#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(absoluteFileName);

	image = pool->loadFileIntoPool(poolName, false);

	jassert(image != nullptr);

#else

	File actualFile = getExternalFile(absoluteFileName);

	image = pool->loadFileIntoPool(actualFile.getFullPathName(), forceUseRealFile);

#endif

	if (imageWasEmpty && image != nullptr)
	{
		setScriptObjectProperty(ScriptComponent::width, image->getWidth());
		setScriptObjectProperty(ScriptComponent::height, image->getHeight());
	}

	parent->sendChangeMessage();
};



ScriptCreatedComponentWrapper * ScriptingApi::Content::ScriptImage::createComponentWrapper(ScriptContentComponent *content, int index)
{
	return new ScriptCreatedComponentWrappers::ImageWrapper(content, this, index);
}

const Image * ScriptingApi::Content::ScriptImage::getImage() const
{
	return image;
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
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setPaintRoutine);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setMouseCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setTimerCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, startTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, stopTimer);
	API_VOID_METHOD_WRAPPER_0(ScriptPanel, changed);
	API_VOID_METHOD_WRAPPER_2(ScriptPanel, loadImage);
	API_VOID_METHOD_WRAPPER_1(ScriptPanel, setDraggingBounds);
};

ScriptingApi::Content::ScriptPanel::ScriptPanel(ProcessorWithScriptingContent *base, Content *parentContent, Identifier panelName, int x, int y, int width, int height) :
ScriptComponent(base, parentContent, panelName, x, y, width, height, 1),
graphics(new ScriptingObjects::GraphicsObject(base, this)),
repainter(this),
controlSender(this, base)
{
	//deactivatedProperties.add(getIdFor(ScriptComponent::Properties::max));
	//deactivatedProperties.add(getIdFor(ScriptComponent::Properties::min));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::bgColour));
	deactivatedProperties.add(getIdFor(ScriptComponent::Properties::macroControl));
    deactivatedProperties.add(getIdFor(ScriptComponent::Properties::text));
	//deactivatedProperties.add(getIdFor(ScriptComponent::Properties::enabled));
	//deactivatedProperties.add(getIdFor(ScriptComponent::Properties::tooltip));
	

	propertyIds.add("borderSize");					ADD_AS_SLIDER_TYPE(0, 20, 1);
	propertyIds.add("borderRadius");				ADD_AS_SLIDER_TYPE(0, 20, 1);
    propertyIds.add("opaque");						ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("allowDragging");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add("allowCallbacks");				ADD_TO_TYPE_SELECTOR(SelectorTypes::ChoiceSelector);
	propertyIds.add("popupMenuItems");				ADD_TO_TYPE_SELECTOR(SelectorTypes::MultilineSelector);
	propertyIds.add("popupOnRightClick");			ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("popupMenuAlign"));  ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	propertyIds.add(Identifier("selectedPopupIndex"));
	propertyIds.add(Identifier("stepSize"));
	propertyIds.add(Identifier("enableMidiLearn"));	ADD_TO_TYPE_SELECTOR(SelectorTypes::ToggleSelector);
	
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
	
	addConstant("data", new DynamicObject());

	ADD_API_METHOD_0(repaint);
	ADD_API_METHOD_1(setPaintRoutine);
	ADD_API_METHOD_1(setMouseCallback);
	ADD_API_METHOD_1(setTimerCallback);
	ADD_API_METHOD_0(changed);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_2(loadImage);
	ADD_API_METHOD_1(setDraggingBounds);
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

void ScriptingApi::Content::ScriptPanel::setPaintRoutine(var paintFunction)
{
	paintRoutine = paintFunction;
	repaint();
}

void ScriptingApi::Content::ScriptPanel::internalRepaint()
{
    const double scaleFactor = Desktop::getInstance().getDisplays().getMainDisplay().scale;
    
	paintCanvas = Image(Image::PixelFormat::ARGB, (int)(scaleFactor * (double)getScriptObjectProperty(ScriptComponent::Properties::width)), (int)(scaleFactor * (double)getScriptObjectProperty(ScriptComponent::Properties::height)), true);

	Graphics g(paintCanvas);

	g.addTransform(AffineTransform::scale((float)scaleFactor));
	
	var thisObject(this);
	var arguments = var(graphics);
	var::NativeFunctionArgs args(thisObject, &arguments, 1);

	graphics->setGraphics(&g, &paintCanvas);

	Result r = Result::ok();

	HiseJavascriptEngine* engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();

	engine->callExternalFunction(paintRoutine, args, &r);

	if (r.failed())
	{
		reportScriptError(r.getErrorMessage());
	}

	graphics->setGraphics(nullptr, nullptr);

	SEND_MESSAGE(this);
}

void ScriptingApi::Content::ScriptPanel::setMouseCallback(var mouseCallbackFunction)
{
	mouseRoutine = mouseCallbackFunction;
}

void ScriptingApi::Content::ScriptPanel::mouseCallback(var mouseInformation)
{
	if (!mouseRoutine.isUndefined())
	{
		var thisObject(this);

		var::NativeFunctionArgs args(thisObject, &mouseInformation, 1);

		Result r = Result::ok();

		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->callExternalFunction(mouseRoutine, args, &r);

		if (r.failed())
		{
			reportScriptError(r.getErrorMessage());
		}
	}
}

void ScriptingApi::Content::ScriptPanel::setTimerCallback(var timerCallback_)
{
	timerRoutine = timerCallback_;
}

void ScriptingApi::Content::ScriptPanel::timerCallback()
{
	if (!timerRoutine.isUndefined())
	{
		var thisObject(this);
		var::NativeFunctionArgs args(thisObject, nullptr, 0);

		Result r = Result::ok();

		dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->callExternalFunction(timerRoutine, args, &r);

		if (r.failed())
		{
			reportScriptError(r.getErrorMessage());
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

	const Image *newImage = pool->loadFileIntoPool(poolName, false);

	jassert(newImage != nullptr);

#else

	File actualFile = getExternalFile(imageName);

	const Image *newImage = pool->loadFileIntoPool(actualFile.getFullPathName(), true);

#endif

	if (newImage != nullptr)
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

void ScriptingApi::Content::ScriptPanel::AsyncControlCallbackSender::handleAsyncUpdate()
{
	if (parent.get() != nullptr)
	{
		p->controlCallback(parent, parent->getValue());
	}
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

    reportScriptError(processorId + " was not found.");
    
	connectedProcessor = nullptr;
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

		if (fileName.isNotEmpty())
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

// ====================================================================================================== Content functions


ScriptingApi::Content::Content(ProcessorWithScriptingContent *p) :
ScriptingObject(p),
height(50),
width(-1),
name(String()),
allowGuiCreation(true),
colour(Colour(0xff777777))
{
	setMethod("addButton", Wrapper::addButton);
	setMethod("addKnob", Wrapper::addKnob);
	setMethod("addLabel", Wrapper::addLabel);
	setMethod("addComboBox", Wrapper::addComboBox);
	setMethod("addTable", Wrapper::addTable);
	setMethod("addImage", Wrapper::addImage);
	setMethod("addModulatorMeter", Wrapper::addModulatorMeter);
	setMethod("addPlotter", Wrapper::addPlotter);
	setMethod("addPanel", Wrapper::addPanel);
	setMethod("addAudioWaveform", Wrapper::addAudioWaveform);
	setMethod("addSliderPack", Wrapper::addSliderPack);
	setMethod("setContentTooltip", Wrapper::setContentTooltip);
	setMethod("setToolbarProperties", Wrapper::setToolbarProperties);
	setMethod("setHeight", Wrapper::setHeight);
	setMethod("setWidth", Wrapper::setWidth);
    setMethod("makeFrontInterface", Wrapper::makeFrontInterface);
	setMethod("setName", Wrapper::setName);
	setMethod("setPropertiesFromJSON", Wrapper::setPropertiesFromJSON);
	setMethod("storeAllControlsAsPreset", Wrapper::storeAllControlsAsPreset);
	setMethod("restoreAllControlsFromPreset", Wrapper::restoreAllControlsFromPreset);
	setMethod("setColour", Wrapper::setColour);
	setMethod("clear", Wrapper::clear);
	setMethod("createPath", Wrapper::createPath);
}

ScriptingApi::Content::~Content()
{
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



ScriptingApi::Content::ScriptComponent * ScriptingApi::Content::getComponent(int index)
{
	if (index == -1) return nullptr;

	return components[index];
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

		if (v.isObject())
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

#undef ADD_TO_TYPE_SELECTOR
#undef ADD_AS_SLIDER_TYPE
#undef SEND_MESSAGE
