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

ApiCollection::ApiCollection(BaseDebugArea *area) :
SearchableListComponent(area),
parentArea(area),
apiTree(ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize)))
{
	setName("API Browser");

	setFuzzyness(0.6);
}


String ApiHelpers::createCodeToInsert(const ValueTree &method, const String &className)
{
	const String name = method.getProperty(Identifier("name")).toString();
	const String arguments = method.getProperty(Identifier("arguments")).toString();

	return String(className + "." + name + arguments);
}


AttributedString ApiHelpers::createAttributedStringFromApi(const ValueTree &method, const String &/*className*/, bool multiLine, Colour textColour)
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
		
		const String returnType = method.getProperty("returnType", "");

		if (returnType.isNotEmpty())
		{
			help.append("\nReturn Type: ", GLOBAL_BOLD_FONT(), textColour);
			help.append(returnType, GLOBAL_MONOSPACE_FONT(), textColour.withAlpha(0.8f));
		}
	}

	return help;
}


ApiCollection::MethodItem::MethodItem(const ValueTree &methodTree_, const String &className_) :
Item(String(className_ + "." + methodTree_.getProperty(Identifier("name")).toString()).toLowerCase()),
methodTree(methodTree_),
name(methodTree_.getProperty(Identifier("name")).toString()),
description(methodTree_.getProperty(Identifier("description")).toString()),
arguments(methodTree_.getProperty(Identifier("arguments")).toString()),
className(className_)
{
	setSize(380 - 16 - 8 - 24, ITEM_HEIGHT);

	help = ApiHelpers::createAttributedStringFromApi(methodTree, className, true, Colours::white);
}


void ApiCollection::MethodItem::mouseDoubleClick(const MouseEvent&)
{
	insertIntoCodeEditor();
}

bool ApiCollection::MethodItem::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::returnKey))
	{
		insertIntoCodeEditor();
		return true;
	}

	return false;
}

void ApiCollection::MethodItem::insertIntoCodeEditor()
{
	ApiCollection *parent = findParentComponentOfClass<ApiCollection>();

	parent->parentArea->findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()->getMainController()->insertStringAtLastActiveEditor(className + "." + name + arguments, arguments != "()");
}

void ApiCollection::MethodItem::paint(Graphics& g)
{

	Colour c(0xFF000000);

	const float h = (float)getHeight();
	const float w = (float)getWidth() - 4;

	ColourGradient grad(c.withAlpha(0.1f), 0.0f, .0f, c.withAlpha(0.2f), 0.0f, h, false);

	g.setGradientFill(grad);

	g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f);

	g.setColour(isMouseOver(true) ? Colours::white : c.withAlpha(0.5f));

	g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f, 2.0f);

	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(name, 10, 0, getWidth() - 20, ITEM_HEIGHT, Justification::centredLeft, true);

}

bool JavascriptCodeEditor::AutoCompletePopup::apiRowsInitialised = false;
JavascriptCodeEditor::AutoCompletePopup::ApiRows JavascriptCodeEditor::AutoCompletePopup::apiRows(false);

ApiCollection::ClassCollection::ClassCollection(const ValueTree &api) :
classApi(api),
name(api.getType().toString())
{
	for (int i = 0; i < api.getNumChildren(); i++)
	{
		items.add(new MethodItem(api.getChild(i), name));

		addAndMakeVisible(items.getLast());
	}
}

void ApiCollection::ClassCollection::paint(Graphics &g)
{
	g.setColour(Colours::white.withAlpha(0.9f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(name, 10, 0, getWidth() - 10, COLLECTION_HEIGHT, Justification::centredLeft, true);
}
