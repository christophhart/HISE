

ApiCollection::ApiCollection(BaseDebugArea *area) :
SearchableListComponent(area),
parentArea(area),
apiTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize))
{
	setName("API Browser");

	setFuzzyness(0.6);
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

	help.setJustification(Justification::topLeft);
	help.setWordWrap(AttributedString::byWord);
	help.setLineSpacing(1.5f);

	help.append("Name:\n  ", GLOBAL_BOLD_FONT(), Colours::white);
	help.append(name, GLOBAL_MONOSPACE_FONT(), Colours::white.withAlpha(0.8f));
	help.append(arguments + "\n\n", GLOBAL_MONOSPACE_FONT(), Colours::white.withAlpha(0.6f));
	help.append("Description:\n  ", GLOBAL_BOLD_FONT(), Colours::white);
	help.append(description + "\n\n", GLOBAL_FONT(), Colours::white.withAlpha(0.8f));

	help.append("Return Type:\n  ", GLOBAL_BOLD_FONT(), Colours::white);
	help.append(methodTree.getProperty("returnType", "void"), GLOBAL_MONOSPACE_FONT(), Colours::white.withAlpha(0.8f));
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

	g.setColour(isMouseOver() ? Colours::white : c.withAlpha(0.5f));

	g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f, 2.0f);

	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(name, 10, 0, getWidth() - 20, ITEM_HEIGHT, Justification::centredLeft, true);

}

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
