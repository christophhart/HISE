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

ScriptComponentList::ScriptComponentItem::ScriptComponentItem(ScriptComponent* c_) :
	Item(c_->getName().toString().toLowerCase()),
	c(c_)
{
	setOpaque(true);

	c->addChangeListener(this);

	Colour co = Colours::white.withAlpha(c->isShowing() ? 1.0f : 0.4f);

	s.setJustification(Justification::centredLeft);

	s.append(c->getName().toString(), GLOBAL_BOLD_FONT(), co);

	co = co.withMultipliedBrightness(0.4f);

	String x = " (" + c->getObjectName().toString() + ")";
	x = x.replace("Scripting", "");
	x = x.replace("Scripted", "");
	x = x.replace("Script", "");


	s.append(x, GLOBAL_FONT(), co);

	setSize(380 - 16 - 8 - 24, ITEM_HEIGHT);
}

ScriptComponentList::ScriptComponentItem::~ScriptComponentItem()
{
	c->removeChangeListener(this);
	c = nullptr;
}

void ScriptComponentList::ScriptComponentItem::changeListenerCallback(SafeChangeBroadcaster* b)
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

	co = b->isSelected(c) ? Colour(SIGNAL_COLOUR) : Colour(0xFF999999);

	co = co.withAlpha(isMouseOver(true) ? 1.0f : 0.6f);

	g.setColour(co);

	g.drawRect(2, 2, (int)w, (int)h - 4, 1);

	
    
    s.draw(g, Rectangle<float>(10.0f, 0.0f, (float)getWidth(), (float)ITEM_HEIGHT));
    
	
}

void ScriptComponentList::ScriptComponentItem::mouseDown(const MouseEvent& event)
{
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
			String s;
			s << "const var " << c->name.toString() << " = Content.getComponent(\"" << c->name.toString() << "\");";
			SystemClipboard::copyTextToClipboard(s);
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
		default:
			break;
		}
		
	}
	else
	{
		auto l = findParentComponentOfClass<ScriptComponentList>();
		auto b = l->getScriptComponentEditBroadcaster();

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
