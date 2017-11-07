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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

ScriptComponentList::ScriptComponentList(BackendRootWindow* window, JavascriptProcessor* p) :
	SearchableListComponent(window),
	ScriptComponentEditListener(dynamic_cast<Processor*>(p)),
	jp(p)
{
	hideButton = new ShapeButton("hide", Colours::white.withAlpha(0.5f), Colours::white, Colours::white);


	addAndMakeVisible(hideButton);
	Path hidePath;

	hidePath.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, sizeof(BackendBinaryData::ToolbarIcons::viewPanel));
	hideButton->setShape(hidePath, false, true, true);
	hideButton->addListener(this);

	addCustomButton(hideButton);

	window->getBackendProcessor()->addScriptListener(this);
	addAsScriptEditListener();

	setOpaque(true);
	setName("Script Component Browser");

	setFuzzyness(0.6);
}

ScriptComponentList::~ScriptComponentList()
{
	dynamic_cast<Processor*>(jp)->getMainController()->removeScriptListener(this);
	removeAsScriptEditListener();
}

void ScriptComponentList::buttonClicked(Button* b)
{
	showOnlyVisibleItems = !b->getToggleState();

	b->setToggleState(showOnlyVisibleItems, dontSendNotification);

	Colour c = showOnlyVisibleItems ? Colour(SIGNAL_COLOUR) : Colours::white;
	
	hideButton->setColours(c.withAlpha(0.5f), c, c);

	rebuildModuleList(true);
}

void getChildDepth(ScriptComponent* sc, int& depth)
{
	int index = sc->getParentComponentIndex();

	if (index != -1)
	{
		auto child = sc->parent->getComponent(index);

		if (depth > 32)
		{
			jassertfalse;
			depth = 0;
			return;
		}

		if (child != nullptr)
		{
			depth++;
			getChildDepth(child, depth);
		}
	}
}


void getSearchTermInternal(ScriptComponent* c, String& term)
{
	term << c->getName().toString();

	int parentIndex = c->getParentComponentIndex();

	if (parentIndex != -1)
	{
		auto parent = c->parent->getComponent(parentIndex);

		getSearchTermInternal(parent, term);
	}
}


String getSearchTerm(ScriptComponent* c)
{
	String term;

	getSearchTermInternal(c, term);

	return term.toLowerCase();

}


ScriptComponentList::ScriptComponentItem::ScriptComponentItem(ScriptComponent* c_) :
	Item(getSearchTerm(c_)),
	c(c_)
{
	setOpaque(true);

	c->addChangeListener(this);

	id = c->getName().toString();

	typeOffset = GLOBAL_BOLD_FONT().getStringWidthFloat(id) + 5.0f;

	typeName = " (" + c->getObjectName().toString() + ")";
	typeName = typeName.replace("Scripting", "");
	typeName = typeName.replace("Scripted", "");
	typeName = typeName.replace("Script", "");

	getChildDepth(c, childDepth);
	
	setSize(380 - 16 - 8 - 24, ITEM_HEIGHT);
}

ScriptComponentList::ScriptComponentItem::~ScriptComponentItem()
{
	c->removeChangeListener(this);
	c = nullptr;
}

void ScriptComponentList::ScriptComponentItem::changeListenerCallback(SafeChangeBroadcaster* /*b*/)
{
	repaint();
}

void ScriptComponentList::ScriptComponentItem::mouseEnter(const MouseEvent&)
{
	repaint();
}

void ScriptComponentList::ScriptComponentItem::mouseExit(const MouseEvent&)
{
	repaint();
}



void ScriptComponentList::ScriptComponentItem::mouseDoubleClick(const MouseEvent&)
{
	ScriptingApi::Content::Helpers::gotoLocation(c);
}



bool ScriptComponentList::ScriptComponentItem::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	if (auto ar = dragSourceDetails.description.getArray())
	{
		var d(c);
		return !ar->contains(d);
	}

	return false;
}

void ScriptComponentList::ScriptComponentItem::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
	insertDragAfterComponent = false;
	insertDragAsParentComponent = false;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDragMove(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	repaint();
}

