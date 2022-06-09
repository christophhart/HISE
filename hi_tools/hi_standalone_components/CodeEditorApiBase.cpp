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
