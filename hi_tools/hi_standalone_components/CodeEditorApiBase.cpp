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

namespace hise { using namespace juce;



juce::AttributedString ValueTreeApiHelpers::createAttributedStringFromApi(const ValueTree &method, const String &, bool multiLine, Colour textColour)
{
	AttributedString help;

	const String name = method.getProperty(Identifier("name")).toString();
	const String arguments = method.getProperty(Identifier("arguments")).toString();
	const String description = method.getProperty(Identifier("description")).toString();
	const String returnType = method.getProperty("returnType", "void");


	help.setWordWrap(AttributedString::byWord);


	if (multiLine)
	{
		help.setJustification(Justification::topLeft);
		help.setLineSpacing(1.5f);
		help.append("Name:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(name, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		help.append(arguments + "\n\n", GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.6f));
		help.append("Description:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(description + "\n\n", GLOBAL_FONT(), textColour.withAlpha(0.8f));

		help.append("Return Type:\n  ", GLOBAL_BOLD_FONT(), textColour);
		help.append(method.getProperty("returnType", "void"), GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
	}

	else
	{
		help.setJustification(Justification::centredLeft);
		help.append(description, GLOBAL_BOLD_FONT(), textColour.withAlpha(0.8f));

		const String oneLineReturnType = method.getProperty("returnType", "");

		if (oneLineReturnType.isNotEmpty())
		{
			help.append("\nReturn Type: ", GLOBAL_BOLD_FONT(), textColour);
			help.append(oneLineReturnType, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		}
	}

	return help;
}

String ValueTreeApiHelpers::createCodeToInsert(const ValueTree &method, const String &className)
{
	const String name = method.getProperty(Identifier("name")).toString();

	if (name == "setMouseCallback")
	{
		const String argumentName = "event";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setLoadingCallback")
	{
		const String argumentName = "isPreloading";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setTimerCallback")
	{
		const String argumentName = "";
		String functionDef = className;
		functionDef << "." << name + "(function(" << argumentName << ")\n";
		functionDef << "{\n\t\n});\n";

		return functionDef;
	}
	else if (name == "setPaintRoutine")
	{
	const String argumentName = "g";
	String functionDef = className;
	functionDef << "." << name + "(function(" << argumentName << ")\n";
	functionDef << "{\n\t\n});\n";

	return functionDef;
	}
	else
	{
	const String arguments = method.getProperty(Identifier("arguments")).toString();

	return String(className + "." + name + arguments);
	}
}

void ValueTreeApiHelpers::getColourAndCharForType(int type, char &c, Colour &colour)
{
	const float alpha = 0.6f;
	const float brightness = 0.8f;

	switch (type)
	{
	case (int)0:	c = 'R'; break;
	case (int)1:		c = 'V'; break;
	case (int)2:			c = 'C'; break;
	case (int)3:	c = 'I'; break;
	case (int)4:			c = 'G'; break;
	case (int)5:			c = 'F'; break;
	case (int)6:			c = 'A'; break;
	case (int)7:		c = 'F'; break;
	case (int)8:		c = 'N'; break;
	default:											c = 'V'; break;
	}

	switch (c)
	{
	case 'I': colour = Colours::blue.withAlpha(alpha).withBrightness(brightness); break;
	case 'V': colour = Colours::cyan.withAlpha(alpha).withBrightness(brightness); break;
	case 'G': colour = Colours::green.withAlpha(alpha).withBrightness(brightness); break;
	case 'C': colour = Colours::yellow.withAlpha(alpha).withBrightness(brightness); break;
	case 'R': colour = Colours::red.withAlpha(alpha).withBrightness(brightness); break;
	case 'A': colour = Colours::orange.withAlpha(alpha).withBrightness(brightness); break;
	case 'F': colour = Colours::purple.withAlpha(alpha).withBrightness(brightness); break;
	case 'E': colour = Colours::chocolate.withAlpha(alpha).withBrightness(brightness); break;
	case 'N': colour = Colours::pink.withAlpha(alpha).withBrightness(brightness); break;
	}
}

void ApiProviderBase::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	colour = Colours::white;
	letter = 'U'; // You know for, like "unused"...
}

hise::DebugableObjectBase* ApiProviderBase::getDebugObject(const String& token)
{
	if (token.isNotEmpty())
	{
		int numObjects = getNumDebugObjects();

		Identifier id(token);

		for (int i = 0; i < numObjects; i++)
		{
			auto info = getDebugInformation(i);

			if (auto obj = info->getObject())
			{
				if (obj->getObjectName() == id || obj->getInstanceName() == id)
				{
					// Don't spit out internal objects by default...
					if (obj->isInternalObject())
						continue;

					return obj;
				}
			}
		}

		return nullptr;
	}

	return nullptr;
}

ApiProviderBase::Holder::~Holder()
{}

void ApiProviderBase::Holder::addEditor(Component* editor)
{
	repaintUpdater.editors.add(editor);
}

void ApiProviderBase::Holder::removeEditor(Component* editor)
{
	repaintUpdater.editors.removeAllInstancesOf(editor);
}

int ApiProviderBase::Holder::getCodeFontSize() const
{ return 15; }

void ApiProviderBase::Holder::setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos)
{}

JavascriptCodeEditor* ApiProviderBase::Holder::getActiveEditor()
{ return nullptr; }

ValueTree ApiProviderBase::Holder::createApiTree()
{ return {}; }

void ApiProviderBase::Holder::jumpToDefinition(const String& token, const String& namespaceId)
{
	ignoreUnused(token, namespaceId);
}

bool ApiProviderBase::Holder::handleKeyPress(const KeyPress& k, Component* c)
{
	ignoreUnused(k, c);
	return false; 
}

void ApiProviderBase::Holder::addPopupMenuItems(PopupMenu& m, Component* c, const MouseEvent& e)
{
	ignoreUnused(m, c, e);
}

bool ApiProviderBase::Holder::performPopupMenuAction(int menuId, Component* c)
{
	ignoreUnused(menuId, c);
	return false;
}

void ApiProviderBase::Holder::handleBreakpoints(const Identifier& codeFile, Graphics& g, Component* c)
{
	ignoreUnused(codeFile, g, c);
}

void ApiProviderBase::Holder::handleBreakpointClick(const Identifier& codeFile, CodeEditorComponent& ed,
	const MouseEvent& e)
{
	ignoreUnused(codeFile, ed, e);
}

juce::ReadWriteLock& ApiProviderBase::Holder::getDebugLock()
{ return debugLock; }

bool ApiProviderBase::Holder::shouldReleaseDebugLock() const
{ return wantsToCompile; }

ApiProviderBase::Holder::CompileDebugLock::CompileDebugLock(Holder& h):
	p(h),
	prevValue(h.wantsToCompile),
	sl(h.getDebugLock())
{
	p.wantsToCompile = true;
}

ApiProviderBase::Holder::CompileDebugLock::~CompileDebugLock()
{
	p.wantsToCompile = prevValue;
}

void ApiProviderBase::Holder::RepaintUpdater::update(int index)
{
	if (lastIndex != index)
	{
		lastIndex = index;
		triggerAsyncUpdate();
	}
}

void ApiProviderBase::Holder::RepaintUpdater::handleAsyncUpdate()
{
	for (int i = 0; i < editors.size(); i++)
	{
		editors[i]->repaint();
	}
}

ApiProviderBase* ApiProviderBase::ApiComponentBase::getProviderBase()
{
	if (holder != nullptr)
		return holder->getProviderBase();

	return nullptr;
}

void ApiProviderBase::ApiComponentBase::providerWasRebuilt()
{}

void ApiProviderBase::ApiComponentBase::providerCleared()
{}

void ApiProviderBase::ApiComponentBase::registerAtHolder()
{
	if (holder != nullptr)
		holder->registeredComponents.addIfNotAlreadyThere(this);
}

void ApiProviderBase::ApiComponentBase::deregisterAtHolder()
{
	if (holder != nullptr)
		holder->registeredComponents.removeAllInstancesOf(this);
}

ApiProviderBase::ApiComponentBase::ApiComponentBase(Holder* h):
	holder(h)
{
	registerAtHolder();
}

ApiProviderBase::ApiComponentBase::~ApiComponentBase()
{
	deregisterAtHolder();
}

ApiProviderBase::~ApiProviderBase()
{}

String ApiProviderBase::getHoverString(const String& token)
{ 
	if (auto obj = getDebugObject(token))
	{
		String s;

		s << obj->getDebugDataType() << " " << obj->getDebugName() << ": " << obj->getDebugValue();
		return s;
	}

	return "";
}

void ApiProviderBase::Holder::rebuild()
{
	for (auto c : registeredComponents)
	{
		if (c != nullptr)
			c->providerWasRebuilt();
	}
}

void ApiProviderBase::Holder::sendClearMessage()
{
    for (auto c : registeredComponents)
    {
        if (c != nullptr)
            c->providerCleared();
    }
}

void DebugableObjectBase::updateLocation(Location& l, var possibleObject)
{
	if (auto obj = dynamic_cast<DebugableObjectBase*>(possibleObject.getObject()))
	{
		auto newLocation = obj->getLocation();
		if (newLocation.charNumber != 0)
			l = newLocation;
	}
}

DynamicDebugableObjectWrapper::DynamicDebugableObjectWrapper(DynamicObject::Ptr obj_, const Identifier& className_,
	const Identifier& instanceId_):
	obj(obj_),
	className(className_),
	instanceId(instanceId_)
{

}

Identifier DynamicDebugableObjectWrapper::getObjectName() const
{ return className; }

Identifier DynamicDebugableObjectWrapper::getInstanceName() const
{ return instanceId; }

String DynamicDebugableObjectWrapper::getDebugValue() const
{ return getInstanceName().toString(); }

void DynamicDebugableObjectWrapper::getAllFunctionNames(Array<Identifier>& functions) const
{
	for (const auto& p : obj->getProperties())
	{
		if (p.value.isMethod())
			functions.add(p.name);
	}
}

void DynamicDebugableObjectWrapper::getAllConstants(Array<Identifier>& ids) const
{
	for (const auto& p : obj->getProperties())
	{
		if (p.value.isMethod())
			continue;

		ids.add(p.name);
	}
}

const var DynamicDebugableObjectWrapper::getConstantValue(int index) const
{ return obj->getProperties().getValueAt(index); }

void DebugInformationBase::doubleClickCallback(const MouseEvent& e, Component* componentToNotify)
{
	if (auto obj = getObject())
		getObject()->doubleClickCallback(e, componentToNotify);
}

int DebugInformationBase::getType() const
{ 
	if (auto obj = getObject())
		return obj->getTypeNumber();

	return 0; 
}

int DebugInformationBase::getNumChildElements() const
{ 
	if (auto obj = getObject())
	{
		auto numCustom = obj->getNumChildElements();

		if (numCustom != -1)
			return numCustom;
	}

	return 0; 
}

DebugInformationBase::Ptr DebugInformationBase::getChildElement(int index)
{ 
	if (auto obj = getObject())
	{
		return obj->getChildElement(index);

	}
	return nullptr; 
}

String DebugInformationBase::getTextForName() const
{
	if (auto obj = getObject())
		return obj->getDebugName();

	return "undefined";
}

String DebugInformationBase::getCategory() const
{ 
	if (auto obj = getObject())
		return obj->getCategory();

	return ""; 
}

DebugableObjectBase::Location DebugInformationBase::getLocation() const
{
	if (auto obj = getObject())
		return obj->getLocation();

	return DebugableObjectBase::Location();
}

String DebugInformationBase::getTextForType() const
{ return "unknown"; }

String DebugInformationBase::getTextForDataType() const
{ 
	if (auto obj = getObject()) 
		return obj->getDebugDataType(); 

	return "undefined";
}

String DebugInformationBase::getTextForValue() const
{
	if (auto obj = getObject())
		return obj->getDebugValue();

	return "empty";
}

bool DebugInformationBase::isWatchable() const
{ 
	if (auto obj = getObject())
		return obj->isWatchable();

	return true; 
}

bool DebugInformationBase::isAutocompleteable() const
{
	if (auto obj = getObject())
		return obj->isAutocompleteable();

	return true;
}

String DebugInformationBase::getCodeToInsert() const
{ 
	return ""; 
}

AttributedString DebugInformationBase::getDescription() const
{
	if (auto obj = getObject())
		return obj->getDescription();

	return AttributedString();
}

DebugableObjectBase* DebugInformationBase::getObject()
{ return nullptr; }

const DebugableObjectBase* DebugInformationBase::getObject() const
{ return nullptr; }

DebugInformationBase::~DebugInformationBase()
{}

String DebugInformationBase::replaceParentWildcard(const String& id, const String& parentId)
{
	static const String pWildcard = "%PARENT%";

	if (id.contains(pWildcard))
	{
		String s;
		s << parentId << id.fromLastOccurrenceOf(pWildcard, false, false);
		return s;
	}

	return id;
}

String DebugInformationBase::getVarType(const var& v)
{
	if (v.isUndefined())	return "undefined";
	else if (v.isArray())	return "Array";
	else if (v.isBool())	return "bool";
	else if (v.isInt() ||
		v.isInt64())	return "int";
	else if (v.isBuffer()) return "Buffer";
	else if (v.isObject())
	{
		if (auto d = dynamic_cast<DebugableObjectBase*>(v.getObject()))
		{
			return d->getDebugDataType();
		}
		else return "Object";
	}
	else if (v.isDouble()) return "double";
	else if (v.isString()) return "String";
	else if (v.isMethod()) return "function";

	return "undefined";
}

StringArray DebugInformationBase::createTextArray() const
{
	StringArray sa;

	sa.add(getTextForType());
	sa.add(getTextForDataType());
	sa.add(getTextForName());
	sa.add(getTextForValue());

	return sa;
}

Component* DebugInformationBase::createPopupComponent(const MouseEvent& e, Component* componentToNotify)
{
	if (auto obj = getObject())
	{
		obj->setCurrentExpression(getCodeToInsert());
		return getObject()->createPopupComponent(e, componentToNotify);
	}
		

	return nullptr;
}

ComponentForDebugInformation::ComponentForDebugInformation(DebugableObjectBase* obj_, ApiProviderBase::Holder* h) :
	holder(h),
	obj(obj_)
{
	expression = obj->currentExpression;
	jassert(expression.isNotEmpty());
}

void ComponentForDebugInformation::search()
{
	if (obj == nullptr)
	{
		if (holder == nullptr)
			return;

		ScopedReadLock sl(holder->getDebugLock());

		auto provider = holder->getProviderBase();

		if (provider == nullptr)
			return;

		for (int i = 0; i < provider->getNumDebugObjects(); i++)
		{
			if (searchRecursive(provider->getDebugInformation(i).get()))
				break;
		}
	}
}

bool ComponentForDebugInformation::searchRecursive(DebugInformationBase* b)
{
	if (b == nullptr)
		return false;

	if (holder->shouldReleaseDebugLock())
		return true;

	if (b->getCodeToInsert() == expression)
	{
		obj = b->getObject();
		refresh();
		return true;
	}

	for (int i = 0; i < b->getNumChildElements(); i++)
	{
		if (searchRecursive(b->getChildElement(i).get()))
			return true;
	}

	return false;
}

String ComponentForDebugInformation::getTitle() const
{
	String s;

	if (obj != nullptr)
		s << obj->getDebugName() << ": ";

	s << expression;
	return s;
}

} // namespace hise