void ScriptComponentList::ScriptComponentItem::itemDropped(const SourceDetails& dragSourceDetails)
{
	insertDragAfterComponent = dragSourceDetails.localPosition.getY() > (ITEM_HEIGHT / 2);
	insertDragAsParentComponent = !insertDragAfterComponent;

	ScriptingApi::Content::Helpers::moveComponents(c, dragSourceDetails.description, insertDragAsParentComponent);

	insertDragAfterComponent = false;
	insertDragAsParentComponent = false;

	repaint();
}


void ScriptComponentList::ScriptComponentItem::mouseUp(const MouseEvent& event)
{
	if (!event.mods.isRightButtonDown() && event.mouseWasDraggedSinceMouseDown())
		return;

	auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();



	if (event.mods.isRightButtonDown())
	{
		enum PopupMenuOptions
		{
			DeleteComponent = 1,
			DeleteSelection,
			RenameComponent,
			CreateScriptVariableDeclaration,
			CreateCustomCallbackDefinition,
			CopyProperties,
			PasteProperties,
			numOptions
		};

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(PopupMenuOptions::DeleteComponent, "Delete Component", true);
		m.addItem(PopupMenuOptions::RenameComponent, "Rename Component", true);
		m.addItem(PopupMenuOptions::DeleteSelection, "Delete selected Components", b->getSelection().size() > 0);
		m.addItem(PopupMenuOptions::CreateScriptVariableDeclaration, "Create script variable definition");
		m.addItem(PopupMenuOptions::CreateCustomCallbackDefinition, "Create custom callback definition");
		m.addItem(PopupMenuOptions::CreateCustomCallbackDefinition, "Create custom callback definition");

		auto clipboardData = JSON::parse(SystemClipboard::getTextFromClipboard());

		m.addItem(PopupMenuOptions::CopyProperties, "Copy properties");
		m.addItem(PopupMenuOptions::PasteProperties, "Paste properties to selection", clipboardData.isObject());

		const PopupMenuOptions result = (PopupMenuOptions)m.show();

		switch (result)
		{
		case DeleteComponent:
		{
			ScriptingApi::Content::Helpers::deleteComponent(c->parent, c->name);
			break;
		}
		case DeleteSelection:
		{
			ScriptingApi::Content::Helpers::deleteSelection(c->parent, b);
			break;
		}
		case RenameComponent:
		{
			auto newId = Identifier(PresetHandler::getCustomName("ID"));

			if (c->parent->getComponentWithName(newId) == nullptr)
			{
				ScriptingApi::Content::Helpers::renameComponent(c->parent, c->name, newId);
			}
			break;
		}
		case CreateScriptVariableDeclaration:
		{
			auto st = ScriptingApi::Content::Helpers::createScriptVariableDeclaration(b->getSelection());

			SystemClipboard::copyTextToClipboard(st);
			break;
		}
		case CreateCustomCallbackDefinition:
		{
			auto name = c->getName();

			NewLine nl;
			String code;


			String callbackName = "on" + name.toString() + "Control";

			code << nl;
			code << "inline function " << callbackName << "(component, value)" << nl;
			code << "{" << nl;
			code << "\t//Add your custom logic here..." << nl;
			code << "};" << nl;
			code << nl;
			code << name.toString() << ".setControlCallback(" << callbackName << ");" << nl;

			SystemClipboard::copyTextToClipboard(code);

		}
		case CopyProperties:
		{
			SystemClipboard::copyTextToClipboard(c->getScriptObjectPropertiesAsJSON());
		}
		case PasteProperties:
		{
			ScriptingApi::Content::Helpers::pasteProperties(b->getSelection(), clipboardData);
		}
		default:
			break;
		}

	}
	else
	{
		auto l = findParentComponentOfClass<ScriptComponentList>();
		
		if (event.mods.isShiftDown() && l->lastClickedComponent != nullptr)
		{
			auto content = c->parent;

			const int index1 = content->getComponentIndex(c->name);
			const int index2 = content->getComponentIndex(l->lastClickedComponent->name);

			const int start = jmin<int>(index1, index2);
			const int end = jmax<int>(index1, index2);

			for (int i = start; i <= end; i++)
			{
				bool isLast = (i == end);
				b->addToSelection(content->getComponent(i), isLast ? sendNotification : dontSendNotification);
			}

			l->lastClickedComponent = c;
		}
		else
		{


			b->updateSelectionBasedOnModifier(c, event.mods, sendNotification);
			repaint();
		}

		l->lastClickedComponent = c;

	}
}

