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
using namespace juce;

namespace raw
{

void addTabs(String& s, int numTabs)
{
	for (int i = 0; i < numTabs; i++)
		s << " ";
}

Positioner::Data::Data(var sc)
{
	auto component = dynamic_cast<ScriptComponent*>(sc.getObject());

	if (component == nullptr)
		return;

	name = component->getName().toString();

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier w("width");
	static const Identifier h("height");

	bounds = component->getPosition();
	
	auto componentProperties = component->getPropertyValueTree();
	auto content = component->getScriptProcessor()->getScriptingContent();

	children.reserve(componentProperties.getNumChildren());

	for (const auto& c : componentProperties)
	{
		auto id = Identifier(c.getProperty("id").toString());
		auto sc = content->getComponentWithName(id);
		children.push_back(Data(sc));
	}
}

juce::String Positioner::Data::toString(int currentTabLevel) const
{
	bool isOneLine = children.empty();

	String s;
	NewLine nl;
	String tab = "\t";

	addTabs(s, currentTabLevel);

	if (isOneLine)
		s << "{ ";
	else
	{
		s << "{" << nl;
		currentTabLevel++;
		addTabs(s, currentTabLevel);
	}

	s << "\"" << name << "\", { ";
	s << bounds.getX() << ", ";
	s << bounds.getY() << ", ";
	s << bounds.getWidth() << ", ";
	s << bounds.getHeight() << " }, ";

	if(isOneLine)
		s << "{} ";
	else
	{
		s << nl;
		addTabs(s, currentTabLevel);

		s << "{" << nl;
		
		currentTabLevel++;

		for (int i = 0; i < children.size(); i++)
		{
			const auto& c = children[i];

			s << c.toString(currentTabLevel);
			bool isLast = i == children.size()-1;
			
			if (!isLast)
				s << ",";
			
			s << nl;
		}

		currentTabLevel--;
		addTabs(s, currentTabLevel);
		s << "}" << nl;

		currentTabLevel--;
		addTabs(s, currentTabLevel);
	}
	
	s << "}";

	return s;
}

void Positioner::Data::apply(Component& c, StringArray& processedComponents) const
{
	// If you hit this, you must ensure that your root component has the same name as the positioning data.
	jassert(c.getName() == name);

	if(!bounds.isEmpty())
		c.setBounds(bounds);

	processedComponents.add(name);

	for (int i = 0; i < c.getNumChildComponents(); i++)
	{
		auto child = c.getChildComponent(i);

		auto n = child->getName();

		for (const auto& ch : children)
		{
			if (ch.name == n)
				ch.apply(*child, processedComponents);
		}
	}
}

void Positioner::Data::fillNameList(StringArray& list) const
{
	list.add(name);

	for (const auto& c : children)
		c.fillNameList(list);
}

Positioner::Positioner(const Data& d) :
	data(d)
{
}

Positioner::Positioner(var component) :
	data(component)
{

}

juce::String Positioner::toString()
{
	return data.toString(0);
}

void Positioner::apply(Component& c)
{

	data.apply(c, processedComponents);
}

void Positioner::applyToChildren(Component& c)
{
	auto originBounds = data.bounds;
	data.bounds = {};
	data.apply(c, processedComponents);
	data.bounds = originBounds;
}

void Positioner::printSummary()
{
#if JUCE_DEBUG
	StringArray unprocessedComponents;
	data.fillNameList(unprocessedComponents);

	for (const auto& p : processedComponents)
		unprocessedComponents.removeString(p);

	DBG("Positioner Statistic:");
	DBG("-------------------------------");
	DBG("  Found:");

	for (const auto& p : processedComponents)
		DBG("    - " + p);

	DBG("  Not Found:");

	for (const auto& p : unprocessedComponents)
		DBG("    - " + p);
#endif
}

}

}