void ScriptComponentList::ScriptComponentItem::mouseDrag(const MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
		return;

	auto b = findParentComponentOfClass<ScriptComponentEditListener>()->getScriptComponentEditBroadcaster();

	auto container = DragAndDropContainer::findParentDragContainerFor(this);

	if (container->isDragAndDropActive())
		return;

	if (event.mouseWasDraggedSinceMouseDown())
	{
		b->prepareSelectionForDragging(c);

		ScriptComponentEditBroadcaster::Iterator iter(b);

		Array<var> list;

		while (auto sc = iter.getNextScriptComponent())
		{
			list.add(var(sc));
		}

		container->startDragging(list, this);
		repaint();
	}
}




void ScriptComponentList::ScriptComponentItem::paint(Graphics& g)
{
	if (getWidth() <= 0)
		return;

	g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));

	Colour co(0xFF000000);

	const float h = (float)getHeight();
	const float w = (float)getWidth() - 4;

	ColourGradient grad(co.withAlpha(0.1f), 0.0f, .0f, co.withAlpha(0.2f), 0.0f, h, false);

	g.setGradientFill(grad);

	g.fillRect(2, 2, (int)w - 4, (int)h - 4);

	auto b = findParentComponentOfClass<ScriptComponentList>()->getScriptComponentEditBroadcaster();

	co = b->isSelected(c) ? Colour(SIGNAL_COLOUR) : Colour(0xFF666666);

	co = co.withAlpha((!isMouseButtonDown() && isMouseOver(true)) ? 1.0f : 0.3f);

	g.setColour(co);

	g.drawRect(2, 2, (int)w, (int)h - 4, 1);
    
	g.setColour(Colours::black.withAlpha(0.1f));

	g.fillRect(3, 3, (int)h - 6, (int)h - 6);

	static const Identifier sip("saveInPreset");

	const bool saveInPreset = c->getScriptObjectProperties()->getProperty(sip);
	
	Colour c3 = saveInPreset ? Colours::green : Colours::red;

	c3 = c3.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.3f));

	g.setColour(c3);

	const float offset = JUCE_LIVE_CONSTANT_OFF(8.0f);
	Rectangle<float> circle(offset, offset, (float)ITEM_HEIGHT - 2.0f * offset, (float)ITEM_HEIGHT - 2.0f * offset);

	g.fillEllipse(circle);

	g.drawEllipse(circle, 1.0f);

	g.setOpacity(c->isShowing() ? 1.0f : 0.4f);

	Colour textColour = Colours::white.withAlpha(c->isShowing() ? 1.0f : 0.4f);

	g.setColour(textColour);
	g.setFont(GLOBAL_BOLD_FONT());
	
	float textOffset = h + 2.0f + (float)(childDepth * 10);
	
	g.drawText(id, (int)textOffset, 0, (int)typeOffset, ITEM_HEIGHT, Justification::centredLeft);

	g.setColour(textColour.withMultipliedBrightness(0.4f));
	g.setFont(GLOBAL_FONT());

	g.drawText(typeName, (int)(textOffset + typeOffset), 0, getWidth() - (int)(textOffset + typeOffset), ITEM_HEIGHT, Justification::centredLeft);

	if (insertDragAfterComponent)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRect(0, getHeight() - 3, getWidth(), 3);
	}
	else if (insertDragAsParentComponent)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));
		g.fillRect(2, 2, (int)w - 4, (int)h - 4);
	}


}


ScriptComponentList::AllCollection::AllCollection(JavascriptProcessor* p, bool showOnlyVisibleItems)
{
	auto content = p->getContent();

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (showOnlyVisibleItems && !content->getComponent(i)->isShowing())
			continue;

		items.add(new ScriptComponentItem(content->getComponent(i)));
		addAndMakeVisible(items.getLast());
	}
}

Component* ScriptComponentList::Panel::createContentComponent(int /*index*/)
{
	return new ScriptComponentList(getRootWindow(), dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()));
}
